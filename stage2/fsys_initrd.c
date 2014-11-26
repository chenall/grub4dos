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

static grub_u64_t initrdfs_base;
static grub_u64_t initrdfs_size;
static grub_u32_t initrdfs_type;
static struct initrdfs_file cur_file;
static grub_u32_t path_len;
static grub_u64_t char2u64(char *node)
{
	char buf[11]="0x";
	grub_u64_t tmp;
	memcpy(&buf[2],node,8);
	char *p=buf;
	if (!safe_parse_maxint(&p,&tmp))
		return 0;
	return tmp;
}

static int test_file(const char *dirname)
{
	if (print_possibilities)
	{
		if (substring (dirname,(char*)cur_file.name, 1) <= 0)
		{
			print_a_completion ((char*)cur_file.name + path_len, 1);
			return 1;
		}
	}
	else if (substring (dirname, (char*)cur_file.name, 1) == 0)
	{
		filemax = cur_file.size;
		return 2;
	}
	return 0;
}

static grub_u32_t cpio_file(struct cpio_header *hdr)
{
	grub_u64_t namesize;
	grub_u16_t hdr_sz;
	cur_file.isdir = char2u64(hdr->c_mode);
	namesize = char2u64(hdr->c_namesize);
	cur_file.size = char2u64(hdr->c_filesize);
	if (errnum || cur_file.isdir == 0)
	{
		errnum = 0;
		return 0;
	}
	hdr_sz = cpio_image_align(sizeof(struct cpio_header) + namesize);
	cur_file.base = (grub_u32_t)hdr + hdr_sz;
	cur_file.name = (grub_u32_t)hdr + sizeof(struct cpio_header);
	cur_file.isdir &= CPIO_MODE_DIR;
	cur_file.name_size = namesize;

	return cpio_image_align(hdr_sz + cur_file.size);
}

static int cpio_dir(const char* dirname)
{
	struct cpio_header *p_cpio_hdr;
	int found = 0;
	grub_u64_t tmp_pos = 0;
	grub_u64_t cur_pos = 0;
	grub_u32_t node_size = 0;

	while(cur_pos < initrdfs_size)
	{
		p_cpio_hdr = (struct cpio_header *)(grub_u32_t)(initrdfs_base + cur_pos);
		if (*(grub_u32_t*)p_cpio_hdr->c_magic != 0x37303730 || p_cpio_hdr->c_magic[5] == 0x30)//07070
		{
			if (node_size)
				cur_pos = tmp_pos + ((node_size + 0xFFF) & ~0xFFF);
			else
				cur_pos += 4096;
			node_size = 0;
			continue;
		}
		node_size = cpio_file(p_cpio_hdr);
		if (node_size == 0 || node_size == -1)
			break;
		switch(test_file(dirname))
		{
			case 2:
				return 2;
			case 1:
				found = 1;
				break;
		}
		tmp_pos = cur_pos;
		cur_pos += node_size;
	}

	return found;
}

int initrdfs_mount (void)
{
	int i;

	if (current_drive == ram_drive)
	{
		initrdfs_base = rd_base;
		initrdfs_size = rd_size;
	}
	else if (current_drive == 0xFFFF)
	{
		initrdfs_base = md_part_base;
		initrdfs_size = md_part_size;
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

	switch(*(grub_u32_t*)(grub_u32_t)initrdfs_base)
	{
		case BAT_SIGN:
			initrdfs_type = 1;
			break;
		case 0x37303730:
			initrdfs_type = 2;
			break;
		default:
			return 0;
	}

	return 1;
}


int initrdfs_dir (char *dirname)
{
	grub_u32_t found = 0;
	char *dirpath = dirname + grub_strlen(dirname);

	while (*dirname == '/')
		dirname++;

	while(dirpath != dirname && *dirpath !='/')
		--dirpath;

	path_len = dirpath - dirname;
	if (*dirpath == '/') ++path_len;

	if (initrdfs_type == 1)
	{
		grub_u16_t filename = '0';
		grub_u32_t pos=0;
		grub_u32_t test = 0;
		cur_file.name = (grub_u32_t)&filename;
		cur_file.isdir = 0;
		cur_file.name_size = 2;

		while(pos < initrdfs_size)
		{
			cur_file.base = initrdfs_base + pos;
			cur_file.size = (*(grub_u32_t*)(grub_u32_t)cur_file.base == BAT_SIGN)?grub_strlen((char*)cur_file.base):initrdfs_size - pos;
			test = test_file(dirname);
			if (test) found = 1;
			if (test == 2) break;
			++filename;
			pos += cur_file.size + 1;
		}
	}
	else
		found = cpio_dir(dirname);

	return found;
}

unsigned long long initrdfs_read (unsigned long long buf, unsigned long long len, unsigned long write)
{
	if (filepos > cur_file.size) return 0;
	if (len > cur_file.size - filepos) len = cur_file.size - filepos;

	grub_u64_t file_base = cur_file.base + filepos;

	if (disk_read_hook)
		disk_read_hook(file_base >>  SECTOR_BITS,file_base & (SECTOR_SIZE - 1),len);

	if (write == GRUB_WRITE) grub_memmove64(file_base,buf,len);
	else if (write == GRUB_READ && buf) grub_memmove64(buf,file_base,len);

	filepos += len;

	return len;
}

void initrdfs_close (void)
{

}

#endif /* FSYS_INITRD */