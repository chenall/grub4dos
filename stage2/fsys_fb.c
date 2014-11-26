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
/* Emm... Room between address 15M and 16M may be used by chipset.
 * Use 64K at 0x150000 instead.      -- tinybit  2012-11-07  */
//#define FB_MENU_ADDR	0xff0000
#define FB_MENU_ADDR	0x150000

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
static unsigned long fb_drive;
static uchar4 fb_ofs;
static uchar4 fb_pri_size;
static struct fbm_file *cur_file;
static uchar *fbm_buff = NULL;
static uchar4 ud_ofs;
static uchar4 ud_pri_size;
static unsigned long ud_inited = 0;

extern unsigned long ROM_int13;
extern unsigned long ROM_int15;
static unsigned long is_virtual (unsigned long drive)
{
	unsigned long i;
	unsigned long addr;
	unsigned long low_mem;
	struct drive_map_slot *drive_map;

	low_mem = (*(unsigned short *)0x413);
	if (low_mem >= 640)
		return 0;
	low_mem <<= (16 + 6);
	low_mem |= 0x100;
	addr = low_mem;
	if (low_mem != (*(unsigned long *)0x4C))
		return 0;
	low_mem >>= 12;		/* int13_handler base address */
	if (grub_memcmp ((char *)(low_mem + 0x103), "$INT13SFGRUB4DOS", 16))
		return 0;
	if (*(unsigned long *)(low_mem + 0x1C) < 0x5A000000) // old int13
		return 0;
	if (*(unsigned long *)(low_mem + 0x0C) < 0x5A000000) // old int15
		return 0;
	if (*(unsigned long *)(low_mem + 0x1C) != ROM_int13) // old int13
		return 0;
	if (*(unsigned long *)(low_mem + 0x0C) != ROM_int15) // old int15
		return 0;
	/* Yes, hooked. The drive map slots begins at (low_mem + 0x20). */
	drive_map = (struct drive_map_slot *)(low_mem + 0x20);
	for (i = 0; i < DRIVE_MAP_SIZE; i++)
	{
		if (drive_map_slot_empty (drive_map[i]))
			break;
		if (drive_map[i].from_drive == drive)
			return addr;//(i * sizeof (struct drive_map_slot)) + low_mem + 0x20;
	}
	return 0;
}

static unsigned long quick_hook (unsigned long addr)
{
	unsigned long low_mem;

	if (addr)
		goto hook;
	low_mem = (*(unsigned short *)0x413);
	if (low_mem >= 640)
		return 0;
	low_mem <<= (16 + 6);
	low_mem |= 0x100;
	addr = low_mem;
	if (low_mem != (*(unsigned long *)0x4C))
		return 0;
	low_mem >>= 12;		/* int13_handler base address */
	if (grub_memcmp ((char *)(low_mem + 0x103), "$INT13SFGRUB4DOS", 16))
		return 0;
	if (*(unsigned long *)(low_mem + 0x1C) < 0x5A000000) // old int13
		return 0;
	if (*(unsigned long *)(low_mem + 0x0C) < 0x5A000000) // old int15
		return 0;
	if (*(unsigned long *)(low_mem + 0x1C) != ROM_int13) // old int13
		return 0;
	if (*(unsigned long *)(low_mem + 0x0C) != ROM_int15) // old int15
		return 0;
	/* Yes, hooked. The drive map slots begins at (low_mem + 0x20). */

	(*(unsigned long *)0x4C) = ROM_int13;
	buf_drive = -1;
	buf_track = -1;
	return addr;

hook:
	(*(unsigned long *)0x4C) = addr;
	buf_drive = -1;
	buf_track = -1;
	return addr;
}

static int fb_init (void)
{
  struct fb_mbr m;
  struct fb_data *data;
  int boot_base = 0, boot_size = 0, list_used, i;
  uchar *fb_list, *p1, *p2;
  uchar4 t_fb_ofs = 0;
  uchar4 t_fb_pri_size = 0;
  unsigned long ret;
  unsigned long fb_drive_virtual;

	fb_drive_virtual = is_virtual(fb_drive);

	if (fb_drive_virtual  && fb_status && fb_drive == (unsigned char)(fb_status >> 8))
		quick_hook (0);

	ret = rawread (fb_drive, 0, 0, 512, (unsigned long long)(unsigned int)&m, 0xedde0d90);
	if (! ret)
		goto init_end;

	data = (struct fb_data *)&m;
	if ((m.fb_magic == FB_MAGIC_LONG) && (m.end_magic == 0xaa55))
	{
		boot_base = m.boot_base;
		t_fb_ofs = m.lba;
		ret = rawread (fb_drive, boot_base + 1 - t_fb_ofs, 0, 512,
		 (unsigned long long)(unsigned int)(char *)data, 0xedde0d90);
		if (! ret)
			goto init_end;
		boot_size = data->boot_size;
		t_fb_pri_size = data->pri_size;
	}
	else if (*(unsigned long *)&m != FB_AR_MAGIC_LONG)
	{
		ret = 0;
		goto init_end;
	}

	if ((data->ver_major != 1) || (data->ver_minor != 6))
	{
		ret = 0;
		goto init_end;
	}

	list_used = data->list_used;

	/* if the dir list exceeds 64K, safely exit with failure. */
	if (list_used > 128)
	{
		errnum = ERR_WONT_FIT;
		ret = 0;
		goto init_end;
	}

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
		{
			ret = 0;
			goto init_end;
		}
		fb_list = fbm_buff;
	}

	ret = rawread (fb_drive, boot_base + 1 + boot_size - t_fb_ofs, 0,
		 (unsigned long long)list_used << 9, (unsigned long long)(unsigned int)fb_list, 0xedde0d90);
	if (! ret)
		goto init_end;

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
	/* ret != 0, success */

init_end:

	if (fb_drive_virtual  && fb_status && fb_drive == (unsigned char)(fb_status >> 8))
		quick_hook (fb_drive_virtual);

	return ret;
}

int fb_mount (void)
{
	int ret;

	if (current_drive == FB_DRIVE)
	{
		if (! fb_status)
			return 0;
		fb_drive = (unsigned char)(fb_status >> 8);
		fb_ofs = ud_ofs;
		fb_pri_size = ud_pri_size;
		if (ud_inited)
			goto return_true;

		ret = fb_init();

		if (ret)
			return ud_inited;
		fb_status = 0;
		ud_inited = 0;
		return 0;
	}

	if (current_partition != 0xFFFFFF)
		return 0;
	fb_drive = current_drive;
	if (fb_inited  == current_drive)
		goto return_true;

	ret = fb_init();

	if (! ret)
		return 0;

return_true:

	//fsys_flags &= ~1; /* bit 0=0 for case sensitive filenames */
	return 1;
  
}

unsigned long long
fb_read (unsigned long long buf, unsigned long long len, unsigned long write)
{
  unsigned long long ret;
  unsigned long sector, ofs, saved_len;
  unsigned long fb_drive_virtual;

  if (! cur_file->size)
    return 0;

  if (! (ret = len))
    return 0;

  fb_drive_virtual = is_virtual(fb_drive);

  if (cur_file->data_start >= fb_pri_size)
    {
      sector = cur_file->data_start + (filepos >> 9) - fb_ofs;

      if (fb_drive_virtual  && fb_status && fb_drive == (unsigned char)(fb_status >> 8))
		quick_hook (0);

      disk_read_func = disk_read_hook;
      ret = rawread (fb_drive, sector, filepos & 0x1ff, len, buf, write);
      disk_read_func = NULL;

      if (fb_drive_virtual  && fb_status && fb_drive == (unsigned char)(fb_status >> 8))
		quick_hook (fb_drive_virtual);

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

  if (fb_drive_virtual  && fb_status && fb_drive == (unsigned char)(fb_status >> 8))
	quick_hook (0);

  while (len)
    {
      unsigned long n;

      n = 510 - ofs;
      if (n > len)
	n = len;

      ret = rawread (fb_drive, sector, ofs, n, buf, write);

      if (! ret)
	break;

      sector++;
      ofs = 0;
      buf += n;
      len -= n;
    }

  if (fb_drive_virtual  && fb_status && fb_drive == (unsigned char)(fb_status >> 8))
	quick_hook (fb_drive_virtual);

  if (! ret)
	return 0;

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

      if (print_possibilities)
	{
	  if (substring (dirname, tmp_name/*cur_file->name*/, 1) <= 0)
	    {
	      found = 1;
	      print_a_completion (tmp_name + i, 1);
	    }
	}
      else
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
