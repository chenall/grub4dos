/*
 *  PXE file system for GRUB
 *
 *  Copyright (C) 2007 Bean (bean123@126.com)
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
#ifdef FSYS_PXE

#include "shared.h"
#include "filesys.h"
#include "pxe.h"

#include "etherboot.h"

#ifdef GRUB_UTIL

int pxe_mount (void) { return 0; }
unsigned long pxe_read (char *buf, unsigned long len, unsigned long write) { return -1; }
int pxe_dir (char *dirname) { return 0; }
void pxe_close (void) {}

#else

#ifndef TFTP_PORT
#define TFTP_PORT	69
#endif

//#define PXE_MIN_BLKSIZE	512
//#define PXE_MAX_BLKSIZE	1432
#define PXE_MIN_BLKSIZE	128
#define PXE_MAX_BLKSIZE	16384

#define DOT_SIZE	1048576

/* PXE_BUF must be in low memory */
//#define PXE_BUF		FSYS_BUF
//#define PXE_BUFLEN	FSYS_BUFLEN
/* use disk buffer for PXE_BUF */
#define PXE_BUF		BUFFERADDR
#define PXE_BUFLEN	BUFFERLEN

//#define PXE_DEBUG	1

unsigned long pxe_entry = 0, pxe_blksize = 512 /*PXE_MAX_BLKSIZE*/;
unsigned short pxe_basemem, pxe_freemem;
unsigned long pxe_keep;

//IP4 pxe_yip, pxe_sip, pxe_gip;
UINT8 pxe_mac_len, pxe_mac_type;
MAC_ADDR pxe_mac;
static UINT8 pxe_tftp_opened;
static unsigned long pxe_saved_pos, pxe_cur_ofs, pxe_read_ofs;

extern PXENV_TFTP_OPEN_t pxe_tftp_open;	/* now it is defined in asm.S */
static char *pxe_tftp_name;

extern unsigned long ROM_int15;
extern unsigned long ROM_int13;
extern unsigned long ROM_int13_dup;
extern struct drive_map_slot bios_drive_map[DRIVE_MAP_SIZE + 1];

static char* pxe_outhex (char* pc, unsigned char c)
{
  int i;

  pc += 2;
  for (i = 1; i <= 2; i++)
    {
      unsigned char t;

      t = c & 0xF;
      if (t >= 10)
        t += 'A' - 10;
      else
        t += '0';
      *(pc - i) = t;
      c = c >> 4;
    }
  return pc;
}

static unsigned long pxe_read_blk (unsigned long buf, int num);

static int try_blksize (int tmp)
{
	unsigned long nr;

	pxe_blksize = tmp;
	grub_printf ("\nTry block size %d ...\n", pxe_blksize);
	if ((tmp = pxe_dir (pxe_tftp_name)))
	{
	    if (filemax <= pxe_blksize)
	    {
		grub_printf ("\nFailure: Size %ld too small.\n", filemax);
	        pxe_close ();
		return 1;
	    }
	    nr = pxe_read_blk (PXE_BUF, 1);
	    if (nr == PXE_ERR_LEN)
	    {
		grub_printf ("\nFailure: Cannot read the first block.\n");
	        pxe_close ();
		return 1;
	    }
	    if (pxe_blksize != nr && filemax > nr && nr <= PXE_MAX_BLKSIZE && nr >= PXE_MIN_BLKSIZE)
	    {
		grub_printf ("\npxe_blksize tuned from %d to %d\n", pxe_blksize, nr);
		pxe_blksize = nr;
	    }
	    grub_printf ("\nUse block size %d\n", pxe_blksize);
	    pxe_close ();
	    return 0;
	}
	return 1;	/* return 0 for seccess, 1 for failure */
}

unsigned long pxe_inited = 0;	/* pxe_detect only run once */
BOOTPLAYER *discover_reply = 0;

int pxe_detect (int blksize, char *config)	//void pxe_detect (void)
{
  unsigned long tmp;
  char *pc;
  int i, ret;

//  if (pxe_inited)
//    return 0;

  if (! pxe_entry)	//if (! pxe_scan ())
    return 0;

  pxe_inited = 1;

  if (discover_reply->bootfile[0])
    {
	int n;

	grub_printf ("\nbootfile is %s\n", discover_reply->bootfile);
	n = grub_strlen ((char*)discover_reply->bootfile) - 1;
	grub_strcpy ((char*)&pxe_tftp_open.FileName, (char*)discover_reply->bootfile);
	while ((n >= 0) && (pxe_tftp_open.FileName[n] != '/')) n--;
	if (n < 0)	/* need to add a slash */
	{
		pxe_tftp_open.FileName[0] = '/';
		grub_strcpy (((char*)&pxe_tftp_open.FileName) + 1, (char*)discover_reply->bootfile);
		n = 0;
	}
	pxe_tftp_name = (char*)&pxe_tftp_open.FileName[n];

	/* read the boot file to determine the block size. */

	if (blksize)
		pxe_blksize = blksize;
	else if (try_blksize (1408) && try_blksize (512))
	{
		pxe_blksize = 512;	/* default to 512 */
		grub_printf ("\nCannot open %s, pxe_blksize set to %d\n", pxe_tftp_name, pxe_blksize);
	}

      //grub_strcpy (pxe_tftp_name, "/menu.lst/");
    }
  else
    {
	pxe_blksize = (blksize ? blksize : 512);	/* default to 512 */
	grub_printf ("\nNo bootfile! pxe_blksize set to %d\n", pxe_blksize);
	pxe_tftp_name = (char*)&pxe_tftp_open.FileName[0];
    }

  pxe_tftp_opened = 0;

  ret = 0;

  grub_memcpy ((char *) saved_pxe_mac, (char *) pxe_mac, 6);
  saved_pxe_ip = pxe_yip;

  if (config)
  {

	if (*config == '/')
	{
		int n;
		n = grub_strlen (config) - 1;
		if (config[n] != '/')
		{
			grub_strcpy (pxe_tftp_name, config);
			grub_printf ("%s\n", pxe_tftp_open.FileName);
			ret = pxe_dir (pxe_tftp_name);
			goto done;
		}
	} 
	else
		return 1;
  }

	//if (pxe_dir ("/menu.lst"))
	//{
	//	grub_strcpy (pxe_tftp_name, "/menu.lst");
	//	ret = 1;
	//	goto done;
	//}

	grub_strcpy (pxe_tftp_name, "/menu.lst");
	#if 0
	grub_printf ("\nFrom now on, we first try to open this FILE: %s\n"
		     "\nCaution: The PXE server should NOT have a DIR of \"%s\"."
		     "\nCaution: The system could hang if \"%s\" is a DIR."
		     "\nCaution: Your original \"menu.lst\" DIR should be renamed to \"menu\".\n"
			, pxe_tftp_open.FileName
			, pxe_tftp_open.FileName
			, pxe_tftp_open.FileName);
	#endif
	ret = pxe_dir (pxe_tftp_name);
	if (ret && filemax)
		goto done;

  /* Reports from Ruymbeke: opening /menu.lst will hang if it is a dir.
   * Do NOT use /menu.lst as a dir any more!! Use /menu for it instead.
   */
	if (!config)
		config = "/menu.lst/";

  grub_strcpy (pxe_tftp_name, config);
	
//#define MENU_DIR_NAME_LENGTH (sizeof("/menu.lst/") - 1)
	int MENU_DIR_NAME_LENGTH = grub_strlen(config);

  pc = pxe_tftp_name + MENU_DIR_NAME_LENGTH;
  pc = pxe_outhex (pc, pxe_mac_type);
  for (i = 0; i < pxe_mac_len; i++)
    {
      *(pc++) = '-';
      pc = pxe_outhex (pc, pxe_mac[i]);
    }
  *pc = 0;
  grub_printf ("\n%s\n", pxe_tftp_open.FileName);
  if (pxe_dir (pxe_tftp_name))
    {
      ret = 1;
      goto done;
    }

  pc = pxe_tftp_name + MENU_DIR_NAME_LENGTH;
  tmp = pxe_yip;
  for (i = 0; i < 4; i++)
    {
      pc = pxe_outhex (pc, tmp & 0xFF);
      tmp >>= 8;
    }
  *pc = 0;
  do
    {
      grub_printf ("%s\n", pxe_tftp_open.FileName);
      if (pxe_dir (pxe_tftp_name))
        {
          ret = 1;
          goto done;
        }
      *(--pc) = 0;
    } while (pc > pxe_tftp_name + MENU_DIR_NAME_LENGTH);
  grub_strcpy (pc, "default");
  grub_printf ("%s\n", pxe_tftp_open.FileName);
  ret = pxe_dir (pxe_tftp_name);

#undef MENU_DIR_NAME_LENGTH

done:

  if (ret && filemax)
    {
#if 1
	char *new_config = config_file;
	char *filename = pxe_tftp_name;
	if (debug > 1)
	{
		grub_printf("PXE boot configfile:%s\n",(char *)pxe_tftp_open.FileName);
		DEBUG_SLEEP
	}
	pxe_close ();
	/* got file name. put it in config_file */
	if (grub_strlen (filename) >= ((char *)0x8270 - new_config))
		return ! (errnum = ERR_WONT_FIT);
	/* set (pd) as root device. */
	saved_drive = PXE_DRIVE;
	saved_partition = 0xFFFFFF;
	/* Copy FILENAME to CONFIG_FILE.  */
	while ((*new_config++ = *filename++) != 0);
	if (pxe_restart_config == 0)
	{
		pxe_restart_config = 1;
		return ret;
	}
	use_config_file = 1;

	/* Make sure that the user will not be authoritative.  */
	auth = 0;
  
	buf_drive = -1;	/* invalidate disk cache. */
	buf_track = -1;	/* invalidate disk cache. */
	saved_entryno = 0;
	//force_cdrom_as_boot_device = 0;
	boot_drive = saved_drive;
	install_partition = saved_partition;
	current_drive = GRUB_INVALID_DRIVE;
	current_partition = 0xFFFFFF;
	fsys_type = NUM_FSYS;
	boot_part_addr = 0;
	current_slice = 0;

	///* Restart pre_stage2.  */
	//(*(char *)0x8205) |= 2;	/* disable keyboard intervention */
	//chain_stage1(0, 0x8200, boot_part_addr);
	/* Restart cmain.  */
	asm volatile ("movl $0x7000, %esp");	/* set stack to STACKOFF */
#ifdef HAVE_ASM_USCORE
	asm volatile ("call _cmain");
	asm volatile ("jmp _stop");
#else
	asm volatile ("call cmain");
	asm volatile ("jmp stop");
#endif

	/* Never reach here.  */
#else
      unsigned long nr;

      nr = 4096 - 1;
      if (nr > filemax)
        nr = filemax;
      nr = pxe_read ((char*)0x800, nr);
      if (nr != PXE_ERR_LEN)
        {
          *(char*)(0x800 + nr) = 0;

          if (preset_menu != (char*)0x800)
            preset_menu = (char*)0x800;
	  if (*config_file)
	      *config_file = 0;		/* only use preset_menu with pxe */

          if (nr < filemax)
            {
              grub_printf ("Boot menu truncated\n");
              //pxe_read (NULL, filemax - nr);
            }
        }
      pxe_close ();
#endif
    }
  //getkey();
  return ret;
}

#if PXE_TFTP_MODE

static int pxe_reopen (void)
{
  pxe_close ();

  pxe_call (PXENV_TFTP_OPEN, &pxe_tftp_open);
  if (pxe_tftp_open.Status)
  {
    return 0;
  }
  pxe_blksize = pxe_tftp_open.PacketSize;
  pxe_saved_pos = pxe_cur_ofs = pxe_read_ofs = 0;

  pxe_tftp_opened = 1;

  return 1;
}

static int pxe_open (char* name)
{
  PXENV_TFTP_GET_FSIZE_t *tftp_get_fsize;

  pxe_close ();

  tftp_get_fsize = (void*)&pxe_tftp_open;
  tftp_get_fsize->ServerIPAddress = pxe_sip;
  tftp_get_fsize->GatewayIPAddress = pxe_gip;

  if (name != pxe_tftp_name)
    grub_strcpy (pxe_tftp_name, name);

  pxe_call (PXENV_TFTP_GET_FSIZE, tftp_get_fsize);
  filemax = tftp_get_fsize->FileSize;
  filepos = 0;
  if (tftp_get_fsize->Status)
  {
    pxe_tftp_opened = 0;
    pxe_saved_pos = pxe_cur_ofs = pxe_read_ofs = 0;
    return 0;
  }



  /* we have to replace pxe_tftp_open.TFTPPort with tftp_get_fsize->FileSize
   * to avoid compiler optimization issue.  */
  //pxe_tftp_open.TFTPPort = htons (TFTP_PORT);
  tftp_get_fsize->FileSize = htons (TFTP_PORT);
  pxe_tftp_open.PacketSize = pxe_blksize;

  return pxe_reopen ();
}

void pxe_close (void)
{
  if (pxe_tftp_opened)
    {
      PXENV_TFTP_CLOSE_t tftp_close;

      pxe_call (PXENV_TFTP_CLOSE, &tftp_close);
      pxe_tftp_opened = 0;
      pxe_saved_pos = pxe_cur_ofs = pxe_read_ofs = 0;
    }
}

#if PXE_FAST_READ

/* Read num packets , BUF must be segment aligned */
static unsigned long pxe_read_blk (unsigned long buf, int num)
{
  PXENV_TFTP_READ_t tftp_read;
  unsigned long ofs;

  tftp_read.Buffer = SEGOFS(buf);
  ofs = tftp_read.Buffer & 0xFFFF;
  pxe_fast_read (&tftp_read, num);

  /* disk cache destroyed, so invalidate it. */
  buf_drive = -1;
  buf_track = -1;
  return (tftp_read.Status) ? PXE_ERR_LEN : ((tftp_read.Buffer & 0xFFFF) - ofs);
}

#else

static unsigned long pxe_read_blk (unsigned long buf, int num)
{
  PXENV_TFTP_READ_t tftp_read;
  unsigned long ofs;

  /* disk cache will be destroyed, so invalidate it. */
  buf_drive = -1;
  buf_track = -1;

  tftp_read.Buffer = SEGOFS(buf);
  ofs = tftp_read.Buffer & 0xFFFF;
  while (num > 0)
    {
      pxe_call (PXENV_TFTP_READ, &tftp_read);
      if (tftp_read.Status)
        return PXE_ERR_LEN;
      tftp_read.Buffer += tftp_read.BufferSize;
      if (tftp_read.BufferSize < pxe_blksize)
        break;
      num--;
    }
  return (tftp_read.Buffer & 0xFFFF) - ofs;
}

#endif

#else
#endif

static unsigned long pxe_read_len (unsigned long long buf, unsigned long long len)
{
  unsigned long old_ofs, sz;

  if (len == 0)
    return 0;

  sz = 0;
  old_ofs = pxe_cur_ofs;
  pxe_cur_ofs += len;
  if (pxe_cur_ofs > pxe_read_ofs)
    {
      unsigned long nb, nr;
      long nb_del, nb_pos;

      sz = (pxe_read_ofs - old_ofs);
      if ((buf) && (sz))
        {
          grub_memmove64 (buf, (unsigned long long)(unsigned int)(char*)(PXE_BUF + old_ofs), sz);
          buf += sz;
        }
      pxe_cur_ofs -= pxe_read_ofs;	/* bytes to read */
      nb = pxe_cur_ofs / pxe_blksize;	/* blocks to read */
      nb_del = DOT_SIZE / pxe_blksize;
      if (nb_del > nb)
        {
          nb_del = 0;
          nb_pos = -1;
        }
      else
        nb_pos = nb - nb_del;
      pxe_cur_ofs -= pxe_blksize * nb;	/* bytes residual */
      if (pxe_read_ofs + pxe_blksize > PXE_BUFLEN)
        pxe_read_ofs = 0;
      while (nb > 0)
        {
          unsigned long nn;

          nn = (PXE_BUFLEN - pxe_read_ofs) / pxe_blksize;
          if (nn > nb)
            nn = nb;
          nr = pxe_read_blk (PXE_BUF + pxe_read_ofs, nn);
          if (nr == PXE_ERR_LEN)
            return nr;
          sz += nr;
          if (buf)
            {
              grub_memmove64 (buf, (unsigned long long)(unsigned int)(char*)(PXE_BUF + pxe_read_ofs), nr);
              buf += nr;
            }
          if (nr < nn * pxe_blksize)
            {
              pxe_read_ofs += nr;
              pxe_cur_ofs = pxe_read_ofs;
              return sz;
            }
          nb -= nn;
          if (nb)
            pxe_read_ofs = 0;
          else
            pxe_read_ofs += nr;
          if ((long)nb <= nb_pos)
            {
              grub_putchar ('.');
              nb_pos -= nb_del;
            }
        }

      if (nb_del)
        {
          grub_putchar ('\r');
          grub_putchar ('\n');
        }

      if (pxe_cur_ofs)
        {
          if (pxe_read_ofs + pxe_blksize > PXE_BUFLEN)
            pxe_read_ofs = 0;

          nr = pxe_read_blk (PXE_BUF + pxe_read_ofs, 1);
          if (nr == PXE_ERR_LEN)
            return nr;
          if (pxe_cur_ofs > nr)
            pxe_cur_ofs = nr;
          sz += pxe_cur_ofs;
          if (buf)
            grub_memmove64 (buf, (unsigned long long)(unsigned int)(char*)(PXE_BUF + pxe_read_ofs), pxe_cur_ofs);
          pxe_cur_ofs += pxe_read_ofs;
          pxe_read_ofs += nr;
        }
      else
        pxe_cur_ofs = pxe_read_ofs;
    }
  else
    {
      sz += len;
      if (buf)
        grub_memmove64 (buf, (unsigned long long)(unsigned int)(char *)PXE_BUF + old_ofs, len);
    }
  return sz;
}

/* Mount the network drive. If the drive is ready, return 1, otherwise
   return 0. */
int pxe_mount (void)
{
  if (current_drive != PXE_DRIVE)
    return 0;

  return 1;
}

/* Read up to SIZE bytes, returned in ADDR.  */
unsigned long long
pxe_read (unsigned long long buf, unsigned long long len, unsigned long write)
{
  unsigned long nr;

  if (write == 0x900ddeed)
    return !(errnum = ERR_WRITE);

  if (! pxe_tftp_opened)
    return PXE_ERR_LEN;

  if (pxe_saved_pos != filepos)
    {
      if ((filepos < pxe_saved_pos) && (filepos+pxe_cur_ofs >= pxe_saved_pos))
        pxe_cur_ofs -= pxe_saved_pos - filepos;
      else
        {
          if (pxe_saved_pos > filepos)
            {
              if (! pxe_reopen ())
                return PXE_ERR_LEN;
            }

          nr = pxe_read_len (0ULL, filepos - pxe_saved_pos);
          if ((nr == PXE_ERR_LEN) || (pxe_saved_pos + nr != filepos))
            return PXE_ERR_LEN;
        }
      pxe_saved_pos = filepos;
    }
  nr = pxe_read_len (buf, len);
  if (nr != PXE_ERR_LEN)
    {
      filepos += nr;
      pxe_saved_pos = filepos;
    }
  return nr;
}

/* Check if the file DIRNAME really exists. Get the size and save it in
   FILEMAX. return 1 if succeed, 0 if fail.  */
int pxe_dir (char *dirname)
{
  int ret;
  char ch;
  ret = 1;
  ch = nul_terminate (dirname);

  if (print_possibilities)
  {
		char dir_tmp[128];
		char *p_dir;
		ret = grub_strlen(dirname);
		p_dir = &dirname[ret];
		*p_dir = ch;
		if (ret && ret <=120)
		{
			while (ret && dirname[ret] != '/') 
			{
				ret--;
			}
			grub_memmove(dir_tmp,dirname,ret);
		}
		else
			ret = 0;

		grub_sprintf(&dir_tmp[ret],"/dir.txt");
		if (pxe_open(dir_tmp))
		{
			char *dir_buff=grub_malloc(filemax+1);
			int found = 0;
			if (dir_buff && pxe_read((unsigned long long)(int)dir_buff,-1,GRUB_READ))
			{
				dir_buff[filemax] = '\0';
				char *p,*p1;
				p1 = dir_buff;
				*p_dir = '\0';
				dirname += ret + 1;
				while ((p = p1))
				{
					p1 = skip_to(0x100,p);
					nul_terminate(p);
					if (*dirname == 0 || substring (dirname, p, 1) < 1)
					{
						found = 1;
						print_a_completion(p, 1);
					}
				}
				*p_dir = ch;
				grub_free(dir_buff);
			}
			pxe_close();
			if (found)
				return 1;
		}
		errnum = ERR_FILE_NOT_FOUND;
		return 0;
  }
  pxe_close ();
  if (! pxe_open (dirname))
    {
      errnum = ERR_FILE_NOT_FOUND;
      ret = 0;
    }

  dirname[grub_strlen(dirname)] = ch;
  return ret;
}

void pxe_unload (void)
{
  PXENV_UNLOAD_STACK_t unload;
  unsigned char code[] = {PXENV_UNDI_SHUTDOWN, PXENV_UNLOAD_STACK, PXENV_STOP_UNDI, 0};
  int i, h;

  if (! pxe_entry)
    return;

  pxe_close ();

  if (pxe_keep)
    return;

  h = unset_int13_handler (1);	/* check if it was set. */
  if (! h)	/* h==0 for set */
    unset_int13_handler (0);	/* unset it */

  /* at this moment, there should be no other handlers.
   * if other handlers do exist, we should quit.
   */
  if (*((unsigned short *)0x413) != pxe_basemem)
  {
	grub_printf ("PXE unload failed because of an unknown handler(e.g., of int13) loaded.\n");
	goto quit;
  }

  i = 0;
  while (code[i])
    {
      grub_memset (&unload, 0, sizeof(unload));
      pxe_call (code[i], &unload);
      if (unload.Status)
        {
          grub_printf ("PXE unload fails: %d\n", unload.Status);
          goto quit;
        }
      i++;
    }
  //if (*((unsigned short *)0x413) == pxe_basemem)
      *((unsigned short *)0x413) = pxe_freemem;
  pxe_entry = 0;
  ROM_int15 = *((unsigned long *)0x54);
  ROM_int13 = ROM_int13_dup = *((unsigned long *)0x4C);
  grub_printf ("PXE stack unloaded\n");
quit:
  if (! h)	/* h==0 for set */
    set_int13_handler (bios_drive_map);
}

static void print_ip (IP4 ip)
{
  int i;

  for (i = 0; i < 3; i++)
    {
      grub_printf ("%d.", (unsigned long)(unsigned char)ip);
      ip >>= 8;
    }
  grub_printf ("%d", (unsigned long)(unsigned char)ip);
}

int pxe_func (char *arg, int flags)
{
  if (! pxe_entry)
    {
      grub_printf ("No PXE stack\n");
      goto bad_argument;
    }
  if (*arg == 0)
    {
      char buf[4], *pc;
      int i;

      pxe_tftp_name[0] = '/';
      pxe_tftp_name[1] = 0;
      grub_printf ("blksize : %d\n", pxe_blksize);
      grub_printf ("basedir : %s\n", pxe_tftp_open.FileName);
      grub_printf ("bootfile: %s\n", discover_reply->bootfile);
      grub_printf ("client ip  : ");
      print_ip (pxe_yip);
      grub_printf ("\nserver ip  : ");
      print_ip (pxe_sip);
      grub_printf ("\ngateway ip : ");
      print_ip (pxe_gip);
      grub_printf ("\nmac : ");
      for (i = 0; i < pxe_mac_len; i++)
        {
          pc = buf;
          pc = pxe_outhex (pc, pxe_mac[i]);
          *pc = 0;
          grub_printf ("%s%c", buf, ((i == pxe_mac_len - 1) ? '\n' : '-'));
        }
    }
  else if (grub_memcmp(arg, "blksize", sizeof("blksize") - 1) == 0)
    {
      unsigned long long val;

      arg = skip_to (0, arg);
      if (! safe_parse_maxint (&arg, &val))
        return 0;
      if (val > PXE_MAX_BLKSIZE)
        val = PXE_MAX_BLKSIZE;
      if (val < PXE_MIN_BLKSIZE)
        val = PXE_MIN_BLKSIZE;
      pxe_blksize = val;
    }
  else if (grub_memcmp (arg, "basedir", sizeof("basedir") - 1) == 0)
    {
      int n;

      arg = skip_to (0, arg);
      if (*arg == 0)
        {
          grub_printf ("No pathname\n");
	  goto bad_argument;
        }
      if (*arg != '/')
        {
          grub_printf ("Base directory must start with /\n");
	  goto bad_argument;
        }
      n = grub_strlen (arg);
      if (n > sizeof(pxe_tftp_open.FileName) - 8)
        {
          grub_printf ("Path too long\n");
	  goto bad_argument;
        }
      grub_strcpy ((char*)pxe_tftp_open.FileName, arg);
      n--;
      while ((n >= 0) && (pxe_tftp_open.FileName[n] == '/'))
        n--;
      pxe_tftp_name = (char*)&pxe_tftp_open.FileName[n + 1];
    }
  else if (grub_memcmp (arg, "keep", sizeof("keep") - 1) == 0)
    pxe_keep = 1;
  else if (grub_memcmp (arg, "nokeep", sizeof("nokeep") - 1) == 0)
    pxe_keep = 0;
  else if (grub_memcmp (arg, "unload", sizeof("unload") - 1) == 0)
    {
      pxe_keep = 0;
      pxe_unload ();
    }
#ifdef PXE_DEBUG
  else if (grub_memcmp (arg, "dump", sizeof("dump") - 1) == 0)
    {
      PXENV_GET_CACHED_INFO_t get_cached_info;
      BOOTPLAYER *bp;
      unsigned long long val;

      arg = skip_to (0, arg);
      if (! safe_parse_maxint (&arg, &val))
        return 0;
      if ((val < 1) || (val > 3))
        {
          grub_printf ("Invalid type\n");
	  goto bad_argument;
        }
      get_cached_info.PacketType = val;
      get_cached_info.Buffer = get_cached_info.BufferSize = 0;
      pxe_call (PXENV_GET_CACHED_INFO, &get_cached_info);
      if (get_cached_info.Status)
        return 0;
      bp = LINEAR(get_cached_info.Buffer);

      grub_printf ("%X\n", (unsigned long)bp);
      hexdump (0, bp, get_cached_info.BufferSize);
    }
#endif
  else if (grub_memcmp (arg, "detect", sizeof("detect") - 1) == 0)
    {
	unsigned long long blksize = 0;
	/* pxe_detect should be done before any other command. */
	arg = skip_to (0, arg);
	if (*arg != '/')
	{
		if (*arg >= '0' && *arg <= '9')
		{
			if (! safe_parse_maxint (&arg, &blksize))
				return 0;
			arg = skip_to (0, arg);
			if (*arg == 0)
				arg = 0;
			//else if (*arg != '/')
			//	goto bad_argument;
		} else if (*arg == 0)
			arg = 0;
		//else
		//	goto bad_argument;
	}
	return pxe_detect ((int)blksize, arg);
    }
  else
    {
bad_argument:
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }
  return 1;
}

#endif

#endif
