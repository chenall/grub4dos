/*
 *  FB file system for GRUB
 *
 *  Copyright (C) 2009 Bean (bean123ch@gmail.com)
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
#ifdef FSYS_FB

#include "shared.h"
#include "filesys.h"

/* The menu is saved at address just below 16m  */
#define FB_MENU_ADDR	0xff0000

#define FB_MAGIC	"FBBF"
#define FB_MAGIC_LONG	0x46424246

#define FB_AR_MAGIC		"FBAR"
#define FB_AR_MAGIC_LONG	0x52414246

#define FBM_TYPE_FILE	1

#define uchar	unsigned char
#define uchar2	unsigned short
#define uchar4	unsigned long

struct fb_mbr
{
  uchar jmp_code;
  uchar jmp_ofs;
  uchar boot_code[0x1ab];
  uchar max_sec;		/* 0x1ad  */
  uchar2 lba;			/* 0x1ae  */
  uchar spt;			/* 0x1b0  */
  uchar heads;			/* 0x1b1  */
  uchar2 boot_base;		/* 0x1b2  */
  uchar4 fb_magic;		/* 0x1b4  */
  uchar mbr_table[0x46];	/* 0x1b8  */
  uchar2 end_magic;		/* 0x1fe  */
} __attribute__((packed));

struct fb_data
{
  uchar2 boot_size;		/* 0x200  */
  uchar2 flags;			/* 0x202  */
  uchar ver_major;		/* 0x204  */
  uchar ver_minor;		/* 0x205  */
  uchar2 list_used;		/* 0x206  */
  uchar2 list_size;		/* 0x208  */
  uchar2 pri_size;		/* 0x20a  */
  uchar4 ext_size;		/* 0x20c  */
} __attribute__((packed));

struct fbm_file
{
  uchar size;
  uchar flag;
  uchar4 data_start;
  uchar4 data_size;
  uchar4 data_time;
  char name[0];
} __attribute__((packed));

static int fb_inited = FB_DRIVE;
static int fb_drive;
static uchar4 fb_ofs;
static uchar4 fb_pri_size;
static struct fbm_file *cur_file;
static uchar *fbm_buff = NULL;
static uchar4 ud_ofs;
static uchar4 ud_pri_size;
static int ud_inited;

static int fb_init (void)
{
  struct fb_mbr m;
  struct fb_data *data;
  int boot_base = 0, boot_size = 0, list_used, i;
  uchar *fb_list, *p1, *p2;
  uchar4 t_fb_ofs = 0;
  uchar4 t_fb_pri_size = 0;

	if (! rawread (fb_drive, 0, 0, 512, (unsigned long long)(unsigned int)&m, 0xedde0d90))
		return 0;

	data = (struct fb_data *)&m;
	if ((m.fb_magic == FB_MAGIC_LONG) && (m.end_magic == 0xaa55))
	{
		boot_base = m.boot_base;
		t_fb_ofs = m.lba;
		if (! rawread (fb_drive, boot_base + 1 - t_fb_ofs, 0, 512,
		 (unsigned long long)(unsigned int)(char *)data, 0xedde0d90))
			return 0;
		boot_size = data->boot_size;
		t_fb_pri_size = data->pri_size;
	}
	else if (*(unsigned long *)&m != FB_AR_MAGIC_LONG)
		return 0;

	if ((data->ver_major != 1) || (data->ver_minor != 6))
		return 0;

	list_used = data->list_used;

	if (current_drive == FB_DRIVE)
	{
		ud_inited++;
		fb_list = (uchar *) FB_MENU_ADDR;
		ud_ofs = t_fb_ofs;
		ud_pri_size = t_fb_pri_size;
	}
	else
	{
		if (fbm_buff == NULL && (fbm_buff = grub_malloc((unsigned long long)list_used << 9)) == NULL)
			return 0;
		fb_list = fbm_buff;
	}

	if (! rawread (fb_drive, boot_base + 1 + boot_size - t_fb_ofs, 0,
		 (unsigned long long)list_used << 9, (unsigned long long)(unsigned int)fb_list, 0xedde0d90))
		return 0;

	p1 = p2 = fb_list;

	for (i = 0; i < list_used - 1; i++)
	{
		p1 += 510;
		p2 += 512;
		grub_memcpy (p1, p2, 510);
	}
	fb_ofs = t_fb_ofs;
	fb_pri_size = t_fb_pri_size;
	if (current_drive != FB_DRIVE)
		fb_inited = fb_drive;
	return 1;
}

int fb_mount (void)
{
	if (current_drive == FB_DRIVE)
	{
		if (! fb_status)
			return 0;
		fb_drive = (fb_status >> 8) & 0xff;
		fb_ofs = ud_ofs;
		fb_pri_size = ud_pri_size;
		if (ud_inited || fb_init())
		{
			if (ud_inited)
				goto return_true;
			return ud_inited;
		}
		fb_status = 0;
		ud_inited = 0;
		return 0;
	}

	if (current_partition != 0xFFFFFF)
		return 0;
	fb_drive = current_drive;
	if (fb_inited  == current_drive)
		goto return_true;
	if (! fb_init ())
		return 0;

return_true:

	//fsys_flags &= ~1; /* bit 0=0 for case sensitive filenames */
	return 1;
  
}

unsigned long long
fb_read (unsigned long long buf, unsigned long long len, unsigned long write)
{
  int ret;
  unsigned long sector, ofs, saved_len;

  if (! cur_file->size)
    return 0;

  if (cur_file->data_start >= fb_pri_size)
    {
      sector = cur_file->data_start + (filepos >> 9) - fb_ofs;
      disk_read_func = disk_read_hook;
      ret = rawread (fb_drive, sector, filepos & 0x1ff, len, buf, write);
      disk_read_func = NULL;

      if (ret)
	filepos += len;

      return (ret) ? len : 0;
    }

  if (! buf)
    {
      filepos += len;
      return len;
    }

  sector = cur_file->data_start + ((unsigned long) filepos / 510) - fb_ofs;
  ofs = (unsigned long) filepos % 510;
  saved_len = len;

  while (len)
    {
      int n;

      n = 510 - ofs;
      if (n > len)
	n = len;

      if (! rawread (fb_drive, sector, ofs, n, buf, write))
	return 0;

      sector++;
      ofs = 0;
      buf += n;
      len -= n;
    }

  filepos += saved_len;

  return saved_len;
}

int fb_dir (char *dirname)
{
  unsigned long found = 0;
  unsigned long i;
  char *dirpath;
  while (*dirname == '/')
    dirname++;
  dirpath = dirname;
  dirpath += grub_strlen(dirname);
  while (dirpath != dirname && *dirpath != '/')
	dirpath--;
  i = dirpath - dirname;
  if (*dirpath == '/')
	i++;

  cur_file = (struct fbm_file *)((current_drive == FB_DRIVE)?FB_MENU_ADDR:(int)fbm_buff);

  while (cur_file->size)
    {
      char tmp_name[512];/* max name len=255, so 512 byte buffer is needed. */
      unsigned long j, k;
      char ch1;

      /* copy cur_file->name to tmp_name, and quote spaces with '\\' */
      for (j = 0, k = 0; j < cur_file->size - 12; j++)
	{
	  if (! (ch1 = cur_file->name[j]))
		break;
	  if (ch1 == ' ')
		tmp_name[k++] = '\\';
	  tmp_name[k++] = ch1;
	}
	tmp_name[k] = 0;

#ifndef STAGE1_5
      if (print_possibilities)
	{
	  if (substring (dirname, tmp_name/*cur_file->name*/, 1) <= 0)
	    {
	      found = 1;
	      print_a_completion (tmp_name + i, 1);
	    }
	}
      else
#endif
	if (substring (dirname, tmp_name/*cur_file->name*/, 1) == 0)
	  {
	    found = 1;
	    filemax = cur_file->data_size;
	    break;
	  }

      cur_file = (struct fbm_file *) ((char *) cur_file + cur_file->size + 2);
    }

  if (! found)
    errnum = ERR_FILE_NOT_FOUND;

  return found;
}

#endif
