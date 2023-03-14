/* builtins.c - the GRUB builtin commands */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004  Free Software Foundation, Inc.
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

#include <shared.h>
#include <filesys.h>
#include <term.h>
#include "iamath.h"
#include <iso9660.h>

//#ifdef SUPPORT_SERIAL
//# include <serial.h>
//# include <terminfo.h>
//#endif

#ifdef USE_MD5_PASSWORDS
# include <md5.h>
#endif

#include "freebsd.h"

/* The type of kernel loaded.  */
kernel_t kernel_type;
//static kernel_t kernel_type_orig;

/* The boot device.  */
static int bootdev = 0;

/* The default entry.  */
int default_entry = 0;
/* The fallback entry.  */
int fallback_entryno;
int fallback_entries[MAX_FALLBACK_ENTRIES];
/* The number of current entry.  */
int current_entryno;
#ifdef SUPPORT_GFX
/* graphics file */
//char graphics_file[64];
#endif
/* The address for Multiboot command-line buffer.  */
static char *mb_cmdline;// = (char *) MB_CMDLINE_BUF;
//static char kernel_option_video[64] = {0};/* initialize the first byte to 0 */
/* The password.  */
char *password_buf;
static char password_str[128];
/* The password type.  */
password_t password_type;
/* The flag for indicating that the user is authoritative.  */
int auth = 0;
/* The timeout.  */
//int grub_timeout = -1;
/* Whether to show the menu or not.  */
int show_menu = 1;
/* Don't display a countdown message for the hidden menu */
int silent_hiddenmenu = 0;
int debug_prog;
int debug_bat = 0;
int debug_ptrace = 0;
//static int debug_break = 0;
//static int debug_pid = 0;
//static int debug_check_memory = 0;
static grub_u8_t msg_password[]="Password: ";
unsigned int pxe_restart_config = 0;
unsigned int configfile_in_menu_init = 0;

/* The first sector of stage2 can be reused as a tmp buffer.
 * Do NOT write more than 512 bytes to this buffer!
 * The stage2-body, i.e., the pre_stage2, starts at 0x8200!
 * Do NOT overwrite the pre_stage2 code at 0x8200!
 */
//extern char *mbr /* = (char *)0x8000 */; /* 512-byte buffer for any use. */

extern int dir (char *dirname);

#ifdef SUPPORT_GRAPHICS
//extern int outline;
#endif /* SUPPORT_GRAPHICS */

/* The BIOS drive map.  */
//int drive_map_slot_empty (struct drive_map_slot item);

static int chainloader_load_segment = 0;
static int chainloader_load_offset = 0;
static int chainloader_skip_length = 0;
static int chainloader_ebx = 0;
static int chainloader_ebx_set = 0;
static int chainloader_edx = 0;
static int chainloader_edx_set = 0;
static int is_io = 0;
//使用于configfile_func, command_open, GetParentUtf8Name
static char chainloader_file_orig[256];

static const char *warning_defaultfile = "# WARNING: If you want to edit this file directly, do not remove any line";
int probe_bpb (struct master_and_dos_boot_sector *BS);
int probe_mbr (struct master_and_dos_boot_sector *BS, unsigned int start_sector1, unsigned int sector_count1, unsigned int part_start1);
int parse_string (char *arg);
int envi_cmd(const char *var,char * const env,int flags);
//extern int count_lines;
//extern int use_pager;
#if defined(__i386__)
char *convert_to_ascii (char *buf, int c, ...);
#else
char *convert_to_ascii (char *buf, int c, unsigned long long lo);
#endif
unsigned int prog_pid;
extern int errorcheck;
unsigned int next_partition_drive;
unsigned int next_partition_dest;
unsigned int *next_partition_partition;
unsigned int *next_partition_type;
unsigned long long *next_partition_start;
unsigned long long *next_partition_len;
unsigned long long *next_partition_offset;
unsigned int *next_partition_entry;
unsigned int *next_partition_ext_offset;
char *next_partition_buf;
unsigned char partition_signature[16];  //分区签名
unsigned char partition_activity_flag;  //分区活动标志
int fsys_type;
extern unsigned int fats_type;
extern unsigned int iso_type;
int get_mmap_entry (struct mmar_desc *desc, int cont);
int no_install_vdisk = 0;  //0/1=安装虚拟磁盘/不安装虚拟磁盘
static int ls_func (char *arg, int flags);
static char *GRUB_MOD_ADDR=0;
static char* mod_end;
static int find_func (char *arg, int flags);
static int insmod_func(char *arg,int flags);
static unsigned long long start_sector, sector_count;
unsigned long long initrd_start_sector;
grub_efi_uint64_t	part_addr;
grub_efi_uint64_t	part_size;
grub_efi_uint32_t cd_boot_entry;
grub_efi_uint16_t cd_boot_start;
grub_efi_uint32_t cd_boot_size;
grub_efi_uint32_t cd_Image_part_start;
grub_efi_uint32_t cd_Image_disk_size;
struct grub_part_data *part_data;

void set_full_path(char *dest, char *arg, grub_u32_t max_len);
void set_full_path(char *dest, char *arg, grub_u32_t max_len)
{
	int len;

	if (*arg != '/' && !(*arg == '(' && arg[1] == ')'))
	{
		grub_memmove (dest, arg, max_len);
		return;
	}
	print_root_device(dest,0);

	len = strlen(dest);
	if (*arg == '/') grub_sprintf(dest + len,"%s%s",saved_dir,arg);
	else grub_sprintf(dest + len,"%s",arg + 2);
}
#if 0
int drive_map_slot_empty (struct drive_map_slot item);
int
drive_map_slot_empty (struct drive_map_slot item)//判断驱动器映像插槽是否为空   为空,返回1
{
	grub_size_t *array = (grub_size_t *)&item;
	
	grub_size_t n = sizeof (struct drive_map_slot) / sizeof (grub_size_t);
	
	while (n)
	{
		if (*array)
			return 0;
		array++;
		n--;
	}

	return 1;
}

int disable_map_info = 0;
#endif
/* Prototypes for allowing straightfoward calling of builtins functions
   inside other functions.  */
static int configfile_func (char *arg, int flags);
int command_func (char *arg, int flags);
int commandline_func (char *arg, int flags);
int errnum_func (char *arg, int flags);
int checkrange_func (char *arg, int flags);

/* Check a password for correctness.  Returns 0 if password was
   correct, and a value != 0 for error, similarly to strcmp. */
int check_password (char* expected, password_t type);
int
check_password (char* expected, password_t type)
{
	/* Do password check! */
	char entered[32];

	/* Wipe out any previously entered password */
	memset(entered,0,sizeof(entered));
	get_cmdline_str.prompt = msg_password;
	get_cmdline_str.maxlen = sizeof (entered) - 1;
	get_cmdline_str.echo_char = '*';
	get_cmdline_str.readline = 0;
	get_cmdline_str.cmdline = (unsigned char*)entered;
	get_cmdline ();
	
  switch (type)
    {
    case PASSWORD_PLAIN:
      return strcmp (entered, expected);

#ifdef USE_MD5_PASSWORDS
    case PASSWORD_MD5:
      return check_md5_password (entered, expected);
#endif
    default: 
      /* unsupported password type: be secure */
      return 1;
    }
}

/* Print which sector is read when loading a file.  */
static void disk_read_print_func (unsigned long long sector, unsigned int offset, unsigned long long length);
static void
disk_read_print_func (unsigned long long sector, unsigned int offset, unsigned long long length)
{
  grub_printf ("[0x%lx,0x%x,0x%lx]", sector, offset, length);
}

extern int rawread_ignore_memmove_overflow; /* defined in disk_io.c */
int query_block_entries;
static unsigned long long map_start_sector[DRIVE_MAP_FRAGMENT];	
static unsigned long long map_num_sectors[DRIVE_MAP_FRAGMENT];

static unsigned long long blklst_start_sector;
static unsigned long long blklst_num_sectors;
static unsigned int blklst_num_entries;
static unsigned int blklst_last_length;

  /* Collect contiguous blocks into one entry as many as possible,
     and print the blocklist notation on the screen.  */
static void disk_read_blocklist_func (unsigned long long sector, unsigned int offset, unsigned long long length);
static void
disk_read_blocklist_func (unsigned long long sector, unsigned int offset, unsigned long long length)
{
	unsigned int sectorsize = buf_geom.sector_size;
//	unsigned char sector_bit = (sectorsize == 2048 ? 11 : sectorsize == 4096 ? 12 : 9);
  unsigned char sector_bit = buf_geom.log2_sector_size;
#ifdef FSYS_INITRD
	if (fsys_table[fsys_type].mount_func == initrdfs_mount)
	{
		if (query_block_entries >= 0)
			printf("(md,0x%lx,0x%lx)+1",(sector << SECTOR_BITS) + offset,length);
		return;
	}
#endif
	if (blklst_num_sectors > 0)
	{
	  if (blklst_start_sector + blklst_num_sectors == sector
	      && offset == 0 && blklst_last_length == 0)
	    {
	      blklst_num_sectors += (length + (sectorsize - 1)) >> sector_bit;
	      blklst_last_length = (length - (sectorsize - offset))&(sectorsize - 1);
	      return;
	    }
	  else
	    {
	      if (query_block_entries >= 0)
	        {
		  if (blklst_last_length == 0)
		    grub_printf ("%s0x%lx+0x%lx", (blklst_num_entries ? "," : ""),
			     (unsigned long long)(blklst_start_sector/* - part_start*/), blklst_num_sectors);
		  else if (blklst_num_sectors > 1)
		    grub_printf ("%s0x%lx+0x%lx,0x%lx[0-0x%x]", (blklst_num_entries ? "," : ""),
			     (unsigned long long)(blklst_start_sector/* - part_start*/), (blklst_num_sectors-1),
			     (unsigned long long)(blklst_start_sector + blklst_num_sectors-1/* - part_start*/),
			     blklst_last_length);
		  else
		    grub_printf ("%s0x%lx[0-0x%x]", (blklst_num_entries ? "," : ""),
			     (unsigned long long)(blklst_start_sector/* - part_start*/), blklst_last_length);
	        }
	        else if (blklst_last_length == 0 && blklst_num_entries < DRIVE_MAP_FRAGMENT)
		{
			map_start_sector[blklst_num_entries] = blklst_start_sector;
			map_num_sectors[blklst_num_entries] = blklst_num_sectors;
		}
	      blklst_num_entries++;
	      blklst_num_sectors = 0;
	    }
	}

	if (offset > 0)
	{
	  if (query_block_entries >= 0)
			grub_printf("%s0x%lx[0x%x-0x%x]", (blklst_num_entries ? "," : ""),
				(unsigned long long)(sector/* - part_start*/), offset, (offset + length));
	  blklst_num_entries++;
	}
      else
	{
	  blklst_start_sector = sector;
	  blklst_num_sectors = (length + sectorsize - 1) >> sector_bit;
	  blklst_last_length = (length - (sectorsize - offset))&(sectorsize - 1);
	}
}

/* blocklist */
static int blocklist_func (char *arg, int flags);
static int
blocklist_func (char *arg, int flags)
{
  char *dummy = NULL;
  int i;
  unsigned long long err;
#ifndef NO_DECOMPRESSION
  int no_decompression_bak = no_decompression;
#endif
  errnum = 0;
  blklst_start_sector = 0;
  blklst_num_sectors = 0;
  blklst_num_entries = 0;
  blklst_last_length = 0;

 for (i = 0; i < DRIVE_MAP_FRAGMENT; i++)
 {
	map_start_sector[i] =0;
	map_num_sectors[i] =0;
	}

  /* Open the file.  */
  if (! grub_open (arg))
    goto fail_open;
#ifdef FSYS_INITRD
  if (fsys_table[fsys_type].mount_func == initrdfs_mount)
  {
    disk_read_hook = disk_read_blocklist_func;
    err = grub_read ((unsigned long long)(grub_size_t)dummy,-1ULL, GRUB_LISTBLK);
    disk_read_hook = 0;
    goto fail_read;
  }
#endif
#ifndef NO_DECOMPRESSION
  if (compressed_file)
  {
    if (query_block_entries < 0 && decomp_type != DECOMP_TYPE_VHD) //为了给vhd_start_sector赋值
    {
	/* compressed files are not considered contiguous. */
	goto fail_read;
    }
    grub_close ();
    no_decompression = 1;
    if (! grub_open (arg))
	goto fail_open;
  }
#endif /* NO_DECOMPRESSION */
  /* Print the device name.  */
  if (query_block_entries >= 0) print_root_device (NULL,1 | 0x100);
  rawread_ignore_memmove_overflow = 1;
  /* Read in the whole file to DUMMY.  */
  disk_read_hook = disk_read_blocklist_func;
  err = grub_read ((unsigned long long)(grub_size_t)dummy, -1ULL, GRUB_LISTBLK);
  disk_read_hook = 0;
  rawread_ignore_memmove_overflow = 0;

  if (fsys_table[fsys_type].mount_func == pxe_mount)
  {
    map_start_sector[0] = (unsigned long long)(grub_size_t)(char*)efi_pxe_buf;
    printf("0x%lx+0x%lx", (unsigned long long)(grub_size_t)(char*)efi_pxe_buf >> 9, (filemax + 0x1ff) >> 9);
    query_block_entries = blklst_num_entries = 1;
    goto fail_read;
  }

  if (! err)
    goto fail_read;
  /* The last entry may not be printed yet.  Don't check if it is a
   * full sector, since it doesn't matter if we read too much. */
  if (blklst_num_sectors > 0)
  {
    if (query_block_entries >= 0)
      grub_printf ("%s0x%lx+0x%lx", (blklst_num_entries ? "," : ""),
          (unsigned long long)(blklst_start_sector/* - part_start*/), blklst_num_sectors);
    else if (blklst_num_entries < DRIVE_MAP_FRAGMENT)
    {
      map_start_sector[blklst_num_entries] = blklst_start_sector;
      map_num_sectors[blklst_num_entries] = blklst_num_sectors;
    }
    blklst_num_entries++;
  }
  if (query_block_entries >= 0)
    grub_putchar ('\n', 255);
  else
    query_block_entries = blklst_num_entries;

fail_read:
  grub_close ();

fail_open:
#ifndef NO_DECOMPRESSION
  no_decompression = no_decompression_bak;
#endif

  if (query_block_entries < 0)
    query_block_entries = 0;
  return ! errnum;
}

static struct builtin builtin_blocklist =
{
  "blocklist",
  blocklist_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE | BUILTIN_NO_DECOMPRESSION,
  "blocklist FILE",
  "Print the blocklist notation of the file FILE."
};

/* WinME support by bean. Thanks! */

#define obuf_init(buf)		obuf_ptr=buf
#define obuf_putc(ch)		*(obuf_ptr++)=ch
#define obuf_copy(ofs,len)	do { for (;len>0;obuf_ptr++,len--) *(obuf_ptr)=*(obuf_ptr-ofs); } while (0)


//static grub_efi_char16_t chainloader_type = 0;
static grub_efi_handle_t image_handle;

void hexdump(grub_u64_t ofs,char* buf,int len);
void hexdump(grub_u64_t ofs,char* buf,int len)
{
  quit_print=0;
  int align = len > 16;
  while (len>0)
    {
      int cnt,k,i,j = 3;

      if (align)
      {
        i = ofs & 0xFLL;
        if (i)
          ofs &= ~0xFLL;
      }
      else
      {
        i = 0;
      }

      if ((ofs >> 32))
          j = 7;

      grub_printf ("%0*lX: ", j == 3?8:10,ofs);

      cnt = 16;
      if (cnt > len)
        cnt = len + i;

      for (k=0;k<16;k++)
        {
          if (i>k || k >=cnt)
            printf("   ");
          else
            printf("%02X ", (unsigned long)(unsigned char)(buf[k - i]));
          if ((k!=15) && ((k & j)==j))
            printf(" ");
        }

      printf("; ");

      for (k=0;k<cnt;k++)
      {
        if (i>k)
           putchar(' ',255);
        else
        {
          j = k - i;
          putchar((((unsigned char)buf[j]>=32) && ((unsigned char)buf[j]<0x7f))?buf[j]:'.', 255);
        }
      }

      printf("\n");

      if (quit_print)
        break;

      ofs+=16;
      cnt -= i;
      len -= cnt;
      buf += cnt;
    }
}


/* cat */
static int cat_func (char *arg, int flags);
static int
cat_func (char *arg, int flags)
{
  unsigned char c;
  unsigned char s[128];
  unsigned long long Hex = 0;
  unsigned int len, i, j;
  char *p;
  unsigned long long skip = 0;
  unsigned long long length = 0xffffffffffffffffULL;
  char *locate = 0;
  char *replace = 0;
  unsigned long long locate_align = 1;
  unsigned long long len_s;
  unsigned long long len_r = 0;
  unsigned long long ret = 0;
  unsigned long long number = -1ULL;
  int locate_ignore_case = 0;

  quit_print = 0;
  errnum = 0;
  for (;;)
  {
	if (grub_memcmp (arg, "--hex=", 6) == 0)
	{
		arg += 6;
		safe_parse_maxint (&arg, &Hex);
	}
	else if (grub_memcmp (arg, "--hex", 5) == 0)
	{
		Hex = 1;
	}
	else if (grub_memcmp (arg, "--skip=", 7) == 0)
	{
		arg += 7;
		safe_parse_maxint(&arg,&skip);
	}
	else if (grub_memcmp (arg, "--length=", 9) == 0)
	{
		arg += 9;
		safe_parse_maxint(&arg, &length);
	}
	else if (grub_memcmp (arg, "--locate=", 9) == 0)
	{
		locate = arg += 9;
	}
	else if (grub_memcmp (arg, "--locatei=", 9) == 0)
	{
		locate_ignore_case = 1;
		locate = arg += 10;
	}
	else if (grub_memcmp (arg, "--replace=", 10) == 0)
	{
		replace = arg += 10;
	}
	else if (grub_memcmp (arg, "--locate-align=", 15) == 0)
	{
		arg += 15;
		if (! safe_parse_maxint (&arg, &locate_align))
			return 0;
		if ((unsigned int)locate_align == 0)
			return ! (errnum = ERR_BAD_ARGUMENT);
	}
	else if (grub_memcmp (arg, "--number=",9) == 0)
	{
		arg += 9;
		safe_parse_maxint (&arg, &number);
	}
	else
		break;
	if (errnum)
		return 0;
	arg = skip_to (0, arg);
  }
  if (! length)
  {
    if (grub_memcmp (arg,"()-1\0",5) == 0 )
    {
        if (! grub_open ("()+1"))
            return 0;
        filesize = filemax*(unsigned long long)part_start;
    } 
    else if (grub_memcmp (arg,"()\0",3) == 0 )
    {
        if (! grub_open ("()+1"))
            return 0;
        filesize = filemax*(unsigned long long)part_length;
    }
    else 
    {
		int no_decompression_bak = no_decompression;
		no_decompression = 1;
       if (! grub_open (arg))
       {
			no_decompression = no_decompression_bak;
            return 0;
       }
       filesize = filemax;
       no_decompression = no_decompression_bak;
    }
	grub_close();
	printf_debug0 ("Filesize is 0x%lX\n", (unsigned long long)filesize);
	return (filesize>>32)?(unsigned long long)-1:filesize;
  }

	if (replace)
	{
		p = replace;
		if (*p++ == '*')
		{
			safe_parse_maxint (&p, &len_r);
			errnum = 0;
		}
		if (! len_r)
		{
			#if 0
			if (*replace == '\"')
			{
				for (i = 0; i < 128 && (r[i] = *(++replace)) != '\"'; i++);
			}else{
				for (i = 0; i < 128 && (r[i] = *(replace++)) != ' ' && r[i] != '\t'; i++);
			}
			r[i] = 0;
			replace = (char*)r;
			len_r = parse_string (replace);
			#else
			wee_skip_to(replace,SKIP_WITH_TERMINATE);
			c = *replace;
			len_r = parse_string (replace);
			if (c == '\"')
				++replace,len_r -= 2;
			#endif
		}
		else
		{
			replace = (char*)(grub_size_t)(unsigned long long)len_r;
			len_r = Hex?Hex:8;
		}
		if ((int)len_r <= 0)
		{
			return ! (errnum = ERR_BAD_ARGUMENT);
		}
	}
  
    if (! grub_open (arg))
    return 0; 
  if (length > filemax)
      length = filemax;
  filepos = skip;
  
  if (locate)
  {
    #if 0
    if (*locate == '\"')
    {
      for (i = 0; i < 128 && (s[i] = *(++locate)) != '\"'; i++);
    }else{
      for (i = 0; i < 128 && (s[i] = *(locate++)) != ' ' && s[i] != '\t'; i++);
    }
    s[i] = 0;
    len_s = parse_string ((char *)s);
    locate = s;
    #else
    wee_skip_to(locate,SKIP_WITH_TERMINATE);
    c = *locate;
    len_s = parse_string (locate);
    if (c == '\"')
      ++locate,len_s -= 2;
    #endif
    if (len_s == 0 || len_s > 16)
    {
      grub_close();
      return ! (errnum = ERR_BAD_ARGUMENT);
    }
    //j = skip;
    grub_memset ((char *)(SCRATCHADDR), 0, 32);
	length += skip;
	unsigned long long k,l;
	for (i = 0,j = skip; ; j += 16)
	{
		len = 0;
		if (j < length)
		{
			len = grub_read ((unsigned long long)(grub_size_t)(char *)(SCRATCHADDR + 16), 16, 0xedde0d90);
		}
		if (len < 16)
		{
			k = j > length ? 16 - (j - length):16 + len;
			grub_memset ((char *)(SCRATCHADDR + (int)k), 0, 32 - (int)k);
		}
		if (j>length)
			l = 16 - (j - length);
		else
			l = 16;

		if (j != skip)
		{
			while (i < l)
			{
				k = j - 16 + i;
				if ((locate_align == 1 || ! ((grub_size_t)k % (grub_size_t)locate_align))
					&& strncmpx (locate, (char *)(SCRATCHADDR + (grub_size_t)i), len_s,locate_ignore_case) == 0)
				{
					/* print the address */
					if (!replace || debug > 1)
						grub_printf (" %lX", (unsigned long long)k);
					/* replace strings */
					if (replace)
					{
						unsigned long long filepos_bak = filepos;
						filepos = k;
						/* write len_r bytes at string replace to file!! */
						grub_read ((unsigned long long)(grub_size_t)replace,len_r, 0x900ddeed);
						i += len_r;
//						if (filepos < filepos_bak)
							filepos = filepos_bak;
					}
					else
						i += len_s;
					ret++;
					Hex = k;
					if (number <= ret)
					{
						len = 0;
						break;
					}
				}
				else
					i++;
			}
			if (quit_print)
				break;
			if (len == 0)
			{
				sprintf(ADDR_RET_STR,"0x%x",Hex);
				break;
			}
			i -= 16;
		}
		grub_memmove ((char *)SCRATCHADDR, (char *)(SCRATCHADDR + 16), 16);
	}
  }else if (Hex == (++ret))	/* a trick for (ret = 1, Hex == 1) */
  {
    j = 16/* - (skip & 0xF)*/;

    if (j > length)
      j = length;
    while ((len = grub_read ((unsigned long long)(grub_size_t)&s, j, 0xedde0d90)))
    {
      hexdump(skip,(char*)&s,len);
      if (quit_print)
        break;
      skip += len;
      length -= len;
      j = (length >= 16)?16:length;
    }
  }else
    for (j = 0; j < length && grub_read ((unsigned long long)(grub_size_t)&c, 1, 0xedde0d90) && c; j++)
    {
		grub_putchar (c, 255);
	if (quit_print)
		break;
    }
  
  grub_close ();
  return ret;
}

static struct builtin builtin_cat =
{
  "cat",
  cat_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "cat [--hex] [--skip=S] [--length=L] [--locate[i]=STRING] [--replace=REPLACE]\n"
  "\t [--locate-align=A] [--number=n] FILE",
  "Print the contents of the file FILE,"
  "Or print the locations of the string STRING in FILE,"
  "--replace replaces STRING with REPLACE in FILE."
  "--number  use with --locate,the max number for locate",
};

void map_to_svbus (grub_efi_physical_address_t address);
void
map_to_svbus (grub_efi_physical_address_t address)
{
#if 0
  int i, j;

  //复制映射插槽
  for (i = 0, j = 0; i < DRIVE_MAP_SIZE; i++)
  {
    if (drive_map_slot_empty (disk_drive_map[i]))   //判断驱动器映像插槽是否为空   为空,返回1
      break;

    //滤除软盘
    if (disk_drive_map[i].from_drive < 0x80)
      continue;
    //复制映射插槽
    grub_memmove ((char *)((char *)(grub_size_t)address + j*24), (char *)&disk_drive_map[i], 24);
    j++;
  }
#else
  struct grub_disk_data *d;	//磁盘数据
  struct drive_map_slot *s = (struct drive_map_slot *)(grub_size_t)address;

	for (d = disk_data; d; d = d->next)	//从设备结构起始查; 只要设备存在,并且驱动器号不为零;
	{
		if (d->sector_count && d->drive >= 0x80)  //如果是映射驱动器, 不是软盘
    {
      s->from_drive = d->drive;
      s->to_drive = d->to_drive;
      s->max_head = 0xfe;    //避免旧svbus蓝屏。支持碎片的新版本不需要
      s->fragment = d->fragment;
      if (d->from_log2_sector == 0xb)
        s->from_cdrom = 1;
      s->to_sector = 0x3f;   //避免旧svbus蓝屏。支持碎片的新版本不需要。
      s->start_sector = d->start_sector;
      s->sector_count = d->sector_count;
      s++;
    }
  }
#endif

  //复制碎片插槽
  grub_memmove ((char *)((char *)(grub_size_t)address + 0x148), (char *)&disk_fragment_map, 0x280);
}

//使用于get_efi_device_boot_path，find_specified_file，chainloader_func，command_func，uuid_func
static char chainloader_file[256];

int find_specified_file (int drive, int partition, char* file);
int 
find_specified_file (int drive, int partition, char* file)
{
  int val;
  int tem_drive = current_drive;
  int tem_partition = current_partition;

  if (drive >= 0xa0)  //光盘
    sprintf (chainloader_file, "(%d)%s\0", drive, file);
  else if (drive >= 0x80)  //硬盘
    sprintf (chainloader_file, "(hd%d,%d)%s\0", drive & 0x7f, partition >> 16, file);
  else                //软盘
    sprintf (chainloader_file, "(%d)%s\0", drive, file);
  putchar_hooked = (unsigned char*)1; //不打印ls_func信息
  val = ls_func (chainloader_file, 1);
  putchar_hooked = 0;
  current_partition = tem_partition;
  current_drive = tem_drive;

  return val;
}

int get_efi_device_boot_path (int drive, int flags);
int
get_efi_device_boot_path (int drive, int flags)  //获得硬盘/光盘启动分区(入口)
{
  struct grub_part_data *p;
  char tmp[512];
  char *cache = 0;
  int self_locking = 0;
  int k = 0;
  part_size = 0;
  int v_mbr, v_bpb, i;
  static grub_efi_physical_address_t address;
  grub_efi_status_t status;
  grub_efi_boot_services_t *b = grub_efi_system_table->boot_services;

  cache = grub_zalloc (0x800);	//分配缓存
  if (!cache)
    return 0;

  if (drive >= 0x80 && drive <= 0x8f) //硬盘
  {
  for (p = partition_info; p; p = p->next)
  {
    if (p->drive != drive)
      continue;
    
    if (p->partition_type != 0xee && !self_locking)  //如果是MBR分区类型, 首先启动活动分区
    {
      struct grub_part_data *p_back = p;
      self_locking = 1;

      for (; p && p->drive == drive; p = p->next) //查找活动分区
      {
        if (p->partition_activity_flag == 0x80)
          if (find_specified_file(drive, p->partition, EFI_REMOVABLE_MEDIA_FILE_NAME) == 1)
            goto complete;
      }
      p = p_back;
    }

    if (find_specified_file(drive, p->partition, EFI_REMOVABLE_MEDIA_FILE_NAME) == 1) //搜索文件 bootx64.efi
      goto complete;
  }
  
  grub_free (cache); 
  printf_warning("bootx64.efi not found\n");
  return 0; 
  
complete:
  part_data = p;
  part_addr = p->partition_start;
  part_size = p->partition_size;
  grub_free (cache); 
  return 1;  
  }
  else  //光盘
  {
    cdrom_volume_descriptor_t *vol = (cdrom_volume_descriptor_t *)cache;
    eltorito_catalog0_t *catalog = NULL;
    int no_open;

    if (get_diskinfo (drive, &tmp_geom, 0))	//获得当前驱动器的磁盘信息
    {
      errnum = ERR_NO_DISK;
      goto fail_free_cache;
    }
    if (flags)
    {
      if (!grub_open (chainloader_file_orig))
        goto fail_free_cache;
    }
    else
    {
      grub_sprintf (chainloader_file, "(0x%X)+0x%lX\0", drive, (unsigned long long)tmp_geom.total_sectors);
      if (! grub_open (chainloader_file))
        goto fail_free_cache;
    }

    filepos = 17 * 0x800;
    grub_read ((unsigned long long)(grub_size_t) vol, 0x800, 0xedde0d90);	//读引导描述符扇区(17,规定值,不变),0x800字节
    if (vol->unknown.type != CDVOL_TYPE_STANDARD ||					//启动记录卷,类型 				CDVOL_TYPE_STANDARD=0
        grub_memcmp ((const char *)vol->boot_record_volume.system_id, (const char *)CDVOL_ELTORITO_ID,	//启动记录卷,系统id       CDVOL_ELTORITO_ID="EL TORITO SPECIFICATION"
        sizeof (CDVOL_ELTORITO_ID) - 1) != 0)
      goto fail_close_free_cache;

    catalog = (eltorito_catalog0_t *) cache;
    filepos = *((grub_efi_uint32_t*) vol->boot_record_volume.elt_catalog) * 0x800;  //启动目录文件所在扇区*0x800
    grub_read ((unsigned long long)(grub_size_t) catalog, 0x800, 0xedde0d90); //读启动目录文件扇区 
    if (catalog[0].indicator1 != ELTORITO_ID_CATALOG) //0x01
      goto fail_close_free_cache;

    no_open = 1;
    for (k = 0; catalog[k].indicator88 == ELTORITO_ID_SECTION_BOOTABLE; k++)
    {
#define	BS	((struct master_and_dos_boot_sector *)tmp)
      if (!no_open)
      {
        no_open = 1;
        if (flags)
        {
          if (!grub_open (chainloader_file_orig))   //二次打开，是由于map_func会破坏grub_open环境
          goto fail_free_cache;
        }
        else
        {
          grub_sprintf (chainloader_file, "(0x%X)+0x%lX\0", drive, (unsigned long long)tmp_geom.total_sectors);
          if (! grub_open (chainloader_file))
            goto fail_free_cache;
        }
      }
      filepos = catalog[k].lba * 0x800;
      grub_read ((unsigned long long)(grub_size_t) tmp, 0x200, 0xedde0d90); //读启动映像mbr
      v_mbr = 1;
      v_bpb = 1;
      v_mbr = probe_mbr (BS, 0, 1, 0);
      v_bpb = probe_bpb(BS);

      if (BS->boot_signature != 0xAA55                                             //没有磁盘签名
                || (v_mbr && v_bpb))  //即无分区表，又无BPB表
        continue;

      if (!v_mbr || v_mbr == 4)  //如果有分区表(排除起始扇区为零)
      {
        for (i = 0; i < 4; i++) //
        {
          if (BS->P[i].total_sectors)
          {
            cd_Image_part_start = BS->P[i].start_lba;
            cd_Image_disk_size = BS->P[i].start_lba + BS->P[i].total_sectors;
            break;
          }
        }
      }
      else if (!v_bpb) //如果有BPB表
      {
        if (BS->total_sectors_short)
          cd_Image_disk_size = BS->total_sectors_short;  //FAT12/16 总扇区数
        else              
          cd_Image_disk_size = BS->total_sectors_long;   //FAT32 总扇区数
        cd_Image_part_start = 0;
      }

//      alloc = grub_memalign (512, cd_Image_disk_size << 9);   //有时cd_Image_disk_size太大，分配不了内存
      grub_efi_uintn_t pages;
      pages = (cd_Image_disk_size + 7) >> 3;
      status = efi_call_4 (b->allocate_pages, GRUB_EFI_ALLOCATE_ANY_PAGES,  
            GRUB_EFI_BOOT_SERVICES_DATA,            //启动服务数据        4
			      pages, &address);	//调用(分配页面,分配类型->任意页面,存储类型->运行时服务数据(6),分配页,地址)
      if (status != GRUB_EFI_SUCCESS)	//如果失败
      {
        printf_errinfo ("out of map memory: %x\n",status);
        goto fail_close_free_cache;
      }      

      filepos = catalog[k].lba * 0x800;
      grub_read (address, (unsigned long long)cd_Image_disk_size << 9 , 0xedde0d90);
      grub_close ();
      grub_sprintf (chainloader_file, "(md)0x%lX+0x%lX (0x60)\0", (address >> 9) + cd_Image_part_start, cd_Image_disk_size - cd_Image_part_start); 
      map_func (chainloader_file, 1);
      i = find_specified_file(0x60, 0xffffff, EFI_REMOVABLE_MEDIA_FILE_NAME); //搜索文件 bootx64.efi
      efi_call_2 (b->free_pages, address, pages);	//释放页
      if (i)
        goto aaaa;

      no_open = 0;
#undef BS
    }
    map_func ("--unmap=0x60", 1);
    goto fail_free_cache;

aaaa:
    map_func ("--unmap=0x60", 1);
    cd_boot_entry = k;
    cd_boot_start = catalog[k].lba;												//19*800=c800

    if (!cd_Image_disk_size)
      cd_boot_size = catalog[k].sector_count;						//1*200=200	
    else
    {
      if (!v_mbr && v_bpb && catalog[k].media_type == 4)  //启动扇区是硬盘格式
        cd_boot_size = (cd_Image_disk_size + 3) >> 2;
      else
        cd_boot_size = cd_Image_disk_size;
    }
   
    if (v_mbr && v_mbr != 4)
      cd_Image_disk_size = 0; //没有分区表，则清除
    grub_free (cache); 
    return 1;
  
fail_close_free_cache:
    grub_close ();

fail_free_cache:
    grub_free (cache); 
    printf_warning("bootx64.efi not found\n");
    return 0;    
  }
}

static unsigned char *initrdefi_mem;
static unsigned long long initrdefi_size;

static grub_efi_handle_t initrdefi_lf2_handle = NULL;

static initrd_media_device_path_t initrdefi_lf2_dp =
{
  {
    {
      GRUB_EFI_MEDIA_DEVICE_PATH_TYPE,
      GRUB_EFI_VENDOR_MEDIA_DEVICE_PATH_SUBTYPE,
      sizeof(grub_efi_vendor_media_device_path_t),
    },
    LINUX_EFI_INITRD_MEDIA_GUID
  },
  {
    GRUB_EFI_END_DEVICE_PATH_TYPE,
    GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE,
    sizeof(grub_efi_device_path_t)
  }
};

static grub_efi_status_t EFIAPI
initrdefi_lf2_func(grub_efi_load_file2_t *this,
                   grub_efi_device_path_t *device_path,
                   grub_efi_boolean_t boot_policy,
                   grub_efi_uintn_t *buffer_size,
                   void *buffer);

static grub_efi_load_file2_t initrdefi_lf2 = { initrdefi_lf2_func };

static grub_efi_guid_t initrdefi_lf2_guid = GRUB_EFI_LOAD_FILE2_PROTOCOL_GUID;
static grub_efi_guid_t initrdefi_lf2_dp_guid = GRUB_EFI_DEVICE_PATH_GUID;

#define BYTES_TO_PAGES(bytes)   (((bytes) + 0xfff) >> 12)

static grub_efi_status_t EFIAPI
initrdefi_lf2_func(grub_efi_load_file2_t *this,
                   grub_efi_device_path_t *device_path,
                   grub_efi_boolean_t boot_policy,
                   grub_efi_uintn_t *buffer_size,
                   void *buffer)
{
  grub_efi_status_t status = GRUB_EFI_SUCCESS;

  if (this != &initrdefi_lf2 || buffer_size == NULL)
    return GRUB_EFI_INVALID_PARAMETER;

  if (device_path->type != GRUB_EFI_END_DEVICE_PATH_TYPE ||
      device_path->subtype != GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE)
    return GRUB_EFI_NOT_FOUND;

  if (boot_policy)
    return GRUB_EFI_UNSUPPORTED;

  if (buffer == NULL || *buffer_size < initrdefi_size)
  {
    *buffer_size = (grub_efi_uintn_t) initrdefi_size;
    return GRUB_EFI_BUFFER_TOO_SMALL;
  }

  grub_memcpy(buffer, initrdefi_mem, initrdefi_size);

  return status;
}

/* boot */
static int boot_func (char *arg, int flags);
static int
boot_func (char *arg, int flags)
{
  grub_efi_boot_services_t *b;
  grub_efi_status_t status;
  b = grub_efi_system_table->boot_services;	//引导服务

  if (kernel_type == KERNEL_TYPE_LINUX)
  {
    if (!initrdefi_lf2_handle && initrdefi_mem)
    {
      status = efi_call_6(b->install_multiple_protocol_interfaces,
                          &initrdefi_lf2_handle,
                          &initrdefi_lf2_guid,
                          &initrdefi_lf2,
                          &initrdefi_lf2_dp_guid,
                          &initrdefi_lf2_dp,
                          NULL);
      if (status != GRUB_EFI_SUCCESS)
         printf("ERROR: Failed to install lf2 protocol.\n");
    }
  }

  map_to_svbus(grub4dos_self_address); //为svbus复制插槽
  //不能释放，否则无法启动
  //grub_efi_fini ();
  printf_debug ("StartImage: %x\n", image_handle);				//开始映射
  status = efi_call_3 (b->start_image, image_handle, 0, NULL);			//启动映像
  printf_debug ("StartImage returned 0x%lx\n", (grub_size_t) status);	//开始映射返回
  status = efi_call_1 (b->unload_image, image_handle);		//卸载映射
  return 1;
}

static struct builtin builtin_boot =
{
  "boot",
  boot_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "boot [-1]",
  "Boot the OS/chain-loader which has been loaded."
  "with option \"-1\" will boot to local via INT 18.",
};
#if 0
static grub_efi_status_t (EFIAPI *orig_open_protocol)
                      (grub_efi_handle_t handle,
                       grub_efi_guid_t *protocol,
                       void **interface,
                       grub_efi_handle_t agentHandle,
                       grub_efi_handle_t controllerHandle,
                       grub_efi_uint32_t attributes) = NULL;
grub_efi_guid_t efi_graphics_output_protocol_guid = GRUB_EFI_GOP_GUID;
/**
 * Intercept OpenProtocol()   拦截OpenProtocol
 *
 * @v handle    EFI handle                句柄
 * @v protocol    Protocol GUID           协议GUID 
 * @v interface   Opened interface        开放式接口 
 * @v agent_handle  Agent handle          代理句柄
 * @v controller_handle Controller handle 控制器句柄 
 * @v attributes  Attributes              属性
 * @ret efirc   EFI status code           返回 状态代码
 */
static grub_efi_status_t EFIAPI efi_open_protocol_wrapper (grub_efi_handle_t handle, grub_efi_guid_t *protocol,
                           VOID **interface, grub_efi_handle_t agent_handle,
                           grub_efi_handle_t controller_handle, grub_efi_uint32_t attributes);
static grub_efi_status_t EFIAPI
efi_open_protocol_wrapper (grub_efi_handle_t handle, grub_efi_guid_t *protocol,
                           VOID **interface, grub_efi_handle_t agent_handle,
                           grub_efi_handle_t controller_handle, grub_efi_uint32_t attributes) //打开协议包装器
{
  static unsigned int count;
  grub_efi_status_t status;
  /* Open the protocol */

  if ((status = orig_open_protocol (handle, protocol, interface,
                                   agent_handle, controller_handle,
                                   attributes)) != 0)
                                   {
    return status;
                                   }
  /* Block first attempt by bootmgfw.efi to open                  阻止bootmgfw.efi首次尝试打开EFI_GRAPHICS_OUTPUT_PROTOCOL.
   * EFI_GRAPHICS_OUTPUT_PROTOCOL.  This forces error messages
   * to be displayed in text mode (thereby avoiding the totally   这将强制以文本模式显示错误消息(从而避免在缺少字体时出现完全空白的错误屏幕)
   * blank error screen if the fonts are missing).  We must
   * allow subsequent attempts to succeed, otherwise the OS will  我们必须允许随后的努力取得成功，否则操作系统将无法启动。
   * fail to boot.
   */
  if ((memcmp ((const char *)protocol, (const char *)&efi_graphics_output_protocol_guid,
               sizeof (*protocol)) == 0) && (count++ == 0) &&
               (cursor_state & 1))
  {
    return GRUB_EFI_INVALID_PARAMETER;       //参数无效
  }
  return 0;
}
#endif
static grub_efi_char16_t *cmdline;
static grub_ssize_t cmdline_len;
static grub_efi_handle_t dev_handle;

/* chainloader */
static int chainloader_func (char *arg, int flags);
static int
chainloader_func (char *arg, int flags)
{
	grub_efi_boot_services_t *b;
  char *filename;
	grub_efi_status_t status;
	void *boot_image = 0;
	b = grub_efi_system_table->boot_services;		//引导服务
  static grub_efi_physical_address_t address;
  static grub_efi_uintn_t pages;
  
  grub_memset(chainloader_file, 0, 256);
  set_full_path(chainloader_file,arg,sizeof(chainloader_file)); //设置完整路径(补齐驱动器号,分区号)  /efi/boot/bootx64.efi -> (hd0,0)/efi/boot/bootx64.efi
  errnum = ERR_NONE;
  filename = set_device (chainloader_file); //设置当前驱动器=输入驱动器号, 当前分区=输入分区号, 其余作为文件名 /efi/boot/bootx64.efi
  //没有设备指定.默认到根设备
  if (errnum)
  {
    current_drive = saved_drive;
    filename = arg;
    errnum = 0;
  }
 
  //兼容旧版本 (hd0)+1
  if (*filename == '+')
    *filename = 0;

  //虚拟盘类型
  if (! *filename)
	{
    if (current_drive >= 0xa0)  //如果是光盘，并且存在“/EFI/BOOT/BOOTX64.EFI(BOOTIA32.EFI)”,则优先加载
    {
      if (find_specified_file(current_drive, 0, EFI_REMOVABLE_MEDIA_FILE_NAME) == 1) //
      {
        sprintf (chainloader_file, "(%d)%s\0", current_drive, EFI_REMOVABLE_MEDIA_FILE_NAME);
        arg = chainloader_file;
        goto file;
      }
      errnum = 0;
    } 
    image_handle = grub_load_image (current_drive, EFI_REMOVABLE_MEDIA_FILE_NAME, 0, 0, &dev_handle);	//虚拟磁盘启动
    if (debug > 1)
    {
      grub_efi_loaded_image_t *image0 = grub_efi_get_loaded_image (image_handle);  //通过映像句柄,获得加载映像
      printf_debug ("image=0x%x image_handle=%x",image0,image_handle);
    }
		kernel_type = KERNEL_TYPE_CHAINLOADER;
		return 1;
	}

file:
  //文件类型
  grub_open (arg);
  if (errnum)
		goto failure_exec_format_0;

	pages = ((filemax + ((1 << 12) - 1)) >> 12);	//计算页
	status = efi_call_4 (b->allocate_pages, GRUB_EFI_ALLOCATE_ANY_PAGES,
			      GRUB_EFI_LOADER_CODE,
			      pages, &address);	//调用(分配页面,分配类型->任意页面,存储类型->装载程序代码(1),分配页,地址)

  if (status != GRUB_EFI_SUCCESS)	//失败退出
	{
		printf_errinfo ("Failed to allocate %u pages\n",(unsigned int) pages);
		goto failure_exec_format;
	}

  boot_image = (void *) ((grub_addr_t) address);	//引导镜像地址
	if (grub_read ((unsigned long long)(grub_size_t)boot_image, filemax, 0xedde0d90) != filemax)
	{
		printf_errinfo ("premature end of file %s",arg);
		goto failure_exec_format;
  }

  char *arg1 = arg;
  arg = skip_to(0,arg);	//标记=0/1/100/200=跳过"空格,回车,换行,水平制表符"/跳过等号/跳到下一行/使用'0'替换
  if (*arg)	//如果有命令行
	{
		// UTF-8 转为 UCS-2 编码命令行
		grub_size_t len = grub_strlen ((const char*)arg) + 1;
		cmdline = grub_malloc (cmdline_len);
		if (! cmdline)
			goto failure_exec_format;
		len = grub_utf8_to_ucs2(cmdline, len, (grub_uint8_t *)arg, len, NULL);
		cmdline[len] = 0;
		cmdline_len = len * sizeof (grub_efi_char16_t);
	}

  image_handle = grub_load_image (current_drive, arg1, boot_image, filemax, &dev_handle);
  grub_efi_loaded_image_t *image1 = grub_efi_get_loaded_image (image_handle);  //通过映像句柄,获得加载映像
  //UEFI固件已经设置了“image1->device_handle = d->handle”。他没有分区信息，启动不了某些bootmgfw.efi。必须在此填充对应分区的句柄。
//  d = get_device_by_drive (current_drive,0);
//  image1->device_handle = d->handle;
  image1->device_handle = dev_handle;
  if (cmdline)
  {
		image1->load_options = cmdline;	//加载选项
		image1->load_options_size = cmdline_len;//加载选项尺寸
  }
  printf_debug ("image=%x device_handle=%x",image1,dev_handle);//113b8e40,11b3d398
  grub_close ();	//关闭文件
#if 0
  //拦截对OpenProtocol的调用
  orig_open_protocol = (void *)image1->system_table->boot_services->open_protocol;
  image1->system_table->boot_services->open_protocol = (void *)efi_open_protocol_wrapper; 
#endif
	kernel_type = KERNEL_TYPE_CHAINLOADER;
  return 1;

failure_exec_format:

  grub_close ();

failure_exec_format_0:

  if (errnum == ERR_NONE)
    errnum = ERR_EXEC_FORMAT;

  if (address)
    efi_call_2 (b->free_pages, address, pages);	//释放页

  return 0;		/* return failure */
}

static struct builtin builtin_chainloader =
{
  "chainloader",
  chainloader_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "chainloader FILE",
  "Load the chain-loader FILE.\n"
  "for example:\n"
  "  chainloader (0xff)\n"
  "  chainloader (hd2)\n"
  "  chainloader (bd)/grub.efi \"vdisk=/_ISO/LINUX/Ubuntu Mint.vdi.vtoy\""
};

/* This function could be used to debug new filesystem code. Put a file
   in the new filesystem and the same file in a well-tested filesystem.
   Then, run "cmp" with the files. If no output is obtained, probably
   the code is good, otherwise investigate what's wrong...  */
/* cmp FILE1 FILE2 */
static int cmp_func (char *arg, int flags);
static int
cmp_func (char *arg, int flags)
{
  /* The filenames.  */
  char *file1, *file2;
  /* The addresses.  */
  char *addr1, *addr2;
  int i, Hex;
  /* The size of the file.  */
  unsigned long long size;
  unsigned long long cur_pos = 0;
    quit_print=0;
    Hex = 0;

  errnum = 0;
  for (;;)
  {
		if (grub_memcmp (arg, "--hex", 5) == 0)
		{
			Hex = 1;
			arg=skip_to (0, arg);
		}
		else if (grub_memcmp (arg, "--skip=", 7) == 0)
		{
			arg += 7;
			if (! safe_parse_maxint_with_suffix (&arg, &cur_pos, 0))
				return 0;
		  while (*arg == ' ' || *arg == '\t') arg++;
		}
		else
			break;
   }
  /* Get the filenames from ARG.  */
  file1 = arg;
  file2 = skip_to (0, arg);
  if (! *file1 || ! *file2)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }

  /* Terminate the filenames for convenience.  */
  nul_terminate (file1);
  nul_terminate (file2);

  /* Read the whole data from FILE1.  */
  // At 6M it will be unifont. Use 64K at 1M instead.
  #define CMP_BUF_SIZE 0x8000ULL

  if (! grub_open (file1))
    return 0;
  
  /* Get the size.  */
  size = filemax;
  grub_close();
 
  if (! grub_open (file2))
    return 0;
  grub_close();

  if ((size != filemax) && !Hex)
    {
      grub_printf ("Differ in size: 0x%lx [%s], 0x%lx [%s]\n", size, file1, filemax, file2);
      grub_close ();
      return 0;
    }
    
  if (Hex)
  {
	if (current_term->setcolorstate)
		current_term->setcolorstate (COLOR_STATE_HEADING);
		
	grub_printf("Compare FILE1:%s <--> FILE2:%s\t\n",file1,file2);
	
	if (current_term->setcolorstate)
		current_term->setcolorstate (COLOR_STATE_STANDARD);
  }
  
 
  if (! grub_open (file1))
	return 0;

  unsigned long long size1, size2;
  
  if (Hex)
	filepos = cur_pos & -16ULL;
  else
    filepos = cur_pos;

	addr1 = grub_malloc (0x10000);
	if (!addr1)
		return 0;
  addr2 = addr1 + CMP_BUF_SIZE;
  while ((size1 = grub_read ((unsigned long long)(grub_size_t)addr1, CMP_BUF_SIZE, 0xedde0d90)))
  {
  		cur_pos = filepos;
		grub_close();
		if (! grub_open (file2))
		{
			grub_free (addr1);
			return 0;
		}
		
		filepos = cur_pos - size1;
		if (! (size2 = grub_read ((unsigned long long)(grub_size_t)addr2, size1, 0xedde0d90)))
		{
			grub_close ();
			grub_free (addr1);
			return 0;
		}
		grub_close();
		
		if (Hex)
		{
			for (i = 0; i < (int)size2; i+=16)
			{
				int k,cnt;
				unsigned char c;
				grub_size_t cur_offset;
				cur_offset = (grub_size_t)(cur_pos - size1 + i);
				if (current_term->setcolorstate)
					current_term->setcolorstate (COLOR_STATE_NORMAL);
				grub_printf("0x%X\t0x%lX/0x%lX\n",cur_offset, size, filemax);
				if (current_term->setcolorstate)
					current_term->setcolorstate (COLOR_STATE_STANDARD);
				grub_printf ("%08X: ",cur_offset);
				cnt = 16;
				if (cnt+i > (int)size2)
					cnt=size2 - i;
				for (k=0;k<cnt;k++)
				{
					c = (unsigned char)(addr1[i+k]);
					if ((addr1[i+k] != addr2[i+k]) && (current_term->setcolorstate))
						current_term->setcolorstate (COLOR_STATE_HIGHLIGHT);
					printf("%02X", c);
					if ((addr1[i+k] != addr2[i+k]) && (current_term->setcolorstate))
						current_term->setcolorstate (COLOR_STATE_STANDARD);
					if ((k != 15) && ((k & 3)==3))
						printf(" ");
					printf(" ");
				}
				for (;k<16;k++)
				{
					printf("   ");
					if ((k!=15) && ((k & 3)==3))
						printf(" ");
				}
				printf("; ");

				for (k=0;k<cnt;k++)
				{
					c=(unsigned char)(addr1[i+k]);
					printf("%c",((c>=32) && (c != 127))?c:'.');
				}
				printf("\n");
				grub_printf ("%08X: ",cur_offset);
				for (k=0;k<cnt;k++)
				{
					c = (unsigned char)(addr2[i+k]);
					if ((addr1[i+k] != addr2[i+k]) && (current_term->setcolorstate))
						current_term->setcolorstate (COLOR_STATE_HIGHLIGHT);
					printf("%02X", c);
					if ((addr1[i+k] != addr2[i+k]) && (current_term->setcolorstate))
						current_term->setcolorstate (COLOR_STATE_STANDARD);
					if ((k!=15) && ((k & 3)==3))
						printf(" ");
					printf(" ");
				}

				for (;k<16;k++)
				{
					printf("   ");
					if ((k!=15) && ((k & 3)==3))
						printf(" ");
				}
				printf("; ");

				for (k=0;k<cnt;k++)
				{
					c = (unsigned char)(addr2[i+k]);
					printf("%c",((c>=32) && (c != 127))?c:'.');
				}
				printf("\n");
				if (quit_print)
				{
					grub_free (addr1);
					return 1;
				}
			}
		}
		else
		{
			  /* Now compare ADDR1 with ADDR2.  */
			for (i = 0; i < (int)size2; i++)
			{
				if (addr1[i] != addr2[i])
				{
					grub_printf ("Differ at the offset %d: 0x%x [%s], 0x%x [%s]\n",
						(grub_size_t)(cur_pos - size1 + i), (grub_size_t) addr1[i], file1,
						(grub_size_t) addr2[i], file2);
					grub_free (addr1);
					return 0;
				}
			}
		}
		
		if (! grub_open (file1))
		{
			grub_free (addr1);
			return 0;
		}
		filepos = cur_pos;
  }
	grub_close();
	grub_free (addr1);
	return 1;
}

static struct builtin builtin_cmp =
{
  "cmp",
  cmp_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "cmp [--hex] FILE1 FILE2",
  "Compare the file FILE1 with the FILE2 and inform the different values"
  " if any."
};


static const char *color_list[16] =
{
    "black",
    "blue",
    "green",
    "cyan",
    "red",
    "magenta",
    "brown",
    "light-gray",
    "dark-gray",
    "light-blue",
    "light-green",
    "light-cyan",
    "light-red",
    "light-magenta",
    "yellow",
    "white"
};

static int color_number (char *str);
//int blinking = 1;
  
  /* Convert the color name STR into the magical number.  */
static int color_number (char *str);
static int
color_number (char *str)
{
      char *ptr;
      int i;
      int color = 0;
      
      /* Find the separator.  */
      for (ptr = str; *ptr && *ptr != '/'; ptr++)
	;

      /* If not found, return -1.  */
      if (! *ptr)
	return -1;

      /* Terminate the string STR.  */
      *ptr = 0;
      /* Search for the color name.  */
      for (i = 0; i < 16; i++)
	if (grub_strcmp (color_list[i], str) == 0)
	  {
	    color |= i;
	    break;
	  }
      *ptr++ = '/';
      if (i == 16)
	return -1;

      str = ptr;
      nul_terminate (str);

      /* Search for the color name.  */      
      for (i = 0; i < 16; i++)
	if (grub_strcmp (color_list[i], str) == 0)
	  {
	    color |= i << 4;
	    break;
	  }

      if (i == 16)
	return -1;

      return color;
}

extern int color_counting;
/* color */
/* Set new colors used for the menu interface. Support two methods to
   specify a color name: a direct integer representation and a symbolic
   color name. An example of the latter is "blink-light-gray/blue".  */
static int color_func (char *arg, int flags);
static int
color_func (char *arg, int flags)
{
  char *normal;
  unsigned long long new_color[COLOR_STATE_MAX];
  unsigned long long new_normal_color;
  int _64bit = 0;

  errnum = 0;
  if (! *arg)
  {
		grub_printf("8_bits  current  normal  highlight  helptext  heading  border  standard\n");
		grub_printf("        %02x       %02x      %02x         %02x        %02x       %02x      %02x\n",current_color,console_color[1],console_color[2],console_color[3],console_color[4],console_color[5],console_color[0]);
		grub_printf("64_bits current           normal            highlight\n");
		grub_printf("        %016lx  %016lx  %016lx\n",current_color_64bit,console_color_64bit[1],console_color_64bit[2]);
		grub_printf("        helptext          heading           border            standard\n");
		grub_printf("        %016lx  %016lx  %016lx  %016lx",console_color_64bit[3],console_color_64bit[4],console_color_64bit[5],console_color_64bit[0]);
    return 1;
  }

  if (!(current_term->setcolor))
      return 0;
//  blinking = 1;
	
	if (memcmp(arg,"--64bit",7) == 0)
	{
		_64bit = 1;
		arg = skip_to (0, arg);
	}
	
  normal = arg;
  arg = skip_to (0, arg);

  new_normal_color = (unsigned long long)(long long)color_number (normal);
  if ((int)new_normal_color < 0 && ! safe_parse_maxint (&normal, &new_normal_color))
  {
	color_state state_t;
	unsigned int state = 0;
	int tag = 0;
	arg = normal;
	while (*arg)
	{
		if (memcmp(arg,"normal",6) == 0)
		{
			state_t = COLOR_STATE_NORMAL;
			if (color_counting == 0)
				tag = 1;
		}
		else if (memcmp(arg,"highlight",9) == 0)
		{
			state_t = COLOR_STATE_HIGHLIGHT;
		}
		else if (memcmp(arg,"helptext",8) == 0)
		{
			state_t = COLOR_STATE_HELPTEXT;
		}
		else if (memcmp(arg,"heading",7) == 0)
		{
			state_t = COLOR_STATE_HEADING;
		}
		else if (memcmp(arg,"standard",8) == 0)
		{
			state_t = COLOR_STATE_STANDARD;
		}
		else if (memcmp(arg,"border",6) == 0)
		{
			state_t = COLOR_STATE_BORDER;
		}
		else
			return 0;
		normal = skip_to(1,arg);
		arg = skip_to(0,normal);
		if (!safe_parse_maxint (&normal, &new_color[state_t]))
		{
		    new_normal_color = (unsigned long long)(long long)color_number (normal);
		    if ((int)new_normal_color< 0)
			return 0;
		    new_color[state_t] = new_normal_color ;
		}

		if (!(new_color[state_t] >> 8) && _64bit == 0)
			new_color[state_t] = color_8_to_64 (new_color[state_t]);

		if (tag && color_counting==0)
		{		
			new_color[COLOR_STATE_HEADING] = new_color[COLOR_STATE_HELPTEXT] = new_color[state_t];		
			new_color[COLOR_STATE_HIGHLIGHT] = 0xffffff | ((splashimage_loaded & 2)?0:(new_color[state_t] & 0xffffffff00000000));			
			state |= (1<<COLOR_STATE_HEADING | 1<<COLOR_STATE_HELPTEXT | 1<<COLOR_STATE_HIGHLIGHT);
		}
		
		state |= 1<<state_t;
		color_counting++;
		if (tag && !(splashimage_loaded & 2) && ((new_color[state_t] & 0xffffffff00000000) == 0))
			new_color[state_t] |= (new_color[COLOR_STATE_NORMAL] & 0xffffffff00000000);
	}

	current_term->setcolor (state,new_color);
	errnum = 0;
	return 1;
  }

	if (!(new_normal_color >> 8) && _64bit == 0)
		new_normal_color = color_8_to_64 (new_normal_color);

	if (!*arg && (flags & (BUILTIN_CMDLINE | BUILTIN_BAT_SCRIPT)))
	{
		current_term->setcolor (1,&new_normal_color);
		return 1;
	}

  new_color[COLOR_STATE_HEADING] = new_color[COLOR_STATE_HELPTEXT] = new_color[COLOR_STATE_NORMAL] = new_normal_color;
  /* The second argument is optional, so set highlight_color
     to inverted NORMAL_COLOR.  */
		new_color[COLOR_STATE_HIGHLIGHT] = 0xffffff | ((splashimage_loaded & 2)?0:(new_normal_color & 0xffffffff00000000));

	if (*arg)
	{
		int i;
		for (i=COLOR_STATE_HIGHLIGHT;i<=COLOR_STATE_HEADING && *arg;++i)
		{
			normal = arg;
			arg = skip_to (0, arg);
			if (*normal == 'n')
				continue;
			new_color[i] = (unsigned long long)(long long)color_number (normal);
			if (((int)new_color[i] < 0) && ! safe_parse_maxint (&normal, &new_color[i]))
				return 0;

			if (!(new_color[i] >> 8) && _64bit == 0)
				new_color[i] = color_8_to_64 (new_color[i]);
			
			if (!(splashimage_loaded & 2) && ((new_color[i] & 0xffffffff00000000) == 0))
				new_color[i] |= (new_color[COLOR_STATE_NORMAL] & 0xffffffff00000000);
		}
	}

  current_term->setcolor (0x1E,new_color);

  return 1;
}

static struct builtin builtin_color =
{
  "color",
  color_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "color NORMAL [HIGHLIGHT [HELPTEXT [HEADING ]]]\n",
  "Change the menu colors. The color NORMAL is used for most lines in the menu,\n"
  "  and the color HIGHLIGHT is used to highlight the line where the cursor points.\n"
  "If you omit HIGHLIGHT, then the 0xffffff(32 bit) is used for the highlighted line.\n"
  "If you omit HELPTEXT and/or HEADING, then NORMAL is used.\n"
  "1. Assign colors by target, the order can not be messed up.\n"
  "   The color can be replaced by a placeholder n.\n"
	"e.g. color 0x888800000000 0x888800ffff00 0x888800880000 0x88880000ff00. (64 bit number.)\n"
	"2. Can assign colors to a specified target. NORMAL should be in the first place.\n"
	"e.g. color normal=0x888800000000. (The rest is the same as NORMAL.)\n"
	"e.g. color normal=0x4444440000ffff helptext=0xff0000 highlight=0x00ffff heading=0xffff00\n"
	"     border=0x00ff00. (Background color from NORMAL.)\n"
	"e.g. color standard=0xFFFFFF. (Change the console color.)\n"
	"e.g. color --64bit 0x30. (Make numbers less than 0x100 treated in 64-bit color.)\n"
	"Display color list if no parameters.\n"
	"Use 'echo -rrggbb' to view colors."
};


/* configfile */
static int configfile_func (char *arg, int flags);
static int
configfile_func (char *arg, int flags)
{
  errnum = 0;
	graphic_type = 0;

	if (flags & BUILTIN_USER_PROG)  //内置用户程序
	{
		if (! grub_open (arg))
				return 0;
		grub_close();
		return sprintf(CMD_RUN_ON_EXIT,"\xEC configfile %.128s",arg);
	}
  char *new_config = config_file;
	if (*arg == 0 && *config_file)
	{
#if 0
	    if	(pxe_restart_config == 0)
	    {
		if (configfile_in_menu_init == 0)
			pxe_restart_config = configfile_in_menu_init = 1;
		return 1;
	    }
	    /* use the original config file */
	    saved_drive = boot_drive;
	    saved_partition = install_partition;
	    *saved_dir = 0;	/* clear saved_dir */
	    arg = config_file;
#else
    return 1;
#endif
	}
  if (grub_strlen(saved_dir) + grub_strlen(arg) + 20 >= (int)sizeof(chainloader_file_orig))
	return ! (errnum = ERR_WONT_FIT);

  grub_memset(chainloader_file_orig, 0, 256);
  set_full_path(chainloader_file_orig,arg,sizeof(chainloader_file_orig));

  //chainloader_file_orig[sizeof(chainloader_file_orig) - 1] = 0;
  arg = chainloader_file_orig;
  nul_terminate (arg);
  /* check possible filename overflow */
	if (grub_strlen (arg) >= 0x49)  //0x8217-0x825f
	return ! (errnum = ERR_WONT_FIT);

  /* Check if the file ARG is present.  */
  if (! grub_open (arg))
  {
		return 0;
  } else
  {
	/* Copy ARG to CONFIG_FILE.  */
	while ((*new_config++ = *arg++));
	grub_close ();
  }

  /* Force to load the configuration file.  */
  use_config_file = 1;
//	pxe_restart_config = 1;		//pxe测试
#if 0
	if (pxe_restart_config == 0)	//pxe重新启动配置
	{
		pxe_restart_config = /* configfile_in_menu_init = */ 1;
		return 1;
	}
#endif
  /* Make sure that the user will not be authoritative.  */
  auth = 0;
  
  saved_entryno = 0;
  /* should not clear saved_dir. see issue 109 reported by ruymbeke. */
  if (current_drive != 0xFFFF && (current_drive != ram_drive || filemax != rd_size))
  {
    boot_drive = current_drive;
    install_partition = current_partition;
  }
  if (animated_type)
    splashimage_func("\0",1); //切换菜单时.避免动画背景残留

  cmain ();
  /* Never reach here.  */
  return 1;
}

static struct builtin builtin_configfile =
{
  "configfile",
  configfile_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "configfile FILE",
  "Load FILE as the configuration file."
};

static grub_efi_status_t
connect_all_efi (void)
{
  grub_efi_status_t status;
  grub_efi_uintn_t handle_count;
  grub_efi_handle_t *handle_buffer;
  grub_efi_uintn_t index;
  grub_efi_boot_services_t *b = grub_efi_system_table->boot_services;
  status = efi_call_5 (b->locate_handle_buffer,
                       GRUB_EFI_ALL_HANDLES,
                       NULL, NULL,
                       &handle_count, &handle_buffer);
  if (status != GRUB_EFI_SUCCESS)
    return status;
  for (index = 0; index < handle_count; index++)
    status = efi_call_4 (b->connect_controller,
                         handle_buffer[index], NULL, NULL, TRUE);
  if (handle_buffer)
    efi_call_1 (b->free_pool, handle_buffer);
  return GRUB_EFI_SUCCESS;
}

/* load */
static int load_func (char *arg, int flags);
static int
load_func (char *arg, int flags)
{
  int connect = 1;
  grub_efi_status_t status;
  grub_efi_boot_services_t *b = grub_efi_system_table->boot_services;
  grub_efi_physical_address_t address;
  grub_efi_uintn_t pages = 0;
  grub_size_t size;
  void *driver_img = NULL;
  grub_efi_handle_t driver_handle;
  grub_efi_loaded_image_t *loaded_image;
  grub_efi_guid_t loaded_image_protocol_guid = GRUB_EFI_LOADED_IMAGE_PROTOCOL_GUID;

  if (!*arg)
    goto load_fail;
  if (grub_memcmp (arg, "-n", 2) == 0)
  {
    connect = 0;
    arg = skip_to (0, arg);
  }
  grub_open (arg);
  if (errnum)
  {
    printf_errinfo ("Failed to open %s\n", arg);
    goto load_fail;
  }
  size = filemax;
  pages = BYTES_TO_PAGES (size);
  status = efi_call_4 (b->allocate_pages, GRUB_EFI_ALLOCATE_ANY_PAGES,
                       GRUB_EFI_LOADER_CODE, pages, &address);
  if (status != GRUB_EFI_SUCCESS)
  {
    grub_close ();
    printf_errinfo ("Failed to allocate pages\n");
    goto load_fail;
  }
  driver_img = (void *) ((grub_addr_t) address);
  if (grub_read ((unsigned long long)(grub_size_t)driver_img,
                  filemax, 0xedde0d90) != filemax)
  {
    grub_close ();
    printf_errinfo ("premature end of file %s", arg);
    goto load_fail;
  }
  grub_close ();

  status = efi_call_6 (b->load_image, 0, grub_efi_image_handle, NULL,
                       driver_img, size, &driver_handle);
  if (status != GRUB_EFI_SUCCESS)
  {
    printf_errinfo ("cannot load image\n");
    goto load_fail;
  }
  loaded_image = grub_efi_get_loaded_image (driver_handle);
  if (!loaded_image)
  {
    printf_errinfo ("no loaded image available\n");
    goto load_fail;
  }
  status = efi_call_3 (b->handle_protocol, driver_handle,
                       &loaded_image_protocol_guid, (void **)&loaded_image);
  if (status != GRUB_EFI_SUCCESS)
  {
    printf_errinfo ("Not a dirver\n");
    goto load_fail;
  }
  status = efi_call_3 (b->start_image, driver_handle, NULL, NULL);
  if (status != GRUB_EFI_SUCCESS)
  {
    printf_errinfo ("ERROR in StartImage\n");
    goto load_fail;
  }
  if (connect)
  {
    status = connect_all_efi ();
    if (status != GRUB_EFI_SUCCESS)
    {
      printf_errinfo ("ERROR in connect_all_efi\n");
      goto load_fail;
    }
  }
  return 1;

load_fail:
  if (driver_img)
    efi_call_2 (b->free_pages, (grub_size_t)driver_img, pages);
  return 0;
}

static struct builtin builtin_load =
{
  "load",
  load_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "load [-n] FILE",
  "Load FILE as EFI driver."
};

/* dd if=IF of=OF */
static int dd_func (char *arg, int flags);
static int
dd_func (char *arg, int flags)
{
  char *p;
  char *in_file = NULL, *out_file = NULL;
  unsigned long long bs = 0;
  unsigned long long count = 0;
  unsigned long long skip = 0;
  unsigned long long seek = 0;
  unsigned long long old_part_start = part_start;
  unsigned long long old_part_length = part_length;
  unsigned int in_drive;
  unsigned int in_partition;
  unsigned int out_drive;
  unsigned int out_partition;
  unsigned long long in_filepos;
  unsigned long long in_filemax;
  unsigned long long out_filepos;
  unsigned long long out_filemax;
  unsigned long long buf_size = 0x10000ULL;
  char tmp_in_file[16];
  char tmp_out_file[16];
  char *buf_addr = 0;
  int SameFile_MoveBack = 0;
  
  errnum = 0;
  for (;;)
  {
    if (grub_memcmp (arg, "if=", 3) == 0)
      {
	if (in_file)
		return !(errnum = ERR_BAD_ARGUMENT);
	in_file = arg + 3;
	if (/* *in_file != '/' &&*/ *in_file != '(')
		return !(errnum = ERR_DEV_FORMAT);
      }
    else if (grub_memcmp (arg, "of=", 3) == 0)
      {
	if (out_file)
		return !(errnum = ERR_BAD_ARGUMENT);
	out_file = arg + 3;
	if (/* *out_file != '/' &&*/ *out_file != '(')
		return !(errnum = ERR_DEV_FORMAT);
      }
    else if (grub_memcmp (arg, "bs=", 3) == 0)
      {
	if (bs)
		return !(errnum = ERR_BAD_ARGUMENT);
	p = arg + 3;
	if (*p == '-')
		return !(errnum = ERR_BAD_ARGUMENT);
	if (! safe_parse_maxint (&p, &bs))
		return 0;
	if (bs == 0 /*|| 0x100000 % bs*/)
		return !(errnum = ERR_BAD_ARGUMENT);
      }
    else if (grub_memcmp (arg, "count=", 6) == 0)
      {
	if (count)
		return !(errnum = ERR_BAD_ARGUMENT);
	p = arg + 6;
	if (*p == '-')
		return !(errnum = ERR_BAD_ARGUMENT);
	if (! safe_parse_maxint (&p, &count))
		return 0;
	if (count == 0)
		return !(errnum = ERR_BAD_ARGUMENT);
      }
    else if (grub_memcmp (arg, "skip=", 5) == 0)
      {
	if (skip)
		return !(errnum = ERR_BAD_ARGUMENT);
	p = arg + 5;
	if (*p == '-')
		return !(errnum = ERR_BAD_ARGUMENT);
	if (! safe_parse_maxint (&p, &skip))
		return 0;
      }
    else if (grub_memcmp (arg, "seek=", 5) == 0)
      {
	if (seek)
		return !(errnum = ERR_BAD_ARGUMENT);
	p = arg + 5;
	if (*p == '-')
		return !(errnum = ERR_BAD_ARGUMENT);
	if (! safe_parse_maxint (&p, &seek))
		return 0;
      }
    else if (*arg)
		return !(errnum = ERR_BAD_ARGUMENT);
    else
	break;
    arg = skip_to (0, arg);
  }
  
  if (! in_file || ! out_file)
	return !(errnum = ERR_BAD_ARGUMENT);
  if (bs == 0)
	bs = 512;

  {
	p = set_device (in_file);
	if (errnum)
		goto fail;
	if (! p)
	{
		if (errnum == 0)
			errnum = ERR_BAD_ARGUMENT;
		goto fail;
	}
	in_drive = current_drive;
	in_partition = current_partition;
	/* if only the device portion is specified */
	if ((unsigned char)*p <= ' ')
	{
		in_file = p = tmp_in_file;
		*p++ = '(';
		*p++ = ')';
		*p++ = '+';
		*p++ = '1';
		*p = 0;
		current_drive = saved_drive;
		current_partition = saved_partition;
		saved_drive = in_drive;
		saved_partition = in_partition;
		in_drive = current_drive;
		in_partition = current_partition;
		grub_open (in_file);
		current_drive = saved_drive;
		current_partition = saved_partition;
		saved_drive = in_drive;
		saved_partition = in_partition;
		in_drive = current_drive;
		in_partition = current_partition;
		if (errnum)
			goto fail;
		in_filemax = (unsigned long long)(buf_geom.sector_size) * part_length;
		grub_sprintf (in_file + 3, "0x%lx", (unsigned long long)part_length);
		grub_close ();
	}
	else
	{
		grub_open (in_file);
		in_filemax = filemax;
		if (errnum)
			goto fail;
		grub_close ();
	}
  }

  {
	p = set_device (out_file);
	if (errnum)
		goto fail;
	if (! p)
	{
		if (errnum == 0)
			errnum = ERR_BAD_ARGUMENT;
		goto fail;
	}
	out_drive = current_drive;
	out_partition = current_partition;
	/* if only the device portion is specified */
	if ((unsigned char)*p <= ' ')
	{
		out_file = p = tmp_out_file;
		*p++ = '(';
		*p++ = ')';
		*p++ = '+';
		*p++ = '1';
		*p = 0;
		current_drive = saved_drive;
		current_partition = saved_partition;
		saved_drive = out_drive;
		saved_partition = out_partition;
		out_drive = current_drive;
		out_partition = current_partition;
		grub_open (out_file);
		current_drive = saved_drive;
		current_partition = saved_partition;
		saved_drive = out_drive;
		saved_partition = out_partition;
		out_drive = current_drive;
		out_partition = current_partition;
		if (errnum)
			goto fail;
		out_filemax = (unsigned long long)(buf_geom.sector_size) * part_length;
		grub_sprintf (out_file + 3, "0x%lx", (unsigned long long)part_length);
		grub_close ();
	}
	else
	{
		grub_open (out_file);
		out_filemax = filemax;
		if (errnum)
			goto fail;
		grub_close ();
	}
  }

  /* calculate in_filepos and out_filepos */
  in_filepos = skip * bs;
  out_filepos = seek * bs;
  if (count)
  {
	if (in_filemax > ((count + skip) * bs))
	    in_filemax = ((count + skip) * bs);
	if (out_filemax > ((count + seek) * bs))
	    out_filemax = ((count + seek) * bs);
  }

  if (in_drive == 0xFFFF && in_file == tmp_in_file &&	/* in_file is (md) */
      out_drive == 0xFFFF && out_file == tmp_out_file)	/* out_file is (md) */
  {
	unsigned long long tmp_part_start = part_start;
	unsigned long long tmp_part_length = part_length;

	count = in_filemax - in_filepos;
	if (count > out_filemax - out_filepos)
	    count = out_filemax - out_filepos;

	part_start = old_part_start;
	part_length = old_part_length;
	grub_memmove64 (out_filepos, in_filepos, count);
	part_start = tmp_part_start;
	part_length = tmp_part_length;
	printf_debug ("\nMoved 0x%lX bytes from 0x%lX to 0x%lX\n", (unsigned long long)count, (unsigned long long)in_filepos, (unsigned long long)out_filepos);
	errnum = 0;
	return count;
  }

  /* (*p == '/') indicates out_file is not a block file */
  /* (*p != '/') indicates out_file is a block file */

#if 0
  if (out_drive != ram_drive && out_drive != 0xFFFF && *p != '/')
  {
	unsigned int j;

	/* check if it is a mapped memdrive */
	j = DRIVE_MAP_SIZE;		/* real drive */
	    for (j = 0; j < DRIVE_MAP_SIZE; j++)
	    {
		if (drive_map_slot_empty (disk_drive_map[j]))   //判断驱动器映像插槽是否为空   为空,返回1
		{
			j = DRIVE_MAP_SIZE;	/* real drive */
			break;
		}

		if (out_drive == disk_drive_map[j].from_drive && disk_drive_map[j].to_drive == 0xFF && !(disk_drive_map[j].to_log2_sector != 11))
			break;			/* memdrive */
	    }

	if (j == DRIVE_MAP_SIZE)	/* real drive 如果不是映射驱动器)*/
	{
	    /* this command is intended for running in command line and inhibited from running in menu.lst */
      //此函数只允许在命令行运行, 禁止在菜单
	    if (flags & (BUILTIN_MENU | BUILTIN_SCRIPT))
	    {
		errnum = ERR_WRITE_TO_NON_MEM_DRIVE;
		goto fail;
	    }
	}
  }
#endif
  {
    unsigned long long in_pos = in_filepos;
    unsigned long long out_pos = out_filepos;
    unsigned long long tmp_size = buf_size;
    unsigned int in_count = (unsigned int)(in_filemax - in_filepos + buf_size - 1) / (unsigned int)buf_size;

    if (debug > 0)
    {
	count = in_filemax - in_filepos;
	if (count > out_filemax - out_pos)
	    count = out_filemax - out_pos;
	count = ((unsigned int)(count + buf_size - 1) / (unsigned int)buf_size);
	printf_debug ("buf_size=0x%lX, loops=0x%lX. in_pos=0x%lX, out_pos=0x%lX\n", (unsigned long long)buf_size, (unsigned long long)count, (unsigned long long)in_pos, (unsigned long long)out_pos);
    }

  //以终止符替换空格
  nul_terminate (in_file);  
  nul_terminate (out_file);
  //同一文件向后移动, 可能会覆盖数据!!!  补丁修复此bug
  if (substring (out_file, in_file, 0) == 0   //返回: 0/1/-1=s1与s2相等/s1不是s2的子字符串/s1是s2的子字符串
          && in_drive == out_drive
          && in_partition == out_partition
          && in_count
          && in_pos < out_pos)
  {
    SameFile_MoveBack = 1;  //同一文件向后移动标记
  }

//    count = 0;
    buf_addr = grub_malloc (buf_size);
    if (!buf_addr)
      return 0;

    while (in_pos < in_filemax && out_pos < out_filemax)
    {
#if 0
	if (debug > 0)
	{
		if (!((char)count & 7))
			grub_printf ("\r");
		grub_printf ("%08X ", (unsigned int)(count));
	}
#endif
	/* open in_file */
	current_drive = saved_drive;
	current_partition = saved_partition;
	saved_drive = in_drive;
	saved_partition = in_partition;
	in_drive = current_drive;
	in_partition = current_partition;
	current_drive = saved_drive;
	current_partition = saved_partition;
  tmp_size = buf_size;
	if (grub_open (in_file))
	{
    if (SameFile_MoveBack)
      in_pos = (in_count - 1) * buf_size + in_filepos;
		filepos = in_pos;
		if (tmp_size > in_filemax - in_pos)
		    tmp_size = in_filemax - in_pos;
		if (grub_read ((unsigned long long)(grub_size_t)buf_addr, tmp_size, 0xedde0d90) != tmp_size)	/* read */
		{
			if (errnum == 0)
				errnum = ERR_READ;
		}
		{
			int err = errnum;
			grub_close ();
			errnum = err;
		}
	}
	current_drive = saved_drive;
	current_partition = saved_partition;
	saved_drive = in_drive;
	saved_partition = in_partition;
	in_drive = current_drive;
	in_partition = current_partition;
	if (errnum)
		goto end;

  if (SameFile_MoveBack)
    in_pos -= tmp_size;
  else
    in_pos += tmp_size;
	
	/* open out_file */
	current_drive = saved_drive;
	current_partition = saved_partition;
	saved_drive = out_drive;
	saved_partition = out_partition;
	out_drive = current_drive;
	out_partition = current_partition;
	current_drive = saved_drive;
	current_partition = saved_partition;
	if (grub_open (out_file))
	{
    if (SameFile_MoveBack)
    {
      out_pos = (in_count - 1) * buf_size + out_filepos;
      if (out_pos >= out_filemax || out_pos < out_filepos)
      {
        out_pos = tmp_size;
        goto asd;
      }
    }
    
		filepos = out_pos;
		if (tmp_size > out_filemax - out_pos)
		    tmp_size = out_filemax - out_pos;
		if (grub_read ((unsigned long long)(grub_size_t)buf_addr, tmp_size, 0x900ddeed) != tmp_size)	/* write */
		{
			if (errnum == 0)
				errnum = ERR_WRITE;
		}
asd:
		{
			int err = errnum;
			grub_close ();
			errnum = err;
		}
	}
	current_drive = saved_drive;
	current_partition = saved_partition;
	saved_drive = out_drive;
	saved_partition = out_partition;
	out_drive = current_drive;
	out_partition = current_partition;
	if (errnum)
		goto end;

  if (SameFile_MoveBack)
    out_pos -= tmp_size;
  else
    out_pos += tmp_size;

//	count++;
	in_count--;
    }

end:
    if (SameFile_MoveBack)
    {
      in_pos = in_filemax - in_filepos;
      out_pos = out_filemax - out_filepos;
    }
    else {
    in_pos -= in_filepos;
    out_pos -= out_filepos;}

    if (debug > 0)
    {
	int err = errnum;
	printf_debug ("\nBytes read / written = 0x%lX / 0x%lX\n", (unsigned long long)in_pos, (unsigned long long)out_pos);
	errnum = err;
    }
  }

fail:

  if (buf_addr)
    grub_free (buf_addr);
  return !(errnum);
}

static struct builtin builtin_dd =
{
  "dd",
  dd_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "dd if=IF of=OF [bs=BS] [count=C] [skip=IN] [seek=OUT]",
  "Copy file IF to OF. BS is blocksize, default to 512. C is blocks to copy,"
  " default is total blocks in IF. IN specifies number of blocks to skip when"
  " read, default is 0. OUT specifies number of blocks to skip when write,"
  " default is 0. Skipped blocks are not touched. Both IF and OF must exist."
  " dd can neither enlarge nor reduce the size of OF, the leftover tail"
  " of IF will be discarded. OF cannot be a gzipped file. If IF is a gzipped"
  " file, it will be decompressed automatically when copying. dd is dangerous,"
  " use at your own risk. To be on the safe side, you should only use dd to"
  " write a file in memory."
};


/* debug */
unsigned int ctrl_c_trap = 1;
static int debug_func (char *arg, int flags);
static int
debug_func (char *arg, int flags)
{
  unsigned long long tmp_debug;

  errnum = 0;
  if (! *arg)
  {
    ///* If ARG is empty, toggle the flag.  */
    //debug = ! debug;
  }
  else if (grub_memcmp (arg, "on", 2) == 0)
    debug = 2;
  else if (grub_memcmp (arg, "normal", 6) == 0)
    debug = 1;
  else if (grub_memcmp (arg, "off", 3) == 0)
    debug = 0;
  else if (grub_memcmp (arg, "status", 6) == 0)
    grub_printf (" Debug is now %d\n", (unsigned long)debug);
  else if (grub_memcmp (arg ,"msg=", 4) == 0)
  {
    debug_msg = arg[4] & 7;
  }
  else if (grub_memcmp (arg, "ctrl-c-trap=", 12) == 0)
  {
    arg += 12;
    if (safe_parse_maxint (&arg, &tmp_debug))
      ctrl_c_trap = tmp_debug;
  }
  else if (safe_parse_maxint (&arg, &tmp_debug))
  {
    debug = tmp_debug;
  }
  else
  {
    int ret;
    debug_prog = 1;
    debug_bat = 1; 
    ret = command_func(arg,flags);
    debug_prog = 0;
    debug_bat = 0;
//    debug_check_memory = 0;
    return ret;
  }

  return debug;
}

struct builtin builtin_debug =
{
  "debug",
  debug_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "debug [on | off | normal | status | ctrl-c-trap= | INTEGER]"
  "\ndebug Batch [ARGS]"
  "\ndebug msg=N",
  "Turn on/off or display/set the debug level or Single-step Debug for batch script"
  "\nmsg=N,sets the message level: 0:off,1-3:on."
  "\nctrl-c-trap=0 (do not check); ctrl-c-trap=1 *(check for ctrl+c)."
};


/* default */
static int default_func (char *arg, int flags);
static int
default_func (char *arg, int flags)
{
  unsigned long long ull;
  int len;
  char *p;

  errnum = 0;
  if (grub_memcmp (arg, "saved", 5) == 0)
    {
	if (! *config_file)
	{
		default_entry = saved_entryno;
		return 1;
	}

	*default_file = 0;	/* initialise default_file */
	grub_strncat (default_file, config_file, sizeof (default_file));
	{
	    int i;
	    for (i = grub_strlen (default_file); i >= 0; i--)
		if (default_file[i] == '/')
			break;
	    default_file[++i] = 0;
	    grub_strncat (default_file + i, "default", sizeof (default_file) - i);
	}

	if (grub_open (default_file))
	{
	    char buf[10]; /* This is good enough.  */
	  
	    p = buf;
	    len = grub_read ((unsigned long long)(grub_size_t)buf, sizeof (buf), 0xedde0d90);
	    printf_debug("len=%d", (unsigned long)len);
	    if (len > 0)
	    {
		buf[sizeof (buf) - 1] = 0;
		safe_parse_maxint (&p, &ull);
		saved_entryno = ull;
	    }

	    grub_close ();
	}

	default_entry = saved_entryno;
	return 1;
    }
  
  if (safe_parse_maxint (&arg, &ull))
    {
      default_entry = ull;
      return 1;
    }

  errnum = ERR_NONE;
  
  /* Open the file.  */
  if (! grub_open (arg))
    return ! errnum;

  if (compressed_file)
  {
    grub_close ();
    return ! (errnum = ERR_DEFAULT_FILE);
  }

  len = grub_read ((unsigned long long)(grub_size_t)mbr, SECTOR_SIZE, 0xedde0d90);
  grub_close ();
  

  if (len < 180 || filemax > 2048)
    return ! (errnum = ERR_DEFAULT_FILE);

  /* check file content for safety */
  p = mbr;
  while (p < mbr + len - 100 && grub_memcmp (++p, warning_defaultfile, 73));

  if (p > mbr + len - 160)
    return ! (errnum = ERR_DEFAULT_FILE);

  len = grub_strlen (arg);
  if (len >= (int)sizeof (default_file) /* DEFAULT_FILE_BUFLEN */)
    return ! (errnum = ERR_WONT_FIT);
  
  grub_memmove (default_file, arg, len);
  default_file[len] = 0;
  boot_drive = current_drive;
  install_partition = current_partition;
  
  p = mbr;
  if (safe_parse_maxint (&p, &ull))
    {
      default_entry = ull;
      return 1;
    }

  errnum = 0;		/* ignore error */
  return errnum;	/* return false */
}

static struct builtin builtin_default =
{
  "default",
  default_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "default [NUM | `saved' | FILE]",
  "Set the default entry to entry number NUM (if not specified, it is"
  " 0, the first entry), or to the entry number saved by savedefault if"
  " the key word `saved\' is specified, or to the entry number previously"
  " saved in the specified file FILE. When FILE is specified, all subsequent"
  " `savedefault\' commands will save default entry numbers into"
  " FILE."
};

//static int terminal_func (char *arg, int flags);

#ifdef SUPPORT_GRAPHICS
char splashimage[128] = {0};
int graphicsmode_func (char *arg, int flags);
unsigned int X_offset,Y_offset;
unsigned char animated_type=0;           //bit 0-3:times   bit 4:repeat forever  bit 7:transparent background  type=00:disable
unsigned short animated_delay;
unsigned char animated_last_num;
unsigned short animated_offset_x;
unsigned short animated_offset_y;
char animated_name[128];
unsigned int fill_color;
int background_transparent=0;

int splashimage_func(char *arg, int flags);
int
splashimage_func(char *arg, int flags)
{
  errnum = 0;
  /* If ARG is empty, we reset SPLASHIMAGE.  */
  unsigned long long val;
  int backup_x, backup_y;
  X_offset=0,Y_offset=0;
  fill_color = 0;
  if (*arg)
  {
    if (strlen(arg) > 127)
      return ! (errnum = ERR_WONT_FIT);
   
    if (grub_memcmp (arg, "--offset=", 9) == 0)	//--offset=type=x=y
    {
      arg += 9;
      if (safe_parse_maxint (&arg, &val))
        if ((val & 0x80) == 0x80)
          background_transparent=1;
      arg++;
      if (safe_parse_maxint (&arg, &val))
        X_offset = val;
      arg++;
      if (safe_parse_maxint (&arg, &val))
        Y_offset = val;
      arg = skip_to (0, arg);
    } 
    else if (grub_memcmp (arg, "--fill-color=", 13) == 0)
    {
      if (graphics_mode < 0xFF)
        return !(errnum = ERR_NO_VBE_BIOS);
      arg += 13;
      if (safe_parse_maxint (&arg, &val))
      {
        fill_color = val;
//      vbe_fill_color(fill_color);
      goto fill;
      }
      return 0;
    }
    else if (grub_memcmp (arg, "--animated=", 11) == 0)
    {
      arg += 11;
      if (safe_parse_maxint (&arg, &val))
      {
        animated_type = val;
        if (!animated_type)
          return 1;
      }
      arg++;
      if (safe_parse_maxint (&arg, &val))
      {
        if (arg[0]==':' && arg[1]=='m' && arg[2]=='s')
        {
          animated_delay = val;
          arg += 3;
        }
        else
          animated_delay = val * 55;
      }
      arg++;
      if (safe_parse_maxint (&arg, &val))
        animated_last_num = val;
      arg++;
      if (safe_parse_maxint (&arg, &val))
        animated_offset_x = val;
      arg++;
      if (safe_parse_maxint (&arg, &val))
        animated_offset_y = val;
      arg = skip_to (0, arg);
		
      strcpy(animated_name, arg);
      animated();
      return 1;
    }    
	}
  strcpy(splashimage, arg);

//	if (! animated_type && ! graphic_type )
//    graphics_end(); graphics_inited = 0;
fill:
//	current_term = term_table + 1;	/* terminal graphics */
	backup_x = fontx;
	backup_y = fonty;
 
	if (! graphics_init())
	{
		return ! (errnum = ERR_EXEC_FORMAT);
	}
	fontx = backup_x;
	fonty = backup_y;
  return 1;
}

static struct builtin builtin_splashimage =
{
  "splashimage",
  splashimage_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "splashimage [--offset=[type]=[x]=[y]] FILE",
  "type: bit 7:transparent background\n"
  "FILE: as the background image when in graphics mode.\n"
  "splashimage --fill-color=[0xrrggbb]\n"
  "splashimage --animated=[type]=[duration]=[last_num]=[x]=[y] START_FILE\n"
  "type: bit 0-4: 0x01-0x0f=play n times, 0x10=infinite play.\n"
  "      bit 5:1=show the menu after playing , 0=Play in the menu.\n"
  "      bit 7:1=transparent, 0=opaque.\n"
  "If type=0, stop working.\n"
  "duration: [10] unit is a tick. [10:ms] units are milliseconds,\n"
  "naming rules for START_FILE: *n.???   n: 1-9 or 01-99 or 001-999\n"
  "hotkey F2,control animation:  play/stop."
};

#endif /* SUPPORT_GRAPHICS */

static int in_range (char *range, unsigned long long val);
static int
in_range (char *range, unsigned long long val)
{
  unsigned long long start_num;
  unsigned long long end_num;

  for (;;)
  {
    if (! safe_parse_maxint ((char **)&range, &start_num))
	break;
    if (val == start_num)
	return 1;
    if (*range == ',')
    {
	range++;
	continue;
    }
    if (*range != ':')
	break;

    range++;
    if (! safe_parse_maxint ((char **)&range, &end_num))
	break;
    if ((long long)val > (long long)start_num && (long long)val <= (long long)end_num)
	return 1;
    if (val > start_num && val <= end_num)
	return 1;
    if (*range != ',')
	break;

    range++;
  }

  errnum = 0;
  return 0;
}

/* checkrange */
int
checkrange_func(char *arg, int flags)
{
  struct builtin *builtin1;
  unsigned int ret;
  char *arg1;

  errnum = 0;
  arg1 = skip_to (0, arg);	/* the command */

  builtin1 = find_command (arg1);

  if ((grub_size_t)builtin1 != (grub_size_t)-1)
  if (! builtin1 || ! (builtin1->flags & flags))
  {
	errnum = ERR_UNRECOGNIZED;
	return 0;
  }

  if ((grub_size_t)builtin1 == (grub_size_t)-1 || ((builtin1->func) != errnum_func && (builtin1->func) != checkrange_func))
	errnum = 0;

  if ((grub_size_t)builtin1 != (grub_size_t)-1)
	ret = (builtin1->func) (skip_to (1, arg1), flags);
  else
	ret = command_func (arg1, flags);

  if ((grub_size_t)builtin1 != (grub_size_t)-1)
  if ((builtin1->func) == errnum_func /*|| (builtin1->func) == checkrange_func*/)
	errnum = 0;
  if (errnum)
	return 0;

  return in_range (arg, ret);
}

static struct builtin builtin_checkrange =
{
  "checkrange",
  checkrange_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "checkrange RANGE COMMAND",
  "Return true if the return value of COMMAND is in RANGE and false otherwise."
};

/* checktime */
static int checktime_func(char *arg, int flags);
static int
checktime_func(char *arg, int flags)
{
//  unsigned long date,time;
	struct grub_datetime datetime;
  int day, month, year, sec, min, hour, dow, ii;
  int limit[5][2] = {{0, 59}, {0, 23}, {1, 31}, {1, 12}, {0, 7}};
  int field[5];

  errnum = 0;
  auto int get_day_of_week (void);
  int get_day_of_week (void)
    {
      int a, y, m;

      a = (14 - month) / 12;
      y = year - a;
      m = month + 12 * a - 2;
      return (day + y + y / 4 - y / 100 + y / 400 + (31 * m / 12)) % 7;
    }

	get_datetime(&datetime);
	year = datetime.year;
	month = datetime.month;
	day = datetime.day;
	hour = datetime.hour;
	min = datetime.minute;
	sec = datetime.second;

  dow = get_day_of_week();

  field[0] = min;
  field[1] = hour;
  field[2] = day;
  field[3] = month;
  field[4] = dow;

  if (! arg[0])
    {
      grub_printf ("%d-%02d-%02d %02d:%02d:%02d %d\n", year, month, day, hour, min, sec, dow);
			return hour;
    }

  for (ii = 0; ii < 5; ii++)
    {
      char *p;
      int ok = 0;

      if (! arg[0])
        break;

      p = arg;
      while (1)
        {
          unsigned long long m1, m2, m3;
	  int j;

          if (*p == '*')
            {
              m1 = limit[ii][0];
              m2 = limit[ii][1];
              p++;
            }
          else
            {
              if (! safe_parse_maxint (&p, &m1))
		return 0;

              if (*p == '-')
                {
                  p++;
                  if (! safe_parse_maxint (&p, &m2))
                    return 0;
                }
              else
                m2 = m1;
            }

          if ((m1 < (unsigned long long)limit[ii][0]) || (m2 > (unsigned long long)limit[ii][1]) || (m1 > m2))
            return 0;

          if (*p == '/')
            {
              p++;
              if (! safe_parse_maxint (&p, &m3))
                return 0;
            }
          else
            m3 = 1;

          for (j = m1; j <= (int)m2; j+= m3)
            {
              if (j == field[ii])
                {
                  ok = 1;
                  break;
                }
            }

          if (ok)
            break;

          if (*p == ',')
            p++;
          else
            break;
        }

      if (! ok)
        break;

      arg = skip_to (0, arg);
    }

  return (ii == 5);
}

static struct builtin builtin_checktime =
{
  "checktime",
  checktime_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "checktime min hour dom month dow",
  "Check time."
};

/* clear */
static int clear_func();
static int 
clear_func() 
{
  errnum = 0;
  if (current_term->cls)
    current_term->cls();
  if (use_pager)
    count_lines = 0;
  return 1;
}

static struct builtin builtin_clear =
{
  "clear",
  clear_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "clear",
  "Clear the screen"
};

/* displaymem */
int displaymem_func (char *arg, int flags);
int
displaymem_func (char *arg, int flags)
{
	grub_efi_uintn_t mmap_size = 0x3000;
  grub_efi_uintn_t desc_size;
	grub_efi_memory_descriptor_t *memory_map;
	grub_efi_memory_descriptor_t *desc;
  grub_efi_status_t status;     //状态
  grub_efi_boot_services_t *b;
  b = grub_efi_system_table->boot_services; //系统表->引导服务
  int mm_status;						    //分配内存状态=1/0/-1=成功/部分/失败
  int i, mode = 0;
  quit_print = 0;               //满屏按“Q”退出打印
	
	if (grub_memcmp (arg, "-s", 2) == 0)			//以扇区数计, 简约模式
		mode = 1;
	else if (grub_memcmp (arg, "-a", 2) == 0)	//以字节计, 全部显示
		mode = 2;
	else if (grub_memcmp (arg, "-mem", 4) == 0) //探测4GB以上满足条件的可用内存
    	mode = 3;
	else if (grub_memcmp (arg, "-init", 5) == 0) //获得内存信息
    	mode = 4;
	else																			//以字节计, 简约模式(默认)
		mode = 0;
		
//	memory_map = grub_malloc (0x3000);
  status = efi_call_4 (b->allocate_pages, GRUB_EFI_ALLOCATE_ANY_PAGES,
			      GRUB_EFI_LOADER_CODE, mmap_size >> 11, (grub_efi_physical_address_t *)&memory_map);	//调用(分配页面,分配类型->任意页面,存储类型->装载程序代码(1),分配页,地址)
  if (status) //如果失败
    printf_errinfo ("cannot allocate memory\n");	//无法分配内存 

  mm_status = grub_efi_get_memory_map (&mmap_size, memory_map, 0, &desc_size, 0);  //获得内存映射(映射尺寸,映射页,0,描述尺寸,0)  返回1/0/-1=成功/部分/失败
                                                                                  //获得内存描述符尺寸desc_size, 获得可用内存描述符集地址偏移map_size
  if (mm_status == 0) //如果是部分
	{
    efi_call_2 (b->free_pages, (grub_efi_physical_address_t)(grub_size_t)memory_map, mmap_size >> 11);	//(释放页,地址,页)
		mmap_size += desc_size * 32; //增大内存尺寸
    status = efi_call_4 (b->allocate_pages, GRUB_EFI_ALLOCATE_ANY_PAGES,
            GRUB_EFI_LOADER_CODE, mmap_size >> 11, (grub_efi_physical_address_t *)&memory_map);	//调用(分配页面,分配类型->任意页面,存储类型->装载程序代码(1),分配页,地址)
		if (! memory_map) //如果失败
			printf_errinfo ("cannot allocate memory\n");	//无法分配内存 

		mm_status = grub_efi_get_memory_map (&mmap_size, memory_map, 0,	&desc_size, 0);  //获得内存映射
	}
	
  if (mm_status < 0)  //如果失败
    printf_errinfo ("cannot get memory map\n");	//无法分配内存

  grub_efi_get_memory_map (&mmap_size, memory_map, 0, &desc_size, 0);

  for (desc = memory_map, i = 0;
       desc < NEXT_MEMORY_DESCRIPTOR (memory_map, mmap_size);
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size), i++)	//下一个内存描述符(描述符,尺寸)
	{
		switch(mode)
		{	
			case 2:
				grub_printf ("Unit_Bytes: t=%x, p=%8lx, v=%8lx, n=%8lx, a=%8lx\n",
						desc->type, desc->physical_start, desc->virtual_start,
						desc->num_pages, desc->attribute);
				break;
			case 0:	
				if (desc->type != GRUB_EFI_CONVENTIONAL_MEMORY)
					break;
				grub_printf ("Unit_Bytes: Base=%8lX, Length=%8lX, End=%8lX\n",
						desc->physical_start,
						desc->num_pages << 12,
						desc->physical_start + (desc->num_pages << 12));
				break;
			case 1:	
				if (desc->type != GRUB_EFI_CONVENTIONAL_MEMORY)
					break;
				grub_printf ("Unit_Sector: Base=%8lX, Length=%8lX, End=%8lX\n",
						desc->physical_start >> 9,
						desc->num_pages << 3,
						(desc->physical_start >> 9) + (desc->num_pages << 3));
				break;	
			case 3:
        if (desc->type == GRUB_EFI_CONVENTIONAL_MEMORY  //可用
            && desc->physical_start >= 0x100000000      //大于等于4GB
            && desc->num_pages >= blklst_num_sectors)   //答疑等于指定内存
        {
          blklst_num_sectors = desc->physical_start;    //返回内存起始地址
//          grub_free (memory_map);
          efi_call_2 (b->free_pages, (grub_efi_physical_address_t)(grub_size_t)memory_map, mmap_size >> 11);	//(释放页,地址,页)
          return 1;
        }
			case 4:
        if (desc->type == GRUB_EFI_CONVENTIONAL_MEMORY) //可用
        {
          if (desc->physical_start < 0x100000)          //常规内存(Kb)  0-a0000
          {
            if (desc->num_pages > saved_mem_lower)
            {
              free_mem_lower_start = desc->physical_start;
              saved_mem_lower = desc->num_pages;
            }
          }
          else if (desc->physical_start < 0x100000000)  //扩展内存(Mb)  1M-4G
          {
            if (desc->num_pages > saved_mem_upper)
              saved_mem_upper = desc->num_pages;
          }
          else                                           //上位内存(Mb)  4G以上
          {
            if (desc->num_pages > saved_mem_higher)
              saved_mem_higher = desc->num_pages;
          }
        }
        else
          break;
		}				
    if (quit_print)
      break;
	}

//	grub_free (memory_map);
  efi_call_2 (b->free_pages, (grub_efi_physical_address_t)(grub_size_t)memory_map, mmap_size >> 11);	//(释放页,地址,页)
  return 1;
}

static struct builtin builtin_displaymem =
{
  "displaymem",
  displaymem_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "displaymem [-s|-a]",
  "Display what GRUB thinks the system address space map of the"
  " machine is, including all regions of physical RAM installed.\n"
  "-s: Display Usable RAM in units of 512-byte sectors.\n"
  "-a: Display all information."
};

/* errnum */
int
errnum_func (char *arg, int flags)
{
  printf_debug0 (" ERRNUM is %d\n", errnum);
  return errnum;
}

static struct builtin builtin_errnum =
{
  "errnum",
  errnum_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "errnum",
  "Return the error number."
};

/* errorcheck */
static int errorcheck_func (char *arg, int flags);
static int
errorcheck_func (char *arg, int flags)
{

  errnum = 0;
  /* If ARG is empty, toggle the flag.  */
  if (! *arg)
  {
    errorcheck = ! errorcheck;
    printf_debug0 (" Error check is toggled now to be %s\n", (errorcheck ? "on" : "off"));
  }
  else if (grub_memcmp (arg, "on", 2) == 0)
    errorcheck = 1;
  else if (grub_memcmp (arg, "off", 3) == 0)
    errorcheck = 0;
  else if (grub_memcmp (arg, "status", 6) == 0)
  {
     printf_debug0 (" Error check is now %s\n", (errorcheck ? "on" : "off"));
  }
  else
      errnum = ERR_BAD_ARGUMENT;

  return errorcheck;
}

static struct builtin builtin_errorcheck =
{
  "errorcheck",
  errorcheck_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "errorcheck [on | off | status]",
  "Turn on/off or display the error check mode, or toggle it if no argument."
};

/* fallback */
static int fallback_func (char *arg, int flags);
static int
fallback_func (char *arg, int flags)
{
  unsigned int i = 0;
  int go=0;
  /* The goto command will set errnum before calling this function. 
   * Clearing the errnum here will cause goto to not work.
   */
  //errnum = 0;
  if (memcmp(arg,"--go",4) == 0)
	{
		go = 1;
		arg = skip_to (0, arg);
	}
  while (*arg)
    {
      unsigned long long entry;
      unsigned int j;
      unsigned char c = *arg;
      if (c == '+' || c == '-')
	      ++arg;
      if (! safe_parse_maxint (&arg, &entry))
	return 0;

      if (c == '+')
	      entry += current_entryno;
      else if (c == '-')
	      entry -= current_entryno;
      /* Remove duplications to prevent infinite looping.  */
      for (j = 0; j < i; j++)
	if (entry == (unsigned long long)fallback_entries[j])
	  break;
      if (j != i)
	continue;
      
      fallback_entries[i++] = entry;
      if (i == MAX_FALLBACK_ENTRIES)
	break;
      
      arg = skip_to (0, arg);
    }

  if (i < MAX_FALLBACK_ENTRIES)
    fallback_entries[i] = -1;

  fallback_entryno = (i == 0) ? -1 : 0;
  if (go) return (errnum = 1000);
		return 0;
}

static struct builtin builtin_fallback =
{
  "fallback",
  fallback_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "fallback NUM...",
  "Go into unattended boot mode: if the default boot entry has any"
  " errors, instead of waiting for the user to do anything, it"
  " immediately starts over using the NUM entry (same numbering as the"
  " `default' command). This obviously won't help if the machine"
  " was rebooted by a kernel that GRUB loaded."
};

/* command */
//static char command_path[128]="(bd)/BOOT/GRUB/";
//static int command_path_len = 15;
static char command_path[128]="(bd)/efi/grub/";
static int command_path_len = 14;
//#define GRUB_MOD_ADDR (0xF00000)
#define UTF8_BAT_SIGN 0x54414221BFBBEFULL
#define LONG_MOD_NAME_FLAG 0xEb

struct exec_array
{
	union
	{
	    char sn[12];
	    struct
	    {
		unsigned int flag;
		unsigned int len;
	    } ln;
	} name;
	unsigned int len;
	char data[];
} *p_exec;

//unsigned int mod_end = GRUB_MOD_ADDR;

static struct exec_array *grub_mod_find(const char *name);
static struct exec_array *grub_mod_find(const char *name)
{
    struct exec_array *p_mod = (struct exec_array *)GRUB_MOD_ADDR;
    char *pn;
    unsigned int mod_len;
    while ((grub_size_t)p_mod < (grub_size_t)mod_end)
    {
	mod_len = p_mod->len;
	if (p_mod->name.ln.flag == LONG_MOD_NAME_FLAG)
	{
	    pn = p_mod->data + p_mod->len;
	    mod_len += p_mod->name.ln.len;
	}
	else
	    pn = p_mod->name.sn;
	
	if (substring(name,pn,1) == 0)
	    return p_mod;
	p_mod = (struct exec_array *)((grub_size_t)(p_mod->data + mod_len + 0xf) & ~0xf);
    }
    return 0;
}

static int grub_mod_add (struct exec_array *mod);
static int grub_mod_add (struct exec_array *mod)
{
   char *name;
   unsigned int data_len = 16;
   if (mod->name.ln.flag == LONG_MOD_NAME_FLAG)
    {
	name = mod->data + mod->len;
	data_len +=  mod->name.ln.len;
    }
    else
	name = mod->name.sn;

   if (grub_mod_find(name) == NULL)
   {
      printf_debug("insmod:%s...\n",name);
      unsigned long long rd_base_bak = rd_base;
      unsigned long long rd_size_bak = rd_size;
      rd_base = (unsigned long long)(grub_size_t)mod->data;
      rd_size = (unsigned long long)mod->len;
      buf_drive = -1;
      grub_open("(rd)+1");
      data_len += filemax;
      if ((mod_end + data_len) >= GRUB_MOD_ADDR + 0x800000)
      {
	 grub_close();
         errnum = ERR_WONT_FIT;
         return 0;
      }
      struct exec_array *p_mod = (struct exec_array *)(grub_size_t)mod_end;
      p_mod->len = filemax;
      grub_read((unsigned long long)(grub_size_t)p_mod->data,-1,GRUB_READ);
      grub_close();
      rd_base = rd_base_bak;
      rd_size = rd_size_bak;

      unsigned long long *a = (unsigned long long *)(p_mod->data + p_mod->len - 8);
      unsigned int *b = (unsigned int *)p_mod->data;
      unsigned long long *c = (unsigned long long *)p_mod->data;
      if (*a != (unsigned long long)0xBCBAA7BA03051805ULL
         && *b != BAT_SIGN
         && (*c & (unsigned long long)0xFFFFFFFFFFFFFFULL) != UTF8_BAT_SIGN) //!BAT with utf-8 BOM 0xBFBBEF
      {
         errnum = ERR_EXEC_FORMAT;
         return 0;
      }
      memmove((void *)p_mod->name.sn,(void*)mod->name.sn,sizeof(mod->name));
      if (p_mod->name.ln.flag == LONG_MOD_NAME_FLAG)
         memmove((void*)(p_mod->data + p_mod->len),(void*)(mod->data + mod->len),p_mod->name.ln.len);
      mod_end = (char *)(grub_size_t)(((grub_size_t)mod_end + data_len + 0xf) & ~0xf);
      printf_debug0("%s loaded\n",name);
   }
   else
      printf_debug0("%s already loaded\n",name);
   return 1;
}

static int grub_mod_list(const char *name);
static int grub_mod_list(const char *name)
{
   struct exec_array *p_mod = (struct exec_array *)GRUB_MOD_ADDR;
   char *pn;
   unsigned int mod_len;
   int ret = 0;
   while ((grub_size_t)p_mod < (grub_size_t)mod_end)
   {
	mod_len = p_mod->len;
	if (p_mod->name.ln.flag == LONG_MOD_NAME_FLAG)
	{
	    pn = p_mod->data + p_mod->len;
	    mod_len += p_mod->name.ln.len;
	}
	else
	    pn = p_mod->name.sn;

      if (*name == '\0' || substring(name,pn,1) == 0)
      {
         printf_debug0(" %s\n",pn);
         ret = (grub_size_t)p_mod->data;
      }
      p_mod = (struct exec_array *)((grub_size_t)(p_mod->data + mod_len + 0xf) & ~0xf);
   }
   return ret;
}

static int grub_mod_del(const char *name);
static int grub_mod_del(const char *name)
{
   struct exec_array *p_mod;
   char *pn;
   grub_size_t mod_len;
   for (p_mod = (struct exec_array *)GRUB_MOD_ADDR; (grub_size_t)p_mod < (grub_size_t)mod_end; p_mod = (struct exec_array *)((grub_size_t)(p_mod->data + mod_len + 0xf) & ~0xf))
   {
 	mod_len = p_mod->len;
	if (p_mod->name.ln.flag == LONG_MOD_NAME_FLAG)
	{
	    pn = p_mod->data + p_mod->len;
	    mod_len += p_mod->name.ln.len;
	}
	else
	    pn = p_mod->name.sn;
      if (substring(name,pn,1) == 0)
      {
         grub_size_t next_mod = ((grub_size_t)p_mod->data + mod_len + 0xf) & ~0xf;
         if (next_mod == (grub_size_t)mod_end)
            mod_end = (char *)p_mod;
         else
         {
            memmove(p_mod,(char *)next_mod,(grub_size_t)mod_end - next_mod);
            mod_end -= next_mod - (grub_size_t)p_mod;
         }
         printf_debug0("%s unloaded.\n",name);
         return 1;
      }
   }
   return 0;
}

static int grub_exec_run(char *program, char *psp, int flags);
static int test_open(char *path);
static int test_open(char *path)
{
    printf_debug ("CHECK: %s\n",path + command_path_len - 1);
    if (grub_open(path + command_path_len - 1))
	return 1;
    printf_debug ("CHECK: %s\n",path);
    if (grub_open(path))
	return 3;
    return 0;
}

static int command_open(char *arg,int flags);
static int command_open(char *arg,int flags)
{
#define t_path chainloader_file_orig
   if (*arg == '(' || *arg == '/')
      return grub_open(arg);
   if ((char *)skip_to(0,arg) - arg > 120)
      return 0;
   if (flags == 0 && (p_exec = grub_mod_find(arg)))
      return 2;
//    char t_path[512];
  grub_memset(t_path, 0, 256);

    int len = strlen(arg) + command_path_len;
    if ((len + 1) >= (int)sizeof(t_path))
    {
	errnum = ERR_WONT_FIT;
	return 0;
    }
    memmove(t_path,command_path,command_path_len + 1);
    memmove(t_path + command_path_len,arg,strlen(arg) + 1);

    int ret = test_open(t_path);
    if (ret)
	return ret;
#ifdef PATHEXT
    if(!PATHEXT[0])
	return 0;
   while(*arg++)//Find ExtName;
   {
	if (*arg == '.')
	    return 0;
   }

    int i = 0;
    arg = PATHEXT;
    while(1)
    {
	if (*arg == ';' || *arg == 0)
	{
	    if (i > 0)
	    {
		sprintf(t_path + len ,"%.*s" ,i , arg-i);
		ret = test_open(t_path);
		if (ret)
		    return ret;
		i = 0;
	    }
	}
	else
	    ++i;
	if (*arg == 0)
	    break;
	++arg;
    }
#endif
    return 0;
#undef t_path
}

int
command_func (char *arg, int flags)
{
#define file_path chainloader_file
  errnum = 0;
  while (*arg == ' ' || *arg == '\t') arg++;

  if (! flags)	/* check syntax only */
  {
    if (*arg == '/' || *arg == '(' || *arg == '+' || *arg=='%')
	return 1;
    if (*arg >= '0' && *arg <= '9')
	return 1;
    if (*arg >= 'a' && *arg <= 'z')
	return 1;
    if (*arg >= 'A' && *arg <= 'Z')
	return 1;
    return 0;
  }

   if (*arg <= ' ')
   {
      if (debug > 0)
      {
	 printf("Current default path: %s\n",command_path);
	 #ifdef PATHEXT
	 if (PATHEXT[0])
	    printf("PATHEXT: %s\n",PATHEXT);
	 #endif
      }
      return 20;
   }

    if (*(short*)arg == 0x2d2d && *(int*)(arg+2) == 0x2d746573)// -- set-
    {
	arg += 6;
	if (grub_memcmp(arg,"path=",5) == 0)
	{
	    arg += 5;
	    if (! *arg)
	    {
//		command_path_len = 15;
		command_path_len = 14;
		return grub_sprintf(command_path,"(bd)/EFI/GRUB/");
	    }

	    int j = grub_strlen(arg);

	    if (j >= 0x60)
	    {
		printf_debug0("Set default command path error: PATH is too long \n");
		return 0;
	    }

	    grub_memmove(command_path, arg, j + 1);
	    if (command_path[j-1] != '/')
		command_path[j++] = '/';
	    command_path[j] = 0;
	    command_path_len = j;
	    return 1;
	}
	#ifdef PATHEXT
	if (grub_memcmp(arg,"ext=",4) == 0)
	    return sprintf(PATHEXT,"%.63s",arg + 4);
	#endif
	arg -= 6;
    }
  /* open the command file. */
  char *filename = arg; //文件名: g4e_wb abcdef
//  char file_path[512];
  grub_memset(file_path, 0, 256);
  unsigned int arg_len = grub_strlen(arg);/*get length for build psp */   //文件名尺寸 d
  char *cmd_arg = skip_to(SKIP_WITH_TERMINATE,arg);/* get argument of command */    //命令参数: abcdef
  p_exec = NULL;

  switch(command_open(filename,0))
  {
     case 2:	//grub_mod_find(arg)
        sprintf(file_path,"(md)/");
        filemax = p_exec->len;
		break;
	 case 3:
		{
		char *p=command_path;
		print_root_device(file_path,1);
		while (*p != '/')
			++p;
		sprintf(file_path+strlen(file_path),"%s",p);
	    break;
		}
	 case 1:
	 	if (*filename != '(')
		{
			print_root_device(file_path,0);
			sprintf(file_path+strlen(file_path),"%s/",saved_dir);
		}
		else
		{
			file_path[0] = '/';
			file_path[1] = 0;
		}
	    break;
     default:
        printf_errinfo ("Error: No such command: %s\n", arg);
        errnum = 0;	/* No error, so that old menus will run smoothly. */
        return 0;/* return 0 indicating a failure or a false case. */
  }

	if (filemax < 9ULL)
	{
		errnum = ERR_EXEC_FORMAT;
		goto fail;
	}


	char *psp;
	unsigned int psp_len;
	unsigned int prog_len;
	char *program;
	char *tmp;
	prog_len = filemax; //程序(文件)尺寸
	psp_len = ((arg_len + strlen(file_path)+ 16) & ~0xF) + 0x10 + 0x20; //psp尺寸
//	tmp = (char *)grub_malloc(prog_len + 4096 + 16 + psp_len + 512);  //缓存
	tmp = (char *)grub_memalign(4096, prog_len + 16 + psp_len + 512);  //对齐4k分配缓存
	if (tmp == NULL)
	{
		goto fail;
	}

//	program = (char *)((grub_size_t)(tmp + 4095) & ~4095); /* 4K align the program 4K对齐程序*/   //程序缓存
//	psp = (char *)((grub_size_t)(program + prog_len + 16) & ~0x0F); //psp地址  向上舍入，否则覆盖program数据
	program = tmp;
	psp = (char *)((grub_size_t)(program + prog_len + 16 + 512) & ~0x0F); //psp地址  向上舍入，否则覆盖program数据
	unsigned long long *end_signature = (unsigned long long *)(program + filemax - (unsigned long long)8);  //程序结束签名地址
	if (p_exec == NULL)
	{
		/* read file to buff and check exec signature. 读取文件到程序缓存并检查exec签名*/
		if ((grub_read ((unsigned long long)(grub_size_t)program, -1ULL, 0xedde0d90) != filemax))
		{
			if (! errnum)
				errnum = ERR_EXEC_FORMAT;
		}
		else if (*end_signature == 0x85848F8D0C010512ULL)
		{
			if (filemax < 512 || filemax > 0x80000)
				errnum = ERR_EXEC_FORMAT;
			else
			{
				grub_free(tmp);
				return 0;
			}
		}
		else if (*end_signature != 0xBCBAA7BA03051805ULL && 
				  *(unsigned int *)program != BAT_SIGN &&
				  (*(unsigned long long *)program & 0xFFFFFFFFFFFFFFULL) != UTF8_BAT_SIGN)
		{
			errnum = ERR_EXEC_FORMAT;
		}

		grub_close ();
		if (errnum)
		{
		   grub_free(tmp);
		   return 0;
		}
	}
	else
	{
		grub_memmove(program,p_exec->data,prog_len);
	}
	if (*end_signature == 0xBCBAA7BA03051805ULL)
	{
//		if (*(unsigned long long *)(program + prog_len - 0x20) == 0x646E655F6E69616D) //bios模式：新版本标记 main_end
    if (*(unsigned long long *)(program + prog_len - 0x20) == 0x646E655F69666575) //uefi模式：新版本标记 uefi_end
		{
			unsigned int *prog_start = (unsigned int *)(program + prog_len - 0x3c); //.text
			unsigned int *bss_end = (unsigned int *)(program + prog_len - 0x24);    //end

			if (prog_len != (*bss_end - *prog_start)){  //如果bss区有数据,外部命令尺寸是filemax+bss尺寸
				grub_free(tmp);
        prog_len = *bss_end - *prog_start;
        char *program1 = program;
//        tmp = (char *)grub_malloc(prog_len + 4096 + 16 + psp_len);
//        tmp = (char *)grub_malloc(prog_len + 4096 + 16 + psp_len + 512);
        tmp = (char *)grub_memalign(4096, prog_len + 16 + psp_len + 512);  //对齐4k分配缓存
        if (tmp == NULL)
					goto fail;
//        program = (char *)((grub_size_t)(tmp + 4095) & ~4095); /* 4K align the program */
        program = tmp;
        if (program != program1)
					grub_memmove (program, program1, prog_len);
//        psp = (char *)((grub_size_t)(program + prog_len + 16) & ~0x0F);
        psp = (char *)((grub_size_t)(program + prog_len + 16 + 512) & ~0x0F);
			}
		} else {//the old program
			char *program1;
			printf_warning ("\nWarning! The program is outdated!\n");
			psp = (char *)grub_malloc(prog_len + 4096 + 16 + psp_len);
			if (psp == NULL)
			{
				grub_free(tmp);
				goto fail;
			}
			program1 = psp + psp_len;
			grub_memmove (program1, program, prog_len);
			program = program1;
			tmp = psp;
		}
	}

	program[prog_len] = '\0';
	grub_memset(psp, 0, psp_len);
	grub_memmove (psp + 16, arg , arg_len + 1);/* copy args into somewhere in PSP. */
	filename = psp + 16 + arg_len + 1;
	grub_strcpy(filename,file_path);
	*(unsigned int *)psp = psp_len;
	*(unsigned int *)(psp + psp_len - 4) = psp_len;	/* PSP length in bytes. it is in both the starting dword and the ending dword of the PSP. */
	*(unsigned int *)(psp + psp_len - 8) = psp_len - 16 - (cmd_arg - arg);	/* args is here. */
	*(unsigned int *)(psp + psp_len - 12) = flags;		/* flags is here. */
	*(unsigned int *)(psp + psp_len - 16) = psp_len - 16;/*program filename here.*/
	*(unsigned int *)(psp + psp_len - 20) = prog_len;//program length
	*(unsigned int *)(psp + psp_len - 24) = psp_len - (filename - psp);
	{//New psp info
		psp_info_t *PI = (psp_info_t*)psp;
		PI->proglen=prog_len;
		PI->arg=(unsigned short)(cmd_arg - arg) + 16;
		PI->path=arg_len + 1 + 16;
	}
	/* (free_mem_start + pid - 16) is reserved for full pathname of the program file. */
	int pid;
	++prog_pid;
	pid = grub_exec_run(program, psp, flags);
	/* on exit, release the memory. */
	grub_free(tmp);
	if (!(--prog_pid) && *CMD_RUN_ON_EXIT)//errnum = -1 on exit run.
	{
		errnum = 0;
		*CMD_RUN_ON_EXIT = 0;
		pid = run_line(CMD_RUN_ON_EXIT+1,flags);
	}
	return pid;

fail:
  grub_close ();
  return 0;
#undef file_path
}

static struct builtin builtin_command =
{
  "command",
  command_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING | BUILTIN_IFTITLE,
  "command [--set-path=PATH|--set-ext=EXTENSIONS] FILE [ARGS]",
  "Run executable file FILE with arguments ARGS."
  "--set-path sets a search PATH for executable files,default is (bd)/boot/grub."
  "--set-ext sets default extensions for executable files."
};

/* 模块结构
 * 首行：  16字节  05 18 05 03 BA A7 BA BC  00 00 00 00 00 00 00 00
 * 正文：
 *     外部命令名称  12字节
 *     尺寸  4字节
 *     外部命令
 * 下一正文
 */
static int insmod_func(char *arg,int flags)
{
   errnum = 0;
   if (arg == NULL || *arg == '\0')
      return 0;
   char *name = skip_to(1|SKIP_WITH_TERMINATE,arg);
   
  if (!GRUB_MOD_ADDR)
  {        
    GRUB_MOD_ADDR = grub_malloc (0x800000);   //模块缓存
    mod_end = GRUB_MOD_ADDR;
  }
   if (substring(skip_to(0,arg) - 4,".mod",1) == 0 || flags == 101)
   {
      if (!command_open(arg,1))
         return 0;
      char *buff=grub_malloc(filemax);
      if (!buff)
      {
        grub_close();
        return 0;
      }
      if (grub_read((unsigned long long)(grub_size_t)buff,-1,GRUB_READ) != filemax)
      {
        grub_close();
        grub_free(buff);
        return 0;
      }
      grub_close();
      char *buff_end = buff+filemax;
      struct exec_array *p_mod = (struct exec_array *)buff;
      //skip grub4dos moduld head.
      if (strcmp(p_mod->name.sn,"\x05\x18\x05\x03\xBA\xA7\xBA\xBC") == 0)
        ++p_mod;
      else
      {
        grub_free(buff);
        goto external_command;
      }
      while ((char *)p_mod < buff_end && grub_mod_add(p_mod))
      {
         p_mod = (struct exec_array *)(p_mod->data + p_mod->len);
      }
      grub_free(buff);
      return 1;
   }

external_command:
   switch(command_open(arg,0))
   {
      case 2:
	 printf_debug("%s already loaded\n",arg);
         return 1;
      case 0:
         return 0;
      default:
         {
            struct exec_array *p_mod = grub_malloc(filemax + sizeof(struct exec_array) + 32);
            
            int ret = 0;
            if (p_mod == NULL)
		return 0;
	    if (grub_read((unsigned long long)(grub_size_t)p_mod->data,-1,GRUB_READ) != filemax)
            {
		grub_close();
		grub_free(p_mod);
		return 0;
            }
            grub_close();
            p_mod->len = filemax;
            if (!*name)
            {
               name = arg;
               if (*arg == '(' || *arg == '/')
               {
                  while (*arg)
                  {
                     if (*arg++ == '/')
                        name = arg;
                  }
               }
            }
            if (strlen(name) < 12)
		grub_strcpy(p_mod->name.sn,name);
	    else
            {
		p_mod->name.ln.flag = LONG_MOD_NAME_FLAG;
		p_mod->name.ln.len = sprintf(p_mod->data + filemax,"%s",name) + 1;
            }

            ret = grub_mod_add(p_mod);
            grub_free(p_mod);
            return ret;
         }
   }
}

static struct builtin builtin_insmod =
{
   "insmod",
   insmod_func,
   BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
   "insmod MODFILE|FILE.MOD",
   "FILE.MOD is MODFILE package, it has multiple MODFILE"
};

static int delmod_func(char *arg,int flags);
static int delmod_func(char *arg,int flags)
{
   errnum = 0;
   if (*arg == '\0')
      return grub_mod_list(arg);
   if (grub_memcmp(arg,"-l",2) == 0)
   {
      arg = skip_to(0,arg);
      return grub_mod_list(arg);
   }

   if (*arg == '*')
   {
     if (GRUB_MOD_ADDR)
       grub_free (GRUB_MOD_ADDR);
     GRUB_MOD_ADDR = 0;
      mod_end = GRUB_MOD_ADDR;
      return 1;
   }

   return grub_mod_del(arg);
}

static struct builtin builtin_delmod =
{
   "delmod",
   delmod_func,
   BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
   "delmod [modname|*]",
   "delete the module loaded by insmod."
};

/* commandline */
int
commandline_func (char *arg, int flags)
{
  errnum = 0;
  enter_cmdline(arg, 0);

  return 1;
}

static struct builtin builtin_commandline =
{
  "commandline",
  commandline_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "commandline",
  "Enter command-line prompt mode."
};

static int real_root_func (char *arg1, int attempt_mnt);
/* find */
/* Search for the filename ARG in all of partitions and optionally make that
 * partition root("--set-root", Thanks to Chris Semler <csemler@mail.com>).
 */
static int find_check(char *filename,struct builtin *builtin1,char *arg,int flags);
static int find_check(char *filename,struct builtin *builtin1,char *arg,int flags)
{
	saved_drive = current_drive;
	saved_partition = current_partition;
	if (filename == NULL || (open_device() && grub_open (filename)))
	{
		grub_close ();
		if (builtin1)
		{
			int ret = strlen(arg) + 1;
			char *_tmp;
			if ((_tmp = grub_malloc(ret)) == NULL)
				return 0;
			memmove(_tmp,arg,ret);
			buf_drive = -1;
			ret = (builtin1->func) (_tmp, flags);
			grub_free(_tmp);	
			if (ret == 0)
				return 0;
		}

		if (debug > 0)
		{
			print_root_device(NULL,0);
			putchar('\n', 255);
		}
		return 1;
	}

	errnum = ERR_NONE;
	return 0;
}

static int
find_func (char *arg, int flags)
{
  struct builtin *builtin1 = 0;
  char *filename;
  unsigned int tmp_drive = saved_drive;
  unsigned int tmp_partition = saved_partition;
  unsigned int got_file = 0;
  char *set_root = 0;
  unsigned int ignore_cd = 0;
  unsigned int ignore_floppies = 0;
  char find_devices[8]="pnuhcf";//find order:pd->nd->ud->hd->cd->fd
  errnum = 0;
	int i = 0;
  struct grub_disk_data *d;	//磁盘数据
#ifdef FSYS_FB
  if (saved_drive == FB_DRIVE && !(unsigned char)(fb_status >> 8))
  {
    unsigned int *a = (unsigned int *)&find_devices[3];
//	*(unsigned long *)&find_devices[3]=0x686366;
    *a=0x686366;
  }
#endif
	for (;;)
	{
		if (grub_memcmp (arg, "--set-root=", 11) == 0)
    {
      set_root = arg + 11;
      if (*set_root && *set_root != ' ' && *set_root != '\t' && *set_root != '/')
        return ! (errnum = ERR_FILENAME_FORMAT);
    }
    else if (grub_memcmp (arg, "--set-root", 10) == 0)
    {
      set_root = "";
    }
    else if (grub_memcmp (arg, "--ignore-cd", 11) == 0)
    {
      ignore_cd = 1;
    }
    else if (grub_memcmp (arg, "--ignore-floppies", 17) == 0)
    {
      ignore_floppies = 1;
    }
		else if (grub_memcmp(arg, "--devices=", 10) == 0)
		{
			arg += 10;
			while (i < 7 && *arg >= 'a')
			{
				find_devices[i++] = *arg++;
			}
			find_devices[i] = '\0';
		}
		else
			break;
    arg = skip_to (0, arg);
  }
  
  /* The filename should NOT have a DEVICE part. */
  filename = set_device (arg);
  if (filename)
    return ! (errnum = ERR_FILENAME_FORMAT);

  if (*arg == '/' || *arg == '+' || (*arg >= '0' && *arg <= '9'))
  {
    filename = arg; 
    arg = skip_to (0, arg);
  } else {
    filename = 0;
  }

  /* arg points to command. */
  if (*arg)
  {
    builtin1 = find_command (arg);
    if ((grub_size_t)builtin1 != (grub_size_t)-1)
      if (! builtin1 || ! (builtin1->flags & flags))
      {
        errnum = ERR_UNRECOGNIZED;
        return 0;
      }
    if ((grub_size_t)builtin1 != (grub_size_t)-1)
      arg = skip_to (1, arg);	/* get argument of command */
    else
      builtin1 = &builtin_command;
  }

  errnum = 0;

	char *devtype = find_devices;
	unsigned int FIND_DRIVES = 0;
	/*check if current root in find_devices list*/
	for (; *devtype; devtype++)
	{
		switch(*devtype)
		{
			case 'h':
				if (tmp_drive >= 0x80 && tmp_drive <= 0x8F)
					FIND_DRIVES = 1;
				break;
			case 'u':
				if (tmp_drive == FB_DRIVE)
					FIND_DRIVES = 1;
				break;
			case 'p':
				if (PXE_DRIVE == tmp_drive)
					FIND_DRIVES = 1;
				break;
			case 'c':
				if (ignore_cd)
					*devtype = ' ';
				else if (tmp_drive >= 0xa0 && tmp_drive <= 0xff)
					FIND_DRIVES = 1;
				break;
			case 'f':
				if (ignore_floppies)
					*devtype = ' ';
				else if (tmp_drive <= 0x7f)
					FIND_DRIVES = 1;
				break;
		}
	}
	/*search in current root device*/
	if (FIND_DRIVES)
	{
		current_drive = saved_drive;
		current_partition = saved_partition;
		if (((current_drive < 0x80 && floppies_orig) || (current_drive >= 0xa0 && cdrom_orig)) && find_check(filename,builtin1,arg,flags) == 1)
		{
			got_file = 1;
			if (set_root)
				goto found;
		}
		else if (current_drive >= 0x80 && current_drive <= 0x8f && harddrives_orig)
		{
			struct grub_part_data *dp;
			for (dp = partition_info; dp; dp = dp->next)
			{
				if (dp->drive == current_drive)
					current_partition = dp->partition;
				else
					continue;
				
				if (find_check(filename,builtin1,arg,flags) == 1)
				{
					got_file = 1;
					if (set_root)
						goto found;
				}
			}
		}
	}
	/*search other devices*/
	for (devtype = find_devices; *devtype; devtype++)
	{
		current_partition = 0xFFFFFF;
		switch(*devtype)
		{
#ifdef FSYS_FB
			case 'u':
				if (fb_status)
					current_drive = FB_DRIVE;
				else
					continue;
				break;
#endif
#ifdef FSYS_PXE
			case 'p':		
				if (pxe_entry)
					current_drive = PXE_DRIVE;
				else
					continue;
				break;
#endif
			case 'c':/*Only search first cdrom*/
        for (d = disk_data; d && cdrom_orig; d = d->next)	//从设备结构起始查
        {
          if (d->disk_type != DISK_TYPE_CD)
            continue;
          current_drive = d->drive;
					if (tmp_drive == current_drive)
						continue;
					if (find_check(filename,builtin1,arg,flags) == 1)
					{
						got_file = 1;
						if (set_root)
							goto found;
					}
				}
#if 0
				for (drive = 0xa0; drive < (unsigned int)0xa0 + cdrom_orig; drive++)
				{
					current_drive = drive;
					if (tmp_drive == current_drive)
						continue;
					if (find_check(filename,builtin1,arg,flags) == 1)
					{
						got_file = 1;
						if (set_root)
							goto found;
					}
				}
#endif
				break;
			case 'f':
        for (d = disk_data; d && floppies_orig; d = d->next)	//从设备结构起始查
        {
          if (d->disk_type != DISK_TYPE_FD)
            continue;
          current_drive = d->drive;
					if (tmp_drive == current_drive)
						continue;
					if (find_check(filename,builtin1,arg,flags) == 1)
					{
						got_file = 1;
						if (set_root)
							goto found;
					}
				}
#if 0
				for (drive = 0; drive < (unsigned int)floppies_orig; drive++)
				{
					current_drive = drive;
					if (tmp_drive == current_drive)
						continue;
					if (find_check(filename,builtin1,arg,flags) == 1)
					{
						got_file = 1;
						if (set_root)
							goto found;
					}
				}
#endif
				break;
			case 'h':
        for (d = disk_data; d && harddrives_orig; d = d->next)	//从设备结构起始查
        {
					struct grub_part_data *dp;
          if (d->disk_type != DISK_TYPE_HD)
            continue;

          current_drive = d->drive;
					current_partition = 0xFFFFFF;

					if (tmp_drive == current_drive)
						continue;
          if (d->drive == (unsigned char)(fb_status >> 8) && find_check(filename,builtin1,arg,flags) == 1)
          {
            got_file = 1;
            if (set_root)
              goto found;
          }
					for (dp = partition_info; dp; dp = dp->next)
					{
						if (dp->drive == d->drive)
							current_partition = dp->partition;
						else
							continue;
					
						if (find_check(filename,builtin1,arg,flags) == 1)
						{
							got_file = 1;
							if (set_root)
								goto found;
						}
					}
				/* next_partition always sets ERRNUM in the last call, so clear it.  */
					errnum = ERR_NONE;
				}
#if 0
				for (drive = 0x80; drive < (unsigned int)0x80 + harddrives_orig; drive++)
				{
					current_drive = drive;
					current_partition = 0xFFFFFF;
					struct grub_part_data *dp;

					if (tmp_drive == current_drive)
						continue;
          if (drive == (unsigned char)(fb_status >> 8) && find_check(filename,builtin1,arg,flags) == 1)
          {
            got_file = 1;
            if (set_root)
              goto found;
          }
					for (dp = partition_info; dp; dp = dp->next)
					{
						if (dp->drive == drive)
							current_partition = dp->partition;
						else
							continue;
						
						if (find_check(filename,builtin1,arg,flags) == 1)
						{
							got_file = 1;
							if (set_root)
								goto found;
						}
					}
				/* next_partition always sets ERRNUM in the last call, so clear it.  */
					errnum = ERR_NONE;
				}
#endif
				#undef FIND_HD_DRIVES
				#undef FIND_FD_DRIVES
				//h,f. no break;default continue;
				break;
			default:
				continue;
		}
	}
	saved_drive = tmp_drive;
	saved_partition = tmp_partition;
found:
	if (got_file)
	{
		errnum = ERR_NONE;
		if (set_root)
		{
			int j;

			/* copy root prefix to saved_dir */
			for (j = 0; j < (int)sizeof (saved_dir); j++)
			{
        char ch;

        ch = set_root[j];
        if (ch == 0 || ch == 0x20 || ch == '\t')
          break;
        if (ch == '\\')
        {
          saved_dir[j] = ch;
          j++;
          ch = set_root[j];
          if (! ch || j >= (int)sizeof (saved_dir))
          {
            j--;
            saved_dir[j] = 0;
            break;
          }
        }
        saved_dir[j] = ch;
      }

      if (saved_dir[j-1] == '/')
      {
        saved_dir[j-1] = 0;
      }
      else
        saved_dir[j] = 0;
    } //if set_root
    return 1;
	}
  errnum = ERR_FILE_NOT_FOUND;
  return 0;
}

static struct builtin builtin_find =
{
  "find",
  find_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "find [--set-root[=DIR]] [--devices=DEVLIST] [--ignore-floppies] [--ignore-cd] [FILENAME] [CONDITION]",
  "Search for the filename FILENAME in all of partitions and print the list of"
  " the devices which contain the file and suffice CONDITION. CONDITION is a"
  " normal grub command, which return non-zero for TRUE and zero for FALSE."
  " DEVLIST specify the search devices and order,the default DEVLIST is upnhcf."
  " DEVLIST must be a combination of these letters (u, p, n, h, c, f)."
  " If the option --set-root is used and FILENAME is found on a device, then"
  " stop the find immediately and set the device as new root."
  " If the option --ignore-floppies is present, the search will bypass all"
  " floppies. And --ignore-cd will skip (cd)."
};


#ifdef SUPPORT_GRAPHICS

/*
 * The code in function GET_NIBBLE is released to the public domain.
 *				tinybit  2011-11-18
 */
static unsigned int get_nibble (unsigned int c);
static unsigned int
get_nibble (unsigned int c)
{
	unsigned int tmp;
	tmp = ((c > '9') ? ((c & 0xdf)- 'A' + 10) : (c - '0'));
	if (tmp & 0xFFFFFFF0)
	{
	    errnum = ERR_UNIFONT_FORMAT;
	}
	return tmp;
}

extern unsigned int ged_unifont_simp (unsigned int unicode);
extern unsigned int
ged_unifont_simp (unsigned int unicode)
{
	int i;
	for (i = 0; i < 8; i++)
	{
		if (unicode >= unifont_simp[i].start && unicode <= unifont_simp[i].end)
			return unicode - unifont_simp[i].offset;
	}
	return 0;
}

//static unsigned long old_narrow_char_indicator = 0;
//#define	old_narrow_char_indicator	narrow_char_indicator

int (*hotkey_func)(char *titles,int flags,int flags1,int key);
struct simp unifont_simp[]={{0,0xff,0},{0x2000,0x206f,0x1f00},{0x2190,0x21ff,0x2020},{0x2e80,0x303f,0x2ca0},{0x31c0,0x9fbf,0x2e20},{0xf900,0xfaff,0x8760},{0xfe30,0xffef,0x8a90}};
unsigned char unifont_simp_on;

unsigned char *UNIFONT_START = 0;
unsigned char *narrow_mem = 0;
/* font */
/* load unifont to UNIFONT_START */
/*
 * The code and text in function FONT_FUNC is released to the public domain.
 *				tinybit  2011-11-18
 */
int font_func (char *arg, int flags);
int
font_func (char *arg, int flags)
{
  unsigned int i, j, k;
  unsigned int len;
  unsigned int unicode=0;
//  unsigned int narrow_indicator;
	unsigned char buf[870];	//48*48
  unsigned int valid_lines;
  unsigned long long saved_filepos;
	unsigned long long val;
	unsigned char num_narrow;
	unsigned char tag[]={'d','o','t','s','i','z','e','='};
	unsigned int font_h_old = font_h;
	unsigned int font_h_new = 0;
  valid_lines = 0;
  errnum = 0;

  if (arg == NULL || *arg == '\0')
  {
			return 0;
  }

	if (flags)
	{
		unifont_simp_on = 0;
	for (; *arg && *arg != '/' && *arg != '(' && *arg != '\n' && *arg != '\r';)
	{
		if (grub_memcmp (arg, "--font-high=", 12) == 0)
		{
			arg += 12;
			if (safe_parse_maxint (&arg, &val))
				font_h_new = val;
		}
		else if (grub_memcmp (arg, "--simp=", 7) == 0)
		{
			len	=	0;
			arg += 7;
			unifont_simp_on = 1;
			for (i = 0; i < 4; i++)
			{
				if (safe_parse_maxint (&arg, &val))
					unifont_simp[i].start = val;
				else
					break;
				arg++;
				if (safe_parse_maxint (&arg, &val))
					unifont_simp[i].end = val;
				arg++;
				unifont_simp[i].offset = unifont_simp[i].start - len;
				len += unifont_simp[i].end - unifont_simp[i].start + 1;
				if (*arg != 0x3d)		//"="
					break;
			}
		}
		else
			break;

		while (*arg == ' ' || *arg == '\t')
			arg++;
	}
	}

	if (! grub_open(arg))
		return 0;
	
	if (flags)
	{
	if (!font_h_new)
	{
		font_h = 16;
		font_w = 8;
	}
	else
	{
		font_h = font_h_new;
		font_w = font_h_new/2;
	}
	}	
	
  if (filemax >> 32)	// file too long
    return !(errnum = ERR_WONT_FIT);

  num_wide = (font_h+7)/8;
  num_narrow = ((font_h/2+7)/8)<<1;

  if (font_h_old != font_h)
  {
    current_term->max_lines = current_y_resolution / (font_h + line_spacing);
    current_term->chars_per_line = current_x_resolution / (font_w + font_spacing);
  }

redo:
	while	(((saved_filepos = filepos), (len = grub_read((unsigned long long)(grub_size_t)(char*)&buf, 6+font_h*num_narrow, 0xedde0d90))))
  {
		if (len != 6+font_h*num_narrow || buf[4] != ':')
    {
	errnum = ERR_UNIFONT_FORMAT;
	break;
    }

    /* get the unicode value */
    unicode = 0;
    for (i = 0; i < 4; i++)
    {
	unsigned short tmp;
	tmp = get_nibble (buf[i]);
	if (errnum)
	    goto close_file;
	unicode |= (tmp << ((3 - i) << 2));
    }
    
  if (!UNIFONT_START || font_h_old != font_h)
  {
    font_h_old = font_h;
    if (UNIFONT_START)
      grub_free (UNIFONT_START);
    UNIFONT_START = grub_zalloc (num_wide * font_h * 0x10000);
    if (!UNIFONT_START)
      return 0;
    if (!narrow_mem)
      narrow_mem = grub_zalloc(0x2000);  //宽窄字符指示器 0/1=窄/宽
    else
      grub_memset (narrow_mem, 0, 0x2000);
    if (!narrow_mem)
      return 0;
  }

		if (buf[5+font_h*num_narrow] == '\n' || buf[5+font_h*num_narrow] == '\r')	/* narrow char */
    {
	/* discard if it is a control char(we will re-map control chars) */

	/* simply put the 8x16 dot matrix at the right half */
		if (unifont_simp_on)
			unicode = ged_unifont_simp (unicode);
//			*(unsigned char *)(narrow_mem + unicode/8) &= ~(1 << (unicode&7));	//初始化已经清零   2023-02-22
			for (j=0; j<font_w; j++)
			{
				unsigned long long dot_matrix = 0;
				for (k=0; k<font_h; k++)
				{
					unsigned long long t = 0;
					t = get_nibble (buf[5+(j>>2)+(k*num_narrow)]);
					if (errnum)
						goto close_file;
					dot_matrix |= ((t >> ((4*num_narrow-1-j) & 3)) & 1) << k;
				}
				for (k=0; k<num_wide; k++)
					((unsigned char *)(UNIFONT_START + unicode*num_wide*font_h + num_wide*font_h/2))[j*num_wide+k] = (dot_matrix >> k*8)&0xff;
				/* the first integer is to be checked for narrow_char_indicator */
			}
//			*(unsigned int *)(UNIFONT_START + unicode*num_wide*font_h) = old_narrow_char_indicator; 
    }
    else
    {
	/* read additional 32 chars and see if it end in a LF */
	len = grub_read((unsigned long long)(grub_size_t)(char*)(buf+6+font_h*num_narrow), font_h*(num_wide*2-num_narrow), 0xedde0d90);
	if (len != font_h*(num_wide*2-num_narrow) || (buf[5+font_h*num_wide*2] != '\n' && buf[5+font_h*num_wide*2] != '\r'))
	{
	    errnum = ERR_UNIFONT_FORMAT;
	    break;
	}

	/* discard if it is a normal ASCII char */
//	if (unicode <= 0x7F)
//	    continue;

	/* discard if it is internally used INVALID chars 0xDC80 - 0xDCFF */
	if (unicode >= 0xDC80 && unicode <= 0xDCFF)
	    continue;
	if (unifont_simp_on)
		unicode = ged_unifont_simp (unicode);
	/* set bit 0: this unicode char is a wide char. */
//	*(unsigned char *)(narrow_mem + unicode) |= 1;	/* bit 0 */
  *(unsigned char *)(narrow_mem + unicode/8) |= (1 << (unicode&7));

	/* put the 16x16 dot matrix */
			for (j=0; j<font_h; j++)
			{
				unsigned long long dot_matrix = 0;
				for (k=0; k<font_h; k++)
				{
					unsigned long long t = 0;
					t = get_nibble (buf[5+(j>>2)+(k*num_wide*2)]);
					if (errnum)
						goto close_file;
					dot_matrix |= ((t >> ((8*num_wide-1-j) & 3)) & 1) << k;
				}
				for (k=0; k<num_wide; k++)
					((unsigned char *)(UNIFONT_START + unicode*num_wide*font_h))[j*num_wide+k] = (dot_matrix >> k*8)&0xff;
			}
    }
    valid_lines++;
  } /* while */

close_file:

  if (errnum && len)	//如果有错误,并且长度非0. 即没有正常加载字库  与内置字库有关
  {
		filepos = saved_filepos;  //从失败处重新开始查找字库
    i=0;
    while ((len = grub_read((unsigned long long)(grub_size_t)(char*)&buf, 1, 0xedde0d90)))
    {
      if (buf[0] == '\n' || buf[0] == '\r')
      {
        goto redo;	/* try the new line */
      }
      if (buf[0] == '\0')	/* NULL encountered ? */
        break;		/* yes, end */

      if ((buf[0] | 0x20)== tag[i]) //{'d','o','t','s','i','z','e','='}
        i++;
      else
        i=0;
      if (i==8)
      {
        grub_read((unsigned long long)(grub_size_t)(char*)&buf, 10, 0xedde0d90);
        char *p = (char *)buf;
        i=0;
        unifont_simp_on = 0;
        if (safe_parse_maxint (&p, &val))
        {
          if (font_h != val)
          {
            font_h = val;			//40
            font_w = val>>1;	//20
            current_term->max_lines = current_y_resolution / (font_h + line_spacing);
            current_term->chars_per_line = current_x_resolution / (font_w + font_spacing);
//            memset ((char *)UNIFONT_START, 0, 0x800000);
            num_wide = (font_h+7)/8;					//5
            num_narrow = ((font_h/2+7)/8)<<1;	//6
            if ((p[1]|0x20)=='s' && (p[2]|0x20)=='i' && (p[3]|0x20)=='m' && (p[4]|0x20)=='p')
              unifont_simp_on = 1;
          }
        }
        filepos -= 7;
      }
    }
  }
  grub_close();
  //old_narrow_char_indicator = narrow_indicator;
//#undef	old_narrow_char_indicator

  return valid_lines;	/* success */
}

static struct builtin builtin_font =
{
  "font",
  font_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "font [--font-high=font_h] [--simp=[start0,end0,...,start7,end7]] [FILE]",
  "Default font_h=16.  chinese can use '--simp='.\n"	
  "Load unifont file FILE, or clear the font if no FILE specified.\n"
	"The font should be the same height and width.\n"
	"The built-in Font head should have 'DotSize=[font_h],['simp']'."
};
#endif /* SUPPORT_GRAPHICS */

void ascii_to_hex (char *arg, char *buf);
void
ascii_to_hex (char *arg, char *buf)
{
	char val;
	while (*arg)
	{
		if (*arg == '-')
			arg++;
		if (*arg <= '9' && *arg >= '0')
			val = *arg & 0xf;
		else if ((*arg | 0x20) <= 'f' && (*arg | 0x20) >= 'a')
			val = (*arg + 9) & 0xf;
		else
			break;
			
		arg++;
		if (*arg <= '9' && *arg >= '0')
			val = (val << 4) | (*arg & 0xf);
		else if ((*arg | 0x20) <= 'f' && (*arg | 0x20) >= 'a')
			val = (val << 4) | ((*arg + 9) & 0xf);
		else
			break;
			
		*buf++ = val;
		arg++;
	}
}

int primary;

/* uuid */
/* List filesystem UUID in all of partitions or search for filesystem
 * with specified UUID and set the partition as root.
 * Contributed by Jing Liu ( fartersh-1@yahoo.com )
 */
//static void print_root_device (char *buffer);
static void get_uuid (char* uuid_found, int tag);
static void get_vol (char* vol_found, int tag);
static int uuid_func (char *argument, int flags);
static int
uuid_func (char *argument, int flags)
{
#define tem chainloader_file
  unsigned int drive;
  unsigned int tmp_drive = saved_drive;
  unsigned int tmp_partition = saved_partition;
  char root_found[16] = "";
  char uuid_found[256];
//  char tem[256];
  grub_memset(tem, 0, 256);
	char uuid_tag[5] = {'U','U','I','D',0};
	char vol_tag[12] = {'V','o','l','u','m','e',' ','N','a','m','e',0};
	char *p;
	char *arg = tem;
	int write = 0, i = 0, j = 0;
	primary = 0;
  struct grub_disk_data *d;	//磁盘数据

	while (*argument && *argument != '\n' && *argument != '\r' && *argument != '(')
	{
    if (grub_memcmp (argument, "--write", 7) == 0)
    {
      write = 1;
      argument += 7;
    }
    else if (grub_memcmp (argument, "--primary", 9) == 0)
    {
      primary = 1;
      argument += 9;
    }
    else
      break;
    argument = skip_to (0, argument);
	}
	
	if (flags)
		p = uuid_tag;
	else
		p = vol_tag;

	while (argument[i])
	{
		if (argument[i] == '"' || argument[i] == '\\' )
		{
			i++;
			continue;
		}
		arg[j++] = argument[i++];
	}
	arg[j] = 0;
	
	if (*arg == '(')
	{
		set_device (arg);
		if (errnum)
			return 0;
		if (! open_device ())
			return 0;
		grub_memset(uuid_found, 0, 256);
		if (errnum != ERR_FSYS_MOUNT && fsys_type < NUM_FSYS && !write)
		{
			if (flags)
				get_uuid (uuid_found,0);
			else
				get_vol (uuid_found,0);
		}
		arg = skip_to (0, arg);
		if (! *arg && !write)
		{
			/* Print the type of the filesystem.  */
			if (debug > 0)
			{
				print_root_device (NULL,1);
				grub_printf (": %s is \"%s\".\n\t", p, ((*uuid_found) ? uuid_found : "(unsupported)"));
				print_fsys_type();
			}
			saved_drive = tmp_drive;
			saved_partition = tmp_partition;
			errnum = ERR_NONE;
			sprintf(ADDR_RET_STR,uuid_found);
			return (*uuid_found);
		}
		if (*arg && write && flags)
		{
			ascii_to_hex (arg, uuid_found);
			get_uuid (uuid_found,1);
			return 1;
		}
		if (*arg && write && !flags)
		{
			p = uuid_found;
			while (*arg)
				*p++ = *arg++;
			get_vol (uuid_found,1);
			return 1;
 		}
		if (write)
			return ! (errnum = ERR_BAD_ARGUMENT);
		errnum = ERR_NONE;
		return ! substring ((char*)uuid_found, arg,1);
	}
	if (write)
		return ! (errnum = ERR_BAD_ARGUMENT);
  errnum = 0;

  char *mem_probe = grub_malloc (0x1000);
  if (!mem_probe)
    return 0;

	for (drive = 0; drive <= 0xff; drive++)
  {
		int bsd_part = 0xff;
		int pc_slice = 0xff;
    
    if (drive == FB_DRIVE)
      continue;

    d = get_device_by_drive (drive,0);
    if (! d)	//如果设备=0, 错误
      continue;
#if 0
    if (drive < (unsigned int)floppies_orig || (drive >=(unsigned int)0x80 && drive < (unsigned int)0x80 + harddrives_orig))
			goto yyyyy;
		if (drive < 0xa0)	
 			continue;
		else if ((drive&0xf) >= (unsigned int)cdrom_orig)
			break;
yyyyy:
#endif

		saved_drive = current_drive = drive;
		saved_partition = current_partition = 0xFFFFFF;
		if (drive >= (unsigned int)0x80 && drive <= (unsigned int)0x8f /*&& grub_memcmp(fsys_table[fsys_type].name, "iso9660", 7) != 0*/)
		{
			grub_efidisk_readwrite (drive,(unsigned long long)0,0x1000,mem_probe,0xedde0d90);
			if (!(fb_status && drive == (unsigned char)(fb_status >> 8)) && !(probe_bpb((struct master_and_dos_boot_sector *)mem_probe)) && open_device())
			{			
				goto qqqqqq;
			}
			else if (probe_mbr((struct master_and_dos_boot_sector *)mem_probe,0,1,0))
			{
				continue;	
			}
		}
		else
		{
			if (open_device ())
			{
				goto qqqqqq;
			}
			else
				continue;
		}
		saved_drive = current_drive = drive;
    struct grub_part_data *q;
    for (i=-1; i < 16 ; i++)
    {
      if (i == -1)
        if (open_device ())
          goto qqqqqq;
      q = get_partition_info (drive, (i<<16 | 0xffff));
      if (!q)
        continue;		
      if (/* type != PC_SLICE_TYPE_NONE
          && */ ! IS_PC_SLICE_TYPE_BSD (q->partition_type)
          && ! IS_PC_SLICE_TYPE_EXTENDED (q->partition_type) && q->partition_start && q->partition_size)
      {
        current_partition = q->partition;
        if (open_device ())
        {
          bsd_part = (q->partition >> 8) & 0xFF;
          pc_slice = q->partition >> 16;
qqqqqq:
          if (errnum != ERR_FSYS_MOUNT && fsys_type < NUM_FSYS)
          {
            grub_memset(uuid_found, 0, 256);
            if (flags)
              get_uuid(uuid_found,0);
            else
              get_vol(uuid_found,0);
          }
          if (! *arg)
          {
            grub_printf ("(%s%d%c%c%c%c):", ((drive<(unsigned int)0x80)?"fd":(drive>=(unsigned int)0xa0)?"cd":"hd"),((drive<(unsigned int)0x80)?drive:((drive<(unsigned int)0xa0)?(drive-0x80):(drive-0xa0))), ((pc_slice==0xff)?'\0':','),((pc_slice==0xff)?'\0' :(pc_slice + '0')), ((bsd_part == 0xFF) ? '\0' : ','), ((bsd_part == 0xFF) ? '\0' : (bsd_part + 'a')));
            if (*uuid_found || debug)
              grub_printf("%s%s is \"%s\".\n\t", " ", p, ((*uuid_found) ? uuid_found : "(unsupported)"));
            print_fsys_type();													
          }
          else if (substring((char*)uuid_found,arg,1) == 0)
          {
            grub_sprintf(root_found,"(%s%d%c%c%c%c)", ((drive<(unsigned int)0x80)?"fd":(drive<(unsigned int)0xa0)?"hd":"cd"),((drive<(unsigned int)0x80)?drive:((drive<(unsigned int)0xa0)?(drive-0x80):(drive-0xa0))), ((pc_slice==0xff)?'\0':','),((pc_slice==0xff)?'\0' :(pc_slice + '0')), ((bsd_part == 0xFF) ? '\0' : ','), ((bsd_part == 0xFF) ? '\0' : (bsd_part + 'a')));
            goto found;
          }
        }
      } 
      if (drive > (unsigned int)0x8f)
        break;
      /* We want to ignore any error here.  */
      errnum = ERR_NONE;
    }

    /* next_partition always sets ERRNUM in the last call, so clear it.  */
    errnum = ERR_NONE;
  }
found:
  grub_free (mem_probe);
  saved_drive = tmp_drive;
  saved_partition = tmp_partition;
  errnum = ERR_NONE;
  if (! *arg)
    return 1;
        
  if (*root_found)
  {
    printf_debug0("setting root to %s\n", root_found);
    return real_root_func(root_found,1);
  };
  errnum = ERR_NO_PART;
  return 0;
#undef tem
}

static struct builtin builtin_uuid =
{
  "uuid",
  uuid_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "uuid [--write] [DEVICE] [UUID]",
  "If DEVICE is not specified, search for filesystem with UUID in all"
  " partitions and set the partition containing the filesystem as new"
  " root (if UUID is specified), or just list uuid's of all filesystems"
  " on all devices (if UUID is not specified). If DEVICE is specified," 
  " return true or false according to whether or not the DEVICE matches"
  " the specified UUID (if UUID is specified), or just list the uuid of"
  " DEVICE (if UUID is not specified).\n"
  "uuid of DEVICE is returned in temporary variables '?'."
};

static void
get_uuid (char* uuid_found, int tag)
{
  unsigned char uuid[32] = "";
	unsigned char buf[32] = "";
	int i, n = 0xedde0d90;
    {
#ifdef FSYS_FAT
      if (grub_memcmp(fsys_table[fsys_type].name, "fat", 3) == 0)
        {
			if (tag)
			{
				for (i=0; i<4; i++)
					uuid[i] = uuid_found[3-i];
				n = 0x900ddeed;
			}
			switch (fats_type)
			{
				case 12:
				case 16:
					devread(0, 0x27, 4, (unsigned long long)(grub_size_t)uuid, n);
					break;
				case 32:
					devread(0, 0x43, 4, (unsigned long long)(grub_size_t)uuid, n);
					break;
				case 64:
					devread(0, 0x64, 4, (unsigned long long)(grub_size_t)uuid, n);
					break;
			}
				if (!tag)
          grub_sprintf(uuid_found, "%02X%02X-%02X%02X", uuid[3], uuid[2], uuid[1], uuid[0]);
          return;
        }  
#endif
#ifdef FSYS_NTFS
      if (grub_memcmp(fsys_table[fsys_type].name, "ntfs", 4) == 0)
        {
					if (tag)
					{
						for (i=0; i<8; i++)
							uuid[i] = uuid_found[7-i];
						n = 0x900ddeed;
					}
          devread(0, 0x48, 8, (unsigned long long)(grub_size_t)uuid, n);
				if (!tag)
          grub_sprintf(uuid_found, "%02X%02X%02X%02X%02X%02X%02X%02X", uuid[7], uuid[6], uuid[5], uuid[4], uuid[3], uuid[2], uuid[1], uuid[0]);
          return;
        }
#endif
#ifdef FSYS_EXT2FS
      if (grub_memcmp(fsys_table[fsys_type].name, "ext2fs", 6) == 0)
        {
					if (tag)
					{
						for (i=0; i<16; i++)
							uuid[i] = uuid_found[i];
						n = 0x900ddeed;
					}
          devread(2, 0x68, 16, (unsigned long long)(grub_size_t)uuid, n);
				if (!tag)
          grub_sprintf(uuid_found, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
          return;
        }
#endif
#ifdef FSYS_REISERFS
      if (grub_memcmp(fsys_table[fsys_type].name, "reiserfs", 8) == 0)
        {
          char version[9];
          devread(0x10, 52, 9, (unsigned long long)(grub_size_t)version, 0xedde0d90);
          if (grub_memcmp(version, "ReIsEr2Fs", 9) == 0 || grub_memcmp(version, "ReIsEr3Fs", 9) == 0)
            devread(0x10, 84, 16, (unsigned long long)(grub_size_t)uuid, 0xedde0d90);
          else
            {
              devread(0x10, 0, 7, (unsigned long long)(grub_size_t)version, 0xedde0d90);
              if (grub_memcmp(version, "ReIsEr4", 7) == 0)
                devread(0x10, 20, 16, (unsigned long long)(grub_size_t)uuid, 0xedde0d90);
              else
                return;
            }
          grub_sprintf(uuid_found, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
          return;
        }
#endif
#ifdef FSYS_JFS
      if (grub_memcmp(fsys_table[fsys_type].name, "jfs", 3) == 0)
        {
          devread(0x40, 136, 16, (unsigned long long)(grub_size_t)uuid, 0xedde0d90);
          grub_sprintf(uuid_found, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
          return;
        }
#endif
#ifdef FSYS_XFS
      if (grub_memcmp(fsys_table[fsys_type].name, "xfs", 3) == 0)
        {
          devread(2, 32, 16, (unsigned long long)(grub_size_t)uuid, 0xedde0d90);
          grub_sprintf(uuid_found, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
          return;
        }
#endif
#ifdef FSYS_ISO9660
	if (grub_memcmp(fsys_table[fsys_type].name, "iso9660", 7) == 0)
	{
		emu_iso_sector_size_2048 = 1;
		devread(0x10, 0x33e, 16, (unsigned long long)(grub_size_t)(char *)uuid, 0xedde0d90);
		ascii_to_hex ((char *)uuid, (char *)buf);
		grub_sprintf(uuid_found, "%02x%02x-%02x-%02x-%02x-%02x-%02x-%02x", buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
		return;
	}
#endif
    }
}

static int
vol_func (char *arg, int flags)
{
	return uuid_func (arg, 0);
}

static struct builtin builtin_vol =
{
  "vol",
  vol_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "vol [--write | --primary] [DEVICE] [VOLUME]",
  "If DEVICE is not specified, search for filesystem with volume in all"
  " partitions and set the partition containing the filesystem as new"
  " root (if VOLUME is specified), or just list volume's of all filesystems"
  " on all devices (if VOLUME is not specified). If DEVICE is specified," 
  " return true or false according to whether or not the DEVICE matches"
  " the specified Volume (if VOLUME is specified), or just list the volume of"
  " DEVICE (if VOLUME is not specified)."
  " Use --primary for ISO Primary Volume Descriptor (as used by linux).\n"
  "vol of DEVICE is returned in temporary variables '?'."
};

int read_mft(char* buf,unsigned int mftno);
static void
get_vol (char* vol_found, int flags)
{
	int i, j, n = 0xedde0d90;
	unsigned char uni[256]={0};

	if (flags)
		n = 0x900ddeed;
	
	if (grub_memcmp(fsys_table[fsys_type].name, "iso9660", 7) == 0)
	{
		if (primary)
		{
			emu_iso_sector_size_2048 = 1;
				devread(0x10, 0x28, 0x20, (unsigned long long)(grub_size_t)(char *)vol_found, n);
			goto pri;
		}
#define BUFFER (unsigned char *)(FSYS_BUF + 0x4000)
		if (flags && iso_type != ISO_TYPE_udf)
			for (i = grub_strlen(vol_found); i<0x20; i++)
				vol_found[i] = 0x20;

		unsigned short *pb = (unsigned short *)&uni[1];
		switch (iso_type)
		{
			case ISO_TYPE_9660:
			case ISO_TYPE_RockRidge:
				emu_iso_sector_size_2048 = 1;
				devread(0x10, 0x28, 0x20, (unsigned long long)(grub_size_t)(char *)vol_found, n);
				break;
			case ISO_TYPE_Joliet:
				if (flags)
				{
					for (i = 0; i < 0x10; i++)
						pb[i] = vol_found[i];
				}
				emu_iso_sector_size_2048 = 1;
				devread(*(unsigned int *)FSYS_BUF, 0x28, 0x20, (unsigned long long)(grub_size_t)(char *)uni, n);
				if (!flags)
				{
					big_to_little ((char *)uni, 0x20);
					unicode_to_utf8 ((unsigned short *)uni, (unsigned char *)vol_found, 0x10);
				}
				break;
			case ISO_TYPE_udf:
				if (udf_BytePerSector == 0x800)
				emu_iso_sector_size_2048 = 1;
				devread(*(unsigned int *)FSYS_BUF, 0, udf_BytePerSector, (unsigned long long)(grub_size_t)(char *)BUFFER, 0xedde0d90);
				if (!flags)
				{
					if (*(BUFFER + 0x70) == 16)
					{
						big_to_little ((char *)(BUFFER + 0x71), 30);
						unicode_to_utf8 ((unsigned short *)(BUFFER + 0x71), (unsigned char *)vol_found, 15);
					}
					else
					{
						unsigned char *pa = (unsigned char *)(BUFFER + 0x71);
						i = 0;
						while ((*vol_found++ = *pa++) && i++ < 30);
						*vol_found = 0;	
					}
				}
				else
				{
					i = 0;
					if (*(BUFFER + 0x70) == 16)
						while (pb[i] = vol_found[i], i++ <15);
					else
						while (uni[i] = vol_found[i], i++ < 30);
					grub_memmove ((unsigned char *)(BUFFER + 0x71),uni,30);
					grub_memmove ((unsigned char *)(BUFFER + 0x131),uni,30);
					*(unsigned short *)(BUFFER + 8) = grub_crc16 ((unsigned char *)(BUFFER+0x10), *(unsigned short *)(BUFFER+0xa));
					unsigned char h = 0;
					for (i=0; i<16; i++)
					{
						if (i==4)
							continue;
						h += *(unsigned char *)(BUFFER + i);
					}
					*(unsigned char *)(BUFFER + 4) = h;

					if (udf_BytePerSector == 0x800)
					emu_iso_sector_size_2048 = 1;
					devread(*(unsigned int *)FSYS_BUF, 0, udf_BytePerSector, (unsigned long long)(grub_size_t)(char *)BUFFER, 0x900ddeed);
				}
				break;
		}
pri:
		if (!flags && (iso_type != ISO_TYPE_udf || primary))
		{
			if (iso_type == ISO_TYPE_Joliet && !primary)
				j = 0x10 - 1;
			else
				j = 0x20 - 1;
			for (i=j; i>=0; i--)
			{
				if (vol_found[i] != ' ')
					break;
				vol_found[i] = 0;
			}
		}
#undef BUFFER
	}
	else if (grub_memcmp(fsys_table[fsys_type].name, "ntfs", 4) == 0)
	{
#define BUFFER (unsigned char *)(FSYS_BUF + 0x8000)
		unsigned char *pa;
		unsigned char *pb;

		read_mft((char*)BUFFER,3);
//		pa = BUFFER+0x38;
		pa = BUFFER + *(unsigned short *)(BUFFER+0x14);
		while (*pa != 0xFF)
		{
			if (*pa == 0x60)
				break;
			pa += pa[4];
		}
		if (pa[8]==0)
		{
			if (!flags && *pa == 0x60)
			{
				j = pa[0x10]&255;
				pa += pa[0x14];
				for (i=0; i<j; i++)
					uni[i] = pa[i];
				uni[i] = 0;
				uni[i+1] = 0;
				unicode_to_utf8 ((unsigned short *)uni, (unsigned char *)vol_found, 255);
			}
			else if (flags)
			{
				i = (((j = ((grub_strlen(vol_found))*2)&255) + 7)&0xfff8) + 0x18;
				if (*pa == 0xff)
					pa[4] = 0;
				if (*pa == 0xff || (i > pa[4] && *pa == 0x60))
				{
					if (i - pa[4] + *(unsigned int *)(BUFFER + 0x18) > *(unsigned int *)(BUFFER + 0x1c))
						return;			
					if (*pa == 0xFF)
					{
						grub_memset(pa, 0, i);
						pa[0] = 0x60;
						pa[0xa] = pa[0x14] = 0x18;
						pa[0xe] = 4;
						*(unsigned int *)&pa[i] = 0xffffffff;
					}
					else if (j > pa[4] - 0x18)
						grub_memmove (pa + i,pa + pa[4],(BUFFER + *(unsigned int *)(BUFFER + 0x18)) - (pa + pa[4]));

					*(unsigned int *)(BUFFER + 0x18) += i - pa[4];
					pa[4] = i;
				}

				pa[0x10] = j;
				pa += pa[0x14];
				unsigned short *pc = (unsigned short *)pa;
				for (i=0; i<j/2; i++)
					*pc++ = *vol_found++;
			
				i = j = *(unsigned short *)(BUFFER + 6) - 1;
				pa = BUFFER + *(unsigned short *)(BUFFER + 4);
				pb = BUFFER-2;
				unsigned short k = *pa;
				while (i>0)
				{
					pb += 0x200;
					pa += 2;
					*(unsigned short *)pa = *(unsigned short *)pb;
					*(unsigned short *)pb = k;
					i--;
				}
				devread(*(unsigned long long *)(FSYS_BUF+0x9000), 0, j*0x200, (unsigned long long)(grub_size_t)BUFFER, 0x900ddeed);
				devread(*(unsigned long long *)(FSYS_BUF+0x9008), 0, j*0x200, (unsigned long long)(grub_size_t)BUFFER, 0x900ddeed);
			}
		}
#undef BUFFER
	}
	else if (grub_memcmp(fsys_table[fsys_type].name, "fat", 3) == 0)
	{
#define SUPERBLOCK (unsigned char *)(FSYS_BUF + 0x7000)
		unsigned int back_drive = saved_drive;
		unsigned int back_partition = saved_partition;
		saved_drive = current_drive;
		saved_partition = current_partition;
		i = dir ("()/$v\0");
		saved_drive = back_drive;
		saved_partition = back_partition;
		if (!i && ((*(unsigned int *)(SUPERBLOCK+0x14)) ? ((filepos-32) >= *(unsigned int *)(SUPERBLOCK+0x14)) : (((filepos-32)&((1<<*(unsigned int *)(SUPERBLOCK+0x34))-1)) == 0)))
			return;
		
		if (flags)
			n = 0x900ddeed;
		
		filepos -= 32;
		
		if (fats_type != 64)
		{
			for (i = grub_strlen(vol_found); flags && i<11; i++)
				vol_found[i] = 0x20;
			vol_found[11] = 8;
			devread (*(unsigned long long *)(SUPERBLOCK+0x58)+(filepos>>9), filepos&0x1ff, 12, (unsigned long long)(grub_size_t)vol_found, n);
			if (flags)
				return;
			vol_found[11] = 0;
			for (i=10; i>=0; i--)
			{
				if (vol_found[i] != ' ')
					break;
				vol_found[i] = 0;
			}
		}
		else
		{
			if (!flags)
			{
				devread (*(unsigned long long *)(SUPERBLOCK+0x58)+(filepos>>9), filepos&0x1ff, 32, (unsigned long long)(grub_size_t)vol_found, 0xedde0d90);
				for (i=0; i < vol_found[1]*2; i++)
					uni[i]	= vol_found[2+i];
				uni[i] = 0;
				uni[i+1] = 0;
				unicode_to_utf8 ((unsigned short *)uni, (unsigned char *)vol_found, 11);
			}
			else
			{
				unsigned short *pb = (unsigned short *)&uni[2];
				uni[0] = 0x83;
				if ((uni[1] = grub_strlen(vol_found)) > 11)
					uni[1] = 11;
				for (i = 0; i < uni[1]; i++)
					pb[i] = vol_found[i];
				devread (*(unsigned long long *)(SUPERBLOCK+0x58)+(filepos>>9), filepos&0x1ff, (uni[1]+1)*2, (unsigned long long)(grub_size_t)uni, 0x900ddeed);	
			}
		}
#undef SUPERBLOCK
	}  
	else if (grub_memcmp(fsys_table[fsys_type].name, "ext2fs", 6) == 0)
	{
		devread(2, 0x78, 16, (unsigned long long)(grub_size_t)vol_found, n);
	}
	else if (grub_memcmp(fsys_table[fsys_type].name, "fb", 2) == 0)
	{
		devread(0, 0x47, 11, (unsigned long long)(grub_size_t)vol_found, n);
	}
	else if (flags)
		grub_printf("Warning: No Volume in %s filesystem type.",fsys_table[fsys_type].name);
	return;
}



/* fstest */
static int fstest_func (char *arg, int flags);
static int
fstest_func (char *arg, int flags)
{

  errnum = 0;
  /* If ARG is empty, toggle the flag.  */
  if (! *arg)
  {
    if (disk_read_hook)
    {
      disk_read_hook = NULL;
      printf_debug0 (" Filesystem tracing is now off\n");
    }else{
      disk_read_hook = disk_read_print_func;
      printf_debug0 (" Filesystem tracing is now on\n");
    }
  }
  else if (grub_memcmp (arg, "on", 2) == 0)
    disk_read_hook = disk_read_print_func;
  else if (grub_memcmp (arg, "off", 3) == 0)
    disk_read_hook = NULL;
  else if (grub_memcmp (arg, "status", 6) == 0)
  {
    printf_debug0 (" Filesystem tracing is now %s\n", (disk_read_hook ? "on" : "off"));
  }
  else
      errnum = ERR_BAD_ARGUMENT;

  return (grub_size_t)disk_read_hook;
}

static struct builtin builtin_fstest =
{
  "fstest",
  fstest_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "fstest [on | off | status]",
  "Turn on/off or display the fstest mode, or toggle it if no argument."
};


/* geometry */
static int geometry_func (char *arg, int flags);
static int
geometry_func (char *arg, int flags)
{
  const char *msg = "LBA";

  /* Get the drive and the partition.  */
  if (! *arg || *arg == ' ' || *arg == '\t')
    {
	current_drive = saved_drive;
	current_partition = saved_partition;
    }
  else
    {
      if (! set_device (arg))
	return 0;
    }

  if (fb_status && current_drive == FB_DRIVE)
  {
	current_drive = (unsigned char)(fb_status >> 8);
	current_partition = 0xFFFFFF;
  }

  /* Check for the geometry.  */
  if (get_diskinfo (current_drive, &tmp_geom, 0))
    {
      errnum = ERR_NO_DISK;
      return 0;
    }

//  grub_printf ("drive 0x%02X(%s): Sector Count/Size=%ld/%d\n",
  grub_printf ("drive 0x%02X(%s): SectorCount/Size=%ld / %d\n",
	       current_drive, msg,
	       (unsigned long long)tmp_geom.total_sectors, tmp_geom.sector_size);

  errnum = 0;

  if (tmp_geom.sector_size == 512 || tmp_geom.sector_size == 4096)
  {
#define	BS	((struct master_and_dos_boot_sector *)mbr)  //主分区和dos引导扇区  0x800
    
    // Make sure rawread will not call get_diskinfo again after force_geometry_tune is reset.
    if (buf_drive != (int)current_drive)
    {
	buf_drive = current_drive;
	buf_track = -1; // invalidate track buffer
    }
    buf_geom = tmp_geom;

    /* Read MBR or the floppy boot sector.  */		//读当前驱动器逻辑0(1扇区)到0x8000
    if (! rawread (current_drive, 0, 0, SECTOR_SIZE, (unsigned long long)(grub_size_t)mbr, 0xedde0d90))
	return 0;

    if (BS->boot_signature == 0xAA55 && !(probe_mbr (BS, 0, 1, 0)))				//BS=0x8000		如果存在分区表
      real_open_partition (1);																						//则打开分区
#undef BS
  }

  if (errnum == 0)
	return 1;
  errnum = 0;	/* ignore error. */
  return 0;	/* indicates error occurred during real_open_partition. */
}

static struct builtin builtin_geometry =
{
  "geometry",
  geometry_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "geometry [DRIVE]",
  "Print the information for drive DRIVE or the current root device if DRIVE"
  " is not specified."
};

static inline grub_uint32_t
decode_length (const grub_uint8_t *ptr, int *numlen)
{
  int num_bytes, i;
  grub_uint32_t ret;
  if (*ptr < 64)
  {
    if (numlen)
      *numlen = 1;
    return *ptr;
  }
  num_bytes = *ptr >> 6;
  if (numlen)
    *numlen = num_bytes + 1;
  ret = *ptr & 0xf;
  ptr++;
  for (i = 0; i < num_bytes; i++)
  {
    ret |= *ptr << (8 * i + 4);
    ptr++;
  }
  return ret;
}

static inline grub_uint32_t
skip_name_string (const grub_uint8_t *ptr, const grub_uint8_t *end)
{
  const grub_uint8_t *ptr0 = ptr;

  while (ptr < end && (*ptr == '^' || *ptr == '\\'))
    ptr++;
  switch (*ptr)
  {
    case '.':
      ptr++;
      ptr += 8;
      break;
    case '/':
      ptr++;
      ptr += 1 + (*ptr) * 4;
      break;
    case 0:
      ptr++;
      break;
    default:
      ptr += 4;
      break;
  }
  return ptr - ptr0;
}

static inline grub_uint32_t
skip_data_ref_object (const grub_uint8_t *ptr, const grub_uint8_t *end)
{
  //DBG ("Data type = 0x%x\n", *ptr);
  switch (*ptr)
  {
    case GRUB_ACPI_OPCODE_PACKAGE:
    case GRUB_ACPI_OPCODE_BUFFER:
      return 1 + decode_length (ptr + 1, 0);
    case GRUB_ACPI_OPCODE_ZERO:
    case GRUB_ACPI_OPCODE_ONES:
    case GRUB_ACPI_OPCODE_ONE:
      return 1;
    case GRUB_ACPI_OPCODE_BYTE_CONST:
      return 2;
    case GRUB_ACPI_OPCODE_WORD_CONST:
      return 3;
    case GRUB_ACPI_OPCODE_DWORD_CONST:
      return 5;
    case GRUB_ACPI_OPCODE_STRING_CONST:
    {
      const grub_uint8_t *ptr0 = ptr;
      for (ptr++; ptr < end && *ptr; ptr++);
      if (ptr == end)
        return 0;
      return ptr - ptr0 + 1;
    }
    default:
      if (*ptr == '^' || *ptr == '\\' || *ptr == '_'
          || (*ptr >= 'A' && *ptr <= 'Z'))
        return skip_name_string (ptr, end);
      grub_printf ("Unknown opcode 0x%x\n", *ptr);
      return 0;
  }
}

static inline grub_uint32_t
skip_term (const grub_uint8_t *ptr, const grub_uint8_t *end)
{
  grub_uint32_t add;
  const grub_uint8_t *ptr0 = ptr;

  switch(*ptr)
  {
    case GRUB_ACPI_OPCODE_ADD:
    case GRUB_ACPI_OPCODE_AND:
    case GRUB_ACPI_OPCODE_CONCAT:
    case GRUB_ACPI_OPCODE_CONCATRES:
    case GRUB_ACPI_OPCODE_DIVIDE:
    case GRUB_ACPI_OPCODE_INDEX:
    case GRUB_ACPI_OPCODE_LSHIFT:
    case GRUB_ACPI_OPCODE_MOD:
    case GRUB_ACPI_OPCODE_MULTIPLY:
    case GRUB_ACPI_OPCODE_NAND:
    case GRUB_ACPI_OPCODE_NOR:
    case GRUB_ACPI_OPCODE_OR:
    case GRUB_ACPI_OPCODE_RSHIFT:
    case GRUB_ACPI_OPCODE_SUBTRACT:
    case GRUB_ACPI_OPCODE_TOSTRING:
    case GRUB_ACPI_OPCODE_XOR:
      /*
       * Parameters for these opcodes: TermArg, TermArg Target, see ACPI
       * spec r5.0, page 828f.
       */
      ptr++;
      ptr += add = skip_term (ptr, end);
      if (!add)
        return 0;
      ptr += add = skip_term (ptr, end);
      if (!add)
        return 0;
      ptr += skip_name_string (ptr, end);
      break;
    default:
      return skip_data_ref_object (ptr, end);
  }
  return ptr - ptr0;
}

static inline grub_uint32_t
skip_ext_op (const grub_uint8_t *ptr, const grub_uint8_t *end)
{
  const grub_uint8_t *ptr0 = ptr;
  int add;
  //DBG ("Extended opcode: 0x%x\n", *ptr);
  switch (*ptr)
  {
    case GRUB_ACPI_EXTOPCODE_MUTEX:
      ptr++;
      ptr += skip_name_string (ptr, end);
      ptr++;
      break;
    case GRUB_ACPI_EXTOPCODE_EVENT_OP:
      ptr++;
      ptr += skip_name_string (ptr, end);
      break;
    case GRUB_ACPI_EXTOPCODE_OPERATION_REGION:
      ptr++;
      ptr += skip_name_string (ptr, end);
      ptr++;
      ptr += add = skip_term (ptr, end);
      if (!add)
    return 0;
      ptr += add = skip_term (ptr, end);
      if (!add)
    return 0;
      break;
    case GRUB_ACPI_EXTOPCODE_FIELD_OP:
    case GRUB_ACPI_EXTOPCODE_DEVICE_OP:
    case GRUB_ACPI_EXTOPCODE_PROCESSOR_OP:
    case GRUB_ACPI_EXTOPCODE_POWER_RES_OP:
    case GRUB_ACPI_EXTOPCODE_THERMAL_ZONE_OP:
    case GRUB_ACPI_EXTOPCODE_INDEX_FIELD_OP:
    case GRUB_ACPI_EXTOPCODE_BANK_FIELD_OP:
      ptr++;
      ptr += decode_length (ptr, 0);
      break;
    default:
      grub_printf ("Unexpected extended opcode: 0x%x\n", *ptr);
      return 0;
  }
  return ptr - ptr0;
}

static int
get_sleep_type (grub_uint8_t *table, grub_uint8_t *ptr, grub_uint8_t *end,
                grub_uint8_t *scope, int scope_len)
{
  grub_uint8_t *prev = table;

  if (!ptr)
    ptr = table + sizeof (struct grub_acpi_table_header);
  while (ptr < end && prev < ptr)
  {
    int add;
    prev = ptr;
    //DBG ("Opcode 0x%x\n", *ptr);
    //DBG ("Tell %x\n", (unsigned) (ptr - table));
    switch (*ptr)
    {
      case GRUB_ACPI_OPCODE_EXTOP:
        ptr++;
        ptr += add = skip_ext_op (ptr, end);
        if (!add)
          return -1;
        break;
      case GRUB_ACPI_OPCODE_CREATE_DWORD_FIELD:
      case GRUB_ACPI_OPCODE_CREATE_WORD_FIELD:
      case GRUB_ACPI_OPCODE_CREATE_BYTE_FIELD:
      {
        ptr += 5;
        ptr += add = skip_data_ref_object (ptr, end);
        if (!add)
          return -1;
        ptr += 4;
        break;
      }
      case GRUB_ACPI_OPCODE_NAME:
        ptr++;
        if ((!scope || grub_memcmp ((void *)scope, "\\", scope_len) == 0) &&
            (grub_memcmp ((void *)ptr, "_S5_", 4) == 0 || grub_memcmp ((void *)ptr, "\\_S5_", 4) == 0))
        {
          int ll;
          grub_uint8_t *ptr2 = ptr;
          //DBG ("S5 found\n");
          ptr2 += skip_name_string (ptr, end);
          if (*ptr2 != 0x12)
          {
            grub_printf ("Unknown opcode in _S5: 0x%x\n", *ptr2);
            return -1;
          }
          ptr2++;
          decode_length (ptr2, &ll);
          ptr2 += ll;
          ptr2++;
          switch (*ptr2)
          {
            case GRUB_ACPI_OPCODE_ZERO:
              return 0;
            case GRUB_ACPI_OPCODE_ONE:
              return 1;
            case GRUB_ACPI_OPCODE_BYTE_CONST:
              return ptr2[1];
            default:
              grub_printf ("Unknown data type in _S5: 0x%x\n", *ptr2);
              return -1;
          }
        }
        ptr += add = skip_name_string (ptr, end);
        if (!add)
          return -1;
        ptr += add = skip_data_ref_object (ptr, end);
        if (!add)
          return -1;
        break;
      case GRUB_ACPI_OPCODE_ALIAS:
        ptr++;
        /* We need to skip two name strings */
        ptr += add = skip_name_string (ptr, end);
        if (!add)
          return -1;
        ptr += add = skip_name_string (ptr, end);
        if (!add)
          return -1;
        break;

      case GRUB_ACPI_OPCODE_SCOPE:
      {
        int scope_sleep_type;
        int ll;
        grub_uint8_t *name;
        int name_len;

        ptr++;
        add = decode_length (ptr, &ll);
        name = ptr + ll;
        name_len = skip_name_string (name, ptr + add);
        if (!name_len)
          return -1;
        scope_sleep_type = get_sleep_type (table, name + name_len,
                           ptr + add, name, name_len);
        if (scope_sleep_type != -2)
          return scope_sleep_type;
        ptr += add;
        break;
      }
      case GRUB_ACPI_OPCODE_IF:
      case GRUB_ACPI_OPCODE_METHOD:
      {
        ptr++;
        ptr += decode_length (ptr, 0);
        break;
      }
      default:
        grub_printf ("Unknown opcode 0x%x\n", *ptr);
        return -1;
    }
  }
  return -2;
}

static void
grub_acpi_halt (void)
{
  struct grub_acpi_rsdp_v20 *rsdp2 = NULL;
  struct grub_acpi_rsdp_v10 *rsdp1 = NULL;
  struct grub_acpi_table_header *rsdt;
  grub_uint32_t *entry_ptr;
  grub_uint32_t port = 0;
  int sleep_type = -1;
  unsigned i;
  grub_efi_packed_guid_t acpi_guid = GRUB_EFI_ACPI_TABLE_GUID;
  grub_efi_packed_guid_t acpi20_guid = GRUB_EFI_ACPI_20_TABLE_GUID;

  for (i = 0; i < grub_efi_system_table->num_table_entries; i++)
  {
    grub_efi_packed_guid_t *guid =
        &grub_efi_system_table->configuration_table[i].vendor_guid;
    if (! grub_memcmp ((void *)guid, (void *)&acpi20_guid,
        sizeof (grub_efi_packed_guid_t)))
    {
      rsdp2 = grub_efi_system_table->configuration_table[i].vendor_table;
      break;
    }
  }

  if (rsdp2)
    rsdp1 = &(rsdp2->rsdpv1);
  else
  {
    for (i = 0; i < grub_efi_system_table->num_table_entries; i++)
    {
      grub_efi_packed_guid_t *guid =
        &grub_efi_system_table->configuration_table[i].vendor_guid;
      if (! grub_memcmp ((void *)guid, (void *)&acpi_guid,
          sizeof (grub_efi_packed_guid_t)))
      {
        rsdp1 = grub_efi_system_table->configuration_table[i].vendor_table;
        break;
      }
    }
  }
  if (!rsdp1)
  {
    grub_printf ("ACPI shutdown not supported.\n");
    return;
  }

  rsdt = (struct grub_acpi_table_header *) (grub_addr_t) rsdp1->rsdt_addr;
  for (entry_ptr = (grub_uint32_t *) (rsdt + 1);
       entry_ptr < (grub_uint32_t *) (((grub_uint8_t *) rsdt) + rsdt->length);
       entry_ptr++)
  {
    if (grub_memcmp ((void *) (grub_addr_t) *entry_ptr, "FACP", 4) == 0)
    {
      struct grub_acpi_fadt *fadt
        = ((struct grub_acpi_fadt *) (grub_addr_t) *entry_ptr);
      struct grub_acpi_table_header *dsdt
        = (struct grub_acpi_table_header *) (grub_addr_t) fadt->dsdt_addr;
      grub_uint8_t *buf = (grub_uint8_t *) dsdt;

      port = fadt->pm1a;

      //DBG ("PM1a port=%x\n", port);

      if (grub_memcmp ((void *)dsdt->signature, "DSDT", sizeof (dsdt->signature)) == 0
          && sleep_type < 0)
        sleep_type = get_sleep_type (buf, NULL, buf + dsdt->length, NULL, 0);
    }
    else if (grub_memcmp ((void *) (grub_addr_t) *entry_ptr, "SSDT", 4) == 0
             && sleep_type < 0)
    {
      struct grub_acpi_table_header *ssdt
        = (struct grub_acpi_table_header *) (grub_addr_t) *entry_ptr;
      grub_uint8_t *buf = (grub_uint8_t *) ssdt;

      //DBG ("SSDT = 0x%x\n", (unsigned) (grub_addr_t) ssdt);

      sleep_type = get_sleep_type (buf, NULL, buf + ssdt->length, NULL, 0);
    }
  }

  //DBG ("SLP_TYP = %d, port = 0x%x\n", sleep_type, port);
  if (port && sleep_type >= 0 && sleep_type < 8)
  {
    //DBG ("Sending shutdown signal\n");
    //outw (GRUB_ACPI_SLP_EN | (sleep_type << GRUB_ACPI_SLP_TYP_OFFSET), port & 0xffff);
    asm volatile ("outw %w0,%w1" : :"a" ((unsigned short int)
        (GRUB_ACPI_SLP_EN | (sleep_type << GRUB_ACPI_SLP_TYP_OFFSET))),
        "Nd" ((unsigned short int) (port & 0xffff)));
  }

  efi_call_1 (grub_efi_system_table->boot_services->stall, 1000000);

  grub_printf ("ACPI shutdown failed\n");
}

/* halt */
static int halt_func (char *arg, int flags);
static int
halt_func (char *arg, int flags)
{
  grub_acpi_halt ();
  grub_halt ();
  /* Never reach here.  */
  return 0;
}

static struct builtin builtin_halt =
{
  "halt",
  halt_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "halt [--no-apm] [--no-acpi]",
  "Halt the system using ACPI and APM."
  "\nIf --no-acpi is specified, only APM is to be tried."
  "\nIf --no-apm is specified, only ACPI is to be tried."
  "\nif both options are specified, return to grub4dos with failure."
};


/* help */
static void print_doc(char *doc,int left);
static void print_doc(char *doc,int left)
{
	int max_doc_len = current_term->chars_per_line;
	if (putchar_hooked)
	{
		grub_printf(doc);
		return;
	}

	while (*doc)
	{
		int i;
		if (quit_print)
			break;
		for(i=0;doc[i];)
		{
			if (grub_isspace(doc[i++]))
				break;
		}
		if ((int)(fontx+i)>max_doc_len)
		{
			putchar('\n',255);
		}
		while ((int)fontx < left)
			grub_putchar(' ',255);
		doc += grub_printf("%.*s",i,doc);
	}
	grub_putchar('\n',255);
}

static int help_func (char *arg, int flags);
static int
help_func (char *arg, int flags)
{
  int all = 0;
  int MAX_SHORT_DOC_LEN = current_term->chars_per_line/2-1;
  errnum = 0;
  quit_print = 0;
  if (grub_memcmp (arg, "--all", sizeof ("--all") - 1) == 0)
    {
      all = 1;
      arg = skip_to (0, arg);
    }
  
  if (! *arg)
    {
      /* Invoked with no argument. Print the list of the short docs.  */
      struct builtin **builtin;
      int left = 1;

      for (builtin = builtin_table; *builtin != 0; builtin++)
	{
	  /* If this cannot be used in the command-line interface,
	     skip this.  */
	  if (! ((*builtin)->flags & BUILTIN_CMDLINE) && (*builtin)->flags)
	    continue;
	  /* If this doesn't need to be listed automatically and "--all"
	     is not specified, skip this.  */
	  if (! all && ! ((*builtin)->flags & BUILTIN_HELP_LIST) && (*builtin)->flags)
	    continue;
		int i,j=MAX_SHORT_DOC_LEN;
		for (i = 0; (i < MAX_SHORT_DOC_LEN) && ((*builtin)->short_doc[i] != 0); i++)
		{
			if ((*builtin)->short_doc[i] == '\n')
			{
				j=i;
				break;
			}
		}
			printf("%-*.*s",MAX_SHORT_DOC_LEN,j-1,(*builtin)->short_doc?(*builtin)->short_doc:(*builtin)->name);

	  if (! left)
	    grub_putchar ('\n', 255);

	  left = ! left;
	}

      /* If the last entry was at the left column, no newline was printed
	 at the end.  */
      if (! left)
	grub_putchar ('\n', 255);
    }
  else
    {
      /* Invoked with one or more patterns.  */
      do
	{
	  struct builtin **builtin;
	  char *next_arg;

	  /* Get the next argument.  */
	  next_arg = skip_to (0, arg);

	  /* Terminate ARG.  */
	  nul_terminate (arg);

	  for (builtin = builtin_table; *builtin; builtin++)
	    {
	      if (substring (arg, (*builtin)->name, 0) < 1 && (*builtin)->short_doc)
		{
		  /* At first, print the name and the short doc.  */
		  grub_printf ("%s:",(*builtin)->name);
		  print_doc((*builtin)->short_doc,4);
		  /* Print the long doc.  */
		  print_doc((*builtin)->long_doc,3);
	  		if (quit_print)
				break;
		}
	    }

	  arg = next_arg;
	}
      while (*arg);
    }

  return 1;
}

static struct builtin builtin_help =
{
  "help",
  help_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "help [--all | PATTERN ...]",
  "Display information about built-in commands. Use option `--all' to show all commands."
};


/* hiddenmenu */
static int hiddenmenu_func (char *arg, int flags);
static int
hiddenmenu_func (char *arg, int flags)
{
  errnum = 0;
  show_menu = 0;

  while (*arg)
  {
    if (grub_memcmp (arg, "--silent", 8) == 0)
      {
        silent_hiddenmenu = 1;
      }
    else if (grub_memcmp (arg, "--off", 5) == 0)
      {
	/* set to the default values. */
	show_menu = 1;
        silent_hiddenmenu = 0;
      }
      else if (grub_memcmp (arg, "--chkpass", 9) == 0)
      {
	unsigned long long t = 0x11bLL;
	arg = skip_to(1,arg);
	safe_parse_maxint(&arg,&t);
	errnum = 0;
	silent_hiddenmenu = t;
      }
    arg = skip_to (0, arg);
  }

  return 1;
}

static struct builtin builtin_hiddenmenu =
{
  "hiddenmenu",
  hiddenmenu_func,
  BUILTIN_MENU,
};

/* hide */
static int set_partition_hidden_flag (int hidden);
static int hide_func (char *arg, int flags);
static int
hide_func (char *arg, int flags)
{
  errnum = 0;
  /* If no arguments, hide current partition in the current drive. */
  if (! *arg || *arg == ' ' || *arg == '\t')
  {
	current_drive = saved_drive;
	current_partition = saved_partition;
  }
  else if (! set_device (arg))
    return 0;

  return set_partition_hidden_flag (1);
}

static struct builtin builtin_hide =
{
  "hide",
  hide_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "hide [PARTITION]",
  "Hide PARTITION by setting the \"hidden\" bit in"
  " its partition type code. The default partition is the current"
  " root device."
};


/* hiddenflag [--set | --clear] [PARTITION] */
static int hiddenflag_func (char *arg, int flags);
static int
hiddenflag_func (char *arg, int flags)
{
  int hidden = -1;	/* -1 for status report */

  errnum = 0;
  for (;;)
  {
    if (grub_memcmp (arg, "--set", 5) == 0)
      {
	hidden = 1;
      }
    else if (grub_memcmp (arg, "--clear", 7) == 0)
      {
	hidden = 0;
      }
    else
	break;
    arg = skip_to (0, arg);
  }
  
  if (! *arg || *arg == ' ' || *arg == '\t')
  {
	current_drive = saved_drive;
	current_partition = saved_partition;
  }
  else if (! set_device (arg))
    return 0;

  return set_partition_hidden_flag (hidden);
}

static struct builtin builtin_hiddenflag =
{
  "hiddenflag",
  hiddenflag_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "hiddenflag [--set | --clear] [PARTITION]",
  "Hide/unhide PARTITION by setting/clearing the \"hidden\" bit in"
  " its partition type code, or report the hidden status."
  " The default partition is the current root device."
};

static void *
allocate_pages_max (grub_efi_physical_address_t max,
                    grub_efi_uintn_t pages)
{
  grub_efi_status_t status;
  grub_efi_boot_services_t *b;
  grub_efi_physical_address_t address = max;
  if (max > 0xffffffff)
    return 0;
  b = grub_efi_system_table->boot_services;
  status = efi_call_4 (b->allocate_pages, GRUB_EFI_ALLOCATE_MAX_ADDRESS,
                       GRUB_EFI_LOADER_DATA, pages, &address);

  if (status != GRUB_EFI_SUCCESS)
    return 0;
  if (address == 0)
  {
    /* Uggh, the address 0 was allocated... This is too annoying,
     * so reallocate another one.  */
    address = max;
    status = efi_call_4 (b->allocate_pages, GRUB_EFI_ALLOCATE_MAX_ADDRESS,
                         GRUB_EFI_LOADER_DATA, pages, &address);
    grub_efi_free_pages (0, pages);
    if (status != GRUB_EFI_SUCCESS)
	  return 0;
  }
  return (void *) ((grub_addr_t) address);
}

/* initrd */
static int initrd_func (char *arg, int flags);
static int
initrd_func (char *arg, int flags)
{
  char *name = arg;
  errnum = 0;
  grub_size_t size = 0;
  grub_efi_boot_services_t *b;
  b = grub_efi_system_table->boot_services;
  /* TODO: add support for multiple initrd files */
  if (kernel_type != KERNEL_TYPE_LINUX)
  {
    errnum = ERR_NEED_LX_KERNEL;
    return 0;
  }
  while (*name && *name != ' ' && *name != '\t')
  {
    grub_open (name);
    if (errnum)
    {
      printf_errinfo ("Failed to open %s\n", arg);
      goto fail;
    }
    size += ALIGN_UP (filemax, 4096);
    grub_close ();
    name = skip_to (0, name);
  }
  if (!size)
  {
    printf_errinfo ("invalid initrd size\n");
    goto fail;
  }
  initrdefi_mem = allocate_pages_max (0x3fffffff, BYTES_TO_PAGES(size));
  if (!initrdefi_mem)
  {
    printf_errinfo ("Failed to allocate initrd memory\n");
    goto fail;
  }
  grub_memset (initrdefi_mem, 0, size);
  name = arg;
  size = 0;
  while (*name && *name != ' ' && *name != '\t')
  {
    grub_open (name);
    if (errnum)
    {
      printf_errinfo ("Failed to open %s\n", arg);
      goto fail;
    }
    if (grub_read ((unsigned long long)(grub_size_t)initrdefi_mem + size,
                   filemax, 0xedde0d90) != filemax)
    {
      printf_errinfo ("premature end of file %s", arg);
      goto fail;
    }
    size += ALIGN_UP (filemax, 4096);
    grub_close ();
    name = skip_to (0, name);
  }
  initrdefi_size = size;
  grub_close ();
  return 1;

fail:
  if (initrdefi_mem)
    efi_call_2 (b->free_pages, (grub_size_t)initrdefi_mem, BYTES_TO_PAGES(size));
  grub_close ();
  return 0;
}

static struct builtin builtin_initrd =
{
  "initrd",
  initrd_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_NO_DECOMPRESSION,
  "initrd [@name=]FILE [@name=][FILE ...]",
  "Load an initial ramdisk FILE for a Linux format boot image and set the"
  " appropriate parameters in the Linux setup area in memory. For Linux"
  " 2.6+ kernels, multiple cpio files can be loaded."
};

/* is64bit */
static int is64bit_func (char *arg, int flags);
static int
is64bit_func (char *arg, int flags)
{
  return is64bit;
}

static struct builtin builtin_is64bit =
{
  "is64bit",
  is64bit_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "is64bit",
  "check 64bit and PAE"
  "return value bit0=PAE supported bit1=AMD64/Intel64 supported"
};

/* kernel */
static int kernel_func (char *arg, int flags);
static int
kernel_func (char *arg, int flags)
{
  int ret = chainloader_func (arg, flags);
  if (ret)
    kernel_type = KERNEL_TYPE_LINUX;
  return ret;
}

static struct builtin builtin_kernel =
{
  "kernel",
  kernel_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "kernel FILE [ARG ...]",
  "Attempt to load the primary boot image from FILE. The rest of the"
  " line is passed verbatim as the \"kernel command line\".  Any modules"
  " must be reloaded after using this command."
};


/* lock */
static int lock_func (char *arg, int flags);
static int
lock_func (char *arg, int flags)
{
  errnum = 0;
  if (! auth && password_buf)
    {
      errnum = ERR_PRIVILEGED;
      return 0;
    }

  return 1;
}

static struct builtin builtin_lock =
{
  "lock",
  lock_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "lock",
  "Break a command execution unless the user is authenticated. errorcheck must be on."
};
  

/* ls */
static int
ls_func (char *arg, int flags)
{
  errnum = 0;
  /* If no arguments, list root dir of current root device. */
  if (! *arg || *arg == ' ' || *arg == '\t')
  {
	return dir ("/");
  }
  else if (substring(arg,"dev",1) == 0)
  {
  	buf_drive = -1;
	sprintf((char *)COMPLETION_BUF,"(");
	return print_completions(1,0);
  }

  return dir (arg);
}

static struct builtin builtin_ls =
{
  "ls",
  ls_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "ls [FILE_OR_DIR]",
  "List file or directory."
};


/* makeactive */
static int makeactive_func (char *arg, int flags);
static int
makeactive_func (char *arg, int flags)
{
  int status = 0;
  int part = 0;

  errnum = 0;
  if (grub_memcmp (arg, "--status", 8) == 0)
    {
      status = 1;
      arg = skip_to (0, arg);
    }

  /* Get the drive and the partition.  */
  if (! *arg || *arg == ' ' || *arg == '\t')
    {
	current_drive = saved_drive;
	current_partition = saved_partition;
    }
  else
    {
      if (! set_device (arg))
	return 0;
    }

  /* The partition must be a primary partition.  */
  if ((part = (current_partition >> 16)) > 3
      /*|| (current_partition & 0xFFFF) != 0xFFFF*/)
    {
      errnum = ERR_DEV_VALUES;
      return 0;
    }

  /* Read the MBR in the scratch space.  */
  if (! rawread (current_drive, 0, 0, SECTOR_SIZE, (unsigned long long)(grub_size_t)mbr, 0xedde0d90))
	return 0;

  /* If the partition is an extended partition, setting the active
     flag violates the specification by IBM.  */
  if (IS_PC_SLICE_TYPE_EXTENDED (PC_SLICE_TYPE (mbr, part)))
    {
	errnum = ERR_DEV_VALUES;
	return 0;
    }

  if (status)
    {
	int active = (PC_SLICE_FLAG (mbr, part) == PC_SLICE_FLAG_BOOTABLE);
	printf_debug0 ("Partition (%cd%d,%d) is %sactive.\n",
				((current_drive & 0x80) ? 'h' : 'f'),
				(current_drive & ~0x80),
				part,
				(active ? "" : "not "));
		
	return active;
    }

  /* Check if the active flag is disabled.  */
  if (PC_SLICE_FLAG (mbr, part) != PC_SLICE_FLAG_BOOTABLE)
    {
	/* Clear all the active flags in this table.  */
	{
	  int j;
	  for (j = 0; j < 4; j++)
	    PC_SLICE_FLAG (mbr, j) = 0;
	}

	/* Set the flag.  */
	PC_SLICE_FLAG (mbr, part) = PC_SLICE_FLAG_BOOTABLE;

	/* Write back the MBR.  */
	if (! rawwrite (current_drive, 0, (unsigned long long)(grub_size_t)mbr))
	    return 0;

	printf_debug0 ("Partition (%cd%d,%d) successfully set active.\n",
				((current_drive & 0x80) ? 'h' : 'f'),
				(current_drive & ~0x80),
				part);
    }
  else
    {
	/* Check if the other 3 entries already cleared. if not, clear them. */
	unsigned int flags_changed = 0;
	{
	  int j;
	  for (j = 0; j < 4; j++)
	  {
	    if (j == part)
		continue;
	    if (PC_SLICE_FLAG (mbr, j))
	    {
		PC_SLICE_FLAG (mbr, j) = 0;
		flags_changed++;
	    }
	  }
	}

	printf_debug0 ("Partition (%cd%d,%d) was already active.\n",
				((current_drive & 0x80) ? 'h' : 'f'),
				(current_drive & ~0x80),
				part);

	if (flags_changed)
	{
		/* Write back the MBR.  */
		if (! rawwrite (current_drive, 0, (unsigned long long)(grub_size_t)mbr))
		    return 0;

		printf_debug0 ("Deactivated %d Partition(s) successfully.\n", flags_changed);
	}
    }

  return 1;
}

static struct builtin builtin_makeactive =
{
  "makeactive",
  makeactive_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "makeactive [--status] [PART]",
  "Activate the partition PART. PART defaults to the current root device."
  " This command is limited to _primary_ PC partitions on a hard disk."
};


//static unsigned long long start_sector, sector_count;
//unsigned long long initrd_start_sector;

  /* Get the start sector number of the file.  */
static void disk_read_start_sector_func (unsigned long long sector, unsigned int offset, unsigned long long length);
static void
disk_read_start_sector_func (unsigned long long sector, unsigned int offset, unsigned long long length)
{
      if (sector_count < 1)
	{
	  start_sector = sector;
	}
      sector_count++;
}

static void print_bios_total_drives(void);
static void
print_bios_total_drives(void)
{	
	grub_printf ("\nfloppies_curr=%d, harddrives_curr=%d, cdrom_curr=%d\n", 
			floppies_orig, harddrives_orig, cdrom_orig);
}

 
unsigned int probed_total_sectors;
unsigned int probed_total_sectors_round = 0x3f * 0xff * 0x3ff;
unsigned int probed_sector_size;
unsigned int probed_heads;								//探测磁头数					在probe_bpb, probe_mbr赋值.
unsigned int probed_sectors_per_track;		//探测每磁道扇区数		在probe_bpb, probe_mbr赋值.
unsigned int probed_cylinders;						//探测柱面数					在probe_bpb, probe_mbr赋值.
unsigned int sectors_per_cylinder;				//每柱面扇区数				在probe_bpb, probe_mbr赋值.
/*  
 * return:
 * 		0		success
 *		otherwise	failure
 *			1	no "55 AA" at the end
 *
 */
int probe_bpb (struct master_and_dos_boot_sector *BS);
int
probe_bpb (struct master_and_dos_boot_sector *BS)
{
  unsigned int i,j; 
  /* first, check ext2 grldr boot sector */
 	if (*(unsigned short *)((char *)BS)  == 0x2EEB										//"jmp + 0x30"
		&& (*(int*)((char *)BS + 0x420) == 0x8000 && *(unsigned short *)((char *)BS + 0x438) == 0xEF53))
		{
	
  /* at 0D: (byte)Sectors per block. Valid values are 2, 4, 8, 16 and 32. */
	i = 2 << *(unsigned int *)((char *)BS + 0x418);
	if (i < 2 || 32 % i) 
	goto failed_ext2_grldr;

  /* at 0E: (word)Bytes per block.
   * Valid values are 0x400, 0x800, 0x1000, 0x2000 and 0x4000.
   */
  j = i << 9;
	i = *(unsigned int *)((char *)BS + 0x414) + 1;	
		if (j == 0x400 && i != 2)
	goto failed_ext2_grldr;

	if (j != 0x400 && i != 1)
	goto failed_ext2_grldr;

  /* at 14: Pointers per block(number of blocks covered by an indirect block).
   * Valid values are 0x100, 0x200, 0x400, 0x800, 0x1000.
   */
	i = j >> 2;
  if (i < 0x100 || 0x1000 % i)
	goto failed_ext2_grldr;
  
  /* at 10: Pointers in pointers-per-block blocks, that is, number of
   *        blocks covered by a double-indirect block.
   * Valid values are 0x10000, 0x40000, 0x100000, 0x400000 and 0x1000000.
   */

  filesystem_type = 5;
	probed_sector_size = 512;
  /* BPB probe success */
  return 0;
	}
	
failed_ext2_grldr:

	/* Second, check exfat grldr boot sector */
	if (*(unsigned long long *)((char *)BS + 03) != 0x2020205441465845)	//'EXFAT   '
		goto failed_exfat_grldr;
	if (*(unsigned char *)((char *)BS + 0x6e) != 1)
		goto failed_exfat_grldr;
		
	probed_sector_size = *(unsigned short *)((char *)BS + 0x0b);
	probed_total_sectors = *(unsigned long long *)((char *)BS + 0x48);
	
	filesystem_type = 6;
	return 0;

failed_exfat_grldr:

	if (*(unsigned short *)((char *)BS + 0x16) == 0x6412)							//'12 64'
	{
		probed_sector_size = *(unsigned short *)((char *)BS + 0x0b);
		probed_total_sectors = *(unsigned int *)((char *)BS + 0x20);
		filesystem_type = 7;																						//fat12-64
		return 0;
	}
	
	if (*(unsigned int *)((char *)BS + 0x1b4) == 0x46424246)					//'FBBF'
	{
		probed_sector_size = 512;
		filesystem_type = 8;		
		return 0;
	}		

	/* Second, check FAT12/16/32/NTFS grldr boot sector */
  probed_total_sectors = BS->total_sectors_short ? BS->total_sectors_short : (BS->total_sectors_long ? BS->total_sectors_long : (unsigned int)BS->total_sectors_long_long);
  if (! BS->sectors_per_cluster || 128 % BS->sectors_per_cluster)
	return 1;
  if (BS->number_of_fats > (unsigned char)2 /* BS->number_of_fats && ((BS->number_of_fats - 1) >> 1) */)
	return 1;
  if (BS->media_descriptor < (unsigned char)0xF0)
	return 1;
  if (! BS->root_dir_entries && ! BS->total_sectors_short && ! BS->sectors_per_fat) /* FAT32 or NTFS */
	if (BS->number_of_fats && BS->total_sectors_long && BS->sectors_per_fat32)
	{
		filesystem_type = 3;
	}
	else if (! BS->number_of_fats && ! BS->total_sectors_long && ! BS->reserved_sectors && BS->total_sectors_long_long)
	{
		filesystem_type = 4;
	} else
		return 1;	/* unknown NTFS-style BPB */
  else if (BS->number_of_fats && BS->sectors_per_fat) /* FAT12 or FAT16 */
	if ((probed_total_sectors - BS->reserved_sectors - BS->number_of_fats * BS->sectors_per_fat - (BS->root_dir_entries * 32 + BS->bytes_per_sector - 1) / BS->bytes_per_sector) / BS->sectors_per_cluster < 0x0ff8 )
	{
		filesystem_type = 1;
	} else {
		filesystem_type = 2;
	}
  else
	return 1;	/* unknown BPB */
  
  probed_sector_size = *(unsigned short *)((char *)BS + 0x0b);

  /* BPB probe success */
  return 0;
	
}


/* on call:
 * 		BS		points to the bootsector
 * 		start_sector1	is the start_sector of the bootimage in the
 *				real disk, if unsure, set it to 0
 * 		sector_count1	is the sector_count of the bootimage in the
 *				real disk, if unsure, set it to 1
 * 		part_start1	is the part_start of the partition in which
 *				the bootimage resides, if unsure, set it to 0
 *  
 * on return:
 * 		0		success
 *		otherwise	failure
 *
 */
int probe_mbr (struct master_and_dos_boot_sector *BS, unsigned int start_sector1, unsigned int sector_count1, unsigned int part_start1);
int
probe_mbr (struct master_and_dos_boot_sector *BS, unsigned int start_sector1, unsigned int sector_count1, unsigned int part_start1)
{
  unsigned long i, j;
  unsigned long non_empty_entries;
  unsigned long ret_val;
  unsigned long active_partitions = 0;
  unsigned long long L[8];
  unsigned long X;
  unsigned long Y;

  /* check boot indicator (0x80 or 0) */
  non_empty_entries = 0; /* count non-empty entries */
  for (i = 0; i < 4; i++)
  {
    int *part_entry;
    /* the boot indicator must be 0x80 (bootable) or 0 (non-bootable) */
    if ((unsigned char)(BS->P[i].boot_indicator << 1))/* if neither 0x80 nor 0 */
    {
      printf_debug ("Error: invalid boot indicator(0x%X) for entry %d.\n", (unsigned char)(BS->P[i].boot_indicator), i);
      ret_val = 2;
      goto err_print_hex;
    }
    if ((unsigned char)(BS->P[i].boot_indicator) == 0x80)
      active_partitions++;
    if (active_partitions > 1)
    {
      printf_debug ("Error: duplicate active flag at entry %d.\n", i);
      ret_val = 3;
      goto err_print_hex;
    }
    /* check if the entry is empty, i.e., all the 16 bytes are 0 */
    part_entry = (int *)&(BS->P[i].boot_indicator);
    if (*part_entry++ || *part_entry++ || *part_entry++ || *part_entry)
    {
      non_empty_entries++;
      /* valid partitions never start at 0, because this is where the MBR
      * lives; and more, the number of total sectors should be non-zero.
      */
      if (! BS->P[i].start_lba)
      {
        printf_warning ("Warning: partition %d should not start at sector 0(the MBR sector).\n", i);
        ret_val = 4;
        goto err_print_hex;
      }
      if (! BS->P[i].total_sectors)
      {
        printf_debug ("Error: number of total sectors in partition %d should not be 0.\n", i);
        ret_val = 5;
        goto err_print_hex;
      }
      /* the partitions should not overlap each other */
      for (j = 0; j < i; j++)
      {
        if ((BS->P[j].start_lba <= BS->P[i].start_lba) && (BS->P[j].start_lba + BS->P[j].total_sectors >= BS->P[i].start_lba + BS->P[i].total_sectors))
          continue;
        if ((BS->P[j].start_lba >= BS->P[i].start_lba) && (BS->P[j].start_lba + BS->P[j].total_sectors <= BS->P[i].start_lba + BS->P[i].total_sectors))
          continue;
        if ((BS->P[j].start_lba < BS->P[i].start_lba) ?
            (BS->P[i].start_lba - BS->P[j].start_lba < BS->P[j].total_sectors) :
            (BS->P[j].start_lba - BS->P[i].start_lba < BS->P[i].total_sectors))
        {
          printf_debug ("Error: overlapped partitions %d and %d.\n", j, i);
          ret_val = 6;
          goto err_print_hex;
        }
      }
      /* the starting cylinder number */
      X = ((BS->P[i].start_sector_cylinder) & 0x3f);
      /* the sector number should not be 0. */
      ///* partitions should not start at the first track, the MBR-track */
      if (! X /* || BS->P[i].start_lba < Smax */)
      {
        printf_debug ("Error: starting S of entry %d should not be 0.\n", i);
        ret_val = 7;
        goto err_print_hex;
      }
      Y = ((BS->P[i].end_sector_cylinder) & 0x3f);
      if (! Y)
      {
        printf_debug ("Error: ending S of entry %d should not be 0.\n", i);
        ret_val = 8;
        goto err_print_hex;
      }
      if (BS->P[i].total_sectors != 0xffffffff)
        L[i+4] = BS->P[i].start_lba + BS->P[i].total_sectors;
      else
        L[i+4] = 0xffffffff;
      if (start_sector1 == part_start1 && L[i+4] != 0xffffffff)/* extended partition is pretending to be a whole drive */
        L[i+4] +=(unsigned long) part_start1;
      if (L[i+4] < Y)
      {
        printf_debug ("Error: partition %d ended too near.\n", i);
        ret_val = 9;
        goto err_print_hex;
      }
      if (L[i+4] > 0x100000000ULL)
      {
        printf_debug ("Error: partition %d ended too far.\n", i);
        ret_val = 10;
        goto err_print_hex;
      }
    }
  }	/* end for */
  if (non_empty_entries == 0)
  {
    printf_debug ("Error: partition table is empty.\n");
    ret_val = 11;
    goto err_print_hex;
  }
  return 0;

err_print_hex:
  /* print the partition table in Hex */
  if (debug > 1)
    for (i = 0; i < 4; i++)
    {
      printf ("%02X, %02X %02X %02X   %02X, %02X %02X %02X   %08X   %08X\n"
          , (unsigned char)(BS->P[i].boot_indicator)
          , BS->P[i].start_head
          , (unsigned char)(BS->P[i].start_sector_cylinder)
          , (unsigned char)(BS->P[i].start_sector_cylinder >> 8)
          , (unsigned char)(BS->P[i].system_indicator)
          , BS->P[i].end_head
          , (unsigned char)(BS->P[i].end_sector_cylinder)
          , (unsigned char)(BS->P[i].end_sector_cylinder >> 8)
          , BS->P[i].start_lba
          , BS->P[i].total_sectors);
    }

  return ret_val;
}

static struct fragment_map_slot *fragment_map_slot_empty(struct fragment_map_slot *q);
static struct fragment_map_slot *
fragment_map_slot_empty(struct fragment_map_slot *q)  //查找碎片空槽			返回=0/非0=没有空槽/空槽位置
{
	int n = FRAGMENT_MAP_SLOT_SIZE;
  while (n > 0)
  {
    if (!q->slot_len)
      return q;
    n -= q->slot_len;
//    q += q->slot_len;
    q = (struct fragment_map_slot *)((char *)q + q->slot_len);
  }
  return 0;
}

struct fragment_map_slot *fragment_map_slot_find(struct fragment_map_slot *q, unsigned int from);
struct fragment_map_slot *
fragment_map_slot_find(struct fragment_map_slot *q, unsigned int from) //在碎片插槽中查找包含from驱动器的插槽    返回=0/非0=没有找到/插槽位置
{
  int n = FRAGMENT_MAP_SLOT_SIZE;

  while (n > 0)
  {
    if (!q->slot_len)
      return 0;
    if (q->from == (unsigned char)from)
      return q;
    n -= q->slot_len;
//    q += q->slot_len;
    q = (struct fragment_map_slot *)((char *)q + q->slot_len);
  }
  return 0;
}

void unload_fragment_slot (unsigned int from);
void
unload_fragment_slot (unsigned int from)  //卸载碎片插槽
{
  struct fragment_map_slot *q;
  char *filename;
  void *start;
  int len;
  
  q = (struct fragment_map_slot *)&disk_fragment_map;     //q=碎片映射插槽起始位置 *q=插槽尺寸 	b6e0
  filename = (char *)q + FRAGMENT_MAP_SLOT_SIZE;          //碎片映射插槽终止位置
  q = fragment_map_slot_find(q, from);		                //q=from驱动器在碎片映射插槽起始位置
  if (q)  		//0/1=没有找到驱动器/找到驱动器位置
  {
    start = filename - q->slot_len;
    len = q->slot_len;
    grub_memmove (q, (char *)q + q->slot_len, filename - (char *)q - q->slot_len);
    grub_memset (start, 0, len);
  }
}

int GetSectorSequence (char* Utf8Name, struct fragment_map_slot** SectorSeq, int exist);
int
GetSectorSequence (char* Utf8Name, struct fragment_map_slot** SectorSeq, int exist)  //获得父VHD文件的扇区序列
{
	if (exist)
	{
		errnum = 0;
		goto aaa;
	}
  int i = no_decompression;
  query_block_entries = -1;           //仅请求块列表*/
  no_decompression = 1;
  blocklist_func (Utf8Name, 1);	//请求块列表   执行成功后,将设置query_block_entries=1,设置errnum=0
  no_decompression = i;
aaa:
  if (errnum || query_block_entries > DRIVE_MAP_FRAGMENT || query_block_entries <= 0)
    return 0;
  *SectorSeq = grub_zalloc (((blklst_num_entries + 1) << 4) + 4); 
  struct fragment_map_slot* q = *SectorSeq;

  struct fragment *p = (struct fragment *)&q->fragment_data;
  if (!p)
    return 0;
  for (i = 0; i < blklst_num_entries; i++)
  {
    p[i].start_sector = map_start_sector[i];
    p[i].sector_count = map_num_sectors[i];   
  }
  p[i].start_sector = 0;
  p[i].sector_count = 0;

  return blklst_num_entries;
}

char *vhd_file_name = 0;
char vhd_file_path [128];
int GetParentUtf8Name (char *dest, grub_uint16_t *src);
int
GetParentUtf8Name (char *dest, grub_uint16_t *src)  //获得utf8格式的父VHD文件名
{
  char *d = chainloader_file_orig;
  grub_uint16_t *s = src;
  int count = 0;

  while (*s)
  {
    *d++ = (*s++ >> 8);
    count++;
  }
  *d = 0;
  
  d = chainloader_file_orig + count;
  while (*d-- != '\\') ;
  d += 2;

  grub_sprintf (dest, "%s%s", vhd_file_path, d);
  count = grub_strlen (dest);

  return count;
}

void add_part_data (int drive);
void
add_part_data (int drive)
{
	struct grub_part_data *p;	//efi分区数据
	struct grub_part_data *dp;	//efi分区数据
	unsigned int back_saved_drive = saved_drive;
	unsigned int back_current_drive	=	current_drive;
	unsigned int back_saved_partition	=	saved_partition;
	unsigned int back_current_partition	=	current_partition;
 
  //查找分区数据结束
  dp = partition_info;
  while (dp->next)
    dp = dp->next;
  
  if (drive < 0x90)
  {
    unsigned int part = 0xFFFFFF;
    unsigned long long start, len, offset;
    unsigned int type, entry1, ext_offset1;
    saved_drive = current_drive = drive;
    saved_partition = current_partition = part;
    while ((	next_partition_drive = drive,   //虽然大部分参数可以由vdisk_install获得，但是map之后，vdisk_install之前需要参数时还的在此获取。
				next_partition_dest = 0xFFFFFF,
				next_partition_partition = &part,
				next_partition_type = &type,
				next_partition_start = &start,
				next_partition_len = &len,
				next_partition_offset = &offset,
				next_partition_entry = &entry1,
				next_partition_ext_offset	= &ext_offset1,
				next_partition_buf = mbr,
				next_partition ()))
    {
      if (*next_partition_type == 0 || *next_partition_type == 5 || *next_partition_type == 0xf)
        continue;
      
      p = grub_zalloc (sizeof (*p));  //分配内存	
      if(! p)  //如果分配内存失败
        return;

      //填充设备结构
      p->drive = drive;
      p->partition = *next_partition_partition;
      p->partition_type = *next_partition_type;
      p->partition_start = *next_partition_start;
      p->partition_size = *next_partition_len;
      p->partition_offset = *next_partition_offset;
      p->partition_entry = *next_partition_entry;
      p->partition_ext_offset = *next_partition_ext_offset;
      p->partition_activity_flag = partition_activity_flag;
      grub_memcpy (&p->partition_signature, &partition_signature, 16);
      p->next = 0;
      if (!partition_info)
      {
        partition_info = p;
        dp = p;
      }
      else
      {
      dp->next = p;
      dp = dp->next;
      }
    }
    if (get_efi_device_boot_path (drive, 1))
    {
      part_data = get_partition_info (drive, part_data->partition);
      part_data->partition_boot = 1;
    }

    saved_drive = back_saved_drive;
    current_drive = back_current_drive;
    saved_partition = back_saved_partition;
    current_partition = back_current_partition;
  }
  else
  {
    p = grub_zalloc (sizeof (*p));  //分配内存	
    if(! p)  //如果分配内存失败
      return;
    get_efi_device_boot_path(drive, 1);
    p->drive = drive;
    p->partition = 0xffff;
    p->partition_start = cd_Image_part_start;
    p->partition_size = cd_Image_disk_size;
    p->boot_start = cd_boot_start;
    p->boot_size = cd_boot_size;
    p->partition_boot = 1;
    p->next = 0;
    if (!partition_info) //使用 'if (!dp)' 会判断错误。 当 partition_info=bp=0 时，if (!partition_info) 返回1；而 if (!dp) 返回0。
      partition_info = p;
    else
      dp->next = p;
  }
}

unsigned long long tmp;
unsigned long long vhd_start_sector;
/* map */
/* Map FROM_DRIVE to TO_DRIVE.  映射 FROM 驱动器到 TO 驱动器*/
int map_func (char *arg, int flags);
int
map_func (char *arg, int flags)  //对设备进行映射		返回: 0/1=失败/成功
{
  char *to_drive;
  char *from_drive;
  unsigned int to, from,primeval_to;
  int k, l, m;
  char *filename;
  char *p;
  struct fragment_map_slot *q;
//  unsigned int extended_part_start;
//  unsigned int extended_part_length;
  int err;
  int prefer_top = 0;
  int no_hook = 0;
  int vhd_disk = 0;
  vhd_start_sector = 0;

  //struct master_and_dos_boot_sector *BS = (struct master_and_dos_boot_sector *) RAW_ADDR (0x8000);
#define	BS	((struct master_and_dos_boot_sector *)mbr)

  unsigned long long mem = -1ULL;					//0=加载到内存   -1=不加载到内存
  int read_only = 0;					            //只读					若read_Only=1,则同时unsafe_boot=1
  unsigned char	from_log2_sector = 9;
  unsigned char	partmap_type = 0;
  unsigned char disk_signature[16];
//  unsigned long long sectors_per_track = -1ULL;
//  unsigned long long heads_per_cylinder = -1ULL;
//  int add_mbt = -1;
  /* prefer_top now means "enable blocks above address of 4GB".         prefere_top 意思是“启用高于4GB地址的块”。
   * By default, prefer_top = 0, meaning that only 32-bit addressable   默认情况下，prefere_top=0，这意味着指定的虚拟内存驱动器只允许32位可寻址内存。
   * memory is allowed for the specified virtual mem-drive. 
						 -- tinybit 2017-01-24 */
  unsigned long long skip_sectors = 0;
//  unsigned long long max_sectors = -1ULL;
  filesystem_type = -1;
  start_sector = sector_count = 0;
  blklst_num_entries = 0;
	grub_efi_status_t status;				//状态
	grub_efi_boot_services_t *b;		//引导服务
	b = grub_efi_system_table->boot_services;	//系统表->引导服务
	char *cache = 0;
	struct grub_disk_data *df, *dt=0, *de;
  //处理入口参数
  errnum = 0;
  
  for (;;)
  {
		if (grub_memcmp (arg, "--status", 8) == 0)		//1. 状况 按扇区显示
		{
			int byte = 0;
//      unsigned long long tmp;
			arg += 8;
			if (grub_memcmp (arg, "-byte", 5) == 0)			//按字节显示  --status-byte
				byte = 1;
			arg = skip_to(1,arg); //标记=0:  跳过"空格,回车,换行,水平制表符"; 标记=1:  跳过"空格,回车,换行,水平制表符,等号"
//			if (*arg>='0' && *arg <='9')		//如果参数在0-9之间
			if ((unsigned char)*arg>='0' /*&& *arg <='9'*/)  //--status=
			{
				if (!safe_parse_maxint(&arg,&mem))				//分析十进制或十六进制ASCII输人字符,转换到64位整数
					return 0; //分析错误
#if 0
				for (i = 0; i < DRIVE_MAP_SIZE && !(drive_map_slot_empty (disk_drive_map[i])); i++)
				{
					if (disk_drive_map[i].from_drive != (unsigned char)mem)  //如果from驱动器号不等于输入参数,继续
						continue;
//					*(unsigned int *)ADDR_RET_STR = (unsigned int)disk_drive_map[i].start_sector;
          sprintf(ADDR_RET_STR,"0x%lx",(unsigned int)disk_drive_map[i].start_sector);
					return disk_drive_map[i].sector_count;  //返回起始扇区(32位)
				}
#else
        df = get_device_by_drive ((unsigned int)mem,1);
        if (df)
        {
          sprintf(ADDR_RET_STR,"0x%lx",df->start_sector);
					return df->sector_count;  //返回起始扇区(32位)
        }
#endif
				return 0;  //没有查到from驱动器号
			}

			print_bios_total_drives();	//打印软盘数,硬盘数
			if (rd_base != -1ULL)		//如果rd基地址被赋值
				grub_printf ("\nram_drive=0x%X, rd_base=0x%lX, rd_size=0x%lX\n", ram_drive, rd_base, rd_size); 
#if 0
			if (drive_map_slot_empty (disk_drive_map[0]))   //判断驱动器映像插槽是否为空   为空,返回1
			{
				grub_printf ("\nThe drive map table is currently empty.\n");
				return 1; //bios_drive_map插槽为空
			}
/*
svbus插槽
typedef struct _GRUB4DOS_DRIVE_MAP_SLOT
{
	UCHAR from_drive;         //1字节
	UCHAR to_drive;           //1字节 0xFF indicates a memdrive
	UCHAR max_head;           //1字节
	UCHAR max_sector:6;       //位0-5            未使用
	UCHAR disable_lba:1;	    //位6: disable lba 未使用
	UCHAR read_only:1;		    //位7: read only   未使用
	USHORT to_cylinder:13;    //位0-12: max cylinder of the TO drive   未使用
	USHORT from_cdrom:1;	    //位13: FROM drive is CDROM(with big 2048-byte sector)
	USHORT to_cdrom:1;		    //位14:  TO  drive is CDROM(with big 2048-byte sector)   未使用
	USHORT to_support_lba:1;  //位15:  TO  drive support LBA           未使用
	UCHAR to_head;			      //1字节 max head of the TO drive         未使用
	UCHAR to_sector:6;		    //位0-5: max sector of the TO drive
	UCHAR fake_write:1;		    //位6: fake-write or safe-boot           未使用
	UCHAR in_situ:1;		      //位7: in-situ                           未使用
	ULONGLONG start_sector;   //8字节
	ULONGLONG sector_count;   //8字节
}GRUB4DOS_DRIVE_MAP_SLOT,*PGRUB4DOS_DRIVE_MAP_SLOT;
*/      
/*
现在：
struct drive_map_slot
{
	Remember to update DRIVE_MAP_SLOT_SIZE once this is modified.
	The struct size must be a multiple of 4.
	unsigned char from_drive;
	unsigned char to_drive;						0xFF indicates a memdrive
	unsigned char max_head;             //旧svbus使用，否则蓝屏。支持碎片的新版本不需要。

	unsigned char :7;
	unsigned char read_only:1;          //位7

	unsigned short to_log2_sector:4;    //位0-3
	unsigned short from_log2_sector:4;  //位4-7
	unsigned short :2;
	unsigned short fragment:1;          //位10
	unsigned short :2;
	unsigned short from_cdrom:1;        //位13
	unsigned short to_cdrom:1;          //位14
	unsigned short :1;
  
	unsigned char to_head;
	unsigned char to_sector;            //旧svbus使用，否则蓝屏。支持碎片的新版本不需要。
	unsigned long long start_sector;
	unsigned long long sector_count;
};
*/

//过去：
	  /* X=max_sector bit 7: read only or fake write */		//只读或假写		read_Only			| fake_write
	  /* Y=to_sector  bit 6: safe boot or fake write */		//安全或假写		!unsafe_boot	| fake_write
	  /* ------------------------------------------- */
	  /* X Y: meaning of restrictions imposed on map */		//约束映射的意义.不能只看X或Y,应当组合来看.
	  /* ------------------------------------------- */
	  /* 1 1: read only=0, fake write=1, safe boot=0 */		//假写
	  /* 1 0: read only=1, fake write=0, safe boot=0 */		//只读
	  /* 0 1: read only=0, fake write=0, safe boot=1 */		//安全
	  /* 0 0: read only=0, fake write=0, safe boot=0 */		//未知

//	unsigned char from_drive;																						//0   from驱动器			映射驱动器
//	unsigned char to_drive;		/* 0xFF indicates a memdrive */						//1   to驱动器    		宿主驱动器			0xff是内存驱动器
//	unsigned char max_head;																							//2   from最大磁头号 	来源: BPB探测磁道数-1  如果chs无效,让MAX_HEAD非0,避免空槽
//	unsigned char max_sector;	/* bit 7: read-only or fake-write*/				//3   from最大扇区号	位7:只读 or 假写    位6:禁止lba
					/* bit 6: disable lba */																			//									  来源: 禁止chs模式=0; 在原处=1; 其他=每磁道扇区数; 																																		
//																																													      如果FROM是cdrom但TO不是cdrom,则=大于1的任意数,表示仿真. 
																																				//前4字节为零, 就是空插槽!!!
//	unsigned short to_cylinder;	/* max cylinder of the TO drive */			//4,5 to最大柱面号		位15:to支持lba  位14:to驱动器是cdrom  位13:from驱动器是cdrom
																																				//									  位12:to是分叉		位11:to引导扇区类型已知 位10:有碎片
																																				//									  位9-0:最大柱面号 0-0x3ff; to是分叉时,0x80代表真实分叉	 	
					/* bit 15:  TO  drive support LBA */													//									  来源: 由几何探测计数
					/* bit 14:  TO  drive is CDROM(with big 2048-byte sector) */  //									  如果in_situ!=0,则to_c含义: 位8=in_situ_flags,位0-7=分区类型
					/* bit 13:	FROM drive is CDROM(with big 2048-byte sector) */ //                    位7-4:from_log2_sector    位3-0:to_log2_sector
					/* bit 12:  TO  drive is BIFURCATE */
					/* bit 11:  TO  drive has a known boot sector type */
					/* bit 10:  TO  drive has Fragment */

//	unsigned char to_head;		/* max head of the TO drive */						//6   to最大磁头号		来源: 几何探测磁头数-1
//	unsigned char to_sector;	/* max sector of the TO drive */					//7   to扇区数  			位7:在原处  位6:假写 or 安全
					/* bit 7: in-situ */																					//									来源: 几何探测每磁道扇区数
					/* bit 6: fake-write or safe-boot */

//	unsigned long start_sector;																					//8   to起始扇区低		来源: 块列表.映射起始扇区  										0表示整体映射
//	unsigned long start_sector_hi;	/* hi dword of the 64-bit value */	//12  to起始扇区高
//	unsigned long sector_count;																					//16  to扇区计数低		来源: grub_open.文件最大/0x200 或者 分区长度	0或1表示整体映射
//	unsigned long sector_count_hi;	/* hi dword of the 64-bit value */	//20  to扇区计数高																									加载到内存时=1

//};

//			TO:		200					800					1000
//--------------------------------------------------------
//FORM:
//200					不变				/4...				/8...
//800					*4					不变				/2...
//1000				*8					*2					不变


	/* From To MaxHead MaxSector ToMaxCylinder ToMaxHead ToMaxSector StartLBA_lo StartLBA_hi Sector_count_lo Sector_count_hi Hook Type */
			grub_printf ("\nFd Td Fs Ts Fg Ro   Start_Sector     Sector_Count"
		       "\n-- -- -- -- ---- -- -- -- ---------------- ----------------\n");
			for (i = 0; i < DRIVE_MAP_SIZE; i++)
	    {
				if (drive_map_slot_empty (disk_drive_map[i]))   //判断驱动器映像插槽是否为空   为空,返回1
					break;
//        if (disk_drive_map[i].to_drive == 0xff)
//          tmp = disk_drive_map[i].start_sector;
//        else
          tmp = disk_drive_map[i].start_sector;
				grub_printf ("%02X %02X %02X %02X %02X %02X %016lX %016lX\n", disk_drive_map[i].from_drive, disk_drive_map[i].to_drive,
						disk_drive_map[i].from_log2_sector, disk_drive_map[i].to_log2_sector, disk_drive_map[i].fragment, disk_drive_map[i].read_only,
						byte?(tmp << disk_drive_map[i].to_log2_sector):tmp,
            byte?((unsigned long long)disk_drive_map[i].sector_count)
						<< disk_drive_map[i].to_log2_sector:(unsigned long long)disk_drive_map[i].sector_count);
	    }
#else
			grub_printf ("\nFd Td Fs Ts Fg Ro   Start_Sector     Sector_Count"
		       "\n-- -- -- -- ---- -- -- -- ---------------- ----------------\n");
      for (df = disk_data; df; df = df->next) 	//从设备结构起始查
      {
        if(df->sector_count)
        {
          grub_printf ("%02X %02X %02X %02X %02X %02X %016lX %016lX\n", df->drive, df->to_drive,
              df->from_log2_sector, df->to_log2_sector, df->fragment, df->read_only,
              byte?(df->start_sector << df->to_log2_sector):df->start_sector,
              byte?((unsigned long long)df->sector_count)
              << df->to_log2_sector:(unsigned long long)df->sector_count);
        }
      }
#endif
			return 1;
		}
    else if (grub_memcmp (arg, "--hook", 6) == 0)
    {
//      map_to_svbus(grub4dos_self_address); //为svbus复制插槽  测试使用
      buf_drive = -1;
      buf_track = -1;
      return 1;
    }
    else if (grub_memcmp (arg, "--no-hook", 9) == 0)
    {
      no_hook = 1;
    }
    else if (grub_memcmp (arg, "--unhook", 8) == 0)
    {
      buf_drive = -1;
      buf_track = -1;
      return 1;
    }
    else if (grub_memcmp (arg, "--unmap=", 8) == 0)
    {
#if UNMAP
      int drive;
      p = arg + 8;
      for (drive = 0xFF; drive >= 0; drive--)    //从0xff到0
      {
        if (drive != INITRD_DRIVE && in_range (p, drive))	//如果驱动器不是0x22及0x23,并且排列存在
        {
          df = get_device_by_drive (drive,0);
          if (df->drive == drive && df->sector_count) //是映射驱动器
          {
            //卸载碎片插槽
            if (df && df->fragment == 1)  //有碎片
            {
              unload_fragment_slot (drive);
#if 0
              q = (struct fragment_map_slot *)&disk_fragment_map;     //q=碎片映射插槽起始位置 *q=插槽尺寸 	b6e0
              filename = (char *)q + FRAGMENT_MAP_SLOT_SIZE;          //碎片映射插槽终止位置
              q = fragment_map_slot_find(q, drive);		                //q=from驱动器在碎片映射插槽起始位置
              if (q)  		//0/1=没有找到驱动器/找到驱动器位置
              {
                void *start = filename - q->slot_len;
                int len = q->slot_len;
                grub_memmove (q, (char *)q + q->slot_len, filename - (char *)q - q->slot_len);
                grub_memset (start, 0, len);
              }
#endif
            }
          }
          //卸载映射磁盘,卸载映射分区
          uninstall (drive, df);
        } 
      }
#endif
      buf_drive = -1;
      buf_track = -1;
      return 1;
    }
    else if (grub_memcmp (arg, "--rehook", 8) == 0)
    {
      buf_drive = -1;
      buf_track = -1;
      return 1;
    }
    else if (grub_memcmp (arg, "--floppies=", 11) == 0)
    {
      return 1;
    }
    else if (grub_memcmp (arg, "--harddrives=", 13) == 0)
    {
      return 1;
    }
    else if (grub_memcmp (arg, "--ram-drive=", 12) == 0)		//8. 内存盘  设置rd驱动器号,默认0x7f,设置区间:0-0xfe
		{
//			unsigned long long tmp;
			p = arg + 12;
			if (! safe_parse_maxint (&p, &tmp))
				return 0;
			if (tmp > 254)
				return ! (errnum = ERR_INVALID_RAM_DRIVE);
			ram_drive = tmp;
	
			return 1;
		}
    else if (grub_memcmp (arg, "--rd-base=", 10) == 0)   //9. rd基址
		{
//			unsigned long long tmp;
			p = arg + 10;
			if (! safe_parse_maxint_with_suffix (&p, &tmp, 9))
				return 0;
			rd_base = tmp;

			return 1;
		}
    else if (grub_memcmp (arg, "--rd-size=", 10) == 0)  //10. rd尺寸
		{
//			unsigned long long tmp;
			p = arg + 10;
			if (! safe_parse_maxint_with_suffix (&p, &tmp, 9))
				return 0;
			rd_size = tmp;
			return 1;
		}
    else if (grub_memcmp (arg, "--e820cycles=", 13) == 0)
    {
      return 1;
    }
    else if (grub_memcmp (arg, "--int15nolow=", 13) == 0)
    {
      return 1;
    }
    else if (grub_memcmp (arg, "--memdisk-raw=", 14) == 0)
    {
      return 1;
    }
    else if (grub_memcmp (arg, "--a20-keep-on=", 14) == 0)
    {
      return 1;
    }
    else if (grub_memcmp (arg, "--safe-mbr-hook=", 16) == 0)
    {
      return 1;
    }
    else if (grub_memcmp (arg, "--int13-scheme=", 15) == 0)
    {
      return 1;
    }
    else if (grub_memcmp (arg, "--mem-max=", 10) == 0)
    {
      return 1;
    }
    else if (grub_memcmp (arg, "--mem-min=", 10) == 0)
    {
      return 1;
    }
    else if (grub_memcmp (arg, "--mem=", 6) == 0)		    //19. 指定mem的位置(扇区数))
		{
			p = arg + 6;
			if (! safe_parse_maxint_with_suffix (&p, &mem, 9))
				return 0;
		}
		else if (grub_memcmp (arg, "--mem", 5) == 0)		    //20. 使用内存映射
		{
			mem = 0;		//0=加载到内存   -1=不加载到内存
		}
    else if (grub_memcmp (arg, "--top", 5) == 0)		//21. 内存映射置顶
		{
    //有人反映：UEFI固件不会自动分配内存到4GB以上，必须强制加载到4GB以上内存。
      prefer_top = 1;
      mem = 0;
		}
    else if (grub_memcmp (arg, "--read-only", 11) == 0)
    {
      read_only = 1;
    }
    else if (grub_memcmp (arg, "--fake-write", 12) == 0)
    {
    }
    else if (grub_memcmp (arg, "--fake-write", 12) == 0)
    {
    }
    else if (grub_memcmp (arg, "--disable-chs-mode", 18) == 0)
    {
    }
    else if (grub_memcmp (arg, "--disable-lba-mode", 18) == 0)
    {
    }
    else if (grub_memcmp (arg, "--in-place=", 11) == 0)
    {
    }
    else if (grub_memcmp (arg, "--in-situ=", 10) == 0)
    {
    }
    else if (grub_memcmp (arg, "--in-place", 10) == 0)
    {
    }
    else if (grub_memcmp (arg, "--in-situ", 9) == 0)
    {
    }
    else if (grub_memcmp (arg, "--heads=", 8) == 0)
    {
    }
    else if (grub_memcmp (arg, "--sectors-per-track=", 20) == 0)
    {
    }
		else if (grub_memcmp (arg, "--add-mbt=", 10) == 0)  //33. 增加存储块  -1,0,1
		{
		}
    else if (grub_memcmp (arg, "--skip-sectors=", 15) == 0)		//34. 跳过扇区
		{
			p = arg + 15;
			if (! safe_parse_maxint_with_suffix (&p, &skip_sectors, 9))
				return 0;
		}
    else if (grub_memcmp (arg, "--max-sectors=", 14) == 0)
    {
    }
    else if (grub_memcmp (arg, "--swap-drive=", 13) == 0)
    {
      return 1; 
    }
    else
			break;
    arg = skip_to (0, arg); //跳到空格后
  }		//入口参数处理完毕
  
  to_drive = arg;                 //to驱动器地址
  from_drive = skip_to (0, arg);	//from驱动器地址
  /* Get the drive number for FROM_DRIVE.  */
  set_device (from_drive);  //设置from驱动器的当前驱动器号  返回: 0/非0=失败/成功

  if (errnum)
    return 0;
  from = current_drive;     //from=当前驱动器
  
  if (from >= 0xa0)
  {
    grub_memset(chainloader_file_orig, 0, 256);
    set_full_path(chainloader_file_orig,to_drive,sizeof(chainloader_file_orig));
    skip_to (SKIP_WITH_TERMINATE,chainloader_file_orig);
  }
  
  de = get_device_by_drive (from, 1); //de非0，则存在重名驱动器
  if (de && de->fragment) //如果存在碎片插槽，删除它
    unload_fragment_slot (from);

  /* Get the drive number for TO_DRIVE.  */
  filename = set_device (to_drive); //设置to驱动器的当前驱动器号
  if (errnum) //如果错误
	{
		/* No device specified. Default to the root device. */
		current_drive = saved_drive;	//则当前驱动器=根驱动器
		filename = 0;									//驱动器号=0
		errnum = 0;										//错误号=0
  }
  
  to = current_drive;						  //to=当前驱动器
	primeval_to = to;               //保存原始to
  /* if mem device is used, assume the --mem option  如果使用mem驱动器,假设--mem已选择*/
  if (to == 0xffff || to == ram_drive || from == ram_drive || to == 0x21)		//如果to=md,或to=rd,或from=rd,或网络驱动器
  {
		if (mem == -1ULL)		//如果mem=-1ULL=0xffffffffffffffff   不加载到内存
			mem = 0;					//则为零  修改为加载到内存   to=md,或to=rd,或from=rd,则一定是mem=0,即一定加载到内存
  }

  if ((current_partition == 0xFFFFFF || (to >= 0x80 && to <= 0xFF)) && filename && (*filename == 0x20 || *filename == 0x09))
	{
    return 0;
#if 0
		if (to == 0xffff /* || to == ram_drive */)
		{
			if (((long long)mem) <= 0)
			{
				return ! (errnum = ERR_MD_BASE);
			}
			start_sector = (unsigned long long)mem;		//起始扇区=mem
			sector_count = 1;													//扇区计数=1  to=md
		}
		else if (to == ram_drive)
		{
			/* always consider this to be a fixed memory mapping   总是认为to是固定内存映像*/
			if ((rd_base & 0x1ff) || ! rd_base)		//如果(rd_base & 0x1ff)不为零,或者rd_base为零
				return ! (errnum = ERR_RD_BASE);		//则返回错误
			to = 0xffff;													//to=md
			mem = (rd_base >> 9);				          //mem=rd_base/0x200
			start_sector = (unsigned long long)mem;	//起始扇区=mem
			sector_count = 1;												//扇区计数=1  to=rd
		}
		else
		{
        /* when whole drive is mapped, the mem option should not be specified. 				//当映射整体驱动器时,mem选项应当没有指定.
         * but when we delete a drive map slot, the mem option means force.						//但是当我们删除驱动器映像插槽时,mem选项意味着强制.
         */
//			if (mem != -1ULL && to != from)		      //如果加载到内存,并且to不等于from       //mem=0/-1=加载到内存/不加载到内存
//				return ! (errnum = ERR_SPECIFY_MEM);	//则返回错误  不应该指定内存
//			sectors_per_track = 1;/* 1 means the specified geometry will be ignored. */	  //每磁道扇区数=1,意味着指定几何探测将被忽略。
//			heads_per_cylinder = 1;/* can be any value but ignored since #sectors==1. */	//每柱面磁头数=1,可以是任何值，但被忽略了因为每磁道扇区数=1。
        /* Note: if the user do want to specify geometry for whole drive map, then
         * use a command like this:	                            //注意: 如果用户不希望指定几何探测整个驱动器映射,则使用命令行: 使每磁道扇区数>1
         * 
         * map --heads=H --sectors-per-track=S (hd0)+1 (hd1)
         * 
         * where S > 1
         */
//			goto map_whole_drive; //转到映射整体驱动器
      return 0; //不允许执行类似的操作: map (4) (4);  map (0x80) (0x81);
		}
#endif
	}

	//保存to驱动器的路径文件名
	if (*(to_drive) == '/' || *(to_drive) == '(')
	{
		grub_memmove (chainloader_file, to_drive, grub_strlen (to_drive) + 1);
    p = skip_to (0, chainloader_file);
    *(p - 1) = 0;

		vhd_file_name = grub_zalloc(256);
    grub_memmove (vhd_file_name, chainloader_file, grub_strlen (chainloader_file) + 1);
    //获得并保存to驱动器的路径
    p--;
    while (*p-- != '/') ;
    *(p + 2) = 0;

		grub_memmove (vhd_file_path, chainloader_file, grub_strlen (chainloader_file) + 1);
	}

//  if (mem == -1ULL)		//如果不加载到内存
  {
    //判断是否连续(填充碎片信息)
    query_block_entries = -1; /* query block list only   仅请求块列表*/
		k = no_decompression;
		no_decompression = 1;
    blocklist_func (to_drive, flags);	//请求块列表   执行成功后,将设置query_block_entries=1,设置errnum=0
		no_decompression = k;
    if (errnum)
      return 0;

		vhd_start_sector = map_start_sector[0];
		if ((compressed_file && decomp_type != DECOMP_TYPE_VHD) || query_block_entries > DRIVE_MAP_FRAGMENT) //如果是压缩文件且不是VHD, 或者碎片太多
    {
			printf_warning ("Too many fragments or compressed file needs to be loaded into memory.");
    }
#if 0
		start_sector = map_start_sector[0];    
      //此处将扇区计数，更改为按每扇区0x200字节计的小扇区!!!
//    sector_count = (filemax + 0x1ff) >> SECTOR_BITS; /* in small 512-byte sectors */
    sector_count = (filemax + 0x1ff) >> buf_geom.log2_sector_size;
		//此处的 start_sector 是相对扇区数，不含分区起始扇区。且只在to=0xffff时使用
		//此处的 sector_count 后面没有使用这个参数
    if (start_sector == part_start && part_start && sector_count == 1)		//如果起始扇区=分区起始,并且分区起始不为零,并且扇区计数=1
			sector_count = part_length;																			    //则扇区计数=分区长度
#endif
  }	
	cache = grub_zalloc (0x800);	//分配缓存
  if (!cache)
    return 0;
	
#if 0
  if ((to == 0xffff /* || to == ram_drive */) && sector_count == 1)		//如果to=md,并且扇区计数=1
  {
    /* fixed memory mapping   安装内存映射*/
		grub_memmove64 ((unsigned long long)(grub_size_t) cache, (start_sector << 9), 0x800);
    grub_memmove64 ((unsigned long long)(grub_size_t) BS, (start_sector << 9), SECTOR_SIZE);
  }
	else
#endif
	{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    {
      if (! grub_open (to_drive))	//打开to驱动器
        goto  fail_free;
    }
//		if ((skip_sectors << SECTOR_BITS) > filemax)
    if (skip_sectors > (filemax >> 9))		//如果跳过扇区>文件最大扇区
		{
			errnum = ERR_EXEC_FORMAT;
      goto fail_free;
		}

    /* disk_read_start_sector_func() will set start_sector and sector_count */
    //disk_read_start_sector_func()函数将设置开始扇区和扇区计数
    start_sector = sector_count = 0;			//开始扇区=扇区计数=0
    rawread_ignore_memmove_overflow = 1;	//读忽视内存移动溢出=1
    
		//disk_read_start_sector_func将挂钩在grub_read。当运行grub_read时，同时运行disk_read_start_sector_func。
		//disk_read_start_sector_func设置start_sector和sector_count。
		//此处的start_sector是绝对扇区数，含分区起始扇区。是首次设置!!!
		//此处的sector_count没有用，即不是碎片数，也不是文件扇区数。
    disk_read_hook = disk_read_start_sector_func;	//磁盘读挂钩=磁盘读开始扇区功能
    filepos = (skip_sectors << 9);				        //文件位置指针=跳过扇区*0x200  skip_sectors一般为零
    /* Read the first sector of the emulated disk.  */
    //读to驱动器分区的首扇区到 cache
		unsigned long long a = filepos;
		grub_read ((unsigned long long)(grub_size_t) cache, 0x800, 0xedde0d90);
		filepos = a;
    err = grub_read ((unsigned long long)(grub_size_t) BS, SECTOR_SIZE, 0xedde0d90);
    disk_read_hook = 0;										//磁盘读挂钩=0
    rawread_ignore_memmove_overflow = 0;	//读忽视内存移动溢出=0

    if (err != SECTOR_SIZE && from != ram_drive)	//如果错误号不是0x200,并且from不是rd
    {
//      grub_close ();															//则关闭
      /* This happens, if the file size is less than 512 bytes.   如果文件尺寸小于512字节,将发生*/
      if (errnum == ERR_NONE)			//如果错误号是0
				errnum = ERR_EXEC_FORMAT;	//则设置错误号为ram_drive
      goto fail_close_free;
    }

    //此处重新设置了 sector_count, 将扇区计数更改为按每扇区0x200字节计的小扇区!!!
//    sector_count = (filemax + 0x1ff) >> 9; /* in small 512-byte sectors */
    sector_count = (filemax + buf_geom.sector_size - 1) >> buf_geom.log2_sector_size;
		if (!start_sector)
			start_sector = vhd_start_sector;
#if 0
    if (part_length		//如果分区长度不为零		此处buf_geom是to驱动器的参数
				&& (buf_geom.sector_size == 2048 ? (start_sector - (skip_sectors >> 2)) : (start_sector - skip_sectors)) == part_start //并且(扇区起始-跳过扇区)=分区起始
				/* && part_start */
				&& (buf_geom.sector_size == 2048 ? (sector_count == 4) : (sector_count == 1)))  //并且扇区计数=1(cd是4)
    { //则修正 sector_count
      // Fixed issue 107 by doing it early before part_length changed.
      sector_count = (buf_geom.sector_size == 2048 ? (part_length << 2) : part_length);  //扇区计数=分区长度
      if (mem != -1ULL)	  //如果加载到内存
      {
				char buf[32];

				grub_close ();		//关闭to驱动器, 然后重新打开修正后的to驱动器
        grub_sprintf (buf, "(%d)%ld+%ld", to, (unsigned long long)part_start, (unsigned long long)part_length);
        
//如果分区长度不为零,并且扇区起始=分区起始, 并且扇区计数=1, 并且加载到内存.   是整体映射的内存盘
//打开宿主to驱动器上的映射驱动器 (to)part_start+part_length	,获得分区起始,扇区尺寸等信息	
        if (! grub_open (buf))	// This changed part_length, causing issue 107.
          goto fail_free; 
        filepos = (skip_sectors + 1) << 9;			//文件位置指针=(跳过扇区+1)*0x200
      }// else if (part_start)
    }
#endif
    //此处又修改sector_count
    sector_count -= skip_sectors;  //扇区计数=扇区计数-跳过扇区
//    if (mem == -1ULL)		//如果不加载到内存
//      grub_close ();		//关闭to驱动器
#if 0
    if (to == 0xffff && sector_count == 1)		//如果to=md,并且扇区计数=1
    {
      grub_printf ("For mem file in emulation, you should not specify sector_count to 1.\n");  //为了仿真内存文件,你应当不指定扇区计数为1.
      errnum = ERR_BAD_ARGUMENT;
      goto fail_free;
    }
#endif
//    if (sector_count > max_sectors)		//如果扇区计数>最大扇区
//			sector_count = max_sectors;     //则扇区计数=最大扇区
	}
  
  //如果(文件最大>=跳过扇区*0x200,并且(文件最大-跳过扇区*0x200)<512),或者引导签名不是0xAA55
//  if ((filemax >= (skip_sectors << 9 ) && filemax - (skip_sectors << 9) < 512) || BS->boot_signature != 0xAA55)
//		goto geometry_probe_failed;  //转到几何探测失败
#if 0    
  /* probe the BPB */
//首先探测to驱动器的BPB  
  if (probe_bpb(BS))        //如果没有bpb
		goto failed_probe_BPB;  //转到探测bpb失败
  
  if (debug > 1 && ! disable_map_info)		//如果调试,并且disable_map_info=0, 根据 filesystem_type,打印FAT12等信息
    grub_printf ("%s BPB found %s 0xEB (jmp) leading the boot sector.\n", 
		filesystem_type == 1 ? "FAT12" :
		filesystem_type == 2 ? "FAT16" :
		filesystem_type == 3 ? "FAT32" :
		filesystem_type == 4 ? "NTFS"  :
		filesystem_type == 6 ? "exFAT" :
		/*filesystem_type == 5 ?*/ "EXT2 GRLDR",
		(BS->dummy1[0] == (char)0xEB ? "with" : "but WITHOUT"));

	//如果不加载到内存,并且from是硬盘,并且to是硬盘,并且(起始扇区=分区起始,同时分区起始不为0,同时扇区计数=分区长度)
  if (mem == -1ULL && (from & 0x80) && (to & 0x80) && (start_sector == part_start && part_start && sector_count == part_length)/* && BS->hidden_sectors >= probed_sectors_per_track */)
  {
    BS->hidden_sectors = (unsigned int)part_start;                          //隐藏扇区=分区起始
		extended_part_start = BS->hidden_sectors - probed_sectors_per_track;    //扩展分区起始=隐藏起始-探测每磁道扇区数
		extended_part_length = probed_total_sectors + probed_sectors_per_track; //扩展分区长度=探测总扇区+探测每磁道扇区数
		
		if (debug > 1 && ! disable_map_info)
			grub_printf ("Try to locate extended partition (hd%d)%d+%d for the virtual (hd%d).\n", (to & 0x7f), extended_part_start, extended_part_length, (from & 0x7f));

//建立form驱动器的映射:  (hdx)扩展分区起始+扩展分区长度
		grub_sprintf ((char *)BS, "(hd%d)%d+%d", (to & 0x7f), extended_part_start, extended_part_length);
//打开宿主to驱动器上的映射驱动器 (to)extended_part_start+extended_part_length	,获得分区起始,扇区尺寸等信息
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		if (! grub_open ((char *)BS))   //打开form驱动器
      goto fail_free;

//读form驱动器的首扇区 
		/* Read the first sector of the emulated disk.  */
		if (grub_read ((unsigned long long)(grub_size_t) BS, SECTOR_SIZE, 0xedde0d90) != SECTOR_SIZE)
		{
			grub_close ();		//关闭form驱动器

			/* This happens, if the file size is less than 512 bytes.   当文件尺寸小于512字节发生*/
			if (errnum == ERR_NONE)
				errnum = ERR_EXEC_FORMAT;
      goto fail_free;
		}
		grub_close ();      //关闭form驱动器
		for (i = 0; i < 4; i++)		//向后
		{
			unsigned char sys = BS->P[i].system_indicator;		//系统类型指示
			if (sys == 0x05 || sys == 0x0f || sys == 0)				//如果是扩展分区
			{
        long long *a = (long long *)&(BS->P[i].boot_indicator);
        *a = 0;                                         //引导指示=0 (0x1be)  结束磁道,柱面,扇区=0  起始扇区=0
//				*(long long *)&(BS->P[i].boot_indicator) = 0LL;
				//*(long long *)&(BS->P[i].start_lba) = 0LL;
				((long long *)&(BS->P[i]))[1] = 0LL;            //起始扇区=0 扇区总数=0
			}
		}
		part_start = extended_part_start;			//分区起始=扩展分区起始 
		part_length = extended_part_length;		//分区长度=扩展分区长度
		start_sector = extended_part_start;		//扇区起始=扩展分区起始
		sector_count = extended_part_length;	//扇区计数=扩展分区长度

		/* when emulating a hard disk using a logical partition, the geometry should not be specified.   仿真硬盘使用逻辑分区时,几何不应当指定*/
//		if ((long long)heads_per_cylinder > 0 || (long long)sectors_per_track > 0)		//如果每柱面磁头>0,或者每磁道扇区>0
//		{
//			errnum = ERR_SPECIFY_GEOM;																				          //则返回错误
//      goto fail_free;
//		}

		goto failed_probe_BPB;		//转到探测bpb失败
  }
	goto geometry_probe_ok;     //转到几何探测成功

//探测bpb失败后,再探测to驱动器的mbr
failed_probe_BPB:
  /* probe the partition table */
  
  if (probe_mbr (BS, start_sector, sector_count, (unsigned int)part_start))       //如果没有分区表
		goto geometry_probe_failed; //转到几何探测失败
  
  goto geometry_probe_ok;       //转到几何探测成功

//几何探测失败
geometry_probe_failed:
	goto map_whole_drive;		//转到映射整体驱动器
  
//几何探测成功  
geometry_probe_ok:

	if (mem != -1ULL && ((long long)mem) <= 0)  //如果加载到内存,并且mem<=0
	{
    //如果-mem<探测总扇区,并且探测总扇区>1,并且扇区计数>=1
		if (((unsigned long long)(-mem)) < probed_total_sectors && probed_total_sectors > 1 && sector_count >= 1/* filemax >= 512 */)
			mem = - (unsigned long long)probed_total_sectors;		//-mem=探测总扇区
	}

//映射整体驱动器
map_whole_drive:
#endif
  if (from != ram_drive)		//如果from不等于rd
  {
#if 0
    /* Search for an empty slot in disk_drive_map.  在磁盘驱动器映射中搜索空插槽*/
    for (i = 0; i < DRIVE_MAP_SIZE; i++)
    {
      /* Perhaps the user wants to override the map.  也许用户想要覆盖映射*/
      //如果 from_drive = from,即 disk_drive_map 存在2个相同的from, 则把前1个作为空插槽,覆盖他, 也就是删除前一个from
      if ((disk_drive_map[i].from_drive == from))
			{
				if (disk_drive_map[i].fragment == 1)  //有碎片
				{
					q = (struct fragment_map_slot *)&disk_fragment_map;   //碎片插槽起始
					filename = (char *)q + FRAGMENT_MAP_SLOT_SIZE;			  //碎片插槽结束
					q = fragment_map_slot_find(q, from);		              //从碎片插槽查找from驱动器
					if (q)  		//q=0/非0=没有找到/from驱动器在碎片插槽位置
					{
						void *start = filename - q->slot_len;
						int len = q->slot_len;
						grub_memmove (q, (char *)q + q->slot_len,filename - (char *)q - q->slot_len);
						grub_memset (start, 0, len);
					}
				}
				break;
			}
      
      if (drive_map_slot_empty (disk_drive_map[i]))   //如果 disk_drive_map 插槽空,即查询完毕
				break;
		}

    if (i == DRIVE_MAP_SIZE)	//如果到末尾
    {
      if (mem != -1ULL) 			//如果加载到内存
        grub_close ();        //关闭to驱动器
      errnum = ERR_WONT_FIT;
      goto fail_free;
    }
#endif
    df = get_device_by_drive (from,1);
    if (df && df->fragment == 1)  //有碎片
    {
      q = (struct fragment_map_slot *)&disk_fragment_map;   //碎片插槽起始
      filename = (char *)q + FRAGMENT_MAP_SLOT_SIZE;			  //碎片插槽结束
      q = fragment_map_slot_find(q, from);		              //从碎片插槽查找from驱动器
      if (q)  		//q=0/非0=没有找到/from驱动器在碎片插槽位置
      {
        void *start = filename - q->slot_len;
        int len = q->slot_len;
        grub_memmove (q, (char *)q + q->slot_len,filename - (char *)q - q->slot_len);
        grub_memset (start, 0, len);
      }
    }
#if 0
    /* If TO == FROM and whole drive is mapped, and, no map options occur, then delete the entry.  */
    //如果TO=FROM,并且是整个驱动器映射，并且没有映射选项出现，然后删除该条目。
    if (to == from && read_only == 0 && start_sector == 0 && (sector_count == 0 ||
	/* sector_count == 1 if the user uses a special method to map a whole drive, e.g., map (hd1)+1 (hd0) */
			(sector_count == 1)))
			{
				grub_free (cache);
        cache = 0;
		goto delete_drive_map_slot;  //删除驱动器映像插槽
			}
#endif
	}

//检查to是否存在父映射   在 disk_drive_map 查找from，是否等于当前to
  /* check whether TO is being mapped */
	if (mem == -1ULL)  //如果不加载到内存
	{
    //查找父插槽
#if 0
		for (j = 0; j < DRIVE_MAP_SIZE; j++)
		{
			if (to != disk_drive_map[j].from_drive/* || (to == 0xFF && (disk_drive_map[j].to_log2_sector == 11))*/)		//如果to != hooked.from, 或者to是cdrom
				continue;															  //继续

      //to改变!!!!
			to = disk_drive_map[j].to_drive;					//设置当前to等于父插槽的to
			if (to == 0xFF && !(disk_drive_map[j].to_log2_sector == 11))		//如果to=0xFF,并且to不是cdrom
			{
				to = 0xFFFF;		/* memory device   内存驱动器*/
			}
      
			//当to=md,或to=rd时,设置扇区计数=1
			//每磁道扇区数=1,意味着指定几何探测将被忽略,是整体映射
			//整体驱动器映射条件(asm.S): FROM和TO都是光盘 || (起始扇区=0 && 扇区计数<=1 && 每磁道扇区数<=1)
			//整体驱动器映射条件(builtins.c): 起始扇区=0 && (扇区计数=0 || (扇区计数=1 && 磁头数<=0 && 每磁道扇区数<=1))
			//驱动器仿真条件: (起始扇区!=0) || (to扇区计数>1) || (每磁道扇区数>1)

			{
#if 0
        //如果是整体映射
//				if (start_sector == 0 && (sector_count == 0 || (sector_count == 1 && (long long)heads_per_cylinder <= 0 && (long long)sectors_per_track <= 1)))
        if (start_sector == 0 && (sector_count == 0 || sector_count == 1))
				{
					sector_count = disk_drive_map[j].sector_count;							//扇区计数=父扇区计数
				}
#endif
        //起始扇区=起始扇区+父起始扇区
        start_sector = (start_sector << disk_drive_map[j].from_log2_sector) >> disk_drive_map[j].to_log2_sector; 
				start_sector += disk_drive_map[j].start_sector;
        sector_count = (sector_count << disk_drive_map[j].from_log2_sector) >> disk_drive_map[j].to_log2_sector;
        //如果to有碎片，调整各碎片起始扇区。起始扇区由blocklist_func获得。
				for (k = 0; (k < DRIVE_MAP_FRAGMENT) && (map_start_sector[k] != 0); k++)
        {
					map_start_sector[k] = (map_start_sector[k] << disk_drive_map[j].from_log2_sector) >> disk_drive_map[j].to_log2_sector;
					map_start_sector[k] += disk_drive_map[j].start_sector;
					map_num_sectors[k] = (map_num_sectors[k] << disk_drive_map[j].from_log2_sector) >> disk_drive_map[j].to_log2_sector;
        }
			}
#if 0
			/* If TO == FROM and whole drive is mapped, and, no map options occur, then delete the entry.  */
      //如果TO=FROM,并且是整个驱动器映射，并且没有映射选项出现，则删除该条目
			if (to == from && read_only == 0 && start_sector == 0 && (sector_count == 0 ||
			/* sector_count == 1 if the user uses a special method to map a whole drive, e.g., map (hd1)+1 (hd0) */
			sector_count == 1))
			{
			/* yes, delete the FROM drive(with slot[i]), not the TO drive(with slot[j]) */
      //是的，删除FROM驱动器(插槽[i])，而不是TO驱动(插槽[j])
				if (from != ram_drive)		      //如果from不是rd
					goto delete_drive_map_slot;   //删除驱动器映像插槽
			}
#endif
			break;
		}
#endif
    dt = get_device_by_drive (to,1);
    if (dt)
    {
      //to改变!!!!
      to = dt->to_drive;					//设置当前to等于父插槽的to
      if (to == 0xFF && !(dt->to_log2_sector == 11))		//如果to=0xFF,并且to不是cdrom
        to = 0xFFFF;		/* memory device   内存驱动器*/
      //确定起时扇区
      if (! dt->fragment)
      {
        //起始扇区=起始扇区+父起始扇区
        start_sector = (start_sector << dt->from_log2_sector) >> dt->to_log2_sector; 
        start_sector += dt->start_sector;
        //扇区计数
        sector_count = (sector_count << dt->from_log2_sector) >> dt->to_log2_sector;
      }
      //如果to有碎片，调整各碎片起始扇区。起始扇区由blocklist_func获得。
      else
      {
        for (k = 0; (k < DRIVE_MAP_FRAGMENT) && (map_start_sector[k] != 0); k++)
        {
          map_start_sector[k] = (map_start_sector[k] << dt->from_log2_sector) >> dt->to_log2_sector;
          map_start_sector[k] += dt->start_sector;
          map_num_sectors[k] = (map_num_sectors[k] << dt->from_log2_sector) >> dt->to_log2_sector;
        }
      }
    }
	}
//至此,start_sector与sector_count最终确定!!!!
//j=from驱动器的父插槽号  也就是说,to不是原生磁盘,是映射盘  dt是from的父驱动器设备
//i=from驱动器的插槽号                                      df是from驱动器的设备
//====================================================================================================================
  if (from != ram_drive && from != 0xffff)
  {
  //获取to驱动器,虚拟分区信息
//  disk_drive_map[i].start_sector = start_sector;
//  disk_drive_map[i].sector_count = sector_count;

	if (from >= 0xa0)	//光盘
	{
		from_log2_sector = 11;
	}
	else if (from >= 0x80)	//硬盘
	{
		struct master_and_dos_boot_sector *mbr1 = (struct master_and_dos_boot_sector *)cache;
		grub_efi_uint32_t active = -1;

		for (k = 0; k < 4; k++)	//判断分区类型 mbr/gpt
		{
			if(mbr1->P[k].system_indicator == GRUB_PC_PARTITION_TYPE_GPT_DISK)			//如果系统标识是EFI分区
				goto get_gpt_info;
      partmap_type = 1; //MBR
		}
		for (k = 0; k < 4; k++)	//查找活动分区
		{
			if(mbr1->P[k].boot_indicator == ACTIVE_PARTITION)
			{
				active = k;
				break;
			}
		}
		if (active == (grub_efi_uint32_t)-1)	//如果没有活动分区,寻找第一个非0分区起始
		{
			for(k = 0; k < 4; k++)
			{
				if(mbr1->P[k].start_lba != 0)
				{
					active = k;
					break;
				}
			}
		}

		if (active == (grub_efi_uint32_t)-1)	//失败
      goto fail_close_free;
		from_log2_sector = 9;
		part_addr = mbr1->P[active].start_lba;	//分区起始扇区
		filepos = part_addr << 9;					//指向dbr 
		grub_read ((unsigned long long)(grub_size_t) cache, 0x800, 0xedde0d90);
		if (probe_bpb(mbr1))	//没有bpb
		{
			from_log2_sector = 12;
			filepos = part_addr << 12;				//指向dbr 是原生4k磁盘
			grub_read ((unsigned long long)(grub_size_t) cache, 0x800, 0xedde0d90);
			if (probe_bpb(mbr1))	//没有bpb
        goto fail_close_free;
		}
    grub_memset (&disk_signature, 0, 16);
    *(unsigned int *)disk_signature = PC_DISK_SIG (next_partition_buf);  //MBR磁盘签名
		goto get_info_ok;
		
get_gpt_info:

		partmap_type = 2; //GPT
		from_log2_sector = 9;
		P_GPT_HDR gpt = (P_GPT_HDR)cache;
		/* "EFI PART" */
		filepos = 1 << 9;					//读逻辑1扇区, 普通磁盘
		grub_read ((unsigned long long)(grub_size_t) cache, 0x800, 0xedde0d90);
		if (gpt->hdr_sig != GPT_HDR_SIG)	//如果签名不符
		{
			from_log2_sector = 12;
			filepos = 1 << 12;			//读逻辑1扇区, 是原生4k磁盘
			grub_read ((unsigned long long)(grub_size_t) cache, 0x800, 0xedde0d90);
			if (gpt->hdr_sig != GPT_HDR_SIG)	//如果签名不符
        goto fail_close_free;
		}
		grub_memmove(&disk_signature, &gpt->hdr_uuid, 16); //GPT磁盘签名
		goto get_info_ok;
	}
	else	//软盘
	{
		from_log2_sector = 9;
	}
  }
		
get_info_ok:
	grub_free (cache);
  cache = 0;

	if (compressed_file && decomp_type == DECOMP_TYPE_VHD)	//vhd动态磁盘
	{
		if (mem == -1ULL)	//不加载到内存
			vhd_disk |= 1;
		else							//加载到内存
			vhd_disk |= 2;
	}

	if (mem == -1ULL)		//如果不加载到内存
		grub_close ();		//关闭to驱动器
//====================================================================================================================  
  /* how much memory should we use for the drive emulation? */
  if (mem != -1ULL)		  //如果加载到内存
	{
		unsigned long long start_byte;		//起始字节
		unsigned long long bytes_needed;	//需要字节
    bytes_needed = 0ULL;	//初始化: 需要字节=基地址=顶端=0
    grub_efi_physical_address_t alloc; //分配				动态地址,变化

		if (start_sector == part_start && part_start == 0 && sector_count == 1)		//如果起始扇区=分区起始,并且分区起始=0,并且扇区计数=1
			sector_count = part_length;  //扇区计数=分区长度
//    if (buf_geom.log2_sector_size != 9)
//      sector_count = (sector_count << buf_geom.log2_sector_size) >> 9;
      /* For GZIP disk image if uncompressed size >= 4GB,  								//如果压缩gzip磁盘图像尺寸>=4GB
         high bits of filemax is wrong, sector_count is also wrong. */		//filemax的高位是错误的，扇区计数也是错的
		if ( (long long)mem < 0LL && sector_count < (-mem) )		//如果mem<0,并且扇区计数<(-mem)
//      bytes_needed = (-mem) << SECTOR_BITS;									//需要字节=(-mem)*0x200
			bytes_needed = (-mem) << buf_geom.log2_sector_size;
		else
//      bytes_needed = sector_count << SECTOR_BITS;           //否则,需要字节=扇区计数*0x200
			bytes_needed = sector_count << buf_geom.log2_sector_size;
      /* filesystem_type
       *	 0		an MBR device
       *	 1		FAT12
       *	 2		FAT16
       *	 3		FAT32
       *	 4		NTFS
       *	-1		unknown filesystem(do not care)
       *	
       * Note: An MBR device is a whole disk image that has a partition table.
       */

//		if (add_mbt<0)		//如果=-1,没有输入参数
//			add_mbt = (filesystem_type > 0 && (from & 0x80) && (from < 0x9F))? 1: 0; /* known filesystem without partition table */
      //增加分配块=1/0=(filesystem_type>0,并且from是硬盘/否则    无分区表的已知文件系统

//		if (add_mbt)			//如果增加分配块=1, 需要字节+每磁道扇区数*0x200
//			bytes_needed += sectors_per_track << SECTOR_BITS;	/* build the Master Boot Track */

		bytes_needed = ((bytes_needed+4095)&(-4096ULL));	/* 4KB alignment   4k对齐*/

//		if ((to == 0xffff || to == ram_drive) && sector_count == 1)			//如果(to=md,或者rd),并且扇区计数=1
//			bytes_needed = 0;		//需要字节=0

//		start_byte = start_sector << SECTOR_BITS;		//起始字节=起始扇区*0x200
    start_byte = start_sector << buf_geom.log2_sector_size;		//起始字节
		if (to == ram_drive)		  //如果to=rd
			start_byte += rd_base;  //起始字节+rd基址
/////////////////////////////////////////////////////////////////////////////////////////////////////以下插入分配内存

    if (to == 0x21) //网络驱动器
    {
//      disk_drive_map[i].start_sector = ((unsigned long long)(grub_size_t)(char*)efi_pxe_buf | 0x200) & 0xfffffffffffffe00;  //此处是内存起始字节!!!
      start_sector = ((unsigned long long)(grub_size_t)(char*)efi_pxe_buf | 0x200) & 0xfffffffffffffe00;  //此处是内存起始字节!!!
      efi_pxe_buf = 0;
    }
    else  //其他
    {
      if (prefer_top) //分配4GB以上内存
      {
        //在此借用一下 blklst_num_sectors 全局变量
        blklst_num_sectors = (grub_efi_uintn_t)bytes_needed >> 12;  //需求页数
        displaymem_func ((char *)"-mem", 1);  //探测4GB以上内存
        if (blklst_num_sectors != (grub_efi_uintn_t)bytes_needed >> 12) //有满足条件的内存
        {
          status = efi_call_4 (b->allocate_pages, GRUB_EFI_ALLOCATE_ADDRESS,  
//              GRUB_EFI_RUNTIME_SERVICES_DATA, 
              GRUB_EFI_RESERVED_MEMORY_TYPE,          //保留内存类型        0
              (grub_efi_uintn_t)bytes_needed >> 12, &blklst_num_sectors);	//调用(分配页面,分配类型->指定页面,存储类型->运行时服务数据(6),分配页,地址)
          if (status == GRUB_EFI_SUCCESS)
          {
            alloc = blklst_num_sectors;
            goto mem_ok;
          }
        }
      }
/*
通过g4e或者grub2作为一个UEFI引导器，加载镜像到内存盘，并且做好了与svbus驱动的对接，然后启动windows,进入桌面。
当内存类型为GRUB_EFI_RUNTIME_SERVICES_DATA时,有的系统可以成功启动,而有的windows系统报错误0xc0000225和0xc0000017.
“c0000225”或“c0000017”错误发生在您试图在运行Windows 7或Windows Server 2008 R2的启用了UEFI的计算机上启动Windows PE RAM磁盘映像时.
看微软的意思是“内存状态不同步，一个内存管理器使用的一些内存仍然被另一个内存管理器标记为可用.
改用内存类型GRUB_EFI_RESERVED_MEMORY_TYPE,完美解决.
*/
      status = efi_call_4 (b->allocate_pages, GRUB_EFI_ALLOCATE_ANY_PAGES,  
//				  GRUB_EFI_RUNTIME_SERVICES_DATA, 
          GRUB_EFI_RESERVED_MEMORY_TYPE,          //保留内存类型        0
			      (grub_efi_uintn_t)bytes_needed >> 12, &alloc);	//调用(分配页面,分配类型->任意页面,存储类型->运行时服务数据(6),分配页,地址)
      
      if (status != GRUB_EFI_SUCCESS)	//如果失败
      {
        printf_errinfo ("out of map memory: %x\n",status);
        return 0;
      }
    }

/////////////////////////////////////////////////////////////////////////////////////////////////////以上插入分配内存
mem_ok:
		sector_count = bytes_needed >> 9;			//扇区计数=加载到内存的扇区数，每扇区0x200字节
//    sector_count = bytes_needed >> buf_geom.log2_sector_size;			//扇区计数=需要扇区 
		//向内存移动映像 第一扇区已经读到了BS
	  /* if image is in memory and not compressed, we can simply move it. */
	  if ((to == 0xffff || to == ram_drive) && !compressed_file) //如果映像在内存中，并且没有压缩，我们可以简单地移动它。
		{
//			if (bytes_needed != start_byte)	//如果需要字节!=起始字节
        grub_memmove64 (alloc, start_byte, filemax);
		}
		else	//如果映像不在内存中，或者被压缩
		{
	    unsigned long long read_result;	//读结果
	    unsigned long long read_size = bytes_needed;        //读尺寸
      //修正读尺寸
			if (read_size > filemax - (skip_sectors << 9))
				read_size = filemax - (skip_sectors << 9);
			filepos = skip_sectors << 9;
			unsigned long long asdf = part_start;
			if (vhd_disk & 2)
				part_start = vhd_start_sector;
			read_result = grub_read (alloc, read_size, 0xedde0d90);	//读结果=返回读尺寸
			part_start = asdf;

			if (read_result != read_size)	//如果读结果!=读尺寸
			{
				grub_close ();     //关闭to驱动器
				if (errnum == ERR_NONE)
					errnum = ERR_READ;
				return 0;
			}				
      blklst_num_entries = 1; //如果文件有碎片，加载到内存后就连续了。避免后续设置碎片。     
		}
	  grub_close ();        //关闭to驱动器
    start_sector = alloc >> 9;
		to = 0xFFFF/*GRUB_INVALID_DRIVE*/;
      /* if FROM is (rd), no mapping is established. but the image will be  //如果FROM是rd,没有建立映射
       * loaded into memory, and (rd) will point to it. Note that Master		//但是映像将装载到内存,并且rd指向它.
       * Boot Track and MBR code have been built as above when needed				//注意主引导磁道和MBR代码已经建立,需要ram_drive是硬盘.
       * for ram_drive > 0x80.
       */
		if (from == ram_drive)		//如果from是rd
		{
      rd_base = alloc;		//rd基址
//			rd_size = (max_sectors >= filemax) ? filemax : (sector_count << 9);
      rd_size = filemax;
//			if (add_mbt)			//增加存储块=1
//				rd_size += sectors_per_track << 9;	/* build the Master Boot Track   rd长度+每磁道扇区数*0x200*/
			return 1;
		}
	}  //if (mem != -1ULL)结束  //如果加载到内存结束
	//加载到内存结束
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
	/* if TO_DRIVE is whole floppy, skip the geometry lookup. 如果TO是软盘,跳过几何检查*/
	if (start_sector == 0 && sector_count == 0 && to < 0x04)
	{
		disk_drive_map[i].from_log2_sector = 9;
	}
#endif
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ buf_geom 是 primeval_to 的信息
  if (primeval_to != to)
    if (get_diskinfo (to, &buf_geom, 0))	//如果'获得磁盘信息'返回非0, 错误
      return !(errnum = ERR_NO_DISK);

//有碎片
//		│                 Virtual_sector             │   虚拟扇区(假设连续)
//		│                                            │
//		│	  To_count(0)			      To_count(1)				│	    To_count(2)		│			        To_count(3)             │
//		├─────────┼────────────┼──────────┼───────────────────┤	To驱动器(父)  有碎片
//To_start(0)		      To_start(1)					      To_start(2)					  To_start(3)
//		│		              								Form_len   
//	  ├--------------------├------------------------------------┤	Form驱动器(子)  可能有碎片
//	  │                Form_statr

  //Determine the start fragment 
  //不加载到内存,并且to不是光盘,并且((原始to != to,并且有碎片) 或者块列表数大于1)
//	if ((mem == -1ULL) && (to < 0x9f) && (((primeval_to != to) && (disk_drive_map[j].fragment == 1)) || (blklst_num_entries > 1)))
  if ((mem == -1ULL) && (to < 0x9f) && (((primeval_to != to) && (dt->fragment == 1)) || (blklst_num_entries > 1)))
	{
    //如果是2次映射,并且有碎片
#if 0
//		if ((primeval_to != to) && (disk_drive_map[j].fragment == 1))		//如果是2次映射,并且有碎片
    if ((primeval_to != to) && (dt->fragment == 1))		//如果是2次映射,并且有碎片
		{
      unsigned long long sum_to_count = 0;      //To计数和, 即各碎片扇区数的和, 是父驱动器的值
      unsigned long long form_statr;            //Form驱动器起始扇区 24eda
      unsigned long long form_len;              //Form驱动器扇区计数 10000
			blklst_num_entries = 0;
      
      //查找父插槽from
			q = (struct fragment_map_slot *)&disk_fragment_map;
			q = fragment_map_slot_find(q, primeval_to);
			struct fragment *f = (struct fragment *)&q->fragment_data;    //父插槽
      //查找空槽
      q = (struct fragment_map_slot *)&disk_fragment_map;
      q = fragment_map_slot_empty(q);
      struct fragment *empty_slot  = (struct fragment *)&q->fragment_data;  //空插槽
      //确定Form_statr在To的哪一个碎片
      form_statr = map_start_sector[0] - f[0].start_sector;
      form_len = map_num_sectors[0];
      for (k = 0; (k < DRIVE_MAP_FRAGMENT) && (f[k].start_sector != 0); k++)
      {
        sum_to_count += f[k].sector_count;
        if (form_statr < sum_to_count)
          break;
      }
//      empty_slot[0].start_sector = f[k].start_sector + (form_statr << (disk_drive_map[j].from_log2_sector - disk_drive_map[j].to_log2_sector)) - (sum_to_count - f[k].sector_count);
      empty_slot[0].start_sector = f[k].start_sector + (form_statr << (dt->from_log2_sector - dt->to_log2_sector)) - (sum_to_count - f[k].sector_count);
      start_sector = empty_slot[0].start_sector;
      //建立碎片映射
      q->from = from;
      q->to = to;
      for (l = 0; form_len && (k < DRIVE_MAP_FRAGMENT) && (f[k].start_sector != 0); k++, l++)
      {
        blklst_num_entries = l + 2;
        //确定起始扇区
        if (l)
          empty_slot[l].start_sector = f[k].start_sector;

        if (form_len <= f[k].sector_count - (empty_slot[l].start_sector - f[k].start_sector))
        {
          empty_slot[l].sector_count = form_len;
          form_len = 0;
        }
        else
        {
          empty_slot[l].sector_count = f[k].sector_count - (empty_slot[l].start_sector - f[k].start_sector);
          form_len -= f[k].sector_count - (empty_slot[l].start_sector - f[k].start_sector);
        }
      }
      q->slot_len = l*16 + 4;
      goto no_fragment;
    }
#endif
//		if ((primeval_to != to) && (disk_drive_map[j].fragment == 1))		//如果是2次映射,并且有碎片
    if ((primeval_to != to) && (dt->fragment == 1))		//如果是2次映射,并且有碎片
		{
      //临时缓存子程序碎片信息
      char *p1 = grub_zalloc (DRIVE_MAP_FRAGMENT * 8);
      if (!p1)
        return 0;
      char *p2 = grub_zalloc (DRIVE_MAP_FRAGMENT * 8);
      if (!p2)
        return 0;
      unsigned long long *map_start_sector_back = (unsigned long long *)p1;
      unsigned long long *map_num_sectors_back = (unsigned long long *)p2;
      grub_memmove64 ((unsigned long long)(grub_size_t)p1, (unsigned long long)(grub_size_t)map_start_sector, DRIVE_MAP_FRAGMENT * 8);
      grub_memmove64 ((unsigned long long)(grub_size_t)p2, (unsigned long long)(grub_size_t)map_start_sector, DRIVE_MAP_FRAGMENT * 8);
      //查找父插槽To_
			q = (struct fragment_map_slot *)&disk_fragment_map;
			q = fragment_map_slot_find(q, primeval_to);
			struct fragment *to_ = (struct fragment *)&q->fragment_data; 
      
      for (m = 0; *map_start_sector_back && m < DRIVE_MAP_FRAGMENT; map_start_sector_back += 8, map_num_sectors_back += 8)
      {
        unsigned long long Virtual_sector = 0;											//虚拟扇区终点(假设扇区连续)  以To_start(0)为起点，把各碎片依序逐次拼接的终端值。
        unsigned long long Form_len = *map_num_sectors_back;				//Form扇区计数
        unsigned long long Form_statr = *map_start_sector_back;			//Form起始扇区
        Virtual_sector = to_[0].start_sector;  //虚拟扇区终端，以To_start(0)为起点

        for (k = 0; (k+m < DRIVE_MAP_FRAGMENT) && (to_[k].start_sector != 0); k++)
        {
          //确定Form_statr在父驱动器的物理地址
          Virtual_sector += to_[k].sector_count;    //拼接第k段后的虚拟扇区终点
          if (Form_statr < Virtual_sector)	        //如果Form起始扇区小于虚拟扇区终点，说明Form起始扇区位于碎片段k
          {
            map_start_sector[m] = to_[k].start_sector + to_[k].sector_count - (Virtual_sector - Form_statr);  //Form_statr在父驱动器的物理地址
            start_sector = map_start_sector[m];
            break;
          }
        }
        //确定碎片尺寸(也可能只有一段)
        l = 0;
        if ((Form_len + Form_statr) <= Virtual_sector)        //如果父碎片段k可容纳Form_len
          goto set_ok;                                        //完成
        else 
        {
          //确定子碎片段0尺寸
          map_num_sectors[m] = Virtual_sector - Form_statr;	  //碎片段k剩余尺寸，作为子碎片段0的尺寸
          Form_len -= map_num_sectors[m];                     //Form扇区计数剩余值
          //确定子碎片段l尺寸
          for (l = 1; ((k+l+m < DRIVE_MAP_FRAGMENT) && (to_[k+l].start_sector != 0)); l++)
          {
            blklst_num_entries = l + 2;
            //
            if (Form_len <= to_[k+l].sector_count)        //如果父碎片段k可容纳Form_len
            {
              map_num_sectors[l+m] = Form_len;            //Form扇区计数剩余值，作为子碎片段l的尺寸
              map_start_sector[l+m] = to_[k+l].start_sector;//父碎片段k+l的起时扇区,作为子碎片段l的起时扇区
              goto set_ok;                                //完成
            }
            else                                          //如果父碎片段k不能容纳Form_len
            {
              map_num_sectors[l+m] = to_[k+l].sector_count;	//父碎片段k+l的起时扇区,作为子碎片段l的起时扇区
              map_start_sector[l+m] = to_[k+l].start_sector;//父碎片段k+l的扇区尺寸,作为子碎片段l的扇区尺寸
              Form_len -= to_[k+l].sector_count;          //Form扇区计数剩余值
            }
          }
        }
set_ok:
        m += l + 1;
      }
      map_num_sectors[m] = 0;
      map_start_sector[m] = 0;
      grub_free (p1);
      grub_free (p2);
    }

		if (blklst_num_entries < 2)
		{
//			start_sector = map_start_sector[0];
			goto no_fragment;
		}
    //查找空槽
		q = (struct fragment_map_slot *)&disk_fragment_map;
		filename = (char *)q;
		q = fragment_map_slot_empty(q);
    //出界检查
		if (((char *)q + blklst_num_entries*16 + 4 - filename) > FRAGMENT_MAP_SLOT_SIZE)
			return ! (errnum = ERR_MANY_FRAGMENTS);
    //建立碎片映射
		q->from = from;
		q->to = to;
		struct fragment *f = (struct fragment *)&q->fragment_data;
		for (k = 0; map_start_sector[k] != 0; k++)
		{
			f[k].start_sector = map_start_sector[k];
			f[k].sector_count = map_num_sectors[k];
		}

		q->slot_len = k*16 + 4;
	}
  
//无碎片
no_fragment:
#if 0
	disk_drive_map[i].from_drive = from;
  disk_drive_map[i].to_drive = (unsigned char)to; /* to_drive = 0xFF if to == 0xffff */
  disk_drive_map[i].from_log2_sector = from_log2_sector;
	disk_drive_map[i].to_log2_sector = buf_geom.log2_sector_size; //????
	disk_drive_map[i].fragment = (blklst_num_entries > 1);        //to有碎片
	disk_drive_map[i].read_only = read_only;	
  if (from_log2_sector == 0xb)
    disk_drive_map[i].from_cdrom = 1;
  if (buf_geom.log2_sector_size == 0xb)
    disk_drive_map[i].to_cdrom = 1;
  disk_drive_map[i].start_sector = start_sector;
  initrd_start_sector = start_sector;
  disk_drive_map[i].sector_count = sector_count;
  disk_drive_map[i].max_head = 0xfe;    //避免旧svbus蓝屏。支持碎片的新版本不需要。
  disk_drive_map[i].to_sector = 0x3f;   //避免旧svbus蓝屏。支持碎片的新版本不需要。
#endif
#undef	BS

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	buf_drive = -1;
	buf_track = -1;
  struct grub_disk_data	*d;
  struct grub_disk_data	*d1;
  if (de)
    d = de;
  else
    d = grub_zalloc (sizeof (*d));  //分配内存, 并清零
  
//填充磁盘数据
  d->drive = from;
  d->to_drive = (unsigned char)to;
  d->from_log2_sector = from_log2_sector;
  d->to_log2_sector = buf_geom.log2_sector_size;
  d->start_sector = start_sector;
  d->sector_count = sector_count;
  d->total_sectors = (filemax + (1 << from_log2_sector) - 1) >> from_log2_sector;
  d->to_block_size = buf_geom.sector_size;
  d->partmap_type = partmap_type;
  d->fragment = (blklst_num_entries > 1);
  d->read_only = read_only;
  d->vhd_disk = vhd_disk;
  if (from >= 0x80 && from <= 0x8f)
    grub_memmove(&d->disk_signature, &disk_signature, 16); //磁盘签名
	
	if (vhd_file_name)
	{
		grub_free (vhd_file_name);
		vhd_file_name = 0;
	}

  if (de)
    return 1;

  d->next = 0;
  if (d->drive >= 0xa0)
  {
		cdrom_orig++;
		d->disk_type = DISK_TYPE_CD;
  }
  else if (d->drive >= 0x80)
  {
		harddrives_orig++;
		d->disk_type = DISK_TYPE_HD;
  }
  else
  {
		floppies_orig++;
		d->disk_type = DISK_TYPE_FD;
  }
  d1 = disk_data;
	
  if (d1 != d)
  {
		for (; d1->next; d1 = d1->next);
		d1->next = d;
  }
  if (d->drive >= 0x80)
  {
		add_part_data (d->drive);
    if (!no_hook)
    {
      current_drive = d->drive;
      part_data = get_boot_partition (current_drive);
      if (!part_data)
        return (int)(!(errnum = ERR_NO_DISK));
      current_partition = part_data->partition;
      status = vdisk_install (current_drive, 0);  //安装虚拟磁盘
      if (status != GRUB_EFI_SUCCESS)							//如果安装失败
      {
        printf_errinfo ("Failed to install vdisk.(%x)\n",status);	//未能安装vdisk
        return 0;
      }
    }
  }

  return 1;

fail_close_free:
  grub_close ();

fail_free:
	if (cache)
		grub_free (cache);
	if (vhd_file_name)
	{
		grub_free (vhd_file_name);
		vhd_file_name = 0;
	}
  return 0;
#if 0
 //删除驱动器映像插槽  带入i=插槽位置  i=0-7
delete_drive_map_slot:
  if (mem != -1ULL)   //如果加载到内存
	  grub_close ();    //关闭to驱动器 
  return 1;
#endif
}

static struct builtin builtin_map =
{
  "map",
  map_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "map [--status[-byte]] [--mem] [--top] [--no-hook] [--unmap=DRIVES] [--ram-drive=RD]\n    [--rd-base=ADDR] [--rd-size=SIZE] [--read-only] TO_DRIVE FROM_DRIVE",
  "Map the drive FROM_DRIVE to the drive TO_DRIVE."
  "\nTO_DRIVE can be a disk file, this indicates a disk emulation."
  "\nIf --read-only is given, the emulated drive will be write-protected."
  "\nIf one of --status, --ram-drive, --rd-base or --rd-size is given,\n"
  "  then any other command-line arguments will be ignored."
  "\nThe --mem option indicates a drive in memory(0-4Gb)."
  "\nThe --mem --top option indicates a drive in memory(>4Gb)."	
  "\nThe --no-hook option indicates FROM_DRIVE is not mounted to UEFI firmware."
  "\nif RESERV is used and <= 0, the minimum memory occupied by the memdrive\n"
  "  is (-RESERV) in 512-byte-sectors."
  "\nif RESERV is used and > 0,the memdrive will occupy the mem area starting\n"
  "  at absolute physical address RESERV in 512-byte-sectors and ending at the end of this mem"
  "\nIf --swap-drivs=DRIVE1=DRIVE2 is given, swap DRIVE1 and DRIVE2 for FROM_DRIVE."
  " block(usually the end of physical mem)."
  "\nIf --in-situ=FLAGS_AND_ID is given, the low byte is FLAGS(default 0) and the high byte\n"
  "  is partition type ID(use 0xnnnn to specify)."
};


#ifdef USE_MD5_PASSWORDS
/* md5crypt */
static int md5crypt_func (char *arg, int flags);
static int
md5crypt_func (char *arg, int flags)
{
  char crypted[36];
  char key[32];
  unsigned int seed;
  int i;
  const char *const seedchars =
    "./0123456789ABCDEFGHIJKLMNOPQRST"
    "UVWXYZabcdefghijklmnopqrstuvwxyz";
  
  /* First create a salt.  */

  errnum = 0;
  /* The magical prefix.  */
  grub_memset (crypted, 0, sizeof (crypted));
  grub_memmove (crypted, "$1$", 3);

  /* Create the length of a salt.  */
  seed = *(unsigned int *)0x46C;

  /* Generate a salt.  */
  for (i = 0; i < 8 && seed; i++)
    {
      /* FIXME: This should be more random.  */
      crypted[3 + i] = seedchars[seed & 0x3f];
      seed >>= 6;
    }

  /* A salt must be terminated with `$', if it is less than 8 chars.  */
  crypted[3 + i] = '$';

#ifdef DEBUG_MD5CRYPT
  grub_printf ("salt = %s\n", crypted);
#endif
  if (*arg)
	sprintf(key,"%.30s",arg);
  else
  {
	  /* Get a password.  */
	  grub_memset (key, 0, sizeof (key));
	  get_cmdline_str.prompt = msg_password;
	  get_cmdline_str.maxlen = sizeof (key) - 1;
	  get_cmdline_str.echo_char = '*';
	  get_cmdline_str.readline = 0;
	  get_cmdline_str.cmdline = (unsigned char*)key;
	  get_cmdline ();
  }
  /* Crypt the key.  */
  make_md5_password (key, crypted);

  grub_printf ("Encrypted: %s\n", crypted);
  return 1;
}

static struct builtin builtin_md5crypt =
{
  "md5crypt",
  md5crypt_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "md5crypt",
  "Generate a password in MD5 format."
};

static int crc32_func(char *arg, int flags);
static int crc32_func(char *arg, int flags)
{
  int crc = grub_crc32(arg,0);
  printf("%08x\n",crc);
  return crc;
}

static struct builtin builtin_crc32 =
{
  "crc32",
  crc32_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE | BUILTIN_NO_DECOMPRESSION,
  "crc32 FILE | STRING",
  "Calculate the crc32 checksum of a FILE or a STRING."
};
#endif /* USE_MD5_PASSWORDS */


/* module */
static int module_func (char *arg, int flags);
static int
module_func (char *arg, int flags)
{
  int len = grub_strlen (arg);

  errnum = 0;
  switch (kernel_type)
    {
    case KERNEL_TYPE_MULTIBOOT:
//      if (mb_cmdline + len + 1 > (char *) MB_CMDLINE_BUF + MB_CMDLINE_BUFLEN)
      if (len + 1 > MB_CMDLINE_BUFLEN)
	{
	  errnum = ERR_WONT_FIT;
	  return 0;
	}
      grub_memmove (mb_cmdline, arg, len + 1);
      if (! load_module (arg, mb_cmdline))
	return 0;
      mb_cmdline += len + 1;
      break;

    case KERNEL_TYPE_LINUX:
    case KERNEL_TYPE_BIG_LINUX:
      if (! load_initrd (arg))
	return 0;
      break;

    default:
      errnum = ERR_NEED_MB_KERNEL;
      return 0;
    }

  return 1;
}

static struct builtin builtin_module =
{
  "module",
  module_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "module FILE [ARG ...]",
  "Load a boot module FILE for a Multiboot format boot image (no"
  " interpretation of the file contents is made, so users of this"
  " command must know what the kernel in question expects). The"
  " rest of the line is passed as the \"module command line\", like"
  " the `kernel' command."
};


/* modulenounzip */
static int modulenounzip_func (char *arg, int flags);
static int
modulenounzip_func (char *arg, int flags)
{
  return module_func (arg, flags);
}

static struct builtin builtin_modulenounzip =
{
  "modulenounzip",
  modulenounzip_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_NO_DECOMPRESSION,
  "modulenounzip FILE [ARG ...]",
  "The same as `module', except that automatic decompression is"
  " disabled."
};

#ifdef SUPPORT_GRAPHICS
#if 0
/* outline [on | off | status] */
static int outline_func (char *arg, int flags);
static int
outline_func (char *arg, int flags)
{

  errnum = 0;
  /* If ARG is empty, toggle the flag.  */
  if (! *arg)
    outline = ! outline;
  else if (grub_memcmp (arg, "on", 2) == 0)
    outline = 1;
  else if (grub_memcmp (arg, "off", 3) == 0)
    outline = 0;
  else if (grub_memcmp (arg, "status", 6) == 0)
  {
    printf_debug0 (" Character outline is now %s\n", (outline ? "on" : "off"));
  }
  else
      errnum = ERR_BAD_ARGUMENT;

  return outline;
}

static struct builtin builtin_outline =
{
  "outline",
  outline_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "outline [on | off | status]",
  "Turn on/off or display the outline mode, or toggle it if no argument."
};
#endif
#endif /* SUPPORT_GRAPHICS */


/* pager [on|off] */
static int pager_func (char *arg, int flags);
static int
pager_func (char *arg, int flags)
{

  errnum = 0;
  /* If ARG is empty, toggle the flag.  */
  if (! *arg)
    use_pager = ! use_pager;
  else if (grub_memcmp (arg, "on", 2) == 0)
    use_pager = 1;
  else if (grub_memcmp (arg, "off", 3) == 0)
    use_pager = 0;
  else if (grub_memcmp (arg, "status", 6) == 0)
  {
    printf_debug0 (" Internal pager is now %s\n", (use_pager ? "on" : "off"));
  }
  else
      errnum = ERR_BAD_ARGUMENT;
  if (use_pager == 0)
    count_lines = -1;
  else if (count_lines == -1)
    count_lines = 0;
  return use_pager;
}

static struct builtin builtin_pager =
{
  "pager",
  pager_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "pager [on | off | status]",
  "Turn on/off or display the pager mode, or toggle it if no argument."
};

  /* Convert a LBA address to a CHS address in the INT 13 format.  */
void lba_to_chs (unsigned int lba, unsigned int *cl, unsigned int *ch, unsigned int *dh);
void
lba_to_chs (unsigned int lba, unsigned int *cl, unsigned int *ch, unsigned int *dh)
{
      unsigned int cylinder, head, sector;
      
  if (lba >= 0xfb03ff) //如果超过8GB，则应将1023、254、63用于CHS。issues#374  
  {
    sector = 63; 
    head = 254;
    cylinder = 1023;
  }
  else
  {
    sector = lba % 0x3f + 1;
    head = (lba / 0x3f) % 0xff;
    cylinder = lba / (0x3f * 0xff);

    if (cylinder > 0x3FF)
      cylinder = 0x3FF;
  }      
  *cl = sector | ((cylinder & 0x300) >> 2);
  *ch = cylinder & 0xFF;
  *dh = head;
}

/* partnew PART TYPE START LEN */
static int partnew_func (char *arg, int flags);
static int
partnew_func (char *arg, int flags)
{
  unsigned long long new_type, new_start, new_len;
  unsigned int start_cl, start_ch, start_dh;
  unsigned int end_cl, end_ch, end_dh;
  unsigned int current_drive_bak;
  unsigned int current_partition_bak;
  char *filename;
  unsigned int entry1, i;
  unsigned int active = -1;
  int force = 0;

  errnum = 0;
resume:
  if (grub_memcmp (arg, "--active", 8) == 0)
    {
      active = 0x80;
      arg = skip_to (0, arg);
      goto resume;
    }
  if (grub_memcmp (arg, "--force", 7) == 0)
    {
      force = 1;
      arg = skip_to (0, arg);
      goto resume;
    }

  /* Get the drive and the partition.  */
  if (! set_device (arg))
    return 0;

  entry1 = current_partition >> 16;
  
  /* The partition must a primary partition.  */
  if (entry1 > 3 || (current_partition & 0xFFFF) != 0xFFFF)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }

  /* Get the new partition type.  */
  arg = skip_to (0, arg);
  if (! safe_parse_maxint (&arg, &new_type))
    return 0;

  /* The partition type is unsigned char.  */
  if (new_type > 0xFF)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }

  /* Get the new partition start and length.  */
  arg = skip_to (0, arg);
  filename = arg;

  current_drive_bak = 0;
  if ((! safe_parse_maxint (&arg, &new_start))
      || ((arg = skip_to (0, arg)), (! safe_parse_maxint (&arg, &new_len))))
  {
      current_drive_bak = current_drive;
      current_partition_bak = current_partition;
      arg = filename;
      filename = set_device (filename);

      if (errnum)
      {
	/* No device specified. Default to the root device. */
	current_drive = saved_drive;
	current_partition = saved_partition;
	filename = 0;
	errnum = 0;
      }
  
      if (current_drive != current_drive_bak)
      {
	printf_debug0 ("Cannot create a partition in drive %X from a file in drive %X.\n", current_drive_bak, current_drive);
	errnum = ERR_BAD_ARGUMENT;
	return 0;
      }

      if (current_partition == 0xFFFFFF)
      {
	printf_debug0 ("Cannot create a partition with a blocklist of a whole drive.\n");
	errnum = ERR_BAD_ARGUMENT;
	return 0;
      }

      query_block_entries = -1; /* query block list only */
      blocklist_func (arg, flags);
      if (errnum == 0)
      {
	if (query_block_entries != 1)
		return ! (errnum = ERR_NON_CONTIGUOUS);
	new_start = map_start_sector[0];
	new_len = (filemax + 0x1ff) >> SECTOR_BITS;
      }
      else
	return ! errnum;

      if (new_start == part_start && part_start && new_len == 1)
	new_len = part_length;

      if (new_start < part_start || new_start + new_len > (unsigned int)(part_start + part_length))
      {
	printf_debug0 ("Cannot create a partition that exceeds the partition boundary.\n");
	return ! (errnum = ERR_BAD_ARGUMENT);
      }
    
      /* Read the first sector.  */
      if (! rawread (current_drive, new_start, 0, SECTOR_SIZE, (unsigned long long)(grub_size_t)mbr, 0xedde0d90))
        return 0;

#define	BS	((struct master_and_dos_boot_sector *)mbr)
      /* try to find out the filesystem type */
      if (BS->boot_signature == 0xAA55 && ! probe_bpb(BS))
      {
	if ((new_type & 0xFFFFFFEF) == 0)	/* auto filesystem type */
	{
		new_type |= 
			filesystem_type == 1 ? 0x0E /* FAT12 */ :
			filesystem_type == 2 ? 0x0E /* FAT16 */ :
			filesystem_type == 3 ? 0x0C /* FAT32 */ :
			filesystem_type == 4 ? 0x07 /* NTFS */  :
			filesystem_type == 6 ? 0x07 /* exFAT */ :
			/*filesystem_type == 5 ?*/ 0x83 /* EXT2 */;
		if (filesystem_type == 5)
			new_type = 0x83; /* EXT2 */
	}
	printf_debug0 ("%s BPB found %s the leading 0xEB (jmp). Hidden sectors=0x%X\n",
		(filesystem_type == 1 ? "FAT12" :
		filesystem_type == 2 ? "FAT16" :
		filesystem_type == 3 ? "FAT32" :
		filesystem_type == 4 ? "NTFS"  :
		filesystem_type == 6 ? "exFAT" :
		/*filesystem_type == 5 ?*/ "EXT2 GRLDR"),
		(BS->dummy1[0] == (char)0xEB ? "with" : "but WITHOUT"),
		BS->hidden_sectors);
	if (BS->hidden_sectors != new_start)
	{
	    printf_debug0 ("Changing hidden sectors 0x%X to 0x%lX... ", BS->hidden_sectors, (unsigned long long)new_start);
	    BS->hidden_sectors = new_start;
	    /* Write back/update the boot sector.  */
	    if (! rawwrite (current_drive, new_start, (unsigned long long)(grub_size_t)mbr))
	    {
		printf_debug0 ("failure.\n");
	        return 0;
	    } else {
		printf_debug0 ("success.\n");
	    }
	}
      }
#undef BS

      current_drive = current_drive_bak;
      current_partition = current_partition_bak;
  }

  /* Read the MBR.  */
  if (! rawread (current_drive, 0, 0, SECTOR_SIZE, (unsigned long long)(grub_size_t)mbr, 0xedde0d90))
    return 0;

  if (current_drive_bak && !force)	/* creating a partition from a file */
  {
	/* if the entry is not empty, it should be a part of another
	 * partition, that is, it should be covered by another partition. */
    if (PC_SLICE_START (mbr, entry1) != 0 || PC_SLICE_LENGTH (mbr, entry1) != 0)
    {
	for (i = 0; i < 4; i++)
	{
		if (i == entry1)
			continue;
		if (PC_SLICE_START (mbr, entry1) < PC_SLICE_START (mbr, i))
			continue;
		if (PC_SLICE_START (mbr, entry1) + PC_SLICE_LENGTH (mbr, entry1)
		    > PC_SLICE_START (mbr, i) + PC_SLICE_LENGTH (mbr, i))
			continue;
		break;	/* found */
	}
	if (i >= 4)
	{
		/* not found */
		printf_debug0 ("Cannot overwrite an independent partition. Can use the parameter '--force' to enforce\n");
		return ! (errnum = ERR_BAD_ARGUMENT);
	}
    }
  }

  if (new_type == 0 && new_start == 0 && new_len == 0)
  {
    /* empty the entry */
    start_dh = start_cl = start_ch = end_dh = end_cl = end_ch = 0;
  }else{
    if (new_start == 0 || new_len == 0)
    {
      errnum = ERR_BAD_PART_TABLE;
      return 0;
    }
    /* Store the partition information in the MBR.  */
    lba_to_chs (new_start, &start_cl, &start_ch, &start_dh);
    lba_to_chs (new_start + new_len - 1, &end_cl, &end_ch, &end_dh);
  }

  if (active == 0x80)
  {
    /* Activate this partition */
    PC_SLICE_FLAG (mbr, entry1) = 0x80;
  } else {
    if (PC_SLICE_FLAG (mbr, entry1) != 0x80)
        PC_SLICE_FLAG (mbr, entry1) = 0;
  }

  /* Deactivate other partitions */
  if (PC_SLICE_FLAG (mbr, entry1) == 0x80)
  {
	for (i = 0; i < 4; i++)
	{
		if (i == entry1)
			continue;
		if (PC_SLICE_FLAG (mbr, i) != 0)
		{
		    if (debug > 0)
		    {
		      if (PC_SLICE_FLAG (mbr, i) == 0x80)
		      {
			grub_printf ("The active flag(0x80) of partition %d was changed to 0.\n", (unsigned int)i);
		      } else {
			grub_printf ("The invalid active flag(0x%X) of partition %d was changed to 0.\n", (unsigned int)(PC_SLICE_FLAG (mbr, i)), (unsigned int)i);
		      }
		    }

		    PC_SLICE_FLAG (mbr, i) = 0;
		}
	}
  }
  PC_SLICE_HEAD (mbr, entry1) = start_dh;
  PC_SLICE_SEC (mbr, entry1) = start_cl;
  PC_SLICE_CYL (mbr, entry1) = start_ch;
  PC_SLICE_TYPE (mbr, entry1) = new_type;
  PC_SLICE_EHEAD (mbr, entry1) = end_dh;
  PC_SLICE_ESEC (mbr, entry1) = end_cl;
  PC_SLICE_ECYL (mbr, entry1) = end_ch;
  PC_SLICE_START (mbr, entry1) = new_start;
  PC_SLICE_LENGTH (mbr, entry1) = new_len;

  /* Make sure that the MBR has a valid signature.  */
  PC_MBR_SIG (mbr) = PC_MBR_SIGNATURE;
  
  /* Write back the MBR to the disk.  */
  buf_track = -1;
  if (! rawwrite (current_drive, 0, (unsigned long long)(grub_size_t)mbr))
    return 0;

  return 1;
}

static struct builtin builtin_partnew =
{
  "partnew",
  partnew_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST | BUILTIN_NO_DECOMPRESSION,
  "partnew [--active] [--force] PART TYPE START [LEN]",
  "Create a primary partition at the starting address START with the"
  " length LEN, with the type TYPE. START and LEN are in sector units."
  " If --active is used, the new partition will be active."
  " If --force is used, can overwrite an independent partition. START can be"
  " a contiguous file that will be used as the content/data of the new"
  " partition, in which case the LEN parameter is ignored, and TYPE can"
  " be either 0x00 for auto or 0x10 for hidden-auto."
};

#define GPT_ATTRIBUTE_NO_DRIVE_LETTER	(0x8000000000000000LL)
#define GPT_ATTRIBUTE_HIDDEN		(0x4000000000000000LL)
#define GPT_ATTRIBUTE_SHADOW_COPY	(0x2000000000000000LL)
#define GPT_ATTRIBUTE_READ_ONLY		(0x1000000000000000LL)
#define GPT_ATTR_HIDE			(0xC000000000000001LL)
#define GPT_HDR_SIZE 			(0x5C)
static int gpt_set_crc(P_GPT_HDR hdr)
{
	char data[SECTOR_SIZE];
	int crc;
	int errnum_bak = errnum;

	if (hdr->hdr_size != GPT_HDR_SIZE)
		return 0;
	errnum = 0;
	sprintf(data,"(0x%X)0x%lx+%u,%u",current_drive,hdr->hdr_lba_table,32,hdr->hdr_entries * hdr->hdr_entsz);
	crc = grub_crc32(data,0);
	if (errnum)
		return 0;
	hdr->hdr_crc_table = crc;
	hdr->hdr_crc_self = 0;
	crc = grub_crc32((char*)hdr,GPT_HDR_SIZE);
	if (errnum)
		return 0;
	hdr->hdr_crc_self = crc;
	buf_track = -1;
	if (! rawread (current_drive, hdr->hdr_lba_self ,0,GPT_HDR_SIZE, (unsigned long long)(grub_size_t)hdr, GRUB_WRITE))
		return 0;
	errnum = errnum_bak;
	return hdr->hdr_crc_table;
}

static int gpt_set_attr(P_GPT_HDR hdr,grub_u32_t part,grub_u64_t attr)
{
	GPT_ENT ent;
	if (! rawread (current_drive, hdr->hdr_lba_table + (part >> 2), (part & 3) * sizeof(GPT_ENT), sizeof(GPT_ENT), (unsigned long long)(grub_size_t)&ent, GRUB_READ))
		return 0;

	if (attr & 0xFF00)
		ent.ms_attr.gpt_att = (unsigned short)attr;
	else
		ent.attributes = attr;

	buf_track = -1;
	if (! rawread (current_drive, hdr->hdr_lba_table + (part >> 2), (part & 3) * sizeof(GPT_ENT), sizeof(GPT_ENT), (unsigned long long)(grub_size_t)&ent, GRUB_WRITE))
		return 0;
	return gpt_set_crc(hdr);
}

static unsigned int gpt_slic_set_attr(grub_u32_t part,grub_u64_t attr)
{
	char data[SECTOR_SIZE];
	int crc1,crc2;
	if (! rawread (current_drive, 1, 0, sizeof(GPT_HDR), (unsigned long long)(grub_size_t)data, GRUB_READ))
		return 0;
	P_GPT_HDR hdr = (P_GPT_HDR)data;
	crc1 = gpt_set_attr(hdr,part,attr);
	if (!crc1)
		return 0;
	if (! rawread (current_drive, hdr->hdr_lba_alt, 0, sizeof(GPT_HDR), (unsigned long long)(grub_size_t)data, GRUB_READ))
		return 0;
	crc2 = gpt_set_attr(hdr,part,attr);
	if (!crc2)
		return 0;
	if (debug > 1 && crc1 != crc2)
		printf("Warning! Main partition table CRC mismatch!");
	return 1;
}

static unsigned long long gpt_slic_get_attr(char *data,unsigned int part)
{
	P_GPT_ENT p = (P_GPT_ENT)data;
	return p->attributes;
}

/* Hide/Unhide CURRENT_PARTITION.  */
static int
set_partition_hidden_flag (int hidden)
{
  struct grub_part_data *q;
  
#if 0
  /* The drive must be a hard disk.  */
  if (! (current_drive & 0x80))
  {
    errnum = ERR_BAD_ARGUMENT;
    return 0;
  }
#endif
  
  /* The partition must be a PC slice.  */
  if ((current_partition >> 16) == 0xFF
      || (current_partition & 0xFFFF) != 0xFFFF)
  {
    errnum = ERR_BAD_ARGUMENT;
    return 0;
  }

  q = get_partition_info (current_drive, current_partition);
  if (!q)
    return 0;
                                                                       
  int part_num = (unsigned int)(unsigned char)(q->partition>>16);
  grub_u64_t part_attr = 0LL,is_hidden;
  if (q->partition_type == PC_SLICE_TYPE_GPT)
  {
		part_attr = gpt_slic_get_attr(mbr,part_num);
		is_hidden = !!(part_attr & GPT_ATTR_HIDE);
  }
  else
		is_hidden = PC_SLICE_TYPE (mbr, q->partition_entry) & PC_SLICE_TYPE_HIDDEN_FLAG;
  /* Found.  */
  if (hidden == -1)	/* status only */
  {
		printf_debug0 ("Partition (%cd%d,%d) is %shidden.\n",
				((current_drive & 0x80) ? 'h' : 'f'),
				(current_drive & ~0x80),
				part_num,
				(is_hidden ? "" : "not "));
		
		return is_hidden;
  }

  /* Check if the hidden flag need change.  */
  if ((!hidden) != (!is_hidden))
  {
    if (q->partition_type == PC_SLICE_TYPE_GPT)
    {
      if (hidden)
        part_attr |= GPT_ATTR_HIDE;
      else
        part_attr &= ~GPT_ATTR_HIDE;

      if (!gpt_slic_set_attr(part_num,part_attr))
        return 0;
    }
    else
    {
			if (hidden)
				PC_SLICE_TYPE (mbr, q->partition_entry) |= PC_SLICE_TYPE_HIDDEN_FLAG;
			else
				PC_SLICE_TYPE (mbr, q->partition_entry) &= ~PC_SLICE_TYPE_HIDDEN_FLAG;

			/* Write back the MBR to the disk.  */
			buf_track = -1;
			if (! rawwrite (current_drive, q->partition_offset, (unsigned long long)(grub_size_t)mbr))
				return 0;
		}

		printf_debug0 ("Partition (%cd%d,%d) successfully set %shidden.\n",
				((current_drive & 0x80) ? 'h' : 'f'),
				(current_drive & ~0x80),
				part_num,
				(hidden ? "" : "un"));
  }
  else
  {
		printf_debug0 ("Partition (%cd%d,%d) was already %shidden.\n",
				((current_drive & 0x80) ? 'h' : 'f'),
				(current_drive & ~0x80),
				part_num,
				(hidden ? "" : "un"));

  }
	  
  /* Succeed.  */
  return 1;
}



/* parttype PART TYPE */
static int
parttype_func (char *arg, int flags)
{
  unsigned long long new_type = -1;
//  unsigned int part = 0xFFFFFF;
//  unsigned long long start, len, offset;
//  unsigned int type, entry1, ext_offset1;
  struct grub_part_data *q;
  /* Get the drive and the partition.  */

  errnum = 0;
  if (! *arg || *arg == ' ' || *arg == '\t')
  {
	current_drive = saved_drive;
	current_partition = saved_partition;
  }
  else if (! set_device (arg))
  {
    if (! safe_parse_maxint (&arg, &new_type))
      return 0;
    current_drive = saved_drive;
    current_partition = saved_partition;
  }
  else
  {
    /* Get the new partition type.  */
    arg = skip_to (0, arg);
    if (*arg && *arg != ' ' && *arg != '\t')
      if (! safe_parse_maxint (&arg, &new_type))
        return 0;
  }

  /* The partition must be a PC slice.  */
  if ((current_partition >> 16) == 0xFF
      || (current_partition & 0xFFFF) != 0xFFFF)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }

  rawread (current_drive, 0, 0, SECTOR_SIZE, (unsigned long long)(grub_size_t)mbr, 0xedde0d90);	//mbr可能存储的是其他分区信息，必需更新一下。2022-11-29
  q = get_partition_info (current_drive, current_partition);
  if (!q)
    return 0;
  
	  errnum = 0;
	  if (new_type == -1)	/* return the current type */
	  {
//		new_type = (q->partition_type == PC_SLICE_TYPE_GPT)?0xEE:PC_SLICE_TYPE (mbr, q->partition_entry);
		new_type = q->partition_type;
		if (debug > 0)
			printf ("Partition type for (%cd%d,%d) is 0x%02X.\n",
				((current_drive & 0x80) ? 'h' : 'f'),
				(current_drive & ~0x80),
				(unsigned int)(unsigned char)(current_partition >> 16),
				(unsigned int)new_type);
		return new_type;
	  }

	  if (q->partition_type == PC_SLICE_TYPE_GPT) /* set gpt partition attributes*/
	    return gpt_slic_set_attr(q->partition>>16,new_type);

	  /* The partition type is unsigned char.  */
	  if (new_type > 0xFF)
	  {
	      errnum = ERR_BAD_ARGUMENT;
	      return 0;
	  }
	  /* Set the type to NEW_TYPE.  */
	  PC_SLICE_TYPE (mbr, q->partition_entry) = new_type;

	  /* Write back the MBR to the disk.  */
	  buf_track = -1;
	  if (! rawwrite (current_drive, q->partition_offset, (unsigned long long)(grub_size_t)mbr))
	    return 0;	/* failure */

	  if (debug > 0)
		printf ("Partition type for (%cd%d,%d) set to 0x%02X successfully.\n",
			((current_drive & 0x80) ? 'h' : 'f'),
			(current_drive & ~0x80),
			(unsigned int)(unsigned char)(current_partition >> 16),
			(unsigned int)new_type);
	  /* Succeed.  */
	  errnum = 0;
	  return 1;
}

static struct builtin builtin_parttype =
{
  "parttype",
  parttype_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "parttype [PART] [TYPE]",
  "Change the type of the partition PART to TYPE. If TYPE is omitted, return "
  "the partition type of the specified device(instead of changing it). PART "
  "default to the current root device."
};

/* password */
static int password_func (char *arg, int flags);
static int
password_func (char *arg, int flags)
{
  int len;
  password_t type = PASSWORD_PLAIN;

  errnum = 0;
#ifdef USE_MD5_PASSWORDS
  if (grub_memcmp (arg, "--md5", 5) == 0)
    {
      type = PASSWORD_MD5;
      arg = skip_to (0, arg);
    }
#endif
  if (grub_memcmp (arg, "--", 2) == 0)
    {
      type = PASSWORD_UNSUPPORTED;
      arg = skip_to (0, arg);
    }

  if ((flags & (BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_BAT_SCRIPT)) != 0)
    {
      nul_terminate (arg);
      if ((len = check_password (arg, type)) != 0)
	{
	  errnum = (len == 0xFFFF ? ERR_MD5_FORMAT : ERR_PRIVILEGED);
	  return 0;
	}
    }
  else
    {
      len = grub_strlen (arg);
      
      /* PASSWORD NUL NUL ... */
      if (len + 2 > (int)sizeof (password_str)/* PASSWORD_BUFLEN */)
	{
	  errnum = ERR_WONT_FIT;
	  return 0;
	}
      
      /* Copy the password and clear the rest of the buffer.  */
      password_buf = password_str;//(char *) PASSWORD_BUF;
      grub_memmove (password_buf, arg, len);
      grub_memset (password_buf + len, 0, sizeof (password_str)/* PASSWORD_BUFLEN */ - len);
      password_type = type;
    }
  return 1;
}

static struct builtin builtin_password =
{
  "password",
  password_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_NO_ECHO,
  "password [--md5] PASSWD [FILE]",
  "If used in the first section of a menu file, disable all"
  " interactive editing control (menu entry editor and"
  " command line). If the password PASSWD is entered, it loads the"
  " FILE as a new config file and restarts the GRUB Stage 2. If you"
  " omit the argument FILE, then GRUB just unlocks privileged"
  " instructions.  You can also use it in the script section, in"
  " which case it will ask for the password, before continueing."
  " The option --md5 tells GRUB that PASSWD is encrypted with"
  " md5crypt."
};


/* pause */
static int pause_func (char *arg, int flags);
static int
pause_func (char *arg, int flags)
{
//  char *p;
  unsigned long long wait = -1;
  int time1;
  int time2 = -1;
  int testkey = 0;

  errnum = 0;
	for (;;)
	{
		if (grub_memcmp (arg, "--test-key", 10) == 0)
		{
			testkey = 1;
		}
		else if (grub_memcmp (arg, "--wait=", 7) == 0)
		{
			arg += 7;
			if (! safe_parse_maxint (&arg, &wait))
				return 0;
		}
		else
			break;
		arg = skip_to (0, arg);
	}
  
  if (*arg)
    printf("%s\n", arg);

  /* Get current time.  */
  int ret = 1;
  while ((time2 = getrtsecs ()) == 0xFF);
  while (wait != 0)
   {
      /* Check if there is a key-press.  */
			if ((ret = checkkey ()) != (int)-1)
      {
      	if (testkey)
      	{
      		printf_debug0("%04x",ret);
      		return ret;
      	}
//         ret &= 0xFF;
         /* Check the special ESC key  */
         if ((unsigned short)ret == 0x011b)
            return 0;	/* abort this entry */
         break;
      }

      if (wait != (unsigned long long)-1 && (time1 = getrtsecs ()) != time2 && time1 != 0xFF)
      {
         printf_debug0("\t%d\t\r",wait);
         time2 = time1;
         wait--;
      }
   }
   return ret;
}

static struct builtin builtin_pause =
{
  "pause",
  pause_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_NO_ECHO,
  "pause [--test-key] [--wait=T] [MESSAGE ...]",
  "Print MESSAGE, then wait until a key is pressed or T seconds has passed."
  "--test-key display keyboard code."	
};


#ifdef FSYS_PXE
/* pxe */
static struct builtin builtin_pxe =
{
  "pxe",
  pxe_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING | BUILTIN_IFTITLE,
  "pxe [cmd] [parameters]",
  "Call PXE command."
};
#endif
#if 0
#ifdef FSYS_IPXE
/* pxe */
static struct builtin builtin_ipxe =
{
  "ipxe",
  ipxe_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING | BUILTIN_IFTITLE,
  "ipxe [cmd] [parameters]",
  "Call iPXE command."
};
#endif
#endif

#ifndef NO_DECOMPRESSION
static int raw_func(char *arg, int flags);
static int raw_func(char *arg, int flags)
{
	return run_line(arg,flags);
}

static struct builtin builtin_raw =
{
  "raw",
  raw_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE | BUILTIN_NO_DECOMPRESSION,
  "raw COMMAND",
  "run COMMAND without auto-decompression."
};
#endif

static int read_func (char *arg, int flags);
static int
read_func (char *arg, int flags)
{
  unsigned long long addr, val;
	int bytes=0, mem=0;

  errnum = 0;
  if (*(int *)arg == 0x2E524156)//VAR. 
  {//for Fast access to system variables.(defined in asm.s)
    arg += sizeof(int);
    if (! safe_parse_maxint (&arg, &addr))
	return 0;
    return (*(grub_size_t **)IMG(0x8308))[addr];
  }
  
  for (;;)
  {
	if (grub_memcmp (arg, "--8", 3) == 0)
	{
		bytes=1;
		arg += 3;
	}
  else if (grub_memcmp (arg, "--mem", 5) == 0)
	{
		mem=1;    //强制使用普通内存，否则0x8200-0x8400(不含)特指G4E头部固定数据区。
		arg += 5;
	}
  else
    break;
  arg = skip_to (0, arg);
  }
  
  if (! safe_parse_maxint (&arg, &addr))
    return 0;

  if (!mem && addr >= 0x8200 && addr < 0x8400)
    addr += (grub_size_t)g4e_data - 0x8200;

	if (!bytes)
		val = *(unsigned int *)(grub_size_t)addr;
	else
		val = *(unsigned long long *)(grub_size_t)addr;
  printf_debug0 ("Address 0x%lx: Value 0x%lx\n", addr, val);
  return val;
}

static struct builtin builtin_read =
{
  "read",
  read_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "read [--8] [--mem] ADDR",
  "Read a 32-bit or 64-bit value from memory at address ADDR and display it in hex format.\n"
  "--mem  Force normal memory, \n"
  "       otherwise, 0x8200-0x8400(not included) specifically refers to G4e head fixed data area."
};

int parse_string (char *arg);
int
parse_string (char *arg)
{
  int len;
  char *p;
  char ch;

  //nul_terminate (arg);
	for (len = 0,p = arg;(ch = *p);p++)
	{
		if (ch == '\\' && (ch = *++p))
		{
			switch(ch)
			{
				case 't':
					*arg++ = '\t';
					break;
				case 'r':
					*arg++ = '\r';
					break;
				case 'n':
					*arg++ = '\n';
					break;
				case 'a':
					*arg++ = '\a';
					break;
				case 'b':
					*arg++ = '\b';
					break;
				case 'f':
					*arg++ = '\f';
					break;
				case 'v':
					*arg++ = '\v';
					break;
				case 'x':		//\xnn
				{
					/* hex */
					int val;

					p++;
					ch = *p;
					if (ch <= '9' && ch >= '0')
						val = ch & 0xf;
					else if ((ch <= 'F' && ch >= 'A') || (ch <='f' && ch >= 'a'))
						val = (ch + 9) & 0xf;
					else
						return len;	/* error encountered */

					p++;
					ch = *p;

					if (ch <= '9' && ch >= '0')
						val = (val << 4) | (ch & 0xf);
					else if ((ch <= 'F' && ch >= 'A') || (ch <='f' && ch >= 'a'))
						val = (val << 4) | ((ch + 9) & 0xf);
					else
					    --p;

					*arg++ = val;
				}
					break;
				case 'X':		//\Xnnnn
				{
					/* hex */
					int val, i = 0;
					char uni[4];

					while (i < 2)
					{
						p++;
						ch = *p;
						if (ch <= '9' && ch >= '0')
							val = ch & 0xf;
						else if ((ch <= 'F' && ch >= 'A') || (ch <='f' && ch >= 'a'))
							val = (ch + 9) & 0xf;
						else
							return len;	/* error encountered */
						
						p++;
						ch = *p;
						if (ch <= '9' && ch >= '0')
							val = (val << 4) | (ch & 0xf);
						else if ((ch <= 'F' && ch >= 'A') || (ch <='f' && ch >= 'a'))
							val = (val << 4) | ((ch + 9) & 0xf);
						else
							--p;

						uni[i] = val;
						i++;
					}
					uni[3] = uni[0];
					uni[0] = uni[1];
					uni[1] = uni[3];
					i=unicode_to_utf8((unsigned short *)uni, (unsigned char *)arg, 1);
					arg += i;
					len += i - 1;
				}
					break;
				default:
					if (ch >= '0' && ch <= '7')
					{
						/* octal */
						int val = ch & 7;
						int i;
						
						for (i=0;i<2 && p[1] >= '0' && p[1] <= '7';i++)
						{
							p++;
							val <<= 3;
							val |= *p & 7;
						}

						*arg++ = val;
						break;
					} else *arg++ = ch;
			}
		} else *arg++ = ch;
		
		len++;
	}

  return len;
}

static int write_func (char *arg, int flags);
static int
write_func (char *arg, int flags)
{
  unsigned long long addr;
  unsigned long long val;
  char *p;
  unsigned int tmp_drive;
  unsigned int tmp_partition;
  unsigned long long offset;
  unsigned long long len;
  unsigned long long bytes = 0;
  char tmp_file[16];
	int mem = 0;
//  int block_file = 0;

  errnum = 0;
  tmp_drive = saved_drive;
  tmp_partition = saved_partition;
  offset = 0;
  for (;;)
  {
    if (grub_memcmp (arg, "--offset=", 9) == 0)
    {
      p = arg + 9;
      if (! safe_parse_maxint (&p, &offset))
        return 0;
    }
    else if (grub_memcmp (arg, "--bytes=", 8) == 0)
    {
      p = arg + 8;
      if (! safe_parse_maxint (&p, &bytes))
        return 0;
    }
		else if (grub_memcmp (arg, "--mem", 5) == 0)
    {
			mem = 1;
			arg += 5;
		}
    else
      break;
    arg = skip_to (0, arg);
  }
  
  p = NULL;
  addr = -1;
  if (*arg == '/' || *arg == '(')
  {
	/* destination is device or file. */
  	if (*arg == '(')
    {
      p = set_device (arg);
      if (errnum)
        goto fail;
      if (! p)
      {
        if (errnum == 0)
          errnum = ERR_BAD_ARGUMENT;
        goto fail;
      }
//      if (*p != '/')
//        block_file = 1;
      saved_drive = current_drive;
      saved_partition = current_partition;
      /* if only the device portion is specified */
      if ((unsigned char)*p <= ' ')
      {
        p = tmp_file;
        *p++ = '(';
        *p++ = ')';
        *p++ = '+';
        *p++ = '1';
        *p = 0;
        p = tmp_file;
        grub_open (p);
        if (errnum)
          goto fail;
        grub_sprintf (p + 3, "0x%lx", (unsigned long long)part_length);
        grub_close ();
      }
    }
    if (p != tmp_file)
	    p = arg;
    grub_open (p);
    current_drive = saved_drive;
    current_partition = saved_partition;
    if (errnum)
      goto fail;
#if 0
    if (current_drive != ram_drive && current_drive != 0xFFFF && block_file)
    {
      unsigned int j;

      /* check if it is a mapped memdrive */
      j = DRIVE_MAP_SIZE;		/* real drive */
      for (j = 0; j < DRIVE_MAP_SIZE; j++)
      {
        if (drive_map_slot_empty (disk_drive_map[j]))   //判断驱动器映像插槽是否为空   为空,返回1
        {
          j = DRIVE_MAP_SIZE;	/* real drive */
          break;
        }

        if (current_drive == disk_drive_map[j].from_drive && disk_drive_map[j].to_drive == 0xFF && !(disk_drive_map[j].to_log2_sector != 11))
          break;			/* memdrive */
      }

      if (j == DRIVE_MAP_SIZE)	/* real drive */
      {
		    /* this command is intended for running in command line and inhibited from running in menu.lst */
		    if (flags & (BUILTIN_MENU | BUILTIN_SCRIPT))
		    {
          grub_close ();
          errnum = ERR_WRITE_TO_NON_MEM_DRIVE;
          goto fail;
		    }
      }
    }
#endif
    filepos = offset;
  }
  else
  {
    /* destination is memory address. */
    if (*arg < '0' || *arg > '9')
    {
      errnum = ERR_BAD_ARGUMENT;
      goto fail;
    }
    if (! safe_parse_maxint (&arg, &addr))
      goto fail;
    if (addr == -1)
    {
      errnum = ERR_BAD_ARGUMENT;
      goto fail;
    }
  }

  /* destination is device or file if addr == -1 */
  /* destination is memory address if addr != -1 */

  arg = skip_to (0, arg);	/* INTEGER_OR_STRING */

  if (addr == (unsigned long long)-1)
  {
    /* string */
    if (! *arg)
    {
      grub_close ();
      errnum = ERR_BAD_ARGUMENT;
      goto fail;
    }
    len = parse_string (arg);

    if (bytes && bytes < len) len = bytes;

    if (saved_drive == 0xFFFF && p == tmp_file)	/* (md) */
    {
      grub_close ();
      grub_memmove64 (offset, (unsigned long long)(grub_size_t)arg, len);
      if ((grub_size_t)&saved_drive + 3 >= offset && (grub_size_t)&saved_drive < offset + len)
        tmp_drive = saved_drive;
      if ((grub_size_t)&saved_partition + 3 >= offset && (grub_size_t)&saved_partition < offset + len)
        tmp_partition = saved_partition;
      errnum = 0;
      goto succ;
    }
    /* write file */
    if (len > filemax - filepos)
	    len = filemax - filepos;
    if (grub_read ((unsigned long long)(grub_size_t)arg, len, 0x900ddeed) != len)	/* write */
    {
      if (errnum == 0)
        errnum = ERR_WRITE;
    }
    {
      int err = errnum;
      grub_close ();
      errnum = err;
    }
succ:
    if (errnum == 0)
      printf_debug0 ("0x%lX bytes written at offset 0x%lX.\n", (unsigned long long)len, (unsigned long long)offset);
  }
  else
  {
    if (bytes > 8) bytes = 8;
    else if (bytes == 0) bytes = 4;

    /* integer */
    p = arg;
    if (! safe_parse_maxint (&p, &val))
      goto fail;
    addr += offset;
    
    if (!mem && addr >= 0x8200 && addr < 0x8400)
      addr += (grub_size_t)g4e_data - 0x8200;
    
    arg = (char*)(grub_size_t)addr;
    p = (char*)(grub_size_t)&val;

    while(bytes--)
    {
      *arg++ = *p++;
    }
    printf_debug0 ("Address 0x%lx: Value 0x%x\n", (unsigned long long)addr, *(unsigned *)(grub_size_t)addr);
    if (addr != (grub_size_t)&saved_drive)
      saved_drive = tmp_drive;
    if (addr != (grub_size_t)&saved_partition)
      saved_partition = tmp_partition;
    errnum = 0;
    return val;
  }

fail:

  saved_drive = tmp_drive;
  saved_partition = tmp_partition;
  return !(errnum);
}

static struct builtin builtin_write =
{
  "write",
  write_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "write [--offset=SKIP] [--bytes=N] [#]ADDR_OR_FILE INTEGER_OR_STRING",
  "Write a 32-bit INTEGER to memory ADDR or write a STRING to FILE(or device!)\n"
  "To memory ADDR: default N=4, otherwise N<=8. Use 0xnnnnnnnn form.\n"
  "To FILE(or device): default STRING size.\n"
  "  UTF-8(or hex values) use \\xnn form, UTF-16(big endian) use \\Xnnnn form."
  "--mem  Force normal memory, \n"
  "       otherwise, 0x8200-0x8400(not included) specifically refers to G4e head fixed data area."
};


/* reboot */
static int reboot_func (char *arg, int flags);
static int
reboot_func (char *arg, int flags)
{
  errnum = 0;
  grub_reboot ();

  /* Never reach here.  */
  return 0;
}

static struct builtin builtin_reboot =
{
  "reboot",
  reboot_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "reboot",
  "Reboot your system."
};


/* Print the root device information to buffer.
   flag 0	saved device.
   flag 1  current device.
*/
void print_root_device (char *buffer,int flag);
void
print_root_device (char *buffer,int flag)
{
  unsigned int no_partition = 0;
  if (flag & 0x100)
  {
    no_partition = 1;
    flag &= 0xff;
  }
	unsigned int tmp_drive = flag?current_drive:saved_drive;
	unsigned int tmp_partition = flag?current_partition:saved_partition;
	unsigned char *tmp_hooked = NULL;

	if (buffer)
	{
		tmp_hooked = set_putchar_hook((unsigned char*)buffer);
	}
	else
		putchar(' ',255);
	switch(tmp_drive)
	{
	#ifdef FSYS_FB
		case FB_DRIVE:
			grub_printf("(ud)");
			break;
	#endif /* FSYS_FB */
	#ifdef FSYS_PXE
		case PXE_DRIVE:
      #if 0
			#ifdef FSYS_IPXE
			if (tmp_partition == IPXE_PART)
				grub_printf("(wd)");
			else
			#endif
      #endif
			grub_printf("(pd)");
			break;
	#endif /* PXE drive. */
		default:
      if (tmp_drive == 0xFFFF)
			{
				grub_printf("(md");
				if (md_part_base) grub_printf(",0x%lx,0x%lx",md_part_base,md_part_size);
			}
			else if (tmp_drive == ram_drive)
			{
				grub_printf("(rd");
			}
			else if (tmp_drive >= 0x9f)
			{
				grub_printf("(0x%x", tmp_drive);
			}
			else if (tmp_drive & 0x80)
			{
				/* Hard disk drive.  */
				grub_printf("(hd%d", (tmp_drive - 0x80));
			}
			else
			{
				/* Floppy disk drive.  */
				grub_printf("(fd%d", tmp_drive);
			}

			if ((tmp_partition & 0xFF0000) != 0xFF0000 && !no_partition)
				grub_printf(",%d", (unsigned int)(unsigned char)(tmp_partition >> 16));

			if ((tmp_partition & 0x00FF00) != 0x00FF00)
				grub_printf(",%c", (unsigned int)(unsigned char)((tmp_partition >> 8) + 'a'));

			putchar(')',1);
			break;
	}
	if (buffer)
		set_putchar_hook(tmp_hooked);

	return;
}

void print_vol (unsigned int drive);
void
print_vol (unsigned int drive)
{
	char uuid_found[256] = {0};
		get_vol (uuid_found,0);
		if (*uuid_found)
			grub_printf (" Volume Name is \"%s\".", uuid_found);
}

static int real_root_func (char *arg, int attempt_mnt);
static int
real_root_func (char *arg, int attempt_mnt)
{
  char *next;
  unsigned int i, tmp_drive = 0;
  unsigned int tmp_partition = 0;
  char ch;

  errnum = 0;
  /* Get the drive and the partition.  */
  if (! *arg || *arg == ' ' || *arg == '\t')
    {
	current_drive = saved_drive;
	current_partition = saved_partition;
	next = 0; /* If ARG is empty, just print the current root device.  */
    }
  else if (grub_memcmp (arg, "endpart", 7) == 0)
    {
	/* find MAX/END partition of the current root drive */
	tmp_partition = saved_partition;
	tmp_drive = saved_drive;

	current_partition = saved_partition;
	current_drive = saved_drive;
	next = arg + 7;
	struct grub_part_data *q;
	for (i=0; i < 16 ; i++)
	{
		q = get_partition_info (current_drive, (i<<16 | 0xffff));
		if (!q)
			continue;
	  if (/* type != PC_SLICE_TYPE_NONE
	      && */ ! IS_PC_SLICE_TYPE_BSD (q->partition_type)
	      && ! IS_PC_SLICE_TYPE_EXTENDED (q->partition_type))
	    {
		saved_partition = current_partition;
		current_partition = q->partition;
		if (attempt_mnt)
		{
		   if (! open_device ())
			current_partition = saved_partition;
		}
	    }

	  /* We want to ignore any error here.  */
	  errnum = ERR_NONE;
	}

	saved_drive = tmp_drive;
	saved_partition = tmp_partition;
	errnum = ERR_NONE;

    }
  else if (grub_memcmp (arg, "bootdev", 7) == 0)
    {
	/* use original boot device */
	current_partition = install_partition;
	current_drive = boot_drive;
	next = arg + 7;
    }
  else
    {
	/* Call set_device to get the drive and the partition in ARG.  */
	if (! (next = set_device (arg)))
	    return 0;
    }
  if (next)
  {
	/* check the length of the root prefix, i.e., NEXT */
	for (i = 0; i < sizeof (saved_dir); i++)
	{
		ch = next[i];
		if (ch == 0 || ch == 0x20 || ch == '\t')
			break;
		if (ch == '\\')
		{
			i++;
			ch = next[i];
			if (! ch || i >= sizeof (saved_dir))
			{
				i--;
				break;
			}
		}
	}

	if (i >= sizeof (saved_dir))
	{
		errnum = ERR_WONT_FIT;
		return 0;
	}

	tmp_partition = current_partition;
	tmp_drive = current_drive;
  }

  errnum = ERR_NONE;

  /* Ignore ERR_FSYS_MOUNT.  */
  if (attempt_mnt)
    {
      if (! open_device () && errnum != ERR_FSYS_MOUNT)
	return 0;

      if (next)
      {
	unsigned long long hdbias = 0;
	char *biasptr;

	/* BSD and chainloading evil hacks !!  */
	biasptr = skip_to (0, next);
	safe_parse_maxint (&biasptr, &hdbias);
	errnum = 0;
	bootdev = set_bootdev (hdbias);
      }
      if (errnum)
	return 0;     
      if (fsys_type != NUM_FSYS || ! next)
        /* Print the type of the filesystem.  */
      {
	    if (! next)
			print_root_device (NULL,0);
		if (! next || debug )
				print_fsys_type ();
      }
      else
	return ! (errnum = ERR_FSYS_MOUNT);
    }
  else if (next)
    {		
      /* This is necessary, because the location of a partition table
	 must be set appropriately.  */
      if (open_partition ())
	{
	  set_bootdev (0);
	  if (errnum)
	    return 0;
	}
    }
  if (next)
  {
	if (kernel_type == KERNEL_TYPE_CHAINLOADER)
	{
	  if (is_io)
	  {
		/* DL=drive, DH=media descriptor: 0xF0=floppy, 0xF8=harddrive */
		chainloader_edx = (tmp_drive & 0xFF) | 0xF000 | ((tmp_drive & 0x80) << 4);
		chainloader_edx_set = 1;

		/* the user might wrongly set these argument, so force them to be correct */

		chainloader_ebx = 0;    // clear BX for WinME
		chainloader_ebx_set = 1;
		chainloader_load_segment = 0x0070;
		chainloader_load_offset = 0;
		chainloader_skip_length = 0x0800;
	  } else {
	    if (chainloader_edx_set)
	    {
		chainloader_edx &= 0xFFFF0000;
		chainloader_edx |= tmp_drive | ((tmp_partition >> 8) & 0xFF00);
	    }

	    if (chainloader_ebx_set && chainloader_ebx)
	    {
		chainloader_ebx &= 0xFFFF0000;
		chainloader_ebx |= tmp_drive | ((tmp_partition >> 8) & 0xFF00);
	    }
	  }
	}

	saved_partition = tmp_partition;
	saved_drive = tmp_drive;
	/* copy root prefix to saved_dir */
	for (i = 0; i < sizeof (saved_dir); i++)
	{
		ch = next[i];
		if (ch == 0 || ch == 0x20 || ch == '\t')
			break;
		if (ch == '\\')
		{
			saved_dir[i] = ch;
			i++;
			ch = next[i];
			if (! ch || i >= sizeof (saved_dir))
			{
				i--;
				saved_dir[i] = 0;
				break;
			}
		}
		saved_dir[i] = ch;
	}

	if (saved_dir[i-1] == '/')
	{
		saved_dir[i-1] = 0;
	} else
		saved_dir[i] = 0;
  }

  if (debug > 0 && *saved_dir)
	grub_printf (" The current working directory (relative path) is %s\n", saved_dir);
	else if (debug && (! *saved_dir) && attempt_mnt)
		print_vol (current_drive);
  /* Clear ERRNUM.  */
  errnum = 0;
  /* If ARG is empty, then return TRUE for harddrive, and FALSE for floppy */
  return next ? 1 : (saved_drive & 0x80);
}

static int root_func (char *arg, int flags);
static int
root_func (char *arg, int flags)
{
  return real_root_func (arg, 1);
}

static struct builtin builtin_root =
{
  "root",
  root_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "root [DEVICE [HDBIAS]]",
  "Set the current \"root device\" to the device DEVICE, then"
  " attempt to mount it to get the partition size (for passing the"
  " partition descriptor in `ES:ESI', used by some chain-loaded"
  " bootloaders), the BSD drive-type (for booting BSD kernels using"
  " their native boot format), and correctly determine "
  " the PC partition where a BSD sub-partition is located. The"
  " optional HDBIAS parameter is a number to tell a BSD kernel"
  " how many BIOS drive numbers are on controllers before the current"
  " one. For example, if there is an IDE disk and a SCSI disk, and your"
  " FreeBSD root partition is on the SCSI disk, then use a `1' for HDBIAS."
};


/* rootnoverify */
static int rootnoverify_func (char *arg, int flags);
static int
rootnoverify_func (char *arg, int flags)
{
  return real_root_func (arg, 0);
}

static struct builtin builtin_rootnoverify =
{
  "rootnoverify",
  rootnoverify_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "rootnoverify [DEVICE [HDBIAS]]",
  "Similar to `root', but don't attempt to mount the partition. This"
  " is useful for when an OS is outside of the area of the disk that"
  " GRUB can read, but setting the correct root device is still"
  " desired. Note that the items mentioned in `root' which"
  " derived from attempting the mount will NOT work correctly."
};


static int time1;
static int time2;

char default_file[60];

static unsigned long long saved_sectors[2];
static unsigned int saved_offsets[2];
static unsigned int saved_lengths[2];
static unsigned long long wait;
static unsigned long long entryno;
static int deny_write;

  /* Save sector information about at most two sectors.  */
static void disk_read_savesect_func1 (unsigned long long sector, unsigned int offset, unsigned long long length);
static void
disk_read_savesect_func1 (unsigned long long sector, unsigned int offset, unsigned long long length)
{
      if (blklst_num_sectors < 2)
	{
	  saved_sectors[blklst_num_sectors] = sector;
	  saved_offsets[blklst_num_sectors] = offset;
	  saved_lengths[blklst_num_sectors] = length;
	}
      blklst_num_sectors++;
}

static void prompt_user (void);
static void
prompt_user (void)
{
	int wait1;
	int c1;
	unsigned short c;
	wait1 = wait;

	printf("\nAbout to write the entry number %d to file %s\n\n"
		"Press Y to allow or N to deny.\n", entryno, default_file);

	/* Get current time.  */
	while ((time1 = getrtsecs ()) == 0xFF);

	for (;;)
	{
	  /* Check if ESC is pressed.  */
		if ((c1 = checkkey ()) != (int)-1 /*&& ASCII_CHAR (getkey ()) == '\e'*/)
	    {
				if ((c = c1) == 0x011b)
	      {
		deny_write = 2;	/* abort this entry */
		break;
	      }
	      if (c == 'Y' || c == 'y')
	      {
		deny_write = -1;
		break;		/* allow the write */
	      }
	      if (c == 'N' || c == 'n')
	      {
		deny_write = 1;
		break;		/* deny the write */
	      }
	      
	      /* any other key restore the wait */
	      wait1 = wait;
	    }

	  if (wait1 != -1
	      && (time1 = getrtsecs ()) != time2
	      && time1 != 0xFF)
	    {
	      if (wait1 == 0)
	      {
		deny_write = 1;
		break;	/* timed out, deny the write */
	      }

	      time2 = time1;
	      wait1--;
	    }
	}

}


/* savedefault */
static int savedefault_func (char *arg, int flags);
static int
savedefault_func (char *arg, int flags)
{
  unsigned int tmp_drive = saved_drive;
  unsigned int tmp_partition = saved_partition;

  char *p;
  errnum = 0;
  blklst_num_sectors = 0;
  wait = 0;
  time2 = -1;
  deny_write = 0;

	if (grub_memcmp (arg, "--wait=", 7) == 0)
	{
		p = arg + 7;
		if (! safe_parse_maxint (&p, &wait))
			return 0;
		arg = skip_to (0, arg);
	}
  
  /* Determine a saved entry number.  */
  if (*arg)
    {
      if (grub_memcmp (arg, "fallback", sizeof ("fallback") - 1) == 0)
	{
	  int i;
	  int index = 0;
	  
	  for (i = 0; i < MAX_FALLBACK_ENTRIES; i++)
	    {
	      if (fallback_entries[i] < 0)
		break;
	      if (fallback_entries[i] == current_entryno)
		{
		  index = i + 1;
		  break;
		}
	    }
	  
	  if (index >= MAX_FALLBACK_ENTRIES || fallback_entries[index] < 0)
	    {
	      /* This is the last.  */
	      errnum = ERR_BAD_ARGUMENT;
	      return 0;
	    }

	  entryno = fallback_entries[index];
	}
   else
   {
		p = arg;
		if (*arg == '-' || *arg == '+')
			++arg;
		if (! safe_parse_maxint (&arg, &entryno))
			return 0;
		if (*p == '-')
			entryno -= current_entryno;
		else if (*p == '+')
			entryno += current_entryno;
	}
    }
  else
    entryno = current_entryno;

  /* Open the default file.  */
  saved_drive = boot_drive;
  saved_partition = install_partition;
  if (grub_open (default_file))
    {
      unsigned int len, len1;
      char buf[12];
      
      if (compressed_file)
	{
	  errnum = ERR_DEFAULT_FILE;
	  goto fail;
	}

      saved_lengths[0] = 0;
      disk_read_hook = disk_read_savesect_func1;
      len = grub_read ((unsigned long long)(grub_size_t)mbr, 512, 0xedde0d90);
      disk_read_hook = 0;
      grub_close ();    
      if (len != ((filemax <= 512) ? filemax : 512))
	{
	  /* Read file failure  */
	  errnum = ERR_READ;
	  goto fail;
	}

      if (len < 180 || filemax > 2048)
	{
	  /* This is too small or too large. Do not modify the file manually!  */
	  errnum = ERR_DEFAULT_FILE;
	  goto fail;
	}

      /* check file content for safety */
      p = mbr;
      while (p < mbr + len - 100 &&
	  grub_memcmp (++p, warning_defaultfile, 73));

      if (p > mbr + len - 160)
	{
	  errnum = ERR_DEFAULT_FILE;
	  goto fail;
	}

      if (blklst_num_sectors > 2 || blklst_num_sectors <= 0 || saved_lengths[0] <= 0 || saved_lengths[0] > SECTOR_SIZE)
	{
	  /* Is this possible?! Too fragmented!  */
	  errnum = ERR_FSYS_CORRUPT;
	  goto fail;
	}
      
      /* Set up a string to be written.  */
      //grub_memset (mbr, '\n', 11);
      len = grub_sprintf (buf, "%d", entryno);
      len++;	/* including the ending null. */
 
	  if (! rawread (current_drive, saved_sectors[0], 0, SECTOR_SIZE, (unsigned long long)(grub_size_t)mbr, 0xedde0d90))
	    goto fail;
	  
	  len1 = saved_lengths[0] < len ? saved_lengths[0] : len;

	  /* it is not good to write a sector frequently, so check if we
	   * can skip the write. */
	  
	  if (grub_memcmp (mbr + saved_offsets[0], buf, len1))
	  {
	    grub_memmove (mbr + saved_offsets[0], buf, len1);

	    /* confirm the write */
	    if (wait && deny_write == 0)
		prompt_user();
	    
	    if (deny_write == 2)
		return 0;

	    if (deny_write <= 0 )
	    {
	      if (! rawwrite (current_drive, saved_sectors[0], (unsigned long long)(grub_size_t)mbr))
		goto fail;
	    }
	  }

	  /* The file is anchored to another file and the first few bytes
	     are spanned in two sectors. Uggh...  */
	  
	  /* only LEN bytes need to be written */
	  if (saved_lengths[0] < len)
	  {
	    /* write the rest bytes to the second sector */
	    if (! rawread (current_drive, saved_sectors[1], 0, 512, (unsigned long long)(grub_size_t)mbr, 0xedde0d90))
		goto fail;
	    
	    /* skip the write if possible. */
	    if (grub_memcmp (mbr + saved_offsets[1],
			buf + saved_lengths[0],
			len - saved_lengths[0]))
	    {
	      grub_memmove (mbr + saved_offsets[1],
			buf + saved_lengths[0],
			len - saved_lengths[0]);
	      
	      if (wait && deny_write == 0)
		prompt_user();
	
	      if (deny_write == 2)
		return 0;

	      if (deny_write <= 0 )
	      {
		if (! rawwrite (current_drive, saved_sectors[1], (unsigned long long)(grub_size_t)mbr))
			goto fail;
	      }
	    }
	  }

      /* Clear the cache.  */
      buf_track = -1;
    }

 fail:
  saved_drive = tmp_drive;
  saved_partition = tmp_partition;

  if (errnum)
  {
	printf_debug0 ("\nError occurred while savedefault.\n");
	/* ignore all errors, but return false. */
	return (errnum = 0);
  }

  return ! errnum;
}

static struct builtin builtin_savedefault =
{
  "savedefault",
  savedefault_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "savedefault [--wait=T] [[+/-]NUM | `fallback']",
  "Save the current entry as the default boot entry if no argument is"
  " specified. If a number is specified, this number is saved. If"
  " `fallback' is used, next fallback entry is saved."
  " If T is not 0, prompt the user to confirm the write operation by"
  " pressing the Y key, and if no key-press detected within T seconds,"
  " the write will be discarded."
};

struct keysym
{
  char *name;			/* the name in unshifted state */
//  unsigned short code;		/* lo=ascii code, hi=scan code */
  unsigned int code;		/* lo=ascii code, hi=scan code */
};

/* The table for key symbols. If the "shifted" member of an entry is
   NULL, the entry does not have shifted state.  */
static struct keysym keysym_table[] =
{
  {"escape",		0x17011B},	// ESC	17011b
  {"1",					0x0031},	// 1
  {"exclam",		0x0021},	// !
  {"2",					0x0032},	// 2
  {"at",				0x0040},	// @
  {"3",					0x0033},	// 3
  {"numbersign",0x0023},	// #
  {"4",					0x0034},	// 4
  {"dollar",		0x0024},	// $
  {"5",					0x0035},	// 5
  {"percent",		0x0025},	// %
  {"6",					0x0036},	// 6
  {"caret",			0x005E},	// ^
  {"7",					0x0037},	// 7
  {"ampersand",	0x0026},	// &
  {"8",					0x0038},	// 8
  {"asterisk",	0x002A},	// *
  {"9",					0x0039},	// 9
  {"parenleft",	0x0028},	// (
  {"0",					0x0030},	// 0
  {"parenright",0x0029},	// )
  {"minus",			0x002D},	// -
  {"underscore",0x005F},	// _
  {"equal",			0x003D},	// =
  {"plus",			0x002B},	// +
  {"backspace",	0x0008},	// BS
  {"ctrlbackspace",	0x2000008},	// 	(DEL)  2000008
  {"tab",				0x0009},	// Tab
  {"q",					0x0071},	// q
  {"Q",					0x0051},	// Q
  {"w",					0x0077},	// w
  {"W",					0x0057},	// W
  {"e",					0x0065},	// e
  {"E",					0x0045},	// E
  {"r",					0x0072},	// r
  {"R",					0x0052},	// R
  {"t",					0x0074},	// t
  {"T",					0x0054},	// T
  {"y",					0x0079},	// y
  {"Y",					0x0059},	// y
  {"u",					0x0075},	// u
  {"U",					0x0055},	// U
  {"i",					0x0069},	// i
  {"I",					0x0049},	// I
  {"o",					0x006F},	// o
  {"O",					0x004F},	// Q
  {"p",					0x0070},	// p
  {"P",					0x0050},	// P
  {"bracketleft",	0x005B},	// [
  {"braceleft",		0x007B},	// {
  {"bracketright",0x005D},	// ]
  {"braceright",	0x007D},	// }
  {"enter",			0x000D},	// Enter
  {"a",					0x0061},	// a
  {"A",					0x0041},	// A
  {"s",					0x0073},	// s
  {"S",					0x0053},	// S
  {"d",					0x0064},	// d
  {"D",					0x0044},	// D
  {"f",					0x0066},	// f
  {"F",					0x0046},	// F
  {"g",					0x0067},	// g
  {"G",					0x0047},	// G
  {"h",					0x0068},	// h
  {"H",					0x0048},	// H
  {"j",					0x006A},	// j
  {"J",					0x004A},	// J
  {"k",					0x006B},	// k
  {"K",					0x004B},	// K
  {"l",					0x006C},	// l
  {"L",					0x004C},	// L
  {"semicolon",	0x003B},	// ;
  {"colon",			0x003A},	// :
  {"quote",			0x0027},	// '
  {"doublequote",	0x0022},	// "
  {"backquote",	0x0060},	// `
  {"tilde",			0x007E},	// ~
  {"backslash",	0x005C},	// "\"
  {"bar",				0x007C},	// |
  {"z",					0x007A},	// z
  {"Z",					0x005A},	// Z
  {"x",					0x0078},	// x
  {"X",					0x0058},	// X
  {"c",					0x0063},	// c
  {"C",					0x0043},	// C
  {"v",					0x0076},	// v
  {"V",					0x0056},	// V
  {"b",					0x0062},	// b
  {"B",					0x0042},	// B
  {"n",					0x006E},	// n
  {"N",					0x004E},	// N
  {"m",					0x006D},	// m
  {"M",					0x004D},	// M
  {"comma",			0x002C},	// ,
  {"less",			0x003C},	// <
  {"period",		0x002E},	// .
  {"greater",		0x003E},	// >
  {"slash",			0x002F},	// /
  {"question",	0x003F},	// ?
  {"space",			0x0020},	// Space
  {"F1",				0xB3B00},	//b3b00
  {"F2",				0xC3C00},	//c3c00
  {"F3",				0xD3D00},	//d3d00
  {"F4",				0xE3E00},	//e3e00
  {"F5",				0xF3F00},	//f3f00
  {"F6",				0x104000},	//104000
  {"F7",				0x114100},	//114100
  {"F8",				0x124200},	//124200
  {"F9",				0x134300},	//134300
  {"F10",				0x144400},	//144400
  /* Caution: do not add NumLock here! we cannot deal with it properly.  */
  {"home",			0x54700},	//54700
  {"uparrow",		0x14800},	//上箭头  14800
  {"pageup",		0x94900},	// PgUp		94900
  {"leftarrow",	0x44B00},	//左箭头 	44b00
  {"rightarrow",0x34D00},	//右箭头  34d00
  {"end",				0x46F00},	//64f00
  {"downarrow",	0x25000},	//下箭头 	25000
  {"pagedown",	0xA5100},	// PgDn		a5100
  {"insert",		0x75200},	// Insert	75200
  {"delete",		0x85300},	// Delete	85300
  {"shiftF1",   0x10B3B00},
  {"shiftF2",   0x10C3C00},
  {"shiftF3",   0x10D3D00},
  {"shiftF4",   0x10E3E00},
  {"shiftF5",   0x10F3F00},
  {"shiftF6",   0x1104000},
  {"shiftF7",   0x1114100},
  {"shiftF8",   0x1124200},
  {"shiftF9",   0x1134300},
  {"shiftF10",  0x1144400},
  {"ctrlF1",		0x20B3B00},
  {"ctrlF2",		0x20C3C00},
  {"ctrlF3",		0x20D3D00},
  {"ctrlF4",		0x20E3E00},
  {"ctrlF5",		0x20F3F00},
  {"ctrlF6",		0x2104000},
  {"ctrlF7",		0x2114100},
  {"ctrlF8",		0x2124200},
  {"ctrlF9",		0x2134300},
  {"ctrlF10",		0x2144400},
  
  {"Aq",        0x4000071},	// A=Alt or AltGr.	Provided by steve.
  {"Aw",        0x4000077},
  {"Ae",        0x4000065},
  {"Ar",        0x4000072},
  {"At",        0x4000074},
  {"Ay",        0x4000079},
  {"Au",        0x4000075},
  {"Ai",        0x4000069},
  {"Ao",        0x400006f},
  {"Ap",        0x4000070},
  {"Aa",        0x4000061},
  {"As",        0x4000073},
  {"Ad",        0x4000064},
  {"Af",        0x4000066},
  {"Ag",        0x4000067},
  {"Ah",        0x4000068},
  {"Aj",        0x400006A},
  {"Ak",        0x400006B},
  {"Al",        0x400006C},
  {"Az",        0x400007A},
  {"Ax",        0x4000078},
  {"Ac",        0x4000063},
  {"Av",        0x4000066},
  {"Ab",        0x4000062},
  {"An",        0x400006E},
  {"Am",        0x400006D},
  {"A1",        0x4000031},
  {"A2",        0x4000032},
  {"A3",        0x4000033},
  {"A4",        0x4000034},
  {"A5",        0x4000035},
  {"A6",        0x4000036},
  {"A7",        0x4000037},
  {"A8",        0x4000038},
  {"A9",        0x4000039},
  {"A0",        0x4000030},
//  {"oem102",    0x565c},
//  {"shiftoem102",   0x567c},
  {"Aminus",        0x400002D},  //-
  {"Aequal",		0x400003D},  //=
  {"Abracketleft",  0x400005B},  //[
  {"Abracketright", 0x400005D},  //]
  {"Asemicolon",    0x400003B},  //;
  {"Aquote",        0x4000027},  //'
  {"Abackquote",    0x4000060},  //`
  {"Abackslash",    0x400005C},  //'\'
  {"Acomma",        0x400002C},  //,
  {"Aperiod",       0x400002E},  //.
  {"Aslash",        0x400002F},  //'/'
};

//static int find_key_code (char *key);
struct key_map ascii_key_map[(KEY_MAP_SIZE + 1) * 4];

int remap_ascii_char (int key);
int
remap_ascii_char (int key)
{
	int i;
	for (i=0; ascii_key_map[i].from_code; i++)
	{
		if (ascii_key_map[i].from_code == key)
			return (ascii_key_map[i].to_code);
	}
	return key;
}

static unsigned int find_ascii_code (char *key);  
static unsigned int
find_ascii_code (char *key)
{
      int i;
      for (i = 0; i < (int)(sizeof (keysym_table) / sizeof (keysym_table[0])); i++)
	{
	  if (grub_strcmp (key, keysym_table[i].name) == 0)
	    return keysym_table[i].code;
	}
      
      return 0;
}
  
static int setkey_func (char *arg, int flags);
static int
setkey_func (char *arg, int flags)
{
  char *to_key, *from_key;
  unsigned int to_code, from_code;
  
  errnum = 0;
  to_key = arg;
  from_key = skip_to (0, to_key);

  if (! *to_key)
    {
      /* If the user specifies no argument, reset the key mappings.  */
      grub_memset (ascii_key_map, 0, KEY_MAP_SIZE * sizeof (unsigned int));

      return 1;
    }
  else if (! *from_key)
    {
      /* The user must specify two arguments or zero argument.  */
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }
  
  nul_terminate (to_key);
  nul_terminate (from_key);
  
  to_code = find_ascii_code (to_key);
  from_code = find_ascii_code (from_key);
  if (! to_code || ! from_code)
    {
	  errnum = ERR_BAD_ARGUMENT;
	  return 0;
    }
  
    {
      int i;
      
      /* Find an empty slot.  */
      for (i = 0; i < KEY_MAP_SIZE; i++)
	{
		if (ascii_key_map[i].from_code == from_code)
	    /* Perhaps the user wants to overwrite the map.  */
	    break;

		if (! ascii_key_map[i].from_code)
	    break;
	}
      
      if (i == KEY_MAP_SIZE)
	{
	  errnum = ERR_WONT_FIT;
	  return 0;
	}
      
      if (to_code == from_code)
	/* If TO is equal to FROM, delete the entry.  */
	grub_memmove ((char *) &ascii_key_map[i],
		      (char *) &ascii_key_map[i + 1],
		      sizeof (unsigned int) * (KEY_MAP_SIZE - i));
      else
	{
	  ascii_key_map[i].from_code = from_code;
	  ascii_key_map[i].to_code = to_code;
	}
    }
      
  return 1;
}

static struct builtin builtin_setkey =
{
  "setkey",
  setkey_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "setkey [NEW_KEY USA_KEY]",
  "Map default USA_KEY to NEW_KEY."
  " Key names: 0-9, A-Z, a-z or escape, exclam, at, numbersign, dollar,"			//Provided by steve.
  " percent, caret, ampersand, asterisk, parenleft, parenright, minus,"
  " underscore, equal, plus, backspace, tab, bracketleft, braceleft,"
  " bracketright, braceright, enter, semicolon, colon, quote, doublequote,"
  " backquote, tilde, backslash, bar, comma, less, period, greater,"
  " slash, question, space, delete"
  " [ctrl|shift]F1-10. For Alt+ prefix with A, e.g. 'setkey at Aequal'."
  " Use 'setkey at at' to reset one key, 'setkey' to reset all keys."
};


//#if defined(SUPPORT_SERIAL) || defined(SUPPORT_HERCULES) || defined(SUPPORT_GRAPHICS)
#if defined(SUPPORT_GRAPHICS)
/* terminal */
static int terminal_func (char *arg, int flags);
static int
terminal_func (char *arg, int flags)
{
  /* The index of the default terminal in TERM_TABLE.  */
  int default_term = -1;
  struct term_entry *prev_term = current_term;
  unsigned long long to = -1;
  unsigned long long lines = 0;
  int no_message = 0;
  unsigned int term_flags = 0;
  /* XXX: Assume less than 32 terminals.  */
  unsigned int term_bitmap = 0;

  errnum = 0;
  /* Get GNU-style long options.  */
  while (1)
    {
      if (grub_memcmp (arg, "--dumb", sizeof ("--dumb") - 1) == 0)
	term_flags |= TERM_DUMB;
      else if (grub_memcmp (arg, "--no-echo", sizeof ("--no-echo") - 1) == 0)
	/* ``--no-echo'' implies ``--no-edit''.  */
	term_flags |= (TERM_NO_ECHO | TERM_NO_EDIT);
      else if (grub_memcmp (arg, "--no-edit", sizeof ("--no-edit") - 1) == 0)
	term_flags |= TERM_NO_EDIT;
      else if (grub_memcmp (arg, "--timeout=", sizeof ("--timeout=") - 1) == 0)
	{
	  char *val = arg + sizeof ("--timeout=") - 1;
	  
	  if (! safe_parse_maxint (&val, &to))
	    return 0;
	}
      else if (grub_memcmp (arg, "--lines=", sizeof ("--lines=") - 1) == 0)
	{
	  char *val = arg + sizeof ("--lines=") - 1;

	  if (! safe_parse_maxint (&val, &lines))
	    return 0;

	  /* Probably less than four is meaningless....  */
	  if (lines < 4)
	    {
	      errnum = ERR_BAD_ARGUMENT;
	      return 0;
	    }
	}
      else if (grub_memcmp (arg, "--silent", sizeof ("--silent") - 1) == 0)
	no_message = 1;
#ifdef SUPPORT_GRAPHICS
      else if (grub_memcmp (arg, "--font-spacing=", 15) == 0)
      {
		arg += 15;
		if (! safe_parse_maxint (&arg, &lines))
			return 0;
		font_spacing = (unsigned char)lines;
		menu_font_spacing = (unsigned char)lines;
		if (*arg++ == ':')
		{
			if (! safe_parse_maxint (&arg, &lines))
				return 0;
			line_spacing = (unsigned char)lines;
			menu_line_spacing = (unsigned char)lines;
		}
		if (graphics_inited && graphics_mode > 0xFF)
		{
			console_setcursor(0);
			current_term->shutdown();
			current_term = term_table + 1;
			current_term->startup();
		}
		return 1;
      }
#endif
      else
	break;

      arg = skip_to (0, arg);
    }
  
  /* If no argument is specified, show current setting.  */
  if (! *arg)
    {
      printf_debug0 ("%s%s%s%s\nchars_per_line=%d  max_lines=%d",
		   current_term->name,
		   (current_term->flags & TERM_DUMB ? " (dumb)" : ""),
		   (current_term->flags & TERM_NO_EDIT ? " (no edit)" : ""),
		   (current_term->flags & TERM_NO_ECHO ? " (no echo)" : ""),
		    current_term->chars_per_line,current_term->max_lines);
      return 1;
    }

  while (*arg)
    {
      int i;
      char *next = skip_to (0, arg);
      
      nul_terminate (arg);

      for (i = 0; term_table[i].name; i++)
	{
	  if (grub_strcmp (arg, term_table[i].name) == 0)
	    {
	      if (term_table[i].flags & TERM_NEED_INIT)
		{
		  errnum = ERR_DEV_NEED_INIT;
		  return 1;
		}
	      
	      if (default_term < 0)
		default_term = i;

	      term_bitmap |= (1 << i);
	      break;
	    }
	}

      if (! term_table[i].name)
	{
	  errnum = ERR_BAD_ARGUMENT;
	  return 0;
	}

      arg = next;
    }

  /* If multiple terminals are specified, wait until the user pushes any
     key on one of the terminals.  */
  if (term_bitmap & ~(1 << default_term))
    {
      time2 = -1;

      /* XXX: Disable the pager.  */
      count_lines = -1;
      
      /* Get current time.  */
      while ((time1 = getrtsecs ()) == 0xFF)
	;

      /* Wait for a key input.  */
      while (to)
	{
	  int i;

	  for (i = 0; term_table[i].name; i++)
	    {
	      if (term_bitmap & (1 << i))
		{
		  if (term_table[i].checkkey () >= 0)
		    {
		      default_term = i;
		      
		      goto end;
		    }
		}
	    }
	  
	  /* Prompt the user, once per sec.  */
	  if ((time1 = getrtsecs ()) != time2 && time1 != 0xFF)
	    {
	      if (! no_message)
		{
		  /* Need to set CURRENT_TERM to each of selected
		     terminals.  */
		  for (i = 0; term_table[i].name; i++)
		    if (term_bitmap & (1 << i))
		      {
			current_term = term_table + i;
			grub_printf ("\rPress any key to continue.\n");
		      }
		  
		  /* Restore CURRENT_TERM.  */
		  current_term = prev_term;
		}
	      
	      time2 = time1;
	      if (to > 0)
		to--;
	    }
	}
    }

 end:
  current_term = term_table + default_term;
  current_term->flags = term_flags;
  
  /* If the interface is currently the command-line,
     restart it to repaint the screen.  */
  if (current_term != prev_term /*&& (flags & BUILTIN_CMDLINE)*/)
  {
    if (prev_term->shutdown)
      prev_term->shutdown();
    if (grub_memcmp (current_term->name, "console", 7) == 0)
    {
      current_term->chars_per_line = 80;
      current_term->max_lines = 25;
      setcursor (1);
    }

    if (current_term->startup)
      current_term->startup();
    cls();
  }
  
  return 1;
}

static struct builtin builtin_terminal =
{
  "terminal",
  terminal_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "terminal [--dumb] [--no-echo] [--no-edit] [--timeout=SECS]\n [--lines=LINES] [--silent] [console] [graphics]",
  "Select a terminal. When multiple terminals are specified, wait until"
  " you push any key to continue. If both console and serial are specified,"
  " the terminal to which you input a key first will be selected. If no"
  " argument is specified, print current setting. The option --dumb"
  " specifies that your terminal is dumb, otherwise, vt100-compatibility"
  " is assumed. If you specify --no-echo, input characters won't be echoed."
  " If you specify --no-edit, the BASH-like editing feature will be disabled."
  " If --timeout is present, this command will wait at most for SECS"
  " seconds. The option --lines specifies the maximum number of lines."
  " The option --silent is used to suppress messages."
};
#endif /* SUPPORT_SERIAL || SUPPORT_HERCULES || SUPPORT_GRAPHICS */

static inline unsigned short * vbe_far_ptr_to_linear (unsigned int ptr);
static inline unsigned short * vbe_far_ptr_to_linear (unsigned int ptr)
{
    unsigned int seg = (ptr >> 16);
    unsigned short off = (unsigned short)(ptr /* & 0xFFFF */);

    return (unsigned short *)(grub_size_t)((seg << 4) + off);
}
  
/* timeout */
static int timeout_func (char *arg, int flags);
static int
timeout_func (char *arg, int flags)
{
  unsigned long long ull;
  errnum = 0;

  if (! safe_parse_maxint (&arg, &ull))
    return 0;
	if ((int)ull > 99)
		ull = 99;
  grub_timeout = ull;

  return 1;
}

static struct builtin builtin_timeout =
{
  "timeout",
  timeout_func,
  BUILTIN_MENU,
};

static int iftitle_func (char *arg, int flags);
static int
iftitle_func (char *arg, int flags)
{
	char *p = arg;
	errnum = 0;
	if (*p != '[')
		return 0;
	char *cmd = ++p;
	while (*p && *p != ']')
		++p;
	if (*p != ']')
		return 0;
	*p++ = 0;
	if (!run_line(cmd,BUILTIN_IFTITLE))
		return 0;
	return (int)(p - arg);
}

struct builtin builtin_iftitle =
{
  "iftitle",
  iftitle_func,
  0/*BUILTIN_TITLE*/,
};

struct builtin builtin_title =
{
  "title",
  NULL/*title_func*/,
  0/*BUILTIN_TITLE*/,
  "title [[$[0xRRGGBB]]TITLE] [\\n[$[0xRRGGBB]]NOTE]",
  "$[0xRRGGBB] Sets the color for TITLE and NOTE.\n"
  "$[] Restore COLOR STATE STANDARD."
};


//extern int tpm_init(void);
/* tpm */
static int tpm_func (char *arg, int flags);
static int
tpm_func (char *arg, int flags)
{
  errnum = 0;
  for (;;)
  {
    if (grub_memcmp (arg, "--init", 6) == 0)
      {
	//return tpm_init();
      }
    else
      return ! (errnum = ERR_BAD_ARGUMENT);
    arg = skip_to (0, arg);
  }
  
  return 1;
}

static struct builtin builtin_tpm =
{
  "tpm",
  tpm_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "tpm --init",
  "Initialise TPM."
};

int builtin_cmd (char *cmd, char *arg, int flags);
int
builtin_cmd (char *cmd, char *arg, int flags)
{
	struct builtin *builtin1 = 0;

	if (cmd == NULL)
	{
		return run_line (arg, flags);
	}

	if (substring(cmd,"exec",1) == 0)
		return command_func(arg, flags);

	builtin1 = find_command (cmd);

	if ((grub_size_t)builtin1 != (grub_size_t)-1)
	{
		if (! builtin1 || ! (builtin1->flags & flags))
		{
			errnum = ERR_UNRECOGNIZED;
			return 0;
		}
		else
		{
			return (builtin1->func) (arg, flags);
		}
	}
	else
		return command_func (cmd, flags);
}

//call_func,s_calc,set_func,calc_func使用
static int read_val(char **str_ptr,long long *val)  //读值
{
      char *p;
      char *arg = *str_ptr;
      while (*arg == ' ' || *arg == '\t') arg++;
      p = arg;
      if (*arg == '*') arg++; //后面紧随内存地址
      if (! safe_parse_maxint_with_suffix (&arg,(unsigned long long *)(grub_size_t)val, 0))
      {
	 return 0;
      }
      
      if (*p == '*')  //取内存的值
      {
    if ((unsigned long long)*val >= 0x8200 && (unsigned long long)*val < 0x8400)
      *val = *((unsigned long long *)(grub_size_t)IMG((grub_size_t)*val));
    else
      *val = *((unsigned long long *)(grub_size_t)*val);
      }
      
      while (*arg == ' ' || *arg == '\t') arg++;
      *str_ptr = arg;
      return 1;
}

long long return_value64; 
static long long s_calc (char *arg, int flags);
static long long
s_calc (char *arg, int flags)
{
   long long val1 = 0;
   long long val2 = 0;
   long long *p_result = &val1;
   char O;
   int mem=0;
#if defined(__i386__)
   unsigned long long r;
#endif
   return_value64 = 0;
   
  errnum = 0;
  if (grub_memcmp (arg, "--mem", 5) == 0)
  {
    arg += 5;
    mem=1;
  }
   if (*arg == '*') //后面紧随内存地址
   {
      arg++;
      if (! safe_parse_maxint_with_suffix (&arg, (unsigned long long*)(grub_size_t)&val1, 0))
      {
	 return 0;
      }

    if (!mem && (unsigned long long)val1 >= 0x8200 && (unsigned long long)val1 < 0x8400)
      p_result = (long long *)(grub_size_t)IMG((int)val1);  //取内存的值
    else
      p_result = (long long *)(grub_size_t)val1;

    val1 = *p_result;
    while (*arg == ' ') arg++;
   }
   else
   {
      if (!read_val(&arg, &val1))
      {
	 return 0;
      }
   }

   if ((arg[0] == arg[1]) && (arg[0] == '+' || arg[0] == '-'))
   {
      if (arg[0] == '+')
         (*p_result)++;
      else
         (*p_result)--;
      arg += 2;
      while (*arg == ' ') arg++;
   }

   if (*arg == '=')
   {
      arg++;
      if (! read_val(&arg, &val1))
	 return 0;
   }
   else if (p_result != &val1)
   {
      p_result = &val1;
   }

   while (*arg)
   {
      val2 = 0ULL;
      O = *arg;
      arg++;

      if (O == '>' || O == '<')
      {
	 if (*arg != O)
		 return 0;
	 arg++;
      }
      
      if (! read_val(&arg, &val2))
	 return 0;

      switch(O)
      {
	 case '+':
		 val1 += val2;
		 break;
	 case '-':
		 val1 -= val2;
		 break;
	 case '*':
		 val1 *= val2;
		 break;
	 case '/':
		 if (val2 == 0)
 			return !(errnum = ERR_DIVISION_BY_ZERO);
#if defined(__i386__)
		 val1 = (long long)grub_divmod64 (val1, val2, 0);
#else
		 val1 = val1 / val2;
#endif
 		 break;
	 case '%':
		 if (val2 == 0)
 			return !(errnum = ERR_DIVISION_BY_ZERO);
#if defined(__i386__)
		 grub_divmod64 (val1, val2, &r);
		 val1 = (long long)r;
#else
		 val1 = val1 % val2;
#endif
		 break;
	 case '&':
		 val1 &= val2;
		 break;
	 case '|':
		 val1 |= val2;
		 break;
	 case '^':
		 val1 ^= val2;
		 break;
	 case '<':
		 val1 <<= val2;
		 break;
	 case '>':
		 val1 >>= val2;
		 break;
	 default:
		 return 0;
      }
   }
   
//   printf_debug0(" %ld (HEX:0x%lX)\n",val1,val1);
//   printf_debug0(" %d (HEX:0x%X)\n",(int)val1,(int)val1);
  printf_debug0(" %d (HEX:0x%X)\n",val1,val1);
	if (p_result != &val1)
	   *p_result = val1;
   return_value64 = val1;
   return val1;
}

static int calc_func (char *arg, int flags);
static int
calc_func (char *arg, int flags)
{
    return (int)s_calc(arg,flags);
}

static struct builtin builtin_calc =
{
  "calc",
  calc_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "calc [*INTEGER=] [*]INTEGER OPERATOR [[*]INTEGER]",
  "GRUB4DOS Simple Calculator.\n"
  "Available Operators: + - * / % << >> ^ & |"
  "\nNote: 1.this is a Simple Calculator and From left to right only."
  "\n      2.'^' is XOR function."
  "\n      3.operators '| % >>' are command operator,can not have space on both sides"
};


int FindHighestSetBit (grub_uint32_t mask);
int
FindHighestSetBit (grub_uint32_t mask)
{
	int bit = 0;
	while (mask)
	{
		mask >>= 8;
		bit += 8;
	}
	return bit;
}

int max (grub_uint32_t bit1, grub_uint32_t bit2);
int
max (grub_uint32_t bit1, grub_uint32_t bit2)
{
	if (bit1 > bit2)
		return bit1;
	else
		return bit2;
}

int GetBitsPerPixel (struct grub_efi_gop_pixel_bitmask *PixelBits);
int
GetBitsPerPixel (struct grub_efi_gop_pixel_bitmask *PixelBits)	//获得像素元素尺寸(像素位掩模)
{
	int HighestPixel = -1;	//最高像素
	int BluePixel;					//兰像素
	int RedPixel;						//红像素
	int GreenPixel;					//绿像素
	int RsvdPixel;					//保留像素

	RedPixel = FindHighestSetBit (PixelBits->r);		//ff0000	查找最高点设置位(像素位掩模->兰掩模) 
	GreenPixel = FindHighestSetBit (PixelBits->g);	//ff00
	BluePixel = FindHighestSetBit (PixelBits->b);		//ff
	RsvdPixel = FindHighestSetBit (PixelBits->a);		//0
	HighestPixel = max (RedPixel, GreenPixel);			//ff0000
	HighestPixel = max (HighestPixel, BluePixel);		//ff0000
	HighestPixel = max (HighestPixel, RsvdPixel);		//ff0000
	return HighestPixel;
}

static void grub_video_gop_get_bitmask (grub_uint32_t mask, unsigned int *mask_size, unsigned int *field_pos);
static void
grub_video_gop_get_bitmask (grub_uint32_t mask, unsigned int *mask_size,
			    unsigned int *field_pos)	//gop视频获得位掩码
{
  int i;
  int last_p;
  for (i = 31; i >= 0; i--)
    if (mask & (1 << i))
      break;
  if (i == -1)
	{
		*mask_size = *field_pos = 0;
		return;
	}
  last_p = i;
  for (; i >= 0; i--)
    if (!(mask & (1 << i)))
      break;
  *field_pos = i + 1;
  *mask_size = last_p - *field_pos + 1;
}

//0x20-0x7f及0x2191、0x2193(上下箭头)的16*16点阵字符。hex格式，lzma压缩。
static char mini_font_lzma[] = {
0x5D,0x00,0x00,0x80,0x00,0xAC,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x19,0x0C,
0x43,0x90,0x39,0x67,0xD5,0x47,0x76,0xE2,0x4A,0xFC,0x16,0xF6,0xCE,0x1E,0x52,0x1A,
0x03,0x6A,0x5B,0x7B,0xE2,0x81,0x9A,0x24,0xEB,0x83,0x3C,0xB7,0xB8,0x17,0xFB,0x21,
0x07,0xC9,0x9E,0x94,0x4F,0x3D,0x72,0x38,0x16,0x78,0xFA,0x10,0xA1,0xD2,0xD6,0x90,
0x17,0x41,0xAE,0xBD,0x64,0x8B,0x23,0xD6,0xD0,0x09,0x00,0xEB,0x23,0x18,0x03,0x34,
0x37,0x29,0x24,0xF7,0x38,0x0D,0x5F,0xE7,0x42,0x03,0xCA,0x49,0xE8,0x5D,0xB7,0x8D,
0x76,0xAC,0x54,0x6D,0x3F,0x63,0x7E,0xE6,0x21,0xEA,0x9C,0x69,0x77,0xB2,0x8C,0x52,
0xE7,0xB6,0x68,0xFD,0x2B,0xBC,0x06,0x00,0xB6,0xB1,0xE9,0x72,0xF6,0xB3,0x88,0x40,
0xC2,0x22,0xAC,0xFF,0x50,0xFB,0x39,0xCC,0x0A,0x08,0x13,0xE0,0xE0,0x78,0xD1,0x7F,
0xAB,0xFB,0xFF,0x9B,0x2C,0x49,0xA4,0xEF,0x71,0xE8,0xBE,0x43,0x41,0x8E,0x2B,0x6A,
0xCA,0x7C,0x9F,0xF1,0xE0,0x11,0x91,0xE0,0xC6,0x38,0x0B,0xA4,0x9A,0xAE,0x5C,0x93,
0x86,0xB2,0x35,0x77,0x20,0xED,0xD7,0x53,0x2B,0xE3,0xD0,0xFA,0x20,0x2E,0xE4,0x4E,
0xF0,0x32,0xBE,0x05,0x40,0x96,0x94,0x24,0xF3,0x65,0x5F,0x73,0x07,0xF6,0x1A,0xEF,
0xF9,0x47,0xBF,0x45,0x7B,0x21,0x6C,0x59,0xD6,0x77,0xD6,0xF5,0x8F,0x4F,0x66,0xE0,
0x7E,0xF5,0x28,0x7F,0x73,0x88,0xD4,0xE1,0x76,0x7C,0xA3,0x54,0x09,0xF3,0x20,0xF5,
0x63,0xFB,0x90,0x18,0xF8,0xDB,0x09,0x1A,0xE7,0x8E,0x46,0x88,0xF6,0x6C,0x75,0x47,
0x5E,0x4A,0x79,0x02,0xFE,0x25,0x0A,0x6A,0x10,0x2B,0xB8,0x99,0x97,0x69,0x13,0x7D,
0x1D,0x59,0x02,0x06,0x8C,0x21,0x2A,0x62,0x4C,0x34,0x8E,0x13,0xCC,0x4A,0x37,0xD3,
0x95,0x02,0x90,0x45,0xE6,0xE6,0x35,0xA1,0x9D,0x1C,0x2C,0xAD,0xDD,0xCA,0xAA,0x5A,
0xC5,0xC9,0xBE,0xA6,0x83,0xC6,0xB7,0xB1,0x18,0x0C,0x7F,0x3F,0x7C,0x70,0xEF,0x07,
0x2F,0x24,0xE4,0x90,0x33,0xDF,0x29,0xDC,0x72,0x5B,0xCC,0x48,0xF6,0x7B,0x62,0x61,
0xA9,0xB3,0x90,0x9E,0x17,0xBA,0x44,0x93,0x5F,0x29,0x5D,0x42,0xBC,0x18,0x82,0x55,
0x9E,0xDA,0xF0,0xAC,0xA0,0xAC,0x1A,0xC0,0x82,0x16,0x3D,0x62,0x72,0xFE,0x90,0x0E,
0x21,0x7E,0x89,0x6A,0x23,0x1D,0x89,0xE6,0x83,0x00,0x43,0x86,0xF7,0x0C,0x50,0x2D,
0xBF,0xDC,0x70,0xB7,0xFC,0x9E,0x7C,0x8E,0x1E,0xA2,0x38,0xDF,0xCD,0x09,0x2B,0xAE,
0x16,0x7A,0xB7,0xAC,0x9B,0xF4,0xDE,0xA1,0xE7,0xB2,0x13,0x3A,0x88,0x21,0x9E,0xBE,
0x28,0x97,0x5B,0xAC,0x83,0xB0,0x00,0x48,0xC9,0x62,0x7F,0x07,0x21,0x2E,0x39,0xF3,
0x67,0x8A,0x8E,0x70,0xB2,0x57,0x7B,0x34,0x1B,0xC9,0xF3,0x05,0x13,0xAB,0x43,0x4B,
0x03,0xCB,0xC0,0x98,0xFD,0x5A,0x17,0xD4,0x7B,0xAA,0xEE,0x69,0xFC,0x94,0x52,0x4C,
0x3A,0x5A,0x75,0x3A,0xC6,0x6B,0xB6,0x57,0x62,0xE8,0xCC,0xF1,0xC9,0xA6,0xE9,0x0A,
0x92,0x09,0x39,0x8F,0x55,0x15,0x2C,0x87,0x41,0x4F,0xFE,0xE8,0xDF,0xDD,0x62,0x30,
0x30,0xBA,0x0F,0x8F,0xAA,0xF9,0x44,0x0E,0xAC,0x5A,0x52,0x0D,0x3A,0x2E,0x5F,0x44,
0x47,0xC5,0x0D,0x01,0x16,0x7D,0xA5,0x55,0xCE,0x29,0x4A,0xF2,0x27,0xE3,0x54,0x91,
0xE8,0x1C,0x30,0x8C,0x48,0x7C,0x1D,0x78,0xB3,0x3A,0xF7,0xEB,0xB2,0xC9,0x8B,0xA1,
0xE5,0x18,0x68,0xA3,0xCD,0x51,0x3B,0x52,0x60,0xF0,0xC6,0xD1,0x49,0xE8,0x1E,0xC7,
0xE4,0x6C,0x36,0x71,0x14,0x07,0x9C,0xE9,0xE4,0x33,0xE4,0x44,0x1D,0xB6,0x78,0x95,
0x6E,0x68,0x79,0xAF,0x88,0x2C,0xCC,0x56,0xC0,0x00,0x93,0xFF,0x8B,0x5B,0xB7,0x29,
0xE4,0xEA,0x79,0x18,0xAE,0xA8,0xD1,0x9D,0x1F,0xBC,0x35,0x51,0x3F,0x05,0xB7,0x9E,
0x05,0xE3,0x21,0xC5,0xB9,0x69,0xC3,0x24,0x9D,0x33,0x9C,0xEB,0x28,0xCF,0x0E,0x63,
0x02,0x32,0xFE,0x80,0x53,0xB7,0x22,0x0C,0xCD,0x07,0x57,0x0F,0x5B,0x0C,0xD2,0x31,
0xEA,0x53,0x70,0xF5,0xD5,0x2F,0x99,0x4E,0xA3,0xA6,0x36,0x6D,0x25,0x36,0x28,0x1E,
0x49,0x74,0xC8,0x51,0xB9,0x0F,0x9B,0x22,0xF9,0xD8,0x27,0x9E,0xC6,0xC3,0x68,0x95,
0x84,0x67,0x87,0xF2,0x95,0x0D,0x48,0xF4,0x96,0x31,0x61,0x68,0xDD,0x4A,0xEC,0x40,
0x30,0x03,0x21,0x6D,0x58,0xC8,0x94,0x57,0x91,0x30,0x79,0x9C,0x28,0xB8,0x1E,0x04,
0xBB,0x86,0xA8,0x3C,0x50,0xD0,0x5A,0xD7,0x38,0x15,0xC5,0x92,0xD6,0xE5,0xB6,0xE6,
0x4D,0x8B,0xE4,0xC9,0xFB,0xA3,0xF5,0x27,0x21,0x9C,0x9B,0xBF,0x6D,0x11,0xD3,0x04,
0x50,0x98,0xA5,0xD3,0x3F,0x95,0x5C,0x32,0x05,0xC9,0xD9,0xEA,0xED,0xF7,0x0A,0x31,
0xD9,0x51,0xC5,0x46,0xD0,0xB8,0x67,0x58,0x80,0x06,0x8E,0x75,0x39,0x33,0x21,0x24,
0x68,0x25,0x46,0xBE,0x09,0xCC,0x4F,0x0B,0x30,0x3C,0x04,0xD5,0x6A,0x54,0xD3,0xB8,
0x0E,0x52,0x7D,0xD6,0x80,0x08,0xC8,0x71,0x06,0x3E,0xAF,0xC7,0xAA,0xF7,0x2D,0x76,
0x9E,0x42,0x0F,0x43,0x63,0xBC,0x5B,0x20,0x27,0xE6,0xB7,0xBA,0xA3,0xD8,0xD4,0xF1,
0x5A,0xE2,0x84,0xE7
};

/* graphicsmode */
int graphicsmode_func (char *arg, int flags);
int
graphicsmode_func (char *arg, int flags)
{
#ifdef SUPPORT_GRAPHICS
  unsigned long long tmp_graphicsmode = -1;
  char *x_restrict = "0:-1";
  char *y_restrict = "0:-1";
  char *z_restrict = "0:-1";
	unsigned int x = 0, x0 = 0; /* x_resolution */
	unsigned int y = 0, y0 = 0; /* y_resolution */
	unsigned int z = 0, z0 = 0; /* bits_per_pixel */
  grub_efi_handle_t *handles;
  grub_efi_uintn_t num_handles;
	unsigned int bytes_per_scanline=0, bits_per_pixel, bytes_per_scanline0=0;
	static struct grub_efi_gop *gop;
	unsigned int red_mask_size, green_mask_size, blue_mask_size, reserved_mask_size;
  unsigned int red_field_pos, green_field_pos, blue_field_pos, reserved_field_pos;
	int mode, test = 0;
	static grub_efi_guid_t graphics_output_guid = GRUB_EFI_GOP_GUID;
  grub_efi_status_t status;

  errnum = 0;
  if (! *arg)
  {
		printf ("The current graphic mode is %d.\n",graphics_mode);
		return graphics_mode;
  }

  if (grub_memcmp (arg, "--info", 6) == 0)
  {
		tmp_graphicsmode = 0x2ff;
  }
  else if (grub_memcmp (arg, "--test", 6) == 0)
  {
		test = 1;
		arg += 6;
		goto transfer;
  }
  else if (safe_parse_maxint (&arg, &tmp_graphicsmode))
  {
		if ((unsigned int)tmp_graphicsmode == (unsigned int)-1) /* mode auto detect */
		{
			unsigned long long tmp_ll;
			char *tmp_arg;
transfer:
			tmp_arg = arg = wee_skip_to (arg, 0);
			if (! *arg)   //只有 -1
				goto xyz_done;
			if (! safe_parse_maxint (&arg, &tmp_ll))
				goto bad_arg;
			if (tmp_ll != -1ULL || (unsigned char)*arg > ' ')
				x_restrict = tmp_arg;

			tmp_arg = arg = wee_skip_to (arg, 0);
			if (! *arg)
				goto xyz_done;  //只有 -1 x
			if (! safe_parse_maxint (&arg, &tmp_ll))
				goto bad_arg;
			if (tmp_ll != -1ULL || (unsigned char)*arg > ' ')
				y_restrict = tmp_arg;

			tmp_arg = arg = wee_skip_to (arg, 0);
			if (! *arg)
				goto xyz_done;  //只有 -1 x y
			if (! safe_parse_maxint (&arg, &tmp_ll))
				goto bad_arg;
			if (tmp_ll != -1ULL || (unsigned char)*arg > ' ')
				z_restrict = tmp_arg; //全部 -1 x y z
		}//if ((unsigned long)tmp_graphicsmode == -1)
	}//lse if (!safe_parse_maxint (&arg, &tmp_graphicsmode))
	else
		goto bad_arg;
xyz_done:

  handles = grub_efi_locate_handle (GRUB_EFI_BY_PROTOCOL,
				    &graphics_output_guid, NULL, &num_handles);	//定位手柄(通过协议,
  if (!handles || num_handles == 0)	//如果句柄为零, 或者句柄数为零
    return 0;												//错误
//  for (i = 0; i < num_handles; i++)
//	{
//		gop_handle = handles[0];	//句柄
		gop = grub_efi_open_protocol (handles[0], &graphics_output_guid,
				    GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);	//打开协议
		if (tmp_graphicsmode == 0x2ff)
		{
			printf_debug0 ("GOP Video current_mode=%d, physical_address=0x%lx, size=0x%x\n",graphics_mode,gop->mode->fb_base,gop->mode->fb_size);//80000000,1d4c00;
			printf_debug0 ("Mode  Format  Resolution  BitPixel  RGBASize   RGBAPos   PixelsScanline\n"); 
		}
		for (mode = gop->mode->max_mode - 1; mode >= 0; mode--)
		{
			grub_efi_uintn_t size1;
			struct grub_efi_gop_mode_info *info = NULL;
			status = efi_call_4 (gop->query_mode, gop, mode, &size1, &info);	//gop查询模式  通过mode查询, 返回模式尺寸及信息

			if (status)	//失败
				goto bad_arg;
			switch (info->pixel_format)
			{
				case GRUB_EFI_GOT_RGBA8:
					red_mask_size = 8;
					red_field_pos = 0;
					green_mask_size = 8;
					green_field_pos = 8;
					blue_mask_size = 8;
					blue_field_pos = 16;
					reserved_mask_size = 8;
					reserved_field_pos = 24;
					bits_per_pixel = sizeof (struct grub_efi_gop_blt_pixel) << 3;
					break;

				case GRUB_EFI_GOT_BGRA8:
					red_mask_size = 8;
					red_field_pos = 16;
					green_mask_size = 8;
					green_field_pos = 8;
					blue_mask_size = 8;
					blue_field_pos = 0;
					reserved_mask_size = 8;
					reserved_field_pos = 24;
					bits_per_pixel = sizeof (struct grub_efi_gop_blt_pixel) << 3;
					break;

				case GRUB_EFI_GOT_BITMASK:
					grub_video_gop_get_bitmask (info->pixel_bitmask.r, &red_mask_size,  &red_field_pos);
					grub_video_gop_get_bitmask (info->pixel_bitmask.g, &green_mask_size, &green_field_pos);
					grub_video_gop_get_bitmask (info->pixel_bitmask.b, &blue_mask_size, &blue_field_pos);
					grub_video_gop_get_bitmask (info->pixel_bitmask.a, &reserved_mask_size, &reserved_field_pos);
					bits_per_pixel = GetBitsPerPixel (&info->pixel_bitmask);	//输出信息->像素位掩模
					break;

				default:
					return printf_errinfo ("unsupported video mode");
			}
			
			if (tmp_graphicsmode == 0x2ff)
			{
				printf_debug0 ("%d     %d     %4u x %-4u    %u     %u:%u:%u:%u  %02u:%02u:%02u:%02u      %u\n",
						mode | 0x100,info->pixel_format,info->width,info->height, bits_per_pixel, red_mask_size, green_mask_size, blue_mask_size,
						reserved_mask_size, red_field_pos, green_field_pos, blue_field_pos, reserved_field_pos, info->pixels_per_scanline);
			}

#define _X_ ((unsigned int)info->width)
#define _Y_ ((unsigned int)info->height)
#define _Z_ ((unsigned int)bits_per_pixel)
				/* ok, find out one valid mode. */
			if ((tmp_graphicsmode & 0xff) == (unsigned long long)mode) /* the specified mode */
			{
				x = _X_;
				y = _Y_;
				z = _Z_;
				bytes_per_scanline = info->pixels_per_scanline * (bits_per_pixel >> 3);
//				status = efi_call_2 (gop->set_mode, gop, mode);	//gop设置模式				
//				if (status)	//失败
//					goto bad_arg;
				break;
	    }
	    else if ((unsigned int)tmp_graphicsmode == (unsigned int)-1 /* mode auto detect */
					&& x * y * z <  _X_ * _Y_ * _Z_
					&& in_range (x_restrict, _X_)
					&& in_range (y_restrict, _Y_)
					&& in_range (z_restrict, _Z_))
	    {
				x = _X_;
				y = _Y_;
				z = _Z_;
				bytes_per_scanline = info->pixels_per_scanline * (bits_per_pixel >> 3);
//				status = efi_call_2 (gop->set_mode, gop, mode);	//gop设置模式
//				if (status)	//失败
//					goto bad_arg;
				tmp_graphicsmode = 0x100 | mode;
				break;
	    }
      else if (mode == 0)
      {
				x0 = _X_;
				y0 = _Y_;
        z0 = _Z_; 
				bytes_per_scanline0 = info->pixels_per_scanline * (bits_per_pixel >> 3);
      }
		} //for (mode = 0; mode < gop->mode->max_mode; mode++)
//	}//for (i = 0; i < num_handles; i

#undef _X_
#undef _Y_
#undef _Z_

  if ((unsigned int)tmp_graphicsmode == (unsigned int)-1)
  {
		if (test)
			return 0;
    tmp_graphicsmode = 0x100;
    x = x0;
    y = y0;
    z = z0;
    bytes_per_scanline = bytes_per_scanline0;
  }

	if (tmp_graphicsmode == 0x2ff)
	{
//		return graphics_mode;
    goto ok;
	}
	else if (tmp_graphicsmode == 3 && graphics_mode != 3)	//在控制台设置模式3会死机。
	{
    cls();
    graphics_end ();
    current_term->chars_per_line = 80;
    current_term->max_lines = 25;
    graphics_mode = tmp_graphicsmode;
    goto ok;
	}
	else if (tmp_graphicsmode > 0xff)
	{
		if (test)
			return tmp_graphicsmode;
    status = efi_call_2 (gop->set_mode, gop, tmp_graphicsmode & 0xff);	//gop设置模式
    if (status)	//失败
      goto bad_arg; 
		current_x_resolution = x;
		current_y_resolution = y;
		current_bits_per_pixel = z;
		current_bytes_per_pixel = (z+7)/8;
		current_phys_base = gop->mode->fb_base;
		current_bytes_per_scanline = bytes_per_scanline;
    
    if (IMAGE_BUFFER)
      grub_free (IMAGE_BUFFER);
    IMAGE_BUFFER = grub_malloc (current_x_resolution * current_y_resolution * current_bytes_per_pixel);//应当在加载图像前设置
    if (!JPG_FILE)
    {
    JPG_FILE = grub_malloc (0x8000);
    }	
    if (!UNIFONT_START)
    {
      if (embed_font_path[0])
      {
        font_func (embed_font_path, 1);
        grub_free (embed_font);
      }
      else
      {
      //进入图形模式，加载袖珍字库，防止黑屏。这样也可以使防止加载精简中文字库(不包含英文字符)的问题。
      grub_memmove ((char *)0x10000, mini_font_lzma, 820);
      font_func ("(md)0x80+2", 1);  //如果是gz压缩格式，要明确压缩文件尺寸，如：font_func ("(md)0x80+2,820", 1);
      }
    }

		if (graphics_mode != (unsigned int)tmp_graphicsmode            //如果当前视频模式不是探测图形模式
				|| current_term != term_table + 1)	/* terminal graphics */ //或者当前终端不是图像终端
    {
      graphics_mode = tmp_graphicsmode;
      if (graphics_inited)	//如果在图形模式
      {
				current_term->chars_per_line = current_x_resolution / (font_w + font_spacing);
				current_term->max_lines = current_y_resolution / (font_h + line_spacing);   
			}
      else
      {
				console_setcursor(0); //避免转到后图形模式后，在某一固定位置遗留一个文本模式的光标。
				console_shutdown ();	//视乎不起作用。
				graphics_init();
      }
    }
    cls();
  }	//else if (safe_parse_maxint (&arg, &tmp_graphicsmode))
#endif
ok:
  return graphics_mode;
bad_arg:
	errnum = ERR_BAD_ARGUMENT;
	return 0;
}

struct builtin builtin_graphicsmode =
{
  "graphicsmode",
  graphicsmode_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "graphicsmode [--info] [MODE] [-1 | RANGE_X_RESOLUTION]",
  "Examples:\n"
  "graphicsmode (display graphic information)\n"
  "graphicsmode ;; set /A GMODE=%@retval%  (get current mode)\n"
  "graphicsmode --info (Returns the currently supported graphics mode)\n"
  "graphicsmode -1 (auto select mode)\n"
  "graphicsmode -1 800 (switch to highest mode for 800 pixel width)\n"
  "graphicsmode -1 100:1000 (The highest mode available in the range of x = 100-1000)"
};


char menu_init_script_file[32];

static int initscript_func (char *arg, int flags);
static int
initscript_func (char *arg, int flags)
{
	errnum = 0;
	if (grub_strlen(arg) > 32 || ! grub_open (arg))
	{
		return 0;
	}
	grub_close();
	grub_strcpy (menu_init_script_file , arg);
	return 1;
}
 
static struct builtin builtin_initscript =
{
  "initscript",
  initscript_func,
  BUILTIN_MENU,
};

static int echo_func (char *arg,int flags);
static int
echo_func (char *arg,int flags)
{
   unsigned int xy_changed = 0;
   unsigned int saved_x = 0;
   unsigned int saved_y = 0;
   unsigned int x;
   unsigned int y;
   unsigned int echo_ec = 0;
   unsigned long long saved_color_64;
   unsigned char saved_color;
	 unsigned char img = 0;
   //y = getxy();
   //x = (unsigned int)(unsigned char)y;
   //y = (unsigned int)(unsigned char)(y >> 8);
   errnum = 0;
   x = fontx;
   y = fonty;
   for(;;)
   {
      if (grub_memcmp(arg,"-P:",3) == 0)
      {
	 arg += 3;
	 char c = 0;
	 char s[5] = {0};
	 char* p = s;
	 unsigned long long length;
	 int i=2, j=0;
 	 if (*arg == '-') c=*arg++;
		if (*arg == '0' && (arg[1]|32) == 'x')
		{
			if (arg[3] == '-' || (arg[4]|32) == 'x')
				i = 3;
			else
				i = 4;
		}
		while(i--)
		s[j++] = *arg++;
		safe_parse_maxint (&p, &length);
		y = length;
//	 y = ((*arg++ - '0') & 15)*10;
//	 y += ((*arg++ - '0') & 15);
	 if ( c != '\0' )
	 {
	    y = current_term->max_lines - y;
	    c = '\0';
	 }

	 if (*arg == '-') c = *arg++;
	 safe_parse_maxint (&arg, &length);
	 x = length;
//	 x = ((*arg++ - '0') & 15)*10;
//	 x += ((*arg++ - '0') & 15);
	 if (c != 0) x = current_term->chars_per_line - x;
	 //saved_xy = getxy();
	 saved_x = fontx;
	 saved_y = fonty;
	 xy_changed = 1;
	 gotoxy(x,y);
      }
      else if (grub_memcmp(arg,"-h",2) == 0 )
      {
	 int i,j;
	 printf(" 0 1 2 3 4 5 6 7-L-0 1 2 3 4 5 6 7");
	 for (i=0;i<16;i++)
	 {
	    if (y < (unsigned int)current_term->max_lines-1)
		y++;
	    else
			{
				current_color_64bit = 0xAAAAAA;
		putchar('\n', 255);
			}

	    gotoxy(x,y);

	    for (j=0;j<16;j++)
	    {
		if (j == 8)
		{
			current_color = A_NORMAL;
			current_color_64bit = 0xAAAAAA;
			printf(" L ");
		}
		current_color = (i << 4) | j;
		current_color_64bit = color_8_to_64 (current_color);
		printf("%02X",current_color);
	    }
	 }
	if (current_term->setcolorstate)
		current_term->setcolorstate(COLOR_STATE_STANDARD);
	 if (xy_changed)
		gotoxy(saved_x, saved_y);	//restore cursor
	 return 1;
      }
      else if (grub_memcmp(arg,"-n",2) == 0)
      {
		echo_ec |= 1;
      }
      else if (grub_memcmp(arg,"-e",2) == 0)
      {
		echo_ec |= 2;
      }
		else if (grub_memcmp(arg,"-v",2) == 0)
		{
			init_page ();;
		}
		else if (grub_memcmp(arg,"-rrggbb",7) == 0 )
		{
			int i,j,k;
			unsigned long long color=0;
			
			if (graphics_mode <= 0xFF) //vga
			{
				printf("Please use in VBE mode.");
				return 1;
			}

			if (y < (unsigned int)current_term->max_lines-1)
				y++;
			else
				putchar('\n', 255);

			gotoxy(x,y);

			for (i=0;i<6;i++)
			{
				for (j=0;j<6;j++)
				{
					for (k=0;k<6;k++)
					{
						current_color_64bit = color;	//00 33 66 99 cc ff
						printf("0x%06x",color);
						printf("  ");
						color += 0x33;
					}
					color &= 0xffff00;
					color -= 0x100;
					color += 0x3300;
				}
				color &= 0xff0000;
				color -= 0x10000;
				color += 0x330000;
			}
			if (current_term->setcolorstate)
				current_term->setcolorstate(COLOR_STATE_STANDARD);
			if (xy_changed)
				gotoxy(saved_x, saved_y);
			return 1;
		}
    else if (grub_memcmp(arg,"--uefi-img",10) == 0)
		{
			printf("%x",(grub_size_t)g4e_data);
      return 1;
		}
		else if (grub_memcmp(arg,"--img=",6) == 0)	//--img=offset=length
		{
			img = 1;
			goto mem;
		}
		else if (grub_memcmp(arg,"-mem=",5) == 0)	  //-mem=offset=length  怪怪的, 为了所谓'兼容'
		{
			arg--;
			goto mem;
		}
		else if (grub_memcmp(arg,"--mem=",6) == 0)	//--mem=offset=length
		{
mem:
			arg += 6;
			
			unsigned long long offset;
			unsigned long long length;
			unsigned char s[16];
			unsigned int j = 16;

			safe_parse_maxint (&arg, &offset);
			arg++;
			safe_parse_maxint (&arg, &length);
			
			if (j > length)
				j = length;

			while (1)
			{
        grub_memmove((void *)(grub_size_t)s, (const void *)(grub_size_t)(img ? (offset - 0x8200 + (grub_size_t)g4e_data) : offset), j);
				hexdump(offset,(char*)&s,j);
				if (quit_print)
					break;
				offset += j;
				length -= j;
				if (!length)
					break;
				j = (length >= 16)?16:length;
			}
			return 1;
		}
    else if (grub_memcmp(arg,"--uefi-v",8) == 0)
		{
      printf("UEFI revision: %x\n",grub_efi_system_table->hdr.revision);
      return 1;
		}
      else break;
   	 arg = skip_to (0,arg);
   }

	if (echo_ec & 2)
	{
		flags = parse_string(arg);
		arg[flags] = 0;
	}
	saved_color = current_color & 0x70;
	saved_color_64 = current_color_64bit & 0xFFFFFFFF00000000LL;
   for(;*arg;arg++)
   {
      if (*(unsigned short*)arg == 0x5B24)//$[
      {
         if (arg[2] == ']')
         {
		if (current_term->setcolorstate)
			current_term->setcolorstate (COLOR_STATE_STANDARD);
		arg += 3;
         }
         else if (arg[3] == 'x')
         {
            unsigned long long ull;
            char *p = arg + 2;
            if (safe_parse_maxint(&p,&ull) && *p == ']')
            {
		if (ull < 0xff)
		{
			current_color = (unsigned char)ull;
			current_color_64bit = color_8_to_64 (current_color);
		}
		else
		{
			current_color_64bit = ull;
			current_color = color_64_to_8 (current_color_64bit);
		}
		arg = p + 1;
            }
            errnum = 0;
         }
         else if (arg[6] == ']')
         {
            int char_attr = 0;
            if (arg[2] & 7)   //A  只要位:0-6有值(位7无效), 高亮字符背景色
		char_attr |= 0x80;
	    if (arg[3] & 7)         //B  只要位:0-6有值(位7无效), 高亮字符前景色
		char_attr |= 8;
            char_attr |= (arg[4] & 7) << 4; //C 位:0-6 字符背景色, 位7无效
            char_attr |= (arg[5] & 7);      //D 位:0-6 字符前景色, 位7无效
            current_color = char_attr;
	    current_color_64bit = color_8_to_64 (current_color);
	    if (!(current_color & 0x70))  //如果字符背景色位3有值, 使用原始背景色
	    {
		current_color |= saved_color;
		current_color_64bit |= saved_color_64;
	    }
            arg += 7;
         }
      }
      if (current_term->setcolorstate)
        current_term->setcolorstate (current_color | 0x100);  //控制台直接输入颜色
//      grub_putchar((unsigned char)*arg, 255);	//移动到判断之后，避免打印‘\0’   2022-11-05
      if (!(*arg))
				break;
      grub_putchar((unsigned char)*arg, 255);
   }
   if (current_term->setcolorstate)
	  current_term->setcolorstate (COLOR_STATE_STANDARD);
	if ((echo_ec & 1) == 0)
	{
		grub_putchar('\r', 255);
		grub_putchar('\n', 255);
	}
   if (xy_changed)
	gotoxy(saved_x, saved_y);	//restore cursor
   return 1;
}
static struct builtin builtin_echo =
{
   "echo",
   echo_func,
   BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
   "echo [-P:XXYY] [-h] [-e] [-n] [-v] [-rrggbb] [--uefi-img] [--uefi-v]\n   [[--mem | --img]=offset=length] [[$[ABCD]]MESSAGE ...]",
   "-P:XXYY position control line(XX) and column(YY).\n"
   "   XX(YY) are both 2 digit decimal or both 3/4 character hex. Can precede with - sign for position from end.\n"
   "-h      show a color panel.\n"
   "-n      do not output the trailing newline.\n"
   "-e      enable interpretation of backslash escapes.\n"
   "        \\xnn show UTF-8(or hex values) characters.\n"
   "        \\Xnnnn show unicode characters(big endian).\n"
   "-v      show version and memory information.\n"
	 "-rrggbb show 24 bit colors.\n"
	 "--uefi-img show the main program (pre_stage2) entrance of UEFI.\n"
	 "--uefi-v show UEFI version.\n"
	 "--mem=memory_offset=length  hexdump.\n"
	 "--img=pre_stage2_offset=length  hexdump.\n"  
   "$[ABCD] the color for MESSAGE.(console only, 8 bit number)\n" 
   "A=bright background, B=bright characters, C=background color, D=Character color.\n"
   "$[0xCD] Sets the 8 or 64 bit numeric color for MESSAGE.\n"
   "        C=background(high 32 bits), D=Character(low 32 bits).\n"
   "$[] Restore COLOR STATE STANDARD."
};

int else_disabled = 0;  //else禁止
int brace_nesting = 0;  //大括弧嵌套数
int is_else_if;         //是else_if调用
static int if_func(char *arg,int flags);
static int if_func(char *arg,int flags)
{
	char *str1,*str2;
	int cmp_flag = 0;
	long long ret = 0;
	errnum = 0;
  
  
  if (is_else_if)           //如果是else_if调用, 则自身复位
    is_else_if = 0;
  else if (!brace_nesting)  //如果不是else_if调用, 并且大括弧嵌套数为零, 则清除else禁止
    else_disabled = 0;

	while(*arg)
	{
		if (substring("/i ", arg, 1) == -1)
			cmp_flag |= 1;
		else if(substring("not ", arg, 1) == -1)
			cmp_flag |= 4;
		else if (substring("exist ", arg, 1) == -1)
			cmp_flag |= 2;
		else
			break;
		arg = skip_to (0, arg);
	}
	if (*arg == '\0')
		return 0;
	if (cmp_flag & 2)
	{
		if (*arg < '@')
		{
			int no_decompression_bak = no_decompression;
			no_decompression = 1;
			ret = grub_open(arg);
			grub_close();
			errnum = 0;
			no_decompression = no_decompression_bak;
		}
		else
			ret = envi_cmd(arg,NULL,1);
		arg = skip_to(0,arg);
	}
	else
	{
		int cmpn = 0;
		unsigned long long v1,v2;
		char *s1 = str1 = arg;
		str2 = arg = skip_to(1,arg);
		arg -= 2;
		if (*(unsigned short *)arg < 0x3D3C /* <= */
			|| *(unsigned short *)arg > 0x3D3E/* >= */
			)
		{
			errnum = ERR_BAD_ARGUMENT;
			return 0;
		}
		cmpn = (unsigned int)(*arg - '=');
		*arg = 0;
		arg = skip_to (SKIP_WITH_TERMINATE,str2);
		if (safe_parse_maxint(&s1,&v1) && safe_parse_maxint(&str2,&v2))
		{
			ret = v1 - v2;
		}
		else
		{
			errnum = 0;
			ret = strncmpx(str1,str2,0,cmp_flag & 1);
		}
		ret = (cmpn == 0)?ret == 0:((cmpn==-1)?ret<=0:ret>=0);
	}

	if (ret ^ (cmp_flag >> 2))
	{
		return *arg?builtin_cmd(arg,skip_to(0,arg),flags):1;
	}
	return 0;
}

static struct builtin builtin_if =
{
   "if",
   if_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "if [/i] [not] STRING1==STRING2 [COMMAND]",
  "if [NOT] exist VARIABLE|FILENAME [COMMAND]"
};


static unsigned int var_ex_size;
static VAR_NAME *var_ex;
static VAR_VALUE *var_ex_value;
#define FIND_VAR_FLAG_EXISTS 0x10000
#define FIND_VAR_FLAG_VAR_EX 0x20000

static int find_var(const char *ch,const int flag);
static int find_var(const char *ch,const int flag)
{
    int i,j = -1;
    //ch[0] == '?' && ch[1] == '\0';
    for( i = (*ch == '?') ?60:0 ; i < MAX_VARS && VAR[i][0]; ++i)
    {
	if (memcmp(VAR[i], ch, MAX_VAR_LEN) == 0)
	    return i | FIND_VAR_FLAG_EXISTS;
	if (j == -1 && VAR[i][0] == '@') j = i;
    }
    if (*ch != '?' && var_ex_size > 0)
    {
	int k;
	for(k=0; k < (int)var_ex_size && var_ex[k][0]; ++k)
	{
	    if (memcmp(var_ex[k], ch, MAX_VAR_LEN) == 0)
		return k | FIND_VAR_FLAG_EXISTS | FIND_VAR_FLAG_VAR_EX;
	    if (j == -1 && var_ex[k][0] == '@') j = k | FIND_VAR_FLAG_VAR_EX;
	}
	if (i == MAX_VARS && k < (int)var_ex_size)
	    i = k | FIND_VAR_FLAG_VAR_EX;
    }
    if (flag == 1 || (j == -1 && i == MAX_VARS ))
	return -1;
    return ((unsigned int)j < (unsigned int)i)? j : i;
}
/*
flags:
0 add or set
1 read	if env is NULL,return true when the variable var is exist.
2 show
3 reset
*/
int envi_cmd(const char *var,char * const env,int flags)
{
	if(flags == 3)
	{
	    if (var_ex_size > 0)
		memset((char *)var_ex, 0, var_ex_size * sizeof(VAR_NAME));
	    memset( (char *)BASE_ADDR, 0, 512 );
	    sprintf(VAR[_WENV_], "?_WENV");
	    sprintf(VAR[_WENV_+1], "?_BOOT");
//	    QUOTE_CHAR = '\"';
	    return 1;
	}

	int i, j = -1;

	if (flags == 2)
	{
		int count=0;
		for(i=0; i < MAX_USER_VARS && VAR[i][0]; ++i)
		{
			if (VAR[i][0] < 'A')
				continue;
			if (var == NULL || substring(var,VAR[i],0) < 1 )
			{
				++count;
				printf("%.8s=%.512s\n",VAR[i],ENVI[i]);
			}
		}

		if (var_ex_size > 0)
		{
		    for(i=0; i < (int)var_ex_size && var_ex[i][0]; ++i)
		    {
			if (var_ex[i][0] < 'A')
			    continue;
			if (var == NULL || substring(var,var_ex[i],0) < 1 )
			{
			    ++count;
			    printf("%.8s=%.512s\n",var_ex[i],var_ex_value[i]);
			}
		    }
		}
		return count;
	}

	char ch[MAX_VAR_LEN +2] = "\0\0\0\0\0\0\0\0\0\0";
	char *p = (char *)var;
	char *p_name = NULL;
	int ou_start = 0;
	int ou_len = 0x200;
	if (*p == '%')
		p++;
	for (i=0;i<=MAX_VAR_LEN && (unsigned char)*p >='.';i++)
	{
		if (*p == '^')
			break;
		if (*(short*)p == 0x7E3A)//:~
		{
			unsigned long long t;
			p += 2;
			ou_start = safe_parse_maxint(&p,&t)?(int)t:0;
			if (*p == ',')
			{
				++p;
				ou_len = safe_parse_maxint(&p,&t)?(int)t:0;
			}
			break;
		}
		ch[i] = *p++;
	}
	if (flags == 4)
	{
		return (*p == '^' || *p== '%')?p-var:0;
	}

	if (flags == 0 && *p && i > MAX_VAR_LEN )
	{
		errnum = ERR_BAD_ARGUMENT;
		return 0;
	}
	if (ch[MAX_VAR_LEN])
		printf_warning("Warning: VAR name [%s] shortened to 8 chars!\n",ch);
	if (*p && (flags != 1 || (*var == '%' && *p != '%')))
		return 0;


	/*
	i >= 60  system variables.
	'@' 	 Built-in variables or deleted.
	*/
	if (ch[0]=='@')
	{
			struct grub_datetime datetime;

	    if (flags != 1)
		return 0;

	    p = WENV_TMP;
	    memset(p, 0, 512 - 0x80);

			get_datetime(&datetime);

	    if (substring(ch,"@date",1) == 0)
	    {
			sprintf(p,"%04d-%02d-%02d",datetime.year,datetime.month,datetime.day);
	    }
	    else if (substring(ch,"@time",1) == 0)
	    {
			sprintf(p,"%02d:%02d:%02d",datetime.hour,datetime.minute,datetime.second);
	    }
	    else if (substring(ch,"@random",1) == 0)
	    {
//		WENV_RANDOM   =  (WENV_RANDOM * (*(unsigned int *)&datetime) + (*(int *)0x46c)) & 0x7fff;
    		WENV_RANDOM = (((datetime.minute << 8) + WENV_RANDOM + 1) * (datetime.second + 1)) & 0x7fff;
		sprintf(p,"%d",WENV_RANDOM);
	    }
	    else if (substring(ch,"@boot",1) == 0)
	    {
		grub_u32_t tmp_drive = current_drive;
		grub_u32_t tmp_partition = current_partition;
		current_drive = boot_drive;
		current_partition = install_partition;
		print_root_device(p,1);
		current_drive = tmp_drive;
		current_partition = tmp_partition;
	    }
	    else if (substring(ch,"@root",1) == 0)
	    {
		print_root_device(p,0);
		sprintf(p+strlen(p),saved_dir);
	    }
	    else if (substring(ch,"@path",1) == 0)
	    {
		p = command_path;
	    }
	    else if (substring(ch,"@retval",1) == 0)
    sprintf(p,"%d",return_value);
	    else if (substring(ch,"@retval64",1) == 0)
    sprintf(p,"%ld",return_value64);
	    #ifdef PATHEXT
	    else if (substring(ch,"@pathext",1) == 0)
		sprintf(p,"%s",PATHEXT);
	    #endif
      else if (substring(ch,"@uefi",1) == 0)
        sprintf(p,"%d",*(char *)IMG(0x8272));
	    else
		return 0;
	}
	else if (*(short *)ch == 0x3f || *(grub_u64_t*)ch == 0x564e45575f3fLL || *(grub_u64_t*)ch == 0x444955555f3fLL)//?_UUID ?_WENV
	{
		p = WENV_ENVI;
		p_name = VAR[_WENV_];
		j = FIND_VAR_FLAG_EXISTS | _WENV_;
	}
	else
	{
	    j = find_var(ch,flags);

	    if (j == -1)//not variable space
		return 0;

	    if (j & FIND_VAR_FLAG_VAR_EX)
	    {
		p_name = var_ex[j & 0xFFFF];
		p = var_ex_value[ j & 0xffff];
	    }
	    else
	    {
		p = ENVI[j &0xff];
		p_name = VAR[j & 0xff];
	    }
	}
	if (flags == 1)
	{
	    if (!(j & FIND_VAR_FLAG_EXISTS))
		return 0;
	    if (env == NULL)
		return 1;
	    for(j=0;j<512 && p[j]; ++j)
	    {
		;
	    }
	    if (ou_start < 0)
	    {
		    if (-ou_start < j)
		    {
			    ou_start += j;
		    }
		    else
		    {
			    ou_start = 0;
		    }
	    }
	    else if (j - ou_start < 0)
		    ou_start = j;
	    j -= ou_start;
	    if (ou_len < 0)
	    {
		    if (-ou_len <j)
			    ou_len += j;
		    else
			    ou_len=0;
	    }
	    return sprintf(env,"%.*s",ou_len,p + ou_start);
	}
	//flags = 0 set/del variables 
	if (env == NULL || env[0] == '\0')//del
	{
	    if (j & FIND_VAR_FLAG_EXISTS)
		*p_name = '@';
	    return 1;
	}

	if (!(j & FIND_VAR_FLAG_EXISTS))
	    memmove(p_name ,ch ,MAX_VAR_LEN);
	return sprintf(p,"%.512s",env);
}

static void case_convert(char *ch,int flag);
static void case_convert(char *ch,int flag)
{
	if (flag != 'a' && flag != 'A')
		return;
	while (*ch)
	{
		if ((unsigned char)(*ch-flag) < 26)
		{
			*ch ^= 0x20;
		}
		++ch;
	}
}

static int set_func(char *arg, int flags);
static int set_func(char *arg, int flags)
{
	errnum = 0;
	if( *arg == '*' )
		return reset_env_all();   //envi_cmd(NULL, NULL, 3)
	else if (strcmp(VAR[_WENV_], "?_WENV") != 0)
		reset_env_all();          //envi_cmd(NULL, NULL, 3)
	if (*arg == '@')
	{
	    if (substring("@extend",arg,1) > 1)
		return 0;
	    arg = skip_to(1,arg);
	    if (*arg)
	    {
		long long l1;
		long long l2;
		if (!read_val(&arg,&l1) || !read_val(&arg,&l2))
		    return 0;
		if ((unsigned int)l2 > 0xFFFF)
		    return 0;
		l2 &= 0xffff;
		var_ex_size = (unsigned int)l2;
		var_ex = (VAR_NAME *)(grub_size_t)l1;
		var_ex_value = (VAR_VALUE*)(grub_size_t)(l1 + ((l2 + 63) >> 6 << 9));
		memset((char *)var_ex, 0, var_ex_size << 3);
		return 1;
	    }
	    else
		return printf("BASE:%X,%X,VARS:%d",(grub_size_t)var_ex,(grub_size_t)var_ex_value,var_ex_size);
	}
	char value[512];
	int convert_flag=0;
	unsigned long long wait_t = 0xffffff00;
	while (*arg)
	{
		flags = *(short *)arg;
		if (flags == 0x612F) // set /a
		{
			convert_flag |= 0x100;
		}
		else if (flags == 0x412F) // set /A
		{
			convert_flag |= 0x500;
		}
		else if (flags == 0x702F) /* set /p */
		{
			convert_flag |= 0x200;
			if (arg[2] == ':')
			{
				char *p = arg + 3;
				safe_parse_maxint(&p,&wait_t);
				errnum = 0;
				wait_t <<= 8;
			}
		}
		else if (flags == 0x6C2F) /* set /l */
		{
			convert_flag |= 'A';
		}
		else if (flags == 0x752F) /* set /u */
		{
			convert_flag |= 'a';
		}
		else
			break;
		arg = skip_to(0, arg);
	}

	if (*arg == '\"')
	{
		convert_flag |= 0x800;
		++arg;
		int len = strlen(arg);
		if (len)
		{
			--len;
			if (arg[len] == '\"')
				arg[len] = 0;
		}
	} else if ((unsigned char)*arg < '.')
		return get_env_all();
	char *var = arg;
	arg = strstr(arg,"=");
	flags = arg?0:2;

	if (convert_flag & 0x800)
	{
		if (arg) *arg++ = 0;
	}
	else
	{
		arg = skip_to(SKIP_WITH_TERMINATE | 1,var);
	}

	if (convert_flag & 0x200)
	{
		value[0] = 0;
		get_cmdline_str.prompt = (unsigned char*)arg;
		get_cmdline_str.maxlen = sizeof (value) - 1;
		get_cmdline_str.echo_char = 0;
		get_cmdline_str.readline = 1 | wait_t;
		get_cmdline_str.cmdline = (unsigned char*)value;
		if (get_cmdline () || !value[0])
			return 0;
		arg = value;
	}
	if (convert_flag & 0x100)
	{
		if (convert_flag & 0x400)
			sprintf(value,"0x%lX",s_calc(arg,flags));
		else
			sprintf(value,"%ld",s_calc(arg,flags));
		errnum = 0;
		arg = value;
	}

	if (*arg)
	{
		case_convert(arg,convert_flag&0xff);
		flags = 0;
	}
	skip_to(SKIP_LINE,arg);
	return envi_cmd(var,arg,flags);
}

static struct builtin builtin_set =
{
   "set",
   set_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_IFTITLE,
  "set [/p] [/a|/A] [/l|/u] [VARIABLE=[STRING]]",
  "/p,Get a line of input;l|/u,lower/upper case;/a|/A,numerical expression that is evaluated(use calc)."
  "/a,set value to a Decimal;/A  to a HEX."
};

typedef struct _SETLOCAL {
	char var_name[480];//user var_names.
	struct _SETLOCAL *prev;
	unsigned int saved_drive;
	unsigned int saved_partition;
	unsigned int boot_drive;
	unsigned int install_partition;
	int debug;
#if defined(__i386__)
	char reserved[8];//预留位置，同时也是为了凑足12字节
#else
  char reserved[4];//预留位置，同时也是为了凑足12字节
#endif
	char var_str[MAX_USER_VARS<<9];//user vars only
	char saved_dir[256];
	char command_path[128];
} SETLOCAL;
static SETLOCAL *bc = NULL;
static SETLOCAL *cc = NULL;
static SETLOCAL *sc = NULL;

static int setlocal_func(char *arg, int flags);
static int setlocal_func(char *arg, int flags)
{
	errnum = 0;
	SETLOCAL *saved;
	if (*arg == '0')
		return printf("0x%X\n",cc);
	if ((saved=grub_malloc(sizeof(SETLOCAL)))== NULL)
		return 0;
	/* Create a copy of the current user environment */
	memmove(saved->var_name,(char *)BASE_ADDR,(MAX_USER_VARS + 1)<<9);
	sprintf(saved->saved_dir,saved_dir);
	sprintf(saved->command_path,command_path);
	saved->prev = cc;
	saved->saved_drive = saved_drive;
	saved->saved_partition = saved_partition;
	saved->boot_drive = boot_drive;
	saved->install_partition = install_partition;
	saved->debug = debug;
	cc = saved;
	if (*arg == '@')
	{
		sc = cc;
	}
	return 1;
}
static struct builtin builtin_setlocal =
{
   "setlocal",
   setlocal_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT,
};


//unsigned char menu_tab = 0;
unsigned char num_string = 0;
unsigned char menu_font_spacing = 0;
unsigned char menu_line_spacing = 0;
unsigned char timeout_x = 0;
unsigned char timeout_y = 0;
unsigned long long timeout_color = 0;
unsigned long long keyhelp_color = 0;
unsigned char graphic_type = 0;
unsigned char graphic_enable = 0;
unsigned char graphic_row;
unsigned char graphic_list;
unsigned short graphic_wide;
unsigned short graphic_high;
unsigned short row_space;
char graphic_file[128];
struct box DrawBox[16];
struct string* strings;
extern int new_menu;
int num_text_char(char *p);
unsigned char DateTime_enable;
unsigned long long hotkey_color_64bit = 0;
unsigned int hotkey_color = 0;
#define MENU_BOX_X	((menu_border.menu_box_x > 2) ? menu_border.menu_box_x : 2)
#define MENU_BOX_W	((menu_border.menu_box_w && menu_border.menu_box_w < (current_term->chars_per_line - MENU_BOX_X - 1)) ? menu_border.menu_box_w : (current_term->chars_per_line - MENU_BOX_X * 2)) //UEFI固件的右上角(0x2510)是宽字符

static int setmenu_func(char *arg, int flags);
static int
setmenu_func(char *arg, int flags)
{
	char *tem;
	unsigned long long val;
	struct border tmp_broder = {218,191,192,217,196,179,2,0,2,0,0,2,0,0,0};
	int i;

	if (new_menu == 0)
	{
		strings = (struct string*) MENU_TITLE;
		num_string = 0;
		DateTime_enable = 0;
		for (i=0; i<16; i++)
		{
			DrawBox[i].enable = 0;
			strings[i].enable = 0;
		}
		new_menu = 1;
	}

	for (; *arg && *arg != '\n' && *arg != '\r';)  
	{
		if (grub_memcmp (arg, "--string=", 9) == 0)
		{
			int x_horiz_center = 0;
			int y_count_bottom = 0;
			int string_width;
			char *p;
			arg += 9;
			if (!*arg)
			{
				num_string = 0;
				DateTime_enable = 0;
				for (i=0; i<16; i++)
					strings[i].enable = 0;
				goto cont;
			}

			if (*arg == 'i')
			{
				arg++;
				if (safe_parse_maxint (&arg, &val))
					i = val;
				else
					i = num_string;
				if (!*arg)
				{
					strings[i].enable = 0;
					goto cont;
				}
				arg++;
			}
			else
				i = num_string;
			if (i > 15)
				return 0;
			if (*arg == '=')
				x_horiz_center = 1;
			else if (*arg == 's')
			{
				x_horiz_center = 1;
				arg++;
			}
			else if (*arg == 'm')
			{
				x_horiz_center = 2;
				arg++;
			}
			else if (safe_parse_maxint (&arg, &val))
				strings[i].start_x = val;						//x
			arg++;
			if (*arg == '-')
			{
				arg++;
				y_count_bottom++;
			}
			if (safe_parse_maxint (&arg, &val))
			{
				if (y_count_bottom == 0)
					strings[i].start_y = val;							//y
				else
					strings[i].start_y = -(val + 1);
			}
			arg++;
			if (safe_parse_maxint (&arg, &val))
				strings[i].color = val;								//color	
			arg += 2;
			strings[i].enable = 1;
			if (grub_memcmp (arg, "date&time", 9) == 0)
			{
				DateTime_enable = i + 1;
				arg += 9;
			}
			p = arg;
			while (*p++ != '"');
			*(p - 1) = 0;
			if (x_horiz_center == 1)
			{
				if (DateTime_enable == i + 1 && !*arg)
						strings[i].start_x = (current_term->chars_per_line - num_text_char(arg)) >> 1;			//x
				else
				strings[i].start_x = (current_term->chars_per_line - num_text_char(arg)) >> 1;
			}
			else if (x_horiz_center == 2)
			{
				if (DateTime_enable == i + 1 && !*arg)
					strings[i].start_x = MENU_BOX_X + ((MENU_BOX_W - num_text_char(arg)) >> 1);			//x
				else
					strings[i].start_x = MENU_BOX_X + ((MENU_BOX_W - num_text_char(arg)) >> 1);
			}
			if ((string_width = parse_string(arg)) > 99)
				return 0;
			p = strings[i].string;
			while (*arg && string_width--)
				*p++ = *arg++;
			*p = 0;	
			p = strings[i].string;
			if (DateTime_enable == i + 1)
			{
				while(*p && *p++ != '=');
				if (*(p - 1) == '=')
					*(p - 1) = 0;
				else
					*(p + 1) = 0;
			}
			num_string++;		
			arg++;
    }
		else if (grub_memcmp (arg, "--draw-box=", 11) == 0)
		{
			arg += 11;
			if (!*arg)
			{
				for (i=0; i<16; i++)
					DrawBox[i].enable = 0;
				goto cont;
			}
			if (safe_parse_maxint (&arg, &val))
				i = val;
			if (!*arg)
			{
				DrawBox[i].enable = 0;
				goto cont;
			}
			if (i > 16)
				return 0;
			DrawBox[i].enable = 1;
			arg++;
			if (safe_parse_maxint (&arg, &val))
				DrawBox[i].start_x = val;
			arg++;
			if (safe_parse_maxint (&arg, &val))
				DrawBox[i].start_y = val;
			arg++;
			if (safe_parse_maxint (&arg, &val))
				DrawBox[i].horiz = val;
			arg++;
			if (safe_parse_maxint (&arg, &val))
				DrawBox[i].vert = val;
			arg++;
			if (safe_parse_maxint (&arg, &val))
				DrawBox[i].linewidth = val;
			arg++;
			if (safe_parse_maxint (&arg, &val))
				DrawBox[i].color = val;	
    }
		else if (grub_memcmp (arg, "--timeout=", 10) == 0)
		{
			arg += 10;
			if (safe_parse_maxint (&arg, &val))
				timeout_x = val;			//x
			arg++;
			if (safe_parse_maxint (&arg, &val))
				timeout_y = val;	//y
			arg++;
			if (safe_parse_maxint (&arg, &val))
				timeout_color = val;	//color
    }
		else if (grub_memcmp (arg, "--u", 3) == 0)
		{
			menu_tab = 0;
			num_string = 0;
			DateTime_enable = 0;			
			menu_font_spacing = 0;
			menu_line_spacing = 0;
			font_spacing = 0;
			line_spacing = 0;
			if (current_x_resolution && current_y_resolution)
			{
				current_term->max_lines = current_y_resolution / font_h;
				current_term->chars_per_line = current_x_resolution / font_w;
			}
			*(unsigned char *)IMG(0x8274) = 0;
//			*(unsigned short *)IMG(0x8308) = 0x1110;
			*(unsigned short *)IMG(0x82c8) = 0x1110;
			memmove ((char *)&menu_border,(char *)&tmp_broder,sizeof(tmp_broder));
			graphic_type = 0;
			for (i=0; i<16; i++)
			{
				DrawBox[i].enable = 0;
				strings[i].enable = 0;
			}
			return 1;
		}
    else if (grub_memcmp (arg, "--ver-on", 8) == 0)
		{
			menu_tab &= 0x7f;
			arg += 8;
		}
		else if (grub_memcmp (arg, "--ver-off", 9) == 0)
		{
			menu_tab |= 0x80;
			arg += 9;
		}
		else if (grub_memcmp (arg, "--lang=en", 9) == 0)
		{
			menu_tab &= 0xdf;
			arg += 9;
		}
		else if (grub_memcmp (arg, "--lang=zh", 9) == 0)
		{
			menu_tab |= 0x20;
			arg += 9;
		}
		else if (grub_memcmp (arg, "--left-align", 12) == 0)
		{
			menu_tab &= 0xbf;
			menu_tab &= 0xf7;
			arg += 12;
		}
		else if (grub_memcmp (arg, "--right-align", 13) == 0)
		{
			menu_tab |= 0x40;
			menu_tab &= 0xf7;
			arg += 13;
		}
		else if (grub_memcmp (arg, "--middle-align", 14) == 0)
		{
			menu_tab |= 8;
			arg += 14;
		}
		else if (grub_memcmp (arg, "--triangle-on", 13) == 0)
		{
//			*(unsigned short *)IMG(0x8308) = 0x1110;
			*(unsigned short *)IMG(0x82c8) = 0x1110;
			arg += 13;
		}
		else if (grub_memcmp (arg, "--triangle-off", 14) == 0)
		{
//			*(unsigned short *)IMG(0x8308) = 0;
			*(unsigned short *)IMG(0x82c8) = 0;
			arg += 14;
		}
		else if (grub_memcmp (arg, "--highlight-short", 17) == 0)
		{
			menu_tab &= 0xef;
			arg += 17;
		}
		else if (grub_memcmp (arg, "--highlight-full", 16) == 0)
		{
			menu_tab |= 0x10;
			arg += 16;
		}
		else if (grub_memcmp (arg, "--keyhelp-on", 12) == 0)
		{
			menu_tab &= 0xfb;
			arg += 12;
		}
    else if (grub_memcmp (arg, "--keyhelp-off", 13) == 0)
		{
			menu_tab |= 4;
			arg += 13;
		}
		else if (grub_memcmp (arg, "--box", 5) == 0)
		{
			arg = skip_to (0, arg);
			for (; *arg && *arg != '\n' && *arg != '\r' && *arg != '-';)
			{
				tem = arg + 2;
				if (safe_parse_maxint (&tem, &val))
				{
					switch(*arg)
					{
						case 'x':
							menu_border.menu_box_x = val;
							break;
						case 'w':
//							if (val != 0)
								menu_border.menu_box_w = val;
//							else
//								menu_border.menu_box_w = current_term->chars_per_line - menu_border.menu_box_x * 2 + 1;  //多余，影响w判断  2023-02-22
							break;
						case 'y':
							menu_border.menu_box_y = val;
							break;
						case 'h':
							if (! graphic_type)
							menu_border.menu_box_h = val;
							break;
						case 'l':
							if (val > 3)
								val = 3;
							menu_border.border_w = val;
							break;
						default:
							break;
					}
				}
				arg = tem;
				while (*arg == ' ' || *arg == '\t')
					arg++;
			}
		}
    else if (grub_memcmp (arg, "--auto-num-all-on", 17) == 0)
		{
			*(unsigned char *)IMG(0x8274) = 2;
			arg += 17;
		}
		else if (grub_memcmp (arg, "--auto-num-on", 13) == 0)
		{
			*(unsigned char *)IMG(0x8274) = 1;
			arg += 13;
		}
		else if (grub_memcmp (arg, "--auto-num-off", 14) == 0)
		{
			*(unsigned char *)IMG(0x8274) = 0;
			arg += 14;
		}
		else if (grub_memcmp (arg, "--font-spacing=", 15) == 0)
		{
			arg += 15;
			if(safe_parse_maxint (&arg, &val))
			{
				menu_font_spacing = val;
				font_spacing = val;
				current_term->chars_per_line = current_x_resolution / (font_w + font_spacing);
			}
			arg++;
			if(safe_parse_maxint (&arg, &val))
			{
				menu_line_spacing = val;
				line_spacing = val;
				current_term->max_lines = current_y_resolution / (font_h + line_spacing);
			}	
		}
		else if (grub_memcmp (arg, "--keyhelp=", 10) == 0)	//--keyhelp=y_offset=color
		{
			arg += 10;
			if (safe_parse_maxint (&arg, &val))
				menu_border.menu_keyhelp_y_offset = val;
			arg++;
			if (safe_parse_maxint (&arg, &val))
				keyhelp_color = val;
		}
		else if (grub_memcmp (arg, "--help=", 7) == 0)	//--help=x=w=y
		{
			arg += 7;
			if (safe_parse_maxint (&arg, &val))
				menu_border.menu_help_x = val;
			arg++;
			if (safe_parse_maxint (&arg, &val))
			{
				if (menu_border.menu_help_x + val > current_term->chars_per_line)
					menu_border.menu_help_w = current_term->chars_per_line - menu_border.menu_help_x;
				else
					menu_border.menu_help_w = val;
			}
			arg++;
			if (safe_parse_maxint (&arg, &val))
				menu_border.menu_box_b = val;								//y
		}
		else if (grub_memcmp (arg, "--graphic-entry=", 16) == 0)	//--graphic-entry=type=row=list=wide=high=row_space FILE 
		{
			arg += 16;
			if (safe_parse_maxint (&arg, &val))
				graphic_type = val;
			arg++;
			if (safe_parse_maxint (&arg, &val))
				graphic_row = val;
			arg++;
			if (safe_parse_maxint (&arg, &val))
				graphic_list = val;
			arg++;
			if (safe_parse_maxint (&arg, &val))
				graphic_wide = val;
			arg++;
			if (safe_parse_maxint (&arg, &val))
				graphic_high = val;
			arg++;
			if (safe_parse_maxint (&arg, &val))
				row_space = val;
			else
				row_space = 0x20;
			arg++;
			strcpy(graphic_file, arg);
			menu_border.menu_box_h = graphic_row * graphic_list;
			menu_border.border_w = 0;
		}
    else if (grub_memcmp (arg, "--hotkey-color=", 15) == 0)   //--hotkey-color=COLOR 64位色
		{
			arg += 15;
			if (safe_parse_maxint (&arg, &val))
				hotkey_color_64bit = val;
      hotkey_color = color_64_to_8 (hotkey_color_64bit);
		}
		else
			return 0;
cont:		
		while(*arg && !isspace(*arg) && *arg != '-')
			arg++;
		while (*arg == ' ' || *arg == '\t')
			arg++;
  }
	return 1;
}

static struct builtin builtin_setmenu =
{
  "setmenu",
  setmenu_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "setmenu --parameter | --parameter | ... ",
  "--ver-on* --ver-off --lang=en* --lang=zh --u (clear all)\n"
	"--left-align* --right-align --middle-align\n"
	"--auto-num-off* --auto-num-all-on --auto-num-on --triangle-on* --triangle-off\n"
	"--highlight-short* --highlight-full --keyhelp-on* --keyhelp-off\n"
	"--font-spacing=FONT:LINE. default 0\n"
	"--string[=iINDEX]=[X|s|m]=[-]Y=COLOR=\"[$[0xRRGGBB]]STRING\"\n"
	"  iINDEX range is i0-i15. Auto-increments if =iINDEX is omitted.\n"
	"  If the horizontal position is 's', \"STRING\" centers across the whole screen.\n"
	"  If the horizontal position is 'm', \"STRING\" centers within menu area.\n"
	"  -Y represents the count from the bottom.\n"
	"  \"STRING\"=\"date&time=FORMAT\"  will update date FORMAT every second.\n"
	"  e.g. \"date&time=MMM.dd.yyyy  HH:mm:ss\"\n"
	"  e.g. \"date&time=dd/MMM/yy  AP hh:mm:ss\"\n"
	"  \"STRING\"=\"date&time\"  ISO8601 format. equivalent to: \"date&time=yyyy-MM-dd  HH:mm:ss\"\n"
	"  --string= to disable all strings.\n"
	"  --string=iINDEX to disable the specified index.\n"
	"  $[0xRRGGBB] Sets the color for STRING.\n"
	"  $[] Restore COLOR STATE STANDARD.\n"
	"--box x=X y=Y w=W h=H l=L\n"
	"  If W=0, menu box in middle. L=menu border thickness 0-4, 0=none.\n"
	"--help=X=W=Y\n"
	"  X=0* menu start and width. X<>0 and W=0 Entire display width minus 2x.\n"
	"--keyhelp=Y_OFFSET=COLOR\n"
	"  Y_OFFSET=0* entryhelp and keyhelp in the same area,entryhelp cover keyhelp.\n"
	"  Y_OFFSET!=0 keyhelp to entryhelp line offset.two coexist.\n"
	"  Y_OFFSET<=4, entryhelp display line number.\n"
	"  COLOR=0* default 'color helptext'.\n"
	"--timeout=X=Y=COLOR\n"
	"  X=Y=0* located at the end of the selected item.\n"
	"  COLOR=0* default 'color highlight'.\n"
	"--graphic-entry=type=row=list=wide=high=row_space START_FILE\n"
	"  type: bit0:highlight  bit1:flip  bit2:box  bit3:highlight background\n"
	"        bit4:Picture and text mixing  bit7:transparent background.\n"
	"  Naming rules for START_FILE: *n.???   n: 00-99\n"
	"--draw-box=INDEX=START_X=START_y=HORIZ=VERT=LINEWIDTH=COLOR.\n"
	"  LINEWIDTH:1-255; all dimensions in pixels. INDEX range is 0-15.\n"
	"  --draw-box=INDEX to disable the specified index.  --draw-box= to clear all indexes.\n"
	"--hotkey-color=COLOR set hotkey color.\n" 
	"Note: * = default. Use only 0xRRGGBB for COLOR."
};

static char *month_list[12] =
{
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

unsigned short refresh = 0;
void DateTime_refresh(void);
void DateTime_refresh(void)
{
	int i = DateTime_enable-1;
	unsigned short year;
	unsigned char month, day, hour, min, sec;
	char *p;
	
	if (strings[i].enable == 0)
	{
		DateTime_enable = 0;
		return;
	}
	if ((cursor_state & 1) == 1 || !show_menu)
		return;
	putchar_hooked = 0;
	if (!refresh)
	{
		struct grub_datetime datetime;
		char y;
		int	backup_x = fontx;
		int	backup_y = fonty;
		
		refresh = 250;
		get_datetime(&datetime);
		current_term->setcolorstate (COLOR_STATE_NORMAL);
		if (strings[i].start_y < 0)
			y = strings[i].start_y + current_term->max_lines;
		else
			y = strings[i].start_y;
		gotoxy (strings[i].start_x, y);
		if (!(strings[i].color & 0xffffffff00000000))
		{
			if (!(splashimage_loaded & 2))
				current_color_64bit = strings[i].color | (console_color_64bit[COLOR_STATE_NORMAL] & 0xffffffff00000000);
      else
        current_color_64bit = strings[i].color | (current_color_64bit & 0xffffffff00000000);
		}
		else
			current_color_64bit = strings[i].color | 0x1000000000000000;
		current_term->setcolorstate (color_64_to_8 (current_color_64bit & 0x00ffffffffffffff) | 0x100);
		year = datetime.year;
		month = datetime.month;
		day = datetime.day;
		hour = datetime.hour;
		min = datetime.minute;
		sec = datetime.second;
		p = (char *)strings[i].string;
		if (*p != '=')
			grub_printf("%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, min, sec);
		else
		{
			gotoxy (strings[i].start_x + num_text_char(p), y);
			while(*p++);
			while(*p)
			{
				if (*p == 'y' && *(p+1) == 'y')
				{
					if (*(p+2) == 'y' && *(p+3) == 'y')
					{
						grub_printf ("%04X", year);
						p += 2;
					}
					else
						grub_printf ("%02X", (unsigned char)year);
				}	
				else if (*p == 'M' && *(p+1) == 'M')
				{
					if (*(p+2) == 'M')
					{
						grub_printf ("%s", month_list[month - 1]);
						p++;
					}
					else
						grub_printf ("%02d", month);
				}
				else if (*p == 'd' && *(p+1) == 'd')
					grub_printf ("%02X", day);
				else if (*p == 'H' && *(p+1) == 'H')
					grub_printf ("%02d", hour);
				else if (*p == 'h' && *(p+1) == 'h')
					grub_printf ("%02d", (hour == 0) ? 12 : ((hour > 12) ? (hour - 12) : hour));
				else if (*p == 'A' && *(p+1) == 'P')
					grub_printf ("%s", (hour >= 12) ? "PM" : "AM");
				else if (*p == 'm' && *(p+1) == 'm')
					grub_printf ("%02X", min);
				else if (*p == 's' && *(p+1) == 's')
					grub_printf ("%02X", sec);
				else
					grub_printf ("%c", *p--);

				p += 2;
			}
		}
		current_term->setcolorstate (COLOR_STATE_NORMAL);
		gotoxy (backup_x,backup_y);
	}
	else
		refresh--;
	return;
}

static int endlocal_func(char *arg, int flags);
static int endlocal_func(char *arg, int flags)
{
	errnum = 0;
	SETLOCAL *saved = cc;
	if (*arg == '@')
	{
		sc = NULL;
	}
	if (cc == bc || cc == sc)
	{
		return 0;
	}

	/* Restore variables from the copy saved by setlocal_func */
	memmove(VAR[0],saved->var_name,MAX_USER_VARS<<3);
	memmove(ENVI[0],saved->var_str,MAX_USER_VARS<<9);
	sprintf(saved_dir,saved->saved_dir);
	sprintf(command_path,saved->command_path);
	saved_drive = saved->saved_drive;
	saved_partition = saved->saved_partition;
	boot_drive = saved->boot_drive;
	install_partition = saved->install_partition;
	debug = saved->debug;
	cc = cc->prev;
//	grub_free(saved);
	return 1;
}
static struct builtin builtin_endlocal =
{
   "endlocal",
   endlocal_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT,
};

struct _debug_break
{
	int pid;
	int line;
};

struct _debug_break debug_break[10] = {{0},{0}};

struct bat_label
{
	char *label;
	int line;
};

/*
prog_pid is current running batch script id.
it must the same of p_bat_prog->pid.
the first batch prog_pid is 1,max 10.so we can run 10 of batch script one time.
when all batch script is exit the prog_pid is 0;
*/
struct bat_array
{
	int pid;
	int debug_break;
	struct bat_label *entry;
	/*
	 (char **)(entry + 0x80) is the entry of bat_script to run.
	*/
	grub_u32_t size;
	char *path;
	char md[0];
} *p_bat_prog = NULL;

static char **batch_args;
/*
find a label in current batch script.(p_bat_prog)
return line of the batch script.
if not find ,return -1;
*/
static int bat_find_label(char *label);
static int bat_find_label(char *label)
{
	struct bat_label *label_entry;
	if (*label == ':') label++;
	nul_terminate(label);
	for (label_entry = p_bat_prog->entry; label_entry->label ; label_entry++)//find label for goto/call.
	{
		if (substring(label_entry->label,label,1) == 0)
		{
			return label_entry->line;
		}
	}

	printf_errinfo(" cannot find the batch label specified - %s\n",label);
	return 0;
}

static int bat_get_args(char *arg,char *buff,int flags);
static int bat_get_args(char *arg,char *buff,int flags)
{
//#define ARGS_TMP RAW_ADDR(0x100000)
  char *ARGS_TMP = grub_zalloc (0x400);
	char *p = ((char *)ARGS_TMP);
	char *s1 = buff;
	int isParam0 = (flags & 0xff);

	if (*arg == '(')
	{
		unsigned int tmp_partition = current_partition;
		unsigned int tmp_drive = current_drive;
		char *cd = set_device (arg);
		if (cd)
		{
			print_root_device(p,1);
			current_partition = tmp_partition;
			current_drive = tmp_drive;
			p += strlen(p);
			arg = cd;
		}
		else
		{
			while ((*p++ = *arg++) != ')')
				;
			*p = 0;
			case_convert((char*)ARGS_TMP,'A');
		}
	}
	else if (isParam0) // if is Param 0
	{
		p += sprintf(p,"%s",p_bat_prog->path) - 1;//use program run dir
	}
	else
	{
		print_root_device(p,0);
		p += strlen(p);
		p += sprintf(p,saved_dir);
	}
	if (*arg != '/')
		*p++ = '/';
	if (p + strlen(arg) >= (char *)ARGS_TMP + 0x400)
		goto quit;
	sprintf(p,"%s",arg);
	p = ((char *)ARGS_TMP);
	flags >>= 8;

	if (flags & 0x20) buff += sprintf(buff,"%s",p_bat_prog->md);

	if (flags & 0x10)
	{
		if (isParam0)
		{
			buff += sprintf(buff,"0x%X",p_bat_prog->size);
		}
		else if (grub_open(p))
		{
			buff += sprintf(buff,"0x%lX",filemax);
			grub_close();
		}
		errnum = 0;
		flags &= 0xf;
		if (flags) *buff++ = '\t';
	}

	if (flags == 0x2f)
	{
		buff += sprintf(buff, p);
		goto quit;
	}

	if (flags & 1)
	{
		while ( *p && *p != '/')
			*buff++ = *p++;
	}

	if (! (p = strstr(p,"/")))
		goto quit;
	char *p0,*p1,*p2 = NULL;
	p0 = p1 = p;

	while (*p)
	{
		if (*p++ == '/')
			p1 = p;
		if (*p == '.')
			p2 = p;
	}

	if (p2 < p1)
		p2 = p;

	if (flags & 2)
	{
		buff += sprintf(buff, "%.*s",p1 - p0,p0);
	}

	if (flags & 4)
	{
		buff += sprintf(buff,"%.*s",p2 - p1,p1);
	}

	if (flags & 8)
	{
		buff += sprintf(buff,p2);
	}

quit:
	grub_free (ARGS_TMP);
	return buff-s1;
}

grub_u32_t ptrace = 0;
/*
bat_run_script
run batch script.
if filename is NULL then is a call func.the first word of arg is a label.
*/
static int bat_run_script(char *filename,char *arg,int flags);
static int bat_run_script(char *filename,char *arg,int flags)
{
//	int debug_bat = debug_prog;
	if (prog_pid != (unsigned int)p_bat_prog->pid)
	{
		errnum = ERR_FUNC_CALL;
		return 0;
	}

	char **bat_entry = (char **)(p_bat_prog->entry + 0x80);
	grub_u32_t i = 1;

	if (filename == NULL)
	{//filename is null is a call func;
		filename = arg;
		arg = skip_to(SKIP_WITH_TERMINATE | 1,arg);
		if ((i = bat_find_label(filename)) == 0)
		{
			errnum = ERR_BAT_CALL;
			return 0;
		}
	}

	if (debug_prog)
  {    
    if (debug_bat)
      printf("S^:%s [%d]\n",filename,prog_pid);
    ptrace++;
    if (ptrace > 1 && debug_ptrace)
      debug_bat = 0;
  }
      

	char **p_entry = bat_entry + i;

	char *s[10];
	char *p_cmd;
	char *p_rep;
	char *p_buff;//buff for command_line
	char *cmd_buff;
	grub_u32_t ret = grub_strlen(arg) + 1;

	if ((cmd_buff = grub_malloc(ret + 0x800)) == NULL)
	{
		return 0;
	}

  else_disabled = 0;  //else禁止
  brace_nesting = 0;  //大括弧嵌套数
	/*copy filename to buff*/
	i = grub_strlen(filename);
	grub_memmove(cmd_buff,filename,i+1);
	p_buff = cmd_buff + ((i + 16) & ~0xf);
	s[0] = cmd_buff;
	/*copy arg to buff*/
	grub_memmove(p_buff, arg, ret);
	arg = p_buff;
	p_buff = p_buff + ((ret + 16) & ~0xf);

	/*build args %1-%9*/
	for (i = 1;i < 9; ++i)
	{
		s[i] = arg;
		if (*arg)
			arg = skip_to(SKIP_WITH_TERMINATE | 1,arg);
	}
	s[9] = arg;// %9 for other args.

	char *p_bat;
	char **backup_args = batch_args;
	SETLOCAL *saved_bc = bc;
	batch_args = s;
	bc = cc; //saved for batch
	ret = 0;

	while ((p_bat = *p_entry))//copy cmd_line to p_buff and then run it;
	{
		p_cmd = p_buff;
		char *file_ext;

		if (p_bat == (char*)-1)//Skip Line
		{
			p_entry++;
			continue;
		}
    
    if (*p_bat == '{') //左大括弧
    {
      if (!ret)
        goto ddd;
      else
      {
        brace_nesting++;  //大括弧嵌套数+1
        p_entry++;        //批处理下一行
        continue;
      }
    }

    if (*p_bat == '}')  //如果是右大括弧
		{ 
      brace_nesting--;  //大括弧嵌套数-1
      else_disabled |= 1 << brace_nesting;  //设置else禁止位
			p_entry++;        //批处理下一行
			continue;
		}

		while(*p_bat)
		{
			if (*p_bat != '%' || (file_ext = p_bat++,*p_bat == '%'))
			{//if *p_bat != '%' or p_bat[1] == '%'(*p_bat == p_bat[1] == '%');
				*p_cmd++ = *p_bat++;
				continue;
			}//file_ext now use for backup p_bat see the loop end.

			i = 0;

			if (*p_bat == '~')
			{
				p_bat++;
				i |= 0x80;
				while (*p_bat)
				{
					if (*p_bat == 'd')
						i |= 1;
					else if (*p_bat == 'p')
						i |= 2;
					else if (*p_bat == 'n')
						i |= 4;
					else if (*p_bat == 'x')
						i |= 8;
					else if (*p_bat == 'f')
						i |= 0xf;
					else if (*p_bat == 'z')
						i |= 0x10;
					else if (*p_bat == 'm')
						i |= 0x20;
					else
						break;
					p_bat++;
				}
			}

			if (*p_bat <= '9' && *p_bat >= '0')
			{
				p_rep = s[*p_bat - '0'];
				if (*p_rep)
				{
					int len_c = 0;
					if ((i & 0x80) && *p_rep == '\"')
					{
						p_rep++;
					}
					if (i & 0x3f)
					{
						len_c = bat_get_args(p_rep,p_cmd,i << 8 | (s[*p_bat - '0'] == cmd_buff));
					}
					else
					{
						len_c = sprintf(p_cmd,p_rep);
					}

					if (len_c)
					{
						if ((i & 0x80) && p_cmd[len_c-1] == '\"')
						--len_c;
						p_cmd += len_c;
					}
				}
			}
			else if (*p_bat == '*')
			{
				for (i = 1;i< 10;++i)
				{
					if (s[i][0])
#if 1
						p_cmd += sprintf(p_cmd,"%s ",s[i]);
#else
					{		//消除末尾的空格  2022-11-05    会使得外部命令SISO、RUN列表文件时，扩展名只显示前2个！  2022-12-15
						if (i == 1)
							p_cmd += sprintf(p_cmd,"%s",s[i]);
						else
							p_cmd += sprintf(p_cmd," %s",s[i]);
					}
#endif
					else
						break;
				}
			}
			else
			{
				p_bat = file_ext;
				*p_cmd++ = *p_bat;
			}
			++p_bat;
		}

		*p_cmd = '\0';
    //批处理调试:  debug 批处理文件名  参数.     例如: debug /run --automenu
//		if (p_bat_prog->debug_break && ((grub_u32_t)p_bat_prog->debug_break == (grub_u32_t)(p_entry-bat_entry))) debug_bat = debug_prog = 1;
    for (i=0; i<10; i++)
    {
      if (debug_break[i].pid == prog_pid && debug_break[i].line == (grub_u32_t)(p_entry-bat_entry))
      {
        debug_bat = debug_prog = 1;
        break;
      }
    }
		if (debug_prog && debug_bat) printf("S[%d#%d]:[%s]\n",prog_pid,((grub_u32_t)(p_entry-bat_entry)),p_buff);
		if (debug_bat)
		{
			Next_key:
			if (current_term->setcolorstate) current_term->setcolorstate(COLOR_STATE_HEADING);
			grub_printf ("[Q->quit,C->Shell,S->Skip,E->End step,B->Breakpoint,N->step Next func]");
			i=getkey() & 0xdf;
			if (current_term->setcolorstate) current_term->setcolorstate(COLOR_STATE_STANDARD);
			grub_printf("\r%75s","\r");
      char *SYSTEM_RESERVED_MEMORY;
			switch(i)
			{
				case 'Q': //结束批处理
					errnum = 2000;
					break;
				case 'C': //进入命令行  按'ESC'键返回批处理调试
          SYSTEM_RESERVED_MEMORY = grub_zalloc (512);
          if (!SYSTEM_RESERVED_MEMORY)
            return 0;
					commandline_func(SYSTEM_RESERVED_MEMORY,0);
          grub_free (SYSTEM_RESERVED_MEMORY);
					break;
				case 'S': //跳过本行
					++p_entry;
					continue;
				case 'N': //运行至断点
					debug_bat = 0;
          debug_ptrace = 0;
					break;
				case 'E': //退出调试模式
					debug_bat = debug_prog = 0;
					break;
				case 'B': //设置断点(共10个)  批处理号(共10个),批处理行号
				{
					char buff[12];
					grub_u64_t t;
					buff[0] = 0;
//					printf("Current:\nDebug Check Memory [0x%x]=>0x%x\nDebug Break Line: %d\n",debug_check_memory,debug_break,p_bat_prog->debug_break);
					get_cmdline_str.prompt = &msg_password[8];
					get_cmdline_str.maxlen = sizeof (buff) - 1;
					get_cmdline_str.echo_char = 0;
					get_cmdline_str.readline = 0;
					get_cmdline_str.cmdline = (grub_u8_t*)buff;
					get_cmdline ();

//					if (buff[0] == '+' || buff[0] == '-' || buff[0] == '*') p_bat = &buff[1];
					if (buff[0] == 'p' || buff[0] == 'c' || buff[0] == 'l') p_bat = &buff[1];
					else p_bat = buff;
#if 0
					if (safe_parse_maxint ((char **)(grub_size_t)&p_bat, (unsigned long long *)(grub_size_t)&t))
					{
						if (buff[0] == '*')
						{
							debug_check_memory = t;
							debug_break = *(int*)(grub_size_t)debug_check_memory;
							printf("\rDebug Check Memory [0x%x]=>0x%x\n",debug_check_memory,debug_break);
						}
						else
						{
							if (buff[0] == '-') t = (grub_u32_t)(p_entry-bat_entry) - (int)t;
							else if (buff[0] == '+') t += (grub_u32_t)(p_entry-bat_entry);

							p_bat_prog->debug_break = t;
							printf("\rDebug Break Line: %d\n",p_bat_prog->debug_break);
						}
					}
#else
          if (buff[0] == 'c') //清除断点   例如: c2 (清除2号断点);  c (清除全部断点)
          {
            t = 0;
            safe_parse_maxint ((char **)(grub_size_t)&p_bat, (unsigned long long *)(grub_size_t)&t);
            if (t)
              debug_break[t-1].pid = 0;
            else
            {
              for (i=0; i<10; i++)
              {
                debug_break[i].pid = 0;
              }
            }
          }
          else if (buff[0] == 'l') //显示断点  (断点号,批处理号,批处理行号)
          {
            for (i=0; i<10; i++)
            {
              if (debug_break[i].pid)
                printf("\ri=%d, pid=%d, line=%d\n",i+1,debug_break[i].pid,debug_break[i].line);
            }
          }
          else //设置断点(隐含当前批处理标号)  例如: 45 (当前批处理,45行)
          {
            i = 0;
            while (debug_break[i].pid && i < 10) i++;
            safe_parse_maxint ((char **)(grub_size_t)&p_bat, (unsigned long long *)(grub_size_t)&t);
            debug_break[i].pid = prog_pid;
            if (buff[0] == 'p') //设置断点  例如: p2,134  (2号批处理,134行)
            {
              debug_break[i].pid = t;
              p_bat++;
              safe_parse_maxint ((char **)(grub_size_t)&p_bat, (unsigned long long *)(grub_size_t)&t);
            }
            debug_break[i].line = t;
            printf("\rDebug Break pid=%d, line=%d\n",debug_break[i].pid,debug_break[i].line);
          }
#endif
					goto Next_key;
				}
        case 'P': //单步执行(跨过子程序)
					debug_ptrace = 1;
					break;
        default:  //单步执行
					debug_ptrace = 0;
					break;
			}
			if (errnum == 2000) break;
		}
		ret = run_line (p_buff,flags);
   
    if (errnum == ERR_BAT_BRACE_END) //如果是批处理大括弧结束
    {
ddd:
      errnum = ERR_NONE;    //消除错误号 
      int brace_count = 0;  //大括弧计数=0
      while (1)
      {
        p_bat = *(p_entry); //批处理入口
aaa:
        while (*p_bat && *p_bat != '{' && *p_bat != '}') p_bat++; //搜索左右大括弧,直至找到,或者结束.
        if (*p_bat == '{')  //如果是左大括弧
        {
          brace_count++;    //大括弧计数+1
          p_bat++;          //下一行批处理
          goto aaa;          //继续搜索
        }
        if (*p_bat == '}')  //如果是右大括弧
        {
          brace_count--;    //大括弧计数-1
          if (!brace_count) //如果大括弧计数=0
          {
            p_entry++;      //批处理下一入口
            break;          //搜索结束
          }
          else              //否则
            p_bat++;        //下一行批处理
          goto aaa;          //继续搜索
        }
       else                 //如果没有搜索左右大括弧
        p_entry++;          //批处理下一入口
      }
      continue;
    }
#if 0 
		if (debug_check_memory)
		{
			if (debug_break != *(int*)(grub_size_t)debug_check_memory)
			{
				printf("\nB: %s\n[0x%x]=>0x%x (0x%x)\n",p_buff,debug_check_memory,*(int*)(grub_size_t)debug_check_memory,debug_break);
				debug_bat = debug_prog = 1;
			}
		}
#endif
		if (ctrl_c_trap && checkkey() == 0x2000063)
		{
			unsigned char k;
			loop_yn:
			grub_printf("\nTerminate batch job (Y/N)? ");
			k = getkey() & 0xDF;
			putchar(k, 255);
			if (k == 'Y')
			{
					errnum = 2000;
					break;
			}
			if (k != 'N')
				goto loop_yn;
		}

		if (errnum == ERR_BAT_GOTO)
		{
			if (ret == 0)
				break;
			p_entry = bat_entry + ret;
			errnum = ERR_NONE;
			continue;
		}
		else if ((unsigned int)errnum >= 1000 )
		{
			break;
		}
		else if (errorcheck && errnum)
		{
			if (debug > 0)
				printf("%s\n",p_buff);
			break;
		}

		p_entry++;
	}
	i = errnum; //save errnum.
	/*release memory. */
	while (bc != cc && cc != sc) //restore SETLOCAL
		endlocal_func(NULL,1);
	bc = saved_bc;
	batch_args = backup_args;
	grub_free(cmd_buff);

	if (debug_prog)
  {
    ptrace--;
    if (ptrace == 1 && debug_ptrace)
    {
      debug_bat = 1;
      debug_ptrace = 0;
    }
    if (debug_bat)
      printf("S$:%s [%d]\n",filename,prog_pid); 
  }

	errnum = (i == 1000) ? 0 : i;
	return errnum?0:(int)ret;
}

static int goto_func(char *arg, int flags);
static int goto_func(char *arg, int flags)
{
	errorcheck_func ("on",0);
	unsigned long long val;
	char *p = arg;
	if (*arg == '+' || *arg == '-' || safe_parse_maxint (&p, &val))
	{
		errnum = ERR_BAT_GOTO;
		return fallback_func(arg,flags);
	}
	errnum = ERR_BAT_GOTO;
	return bat_find_label(arg);
}

static struct builtin builtin_goto =
{
   "goto",
   goto_func,
   BUILTIN_SCRIPT | BUILTIN_BAT_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_MENU | BUILTIN_CMDLINE,
   "goto [+|-|:]DESTINATION",
   "e.g. goto [+|-]NUM. Use in menus. Jump to the specified title.\n"
   "e.g. goto [:]LABEL. Use in batch files or menus. Jump to the specified ':LABEL'.\n"
   "When the LABEL there is no prefix ':', the LABEL can't be a number."
};

static int call_func(char *arg,int flags);
static int call_func(char *arg,int flags)
{
	errnum = 0;
	if (*arg==':')
	{
		return bat_run_script(NULL, arg, flags);
	}
	if (*(short *)arg == 0x6E46)  //Fn  Fn.[func] 参数          call Fn.26
	{
//    unsigned int func;
    grub_size_t func;
		long long ull;
		int i;
		char *ch[10]={0};
		arg += 3;
		if (! read_val(&arg,&ull))
			return 0;
//    func=(unsigned int)ull;
    func = (grub_size_t)ull;
		arg[parse_string(arg)] = 0;
		for (i=0;i<10;++i)
		{
      if (*arg == 'g' && *(arg+1) == '4' && *(arg+2) == 'e')
      { 
        arg += 3;
        safe_parse_maxint (&arg, (unsigned long long *)&ull);
        ch[i] = (char *)(grub_size_t)IMG(ull);
      }
      else if (read_val(&arg,&ull))
      {
				ch[i] = (char *)(grub_size_t)ull;
      }
			else
			{
				ch[i] = arg;
				arg = skip_to(SKIP_WITH_TERMINATE,arg);
				if (ch[i][0] == '\"')
				{
					++ch[i];
					ch[i][strlen(ch[i])-1] = 0;
				}
			}
		}
		errnum = 0;
//		if (func<0xFF)
//			func = (*(int **)0x8300)[func];

		if (func<0xFF)
//      func = (grub_size_t)(*(int *)(grub_size_t)(*(int *)IMG(0x8300) + func * sizeof(long)));
      func = (*(grub_size_t **)IMG(0x8300))[func];

    return ((int (*)())func)(ch[0],ch[1],ch[2],ch[3],ch[4],ch[5],ch[6],ch[7],ch[8],ch[9]);
/*
#define	ABS(x)	((x) - EXT_C(main) + 0x8200)
#define	IMG(x)	((x) - 0x8200 + g4e_data)
grub_image = 112c6000
EXT_C(main) = IMG(0x8200) = 112c6400
ABS(x) = x - (EXT_C(main) - 0x8200) = x - (112c6400 - 0x8200)
IMG(x) = x + (112c6400 - 0x8200)
grub_printf("\ncall_func-1,%x,%x,%x,%x,%x,",IMG(0x8300),*(int *)IMG(0x8300),*(int **)IMG(0x8300),*(int *)IMG(*(int *)IMG(0x8300)),*(long *)IMG(*(int *)IMG(0x8300)));
112c6500,9c08,9e8800009c08,1f444,1f444;
*/
	}
	else
	{
		return run_line(arg,flags);
	}
}
/*
int (*)() 这是一个函数的指针，指向一个 int () 这样的函数。它要指向一个函数才能有用。指向一个函数之后可以用它来代替该函数。之后使用这个指针相当于使用该函数。
*/


static struct builtin builtin_call =
{
   "call",
   call_func,
  BUILTIN_BAT_SCRIPT | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_IFTITLE | BUILTIN_MENU,
};


static int exit_g4d_func(char *arg, int flags);
static int exit_g4d_func(char *arg, int flags)
{
  grub_machine_fini ();
  efi_call_4 (grub_efi_system_table->boot_services->exit,
              grub_efi_image_handle, GRUB_EFI_SUCCESS, 0, 0);
  for (;;) ;
  return 1;
}

static struct builtin builtin_exit_g4d =
{
  "exit_g4d",
  exit_g4d_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "exit",
  "exit GRUB4DOS_for_UEFI"
};


static int exit_func(char *arg, int flags);
static int exit_func(char *arg, int flags)
{
#if 0
  if (flags == BUILTIN_SCRIPT)
  {
    errnum = MAX_ERR_NUM;
  } else
#endif
  {
    long long t = 0;
    read_val(&arg, &t);
    errnum = 1000 + t;
  }
	return errnum;
}

static struct builtin builtin_exit =
{
  "exit",
  exit_func,
  BUILTIN_BAT_SCRIPT | BUILTIN_SCRIPT,
  "exit [n]",
  "Exit batch script with a status of N or Exit menu script"
};

static int shift_func(char *arg, int flags);
static int shift_func(char *arg, int flags)
{
	char **s = batch_args;
	errnum = 0;
	if (*arg == '/')
		++arg;
	unsigned int i = *arg - '0';
	if (i > 8)
		i = 0;
	while (i < 9 && s[i][0])
	{
		s[i] = s[i+1];
		++i;
	}
	if (i == 9)
	{
		s[9] = skip_to(SKIP_WITH_TERMINATE | 1,s[8]);
	}
	return 1;
}

static struct builtin builtin_shift =
{
  "shift",
  shift_func,
  BUILTIN_BAT_SCRIPT,
  "shift [[/]n]",
  "The positional parameters from %n+1 ... are renamed to %1 ...  If N is"
  " not given, it is assumed to be 0."
};

static int grub_exec_run(char *program, char *psp, int flags);
static int grub_exec_run(char *program, char *psp, int flags)
{
	int pid;
	psp_info_t *PI=(psp_info_t *)psp;
	char *arg = psp + PI->arg;
		/* kernel image is destroyed, so invalidate the kernel */
	if (kernel_type < KERNEL_TYPE_CHAINLOADER)
		kernel_type = KERNEL_TYPE_NONE;
	/*Is a batch file? */
	if (*(unsigned int *)program == BAT_SIGN || *(unsigned int *)program == 0x21BFBBEF)//BAT_SIGN=!BAT
	{
		int crlf = 0;
		if (prog_pid >= 10)
		{
			return 0;
		}
//		struct bat_array *p_bat_array = (struct bat_array *)grub_malloc(0x2600);
		struct bat_array *p_bat_array = (struct bat_array *)grub_malloc(0x200 + sizeof(struct bat_label) * 0x80 + sizeof(char *) * 0x800);
		if (p_bat_array == NULL)
			return 0;
		p_bat_array->path = psp + PI->path;
		struct bat_array *p_bat_array_orig = p_bat_prog;

		char *filename = PI->filename;
		char *p_bat = program;
		struct bat_label *label_entry =(struct bat_label *)((char *)p_bat_array + 0x200);
		char **bat_entry = (char **)(label_entry + 0x80);//0x400/sizeof(label_entry)
		unsigned int i_bat = 1,i_lab = 1;//i_bat:lines of script;i_lab=numbers of label.
		unsigned int type = 0; //type=0/1/2=Windows:(每行结尾是“\r\n”)/Unix:(每行结尾是“\n”)/Mac OS:(每行结尾是“\r”)
		grub_u32_t size = grub_strlen(program); //取批处理尺寸(注意:PI->proglen是包含尾随文件的尺寸,如果有的话)

		p_bat_array->size = size++;
		sprintf(p_bat_array->md,"(md,0x%x,0x%x)",program + size,PI->proglen - size);
		if (PI->proglen > size) //如果批处理有尾随文件,移动尾随文件,对齐0x200
		{
		bat_md_start = (grub_size_t)(program + size + 511) & (-512);
		bat_md_count = PI->proglen - size;
		grub_memmove((void *)bat_md_start,(void *)(program + size),PI->proglen - size);
		sprintf(p_bat_array->md,"(md,0x%x,0x%x)",bat_md_start,bat_md_count);
		bat_md_start >>= 9;
		bat_md_count >>= 9;
		}

		//判断回车换行模式
		if (debug_prog)
		{
			while(*p_bat++)
			{
				if (*p_bat == '\r' && *(p_bat+1) == '\n')
          crlf = 1;
        else if (*p_bat == '\r')
          type = 2;
				else if (*p_bat == '\n')
          type = 1;
        else
					continue;

				break;
			}
		}
//		program = skip_to(SKIP_LINE,program);//skip head
		if (debug_prog) bat_entry[i_bat++] = (char*)-1;
		while ((p_bat = program))//scan batch file and make label and bat entry.
		{
			program = skip_to(SKIP_LINE,program);
			if (*p_bat == ':')
			{
				nul_terminate(p_bat);
				label_entry[i_lab].label = p_bat + 1;
				label_entry[i_lab].line = i_bat;
				if (debug_prog) bat_entry[i_bat++] = (char*)-1;
				i_lab++;
			}
			else if (*(unsigned int *)p_bat != BAT_SIGN && *(unsigned int *)p_bat != 0x21BFBBEF)
				bat_entry[i_bat++] = p_bat;

			if (debug_prog)
			{
				char *p = p_bat;
				p += grub_strlen(p_bat) + crlf;
				while(++p < program)
				{
//					if (!*p || *p == '\n') bat_entry[i_bat++] = (char*)-1;
          if (type == 0 && *p == '\n')
            bat_entry[i_bat++] = (char*)-1;
          else if (type == 1 && (!*p || *p == '\n'))
            bat_entry[i_bat++] = (char*)-1;
          if (type == 2 && (!*p || *p == '\r'))
            bat_entry[i_bat++] = (char*)-1;
				}
			}
#if 0
			if ((i_lab & 0x80) || (i_bat & 0x800))//max label 128,max script line 2048.
			{
				grub_free(p_bat_array);
				return 0;
			}
#else
      if ((i_lab & 0x80) || (i_bat & 0x800))
        break;
#endif
		}

		label_entry[i_lab].label = NULL;
		bat_entry[i_bat] = NULL;
		label_entry[0].label = "eof";
		label_entry[0].line = i_bat;
		p_bat_array->pid = prog_pid;
		p_bat_array->debug_break = 0;
		p_bat_array->entry = label_entry;
		p_bat_prog = p_bat_array;

		pid = bat_run_script(filename, arg,flags | BUILTIN_BAT_SCRIPT | BUILTIN_USER_PROG);//run batch script from line 0;
		p_bat_prog = p_bat_array_orig;

		grub_free(p_bat_array);
		return pid;
	}
	
	/* call the new program. */
//	pid = ((int (*)(char *,int))program)(arg, flags | BUILTIN_USER_PROG);/* pid holds return value. */
  pid = ((int (*)(char *,int,int,int))program)(arg, flags | BUILTIN_USER_PROG, 0, 0);/* pid holds return value. */
	return pid;
}


static int else_func(char *arg, int flags);
static int else_func(char *arg, int flags)
{
	if (else_disabled & (1 << brace_nesting)) //如果else禁止
  {
    return !(errnum = ERR_BAT_BRACE_END);
  }
  else                                      //如果else允许
  {
    if (*arg == 'i')  //如果是 else if
    {
      is_else_if = 1; //告知 if 函数, 这是 else_if
      return  builtin_cmd (0,arg,flags);
    }
    else              //如果是 else
      return 1;
  }
}

static struct builtin builtin_else =
{
  "else",
  else_func,
  BUILTIN_BAT_SCRIPT | BUILTIN_SCRIPT,      //使用于批处理脚本及脚本
};

static inline unsigned char inb (unsigned short p)
{
  unsigned char r;
  asm ("inb %1, %0" : "=a" (r) : "dN" (p));
  return r;
}

static inline void outb (unsigned short p, unsigned char d)
{
  asm ("outb %1, %0" : : "dN" (p), "a" (d));
}

static void
speaker_play (unsigned int hz, unsigned long time)
{
  unsigned int div = 0;
  unsigned long ms = 1000 * time;
  if (hz == 0)
  {
    outb (0x61, inb (0x61) & 0xfc);
    efi_call_1 (grub_efi_system_table->boot_services->stall, ms);
    return;
  }
  if (hz < 20)
    hz = 20;
  if (hz > 20000)
    hz = 20000;
  div = 1193180 / hz;
  /* speaker freq */
  outb (0x43, 0xb6);
  outb (0x42, (unsigned char) div);
  outb (0x42, (unsigned char) (div >> 8));
  /* speaker on */
  outb (0x61, inb (0x61) | 0x3);
  /* sleep */
  efi_call_1 (grub_efi_system_table->boot_services->stall, ms);
  /* speaker off */
  outb (0x61, inb (0x61) & 0xfc);
}

static int beep_func (char *arg, int flags);
static int beep_func (char *arg, int flags)
{
  unsigned int hz;
  unsigned long time;
  unsigned long long val;
  while (1)
  {
    if (grub_memcmp(arg,"--play=",7) == 0)
    {
    }
    else if (grub_memcmp(arg,"--start",7) == 0)
    {
    }
    else if (grub_memcmp(arg,"--mid",5) == 0)
    {
    }
    else if (grub_memcmp(arg,"--end",5) == 0)
    {
    }
    else if (grub_memcmp(arg,"--nowait",8) == 0)
    {
    }
    else
      break;
    arg = skip_to (0, arg);
  }

  while (*arg)
  {    
    if (safe_parse_maxint (&arg, &val))
      hz = val;
    else
      break;
    arg = skip_to (0, arg);
    if (safe_parse_maxint (&arg, &val))
      time = val;
    else
      break;
    speaker_play (hz, time);
    arg = skip_to (0, arg);
  }
  speaker_play (0, 0);
  return 1;
}

static struct builtin builtin_beep =
{
  "beep",
  beep_func,
  BUILTIN_BAT_SCRIPT | BUILTIN_SCRIPT | BUILTIN_CMDLINE | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "beep FREQUENCY DURATION FREQUENCY DURATION ...",
  "FREQUENCY: Hz. DURATION: ms."
};

/* The table of builtin commands. Sorted in dictionary order.  */
struct builtin *builtin_table[] =
{
  &builtin_beep,
  &builtin_blocklist,
  &builtin_boot,
  &builtin_calc,
  &builtin_call,
  &builtin_cat,
  &builtin_chainloader,
  &builtin_checkrange,
  &builtin_checktime,
  &builtin_clear,
  &builtin_cmp,
  &builtin_color,
  &builtin_command,
  &builtin_commandline,
  &builtin_configfile,
  &builtin_crc32,
  &builtin_dd,
  &builtin_debug,
  &builtin_default,
  &builtin_delmod,
  &builtin_displaymem,
  &builtin_echo,
  &builtin_else,
  &builtin_endlocal,
  &builtin_errnum,
  &builtin_errorcheck,
  &builtin_exit,
  &builtin_exit_g4d,
  &builtin_fallback,
  &builtin_find,
#ifdef SUPPORT_GRAPHICS
  &builtin_font,
#endif
  &builtin_fstest,
  &builtin_geometry,
  &builtin_goto,
  &builtin_graphicsmode,
  &builtin_halt,
  &builtin_help,
  &builtin_hiddenflag,
  &builtin_hiddenmenu,
  &builtin_hide,
  &builtin_if,
  &builtin_iftitle,
  &builtin_initrd,
  &builtin_initscript,
  &builtin_insmod,
#ifdef FSYS_IPXE
//  &builtin_ipxe,
#endif
  &builtin_is64bit,
  &builtin_kernel,
  &builtin_load,
  &builtin_lock,
  &builtin_ls,
  &builtin_makeactive,
  &builtin_map,
#ifdef USE_MD5_PASSWORDS
  &builtin_md5crypt,
#endif /* USE_MD5_PASSWORDS */
  &builtin_module,
  &builtin_modulenounzip,
#ifdef SUPPORT_GRAPHICS
//  &builtin_outline,
#endif /* SUPPORT_GRAPHICS */
  &builtin_pager,
  &builtin_partnew,
  &builtin_parttype,
  &builtin_password,
  &builtin_pause,
#ifdef FSYS_PXE
  &builtin_pxe,
#endif
#ifndef NO_DECOMPRESSION
  &builtin_raw,
#endif
  &builtin_read,
  &builtin_reboot,
  &builtin_root,
  &builtin_rootnoverify,
  &builtin_savedefault,
  &builtin_set,
  &builtin_setkey,
  &builtin_setlocal,
  &builtin_setmenu,
  &builtin_shift,
#ifdef SUPPORT_GRAPHICS
  &builtin_splashimage,
#endif /* SUPPORT_GRAPHICS */
#if defined(SUPPORT_GRAPHICS)
  &builtin_terminal,
#endif /* SUPPORT_SERIAL || SUPPORT_HERCULES SUPPORT_GRAPHICS */
  &builtin_timeout,
  &builtin_title,
  &builtin_tpm,
  &builtin_uuid,
  &builtin_vol,
  &builtin_write,
  0
};
