/* boot.c - load and bootstrap a kernel */
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


#include "shared.h"
#include <term.h>
#include "cpio.h"
#include "freebsd.h"

struct exec
  {
    unsigned long a_midmag;	/* htonl(flags<<26 | mid<<16 | magic) */
    unsigned long a_text;	/* text segment size */
    unsigned long a_data;	/* initialized data size */
    unsigned long a_bss;	/* uninitialized data size */
    unsigned long a_syms;	/* symbol table size */
    unsigned long a_entry;	/* entry point */
    unsigned long a_trsize;	/* text relocation size */
    unsigned long a_drsize;	/* data relocation size */
  };
#define ntohl(x) ((x << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | (x >> 24))
#define htonl(x) ntohl(x)
#define N_GETMAGIC(ex) 	( (ex).a_midmag & 0xffff )
#define N_GETMAGIC_NET(ex) 	(ntohl((ex).a_midmag) & 0xffff)
#define	OMAGIC          0x107	/* 0407 old impure format */
#define	NMAGIC          0x108	/* 0410 read-only text */
#define	ZMAGIC          0x10b	/* 0413 demand load format */
#define QMAGIC          0xcc	/* 0314 "compact" demand load format */
#define	N_BADMAG(ex) \
	(N_GETMAGIC(ex) != OMAGIC && N_GETMAGIC(ex) != NMAGIC && \
	 N_GETMAGIC(ex) != ZMAGIC && N_GETMAGIC(ex) != QMAGIC && \
	 N_GETMAGIC_NET(ex) != OMAGIC && N_GETMAGIC_NET(ex) != NMAGIC && \
	 N_GETMAGIC_NET(ex) != ZMAGIC && N_GETMAGIC_NET(ex) != QMAGIC)
#define __LDPGSZ 0x1000
#define	N_TXTOFF(ex) \
	(N_GETMAGIC(ex) == ZMAGIC ? __LDPGSZ : (N_GETMAGIC(ex) == QMAGIC || \
	N_GETMAGIC_NET(ex) == ZMAGIC) ? 0 : sizeof(struct exec))

/* ELF header */
typedef struct
{
  
#define EI_NIDENT 16
  
  /* first four characters are defined below */
#define EI_MAG0		0
#define ELFMAG0		0x7f
#define EI_MAG1		1
#define ELFMAG1		'E'
#define EI_MAG2		2
#define ELFMAG2		'L'
#define EI_MAG3		3
#define ELFMAG3		'F'
  
#define EI_CLASS	4	/* data sizes */
#define ELFCLASS32	1	/* i386 -- up to 32-bit data sizes present */
  
#define EI_DATA		5	/* data type and ordering */
#define ELFDATA2LSB	1	/* i386 -- LSB 2's complement */
  
#define EI_VERSION	6	/* version number.  "e_version" must be the same */
#define EV_CURRENT      1	/* current version number */

#define EI_OSABI	7	/* operating system/ABI indication */
#define ELFOSABI_FREEBSD	9
  
#define EI_ABIVERSION	8	/* ABI version */
  
#define EI_PAD		9	/* from here in is just padding */
  
#define EI_BRAND	8	/* start of OS branding (This is
				   obviously illegal against the ELF
				   standard.) */
  
  unsigned char e_ident[EI_NIDENT];	/* basic identification block */
  
#define ET_EXEC		2	/* we only care about executable types */
  unsigned short e_type;		/* file types */
  
#define EM_386		3	/* i386 -- obviously use this one */
  unsigned short e_machine;	/* machine types */
  unsigned long e_version;	/* use same as "EI_VERSION" above */
  unsigned long e_entry;	/* entry point of the program */
  unsigned long e_phoff;	/* program header table file offset */
  unsigned long e_shoff;	/* section header table file offset */
  unsigned long e_flags;	/* flags */
  unsigned short e_ehsize;		/* elf header size in bytes */
  unsigned short e_phentsize;	/* program header entry size */
  unsigned short e_phnum;		/* number of entries in program header */
  unsigned short e_shentsize;	/* section header entry size */
  unsigned short e_shnum;		/* number of entries in section header */
  
#define SHN_UNDEF       0
#define SHN_LORESERVE   0xff00
#define SHN_LOPROC      0xff00
#define SHN_HIPROC      0xff1f
#define SHN_ABS         0xfff1
#define SHN_COMMON      0xfff2
#define SHN_HIRESERVE   0xffff
  unsigned short e_shstrndx;	/* section header table index */
}
Elf32_Ehdr;


#define BOOTABLE_I386_ELF(h) \
 ((h.e_ident[EI_MAG0] == ELFMAG0) & (h.e_ident[EI_MAG1] == ELFMAG1) \
  & (h.e_ident[EI_MAG2] == ELFMAG2) & (h.e_ident[EI_MAG3] == ELFMAG3) \
  & (h.e_ident[EI_CLASS] == ELFCLASS32) & (h.e_ident[EI_DATA] == ELFDATA2LSB) \
  & (h.e_ident[EI_VERSION] == EV_CURRENT) & (h.e_type == ET_EXEC) \
  & (h.e_machine == EM_386) & (h.e_version == EV_CURRENT))

typedef struct
{
  unsigned long	sh_name;		/* Section name (string tbl index) */
  unsigned long sh_type;		/* Section type */
  unsigned long	sh_flags;		/* Section flags */
  unsigned long	sh_addr;		/* Section virtual addr at execution */
  unsigned long	sh_offset;		/* Section file offset */
  unsigned long	sh_size;		/* Section size in bytes */
  unsigned long	sh_link;		/* Link to another section */
  unsigned long	sh_info;		/* Additional section information */
  unsigned long	sh_addralign;		/* Section alignment */
  unsigned long	sh_entsize;		/* Entry size if section holds table */
}
Elf32_Shdr;

typedef struct
{
  unsigned long p_type;
  unsigned long p_offset;
  unsigned long p_vaddr;
  unsigned long p_paddr;
  unsigned long p_filesz;
  unsigned long p_memsz;
  unsigned long p_flags;
  unsigned long p_align;
}
Elf32_Phdr;

#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6


unsigned long cur_addr;
entry_func entry_addr;

/*
 * module list is a variable in the BSS area, so it is in between physical
 * address 3M and 4M, conflict with multi_boot(). This is why we have
 * fixed the multi_boot() function.
 * sizeof(mod_list) == 16. So mll[99] occupies less than 2K.
 */
struct mod_list mll[99];
static unsigned long long linux_mem_size;

/*
 *  The next two functions, 'load_image' and 'load_module', are the building
 *  blocks of the multiboot loader component.  They handle essentially all
 *  of the gory details of loading in a bootable image and the modules.
 */

kernel_t
load_image (char *kernel, char *arg, kernel_t suggested_type,
	    unsigned long load_flags)
{
  unsigned long len, i, exec_type = 0, align_4k = 1;
  entry_func real_entry_addr = 0;
  kernel_t type = KERNEL_TYPE_NONE;
  unsigned long flags = 0, text_len = 0, data_len = 0, bss_len = 0;
  char *str = 0, *str2 = 0;
  struct linux_kernel_header *lh;
  union
    {
      struct multiboot_header *mb;
      struct exec *aout;
      Elf32_Ehdr *elf;
    }
  pu;
  /* presuming that MULTIBOOT_SEARCH is large enough to encompass an
     executable header */
  unsigned char *buffer = (unsigned char *)(FSYS_BUF - MULTIBOOT_SEARCH);

  if (free_mem_start > (unsigned long)linux_bzimage_tmp_addr)
  {
	errnum = ERR_KERNEL_WITH_PROGRAM;
	goto failure;
  }
    
  errnum = ERR_NONE;
  /* sets the header pointer to point to the beginning of the
     buffer by default */
  pu.aout = (struct exec *) buffer;

  if (!grub_open (kernel))
	goto failure;

  if (!(len = grub_read ((unsigned long long)(unsigned long)buffer, MULTIBOOT_SEARCH, 0xedde0d90)) || len < 32)
	goto failure_exec_format;

  for (i = 0; i < len; i++)
    {
      if (MULTIBOOT_FOUND ((int) (buffer + i), len - i))
	{
	  flags = ((struct multiboot_header *) (buffer + i))->flags;
	  if (flags & MULTIBOOT_UNSUPPORTED)
	    {
	      errnum = ERR_BOOT_FEATURES;
	      goto failure_exec_format;
	    }
	  type = KERNEL_TYPE_MULTIBOOT;
	  str2 = "Multiboot";
	  break;
	}
    }

  /* Use BUFFER as a linux kernel header, if the image is Linux zImage
     or bzImage.  */
  lh = (struct linux_kernel_header *) buffer;
  
  /* ELF loading supported if multiboot, FreeBSD and NetBSD.  */
  if ((type == KERNEL_TYPE_MULTIBOOT
       || pu.elf->e_ident[EI_OSABI] == ELFOSABI_FREEBSD
       || grub_strcmp ((const char *)(pu.elf->e_ident + EI_BRAND), "FreeBSD") == 0
       || suggested_type == KERNEL_TYPE_NETBSD)
      && len > sizeof (Elf32_Ehdr)
      && BOOTABLE_I386_ELF ((*((Elf32_Ehdr *) buffer))))
    {
      if (type == KERNEL_TYPE_MULTIBOOT)
	entry_addr = (entry_func) pu.elf->e_entry;
      else
	entry_addr = (entry_func) (pu.elf->e_entry & 0xFFFFFF);

      if (entry_addr < (entry_func) 0x100000)
      {
	errnum = ERR_BELOW_1MB;
	goto failure_exec_format;
      }

      /* don't want to deal with ELF program header at some random
         place in the file -- this generally won't happen */
      if (pu.elf->e_phoff == 0 || pu.elf->e_phnum == 0
	  || ((pu.elf->e_phoff + (pu.elf->e_phentsize * pu.elf->e_phnum))
	      >= len))
	goto failure_exec_format;
      str = "elf";

      if (type == KERNEL_TYPE_NONE)
	{
	  /* At the moment, there is no way to identify a NetBSD ELF
	     kernel, so rely on the suggested type by the user.  */
	  if (suggested_type == KERNEL_TYPE_NETBSD)
	    {
	      str2 = "NetBSD";
	      type = suggested_type;
	    }
	  else
	    {
	      str2 = "FreeBSD";
	      type = KERNEL_TYPE_FREEBSD;
	    }
	}
    }
  else if (flags & MULTIBOOT_AOUT_KLUDGE)
    {
      pu.mb = (struct multiboot_header *) (buffer + i);
      entry_addr = (entry_func) pu.mb->entry_addr;
      cur_addr = pu.mb->load_addr;
      /* first offset into file */
      filepos = i - (pu.mb->header_addr - cur_addr);

      /* If the load end address is zero, load the whole contents.  */
      if (! pu.mb->load_end_addr)
	pu.mb->load_end_addr = cur_addr + filemax - filepos;
      
      text_len = pu.mb->load_end_addr - cur_addr;
      data_len = 0;

      /* If the bss end address is zero, assume that there is no bss area.  */
      if (! pu.mb->bss_end_addr)
	pu.mb->bss_end_addr = pu.mb->load_end_addr;
      
      bss_len = pu.mb->bss_end_addr - pu.mb->load_end_addr;

      if (pu.mb->header_addr < pu.mb->load_addr
	  || pu.mb->load_end_addr <= pu.mb->load_addr
	  || pu.mb->bss_end_addr < pu.mb->load_end_addr
	  || (pu.mb->header_addr - pu.mb->load_addr) > i)
	goto failure_exec_format;

      if (cur_addr < 0x100000)
      {
	errnum = ERR_BELOW_1MB;
	goto failure_exec_format;
      }

      pu.aout = (struct exec *) buffer;
      exec_type = 2;
      str = "kludge";
    }
  else if (len > sizeof (struct exec) && !N_BADMAG ((*(pu.aout))))
    {
      entry_addr = (entry_func) pu.aout->a_entry;

      if (type == KERNEL_TYPE_NONE)
	{
	  /*
	   *  If it doesn't have a Multiboot header, then presume
	   *  it is either a FreeBSD or NetBSD executable.  If so,
	   *  then use a magic number of normal ordering, ZMAGIC to
	   *  determine if it is FreeBSD.
	   *
	   *  This is all because freebsd and netbsd seem to require
	   *  masking out some address bits...  differently for each
	   *  one...  plus of course we need to know which booting
	   *  method to use.
	   */
	  entry_addr = (entry_func) ((int) entry_addr & 0xFFFFFF);
	  
	  if (buffer[0] == 0xb && buffer[1] == 1)
	    {
	      type = KERNEL_TYPE_FREEBSD;
	      cur_addr = (int) entry_addr;
	      str2 = "FreeBSD";
	    }
	  else
	    {
	      type = KERNEL_TYPE_NETBSD;
	      cur_addr = (int) entry_addr & 0xF00000;
	      if (N_GETMAGIC ((*(pu.aout))) != NMAGIC)
		align_4k = 0;
	      str2 = "NetBSD";
	    }
	}

      /* first offset into file */
      filepos = N_TXTOFF (*(pu.aout));
      text_len = pu.aout->a_text;
      data_len = pu.aout->a_data;
      bss_len = pu.aout->a_bss;

      if (cur_addr < 0x100000)
      {
	errnum = ERR_BELOW_1MB;
	goto failure_exec_format;
      }

      exec_type = 1;
      str = "a.out";
    }
  else if (lh->boot_flag == BOOTSEC_SIGNATURE
	   && lh->setup_sects <= LINUX_MAX_SETUP_SECTS)
    {
      int big_linux = 0;
      int setup_sects = lh->setup_sects;

      if (lh->header == LINUX_MAGIC_SIGNATURE && lh->version >= 0x0200)
	{
	  big_linux = (lh->loadflags & LINUX_FLAG_BIG_KERNEL);
	  lh->type_of_loader = LINUX_BOOT_LOADER_TYPE;

	  /* Put the real mode part at as a high location as possible.  */
	  linux_data_real_addr
	    = (char *) (((*(unsigned short *)0x413) << 10/*saved_mem_lower << 10*/) - LINUX_SETUP_MOVE_SIZE);
	  /* But it must not exceed the traditional area.  */
	  if (linux_data_real_addr > (char *) LINUX_OLD_REAL_MODE_ADDR)
	      linux_data_real_addr = (char *) LINUX_OLD_REAL_MODE_ADDR; /* 0x90000 */

	  if (lh->version >= 0x0201)
	    {
	      lh->heap_end_ptr = LINUX_HEAP_END_OFFSET;
	      lh->loadflags |= LINUX_FLAG_CAN_USE_HEAP;
	    }

	  if (lh->version >= 0x0202)
	    lh->cmd_line_ptr = linux_data_real_addr + LINUX_CL_OFFSET;
	  else
	    {
	      lh->cl_magic = LINUX_CL_MAGIC;
	      lh->cl_offset = LINUX_CL_OFFSET;
	      lh->setup_move_size = LINUX_SETUP_MOVE_SIZE;
	    }
	}
      else
	{
	  /* Your kernel is quite old...  */
	  lh->cl_magic = LINUX_CL_MAGIC;
	  lh->cl_offset = LINUX_CL_OFFSET;
	  
	  setup_sects = LINUX_DEFAULT_SETUP_SECTS;

	  linux_data_real_addr = (char *) LINUX_OLD_REAL_MODE_ADDR; /* 0x90000 */
	}
      
      /* If SETUP_SECTS is not set, set it to the default (4).  */
      if (! setup_sects)
	setup_sects = LINUX_DEFAULT_SETUP_SECTS;

      data_len = setup_sects << 9;
      text_len = filemax - data_len - SECTOR_SIZE;

      linux_data_tmp_addr = linux_bzimage_tmp_addr + text_len;

      if (! big_linux
	  && text_len > linux_data_real_addr - (char *) LINUX_ZIMAGE_ADDR)	/* 0x10000 */
	{
	  grub_printf (" linux 'zImage' kernel too big, try 'make bzImage'\n");
	  errnum = ERR_WONT_FIT;
	  goto failure_exec_format;
	}
      if (linux_data_real_addr + LINUX_SETUP_MOVE_SIZE
	       > RAW_ADDR ((char *) ((*(unsigned short *)0x413) << 10/*saved_mem_lower << 10*/)))
	{
	  errnum = ERR_WONT_FIT;
	  goto failure_exec_format;
	}
      
	if (debug > 0)
	      grub_printf ("   [Linux-%s, setup=0x%x, size=0x%x]\n",
		       (big_linux ? "bzImage" : "zImage"), data_len, text_len);

	  /* Video mode selection support. What a mess!  */
	  /* NOTE: Even the word "mess" is not still enough to
	     represent how wrong and bad the Linux video support is,
	     but I don't want to hear complaints from Linux fanatics
	     any more. -okuji  */
	{
	    char *vga;
	
	    /* Find the substring "vga=".  */
	    vga = grub_strstr (arg, "vga=");
	    if (vga)
	      {
		char *value = vga + 4;
		unsigned long long vid_mode;
	    
		/* Handle special strings.  */
		if (substring ("normal", value, 0) < 1)
		  vid_mode = LINUX_VID_MODE_NORMAL;
		else if (substring ("ext", value, 0) < 1)
		  vid_mode = LINUX_VID_MODE_EXTENDED;
		else if (substring ("ask", value, 0) < 1)
		  vid_mode = LINUX_VID_MODE_ASK;
		else if (! safe_parse_maxint (&value, &vid_mode))
			goto failure_exec_format;
	    
		lh->vid_mode = (unsigned short)vid_mode;
	      }
	}

	/* Check the mem= option to limit memory used for initrd.  */
	{
	    char *mem;
	
	    mem = grub_strstr (arg, "mem=");
	    if (mem)
	      {
		char *value = mem + 4;
	    
		safe_parse_maxint (&value, &linux_mem_size);
		switch (errnum)
		  {
		  case ERR_NUMBER_OVERFLOW:
		    /* If an overflow occurs, use the maximum address for
		       initrd instead. This is good, because MAXINT is
		       greater than LINUX_INITRD_MAX_ADDRESS.  */
		    linux_mem_size = LINUX_INITRD_MAX_ADDRESS;
		    errnum = ERR_NONE;
		    break;
		
		  case ERR_NONE:
		    {
		      int shift = 0;
		  
		      switch (grub_tolower (*value))
			{
			case 'g':
			  shift += 10;
			case 'm':
			  shift += 10;
			case 'k':
			  shift += 10;
			default:
			  break;
			}
		  
		      /* Check an overflow.  */
		      if (linux_mem_size > (MAXINT >> shift))
			linux_mem_size = LINUX_INITRD_MAX_ADDRESS;
		      else
			linux_mem_size <<= shift;
		    }
		    break;
		
		  default:
		    linux_mem_size = 0;
		    errnum = ERR_NONE;
		    break;
		  }
	      }
	    else
	      linux_mem_size = 0;
	}
      
	  /* It is possible that DATA_LEN + SECTOR_SIZE is greater than
	     MULTIBOOT_SEARCH, so the data may have been read partially.  */
	if (data_len + SECTOR_SIZE <= MULTIBOOT_SEARCH)
	    grub_memmove (linux_data_tmp_addr, buffer,
			  data_len + SECTOR_SIZE);
	else
	  {
	      grub_memmove (linux_data_tmp_addr, buffer, MULTIBOOT_SEARCH);
	      grub_read ((unsigned long long)(unsigned long)(linux_data_tmp_addr + MULTIBOOT_SEARCH),
			 data_len + SECTOR_SIZE - MULTIBOOT_SEARCH, 0xedde0d90);
	  }
	  
	if (lh->header != LINUX_MAGIC_SIGNATURE || lh->version < 0x0200)
	    /* Clear the heap space.  */
	    grub_memset (linux_data_tmp_addr + ((setup_sects + 1) << 9),
			 0,
			 (64 - setup_sects - 1) << 9);
      
	  /* Copy command-line plus memory hack to staging area.
	     NOTE: Linux has a bug that it doesn't handle multiple spaces
	     between two options and a space after a "mem=" option isn't
	     removed correctly so the arguments to init could be like
	     {"init", "", "", NULL}. This affects some not-very-clever
	     shells. Thus, the code below does a trick to avoid the bug.
	     That is, copy "mem=XXX" to the end of the command-line, and
	     avoid to copy spaces unnecessarily. Hell.  */
	{
	    char *src = arg;
	    char *dest = linux_data_tmp_addr + LINUX_CL_OFFSET;
	
	    while (dest < linux_data_tmp_addr + LINUX_CL_END_OFFSET && *src)
	      *(dest++) = *(src++);
	
	    /* Old Linux kernels have problems determining the amount of
	       the available memory.  To work around this problem, we add
	       the "mem" option to the kernel command line.  This has its
	       own drawbacks because newer kernels can determine the
	       memory map more accurately.  Boot protocol 2.03, which
	       appeared in Linux 2.4.18, provides a pointer to the kernel
	       version string, so we could check it.  But since kernel
	       2.4.18 and newer are known to detect memory reliably, boot
	       protocol 2.03 already implies that the kernel is new
	       enough.  The "mem" option is added if neither of the
	       following conditions is met:
	       1) The "mem" option is already present.
	       2) The "kernel" command is used with "--no-mem-option".
	       3) GNU GRUB is configured not to pass the "mem" option.
	       4) The kernel supports boot protocol 2.03 or newer.  */
	    if (! grub_strstr (arg, "mem=")
		&& ! (load_flags & KERNEL_LOAD_NO_MEM_OPTION)
		&& lh->version < 0x0203		/* kernel version < 2.4.18 */
		&& dest + 15 < linux_data_tmp_addr + LINUX_CL_END_OFFSET)
	      {
		*dest++ = ' ';
		*dest++ = 'm';
		*dest++ = 'e';
		*dest++ = 'm';
		*dest++ = '=';
	    
		dest = convert_to_ascii (dest, 'u', (extended_memory + 0x400), 0);
		*dest++ = 'K';
	      }
	
	    *dest = 0;
	}
      
	/* offset into file */
	filepos = data_len + SECTOR_SIZE;
      
	cur_addr = (int) linux_data_tmp_addr + LINUX_SETUP_MOVE_SIZE;
	grub_read ((unsigned long long)(unsigned long) linux_bzimage_tmp_addr, text_len, 0xedde0d90);
      
	if (errnum)
		goto failure_exec_format;

	/* Sanity check.  */
	if (suggested_type != KERNEL_TYPE_NONE
		&& ((big_linux && suggested_type != KERNEL_TYPE_BIG_LINUX)
		    || (! big_linux && suggested_type != KERNEL_TYPE_LINUX)))
		goto failure_exec_format;
	  
	/* Ugly hack.  */
	linux_text_len = text_len;
	  
	type = (big_linux ? KERNEL_TYPE_BIG_LINUX : KERNEL_TYPE_LINUX);
	goto success; 
    }
  else				/* no recognizable format */
	goto failure_exec_format;

  if (errnum)
	goto failure_exec_format;

  /* fill the multiboot info structure */
  mbi.cmdline = (int) arg;
  mbi.mods_count = 0;
  mbi.mods_addr = 0;
  mbi.boot_device = (current_drive == ram_drive ? (saved_drive << 24) | saved_partition :(current_drive << 24) | current_partition);
  mbi.flags &= ~(MB_INFO_MODS | MB_INFO_AOUT_SYMS | MB_INFO_ELF_SHDR);
  mbi.syms.a.tabsize = 0;
  mbi.syms.a.strsize = 0;
  mbi.syms.a.addr = 0;
  mbi.syms.a.pad = 0;
#ifdef FSYS_FB
	if ((mbi.boot_device>>24) == FB_DRIVE)
		mbi.boot_device = (fb_status << 16) | 0xFFFFFF;
#endif

  if (debug > 0)
      printf ("   [%s-%s", str2, str);

  str = "";

  if (exec_type)		/* can be loaded like a.out */
    {
      if (flags & MULTIBOOT_AOUT_KLUDGE)
	str = "-and-data";

      if (debug > 0)
        printf (", loadaddr=0x%x, text%s=0x%x", cur_addr, str, text_len);

      /* we have to increase cur_addr for now... */
      cur_addr += SYSTEM_RESERVED_MEMORY;	/* cur_addr is above SYSTEM_RESERVED_MEMORY. */

      /* read text, then read data */
      if (grub_read ((unsigned long long) RAW_ADDR (cur_addr), text_len, 0xedde0d90) != text_len)
	goto failure_exec_format;

      cur_addr += text_len;

      if (!(flags & MULTIBOOT_AOUT_KLUDGE))
        {
	  /* we have to align to a 4K boundary */
	  if (align_4k)
		cur_addr = (cur_addr + 0xFFF) & 0xFFFFF000;
	  else if (debug > 0)
		printf (", C");

	  if (debug > 0)
		printf (", data=0x%x", data_len);

	  if (grub_read ((unsigned long long) RAW_ADDR (cur_addr), data_len, 0xedde0d90) != data_len)
		goto failure_exec_format;
	  cur_addr += data_len;
        }

      if (errnum)
	goto failure_exec_format;

      memset ((char *) RAW_ADDR (cur_addr), 0, bss_len);
      cur_addr += bss_len;

      if (debug > 0)
	printf (", bss=0x%x", bss_len);

      if (pu.aout->a_syms && pu.aout->a_syms < (filemax - filepos))
	{
	  int symtab_err, orig_addr = cur_addr;

	  /* we should align to a 4K boundary here for good measure */
	  if (align_4k)
	    cur_addr = (cur_addr + 0xFFF) & 0xFFFFF000;

	  mbi.syms.a.addr = cur_addr - SYSTEM_RESERVED_MEMORY;

	  *((int *) RAW_ADDR (cur_addr)) = pu.aout->a_syms;
	  cur_addr += sizeof (int);
	  
	  if (debug > 0)
	      printf (", symtab=0x%x", pu.aout->a_syms);

	  if (grub_read ((unsigned long long) RAW_ADDR (cur_addr), pu.aout->a_syms, 0xedde0d90) == pu.aout->a_syms)
	    {
	      cur_addr += pu.aout->a_syms;
	      mbi.syms.a.tabsize = pu.aout->a_syms;

	      if (grub_read ((unsigned long long)(unsigned long) &i, sizeof (int), 0xedde0d90) == sizeof (int))
		{
		  *((int *) RAW_ADDR (cur_addr)) = i;
		  cur_addr += sizeof (int);

		  mbi.syms.a.strsize = i;

		  i -= sizeof (int);

		  if (debug > 0)
		      printf (", strtab=0x%x", i);

		  symtab_err = (grub_read ((unsigned long long) RAW_ADDR (cur_addr), i, 0xedde0d90) != i);
		  cur_addr += i;
		}
	      else
		symtab_err = 1;
	    }
	  else
	    symtab_err = 1;

	  if (symtab_err)
	    {
	      if (debug > 0)
		  printf ("(bad)");
	      cur_addr = orig_addr;
	      mbi.syms.a.tabsize = 0;
	      mbi.syms.a.strsize = 0;
	      mbi.syms.a.addr = 0;
	    }
	  else
	    mbi.flags |= MB_INFO_AOUT_SYMS;
	}
    }
  else
    /* ELF executable */
    {
      unsigned loaded = 0;
      Elf32_Phdr *phdr;
      Elf32_Shdr *shdr = NULL;
      int tab_size, sec_size;
      int symtab_err = 0;

      /* reset this to zero for now */
      cur_addr = 0;

      /* scan for program segments */
      for (i = 0; i < pu.elf->e_phnum; i++)
	{
	  phdr = (Elf32_Phdr *)(pu.elf->e_phoff + ((int) buffer) + (pu.elf->e_phentsize * i));
	  if (phdr->p_type == PT_LOAD)
	    {
	      unsigned memaddr, memsiz, filesiz;

	      /* offset into file */
	      filepos = phdr->p_offset;
	      filesiz = phdr->p_filesz;
	      
	      if (type == KERNEL_TYPE_FREEBSD || type == KERNEL_TYPE_NETBSD)
		memaddr = RAW_ADDR (phdr->p_paddr & 0xFFFFFF);
	      else
		memaddr = RAW_ADDR (phdr->p_paddr);
	      
	      memsiz = phdr->p_memsz;
	      if (memaddr < RAW_ADDR (0x100000))
	      {
		errnum = ERR_BELOW_1MB;
		goto failure_exec_format;
	      }

	      /* If the memory range contains the entry address, get the
		 physical address here.  */
	      if (type == KERNEL_TYPE_MULTIBOOT && (unsigned) entry_addr >= phdr->p_vaddr && (unsigned) entry_addr < phdr->p_vaddr + memsiz)
		real_entry_addr = (entry_func) ((unsigned) entry_addr + memaddr - phdr->p_vaddr);
		
	      /* make sure we only load what we're supposed to! */
	      if (filesiz > memsiz)
		  filesiz = memsiz;
	      if (debug > 0)
		  printf (", <0x%x:0x%x:0x%x>", memaddr, filesiz, (memsiz - filesiz));
	      /* increment number of segments */
	      loaded++;

	      /* load the segment */
	      if (! memcheck (memaddr, memsiz))
		goto failure_newline;

	      memaddr += SYSTEM_RESERVED_MEMORY;

	      /* mark memory as used */
	      if (cur_addr < memaddr + memsiz)
		  cur_addr = memaddr + memsiz;
	      if (grub_read ((unsigned long long) memaddr, filesiz, 0xedde0d90) != filesiz)
		goto failure_newline;
	      if (memsiz > filesiz)
		memset ((char *) (memaddr + filesiz), 0, memsiz - filesiz);
	    }
	}

      if (errnum)
	goto failure_newline;

      if (! loaded)
	goto failure_newline;

      /* Load ELF symbols.  */

      mbi.syms.e.num = pu.elf->e_shnum;
      mbi.syms.e.size = pu.elf->e_shentsize;
      mbi.syms.e.shndx = pu.elf->e_shstrndx;

      /* We should align to a 4K boundary here for good measure.  */
      if (align_4k)
	cur_addr = (cur_addr + 0xFFF) & 0xFFFFF000;

      tab_size = pu.elf->e_shentsize * pu.elf->e_shnum;

      filepos = pu.elf->e_shoff;
      if (grub_read ((unsigned long long) RAW_ADDR (cur_addr), tab_size, 0xedde0d90) == tab_size)
	{
	  mbi.syms.e.addr = cur_addr - SYSTEM_RESERVED_MEMORY;
	  shdr = (Elf32_Shdr *) mbi.syms.e.addr;
	  cur_addr += tab_size;

	  if (debug > 0)
	      printf (", shtab=0x%x", cur_addr);

	  for (i = 0; i < mbi.syms.e.num; i++)
	    {
	      /* This section is a loaded section,
		 so we don't care.  */
	      if (shdr[i].sh_addr != 0)
		continue;

	      /* This section is empty, so we don't care.  */
	      if (shdr[i].sh_size == 0)
		continue;

	      /* Align the section to a sh_addralign bits boundary.  */
	      cur_addr = ((cur_addr + shdr[i].sh_addralign) & (-(int)shdr[i].sh_addralign));

	      filepos = shdr[i].sh_offset;
	      sec_size = shdr[i].sh_size;

	      if (! (memcheck (cur_addr, sec_size)
		     && (grub_read ((unsigned long long) RAW_ADDR (cur_addr),
				    sec_size, 0xedde0d90)
			 == sec_size)))
		{
		  symtab_err = 1;
		  break;
		}

	      shdr[i].sh_addr = cur_addr - SYSTEM_RESERVED_MEMORY;
	      cur_addr += sec_size;
	    }
	}
      else 
	symtab_err = 1;

      if (mbi.syms.e.addr < (unsigned long)(RAW_ADDR(0x10000)))
	symtab_err = 1;

      if (symtab_err) 
	{
	  if (debug > 0)
	      printf ("(bad)");
	  mbi.syms.e.num = 0;
	  mbi.syms.e.size = 0;
	  mbi.syms.e.addr = 0;
	  mbi.syms.e.shndx = 0;
	  cur_addr = 0;
	}
      else
	mbi.flags |= MB_INFO_ELF_SHDR;
    }

  if (errnum)
	goto failure_newline;

  if (debug > 0)
	grub_printf (", entry=0x%x]\n", entry_addr);
      
  /* If the entry address is physically different from that of the ELF
     header, correct it here.  */
  if (real_entry_addr)
	entry_addr = real_entry_addr;

  /* Sanity check.  */
  if (suggested_type != KERNEL_TYPE_NONE && suggested_type != type)
	goto failure_exec_format;
  
success:

  grub_close ();
  return type;

failure_newline:

  if (debug > 0)
	putchar ('\n', 255);

failure_exec_format:

  grub_close ();

  if (errnum == ERR_NONE)
	errnum = ERR_EXEC_FORMAT;

failure:

  return KERNEL_TYPE_NONE;
}

int
load_module (char *module, char *arg)
{
  unsigned long len;

  /* if we are supposed to load on 4K boundaries */
  cur_addr = (cur_addr + 0xFFF) & 0xFFFFF000;

  if (!grub_open (module))
    return 0;

  len = grub_read ((unsigned long long) cur_addr, -1ULL, 0xedde0d90);
  if (! len)
    {
      grub_close ();
      return 0;
    }

  if (debug > 0)
      printf ("   [Multiboot-module @ 0x%x, 0x%x bytes]\n", cur_addr, len);

  /* these two simply need to be set if any modules are loaded at all */
  mbi.flags |= MB_INFO_MODS;
  mbi.mods_addr = 0x20000; /* yes, multi_boot() will move mll here. */

  mll[mbi.mods_count].cmdline = (int) arg;
  mll[mbi.mods_count].mod_start = cur_addr - SYSTEM_RESERVED_MEMORY;
  cur_addr += len;
  mll[mbi.mods_count].mod_end = cur_addr - SYSTEM_RESERVED_MEMORY;
  mll[mbi.mods_count].pad = 0;

  /* increment number of modules included */
  mbi.mods_count++;

  grub_close ();
  return 1;
}

struct linux_kernel_header *linux_header;
void cpio_set_field(char *field,unsigned long value);

void cpio_set_field(char *field,unsigned long value)
{
	char buf[9];
	sprintf(buf,"%08x",value);
	memcpy(field,buf,8);
}

int
load_initrd (char *initrd)
{
  unsigned long long len;
  unsigned long long moveto;
  unsigned long long tmp;
  unsigned long long top_addr;
  char *arg = initrd;

  linux_header = (struct linux_kernel_header *) (cur_addr - LINUX_SETUP_MOVE_SIZE);
  tmp = ((linux_header->header == LINUX_MAGIC_SIGNATURE && linux_header->version >= 0x0203)
	      ? linux_header->initrd_addr_max : LINUX_INITRD_MAX_ADDRESS);

  if (linux_mem_size)
    moveto = linux_mem_size;
  else
    moveto = (saved_mem_upper + 0x400) << 10;

  if (moveto > 0x100000000ULL)
      moveto = 0x100000000ULL;
  top_addr = moveto;

  /* XXX: Linux 2.3.xx has a bug in the memory range check, so avoid
     the last page.
     XXX: Linux 2.2.xx has a bug in the memory range check, which is
     worse than that of Linux 2.3.xx, so avoid the last 64kb. *sigh*  */
  moveto -= 0x10000;

  if (moveto > tmp)
      moveto = tmp;

  moveto &= 0xfffff000;

next_file:

  if (*initrd == '@')
  {
    moveto -= 0x200;
    initrd = skip_to (1, initrd);
  }

  if (! grub_open (initrd))
    goto fail;

  if (! filemax)
    {
      grub_close ();
      errnum = ERR_EXEC_FORMAT;	/* empty file */
      goto fail;
    }

  if (moveto < filemax + linux_text_len + 0x100000)
    {
      grub_close ();
      errnum = ERR_WONT_FIT;	/* file too long */
      goto fail;
    }

  moveto -= filemax;
  moveto &= 0xfffff000;

  tmp = filemax;
  grub_close ();
  initrd = skip_to (0, initrd);

  if (*initrd)
      goto next_file;

  {
	char map_tmp[64];
	grub_u32_t cpio_hdr_sz;
	grub_u32_t cpio_img_sz;
	tmp = top_addr - moveto;
	tmp += 0x1FF;
	tmp >>= 9;	/* sectors needed */
	sprintf (map_tmp, "--mem=-%d (md)0x800+8 (0x22)", (unsigned long)tmp);	// INITRD_DRIVE

	if (debug > 1)
	{
		printf ("Create INITRD_DRIVE:\tmap %s\n", map_tmp);
	}
	errnum = 0;
	disable_map_info = 1;
	{
		int is64bit_bak = is64bit;
		is64bit = 0;
		map_func (map_tmp, 0/*flags*/);
		is64bit = is64bit_bak;
	}
	disable_map_info = 0;

	if (errnum)
	{
		if (debug > 0)
		{
			printf ("Fatal: Error %d occurred while 'map %s'. Please report this bug.\n", errnum, map_tmp);
		}
		goto fail;
	}
	top_addr = moveto = initrd_start_sector << 9;
	memset ((char *)(unsigned long)top_addr, 0, tmp << 9);
	initrd = arg;
	len = 0;

next_file1:

	if (*initrd == '@')
	{
		char *name = initrd + 1;
		initrd = skip_to (SKIP_WITH_TERMINATE |1, initrd);
		struct cpio_header *cpio = (struct cpio_header *)(grub_u32_t)moveto;
		grub_u32_t name_len = grub_strlen(name);
		grub_open (initrd);
		memset(cpio,'0',sizeof(struct cpio_header));
		memcpy(cpio->c_magic,CPIO_MAGIC,sizeof(cpio->c_magic));
		cpio_set_field (cpio->c_mode, 0100644 );
		cpio_set_field (cpio->c_nlink,1);
		cpio_set_field (cpio->c_filesize, filemax);
		cpio_set_field (cpio->c_namesize, name_len);
		memcpy((void*)(cpio+1),name,name_len);
		cpio_hdr_sz = (sizeof(struct cpio_header) + 3 + name_len) & ~3;
	}
	else
	{
		cpio_hdr_sz = 0;
		grub_open (initrd);
	}

	arg = skip_to (0, initrd);
	if (debug) printf("Loading:%.*s\n",arg-initrd,initrd);

	tmp = grub_read (RAW_ADDR (moveto + cpio_hdr_sz), -1ULL, GRUB_READ);
	grub_close ();

	if (tmp != filemax)
	{
		//sprintf (map_tmp, "(0x22) (0x22)");	// INITRD_DRIVE
		//map_func (map_tmp, 0/*flags*/);
		map_func ("(0x22) (0x22)", 0/*flags*/);
		if (! errnum)
			errnum = ERR_READ;
		goto fail;
	}

	cpio_img_sz = (tmp + cpio_hdr_sz + 0xFFF) & ~0xFFF;
	moveto += cpio_img_sz;
	initrd = arg;

	if (*initrd)
	{
		len += cpio_img_sz;
		goto next_file1;
	}

	len += tmp + cpio_hdr_sz;

	unset_int13_handler (0);		/* unhook it */
	set_int13_handler (bios_drive_map);	/* hook it */
	buf_drive = -1;
	buf_track = -1;
  }
  if (debug > 0)
      printf ("   [Linux-initrd @ 0x%x, 0x%x bytes]\n", (unsigned long)top_addr, (unsigned long)len);

  /* FIXME: Should check if the kernel supports INITRD.  */
  linux_header->ramdisk_image = RAW_ADDR (top_addr);
  linux_header->ramdisk_size = len;

 fail:

  return ! errnum;
}


/*
 *  All "*_boot" commands depend on the images being loaded into memory
 *  correctly, the variables in this file being set up correctly, and
 *  the root partition being set in the 'saved_drive' and 'saved_partition'
 *  variables.
 */


void
bsd_boot (kernel_t type, int bootdev, char *arg)
{
  char *str;
  int clval = 0, i;

  struct bootinfo *bi = (struct bootinfo *)mbr;	// tmp. use mbr
  stop_floppy ();

  while (*(++arg) && *arg != ' ');
  str = arg;
  while (*str)
    {
      if (*str == '-')
	{
	  while (*str && *str != ' ')
	    {
	      if (*str == 'C')
		clval |= RB_CDROM;
	      if (*str == 'a')
		clval |= RB_ASKNAME;
	      if (*str == 'b')
		clval |= RB_HALT;
	      if (*str == 'c')
		clval |= RB_CONFIG;
	      if (*str == 'd')
		clval |= RB_KDB;
	      if (*str == 'D')
		clval |= RB_MULTIPLE;
	      if (*str == 'g')
		clval |= RB_GDB;
	      if (*str == 'h')
		clval |= RB_SERIAL;
	      if (*str == 'm')
		clval |= RB_MUTE;
	      if (*str == 'r')
		clval |= RB_DFLTROOT;
	      if (*str == 's')
		clval |= RB_SINGLE;
	      if (*str == 'v')
		clval |= RB_VERBOSE;
	      str++;
	    }
	  continue;
	}
      str++;
    }

  if (type == KERNEL_TYPE_FREEBSD)
    {
      clval |= RB_BOOTINFO;

      bi->bi_version = BOOTINFO_VERSION;

      *arg = 0;
      while ((--arg) > (char *) MB_CMDLINE_BUF && *arg != '/');
      if (*arg == '/')
	bi->bi_kernelname = (unsigned char *)arg + 1;
      else
	bi->bi_kernelname = 0;

      bi->bi_nfs_diskless = 0;
      bi->bi_n_bios_used = 0;	/* this field is apparently unused */

      for (i = 0; i < N_BIOS_GEOM; i++)
	{
//	  struct geometry tmp_geom;

	  /* XXX Should check the return value.  */
	  get_diskinfo (i + 0x80, &tmp_geom, 0);
	  /* FIXME: If HEADS or SECTORS is greater than 255, then this will
	     break the geometry information. That is a drawback of BSD
	     but not of GRUB.  */
	  bi->bi_bios_geom[i] = (((tmp_geom.cylinders - 1) << 16)
				+ (((tmp_geom.heads - 1) & 0xff) << 8)
				+ (tmp_geom.sectors & 0xff));
	}

      bi->bi_size = sizeof (struct bootinfo);
      bi->bi_memsizes_valid = 1;
      bi->bi_bios_dev = saved_drive;
      bi->bi_basemem = (*(unsigned short *)0x413)/*saved_mem_lower*/;
      bi->bi_extmem = extended_memory;

      if (mbi.flags & MB_INFO_AOUT_SYMS)
	{
	  bi->bi_symtab = mbi.syms.a.addr;
	  bi->bi_esymtab = mbi.syms.a.addr + 4 + mbi.syms.a.tabsize + mbi.syms.a.strsize;
	}
#if 0
      else if (mbi.flags & MB_INFO_ELF_SHDR)
	{
	  /* FIXME: Should check if a symbol table exists and, if exists,
	     pass the table to BI.  */
	}
#endif
      else
	{
	  bi->bi_symtab = 0;
	  bi->bi_esymtab = 0;
	}

      /* call entry point */
      //(*entry_addr) (clval, bootdev, 0, 0, 0, ((int) bi));
      multi_boot ((int) entry_addr, clval, bootdev, 0, 0, 0, ((int) bi));
    }
  else
    {
      /*
       *  We now pass the various bootstrap parameters to the loaded
       *  image via the argument list.
       *
       *  This is the official list:
       *
       *  arg0 = 8 (magic)
       *  arg1 = boot flags
       *  arg2 = boot device
       *  arg3 = start of symbol table (0 if not loaded)
       *  arg4 = end of symbol table (0 if not loaded)
       *  arg5 = transfer address from image
       *  arg6 = transfer address for next image pointer
       *  arg7 = conventional memory size (640)
       *  arg8 = extended memory size (8196)
       *
       *  ...in actuality, we just pass the parameters used by the kernel.
       */

      /* call entry point */
      unsigned long end_mark;

      if (mbi.flags & MB_INFO_AOUT_SYMS)
	end_mark = (mbi.syms.a.addr + 4 + mbi.syms.a.tabsize + mbi.syms.a.strsize);
      else
	/* FIXME: it should be mbi.syms.e.size.  */
	end_mark = 0;
      
      //(*entry_addr) (clval, bootdev, 0, end_mark, extended_memory, (*(unsigned short *)0x413)/*saved_mem_lower*/);
      multi_boot ((int) entry_addr, clval, bootdev, 0, end_mark, extended_memory, (*(unsigned short *)0x413)/*saved_mem_lower*/);
    }
}
