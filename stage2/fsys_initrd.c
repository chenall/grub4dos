/*
 *  initrd file system for grub4dos
 *
 *  Copyright (C) 2014 chenall (chenall.cn@gmail.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef FSYS_INITRD

#include "shared.h"
#include "filesys.h"
#include "fsys_initrd.h"

#define FILE_INFO

static grub_u64_t initrdfs_base;
static grub_u64_t initrdfs_size;

struct initrdfs_file *p_fnode = (struct initrdfs_file*)FSYS_BUF;
struct initrdfs_file *cur_file = (struct initrdfs_file*)FSYS_BUF;

grub_u64_t char2u64(char *node)
{
	char buf[11]="0x";
	grub_u64_t tmp;
	memcpy(&buf[2],node,8);
	char *p=buf;
	if (!safe_parse_maxint(&p,&tmp))
		return 0;
	return tmp;
}

grub_u32_t raw_file(grub_u64_t base,struct initrdfs_file *p_fn,grub_u32_t max_size)
{
	grub_u32_t size = 0;
	p_fn->isdir = 0;
	p_fn->base = base;
	p_fn->name = -1LL;

	while(size < max_size)
	{
		size += 4096;//4KB align
		char *p = (grub_u32_t)(base + size);
		if (*(grub_u32_t*)p == 0x54414221UL)//!BAT
			break;
		if (*(grub_u16_t*)p == 0x8B1F)//GZIP
			break;
		if (*(p-1) == '\0')
		{
			grub_u32_t *t = (grub_u32_t*)p;
			while(*t == 0)
			{
				if ((grub_u32_t)(++t) - (grub_u32_t)p >= 4096)
					break;
			}
			if (*t) break;
		}
	}
	p_fn->size = size;
	return size;
}

grub_u32_t cpio_file(grub_u64_t base,struct initrdfs_file *p_fn)
{
	grub_u64_t namesize;
	grub_u16_t hdr_sz;
	struct cpio_header *p_cpio;
	p_cpio =(struct cpio_header *)(grub_u32_t)base;
	p_fn->isdir = char2u64(p_cpio->c_mode);
	namesize = char2u64(p_cpio->c_namesize);
	p_fn->size = char2u64(p_cpio->c_filesize);
	if (errnum || p_fn->isdir == 0)
	{
		errnum = 0;
		return 0;
	}
	p_fn->name=base + sizeof(struct cpio_header);
	hdr_sz = cpio_image_align(sizeof(struct cpio_header) + namesize);
	p_fn->isdir &= CPIO_MODE_DIR;
	p_fn->base = base + hdr_sz;
	return cpio_image_align(hdr_sz + p_fn->size);
}

int cpio_dir(void)
{
	struct initrdfs_file *p_fn = p_fnode;
	grub_u64_t cpio_base = initrdfs_base;
	grub_u64_t cpio_end = cpio_base + initrdfs_size;
	grub_u32_t node_size = 0;
	p_fnode->base = 0ll;

	while(cpio_base < cpio_end)
	{
		grub_u16_t c_flag = *(grub_u16_t*)(grub_u32_t)(cpio_base+4);
		if (*(grub_u32_t*)(grub_u32_t)cpio_base == 0x37303730 && (c_flag == 0x3130 || c_flag == 0x3230))
			node_size = cpio_file(cpio_base,p_fn);
		else
			node_size = raw_file(cpio_base,p_fn,cpio_end - cpio_base);

		if (node_size == 0 || node_size == -1)
			break;
		cpio_base += node_size;
		if (*(grub_u32_t*)(grub_u32_t)cpio_base != 0x37303730)
		{
			cpio_base += (node_size + 0xFFF) & ~0xFFF;
			cpio_base -= node_size;
		}
		++p_fn;
	}
	p_fn->base=0;
	return p_fnode->base;
}

int initrdfs_mount (void)
{
	int i;

	initrdfs_base = 0;
	initrdfs_size = 0;
	if (current_drive == ram_drive)
	{
		initrdfs_base = rd_base;
		initrdfs_size = rd_size;
	}
	else
	{
		if (unset_int13_handler (1))
			return 0;
		for (i = 0; i < DRIVE_MAP_SIZE && !drive_map_slot_empty (hooked_drive_map[i]); i++)
		{
			if (hooked_drive_map[i].from_drive == (unsigned char)current_drive)
			{
				if (hooked_drive_map[i].to_drive == 0xFF)
				{
					initrdfs_base = (grub_u64_t)hooked_drive_map[i].start_sector << 9;
					initrdfs_size = (grub_u64_t)hooked_drive_map[i].sector_count << 9;
				}
				break;
			}
		}
	}

	if (initrdfs_base == 0) return 0;

	return cpio_dir();
}

int initrdfs_dir (char *dirname)
{
	grub_u32_t found = 0;
	char raw_name[9];
	char *dirpath = dirname + grub_strlen(dirname);
	grub_u32_t path_len;
	cur_file = p_fnode;

	while (*dirname == '/')
		dirname++;

	while(dirpath != dirname && *dirpath !='/')
		--dirpath;

	path_len = dirpath - dirname;
	if (*dirpath == '/') ++path_len;

	while(cur_file->base)
	{
		char *filename;
		if (cur_file->name != -1)
			filename = (char*)(grub_u32_t)cur_file->name;
		else
		{
			sprintf(raw_name,"%08X",(grub_u32_t)cur_file->base);
			filename = raw_name;
		}

		if (print_possibilities)
		{
			if (substring (dirname,filename, 1) <= 0/* && grub_strstr(filename + path_len,"/") == 0*/)
			{
				found = 1;
				print_a_completion (filename + path_len, 1);
			}
		}
		else if (substring (dirname, filename, 1) == 0)
		{
			found = 1;
			filemax = cur_file->size;
			break;
		}
		++cur_file;
	}

	if (!found) errnum = ERR_FILE_NOT_FOUND;

	return found;
}

unsigned long long initrdfs_read (unsigned long long buf, unsigned long long len, unsigned long write)
{
	if (disk_read_hook && debug > 1)
		printf("<B:%ld,S:%ld,P:%ld>\n",cur_file->base,cur_file->size,filepos);
	if (filepos > cur_file->size) return 0;
	if (len > cur_file->size - filepos) len = cur_file->size - filepos;
	if (write == GRUB_WRITE) grub_memmove64(cur_file->base + filepos,buf,len);
	if (write == GRUB_READ && buf) grub_memmove64(buf,cur_file->base + filepos,len);

	filepos += len;
	return len;
}

#endif /* FSYS_INITRD */