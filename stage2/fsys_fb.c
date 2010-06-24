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

static int fb_inited;
static int fb_drive;
static uchar4 fb_ofs;
static uchar4 fb_pri_size;
static struct fbm_file *cur_file;

static void fb_init (void)
{
  struct fb_mbr *m;
  struct fb_data *data;
  int boot_base, boot_size, list_used, i;
  uchar *fb_list, *p1, *p2;

  fb_inited++;

  if (! fb_status)
    return;

  m = (struct fb_mbr *) FB_MENU_ADDR;
  fb_drive = (fb_status >> 8) & 0xff;

  grub_printf ("%d\n", fb_drive);
  if (! rawread (fb_drive, 0, 0, 512, (unsigned long long)(unsigned int)(char *) m, 0xedde0d90))
    goto fail;

  if ((m->fb_magic != FB_MAGIC_LONG) || (m->end_magic != 0xaa55))
    goto fail;

  boot_base = m->boot_base;
  fb_ofs = m->lba;

  data = (struct fb_data *) m;
  if (! rawread (fb_drive, boot_base + 1 - fb_ofs, 0, 512,
		 (unsigned long long)(unsigned int)(char *)data, 0xedde0d90))
    goto fail;

  if ((data->ver_major != 1) || (data->ver_minor != 6))
    goto fail;

  boot_size = data->boot_size;
  list_used = data->list_used;
  fb_pri_size = data->pri_size;

  fb_list = (char *) data;
  if (! rawread (fb_drive, boot_base + 1 + boot_size - fb_ofs, 0,
		 list_used << 9, fb_list, 0xedde0d90))
    goto fail;

  p1 = p2 = fb_list;
  for (i = 0; i < list_used - 1; i++)
  {
	p1 += 510;
	p2 += 512;
	grub_memcpy (p1, p2, 510);
   }

  return;

 fail:
  fb_status = 0;
}

int fb_mount (void)
{
  if (! fb_inited)
    fb_init ();

  return (current_drive == FB_DRIVE);
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
  int found = 0;

  while (*dirname == '/')
    dirname++;

  cur_file = (struct fbm_file *) FB_MENU_ADDR;

  while (cur_file->size)
    {
#ifndef STAGE1_5
      if (print_possibilities)
	{
	  if (substring (dirname, cur_file->name, 1) <= 0)
	    {
	      found = 1;
	      print_a_completion (cur_file->name);
	    }
	}
      else
#endif
	if (substring (dirname, cur_file->name, 1) == 0)
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
