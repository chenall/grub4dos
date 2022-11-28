/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2004 Free Software Foundation, Inc.
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


/*
 * Note: These functions defined in this file may be called from C.
 *       Be careful of that you must not modify some registers. Quote
 *       from gcc-2.95.2/gcc/config/i386/i386.h:

   1 for registers not available across function calls.
   These must include the FIXED_REGISTERS and also any
   registers that can be used without being saved.
   The latter must include the registers where values are returned
   and the register where structure-value addresses are passed.
   Aside from that, you can include as many other registers as you like.

  ax,dx,cx,bx,si,di,bp,sp,st,st1,st2,st3,st4,st5,st6,st7,arg
{  1, 1, 1, 0, 0, 0, 0, 1, 1,  1,  1,  1,  1,  1,  1,  1,  1 }
 */

#define ASM_FILE
#define repair_memory_holes 1

#include "filesys.h"
#include "shared.h"

# define	ABS(x)	((x) - EXT_C(main) + 0x8200)

	.file	"asm.S"

	.text

	/*
	 * In stage2, do not link start.S with the rest of the source
	 * files directly, so define the start symbols here just to
	 * force ld quiet. These are not referred anyway.
	 */
	.globl	start, _start
start:
_start:

ENTRY(main)

	.code16

	/* If BP=0x07C0, it indicates boot from pxe or no-emulation cdrom */

	/*
	 *  Guarantee that "main" is loaded at 0x0:0x8200 in stage2 and
	 *  at 0x0:0x2200 in stage1.5.
	 */
	ljmp $0, $ABS(codestart)

	. = EXT_C(main) + 0x5

	/* control byte: pxe, DUCE, tune
	 * bit 0 = 1: disable pxe
	 * bit 1 = 1: disable keyboard intervention in boot process
	 * bit 2 = 1: disable the "unconditional command-line entrance" feature
	 * bit 3 = 1: disable geometry tune
	 * bit 4 = 1: disable startup cdrom drive look-up.
	 * bit 5 = 1: enable HLT instruction for checkkey idle loop.
	 */
	.byte	0

	. = EXT_C(main) + 0x6

	.byte	COMPAT_VERSION_MAJOR, COMPAT_VERSION_MINOR

	. = EXT_C(main) + 0x8

VARIABLE(install_partition)
	.long	0x00FFFFFF

#define	preset_menu 0x307FFC
#define	use_preset_menu 0x307FF8

	. = EXT_C(main) + 0xC

VARIABLE(saved_entryno)
	/* Note: GRUB for DOS uses this for the commandline preset_menu.
	 * A preset_menu can be embedded in the commandline of GRUB.EXE.
	 * This new preset_menu overrides the built-in preset_menu.
	 * If the variable is not touched, and the first byte at
	 * config_file == 0, then the new menu at 0x0800 will work.
	 * If the variable here is cleared to 0, or the first byte at
	 * config_file != 0, then the built-in preset_menu will work.
	 *
	 * Do NOT change this variable to other value than 0.
	 */
	/* Some external utilities use this to locate the preset_menu. */
	.long	ABS(preset_menu_old)
	//.long	preset_menu	/* it is now at a fixed address 0x307FFC */

	. = EXT_C(main) + 0x10

VARIABLE(stage2_id)
	.byte	STAGE2_ID

VARIABLE(force_lba)
	/* only bit 0 is now used for force_lba. */
	/* bit 1=1 means we are booting as a Linux kernel. */
	/* bit 2=0 enable single-key-selection feature. */
	.byte	0x04

VARIABLE(version_string)
	.string VERSION

VARIABLE(config_file)
	.string "/menu.lst"

	. = EXT_C(main) + 0x34

	.word	0x14C	/* CPU type for i386 and compatibles */
	.word	1	/* there is 1 section */

	. = EXT_C(main) + 0x44

	.word	0x00	/* size of optional header */
	.word	3	/* flags: "no relocation" and "valid executable" */
	.word	0x722E	/* section name for resources */
	.word	0x7273
	.word	0x63

	/* Utilities may use this to locate the bss starting address,
	 * where the "bootlace" signature is placed and preset-menu
	 * follows.
	 */
	. = EXT_C(main) + 0x60

VARIABLE(hotkey_func)  //int*                                             //0x8260	外置热键功能
  .long 0 

	. = EXT_C(main) + 0x6C  #; bss starting address

preset_menu_old:
#if defined(HAVE_USCORE_USCORE_BSS_START_SYMBOL)
	.long	__bss_start - 0x300000
#elif defined(HAVE_USCORE_EDATA_SYMBOL)
	.long	_edata - 0x300000
#elif defined(HAVE_EDATA_SYMBOL)
	.long	edata - 0x300000
#else
#error no bss starting address
#endif

	. = EXT_C(main) + 0x70

/* the real mode code continues... */
codestart:
	jmp	real_codestart

	. = EXT_C(main) + 0x73
VARIABLE(debug_msg)
	.byte 2
	/* internal variables follow */
	. = EXT_C(main) + 0x74
VARIABLE(menu_num_ctrl)
	.long 0x2000
	#include "grub4dos_int_ver.h"
	#ifdef GRUB4DOS_INT_VER
	.long GRUB4DOS_INT_VER
	#else
	.long 0
	#endif
	#undef GRUB4DOS_INT_VER
	.long 0
	. = EXT_C(main) + 0x80

VARIABLE(boot_drive)
	.long	0xFFFFFFFF	/* can be setup by external programs. */

VARIABLE(pxe_yip)
	.long	0
VARIABLE(pxe_sip)
	.long	0
VARIABLE(pxe_gip)
	.long	0

	. = EXT_C(main) + 0x90

VARIABLE(filesize)
	.long	0, 0
VARIABLE(saved_mem_upper)	/* maximum contiguous memory in KB starting at 1M and below 4G */
	.long	0
VARIABLE(saved_partition)
	.long	0

	. = EXT_C(main) + 0xA0

VARIABLE(saved_drive)
	.long	0
VARIABLE(no_decompression)
	.long	0
VARIABLE(part_start)
	.long	0, 0

	. = EXT_C(main) + 0xB0

VARIABLE(part_length)
	.long	0, 0

VARIABLE(fb_status)
	.long	0

VARIABLE(is64bit)
	.long	0

	. = EXT_C(main) + 0xC0

VARIABLE(saved_mem_higher)	/* maximum contiguous memory in KB starting at 4G */
	.long	0, 0

VARIABLE(cdrom_drive)
	.long	0xFFFFFFFF	/* default is the invalid drive number */

VARIABLE(ram_drive)
	.long	0x7F	/* default is a floppy */

	. = EXT_C(main) + 0xD0

VARIABLE(rd_base)
	.long	0, 0

VARIABLE(rd_size)
	.long	0, 1	/* default is 4G */

	. = EXT_C(main) + 0xE0

	/* mem alloc array structure

		long	// mem block starting address(16 byte align)
			// bit0=0 for free mem, 1 for used mem
			// bit 2:1 =0 for program code
			//         =1 for environment
			//         =2 for extra data
			//         =3 reserved
			// bit 3=reserved
		long	// process id.
			// 0 - 255 for kernel, others for user program
	 */
VARIABLE(mem_alloc_array_start)
	.long	0x800000	/* at 8M */

VARIABLE(mem_alloc_array_end)
	.long	0xA00000	/* at 10M */

VARIABLE(page_map_start)
	.long	0xA00000	/* at 10M */

VARIABLE(page_map_end)
	.long	0xE00000	/* at 14M */


	. = EXT_C(main) + 0xF0

VARIABLE(free_mem_start)	/* unused mem starting address. */
	.long	SYSTEM_RESERVED_MEMORY	/* may increase later. */

VARIABLE(free_mem_end)		/* unused mem ending address */
	.long	0		/* 0 for 4G(i.e., unlimited) */

VARIABLE(saved_mmap_addr)	/* system memory map starting addess */
	.long	ABS(EXT_C(end_of_low_16bit_code))

VARIABLE(saved_mmap_length)	/* system memory map length */
	.long	0

	. = EXT_C(main) + 0x100

VARIABLE(addr_system_functions)
	.long	ABS(EXT_C(system_functions))

VARIABLE(addr_system_variables)
	.long	ABS(EXT_C(system_variables))
VARIABLE(menu_cfg) /*char menu_cfg[]*/
	.long 0x1110
VARIABLE(iso_types)/*char iso_types*/
	.byte 0xff
	. = EXT_C(main) + 0x110

VARIABLE(addr_saved_dir)
	.long	ABS(EXT_C(saved_dir))

VARIABLE(errnum)
	.long	0

VARIABLE(current_drive)
	.long	0xFFFFFFFF

VARIABLE(current_partition)
	.long	0

	. = EXT_C(main) + 0x120

VARIABLE(filemax)
	.long	0, 0

VARIABLE(filepos)
	.long	0, 0

	. = EXT_C(main) + 0x130

VARIABLE(debug)
	/* 0 for silent, 1 for normal, 2..MAX_UINT for verbose.
	 * if debug == MAX_UINT or debug < 0, then filesystem debug mode is on.
	 * (MAX_UINT is 0x7FFFFFFF)
	 */
	.long	0

VARIABLE(current_slice)
	.long	0

VARIABLE(dos_drive_geometry)
	# byte 0=DOS boot drive: 0 for floppy, 0x80 for harddrive.
	# byte 1=max head number for DOS boot drive=(number of heads - 1).
	# byte 2=sectors per track for DOS boot drive.
	# byte 3=0(reserved).
	.long	0

VARIABLE(dos_part_start)
	# partition start sector number(or hidden sectors) for DOS boot drive.
	.long	0

	. = EXT_C(main) + 0x140

VARIABLE(buf_track)
	.long	-1
	.long	0	/* buf_track will grow to 64-bit */
VARIABLE(buf_drive)
	.long	-1
VARIABLE(current_x_resolution)	//0x834c  当前水平像素 int

	. = EXT_C(main) + 0x150
	
VARIABLE(usb_count_error)		//bit 7:   0 = usb_count; 1 = usb_error
	.byte	0
VARIABLE(usb_drive_num)	
	.byte	0,0,0,0
VARIABLE(usb_md_address)
	.word	0
VARIABLE(count_ms)
	.word	0
VARIABLE(beep_frequency)
	.word	0
	
	. = EXT_C(main) + 0x160	
	
real_codestart:

	cli
	cld

	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movl	$STACKOFF, %esp

	sti		/* added 2008-08-04 */

	/* save DX at 0x8206, overwriting the unused version numbers. */
	movw	%dx, ABS(EXT_C(install_partition)) - 2

	# it is time to do this after DX has been saved.
	pushal
	movw	$0x0003, %ax	/* set display mode: 80*25 color text */
	int	$0x10		# this might destroy our DX
	popal

	/* check for fb structure  */
	cmpl	$0x46424246, (0x21b4)
	jnz	1f
	movw	%ax, (0x21b4) //destroy it to prevent repeated fb recognition
	movl	(0x21ae), %eax
	movb	(0x21ad), %al
	movl	%eax, ABS(EXT_C(fb_status))
	cmpb	$0,%ah
	jnz	1f
	/* it is the floppy (fd0) */
	testb	$0x01,0x0410
	jnz	1f
	/* no floppy in BDA, so we set floppy count to 1. */
	orb	$0x01,0x0410	/* diskette set as present */
	andb	$0x3F,0x0410	/* clear bit 6-7 */

//	movb	$(FB_DRIVE),%dl
1:
	movb	0x0410, %al
	movb	%al, ABS(EXT_C(floppies_orig))
	movb	0x0475, %al
	movb	%al, ABS(EXT_C(harddrives_orig))

	//movb	$((int13_handler_end - int13_handler + 0x3ff) / 0x400), ABS(int13_handler)

	movb	$1, %cl		// CX != 0, indicating our int13 handler not present

	movl	0x54, %eax
	movl	%eax, ABS(EXT_C(ROM_int15))
	movl	0x4C, %eax
	movl	%eax, ABS(EXT_C(ROM_int13))
	movl	%eax, ABS(EXT_C(ROM_int13_dup))
	cmpl	$0xC0000000, %eax
	jnb	1f
	cmpl	$0x5A000000, %eax	// cmpl	$0x9A000000, %eax
	jb	1f
	andl	$0x3FFFFF, %eax
//	cmpl	$0x100, %eax
	cmpl	$(int13_handler_entry - int13_handler), %eax
	jnz	1f
	movw	0x413, %ax		/* Memory size in Kb */
	shlw	$6, %ax			/* Memory size in paragragh */
	cmpw	0x4E, %ax		/* 0000:004E=current int 13 segment */
	jne	1f			/* not hooked */

	movw	%ax, %ds		/* DS=current int13 code segment */

	/* check our int13 signature "$INT13SFGRUB4DOS" */
//	cmpl	$0x544E4924, 0x103	/* $INT */
	cmpl	$0x544E4924, (int13_handler_entry - int13_handler + 3)	/* $INT */
	jnz	2f
//	cmpl	$0x46533331, 0x107	/* 13SF */
	cmpl	$0x46533331, (int13_handler_entry - int13_handler + 7)	/* 13SF */
	jnz	2f
//	cmpl	$0x42555247, 0x10B	/* GRUB */
	cmpl	$0x42555247, (int13_handler_entry - int13_handler + 0xb)	/* GRUB */
	jnz	2f
//	cmpl	$0x534F4434, 0x10F	/* 4DOS */
	cmpl	$0x534F4434, (int13_handler_entry - int13_handler + 0xf)	/* 4DOS */
	jnz	2f

	movl	(EXT_C(ROM_int13) - int13_handler), %eax	/* 0x1C=ROM int 13 */
	cmpl	$0x5A000000, %eax	// cmpl	$0x9A000000, %eax
	jb	2f					/* not our handler */

	movl	(EXT_C(ROM_int15) - int13_handler), %eax	/* 0x0C=ROM int 15 */
	cmpl	$0x5A000000, %eax	// cmpl	$0x9A000000, %eax
	jb	2f					/* not our handler */

	/* restore old emu data, except the first byte of handler size. */
//	movw	$(0x140 - 1), %cx
	mov $(restore_old_emu - int13_handler - 1), %cx
	movw	$1, %si			/* DS=current int13 code segment */
	movw	$ABS(int13_handler + 1), %di		/* ES=0 */
	repz movsb
#ifdef CDROM_INIT
	/* calculate the new max_cdrom_id. */
	movw	$2, %si
	lodsw				/* AL=atapi_dev_count, AH=min_cdrom_id */
	addb	%ah, %al
	decw	%ax			/* AL=max_cdrom_id */
	movb	%al, %cs:ABS(max_cdrom_id)		/* CS=0 */
#endif
	/* initialize bios_drive_map with hooked_drive_map_1 */
#if	MAP_NUM_16
	movw	$(DRIVE_MAP_SIZE * DRIVE_MAP_SLOT_SIZE / 4), %cx
	movw	$ABS(EXT_C(hooked_drive_map_1)), %si		/* CS=0 */
#else
	movw	$(DRIVE_MAP_SIZE * DRIVE_MAP_SLOT_SIZE / 2), %cx
	movw	$ABS(EXT_C(hooked_drive_map)), %si		/* CS=0 */
#endif
	movw	$ABS(EXT_C(bios_drive_map)), %di		/* ES=0 */
	cs repz movsw
	
	movw	$(0x3000 - 0x2800)/8, %cx
	movw	$0x2800, %si
3:
	cmpl	$0x47415246, (%si)		/* FRAGMENT */
	je	3f
	addw	$8, %si
	loop	3b
	jmp	2f
3:
	cmpl	$16, 0xfc							//16 FRAGMENT
	jne	3f	

#if	MAP_NUM_16	
	pushw	%si
	// BIOS-hooked_drive_map_2 -> G4D-hooked_drive_map_2
	movw	$(DRIVE_MAP_SIZE * DRIVE_MAP_SLOT_SIZE / 4), %cx
	pushw	%cx
	subw	$((DRIVE_MAP_SIZE / 2 + 1) * DRIVE_MAP_SLOT_SIZE), %si
	pushw	%si
	movw	$ABS(EXT_C(hooked_drive_map_2)), %di		/* ES=0 */
	repz	movsw
	//BIOS_hooked_drive_map_2 -> G4D-bios_drive_map_8
	popw	%si
	popw	%cx
	movw	$ABS(EXT_C(bios_drive_map) + ((DRIVE_MAP_SIZE / 2) * DRIVE_MAP_SLOT_SIZE)), %di		/* ES=0 */
	repz	movsw		
	popw	%si
#endif
3:
	addw	$8, %si
	movw	$ABS(EXT_C(hooked_fragment_map)), %di
	movw	$FRAGMENT_MAP_SLOT_SIZE/2, %cx
	repz	movsw

	/* Now CX = 0, indicating our int13 handler is present. */
2:
	xorw	%ax, %ax
	movw	%ax, %ds			/* DS=0 */
1:
	jcxz	1f	/* our int13 handler is present, skip */
	/* clear highest 2 bits of byte at 0x410. */
	andb	$0x3F, 0x0410	/* clear bit 6-7 */
	cmpw	$0x07C0, %bp	/* boot from pxe or cdrom? */
	je	3f		/* yes, directly mask off floppy. */
	/* check fbinst boot */
	movl	ABS(EXT_C(fb_status)), %ecx
	jecxz	2f	/* did not boot by fbinst, continue */
	cmpb	$0, %ch		/* fbinst boot drive is floppy? */
	jz	1f		/* yes, skip */
2:
	/* check DOS boot of grub.exe */
	movl	ABS(EXT_C(dos_drive_geometry)), %ecx
	jecxz	2f	/* did not boot off DOS, continue */
	cmpb	$0, %cl		/* DOS boot drive is floppy? */
	jz	1f		/* yes, skip */
2:
	/* check saved DX register */
	movw	ABS(EXT_C(install_partition)) - 2, %cx
	cmpb	$0, %cl		/* BIOS boot drive is floppy? */
	jz	1f		/* yes, skip */
3:
	/* did not boot from floppy. clear lowest bit of byte at 0x410. */
	andb	$0xFE, 0x0410	/* clear bit 0 */
1:
	/* check the BIOS type (currently only for Bochs) */
	movw	$0xF000, %ax
	movw	%ax, %es			/* ES=0xF000 */
	movw	$0xFF00, %di
	movw	$ABS(bochs_copygrght_string), %si
	movw	$0x22, %cx
	repz cmpsw
	setz	ABS(EXT_C(bios_id))	/* 1 for bochs, 0 for unknown. */
	xorw	%ax, %ax
	movw	%ax, %es			/* ES=0 */

//	pushw	%dx		/* DL=boot drive */

	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es

	sti

	/* check the DUCE indicator */

	movl	0x5FC, %eax
	movl	%eax, %ecx
	testb	$0x0C, %al
	jz	1f
	orb	$0x0C, %al
	cmpl	$0x4543554C, %eax
	jne	1f
	andb	$0x0C, %cl	/* bit2=DUCE, bit3=chs-no-tune */
	orb	%cl, ABS(EXT_C(main)) + 5 /* at 0x8205 */
1:
	movw	$1, %bx		/* BL=1, BH=0 */

	testb	$0x02, ABS(EXT_C(main)) + 5	/* disable keyboard intervention? */
	jnz	4f			/* yes */

	movl	0x46c, %eax	/* initial tick */
	addl	$5, %eax	/* wait 0.27 seconds */
	pushl	%eax

2:
	/* checkkey 'c' */

	pushw	%bx
	movb	$0x01, %ah	/* checkkey */
	int	$0x16
	popw	%bx

	jz	1f		/* no keypress */

	/* getkey */

	pushw	%bx
	movb	$0x00, %ah	/* getkey */
	int	$0x16
	popw	%bx

	cmpw	$KEY_IC, %ax	/* insert char */
	jne	3f
	orb	$1, %bh		/* DEBUG_KEY pressed */
	jmp	2b		/* get next key */
3:
	orb	$0x20, %al
	cmpb	$0x73, %al	/* is "S"? */
	jne	3f
	orb	$2, %bh		/* Single Sector for Buggy BIOS */
3:
	cmpb	$0x63, %al	/* is "C"? */
	jne	2b		/* no, get next key */

	/* "C" is pressed. */

	testb	$0x04, ABS(EXT_C(main)) + 5 /* at 0x8205, bit2=DUCE */
	jnz	2b

	/* Bypass all config files */
	movb	$0, %bl
	jmp	2b
1:

	popl	%eax
	pushl	%eax
	movl	0x46c, %ecx	/* current tick */
	cmpl	%eax, %ecx
	jnb	1f
	subl	$5, %eax
	cmpl	%eax, %ecx
	jnb	2b
	cmpl	$5, %ecx
	jb	2b
1:
	popl	%eax
4:
	movb	%bh, ABS(EXT_C(debug_boot))

	testb	%bl, %bl
	jnz	1f
	movl	$0, ABS(EXT_C(use_config_file))
1:
//	popw	%dx		/* DL=boot drive */

//	movzbl	%dl, %eax
//	movl	%eax, ABS(EXT_C(boot_drive))

	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es

	testb	$0x02, ABS(EXT_C(main)) + 5	/* disable keyboard intervention? */
	jnz	4f			/* yes */

	//pushw	%bx

	/* clear keyboard buffer */
2:
	movb	$0x01, %ah	/* checkkey */
	int	$0x16

	jz	1f		/* no keypress */

	movb	$0x00, %ah	/* getkey */
	int	$0x16
	jmp	2b
1:

	//popw	%bx

4:

	/* Oops! I forgot to enable A20. Thanks to zw2312914. */
	
	movw	$0x00ff, %cx		# try so many times on failure
	movw	$0x0101, %dx		# DL=1(enable A20), DH=1(debug on)

	cli	/* yes, keep interrupt off when controlling A20 */
	call	enable_disable_a20	# EAX, CX modified
	sti

	jz	1f		/* enabled A20 with success */

	/* A20 control failed. Notify the user. */
	pushw	$ABS(bootup_a20_failure)	# the format string
	call	realmode_printf
	//addw	$2, %sp	# adjust the stack pointer, but not required for now.
	jmp	hard_stop

bootup_a20_failure:
	.ascii	"\r\ngrub4dos: A20 failure. Please report with error code(if any in the above line).\r\n\0"

1:
	movw %ds:0x046c, %dx
	xorl %ecx, %ecx
1:
	movw %ds:0x046c, %ax
	cmpw %dx, %ax
	je 1b
1:
	cmpw %ds:0x046c, %ax
	jne 2f
	pushw %dx
	pushw %ax
	movw $0xcf8, %dx
	inw %dx, %ax
	popw %ax
	popw %dx
	incl %ecx 
	cmpl $0, %ecx
	jmp 1b
2:
	xorl %edx, %edx
	movl %ecx, %eax
	movl $0x37, %ecx
	divl %ecx
	movw %ax, ABS(EXT_C(count_ms))
	
	xorb	%bh, %bh
	movb	$0, %bl
	movw	$0x1003, %ax
	int	$0x10

	/* transition to protected mode */
	call EXT_C(real_to_prot)

	.code32

	/* enable preset_menu */
	movl	$1, use_preset_menu

	/* if force_cdrom_as_boot_device==0, we are running by configfile, so we skip preset menu check */
	cmpl	$0, ABS(EXT_C(force_cdrom_as_boot_device))
	je	1f		/* skip all */

	/* before clearing the bss, we move preset_menu to 0x800 */

#if 0
	movl	preset_menu, %eax

#if defined(HAVE_USCORE_USCORE_BSS_START_SYMBOL)
	cmpl	$__bss_start - 0x300000, %eax
#elif defined(HAVE_USCORE_EDATA_SYMBOL)
	cmpl	$_edata - 0x300000, %eax
#elif defined(HAVE_EDATA_SYMBOL)
	cmpl	$edata - 0x300000, %eax
#else
#error no bss starting address
#endif
	jnz	4f		/* use old bootp for diskless */
#endif

	xorl	%eax, %eax
	cmpb	%al, ABS(EXT_C(config_file))	/* AL == 0 */
	jnz	2f
	movl	ABS(EXT_C(saved_entryno)), %ebx
	testl	%ebx, %ebx
	jnz	3f	/* use menu embedded in commnad-line of grub.exe */
2:
	/* use builtin preset_menu */

	/* set the starting address of the preset_menu */

#if defined(HAVE_USCORE_USCORE_BSS_START_SYMBOL)
	movl	$__bss_start - 0x300000, %esi
#elif defined(HAVE_USCORE_EDATA_SYMBOL)
	movl	$_edata - 0x300000, %esi
#elif defined(HAVE_EDATA_SYMBOL)
	movl	$edata - 0x300000, %esi
#else
#error no bss starting address
#endif

	cld
	addl	$16, %esi	/* skip 4 bytes of B0 02 1A CE */
				/* skip 4 bytes of reserved */
				/* skip 4 bytes of reserved */
				/* skip 4 bytes of zeroes */
	/* check LZMA */
	cmpb	$0xE0, (%esi)
	ja	5f		/* not lzma */
	movl	1(%esi), %ecx
	jecxz	5f		/* not lzma */
	cmpl	$0x1000, %ecx
	jb	5f		/* unaccepted lzma */
	movl	$0x80000000, %eax
	xorl	%edx, %edx	/* dividend=EDX:EAX */
	divl	%ecx		/* divisor=ECX */
				/* result: EAX=quotient, EDX=remainder */
	testl	%edx, %edx
	jnz	5f		/* unaccepted lzma */
	cmpl	%edx, 9(%esi)	/* EDX=0 */
	jnz	5f		/* unaccepted lzma */
	/* now it is LZMA */
	movl	$0x10000, %ecx	/* move 256KB of the menu ... */
	movl	$0x110000, %edi	/* ... to address 1M+64K */
	repz movsl
	movl	$0x110000, preset_menu	/* use new menu at 1M+64K */
	jmp	4f
5:
	movl	$0x400, %ecx	/* move 4KB of the menu ... */
	movl	$0x800, %edi	/* ... to 0x800 */
	repz movsl

3:
	movl	$0x0800, preset_menu	/* use new menu at 0x800 */
4:

//	/* if force_cdrom_as_boot_device==0, we are running by configfile, so we do not clear bss */
//	cmpl	$0, ABS(EXT_C(force_cdrom_as_boot_device))
//	je	1f

	/* before clearing bss, move 32-bit code to above 3M. */
	/* This need A20 to be enabled. */

	movl	$ABS(EXT_C(end_of_low_16bit_code)), %esi
	leal	0x300000(%esi), %edi

#if defined(HAVE_USCORE_USCORE_BSS_START_SYMBOL)
	movl	$__bss_start, %ecx
#elif defined(HAVE_USCORE_EDATA_SYMBOL)
	movl	$_edata, %ecx
#elif defined(HAVE_EDATA_SYMBOL)
	movl	$edata, %ecx
#else
#error no bss starting address
#endif
	subl	%edi, %ecx
	shrl	$2, %ecx
	cld
	repz movsl

	/* clean out the bss */

//	/* set %edi to the bss starting address */
//#if defined(HAVE_USCORE_USCORE_BSS_START_SYMBOL)
//	movl	$__bss_start, %edi
//#elif defined(HAVE_USCORE_EDATA_SYMBOL)
//	movl	$_edata, %edi
//#elif defined(HAVE_EDATA_SYMBOL)
//	movl	$edata, %edi
//#else
//#error no bss starting address
//#endif

	/* set %ecx to the bss end */
#if defined(HAVE_END_SYMBOL)
	movl	$end, %ecx
#elif defined(HAVE_USCORE_END_SYMBOL)
	movl	$_end, %ecx
#else
#error no bss ending address
#endif

	/* compute the bss length */
	subl	%edi, %ecx
	shrl	$2, %ecx

	/* zero %eax */
	xorl	%eax, %eax

	/* set the direction */
	cld

	/* clean out */
	rep stosl

	/* initialize IDT */

	movl	$0x300000, %edi
	movl	$0x100, %ecx
	xorl	%ebx, %ebx
2:
	movl	$ABS(default_iret), %eax
	cmpb	$0xFE, %cl		// int 0x02 nmi
	jne	3f
	movl	$ABS(int2_nmi), %eax
	jmp	5f
3:
	cmpb	$0xF8, %cl
	jg	5f			// int 0x00 - 0x07
	cmpb	$0xF1, %cl
	jl	3f			// int 0x10 - 0xFF
	cmpb	$8, %bl
	jne	4f
	movl	$ABS(irq_0), %eax	// int 0x08
	jmp	5f
4:
	movl	$(irq_2 - irq_1), %eax	// int 0x09 - 0x0F
	subl	$0x09, %ebx
	mull	%ebx
	addl	$0x09, %ebx
	addl	$ABS(irq_1), %eax
	jmp	5f
3:
	cmpb	$0x90, %cl
	jg	5f			// int 0x10 - 0x6F
	cmpb	$0x88, %cl
	jl	3f			// int 0x78 - 0xFF
	movl	$(irq_9 - irq_8), %eax	// int 0x70 - 0x77
	subl	$0x70, %ebx
	mull	%ebx
	addl	$0x70, %ebx
	addl	$ABS(irq_8), %eax
	//jmp	5f
3:
5:
	stosw
	shrl	$16, %eax
	movl	$(0x8E000000 + 40), %edx
	xchgl	%eax, %edx
	stosl
	xchgl	%eax, %edx
	stosw
	incl	%ebx
	loop	2b

1:
	call	EXT_C(prot_to_real)
	
	.code16

	call	EXT_C(real_to_prot)

	.code32

	sti

	/*
	 *  Call the start of main body of C code, which does some
	 *  of it's own initialization before transferring to "cmain".
	 */
	call EXT_C(init_bios_info)
	call EXT_C(cmain)


/*
 *  This call is special...  it never returns...  in fact it should simply
 *  hang at this point!
 */

ENTRY(stop)
	call	EXT_C(prot_to_real)

	/*
	 * This next part is sort of evil.  It takes advantage of the
	 * byte ordering on the x86 to work in either 16-bit or 32-bit
	 * mode, so think about it before changing it.
	 */

	/* No external program ever calls HARD_STOP. HARD_STOP is only called
	 * by the asm.S itself, and all calls are from real mode. So we
	 * could(and should) use .code16 here clearly.
	 */

	.code16

hard_stop:
	sti
	//hlt
	call	conditional_hlt
	jmp hard_stop
	
conditional_hlt:
//DS should already be 0, but ...
	pushw	$0
	popw	%ds								// just in case DS changed by buggy BIOS
	testb	$0x20, 0x8205			// Enable HLT?
	jz	1f									// No. Skip the HLT instruction.
	hlt											// Yes. Execute the HLT instruction.
1:
	ret											// on return, DS=0

	.code32

VARIABLE(force_cdrom_as_boot_device)
	.long	1
VARIABLE(use_config_file)
	.long	1
VARIABLE(configfile_opened)
	.long	0

/* If preset_menu == __bss_start, the new menu at end of pre_stage2 will be used. */

#if 0
VARIABLE(preset_menu)
#if defined(PRESET_MENU_STRING)
#if defined(HAVE_USCORE_USCORE_BSS_START_SYMBOL)
	.long	__bss_start - 0x300000
#elif defined(HAVE_USCORE_EDATA_SYMBOL)
	.long	_edata - 0x300000
#elif defined(HAVE_EDATA_SYMBOL)
	.long	edata - 0x300000
#else /* ! HAVE_EDATA_SYMBOL */
#error no bss starting address
#endif /* ! HAVE_EDATA_SYMBOL */
#else  /* ! PRESET_MENU_STRING */
	.long	0
#endif /* PRESET_MENU_STRING */
#endif

VARIABLE(debug_boot)
	.long	0

#if 0
	/* real mode print string */

/* prints string DS:SI (modifies AX BX SI) */

print_message:

	.code16

1:
	sti		/* for hardware interrupt or watchdog */
	cld
	lodsb	(%si), %al	/* get token */
	xorw	%bx, %bx	/* video page 0 */
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	cmpb	$0, %al		/* end of string? */
	jne	1b		/* until done */
	ret

reset_disk_string:
	.ascii	"Reseting the boot drive... \0"
reset_disk_success_string:
	.ascii	"Success.\r\n\0"
reset_disk_failure_string:
	.ascii	"Failure!\r\n\0"
#endif

bochs_copygrght_string:
	.ascii	"(c) 2002 MandrakeSoft S.A. Written by Kevin Lawton & the Bochs team.\0"
ENTRY(bios_id)
	.long	0	/* 1 for bochs, 0 for unknown. */

	.code32


#ifdef FSYS_PXE
/* unsigned long pxe_scan(void)
 *
 * Scan pxe runtime.
 *
 * Return 0 on failure. Return the pxe_entry pointer on success.
 */
ENTRY(pxe_scan)

	.code32

	pushl	%ebx
	pushl	%ebp	/* save this in case int1A would change it! */
	pushl	%esi	/* save this in case int1A would change it! */
	pushl	%edi	/* save this in case int1A would change it! */

	call	EXT_C(prot_to_real)
	.code16

	sti		/* for hardware interrupt or watchdog */
	movw	$0x5650, %ax
	int	$0x1A
	xorl	%ecx, %ecx		//initialize it for "failure"
	cmpw	$0x564E, %ax
	jnz	1f
	cmpl	$0x4E455850, %es:(%bx)		// PXEN(V+)
	jnz	1f
	cmpw	$0x201, %es:6(%bx)		// API version
	jb	1f
	lesw	%es:0x28(%bx), %bx		// !PXE structure
	cmpl	$0x45585021, %es:(%bx)		// !PXE
	jnz	1f
	movw	%es, %cx
	shll	$4, %ecx
	movzwl	%bx, %ebx
	addl	%ebx, %ecx
1:

	call EXT_C(real_to_prot)
	.code32

	sti

	/* EAX=0 */
	testl	%ecx, %ecx
	jz	3f		// failure

	movl	0x10(%ecx), %eax	/* PXE API entry point, segoff16 */
	movl	0x2A(%ecx), %edx
	cmpl	0x32(%ecx), %edx
	ja	1f
	movl	0x32(%ecx), %edx
	movzwl	0x36(%ecx), %ebx
	jmp	2f
1:
	movzwl	0x2E(%ecx), %ebx
2:
	addl	%ebx, %edx
	addl	$1023, %edx
	shrl	$10, %edx
	movw	%dx, EXT_C(pxe_freemem)

	/* calculate base mem */

	/* first, the low mem size */
	movzwl	0x413, %esi
	shll	$10, %esi

	movl	$0xA0000, %ebx

	movl	0x22(%ecx), %edx
	cmpl	%esi, %edx
	jb	4f
	cmpl	%ebx, %edx
	jnb	4f
	movl	%edx, %ebx
4:
	movl	0x2A(%ecx), %edx
	cmpl	%esi, %edx
	jb	4f
	cmpl	%ebx, %edx
	jnb	4f
	movl	%edx, %ebx
4:
	movl	0x32(%ecx), %edx
	cmpl	%esi, %edx
	jb	4f
	cmpl	%ebx, %edx
	jnb	4f
	movl	%edx, %ebx
4:
	movl	0x3A(%ecx), %edx
	cmpl	%esi, %edx
	jb	4f
	cmpl	%ebx, %edx
	jnb	4f
	movl	%edx, %ebx
4:
	movl	0x42(%ecx), %edx
	cmpl	%esi, %edx
	jb	4f
	cmpl	%ebx, %edx
	jnb	4f
	movl	%edx, %ebx
4:
	movl	0x4A(%ecx), %edx
	cmpl	%esi, %edx
	jb	4f
	cmpl	%ebx, %edx
	jnb	4f
	movl	%edx, %ebx
4:
	movl	0x52(%ecx), %edx
	cmpl	%esi, %edx
	jb	4f
	cmpl	%ebx, %edx
	jnb	4f
	movl	%edx, %ebx
4:
	shrl	$10, %ebx
	movw	%bx, EXT_C(pxe_basemem)

3:
	movl	%eax, EXT_C(pxe_entry)

	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
	ret

/* int pxe_call(int func,void* data)
 *
 * PXE function call
 */
ENTRY(pxe_call)

	.code32

	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi
	pushl	%ebx

	movl	8(%ebp), %ecx
	movl	12(%ebp), %edx
	movl	%edx, %eax
	andl	$0xF, %eax
	shrl	$4, %edx
	shll	$16, %edx
	addl	%eax, %edx
	movl	EXT_C(pxe_entry), %ebx

	call	EXT_C(prot_to_real)
	.code16

	sti		/* for hardware interrupt or watchdog */
	pushl	%ebx
	pushl	%edx
	pushw	%cx
	movw	%sp, %bx
	lcall	*%ss:6(%bx)
	cld
	addw	$10, %sp
	movw	%ax, %cx

	call EXT_C(real_to_prot)
	.code32

	sti

	movzwl	%cx, %eax

	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp
	ret

/* this was originally defined in fsys_pxe.c, but should be in low memory.
 *
static PXENV_TFTP_OPEN_t pxe_tftp_open;
 *
 * So define it here.
 *
typedef struct {
  PXENV_STATUS	Status;
  IP4		ServerIPAddress;
  IP4		GatewayIPAddress;
  UINT8		FileName[128];
  UDP_PORT	TFTPPort;
  UINT16	PacketSize;
} PACKED PXENV_TFTP_OPEN_t;
typedef unsigned short	UINT16;
typedef UINT16		PXENV_STATUS;
typedef unsigned long	UINT32;
typedef UINT32		IP4;
typedef unsigned char	UINT8;
typedef UINT16		UDP_PORT;
 *
 */

	.align	2
VARIABLE(pxenv)
VARIABLE(pxe_tftp_open)
	.word	0		# Status
	.long	0		# ServerIPAddress
	.long	0		# GatewayIPAddress
	.space	128		# FileName[128];
	.word	0		# TFTPPort
	.word	0		# PacketSize

#if PXE_FAST_READ

/* int pxe_fast_read(void* data,int num)
 *
 * Read multiple packets
 */
ENTRY(pxe_fast_read)

	.code32

	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi
	pushl	%ebx

	movl	8(%ebp), %edx
	movl	12(%ebp), %esi	/* num */
	movl	EXT_C(pxe_blksize), %edi
	movl	%edx, %eax
	andl	$0xF, %eax
	shrl	$4, %edx
	shll	$16, %edx
	addl	%eax, %edx
	movl	EXT_C(pxe_entry), %ebx

	call	EXT_C(prot_to_real)
	.code16

	sti		/* for hardware interrupt or watchdog */
	movw	%si, %cx	/* num */
1:
	pushw	%cx		/* blocks to read */
	pushw	%di		/* block size */
	pushl	%ebx		/* pxe_entry */
	pushl	%edx		/* data pointer */
	pushw	$0x22		// PXENV_TFTP_READ
	movw	%sp, %bp
	lesw	2(%bp), %si	/* ES:SI=data pointer */

	movw	%di, %es:4(%si)	/* block size */
	lcall	*6(%bp)
	movw	%sp, %bp
	lesw	2(%bp), %si	/* ES:SI=data pointer */
	popw	%dx		// PXENV_TFTP_READ
	popl	%edx		/* data pointer */
	popl	%ebx		/* pxe_entry */
	popw	%di		/* block size */
	popw	%cx		/* blocks to read */
	cld

	cmpw	$0, %es:(%si)
	jnz	2f
	movw	%es:4(%si), %bp
//	cmpw	$512, %dx
//	jb	2f
//	cmpw	%di, %dx
//	ja	2f
//	je	3f
//	cmpl	EXT_C(filemax), %edx
//	jae	3f
//	movl	%edx, EXT_C(pxe_blksize)
//	movl	%edx, %edi
//3:
	addw	%bp, %es:6(%si)
	cmpw	%di, %bp
	jb	2f
	loop	1b

2:
	//addw	$10, %sp
	movw	%ax, %cx

	call EXT_C(real_to_prot)
	.code32

	sti

	movzwl	%cx, %eax

	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp
	ret
#endif /* PXE_FAST_READ */
#endif /* FSYS_PXE */

/*
 * stop_floppy()
 *
 * Stops the floppy drive from spinning, so that other software is
 * jumped to with a known state.
 */
ENTRY(stop_floppy)

	.code32

	pushal
	call	EXT_C(prot_to_real)
	.code16
	sti	#; added 2006-11-30
	xorb	%dl, %dl
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	call EXT_C(real_to_prot)
	.code32
	sti

	popal
	ret

/*
 * boot_int18()
  */
ENTRY(boot_int18)

	.code32

	call	EXT_C(prot_to_real)

	.code16
	int $0x18
	ljmp	$0xFFFF, $0x0000

/*
 * grub_reboot()
 *
 * Reboot the system. At the moment, rely on BIOS.
 */
ENTRY(grub_reboot)

	.code32

	call	EXT_C(prot_to_real)

	.code16

	/* cold boot */

	//sti		/* needn't enable interrupt here. comment it out */

	movw	$0x0472, %di
	movw	%ax, (%di)
	ljmp	$0xFFFF, $0x0000

/*
 * grub_halt(int skip_flags)
 *
 * Halt the system, using APM or ACPI if possible. If NO_APM is true, don't use
 * APM even if it is available.
 */

ENTRY(grub_halt)
	.code32
	
	cli
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax		/* get the argument: skip_flags */

	/* first, try APM BIOS call. */
	testl	$1, %eax		/* bit 0=1 for --no-apm */
	jnz	1f
	call	try_apm
1:
	/* second, try ACPI. */
	testl	$2, %eax		/* bit 1=1 for --no-acpi */
	jnz	1f
	call 	acpi_soft_off
1:
	/* halt failed. */
	xorl	%eax, %eax		/* return 0 for false */

	pushl	%eax			/* EAX=0 */
	popfl				/* cld, cli, ... */

	movl	$74, EXT_C(errnum)	/* set errnum to ERR_HALT(=74) */
	popl	%ebp
	ret

acpi_soft_off:		/* contributor: zw2312914(bbs.znpc.net) */

	.code32

	pushal
	
	/* find RSDP in 0xE0000-0xFFFFF. it should be there in most cases. */
	movl	$0xE0000, %eax
	movl	$(0x20000/0x10), %ecx
	call	find_rsdp
	jnz	3f			/* rsdp found */
	
	/* try the 1st 1KB of EBDA */
	movzwl	0x40E, %eax		/* EBDA segment */
	testl	%eax, %eax		/* segment should >= 0x8000 */
	jns	1f			/* no EBDA, fail */
	shll	$4, %eax		/* EAX=physical address */
	movl	$(1024/16), %ecx
	call	find_rsdp
	jz	1f			/* fail */
3:
	/* locate rsdt, ignoring xsdt table for simplicity. */
	movl	16(%eax), %esi		/* physical address of RSDT */
	cmpl	$0x54445352, (%esi)	/* 'RSDT' */
	jnz	1f			/* fail, ignoring xsdt */
	movl	4(%esi), %ecx		/* RSDT length */
	subl	$0x24, %ecx
	jbe	1f			/* fail */
	shrl	$2, %ecx
	addl	$0x24, %esi

4://find_facp:
	cld
	lodsl
	cmpl	$0x50434146, (%eax)	/* 'FACP' */
	jz	5f			/* found, exit loop */
	loop	4b			/* try next DWORD */
	jmp	1f			/* fail */

5://facp_found:
	movl	%eax, %ebx		/* fadt address */
	cmpl	$0, 64(%ebx)		/* PM1a_CNT_BLK port must be true */
	jz	1f			/* fail */
	
	movl	$1, %edi		/* pass EDI = 1 for enable ACPI */
	call 	write_smi_port		/* this call running at here , purpose of the delay is sufficient */
	jz	1f			/* fail, don't transform */
	
	movl	40(%ebx), %edi		/* physical address of DSDT */
	cmpl	$0x54445344, (%edi)	/* 'DSDT' */
	jnz	12f			/* if fail, disable_acpi */
	movl	4(%edi), %eax		/* DSDT length */
	subl	$40, %eax
	jbe	12f			/* if fail, disable_acpi */
	addl	$36, %edi

6://locate_S5:
	cmpl	$0x5F35535F, (%edi)	/* '_S5_' */
	jnz	7f
	cmpb	$0x08, -1(%edi)		/* NameOp */
	jz	8f
	cmpb	$0x08, -2(%edi)		/* NameOp with RootChar prefix '\' */
	jnz	7f

8://locate_next:	
	#; DefPackage := PackageOp PkgLength NumElements PackageElementList
	#; PackageOp  := 0x12
	#; NumElements        := ByteData
	#; PackageElementList := Nothing | <PackageElement PackageElementList>
	#; PackageElement     := DataRefObject | NameString
	#; PkgLength          := PkgLeadByte |
	#;                       <PkgLeadByte ByteData> |
	#;                       <PkgLeadByte ByteData ByteData> |
	#;                       <PkgLeadByte ByteData ByteData ByteData>
	#; PkgLeadByte        := <bit 7-6: ByteData count that follows (0-3)>
	#;                       <bit 5-4: Only used if PkgLength < 63>
	#;                       <bit 3-0: Least significant PkgLength nybble>
	#; Note: The high 2 bits of the first byte reveal how many follow bytes
	#; are in the PkgLength. If the PkgLength has only one byte, bit 0
	#; through 5 are used to encode the package length (in other words,
	#; values 0-63). If the package length value is more than 63, more than
	#; one byte must be used for the encoding in which case bit 4 and 5 of
	#; the PkgLeadByte are reserved and must be zero. If the multiple bytes
	#; encoding is used, bits 0-3 of the PkgLeadByte become the least
	#; significant 4 bits of the resulting package length value. The next
	#; ByteData will become the next least significant 8 bits of the
	#; resulting value and so on, up to 3 ByteData bytes. Thus, the maximum
	#; package length is 2**28.

	cmpb	$0x12, 4(%edi)			/* PackageOp */
	jnz	7f
	cmpb	$4, 5(%edi)				/* PkgLeadByte,  the minimum must be 4 for \S5_ object */
	jb	7f
	movb	6(%edi), %dl			/* NumElements */

	cmpb	$4, %dl					/* 	Complete _S5_ object should contain 4 NumElements,
										but some BIOSes reduced to 2 . 
										because 2 reserved NumElements be ignored */
	ja	7f
	cmpb	$0, %dl
	ja	8f							/* find succeeded */

7://loop_continue:
	incl	%edi
	decl	%eax
	jnz	6b
	jmp	12f							/* if fail, disable_acpi */

8://get_slp_typx:
	leal	7(%edi), %edi
	call 	data_analyze
	incl 	%edi
	call	data_analyze			/* slp_typ_a in %cl, slp_typ_b in %ch */
	andl	$0x0707, %ecx			/* slp_typX only have 3 bit */
	shll	$2, %ecx				/* the description of slp_typX in pm1_cnt_blk Description see below */

/* The following will enter G2 state (ie acpi soft off)*/
/* AT here, %ebx = FADT address, %cx = slp_typX(a and b), other regsiters is free */

/* Thanks for MR.rockrock99 and MR.xianglang reports their different and special hardware response mechanism */

9://is_keep_SCI:
	testl	$0x4, 8(%ebp)	/* check for --force-sci */
	jnz	10f		/* if --force-sic is true, don't disable acpi */
	xorl	%edi, %edi	/* pass EDI = 0 for disable SCI */
	call	write_smi_port	/* faster close SCI for MR.xianglang's Pc */
	jz	1f							/* fail, don't transform */
	
10://read_pm1_cnt_group:
	xorl	%esi, %esi
	movl	64(%ebx), %edx	/* PM1a_CNT_BLK port */
	inw	%dx, %ax
	call	wait_io_delay
	call	test_pm1b_port	/* when pm1b_cnt_blk value is true, will use slp_typ_b port */
	jz	11f
	xchgl	%eax, %esi
	inw	%dx, %ax
	call	wait_io_delay
	
11://write_pm1_cnt_group:
	orl	%esi, %eax	/* PM1_cnt_group state is pm1a OR pm1b */
	movl	64(%ebx), %edx	/* PM1a_CNT_BLK port */
	call	write_pm1_cnt_group
	call	test_pm1b_port 	/* when pm1b_cnt_blk value is true, will use slp_typ_b port */
	jz	12f
	xchgb	%ch, %cl
	call	write_pm1_cnt_group
	
12://disable_acpi:		/* acpi soft off failed. disable acpi if possible!!! */
	xorl	%edi, %edi
	call	write_smi_port	/* pass EDI = 0 for disable ACPI */

1://acpi_fail:
	popal
	ret

find_rsdp:
	/* will change EAX, ECX, EDX, ESI */
1:
	cmpl	$0x20445352, (%eax)	/* 'RSD ' */
	jnz	3f
	cmpl	$0x20525450, 4(%eax)	/* 'PTR ' */
	jnz	3f
	
	/* checksum for rsdp version 0 */
	xorl	%edx, %edx
	xorl	%esi, %esi
2:
	addb	(%eax, %esi), %dl
	incl	%esi
	cmpl	$20, %esi
	jnz	2b
	jz	4f
3:
	leal	16(%eax), %eax
	loop	1b
4:
	testl	%ecx, %ecx	/* success with ZF=0, failure with ZF=1 */
	ret

test_pm1b_port:
	movl	68(%ebx), %edx	/* PM1b_CNT_BLK port */
	testl	%edx, %edx
	ret

data_analyze:
	cmpb	$0x0A, (%edi)	/* ByteData Prefix */
	jnz	1f
	incl	%edi
1:
	movb 	(%edi), %cl
	xchgb	%cl, %ch
	ret

wait_io_delay:			/* NOTE: we have not save %edi, and %edi will be set to zero */
	movl	$0x1FFFFF, %edi
1:
	nop
	decl	%edi
	jnz	1b
	ret

write_smi_port:
	/* input: %edi = 0 for disable acpi, %edi = 1 for enable acpi */
	movl	48(%ebx), %edx	/* SMI_CMD port */
	testl   %edx, %edx	/* must be non-zero */
	jz	2f
	movw	52(%ebx), %ax	/* AH for ACPI_DISABLE, AL for ACPI_ENABLE */
	testl	%edi, %edi
	jnz	1f
	xchgb	%ah, %al
1:
	testb	%al, %al	/* must be non-zero */
	jz	2f
	outb	%al, %dx
	call	wait_io_delay
	testl	%eax, %eax
2:
	ret

write_pm1_cnt_group:
	/*	PM1 Control Registers Fixed Hardware Feature Control Bits
	Bit    Name	Description
	---    ----	------------------------------------------------------
	0      SCI_EN	Selects the power management event to be either an SCI
			or SMI interrupt for the following events. When this
			bit is set, then power management events will generate
			an SCI interrupt. When this bit is reset power
			management events will generate an SMI interrupt. It
			is the responsibility of the hardware to set or reset
			this bit. OSPM always preserves this bit position.
	1      BM_RLD	When set, this bit allows the generation of a bus
			master request to cause any processor in the C3 state
			to transition to the C0 state. When this bit is reset,
			the generation of a bus master request does not affect
			any processor in the C3 state.
	2      GBL_RLS	This write-only bit is used by the ACPI software to
			raise an event to the BIOS software, that is,
			generates an SMI to pass execution control to the BIOS
			for IA-PC platforms. BIOS software has a corresponding
			enable and status bit to control its ability to
			receive ACPI events (for example, BIOS_EN and
			BIOS_STS). The GBL_RLS bit is set by OSPM to indicate
			a release of the Global Lock and the setting of the
			pending bit in the FACS memory structure.
	3-8    Reserved These bits are reserved by OSPM.
	9      Ignore	Software ignores this bit field.
	10-12  SLP_TYPx	Defines the type of sleeping state the system enters
			when the SLP_EN bit is set to one. This 3-bit field
			defines the type of hardware sleep state the system
			enters when the SLP_EN bit is set. The \_Sx object
			contains 3-bit binary values associated with the
			respective sleeping state(as described by the object).
			OSPM takes the two values from the \_Sx object and
			programs each value into the respective SLP_TYPx
			field.
	13     SLP_EN   This is a write-only bit and reads to it always return
			a zero. Setting this bit causes the system to sequence
			into the sleeping state associated with the SLP_TYPx
			fields programmed with the values from the \_Sx
			object.
	14-15  Reserved	This field always returns zero. */
	
	andw	$0x63F9, %ax	/* reserved and ignored bits must use read back value, clear others */	   
	orb	%cl, %ah	/* set new slp_typX from \S5 object */
	bts	$13, %ax	/* set slp_en bit */
	outw	%ax, %dx
	call	wait_io_delay
	ret
	
try_apm:
	
	call	EXT_C(prot_to_real)

	.code16
	
	pushw	%ds
	pushw	%es
	pushal

	sti

	/* detect APM */
	movw	$0x5300, %ax
	xorw	%bx, %bx
	int	$0x15
	jc	1f		/* APM fail */
	/* don't check %bx for buggy BIOSes... */

	/* connect real mode interface of APM */		
	movw	$0x5301, %ax
	xorw	%bx, %bx
	int	$0x15
	jc	1f		/* APM fail */

	/* set APM protocol level - 1.1 or bust. (this covers APM 1.2 also) */
	movw	$0x530E, %ax
	xorw	%bx, %bx
	movw	$0x0101, %cx
	int	$0x15
	jc	1f

	/* turn off system by APM 1.1+ */
	movw	$0x5307, %ax
	movw	$1, %bx
	movw	$3, %cx
	int	$0x15

1:
	popal
	popw	%es
	popw	%ds

	call	EXT_C(real_to_prot)

	.code32
	sti

	ret			/* APM code end */

	/* end of halt code */

////////////////////////////////////////////////////////////////////////////

/*
 * int tpm_init (void)
 *      return non-zero for success and zero for failure.
 */
ENTRY(tpm_init)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	call	EXT_C(prot_to_real)	/* enter real mode */

	.code16

	sti

	movw	$0xBB00, %ax
	int	$0x1A
	testl	%eax, %eax
	jnz	1f			/* failure */
	cmpl	$0x41504354, %ebx	/* "TCPA" */
	jnz	1f			/* failure */
	cmpw	$0x102, %cx		/* TCG BIOS version 1.2 */
	jb	1f			/* failure */
	movw	%ax, %es		/* ES=0 */
	movw	$0xBB07, %ax		/* eax hi word=0 */
	movl	$0x00000200, %ecx	/* buffer size to hash */
	movl	$0x00000008, %edx	/* PCR index for the hashed result */
	xorl	%esi, %esi		/* place 0 into the event field */
	movl	$0x00007C00, %edi	/* ES:DI point to data buffer to hash */
	int	$0x1A

	testl	%eax, %eax
1:
	setz	%dl

	call	EXT_C(real_to_prot)
	.code32

	sti

	movzbl	%dl, %eax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/* Catch CPU exceptions 0 - 7
 *	0	Divide
 *	1	Debug
 *	2	NMI
 *	3	Break point
 *	4	Overflow
 *	5	Bound
 *	6	Invalid Instruction
 *	7	no coprocessor
 */

set_fault_recovery_handler:

	.code16

	pushfw
	pushw	%ds
	pushw	%es
	pushaw

	/* backup int 00 - 07 */

	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es
	xorw	%si, %si
	movw	$ABS(int_00_07_vectors), %di
	movw	$16, %cx
	cld
	repz movsw

	/* set to new vector */

	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es
	xorw	%si, %si
	movw	$ABS(int_00_07_vectors), %di

	pushl	%eax

	xorw	%di, %di
	movl	$ABS(fault_recovery_handler), %eax	/* 0000:fault_recovery_handler */
	movw	$8, %cx
	cld
	repz stosl

	popl	%eax

	popaw
	popw	%es
	popw	%ds
	popfw
	ret

unset_fault_recovery_handler:

	.code16

	pushfw
	pushw	%ds
	pushw	%es
	pushaw

	/* restore int 00 - 07 */

	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es
	xorw	%di, %di
	movw	$ABS(int_00_07_vectors), %si
	movw	$16, %cx
	cld
	repz movsw

	popaw
	popw	%es
	popw	%ds
	popfw
	ret

	.align	4

int_00_07_vectors:
	.space	32

original_registers:
	.space	48

safe_int13:

	.code16

	/* setup our fault recovery handler */

	call	set_fault_recovery_handler

	/* backup old registers. Note: CS=0 */

	movw	%ds, %cs:ABS(original_registers)
	movw	%es, %cs:ABS(original_registers) + 4
	movw	%ss, %cs:ABS(original_registers) + 8
	movl	%esp, %cs:ABS(original_registers) + 12
	movl	%eax, %cs:ABS(original_registers) + 16
	movl	%ebx, %cs:ABS(original_registers) + 20
	movl	%ecx, %cs:ABS(original_registers) + 24
	movl	%edx, %cs:ABS(original_registers) + 28
	movl	%esi, %cs:ABS(original_registers) + 32
	movl	%edi, %cs:ABS(original_registers) + 36
	movl	%ebp, %cs:ABS(original_registers) + 40

	pushw	%bp
	pushw	%ax
	movw	%sp, %bp
	movw	4(%bp), %ax
	movw	%ax, %cs:ABS(original_registers) + 44	#; return IP
	popw	%ax
	popw	%bp

	int	$0x13

	call	unset_fault_recovery_handler

	ret

fault_recovery_handler:

	.code16

	/* restore old registers. Note: CS=0 */

	movw	%cs:ABS(original_registers), %ds
	movw	%cs:ABS(original_registers) + 4, %es
	movw	%cs:ABS(original_registers) + 8, %ss
	movl	%cs:ABS(original_registers) + 12, %esp
	movl	%cs:ABS(original_registers) + 16, %eax
	movl	%cs:ABS(original_registers) + 20, %ebx
	movl	%cs:ABS(original_registers) + 24, %ecx
	movl	%cs:ABS(original_registers) + 28, %edx
	movl	%cs:ABS(original_registers) + 32, %esi
	movl	%cs:ABS(original_registers) + 36, %edi
	movl	%cs:ABS(original_registers) + 40, %ebp

	/* stack is available, so we can push and pop. */

	pushw	%bp
	pushw	%ax
	movw	%sp, %bp
	movw	%cs:ABS(original_registers) + 44, %ax	#; return IP
	movw	%ax, 4(%bp)
	popw	%ax
	popw	%bp

	pushl	$1		#; CF=1 indicating error
	popfl			#; CLD, CLI, and many more...

	call	unset_fault_recovery_handler

	ret

	/* never come here. */

	iret

#if 0
/*
 * Translate a key code to another.
 *
 * Note: This implementation cannot handle more than one length
 * scancodes (such as Right Ctrl).
 */

int15_handler:

	.code16

	/* if non-carrier, ignore it */
	jnc	1f
	/* check if AH=4F */
	cmpb	$0x4F, %ah
	jne	1f

	/* E0 and E1 are special */
	cmpb	$0xE1, %al
	je	4f
	cmpb	$0xE0, %al
	/* this flag is actually the machine code (je or jmp) */
int15_skip_flag:
	je	4f

	pushw	%bp
	movw	%sp, %bp

	pushw	%bx
	pushw	%dx
	pushw	%ds
	pushw	%si

	/* save bits 0-6 of %al in %dl */
	movw	%ax, %dx
	andb	$0x7f, %dl
	/* save the highest bit in %bl */
	movb	%al, %bl
	xorb	%dl, %bl
	/* set %ds to 0 */
	xorw	%ax, %ax
	movw	%ax, %ds
	/* set %si to the key map */
	movw	$ABS(EXT_C(bios_key_map)), %si

	/* find the key code from the key map */
2:
	lodsw
	/* check if this is the end */
	testw	%ax, %ax
	jz	3f
	/* check if this matches the key code */
	cmpb	%al, %dl
	jne	2b
	/* if so, perform the mapping */
	movb	%ah, %dl
3:
	/* restore %ax */
	movw	%dx, %ax
	orb	%bl, %al
	/* make sure that CF is set */
	orw	$1, 6(%bp)
	/* restore other registers */
	popw	%si
	popw	%ds
	popw	%dx
	popw	%bx
	popw	%bp
	iret

4:
	/* tricky: jmp (0x74) <-> je (0xeb) */
	xorb	$(0x74 ^ 0xeb), ABS(int15_skip_flag)
1:
	/* just cascade to the original */
	/* ljmp */
	.byte	0xea
int15_offset:	.word	0
int15_segment:	.word	0

	.align	4
ENTRY(bios_key_map)
	.space	(KEY_MAP_SIZE + 1) * 2
#endif


/*
 * Map a drive to another drive or a disk image file.
 */

	.code16

	.align	4

ENTRY(bios_drive_map)
	.space	(DRIVE_MAP_SIZE + 1) * DRIVE_MAP_SLOT_SIZE

	/* align it this way so that int13_handler can be used as a segment
	 * base address. The `cdrom' command requires this.
	 */
	.align	16

int13_handler:

	/* memory size in K that int13 handler uses. */
#ifdef CDROM_INIT
	.byte	((int13_handler_end - int13_handler + 0x3ff) / 0x400)
#else
	.byte	((cdrom_start - int13_handler + 0x3ff) / 0x400)
#endif
	/* 1-byte space reserved. */

	. = int13_handler + 0x2

ENTRY(atapi_dev_count)	.byte	0
ENTRY(min_cdrom_id)	.byte	0xE0

	. = int13_handler + 0x4

	/* Signature */
	.ascii	"G4DS"		# Please don't use this signature any longer.
				# This field might be used for other purposes.
				# Use signature at offset 0x103 instead.

	. = int13_handler + 0x08

	/* Version number */
	.word	1

	. = int13_handler + 0x0A

VARIABLE(floppies_orig)
	.byte	0			/* original value at 0040:0010 */

	. = int13_handler + 0x0B

VARIABLE(harddrives_orig)
	.byte	0			/* original value at 0040:0075 */

	. = int13_handler + 0x0C

VARIABLE(ROM_int15)
	.long	0			/* original int15 vector */

	. = int13_handler + 0x10

	/* 12-byte space reserved. */

	. = int13_handler + 0x1C

VARIABLE(ROM_int13)
	.long	0		/* original int13 vector */

	. = int13_handler + 0x20	/* drive map table begins at 0x20 */

#if	MAP_NUM_16
ENTRY(hooked_drive_map_1)
	.space	(DRIVE_MAP_SIZE / 2 + 1) * DRIVE_MAP_SLOT_SIZE
#else
ENTRY(hooked_drive_map)
	.space	(DRIVE_MAP_SIZE + 1) * DRIVE_MAP_SLOT_SIZE
	/* 8-byte space reserved. */
#endif

	. = int13_handler + 0xFC
	.long	16

	. = int13_handler + 0x100	/* real int13 handler entry at 0x100 */
	
int13_handler_entry:
	jmp	1f

	. = int13_handler + 0x103

	/*******************************************************************/
	/* ----------------- SafeMBRHook structure begin ----------------- */

	.ascii	"$INT13SF"		/* Win9x use this! Don't touch! */

	.ascii	"GRUB4DOS"		/* 8-byte Vender ID */

	. = int13_handler + 0x113

VARIABLE(ROM_int13_dup)	/* Win9x Safe-MBR-Hook structure requires this! */

	.long	0

	. = int13_handler + 0x117
	
VARIABLE(safe_mbr_hook)

	/* 2012-12-13: set to 0 according to sha0:
	 *	http://reboot.pro/topic/17881-g4d-safe-hook-flags/
	 *	http://www.osronline.com/ddkx/w98ddk/storage_5l6g.htm
	 *
	 * 2013-01-12: sometimes 0 causes Win98 boot failure, so restore
	 *		the original value of 1. See also:
	 *	http://www.osronline.com/ddkx/w98ddk/storage_5o6r.htm
	 *		which says:
	 *
	 *	Safe MBR Hook
	 *
	 * To create a safe MBR hook for translation, you can either write
	 * an MBR hooker and a corresponding VSD or just write an MBR hooker
	 * that sets the smhflgs member to 1. In either case, the MBR hooker
	 * must have a SafeMBRHook structure at the beginning of its hook
	 * routine, as described in About Vendor-Supplied Drivers.
	 *
	 * You do not have to write a VSD if the MBR hooker only has
	 * functionality that the Windows 95 IDE driver already supports. In
	 * this case, the MBR hooker can set the smhflgs member to 1 on its
	 * own so that Windows 95 can take over in protect mode.
	 *
	 * If the MBR hooker has functionality that is not already supported
	 * by Windows 95, for example, encryption, you must write a VSD or
	 * leave your MBR hooker marked as unsafe. If you write a VSD to
	 * support the extra functionality in protect mode, the VSD is
	 * responsible for setting the smhflgs member to 1 to indicate to
	 * Windows 95 that it is safe to take over in protect mode.
	 *
	 *	Built on Friday, April 11, 2003
	 */
	.long	0x00000001		/* safe MBR hook flag */

	/* -----------------  SafeMBRHook structure end  ----------------- */
	/*******************************************************************/

	/* But Win9x may expect additional data after SafeMBRHook structure.
	 * This is undocumented, and mysterious. If this area is not what
	 * Win9x expected, Win9x could hang.
	 */

	. = int13_handler + 0x11B

VARIABLE(int13_scheme)

	/* bit 0 controls how we access sectors in protected mode.
	 * bit0=0: use pushf and far call for ROM int13 service.
	 * bit0=1: use the `int $0x13' instruction.
	 */
	.long	0x00000001
	
	. = int13_handler + 0x120

VARIABLE(saved_pxe_ip)

	.long	0

	. = int13_handler + 0x124

VARIABLE(saved_pxe_mac)
	
	.byte	0, 0, 0, 0, 0, 0

	/* space reserved. */

	. = int13_handler + 0x140
	.ascii	"FRAGMENT"
	
ENTRY(hooked_fragment_map)	.space	FRAGMENT_MAP_SLOT_SIZE
	
restore_old_emu:
  
	/* GDT used by int13_handler RAM disk (32-bit protected mode, PAE
	 * paging, and 64-bit long mode).
	 * Linear address will be set by set_int13_handler routine.
	 * int13_handler does not have to modify it.
	 */
	.align 16
MyGDT:
//gdt:
/* 0*/	.word	0	/* NULL entry. Can be used at will. */
gdtdesc:		/* 6 bytes used for gdt descriptor. */
	.word	MyGDTEnd - MyGDT - 1	/* GDT limit */
	/* The gdt linear base address will be adjusted by set_int13_handler,
	 * but not touched outside int13_handler. */
	.long	ABS(MyGDT)		/* GDT linear address */

/* 8*/	PM_DS16 = (. - MyGDT)	/* adjusted base = int13_handler */
	.long	0x0000FFFF, 0x00009300	/* 16-bit data 64K limit */
/*16*/	PM_DS32 = (. - MyGDT)
	.long	0x0000FFFF, 0x00CF9300	/* 32-bit data 4GB limit */

	/* The 16-bit code segment base will be adjusted by set_int13_handler,
	 * but not touched outside int13_handler. */
/*24*/	PM_CS16 = (. - MyGDT)	/* adjusted base = int13_handler */
	.long	0x0000FFFF, 0x00009B00	/* 16-bit code 64K limit */
/*32*/	LM_CS64 = (. - MyGDT)
	.long	0x0000FFFF, 0x00AF9B00	/* 64-bit code 4GB limit */

	/* This gdt entry is not used by int13_handler, but used outside. */
/*40*/	PM_CS32 = (. - MyGDT)
	.long	0x0000FFFF, 0x00CF9B00	/* 32-bit code 4GB limit */
MyGDTEnd:

OldGDT:
	/* The int13_handler only uses it as a temp space for the OldGDT
	 * variable. Only the last 6 bytes are used by OldGDT, and the
	 * starting 2 bytes are not used by OldGDT. */
	//.word	0
int13_is64bit:
	.byte	0	// bit0 1=support PAE. bit1 1=support AMD64 long mode.
int13_need_64_bit:
int13_prev_DL:		// used in 64-bit code
	.byte	0
OldGDTdesc:
	.word	0
	.long	0

	.align 4
int13_prev_CR0:
	.long	0
int13_prev_CR3:
old_cr3:
	.long	0
int13_prev_CR4:
old_cr4:
	.long	0
int13_prev_PML4T_entry:
	.long	0, 0
int13_prev_PDPT_entry:
	.long	0, 0
int13_prev_PDT_entry:
	.long	0, 0
	.long	0, 0
	.long	0, 0

//DS_seg:	.word	0
//ES_seg:	.word	0
//FS_seg:	.word	0
//GS_seg:	.word	0
//SP_off:	.word	0
//SS_seg:	.word	0
//DX_reg:	.word	0
//DI_reg:	.word	0

data_addr_above_1M:
	.long	0
	.long	0
data_addr_below_1M:
	.long	0
//	.long	0

	.align	4

int13_old_cs_ip:	.long	0
int13_old_eax:		.long	0
int13_old_ebx:		.long	0
int13_old_ecx:		.long	0
int13_old_edx:		.long	0
int13_old_esi:		.long	0
int13_old_edi:		.long	0
int13_old_esp:		.long	0
int13_old_ebp:		.long	0
int13_old_ds:		.word	0
int13_old_es:		.word	0
int13_old_flags:	.word	0
int13_new_bp:		.word	0


/* cache 1 big 2048-byte sector at edd30_disk_buffer */

	.align	4
int13_cdrom_edi:
	.long	0
int13_cdrom_ebx:
	.long	0
int13_cdrom_cx:
	.word	0
int13_cdrom_bx:
	.word	0
int13_cdrom_es:
	.word	0

last_read_cd_drive:
	.byte	0	#; a value < 0x80 normally invalidate the cache

	.align	4
last_read_sector: /* set it to 0xFFFFFFFFFFFFFFFF to invalidate the cache */
	.long	0xFFFFFFFF	#; lo dword. for a valid cache, lowest 2 bits=0
	.long	0xFFFFFFFF	#; hi dword

tmp_dap:
	.long	0
	.long	0
	.long	0
	.long	0

#if 0
1:
	/* backup far return address */
	popl	%cs:(int13_old_cs_ip - int13_handler)

	/* build new stack: flags */
	popw	%cs:(int13_handler_end - 2 - int13_handler)

	/* backup old stack */
	movw	%sp, %cs:(int13_old_sp - int13_handler)
	movw	%ss, %cs:(int13_old_ss - int13_handler)

	/* build new stack pointer */
	movw	$(int13_handler_end - 2 - int13_handler), %cs:(int13_new_sp - int13_handler)
	movw	%cs, %cs:(int13_new_ss - int13_handler)

	/* switch to new stack */
	lssw	%cs:(int13_new_sp - int13_handler), %sp

	//pushfw			/* flags already on stack */
	pushw	%cs
	call	1f

	/* restore old stack */
	lssw	%cs:(int13_old_sp - int13_handler), %sp

	/* transfer control to caller */
	ljmp	*%cs:(int13_old_cs_ip - int13_handler)

	//iret				/* never reach here */

	.align	4

int13_old_cs_ip:	.long	0
int13_old_sp:		.word	0
int13_old_ss:		.word	0
int13_new_sp:		.word	0
int13_new_ss:		.word	0

#endif

1:
	
	/* backup far return address */
	popl	%cs:(int13_old_cs_ip - int13_handler)

	/* backup old flags */
	popw	%cs:(int13_old_flags - int13_handler)

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	cld
	movl	%eax, %cs:(int13_old_eax - int13_handler)
	movl	%ebx, %cs:(int13_old_ebx - int13_handler)
	movl	%ecx, %cs:(int13_old_ecx - int13_handler)
	movl	%edx, %cs:(int13_old_edx - int13_handler)
	movl	%esi, %cs:(int13_old_esi - int13_handler)
	movl	%edi, %cs:(int13_old_edi - int13_handler)
	movl	%esp, %cs:(int13_old_esp - int13_handler)
	movl	%ebp, %cs:(int13_old_ebp - int13_handler)
	movw	%ds, %cs:(int13_old_ds - int13_handler)
	movw	%es, %cs:(int13_old_es - int13_handler)

//	# this check is not too useful, so comment out to save space
//	cmpb	$0x1a, %ah	/* PS/2 low level format ESDI drive!!!! */
//	je	error_01_disable	/* disabled in any case */
#ifdef CDROM_INIT
	cmpb	$0, %cs:(EXT_C(atapi_dev_count) - int13_handler)
	jz	1f		/* no cdrom */
	cmpb	%cs:(EXT_C(min_cdrom_id) - int13_handler), %dl
	jb	1f		/* not cdrom drive */
	cmpb	%cs:(max_cdrom_id - int13_handler), %dl
	jbe	edd30_for_cdrom
1:
#endif
/****************************************************************************/
	/* find the drive number from the drive map */
#if	MAP_NUM_16
	movw	$(EXT_C(hooked_drive_map_1) - int13_handler - DRIVE_MAP_SLOT_SIZE), %bp
#else
	movw	$(EXT_C(hooked_drive_map) - int13_handler - DRIVE_MAP_SLOT_SIZE), %bp
#endif
1:
	addw	$DRIVE_MAP_SLOT_SIZE, %bp
#if	MAP_NUM_16	
	cmpw	$EXT_C(hooked_drive_map_1) - int13_handler + DRIVE_MAP_SIZE * DRIVE_MAP_SLOT_SIZE / 2, %bp
	jne	2f
	movw	$(EXT_C(hooked_drive_map_2) - int13_handler), %bp
2:	
	cmpw	$EXT_C(hooked_drive_map_2) - int13_handler + DRIVE_MAP_SIZE * DRIVE_MAP_SLOT_SIZE / 2, %bp
	je	2f	
#endif
	
	movw	%bp, %cs:(int13_new_bp - int13_handler)
	movl	%cs:(%bp), %eax		/* FROM, TO, Hmax, Smax */

	/* check if this is the end */
	testl	%eax, %eax
	jnz	3f			/* not end, continue */

	movl	%cs:12(%bp), %eax	/* StartLBA_Hi */
	testl	%eax, %eax
	jnz	3f			/* not end, continue */

	movl	%cs:8(%bp), %eax	/* StartLBA_Lo */
	testl	%eax, %eax
	jnz	3f			/* not end, continue */

	movl	%cs:20(%bp), %eax	/* S_count_Hi */
	testl	%eax, %eax
	jnz	3f			/* not end, continue */

	movl	%cs:16(%bp), %eax	/* S_count_Lo */
	testl	%eax, %eax
	jz	2f		/* map whole drive to itself signals the end */
3:
	/* Now this is a valid drive map slot */
	cmpb	%cs:(%bp), %dl	/* check if this matches the drive number */
	jne	1b		/* no, continue to check the next map */

	/* yes, found the map corresponding to drive DL */

//	movw	%cs:2(%bp), %ax		/* AL=Hmax, AH=Smax */

	/* bit 1-5 already cleared for in-situ */
	//andb	$0xC1, %ah		/* clear bit 5 - bit 1 */

	testb	$0x80, %cs:7(%bp)	/* TO_S */
	jnz	1f			/* in-situ */

	/* non-zero StartLBA signals emulation */

	cmpl	$0, %cs:12(%bp)		/* StartLBA_Hi */
	jnz	drive_emulation
	cmpl	$0, %cs:8(%bp)		/* StartLBA_Lo */
	jnz	drive_emulation

	/* StartLBA == 0 */

	/* if FROM and TO are both cdrom, this is a whole drive map. */
	testw	$0x4000, %cs:4(%bp)	/* TO_C bit 14=(TO is cdrom) */
	jz	3f			/* not cdrom */
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jz	3f			/* not cdrom */
#ifdef CDROM_INIT
	movb	%cs:1(%bp), %dl		/* DL changed(!!) to TO_drive */
	cmpb	$0, %cs:(EXT_C(atapi_dev_count) - int13_handler)
	jz	2f	/* no cdrom, so it is a bios drive map. */
	cmpb	%cs:(EXT_C(min_cdrom_id) - int13_handler), %dl	/* TO_drive */
	jb	2f	/* not cdrom, so it is a bios drive map. */
	cmpb	%cs:(max_cdrom_id - int13_handler), %dl	/* TO_drive */
	ja	2f	/* not cdrom, so it is a bios drive map. */
	jmp	edd30_for_cdrom
#endif
3:

	/* S_count being not 1 signals emulation */

	movl	%cs:20(%bp), %eax	/* S_count_Hi */
	testl	%eax, %eax
	jnz	drive_emulation
	movl	%cs:16(%bp), %eax	/* S_count_Lo */
	shrl	$1, %eax
	jnz	drive_emulation

	/* now StartLBA=0 and sector count=1(for whole disk) */

	/* if sectors per track > 1, this is force geometry screw. */

	movw	%cs:2(%bp), %ax		/* AL=Hmax, AH=Smax */
	testb	$62, %ah	/* Sectors > 1 means force geom, this -- */
	jnz	drive_emulation	/* -- also leads to drive emulation */

	/* ignore geom and directly map a whole drive */

1:
	/* bit 7 of the TO_S is for in-situ primary partition(alter MBR) */

	/* bits of AH:
	 *    7		bit set means readonly/fakewrite
	 *    6		bit set means disable LBA
	 *  5 - 1	bits already cleared(=0)
	 *    0		bit cleared means disable CHS
	 * So, if AH!=1, it is a restricted disk access;
	 * and if AH=1, it is a normal disk access.
	 */
//	cmpb	$1, %ah
	testb	$0xc0, %ah
	je	1f
	call	restricted_map
1:
	/* map a whole drive, normal access */

	/* but if --in-situ was used, we should avoid writing the MBR! */
	testb	$0x80, %cs:7(%bp)	/* TO_S */
	jz	1f			/* not in-situ, allow write */
	testb	$0x40, %cs:7(%bp)	/* TO_S. bit 6 here means safe-boot */
	jz	1f			/* unsafe-boot, allow write */
	movw	%cs:(int13_old_eax - int13_handler), %ax
	cmpb	$0x03, %ah		/* is it CHS write? */
	jne	3f

	/* check if it is a write to MBR, i.e., C/H/S=0/0/1 */
	cmpw	$0x0001, %cx		/* C=0, S=1 */
	jne	1f
	cmpb	$0x00, %dh		/* H=0 */
	jne	1f

	/* deny the write and end the int 13 call */
	call	readonly_fakewrite	/* NO RETURN!! */
3:
	cmpb	$0x43, %ah		/* is it LBA write? */
	jne	1f			/* no, continue the normal access */

	/* check if it is a write to MBR, i.e., LBA=0 */

	xorl	%eax, %eax
	orl	12(%si), %eax		/* LBA_hi */
	orl	8(%si), %eax		/* LBA_lo */
	je	readonly_fakewrite	/* deny the write and end */

1:
	movb	%cs:1(%bp), %dl	/* Let DL access TO instead of FROM */
2:
	/* might map to itself, i.e., actually not mapped */

	movl	%cs:(int13_old_eax - int13_handler), %eax
	movl	%cs:(int13_old_ebp - int13_handler), %ebp  /* BP changed!! */

	call	backup_int13

	pushw	%cs:(int13_old_flags - int13_handler)
	popfw

	int	$0x13

	/* save the returned CF flag to int13_old_flags */

	jnc	1f
	orb	$1, %cs:(int13_old_flags - int13_handler)
	jmp	2f
1:
	andb	$0xFE, %cs:(int13_old_flags - int13_handler)
2:
	call	restore_int13

	/* restore BP!! */
	movw	%cs:(int13_new_bp - int13_handler), %bp
	xchgw	%ax, %cs:(int13_old_eax - int13_handler)
				/* old AX changed!! */

	/* check int13/AH=4Bh for cdrom */
	testw	$0x4000, %cs:4(%bp)	/* TO_C bit 14=(TO is cdrom) */
	jz	1f			/* not cdrom */
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jz	1f			/* not cdrom */
	cmpb	$0x4B, %ah
	jne	1f
	cmpb	$0x01, %al
	ja	1f
	/* this is int13 function 0x4B00 or 0x4B01 */
	/* restore DS:SI, just in case they were changed by buggy int13 */
	movw	%cs:(int13_old_esi - int13_handler), %si
	movw	%cs:(int13_old_ds - int13_handler), %ds
	cmpb	$0x13, (%si)		/* packet size */
	jne	1f
	testb	$0x0F, 1(%si)		/* boot type(0=no emu) */
	jne	1f
	movb	%cs:1(%bp), %dl		/* just in case DL was changed by a buggy int13 */
	cmpb	%dl, 2(%si)
	jne	1f
	movb	%cs:(%bp), %dl		/* restore DL back to the FROM drive */
	movb	%dl, 2(%si)
	jmp	3f			/* return */
1:


#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	pushw	%cs:(int13_old_flags - int13_handler)
	popfw

	/* check if should restore(reversely map) the drive number */
	jc	1f		/* restore DL on error */

	/* alter MBR after read */

	testb	$0x80, %cs:7(%bp)	/* TO_S */
	jz	2f

	cmpb	$0x02, %ah
	je	4f		/* alter MBR */
	cmpb	$0x42, %ah
	je	4f		/* alter MBR */
2:
	cmpb	$0x08, %ah	/* int13 AH=08h, read drive parameters */
	jne	2f

	/* DL==number of drives, should not restore */
	xorw	%ax, %ax
	movw	%ax, %ds
	movb	0x475, %dl	/* DL=number of hard drives */
	testb	$0x80, %cs:(int13_old_edx - int13_handler) /* harddrive? */
	jnz	3f		/* yes, jump */
	//movw	$0x10, %bx	/* for floppy, refer to function 0x48 */
	movb	0x410, %al
	rorb	$1, %al
	cbw
	shlb	$1, %al
	shrb	$6, %al
	incw	%ax
	andb	%ah, %al
	movb	%al, %dl	/* DL=number of floppy drives */
	//lesw	0x0078, %di	/* point to int 1E floppy parameters */
	jmp	3f
2:
	cmpb	$0x15, %ah	/* read drive type */
	jne	2f
	testb	$0x80, %cs:(int13_old_edx - int13_handler) /* harddrive? */
	jnz	3f		/* yes, do not restore DL */
	/* restore DL for floppy int13 AH=15h call */
	jmp	1f
4:
	call	modify_in_situ
	movl	%cs:(int13_old_ebx - int13_handler), %ebx
	movl	%cs:(int13_old_ecx - int13_handler), %ecx
	movl	%cs:(int13_old_edx - int13_handler), %edx
	movl	%cs:(int13_old_edi - int13_handler), %edi
	movw	%cs:(int13_old_es - int13_handler), %es
2:
	movw	%cs:(%bp), %ax	/* get the drive mapping */
	/* try to restore DL if possible */
	cmpb	%al, %ah	/* check if the mapping was performed */
	je	3f		/* not performed, so need not restore DL */
	cmpb	%dl, %ah
	jne	3f		/* DL changed by int13, so do not restore */
1:
	movb	%cs:(int13_old_edx - int13_handler), %dl	/* restore DL back to the FROM drive */
3:
	/* return */

	/* BX, CX, DX, ES, DI are output registers and should not touch. */

	pushw	%cs:(int13_old_flags - int13_handler)
	popfw

	movl	%cs:(int13_old_esi - int13_handler), %esi
	movw	%cs:(int13_old_ds - int13_handler), %ds
	movl	%cs:(int13_old_eax - int13_handler), %eax
	movl	%cs:(int13_old_ebp - int13_handler), %ebp
	movw	%cs:(int13_old_esp - int13_handler), %sp
	ljmp	*%cs:(int13_old_cs_ip - int13_handler)

/****************************************************************************/
restricted_map:
	movw	%cs:(int13_old_eax - int13_handler), %ax

	/* CHS read functions */
	cmpb	$0x02, %ah	/* read sectors */
	je	2f
	cmpb	$0x04, %ah	/* verify sectors, also a read operation */
//	# these checks are not too useful, so comment out to save space
//	je	2f
//	cmpb	$0x0a, %ah	/* read long sectors */
//	je	2f
//	cmpb	$0x0c, %ah	/* seek to cylinder */
//	je	2f
//	cmpb	$0x21, %ah	/* PS/1 PS/2 - read multiple disk sectors */
	jne	1f
2:
	movb	%cs:3(%bp), %ah	/* AH=Smax */
	testb	$63, %ah	/* check if Sectors=0, i.e., disable CHS */
	jz	error_01_disable
	ret
1:
	/* CHS write functions */
	cmpb	$0x03, %ah	/* CHS write sectors */
	je	2f
	cmpb	$0x05, %ah	/* PC/XT/AT/EISA format tracks */
//	# these checks are not too useful, so comment out to save space
//	je	2f
//	cmpb	$0x06, %ah	/* PC/XT format tracks with bad sectors */
//	je	2f
//	cmpb	$0x07, %ah	/* PC/XT format multiple cylinders */
//	je	2f
//	cmpb	$0x0b, %ah	/* PC/XT/AT/EISA write sectors with ECC */
//	je	2f
//	cmpb	$0x0f, %ah	/* PC/XT/PS/1 write sector buffer */
//	je	2f
//	cmpb	$0x22, %ah	/* PS/1 PS/2 - write multiple disk sectors */
	jne	1f
2:
	movb	%cs:3(%bp), %ah	/* AH=Smax */
	testb	$63, %ah	/* check if Sectors=0, i.e., disable CHS */
	jz	error_01_disable
	testb	$0x80, %ah	/* readonly access? */
	jnz	readonly_fakewrite
	ret
1:
	/* LBA read functions */
	cmpb	$0x41, %ah	/* Extensions - INSTALLATION CHECK */
	je	2f
	cmpb	$0x42, %ah	/* Extensions - EXTENDED READ */
	je	2f
	cmpb	$0x44, %ah	/* Extensions - verify sectors */
	je	2f
//	# these checks are not too useful, so comment out to save space
//	cmpb	$0x45, %ah	/* Extensions - LOCK/UNLOCK DRIVE */
//	je	2f
//	cmpb	$0x46, %ah	/* Extensions - EJECT MEDIA */
//	je	2f
//	cmpb	$0x47, %ah	/* Extensions - EXTENDED SEEK */
//	je	2f
	cmpb	$0x48, %ah	/* Extensions - GET DRIVE PARAMETERS */
//	# these checks are not too useful, so comment out to save space
//	je	2f
//	cmpb	$0x49, %ah	/* Extensions - detect media change */
//	je	2f
//	cmpb	$0x4a, %ah	/* CDROM - INITIATE DISK EMULATION */
//	je	2f
//	cmpb	$0x4b, %ah	/* CDROM - TERMINATE DISK EMULATION */
//	je	2f
//	cmpb	$0x4c, %ah	/* CDROM - INITIATE DISK EMULATION AND BOOT */
//	je	2f
//	cmpb	$0x4d, %ah	/* CDROM - RETURN BOOT CATALOG */
//	je	2f
//	cmpb	$0x4e, %ah	/* Extensions - SET HARDWARE CONFIGURATION */
	jne	1f
2:
	movb	%cs:3(%bp), %ah	/* AH=Smax */
	testb	$64, %ah	/* disable LBA? */
	jnz	error_01_disable
	ret
1:
	/* LBA write functions */
	cmpb	$0x43, %ah	/* Extensions - EXTENDED WRITE */
	jne	1f
2:
	movb	%cs:3(%bp), %ah	/* AH=Smax */
	testb	$64, %ah	/* disable LBA? */
	jnz	error_01_disable
	testb	$0x80, %ah	/* readonly access? */
	jnz	readonly_fakewrite
1:
	/* no restrictions, return and continue */
	ret

error_01_disable:
	/* function not supported, or the input CHS is invalid */
	movl	%cs:(int13_old_eax - int13_handler), %eax
	movb	$0x01, %ah	/* invalid function call */
1:
	pushw	%cs:(int13_old_flags - int13_handler)
	popfw
	stc			/* error */
	jmp	1f

readonly_fakewrite:
	testb	$0x40, %cs:7(%bp)	/* bit 6 of TO_S */
	movl	%cs:(int13_old_eax - int13_handler), %eax
	movb	$0x03, %ah	/* write protection */
	jz	1b		/* read only */
	xorb	%ah, %ah	/* fake write */
	pushw	%cs:(int13_old_flags - int13_handler)
	popfw
	clc			/* write succeeded */
1:
	movl	%cs:(int13_old_esi - int13_handler), %esi
	movw	%cs:(int13_old_ds - int13_handler), %ds
	movl	%cs:(int13_old_ebp - int13_handler), %ebp
	movw	%cs:(int13_old_esp - int13_handler), %sp
	ljmp	*%cs:(int13_old_cs_ip - int13_handler)


/****************************************************************************/
drive_emulation:
	movw	%cs:2(%bp), %ax	/* AL=Hmax, AH=Smax */
	testb	$63, %ah	/* disable CHS? */
	jz	2f		/* yes, call restricted map */
	testb	$0xc0, %ah	/* readonly or disable LBA? */
	jz	1f
2:
	call	restricted_map
1:
	movw	%cs:(int13_old_eax - int13_handler), %ax

	testb	%ah, %ah	/* reset disk system, always succeed */
	jnz	1f
	/*clc*/			/* CF already cleared by TEST */
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x01, %ah	/* get status, always succeed */
	jnz	1f
	xorb	%ah, %ah
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x04, %ah	/* verify sectors, always succeed */
	jnz	1f
	xorb	%ah, %ah
	jmp	int13_return
/****************************************************************************/
//	# this check is not too useful, so comment out to save space
//1:
//	cmpb	$0x09, %ah /* INITIALIZE CONTROLLER WITH DRIVE PARAMETERS */
//	jnz	1f
//	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
//	jnz	error_01_invalid	/* CDROM */
//	xorb	%ah, %ah
//	jmp	int13_return
/****************************************************************************/
//	# this check is not too useful, so comment out to save space
//1:
//	cmpb	$0x0c, %ah	/* SEEK TO CYLINDER */
//	jnz	1f
//	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
//	jnz	error_01_invalid	/* CDROM */
//	xorb	%ah, %ah
//	jmp	int13_return
/****************************************************************************/
//	# this check is not too useful, so comment out to save space
//1:
//	cmpb	$0x0d, %ah	/* reset hard disks */
//	jnz	1f
//	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
//	jnz	error_01_invalid	/* CDROM */
//	xorb	%ah, %ah
//	jmp	int13_return
/****************************************************************************/
//	# this check is not too useful, so comment out to save space
//1:
//	cmpb	$0x10, %ah	/* check if drive ready */
//	jnz	1f
//	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
//	jnz	error_01_invalid	/* CDROM */
//	xorb	%ah, %ah
//	jmp	int13_return
/****************************************************************************/
//	# this check is not too useful, so comment out to save space
//1:
//	cmpb	$0x11, %ah	/* recalibrate drive */
//	jnz	1f
//	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
//	jnz	error_01_invalid	/* CDROM */
//	xorb	%ah, %ah
//	jmp	int13_return
/****************************************************************************/
//	# this check is not too useful, so comment out to save space
//1:
//	cmpb	$0x14, %ah	/* CONTROLLER INTERNAL DIAGNOSTIC */
//	jnz	1f
//	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
//	jnz	error_01_invalid	/* CDROM */
//	xorb	%ah, %ah
//	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x05, %ah	/* format track */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */

	/* CH=cylinder number (bits 8,9 in high bits of CL)
	 * CL=high bits of cylinder number (bits 7,6)
	 * DH=head number
	 * DL=drive number
	 */

	xorb	%ah, %ah	/* do nothing but return success */
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x08, %ah	/* get drive parameters */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */
	pushw	%ds
	xorw	%ax, %ax
	movw	%ax, %ds
	testb	%dl, %dl	/* hard drive? */
	movb	0x475, %dl	/* DL=number of hard drives */
	js	2f		/* yes, jump */
	movw	$0x10, %bx	/* for floppy, refer to function 0x48 */
	movb	0x410, %al
	rorb	$1, %al
	cbw
	shlb	$1, %al
	shrb	$6, %al
	incw	%ax
	andb	%ah, %al
	xchgw	%ax, %dx	/* DL=number of floppy drives */
	lesw	0x0078, %di	/* point to int1E floppy parameters */
2:
	popw	%ds
	movw	%cs:2(%bp), %ax	/* AL=Hmax, AH=Smax */
	movb	%al, %dh	/* max head number */
	andb	$63, %ah
	movb	%ah, %cl	/* max sector number */

	/* max cylinder number */

	pushl	%edx
	pushl	%ecx
	movzbl	%dh, %eax
	movzbl	%cl, %ecx
	incl	%eax
	mull	%ecx		/* EDX=0, EAX=sectors per cylinder */
	xchgl	%eax, %ecx

	movl	%cs:16(%bp), %eax	/* S_count_Lo */
	cmpl	$4, %eax
	ja	2f
	cmpl	$0, %cs:8(%bp)		/* StartLBA_Lo */
	jnz	2f
	/* map whole drive, use TO_C instead. */
	movw	%cs:4(%bp), %ax		/* TO_C */
	jmp	3f
2:
	decl	%eax
	//xorl	%edx, %edx
	divl	%ecx		/* EAX=max cylinder number */
3:
	popl	%ecx
	popl	%edx

	movb	%al, %ch	/* low 8 bits of cylinder */
	shlb	$6, %ah		/* high 2 bits of cylinder */
	orb	%ah, %cl

	xorw	%ax, %ax
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x15, %ah	/* get disk type */
	jnz	1f
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */
	testb	%dl, %dl	/* hard drive? */
	js	2f		/* yes, jump */
	movb	$0x02, %ah	/* floppy (or other removable drive) with change-line support */
	clc
	jmp	int13_return
2:
	movb	$0x03, %ah	/* hard disk */

	/* CX:DX=total number of sectors */
	movw	%cs:16(%bp), %dx	/* lo word of S_count_Lo */
	movw	%cs:18(%bp), %cx	/* hi word of S_count_Lo */
	clc
	jmp	int13_return
/****************************************************************************/
	# lbacache requires this. See http://reboot.pro/14924/
1:
	# FLOPPY DISK - DETECT DISK CHANGE (XT 1986/1/10 or later,XT286,AT,PS)

	cmpb	$0x16, %ah
	jnz	1f
//	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
//	jnz	error_01_invalid	/* CDROM */
	xorb	%ah, %ah	/* AH=0 means disk not changed */
	jmp	int13_return
/****************************************************************************/
//	# this check is not too useful, so comment out to save space
//1:
//	cmpb	$0x17, %ah	/* set floppy type for format */
//	jnz	1f
//	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
//	jnz	error_01_invalid	/* CDROM */
//	movb	$0x03, %al	/* 1.44M drive, 1.44M floppy */
//	xorb	%ah, %ah
//	jmp	int13_return
/****************************************************************************/
//	# this check is not too useful, so comment out to save space
//1:
//	cmpb	$0x18, %ah	/* set media type for format */
//	jnz	1f
//	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
//	jnz	error_01_invalid	/* CDROM */
//	testb	%dl, %dl	/* hard drive? */
//	js	error_01_invalid
//	pushw	%ax
//	xorw	%ax, %ax
//	movw	%ax, %es
//	movw	$0x0078, %di
//	movl	%es:(%di), %eax
//	movw	%ax, %di
//	shrl	$0x10, %eax
//	movw	%ax, %es
//	popw	%ax
//	xorb	%ah, %ah
//	jmp	int13_return
/****************************************************************************/
1:
	/* Now AH is neither 0 nor 1 */

	testb	$0xfc, %ah	/* CHS read/write sectors */
	jnz	1f

	/* so AH is either 2(for read) or 3(for write) */

	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	error_01_invalid	/* CDROM */

	cmpw	$0x0301, %ax	/* is it write 1 sector? */
	jne	2f
	cmpw	$0x0001, %cx	/* write to cylinder 0, sector 1? */
	jne	2f
	cmpb	$0x01, %dh	/* write to head 0 or 1? */
	ja	2f
	/* cmpw	$0xaa55, %es:0x1fe(%bx)
	je	2f */
	testb	%dl, %dl
	js	3f		/* protect hard disk head 0 and 1 */
	testb	%dh, %dh	/* write to floppy head 0? */
	jne	2f		/* no, write permitted */
3:
	testb	$0x40, %cs:7(%bp)	/* TO_S, bit 6=fake write(safeboot) */
	jnz	readonly_fakewrite	/* fake the write */
2:
	cmpb	$0x7F, %al	/* check if sectors exceed 127 */
	ja	error_01_invalid
	testb	%al, %al	/* read 0 sectors not allowed */
	jz	error_01_invalid
	testb	$63, %cl	/* beginning sector number 0 is invalid */
	jz	error_01_invalid
	movb	%cs:3(%bp), %ah	/* AH=Smax */
	andb	$63, %ah
	pushw	%cx
	andb	$63, %cl
	cmpb	%ah, %cl	/* CL should not > max sector number */
	popw	%cx
	ja	error_01_invalid
	movb	%cs:2(%bp), %ah	/* AH=Hmax */
	cmpb	%ah, %dh	/* DH should not > max head number */
	ja	error_01_invalid

	movw	%cs, %si	/* use SI as temp var, instead of AX */
	movw	%si, %ds	/* DS=CS */

	movw	$(EBIOS_disk_address_packet - int13_handler), %si
	movw	$0x0010, (%si)	/* Disk Address Packet length */
	movb	%al, 2(%si)	/* number of sectors to transfer */
	movw	%bx, 4(%si)	/* offset */
	movw	%es, 6(%si)	/* segment */
	xorl	%eax, %eax	/* EAX=0 */
	movb	%ah, 3(%si)	/* sectors_hi_reserved */
	movl	%eax, 12(%si)	/* zero out hi 32 bits */
	movb	%ch, %al	/* cylinder number lo 8 bits */
	movb	%cl, %ah	/* CL holds higher 2 bits */
	shrb	$6, %ah		/* AH lower holds the 2 bits */
				/* EAX=cylinder number, <1024 */
	pushl	%ebx		/* save EBX */

	xorl	%ebx, %ebx	/* EBX=0 */
	movb	%cs:2(%bp), %bl	/* BL=Hmax */
	incw	%bx		/* EBX=total heads, <=256 */

	pushl	%edx
	mull	%ebx		/* EDX=0, EAX=tracks before this cylinder */
	popl	%edx

	xorw	%bx, %bx	/* EBX=0 */
	movb	%dh, %bl	/* EBX=head number */
	addl	%ebx, %eax	/* EAX=tracks before this head */
	movb	%cs:3(%bp), %bl	/* Max sector number */
	andb	$63, %bl	/* EBX=sectors per track */

	pushl	%edx
	mull	%ebx		/* EDX=0, EAX=sectors before this head */
	popl	%edx

	movb	%cl, %bl	/* sector number */
	andb	$63, %bl
	decb	%bl
	addl	%ebx, %eax	/* EAX=lo 32 bits of logical sector number */
	movl	%eax, 8(%si)

	popl	%ebx		/* restore EBX */

disk_address_packet_ready:

	/* DS=CS */

	/* start-sector-number(LBA) in DAP is still for FROM_DRIVE */

/****************************************************************************/
	/*
	 * check if the request exceeds the boundary of the emulated disk.
	 *
	 * input:	DS:SI
	 * output:	CF=0, success, all sectors transferred
	 *		CF=1, failure, no sectors transferred
	 *		EAX changed
	 *
	 */
	cmpl	$0, %cs:20(%bp)		/* S_count_Hi */
	ja	2f
	cmpl	$1, %cs:16(%bp)		/* S_count_Lo */
	ja	2f
	cmpl	$0, %cs:12(%bp)		/* StartLBA_Hi */
	jne	2f
	cmpl	$0, %cs:8(%bp)		/* StartLBA_Lo */
	je	4f			/* map whole drive, no restrictions */
2:
	movl	%cs:20(%bp), %eax	/* S_count_Hi */
	cmpl	%eax, 12(%si)		/* requested StartLBA_Hi */
	ja	3f
	jb	2f
	movl	%cs:16(%bp), %eax	/* S_count_Lo */
	cmpl	%eax, 8(%si)		/* requested StartLBA_Lo */
	jnb	3f
2:
	pushl	%ebx
	subl	8(%si), %eax
	movl	%cs:20(%bp), %ebx	/* S_count_Hi */
	sbbl	12(%si), %ebx		/* EBX:EAX available sectors */
	testl	%ebx, %ebx		/* CF=0 */
	popl	%ebx
	jnz	4f
	pushl	%ebx
	movzwl	2(%si), %ebx		/* requested sectors */
	cmpl	%ebx, %eax
	popl	%ebx
//	jnb	4f
//3:
//	stc
4:
/****************************************************************************/

	jc	3f		/* no sectors to transfer, fail */

//			 To_count(0)			To_count(1)					To_count(2)					To_count(3)
//		├──────────────┼───────────────────┼───────────────────┼───────────────────┤
//To_start(0)		To_start(1)					To_start(2)					To_start(3)
//																									Form_len
//	 ┇┅┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄├───────────────────────┤
//	 0																		Form_statr

	pushaw
	movw	2(%si), %ax
	movw	%ax, %cs:(old_form_len - int13_handler)
	movw	4(%si), %ax
	movw	%ax, %cs:(old_form_offset - int13_handler)
	movl	8(%si), %eax
	movl	%eax, %cs:(old_form_statr_lo - int13_handler)
	movl	12(%si), %eax
	movl	%eax, %cs:(old_form_statr_hi - int13_handler)
	movw	$0, %cs:(next_fragment_len - int13_handler)
	xorw	%di, %di
	testw	$0x400, %cs:4(%bp)
	je	no_sp
	movw	$(EXT_C(hooked_fragment_map) - int13_handler), %bp
	
2:
	cmpb	$0, %cs:(%bp)
	je	no_sp
	cmpb	%dl, %cs:2(%bp)
	je	2f
	movw	%cs:(%bp), %ax
	addw	%ax, %bp
	loop	2b
2:
	movw	%bp, %cs:(next_fragment_base - int13_handler)
	movl	8(%si), %eax					//Form_statr_Lo		0000
	movl	12(%si), %ebx					//Form_statr_Hi		0000
	xorl	%ecx, %ecx
	xorl	%edx, %edx
2:
	addl	%cs:12(%bp,%di), %ecx	//∑To_count(0→i) + To_count(i+1)
	adcl	%cs:16(%bp,%di), %edx
	call	compare_64						//Form_statr <> ∑To_count(0→i)
	jb	2f
	addw	$16, %di
//	cmpw	$(16*DRIVE_MAP_FRAGMENT), %di
//	jb	2b
	jmp	2b
2:	
	addl	%cs:12(%bp,%di), %eax	//Form_statr + To_count(i)
	adcl	%cs:16(%bp,%di), %ebx
	addl	%cs:4(%bp,%di), %eax	//Form_statr + To_count(i) + To_start(i)
	adcl	%cs:8(%bp,%di), %ebx
	subl	%ecx, %eax						//Form_statr + To_count(i) + To_start(i) - ∑To_count(0→i)
	sbbl	%edx, %ebx

	subl	8(%si), %ecx						//∑To_count(0→i) - Form_statr
	sbbl	12(%si), %edx
	movl	%eax, 8(%si)						//Form_statr - ∑To_count(0→i-1) + To_start(i)
	movl	%ebx, 12(%si)	
	movzwl	2(%si), %eax				 	//Form_len
	xorl	%ebx, %ebx
	call	compare_64							//Form_len <> ∑To_count(0→i) - Form_statr
	jbe	2f

	movw	%cx, 2(%si)						  //∑To_count(0→i) - Form_statr														
	subw	%cx, %ax								//Form_len - (∑To_count(0→i) - Form_statr)
	movw	%ax, %cs:(next_fragment_len - int13_handler)
	addw	$16, %di
	movw	%di, %cs:(next_fragment_offset - int13_handler)
2:
	popaw
	jmp 2f


no_sp:	
	popaw
	
	/* adjust start-sector-number(LBA) to access TO_DRIVE */
	movl	%cs:8(%bp), %eax	/* StartLBA_Lo */
	addl	%eax, 8(%si)
	movl	%cs:12(%bp), %eax	/* StartLBA_Hi */
	adcl	%eax, 12(%si)
	
2:
	/* set drive number(TO_DRIVE) and function number(EBIOS) */
	movb	%cs:1(%bp), %dl		/* DL=TO_DRIVE */
	movb	%cs:(int13_old_eax - int13_handler + 1), %ah
				/* 0x02=read, 0x03=write */
	orb	$0x40, %ah	/* 0x42=EXT_read, 0x43=EXT_write */

	call	real_int13_service

	jc	3f			/* failure */
	
2:	
	pushaw
	movw	%cs:(next_fragment_len - int13_handler), %cx
	orw	%cx, %cx
	je	2f

	movw	%cs:(next_fragment_base - int13_handler), %bp
	movw	$0, %cs:(next_fragment_len - int13_handler)	
	movw	%cs:(next_fragment_offset - int13_handler), %di
	movl	%cs:4(%bp,%di), %eax
	movl	%eax, 8(%si)
	movl	%cs:8(%bp,%di), %eax
	movl	%eax, 12(%si)
	movw	4(%si), %ax
	movw	2(%si), %bx
	shlw	$9, %bx
	addw	%bx, %ax
	movw	%ax, 4(%si)
	movw	%cs:12(%bp,%di), %ax
	cmpw	%ax, %cx
	jbe	4f

	subw	%ax, %cx
	movw	%cx, %cs:(next_fragment_len - int13_handler)
	addw	$16, %di
	movw	%di, %cs:(next_fragment_offset - int13_handler)
	xchgw	%ax, %cx
4:
	movw	%cx, 2(%si)
	popaw
	call	real_int13_service 
	jc	3f			/* failure */
	jmp	2b
	
2:	
	popaw
	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jnz	4f			/* CDROM */
	
	movw	$(EBIOS_disk_address_packet - int13_handler), %si
	movw	%cs, %ax
	movw	%ax, %ds
	
	/* restore original start-sector-number(in the DAP) */
//	movl	%cs:8(%bp), %eax	/* StartLBA_Lo */
//	subl	%eax, 8(%si)
//	movl	%cs:12(%bp), %eax	/* StartLBA_Hi */
//	sbbl	%eax, 12(%si)

	movw	%cs:(old_form_len - int13_handler), %ax
	movw	%ax, 2(%si)
	movw	%cs:(old_form_offset - int13_handler), %ax
	movw	%ax, 4(%si)
	movl	%cs:(old_form_statr_lo - int13_handler), %eax
	movl	%eax, 8(%si)
	movl	%cs:(old_form_statr_hi - int13_handler), %eax
	movl	%eax, 12(%si)

	call	modify_boot_sectors
4:
	movb	%cs:(int13_old_eax - int13_handler), %al
	xorb	%ah, %ah
	jmp	4f			/* success */
	
3:
	movw	$0x400, %ax	/* no sectors transferred */
	/* write back sectors transferred to original DAP for LBA access */
	cmpb	$0x40, %cs:(int13_old_eax - int13_handler + 1)
	jb	4f		/* CF=1, function 02 or 03, not an LBA call */

	movw	%cs:(int13_old_esi - int13_handler), %si
	movw	%cs:(int13_old_ds - int13_handler), %ds

	/* DS:SI -> original DAP */
	movb	%al, 2(%si)
	stc				/* failure */
4:
	movl	%cs:(int13_old_edx - int13_handler), %edx
	movl	%cs:(int13_old_ebx - int13_handler), %ebx
	movl	%cs:(int13_old_ecx - int13_handler), %ecx
	movl	%cs:(int13_old_edi - int13_handler), %edi
	movw	%cs:(int13_old_es - int13_handler), %es
	jmp	int13_return
	
old_form_statr_lo:
	.long	0
old_form_statr_hi:
	.long	0
old_form_len:	
	.word	0
old_form_offset:
	.word	0
next_fragment_base:
	.word	0
next_fragment_offset:
	.word	0	
next_fragment_len:	
	.word	0
	
//input: ebx=A_hi, eax=A_lo, edx=B_hi, ecx=B_lo
compare_64:	
	cmpl	%edx, %ebx		//A_hi <> B_hi
	jne	compare_64_end	//A_hi != B_hi,  end
	//A_hi = B_hi	
	cmpl	%ecx, %eax		//A_lo <> B_lo
compare_64_end:
	ret
/****************************************************************************/
1:
	cmpb	$0x41, %ah	/* EBIOS installation check */
	jnz	1f
	cmpw	$0x55aa, %bx
	jnz	error_01_invalid
//	testb	%dl, %dl
//	jns	error_01_invalid
	movw	$0xaa55, %bx
	movb	$0x21, %ah	/* major version 2.1(EDD-1.1) */
	movw	$0x01, %cx	/* support functions 42h,43h,44h,47h,48h */
	clc
	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x42, %ah	/* EBIOS read sectors */
	jz	2f
	cmpb	$0x43, %ah	/* EBIOS write sectors */
	jnz	1f

//	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
//	jz	2f
//
//	/* CDROM */
//	movb	$0x03, %ah	/* write protection */
//	stc			/* error */
//	jmp	int13_return
2:
	/* get old SI, disk address packet */
	movw	%cs:(int13_old_esi - int13_handler), %si
	movl	(%si), %eax	/* packet length, sectors, etc. */
	testb	%ah, %ah
	jnz	error_01_invalid
	testb	$0xf0, %al
	jz	error_01_invalid
	shrl	$16, %eax
	testw	$0xff80, %ax
	jnz	error_01_invalid
	testb	%al, %al
	jz	error_01_invalid

	/* copy disk address packet to EBIOS_disk_address_packet */

	pushw	%es
	pushw	%cx
	pushw	%di

	movw	%cs, %ax
	movw	%ax, %es	/* ES=CS */

	movw	$(EBIOS_disk_address_packet - int13_handler), %di
	movw	$8, %cx		/* will copy only 16 bytes! */
	cld
	repz movsw

	popw	%di
	popw	%cx
	popw	%es

	/* set DS:SI */

	movw	%cs, %ax
	movw	%ax, %ds	/* DS=CS */
	movw	$(EBIOS_disk_address_packet - int13_handler), %si

	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jz	2f
	shlb	$2, 2(%si)
	movl	8(%si), %eax
	shldl	$2, %eax, 12(%si)
	shll	$2, 8(%si)
2:
	//movw	4(%si), %bx
	//movw	6(%si), %es
	lesw	4(%si), %bx
	jmp	disk_address_packet_ready

/****************************************************************************/
1:
	cmpb	$0x44, %ah	/* EBIOS verify sectors */
	jnz	1f
	xorb	%ah, %ah
	clc
	jmp	int13_return
/****************************************************************************/
//	# this check is not too useful, so comment out to save space
//1:
//	cmpb	$0x47, %ah	/* EBIOS seek */
//	jnz	1f
//	xorb	%ah, %ah
//	clc
//	jmp	int13_return
/****************************************************************************/
1:
	cmpb	$0x48, %ah	/* EBIOS GET DRIVE PARAMETERS */
	jnz	1f

	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jz	2f			/* normal disks */

	/* CDROM */
	/* get old SI, extended drive parameter table */
	movw	%cs:(int13_old_esi - int13_handler), %si
	cmpw	$26, (%si)
	jb	error_01_invalid

	movw	$26, (%si)	/* buffer length */
	movw	$0x00, 2(%si)		# no flags
	movw	$0x800, 24(%si)		# bytes per sect=2048
	xorl	%eax, %eax
	decw	%ax
	movl	%eax, 4(%si)		# cylinders=0xFFFF
	movb	$0, %ah
	movl	%eax, 8(%si)		# heads=0xFF
	movb	$15, %al
	movl	%eax, 12(%si)		# sectors per track=15
	xorw	%ax, %ax		# CF cleared
	movl	%eax, 20(%si)		# total sectors hi dword=0
//	xorw	%ax, %ax		# CF cleared
	decl	%eax			# EAX=0xFFFFFFFF
	movl	%eax, 16(%si)		# total sectors lo dword
					# CF is cleared

	xorw	%ax, %ax	/* success, CF cleared */
	jmp	int13_return
2:
	/* get old SI, extended drive parameter table */
	movw	%cs:(int13_old_esi - int13_handler), %si
	movw	$26, (%si)	/* buffer length */
	movw	$2, 2(%si)	/* info */
	xorl	%eax, %eax
	movl	%eax, 8(%si)	/* total heads */
	movl	%eax, 12(%si)	/* sectors per track */
	movl	%eax, 16(%si)	/* total sectors */
	movl	%eax, 20(%si)	/* total sectors hi */
	pushl	%ebx
	xorl	%ebx, %ebx
	movw	%cs:2(%bp), %ax	/* AL=Hmax, AH=Smax */
	andb	$63, %ah
	movb	%ah, 12(%si)	/* sectors per track */
	movb	%ah, %bl
	xorb	%ah, %ah
	incw	%ax		/* total heads=Hmax+1 */
	movw	%ax, 8(%si)	/* total heads */
	pushl	%edx
	mulw	%bx		/* DX:AX=product, DX=0 */
	movw	%ax, %bx	/* BX=sectors per cylinder */
	movl	%cs:20(%bp), %edx	/* S_count_Hi */
	movl	%edx, 20(%si)	/* total sectors hi */
	movl	%cs:16(%bp), %eax	/* S_count_Lo */
	movl	%eax, 16(%si)	/* total sectors lo */
	/*xorl	%edx, %edx*/	/* EDX:EAX=64bit total sectors */
	testw	%bx, %bx
	jz	2f
	divl	%ebx		/* EAX=quotient, EDX=residue */
2:
	testl	%edx, %edx
	popl	%edx
	popl	%ebx
	jz	2f
	incl	%eax
2:
	movl	%eax, 4(%si)	/* total cylinders */
	movw	$512, 24(%si)	/* bytes per sector */
	xorb	%ah, %ah
	/*clc*/			/* signal success, CF already cleared by XOR */
	jmp	int13_return

/****************************************************************************/
1:
	cmpw	$0x4B01, %ax	/* CDROM GET DISK EMULATION STATUS */
	jnz	1f

	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jz	error_01_invalid	/* normal disks have no 0x4B01 function */

	/* CDROM */
	pushw	%es
	pushw	%di
	movw	%cs:(int13_old_esi - int13_handler), %di	/* old SI */
	pushw	%ds
	popw	%es
	movw	$0x0013, %ax	/* packet size=13h, boot type=0 (no-emu) */
	cld
	stosw
	movb	%dl, %al	/* drive=DL, controller=0 */
	stosw

	pushl	%cs:EXT_C(lba_cd_boot) - int13_handler
	popw	%ax
	stosw
	popw	%ax
	stosw			/* LBA for no-emu image */

	xorw	%ax, %ax
	stosw			/* device specification */
	stosw			/* user buffer segment */
	stosw			/* load segment */
	movb	$4, %al
	stosw			/* sector count=4 */
				/* CHS makes no sense for no-emu */
	popw	%di
	popw	%es
	xorb	%ah, %ah	/* success, CF cleared */
	jmp	int13_return

/****************************************************************************/
1:
/****************************************************************************/
error_01_invalid:
	movb	$0x01, %ah	/* unsupported function call */
	stc			/* signal error */

int13_return:
	movw	%ax, %cs:(int13_old_eax - int13_handler)	/* status */
	jnc	1f
	orb	$1, %cs:(int13_old_flags - int13_handler)
	jmp	2f
1:
	andb	$0xFE, %cs:(int13_old_flags - int13_handler)
2:
	pushw	%cs:(int13_old_flags - int13_handler)
	popfw

	movl	%cs:(int13_old_eax - int13_handler), %eax
	movl	%cs:(int13_old_esi - int13_handler), %esi
	movw	%cs:(int13_old_ds - int13_handler), %ds
	movl	%cs:(int13_old_ebp - int13_handler), %ebp
	movw	%cs:(int13_old_esp - int13_handler), %sp
	ljmp	*%cs:(int13_old_cs_ip - int13_handler)

/****************************************************************************/
real_int13_service:

	/* AH = 0x42 or 0x43 */

	/* save return address to memory */
	popw	%cs:(int13_ret_IP - int13_handler)
	/* save AX to memory */
	movw	%ax, %cs:(int13_reg_AX - int13_handler)

	cmpb	$0xff, %dl	/* mem drive */
	jne	normal_disk_drive

	testw	$0x4000, %cs:4(%bp)	/* TO_C bit 14=(TO is cdrom) */
	jnz	normal_disk_drive

	/* handle memdrive */

	sti		/* for hardware interrupt or watchdog */
	cmpb	$0x42, %ah
	je	3f
	cmpb	$0x43, %ah
	je	3f
	stc
	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret
	/* DS:SI points to disk address packet 	*/
	/* 	2(%si), byte, sectors(1-127) 	*/
	/* 	4(%si), word, offset 		*/
	/* 	6(%si), word, segment 		*/
	/* 	8(%si), qword, lba		*/
3:
	/* AH = 0x42 or 0x43 */
	movzwl	4(%si), %ebx	#;  BX=offset, EBX_high_word=0
	movzwl	6(%si), %edi	#;  DI=segment
	shll	$4, %edi	#; EDI=linear base address of segment
	addl	%ebx, %edi	#; EDI=linear address of BUFFER(below 1M)

	movl	12(%si), %ebx	#; EBX=LBA of MEMDRIVE SECTOR hi
	movl	8(%si), %ecx	#; ECX=LBA of MEMDRIVE SECTOR lo
	shldl	$9, %ecx, %ebx
	shll	$9, %ecx	#; EBX:ECX=linear address of SECTOR(above 1M)
	//xorl	%edx, %edx
	movl	%edi, %cs:(data_addr_below_1M - int13_handler)
	//movl	%edx, %cs:(data_addr_below_1M - int13_handler + 4)
	movl	%ecx, %cs:(data_addr_above_1M - int13_handler)
	movl	%ebx, %cs:(data_addr_above_1M - int13_handler + 4)
	testl	%ebx, %ebx
	setnz	%dl
	movzbl	2(%si), %ebx	# sectors to read/write
	shll	$9, %ebx	# bytes to read/write
	decl	%ebx
	addl	%ecx, %ebx
	setc	%dh
	orb	%dh, %dl
	movb	%dl, %cs:(int13_need_64_bit - int13_handler)

	/* if RAM disk is at above 4G, the int15/ah=87h call will fail. */

	# int15/ah=87h is buggy on many BIOSes. You should use the default RAW.
	cmpl	$0, %cs:(EXT_C(memdisk_raw) - int13_handler)
	je	int15_87	/* do not use raw mode, but use BIOS. */

	# win9x vm86-mode int15/ah=87h will also fail in accessing more than 4G
	# memory. Hope someone would develop a driver for win9x in the future.
	smsw	%bx
	testb	$1, %bl
	jnz	int15_87	/* vm86/protected mode also need int15_87 */

	/* raw mode as in memdisk, contributed by Bean */

	/* switch to protected mode myself */
	/* ebx destroy */

	/* EDI=linear address of BUFFER(below 1M) */
	/* ECX=linear address of SECTOR(above 1M), lower 32 bits */
	movzbl	2(%si), %ebx	# sectors to read/write
	shll	$9, %ebx	# bytes to read/write
	movl	%ebx, %edx	# bytes to read/write
	addl	%edi, %ebx	# point to the byte after the buffer
	decl	%ebx		# point to the last byte of the source
	orl	%edi, %ebx	# to see if the buffer is inside an even mega
	addl	%ecx, %edx	# point to the byte after the sectors
	decl	%edx		# point to the last byte of the sectors
	orl	%ecx, %edx	# to see if the sectors is inside an even mega
	orl	%ebx, %edx	# to see if both are in even megas
	andl	$0x100000, %edx	# zero means both are in even megas, DX=0
	setz	%dl		# we think of it as if A20 were on
	jz	3f		# no need to enable A20. DX=1.

	/* DX=0 */
	movw	$0x00ff, %cx	# try so many times on failure
	incw	%dx		# DL=1(enable A20), DH=0(debug off)

	cli	/* yes, keep interrupt off when controlling A20 */
	call	enable_disable_a20	# EAX, CX modified
	sti

	setc	%dl		# CF=1 means A20 was originally enabled.
	jz	3f		/* enabled A20 with success */

	/* A20 control failed. Notify the user. */
	movw	$(a20_failure - int13_handler), %si
	xorw	%bx, %bx
5:	
	lodsb
	cmpb	$0x20, %al
	jb	5f
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	jmp	5b
5:	
	stc
	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret

a20_failure:
		.ascii	"\r\ngrub4dos: A20 failure.\r\n\0"

3:
	/* EDI=linear address of BUFFER(below 1M) */
	xorl	%ebx, %ebx
	movw	$(EBIOS_disk_address_packet - int13_handler), %si
	movw	%cs, %bx
	movw	%bx, %ds

	// GDT is initialized by set_int13_handler
	//shll	$4, %ebx
	//addl	$(MyGDT - int13_handler), %ebx
	//movl	%ebx, %cs:MyGDT - int13_handler + 4

	movzbl	2(%si), %ecx	#; ECX=number of sectors to transfer
	shll	$7, %ecx	/* 128 dwords per sector */
	cld

	sgdtl	%cs:(OldGDTdesc - int13_handler)
	lgdt	%cs:(gdtdesc - int13_handler)
	testb	$0xFF, %cs:(int13_need_64_bit - int13_handler)	# need 64 bit?
	jnz	move_block_using_64_bit			# yes
	//jmp	move_block_using_64_bit		/* only test 64-bit code */

	/*********************************/
	/* normal 32-bit(4GB) block move */
	/*********************************/

	/* EDI=linear address of BUFFER(below 1M) */

	//movl	%cs:(data_addr_below_1M - int13_handler), %edi
	movl	%cs:(data_addr_above_1M - int13_handler), %esi
	testb	$1, %cs:(int13_reg_AX - int13_handler + 1)	# AH
	jz	3f						# read
	xchgl	%esi, %edi
3:
				#; ESI changed!!
	movl	%cr0, %eax
	//andl	$0x7FFFFFFF, %eax
	orb	$1, %al		// set CR0.PE(bit0)

	cli
	movl	%eax, %cr0	/* Switch to protected mode */

	movw	$(PM_DS32), %bx	/* Switch to 4G data segment */
	movw	%bx, %ds
	movw	%bx, %es

	addr32	rep movsl	/* ESI, EDI changed! */

	/* Don't switch back to 64K limit for DS and ES. Keeping 4G limit
	 * would hurt nothing(in principle). There are softwares which run
	 * initially with DS and ES able to access 4G memory. They call
	 * int13 and expect on return the DS and ES could still access 4G
	 * memory. So we should not forcibly set the limit to a lower value
	 * of 64K. -- tinybit
	 */
#if 0
	movw	$(PM_DS16), %bx	// 64KB data segment limit
	movw	%bx,  %ds
	movw	%bx,  %es
#endif

	//movl	%cr0, %eax	/* EAX not touched */
	andb	$0xFE, %al	// reset CR0.PE(bit0)
	movl	%eax, %cr0	// back to real mode

	/* no problem if we skip this and save some bytes of code. */
#if 0
	movw	%cs,  %bx
	movw	%bx,  %ds
	movw	%bx,  %es
#endif

move_block_finished:
	lgdtl	%cs:(OldGDTdesc - int13_handler)

	sti

	cmpl	$0, (EXT_C(a20_keep_on) - int13_handler)
	jne	1f	/* Keep A20 on. This should hurt nothing. */

	/* Disable A20 if necessary ! */
	testw	%dx, %dx	/* 0=orig A20 off, 1=orig A20 on */
	jnz	1f		/* should not turn off A20 */

	movw	$0x0004, %cx	# try so many times on failure
	//xorw	%dx, %dx	# DL=0(disable A20), DH=0(debug off)
	cli	/* yes, keep interrupt off when controlling A20 */
	call	enable_disable_a20
	sti

1:
	clc
	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret

move_block_using_64_bit:

#if 1	/* Yes, you may comment out the 64-bit code on a 32-bit platform. */

	/* EDI=linear address of BUFFER(below 1M) */

	/* DS=CS */

	//movw	%cs, /*%cs:*/(CS_seg - int13_handler)
	//movw	%es, /*%cs:*/(ES_seg - int13_handler)
	//movw	%fs, /*%cs:*/(FS_seg - int13_handler)
	//movw	%gs, /*%cs:*/(GS_seg - int13_handler)
	//movw	%ss, /*%cs:*/(SS_seg - int13_handler)
	//movw	%sp, /*%cs:*/(SP_off - int13_handler)
	//movw	%di, /*%cs:*/(DI_reg - int13_handler)

	////sidtl	%cs:OldIDT - int13_handler
	movb	%dl, /*%cs:*/(int13_prev_DL - int13_handler)	/* save DL */
	movl	%cr3, %eax
	movl	%eax, /*%cs:*/(int13_prev_CR3 - int13_handler)	/* save CR3 */
	movl	%cr4, %eax
	movl	%eax, /*%cs:*/(int13_prev_CR4 - int13_handler)	/* save CR4 */

	/* EDI=linear address of BUFFER(below 1M) */

	/* if BUF_ADDR >= 16K, we build page tables at 4K, 8K and 12K; */
	/* otherwise, we build page tables at 84K, 88K and 92K. */
	/* in either case, the page tables will not overlap the buffer, */
	/* because the buffer length is <= 64K as for each int13 call. */
	xorl	%ebx, %ebx
	movb	$0x01, %bh	/* BX=0x0100 */
	cmpl	$0x4000, %edi
	jnb	3f
	movb	$0x15, %bh	/* BX=0x1500 */
3:
	//movw	%bx, /*%cs:*/(DS_seg - int13_handler)	/* for page tables */
	movw	%bx, %ds	/* DS:0000 = PML4 base */
	/* DS changed!! DS != CS */

	shll	$4, %ebx	/* EBX=0x1000 or 0x15000 */

	movw	%cs, %ax
	movw	%ax, %es
	/* ES changed!! ES=CS */

	movw	$(int13_prev_PML4T_entry - int13_handler), %di
				// DI = offset to backup variables
	/* DI changed!! */
	xorw	%si, %si

	//cld
	cli
	/* backup PML4 table and build it */
	/* lo of entry 0 */
	movsl
	movl	%ebx, %eax
	addw	$0x1007, %ax	/* EAX=EAX+0x1007, CF=0 */
	movl	%eax, -4(%si)	//movl	%eax, 0

	/* hi of entry 0 */
	movsl
	movl	$0, -4(%si)	//movl	$0, 4

	/* backup PDP table and build it */
	/* lo of entry 0 */
	movw	$0x1000, %si
	addw	%si, %ax	//CF=0, so the same as addl $0x1000, %eax
				//EAX = (addr of PD | 7)
	movsl
	movl	%eax, -4(%si)	//movl	%eax, 0x1000

	/* hi of entry 0 */
	movsl
	movl	$0, -4(%si)	//movl	$0, 0x1004

	/* backup PD table and build it */
	/* entry 0 is identity mapping, 2M page size */
	/* lo of entry 0 */
	movw	$0x2000, %si
	movsl
	movl	$0x00000087, -4(%si)	//movl	$0x00000087, 0x2000
	/* hi of entry 0 */
	movsl
	movl	$0, -4(%si)	//movl	$0, 0x2004

	/* load linear address of the disk block into EDX:EAX */
	movl	%cs:(data_addr_above_1M - int13_handler), %eax
	movl	%cs:(data_addr_above_1M - int13_handler + 4), %edx

	andl	$0xFFE00000, %eax	# align to 2M boundary
	orb	$0x87, %al
	/* entry 1 is mapping to the first half 2M page of the disk block. */
	/* lo of entry 1 */
	movsl
	movl	%eax, -4(%si)	//movl	%edx, 0x2008
	/* hi of entry 1 */
	movsl
	movl	%edx, -4(%si)	//movl	%ebx, 0x200C

	addl	$0x00200000, %eax
	adcl	$0, %edx
	/* entry 2 is mapping to the last half 2M page of the disk block. */
	/* lo of entry 2 */
	movsl
	movl	%eax, -4(%si)	//movl	%edx, 0x2010
	/* hi of entry 2 */
	movsl
	movl	%edx, -4(%si)	//movl	%ebx, 0x2014

	/* virtual linear address of disk block */
	movl	%cs:(data_addr_above_1M - int13_handler), %esi
	andl	$0x001FFFFF, %esi
	addl	$0x00200000, %esi
	movl	%cs:(data_addr_below_1M - int13_handler), %edi

	testb	$1, %cs:(int13_reg_AX - int13_handler + 1)	# AH
	jz	3f						# read
	xchgl	%esi, %edi
3:

	/* prepare to enter paging or 64-bit mode */
	movl	%cr4, %eax
	orb	$0x30, %al		# 0x80=PGE, 0x20=PAE, 0x10=PSE
	movl	%eax, %cr4

	/* EBX = address of PML4T, EBX+0x1000 = address of PDPT */

	testb	$2, %cs:(int13_is64bit - int13_handler) 
	jz	int13_paemove	// use PAE if AMD64 is not supported
	//jmp	int13_paemove	// only test PAE code

	.code16
//int13_lm64move:
	movl	%ebx, %cr3	// CR3 = address of PML4T

	movl	%ecx, %ebx		# save ECX into EBX
	/* rdmsr will change EDX:EAX */
	movl	$0xC0000080, %ecx	# specify EFER MSR
	rdmsr				# enable long mode(EFER.LME=1)
	orb	$0x1, %ah
	wrmsr
	movl	%ebx, %ecx		# restore ECX from EBX

	xorl	%ebx, %ebx
	movw	%cs, %bx
	shll	$4, %ebx	// EBX=linear address of int13_handler

	movl	%cr0, %eax		# Activate long mode by enabling
	orl	$0x80000001, %eax	# paging and protection simultaneously
	movl	%eax, %cr0		# skipping protected mode entirely

	.byte	0x66, 0xEA		# 32-bit long jmp
int13_lm64move_lm64_start_addr:
	/* linear address of start_64_bit, adjusted by set_int13_handler */
	.long	(int13_lm64move_lm64_start - int13_handler)
	.word	LM_CS64		# 64-bit code 4GB limit

	.code64
int13_lm64move_lm64_start:
	/* DS, ES is not used in 64-bit */

	shrl	$1, %ecx	/* we will move 2 dwords at a time */

	/* only use 32-bit virtual adresses ESI, EDI */
	addr32	rep movsq	/* ESI, EDI changed! */

	/* switch to 16-bit compatibility mode then back to real mode */
	ljmp	*(int13_lm64move_cm16_addr - int13_handler)(%ebx)
				// EBX=linear address of int13_handler
int13_lm64move_cm16_addr:
	.long	(int13_lm64move_cm16 - int13_handler)
	.word	PM_CS16		# 16-bit code 64K limit

	.code16
int13_paemove:
	// DS:0x1000=PDPTE. The lowest byte of PDPTE was set to 7 by code above.  For PAE paging, only bit 0 must be 1, bit1-2 must be 0.
	movb	$1, 0x1000
	addl	$0x1000, %ebx	// EBX = linear address of PDPT
	movl	%ebx, %cr3	// CR3 = linear address of PDPT

	movl	%cr0, %eax
	orl	$0x80000001, %eax
	movl	%eax, %cr0	// protected mode with PAE paging
	ljmp	$(PM_CS16), $(int13_paemove_pm_start - int13_handler)
int13_paemove_pm_start:
	movw	$PM_DS32, %bx	/* Switch to 4G data segment */
	movw	%bx, %ds
	movw	%bx, %es

	/* use 32-bit virtual adresses ESI, EDI */
	addr32	rep movsl	/* ESI, EDI changed! */

	movl	%cr0, %eax
	andl	$0x7FFFFFFF, %eax	# disable paging
	movl	%eax, %cr0

	subl	$0x1000, %ebx	// EBX=linear address of PML4E
	jmp	int13_pgmove_exit_pm

	.code16
int13_lm64move_cm16:
	/* now in 16-bit compatability mode */
	movl	%cr0, %eax
	andl	$0x7FFFFFFF, %eax	# disable paging, leaving long mode
	movl	%eax, %cr0
	/* now in 16-bit flat protected mode */
	/* rdmsr will change EDX:EAX */
	movl	$0xC0000080, %ecx	# specify EFER MSR
	rdmsr
	andb	$0xFE, %ah		# disable long mode(EFER.LME=0)
	wrmsr
	movl	%cr3, %ebx	// EBX=linear address of PML4E
//	jmp	int13_pgmove_exit_pm

int13_pgmove_exit_pm:	// shared by PAE and long mode block move
	movw	$(PM_DS16), %ax // int13_handler data segment
	movw	%ax, %ds	// DS limit=64KB base=int13_handler
	//movw	%ax, %es	// ES limit=64KB base=int13_handler

	movl	(int13_prev_CR4 - int13_handler), %eax
	movl	%eax, %cr4
	movl	(int13_prev_CR3 - int13_handler), %eax
	movl	%eax, %cr3

	/* Switching to 4GB limit for DS and ES would hurt nothing(in
	 * principle). There are softwares which run initially with DS and
	 * ES able to access 4G memory. They call int13 and expect on return
	 * the DS and ES could still access 4G memory. So we should not
	 * forcibly set the limit to a lower value of 64K. -- tinybit
	 */
	movw	$(PM_DS32), %ax // 4G data segment
	movw	%ax, %ds	// DS limit=4GB base=0
	movw	%ax, %es	// ES limit=4GB base=0

	movl	%cr0, %eax
	andb	$0xFE, %al	// reset CR0.PE(bit0)
	movl	%eax, %cr0	// back to real mode
	ljmp	$0, $(int13_pgmove_to_rm - int13_handler)
int13_pgmove_to_rm_cs  = . - 2	// segment will be set by set_int13_handler
int13_pgmove_to_rm:
	movw	%cs, %ax	
	movw	%ax, %ds	// DS=int13_handler segment
	shrl	$4, %ebx
	movw	%bx, %es	// ES=PML4T, PDPT, PD segment

	/* restore page tables */
	movw	$(int13_prev_PML4T_entry - int13_handler), %si
	xorw	%di, %di
	//cld
	movsl
	movsl
	movw	$0x1000, %di
	movsl
	movsl
	movw	$0x2000, %di
	movsl
	movsl
	movsl
	movsl
	movsl
	movsl

	// no need to restore ES
	//movw	/*%cs:*/(ES_seg - int13_handler), %es

	/* restore DL and clear DH */
	movzbw	(int13_prev_DL - int13_handler), %dx

#endif	/* end of 64-bit code */
	jmp	move_block_finished

int15_87:
	/* EDI=linear address of BUFFER(below 1M) */
	/* ECX=linear address of SECTOR(above 1M) */

	negb	%dl		/* need_64_bit? */
	jc	6f		/* Yes. Cannot handle. Failure. */
	xchgl	%eax, %edi	/* movl %edi, %eax */
	testb	$1, %cs:(int13_reg_AX - int13_handler + 1)	# AH
	jnz	3f						# write
	xchgl	%eax, %ecx					# read
3:
	/* Data transfer: (%eax) -> (%ecx), number of sectors 2(%si) */

	movb	2(%si), %bl	/* number of sectors to be moved */
	pushw	%cs
	popw	%es
	movw	$(GDT_data - int13_handler), %si /* SI changed!! */
	movw	%ax, %es:0x12(%si)	#; source physical address low 16 bits
	shrl	$16, %eax
	movb	%al, %es:0x14(%si)	#; source physical address bit 16-23
	movb	%ah, %es:0x17(%si)	#; source physical address bit 24-32
	movw	%cx, %es:0x1a(%si)	#; dest physical address low 16 bits
	shrl	$16, %ecx
	movb	%cl, %es:0x1c(%si)	#; dest physical address bit 16-23
	movb	%ch, %es:0x1f(%si)	#; dest physical address bit 24-32

	xorw	%cx, %cx	/* ECX is 0 */
	movl	%ecx, %es:(%si)
	movl	%ecx, %es:0x04(%si)
	movl	%ecx, %es:0x08(%si)
	movl	%ecx, %es:0x0c(%si)
	movl	%ecx, %es:0x20(%si)
	movl	%ecx, %es:0x24(%si)
	movl	%ecx, %es:0x28(%si)
	movl	%ecx, %es:0x2c(%si)
	movb	%bl, %ch	/* CX=number of words to be moved */
	movb	$0x87, %ah	/* access extended memory */

	pushw	%ax
	smsw	%ax
	testb	$1, %al
	popw	%ax
	jnz	4f		/* protected mode */

	/* real mode */
	pushfw
	lcall	%cs:*(EXT_C(ROM_int15) - int13_handler)

	jmp	6f
4:
	/* protected mode */
	int	$0x15
6:
	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret

/****************************************************************************/
normal_disk_drive:

	/* handle normal disk drive */

	/* AH = 0x42 or 0x43 */
	pushw	%ax
	smsw	%ax
	testb	$1, %al
	popw	%ax
	jz	3f	// real mode

	/* protected mode(Windows 98, EMM386) */

	pushw	%si		/* save SI */
	pushl	%eax		/* save EAX */
	pushl	%ebx		/* save EBX */

	/* set SI to the drive map */
#if	MAP_NUM_16
	movw	$(EXT_C(hooked_drive_map_1) - int13_handler), %si
#else
	movw	$(EXT_C(hooked_drive_map) - int13_handler), %si
#endif
	/* find the drive number from the drive map */
	cld
	subw	$DRIVE_MAP_SLOT_SIZE - 4, %si
2:
	addw	$DRIVE_MAP_SLOT_SIZE - 4, %si
	
#if	MAP_NUM_16
	cmpw	$EXT_C(hooked_drive_map_1) - int13_handler + DRIVE_MAP_SIZE * DRIVE_MAP_SLOT_SIZE / 2, %si
	jne	4f
	movw	$(EXT_C(hooked_drive_map_2) - int13_handler), %si
4:	
	cmpw	$EXT_C(hooked_drive_map_2) - int13_handler + DRIVE_MAP_SIZE * DRIVE_MAP_SLOT_SIZE / 2, %si
	je	2f
#endif

	lodsl	%cs:(%si), %eax
	testl	%eax, %eax	/* end of map table? */
	movl	%eax, %ebx	/* save the map to EBX */
	jz	2f		/* yes, no map found */
	cmpb	%dl, %ah	/* found the map? */
	jne	2b		/* no, check the next slot */

	/* drive is mapped. check if map a whole drive */
	shrl	$16, %eax
	testb	$62, %ah
	jnz	2b			/* no, check the next slot */
	movl	%cs:8(%si), %eax	/* StartLBA_Hi */
	testl	%eax, %eax
	jnz	2b			/* no, check the next slot */
	movl	%cs:4(%si), %eax	/* StartLBA_Lo */
	testl	%eax, %eax
	jnz	2b			/* no, check the next slot */
	movl	%cs:16(%si), %eax	/* S_count_Hi */
	testl	%eax, %eax
	jnz	2b			/* no, check the next slot */
	movl	%cs:12(%si), %eax	/* S_count_Lo */
	shrl	$1, %eax
	jnz	2b		/* no, check the next slot */
2:
	testl	%ebx, %ebx	/* mapped or not mapped ? */
	jz	2f		/* not mapped, do nothing */
	movb	%bl, %dl	/* use the mapped FROM_DRIVE for win 98 */
2:
	movb	%cs:(%bp), %al	/* AL=FROM_DRIVE */
	testb	%al, %al	/* hard drive emulation? */
	jns	2f		/* floppy, jump */
	cmpb	%al, %dl
	jb	2f
	incb	%dl
2:
	popl	%ebx
	popl	%eax
	popw	%si

	/* fall through to real mode... */

3:

	/* in real mode DOS, call original real mode int13 */
	/* DS:SI points to disk address packet 	*/
	/* 	2(%si), byte, sectors(1-127) 	*/
	/* 	4(%si), word, offset 		*/
	/* 	6(%si), word, segment 		*/
	/* 	8(%si), qword, lba		*/

	testw	$0x4000, %cs:4(%bp)	/* TO_C bit 14=(TO is cdrom) */
	jz	1f

	/* LBA supported, CHS not applicable for CDROM */

	movw	%es, %cs:(int13_cdrom_es - int13_handler)
	movw	%cx, %cs:(int13_cdrom_cx - int13_handler)
	movl	%ebx, %cs:(int13_cdrom_ebx - int13_handler)
	movl	%edi, %cs:(int13_cdrom_edi - int13_handler)

	movzbw	2(%si), %cx	/* sectors to transfer */
	movl	8(%si), %edi	/* start sector number(LBA) */
	movl	12(%si), %ebx
3:
	/* read 1 small sector at LBA=EDI to edd30_disk_buffer */

	/* if we have previously read it, we can skip this step now. */
	cmpb	%dl, %cs:last_read_cd_drive - int13_handler
	jne	2f
	cmpl	%ebx, %cs:last_read_sector - int13_handler + 4
	jne	2f
	movl	%edi, %eax
	andb	$0xFC, %al
	cmpl	%eax, %cs:last_read_sector - int13_handler	#; // + 4
	je	4f
2:
	movl	(%si), %eax
	movl	%eax, %cs:(tmp_dap - int13_handler)
	movl	4(%si), %eax
	movl	%eax, %cs:(tmp_dap - int13_handler + 4)
	movl	8(%si), %eax
	movl	%eax, %cs:(tmp_dap - int13_handler + 8)
	movl	12(%si), %eax
	movl	%eax, %cs:(tmp_dap - int13_handler + 12)

	movl	%edi, 8(%si)
	movl	%ebx, 12(%si)
	shrdl	$2, %ebx, 8(%si)	/* turn LBA small to LBA big */
	shrl	$2, 12(%si)

	movb	$1, 2(%si)		/* read 1 big sector */

	//pushw	%es
	movw	%bx, %cs:(int13_cdrom_bx - int13_handler)
	movw	%cs, %ax
	movw	%ax, %es
	movw	%ax, 6(%si)		/* set the buffer segment */
	movw	$edd30_disk_buffer - int13_handler, %bx
	movw	%bx, 4(%si)		/* set the buffer offset */
	movw	%cs:(int13_reg_AX - int13_handler), %ax
#ifdef CDROM_INIT
	/* check if DL is for anyone of (cd?)'s */
	cmpb	$0, %cs:(EXT_C(atapi_dev_count) - int13_handler)
	jz	2f			/* no cdX'es */
	cmpb	%cs:(EXT_C(min_cdrom_id) - int13_handler), %dl
	jb	2f			/* not cdX */
	cmpb	%cs:(max_cdrom_id - int13_handler), %dl
	ja	2f			/* not cdX */
	/* read 1 cdrom sector using our builtin cdrom driver. */
	movb	$1, %cs:(force_int13 - int13_handler)
2:
#endif
	call	int13_with_retry	/* read a big 2048-byte sector */
#ifdef CDROM_INIT
	movb	$0, %cs:(force_int13 - int13_handler)
#endif
	movw	%cs:(int13_cdrom_bx - int13_handler), %bx
	//popw	%es
	jc	3f			/* failed */

	movl	%cs:(tmp_dap - int13_handler), %eax
	movl	%eax, (%si)
	movl	%cs:(tmp_dap - int13_handler + 4), %eax
	movl	%eax, 4(%si)
	movl	%cs:(tmp_dap - int13_handler + 8), %eax
	movl	%eax, 8(%si)
	movl	%cs:(tmp_dap - int13_handler + 12), %eax
	movl	%eax, 12(%si)

	pushw	%di
	andw	$0xFFFC, %di
	movl	%edi, %cs:last_read_sector - int13_handler
	popw	%di
	movl	%ebx, %cs:last_read_sector - int13_handler + 4
	movb	%dl, %cs:last_read_cd_drive - int13_handler
4:
	pushw	%cx
	pushw	%bx
	pushw	%di

	/* calculate the buffer ES:BX */
	//movw	%di, %ax
	subw	8(%si), %di	/* sectors already read */
	shlw	$5, %di		/* paragraghs */
	addw	6(%si), %di	/* segment */
	movw	4(%si), %bx	/* offset */
	movw	%bx, %ax
	shrw	$4, %ax		/* turn to paragraghs */
	andw	$0x000F, %bx
	addw	%di, %ax	/* segment */
	movw	%ax, %es

	movw	%bx, %di
	popw	%ax		/* AX=old DI */
	pushw	%ax

	andb	$3, %al
	shlw	$9, %ax		/* offset in edd30_disk_buffer */

	addw	$edd30_disk_buffer - int13_handler, %ax

	xchgw	%ax, %si

	/* move 512 bytes */
	movw	$0x100, %cx
	cld
	cs repz movsw

	xchgw	%ax, %si

	popw	%di
	popw	%bx

	incl	%edi		/* next sector */
	jnz	2f
	incl	%ebx
2:

	popw	%cx
	decw	%cx
	jnz	3b
//	loop	3b

	movw	%cs:(int13_cdrom_es - int13_handler), %es
	movw	%cs:(int13_cdrom_cx - int13_handler), %cx
	movl	%cs:(int13_cdrom_ebx - int13_handler), %ebx
	movl	%cs:(int13_cdrom_edi - int13_handler), %edi
	clc
	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret
3:
	movw	%cs:(int13_cdrom_es - int13_handler), %es
	movw	%cs:(int13_cdrom_cx - int13_handler), %cx
	movl	%cs:(int13_cdrom_ebx - int13_handler), %ebx
	movl	%cs:(int13_cdrom_edi - int13_handler), %edi
	stc
	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret

1:
	#;jmp	1f	/* just check if CHS translation code works */
	testw	$0x8000, %cs:4(%bp)	/* TO_C bit 15=LBA support */
#if 0
	jnz	int13_with_retry	/* LBA mode */
#else
	jz	1f			/* CHS only, so skip the LBA access */
	movw	%es, %cs:(int13_tmp_es_bx - int13_handler + 2)
	movw	%bx, %cs:(int13_tmp_es_bx - int13_handler)
	movw	4(%si), %bx
	shrw	$4, %bx
	addw	%bx, 6(%si)
	andw	$0x000F, 4(%si)
	cmpb	$0x80, 2(%si)
	jb	2f			/* 0x7F sectors or less is ok */
	ja	3f			/* 0x81 sectors or more is bad */

	/* now 0x80 sectors requested. */

	testw	$0x2000, %cs:4(%bp)	/* TO_C bit 13=(FROM is cdrom) */
	jz	3f			/* not CDROM, fail out. */
	/* virtual cdrom function call could request 64K data each time */
	/* transfer 0x80 sectors = twice 0x40 sectors */
	movb	$0x40, 2(%si)		/* transfer the first 32K */
	//movw	4(%si), %bx
	//movw	6(%si), %es
	lesw	4(%si), %bx
	call	int13_with_retry
	jc	4f
	movb	$0x40, 2(%si)
	addw	$0x800, 6(%si)		/* step to next 32K of the buffer */
	//movw	4(%si), %bx
	//movw	6(%si), %es
	lesw	4(%si), %bx
	pushl	%eax
	xorl	%eax, %eax
	movb	$0x40, %al
	addl	%eax, 8(%si)		/* adjust start_LBA */
	movb	$0, %al
	adcl	%eax, 12(%si)
	popl	%eax
	call	int13_with_retry
	/* we ignore this step and keep start_LBA in a modified state. This is
	 * because only modify_boot_sectors need start_LBA and we are accessing
	 * the virtual cdrom, so we will not call modify_boot_sectors.
	 */
	//pushfw
	//pushl	%eax
	//xorl	%eax, %eax
	//movb	$0x40, %al
	//subl	%eax, 8(%si)		/* adjust start_LBA */
	//movb	$0, %al
	//sbbl	%eax, 12(%si)
	//popl	%eax
	//popfw
4:
	movb	$0x80, 2(%si)		/* restore its original value */
	lesw	%cs:(int13_tmp_es_bx - int13_handler), %bx
	movw	%bx, 4(%si)		/* restore its original value */
	movw	%es, 6(%si)		/* restore its original value */
	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret

2:
	//movw	4(%si), %bx
	//movw	6(%si), %es
	lesw	4(%si), %bx
	call	int13_with_retry
	lesw	%cs:(int13_tmp_es_bx - int13_handler), %bx
	movw	%bx, 4(%si)		/* restore its original value */
	movw	%es, 6(%si)		/* restore its original value */
	jnc	4f
	testw	$0x1000, %cs:4(%bp)	/* TO_C bit 12=BIFURCATE */
	jz	1f	/* not bifurcate, use CHS access once more. */
	stc		/* bifurcate, error out. */
4:
	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret

3:
	movb	$1, %ah
	stc
	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret

	.align	4
int13_tmp_es_bx:	.long	0

1:
#endif
	/* CHS mode */

	/* byte at SI+2: number of sectors to access */
	/* 8 bytes at SI+8: StartLBA */

	/* CHS mode cannot access large addresses */
	cmpl	$0, 12(%si)
	jnz	1f

	pushl	%edx
	pushl	%ebx
	movw	%cs:4(%bp), %ax	/* TO_C */
	andw	$0x3FF, %ax	/* get lo 10 bits */
	incw	%ax		/* cylinders */
	pushw	%ax
	movzbw	%cs:6(%bp), %ax	/* TO_H */
	incw	%ax		/* heads */
	movzbw	%cs:7(%bp), %bx	/* TO_S */
	andb	$0x3F, %bl
	mulw	%bx		/* DX=0, AX=heads*sectors */
	popw	%bx		/* cylinders */
	mulw	%bx		/* DX:AX=total sectors in drive */
	pushw	%dx
	pushw	%ax
	movzbl	2(%si), %eax	/* number of sectors to access */
	addl	8(%si), %eax	/* last sector number + 1 */
	popl	%edx		/* total sectors in drive */
	cmpl	%edx, %eax
	popl	%ebx
	popl	%edx
	jnb	1f

	/* all requested sectors can be accessed by CHS */

	/* we will access one sector at a time */

	movzbw	2(%si), %cx	/* sectors to transfer */
	movl	8(%si), %edi	/* start sector number(LBA) */
3:
	/* translate LBA to CHS */

	//pushw	%cx
	movw	%cx, %cs:(int13_chs_cx - int13_handler)

	/* get sectors per cylinder */

	//pushw	%dx
	movw	%dx, %cs:(int13_chs_dx - int13_handler)

	pushl	%edi		/* lba */
	movzbw	%cs:6(%bp), %ax	/* TO_H */
	incw	%ax		/* heads */
	movzbw	%cs:7(%bp), %bx	/* TO_S */
	andb	$0x3F, %bl
	mulw	%bx		/* DX=0, AX=sectors per cylinder */
	popw	%cx		/* lba_lo */
	xchgw	%ax, %cx	/* CX=sectors per cylinder, AX=lba_lo */
	popw	%dx		/* DX:AX=lba */

	divw	%cx		/* AX=cylinder number, DX=rem */
	xchgw	%ax, %dx	/* DX=cylinder number, AX=rem */

	movb	%dl, %ch	/* CH=lo 8 bits of cylinder */

	divb	%bl		/* AL=head number, AH=sector number - 1 */
	movb	%ah, %cl
	incw	%cx		/* CL=sector number */
	shlb	$6, %dh		/* hi 2 bits of cylinder */
	orb	%dh, %cl

	movw	%cs:(int13_chs_dx - int13_handler), %dx	/* DL=drive number */
	//popw	%dx		/* DL=drive number */
	//pushw	%dx

	movb	%al, %dh	/* DH=head number */

	//pushl	%edi
	movl	%edi, %cs:(int13_chs_edi - int13_handler)

	//movw	%di, %ax
	subw	8(%si), %di	/* sectors already read */
	shlw	$5, %di		/* paragraghs */
	addw	6(%si), %di	/* segment */
	movw	4(%si), %bx	/* offset */
	movw	%bx, %ax
	shrw	$4, %ax		/* turn to paragraghs */
	andw	$0x000F, %bx
	addw	%di, %ax	/* segment */
	movw	%ax, %es

	movb	%cs:(int13_old_eax - int13_handler + 1), %ah
				/* 0x42/0x02=read, 0x43/0x03=write */
	andb	$0x03, %ah	/* AH=0x02 or 0x03 */
	movb	$0x01, %al	/* number of sectors to read/write */

	call	int13_with_retry

	movl	%cs:(int13_chs_edi - int13_handler), %edi
	movw	%cs:(int13_chs_dx - int13_handler), %dx
	//popl	%edi
	//popw	%dx

	incl	%edi		/* next sector */

	movw	%cs:(int13_chs_cx - int13_handler), %cx
	//popw	%cx
	jc	1f
	loop	3b

	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret
1:
	stc
	jmp	*%cs:(int13_ret_IP - int13_handler)	//ret

	.align	2
int13_ret_IP:
	.word	0
int13_reg_AX:
	.word	0
int13_chs_cx:
	.word	0
int13_chs_dx:
	.word	0

	.align	4
int13_chs_edi:
	.long	0

/****************************************************************************/
int13_with_retry:

	/* save return address to memory */
	popw	%cs:(int13_retry_IP - int13_handler)
//	/* backup the last DWORD in the buffer */
//	pushl	%es:0x1FC(%bx)
//	popl	%cs:(sector_last_dword - int13_handler)

	/* retry count should be an odd number initially */
	movb	$5, %cs:(retry_count - int13_handler)
#ifdef CDROM_INIT
	cmpb	$0, %cs:(force_int13 - int13_handler)
	jnz	3f	/* we should use current int 13 instead of ROM_int13 */
#endif
	call	backup_int13
3:
	movb	%ah, %cs:(int13_retry_ah - int13_handler)
	call	int13_simple

	jnc	3f		/* success */

//	/* if the last dword changed, consider it is success */
//
//	pushl	%eax
//	movl	%es:0x1FC(%bx), %eax
//	cmpl	%eax, %cs:(sector_last_dword - int13_handler)
//	popl	%eax
//
//	jne	3f		/* success */

	decb	%cs:(retry_count - int13_handler)
	jz	2f		/* finally failed */

	/* reset disk and try again */
	movb	$0, %cs:(int13_retry_ah - int13_handler)
	call	int13_simple

//	testb	$1, %cs:(int13_old_eax - int13_handler + 1)
//				/* 0x42/0x02=read, 0x43/0x03=write */
//	jnz	3b		/* it is write */
//
//	/* touch the last dword of the buffer */
//	notl	%es:0x1FC(%bx)
//	notl	%cs:(sector_last_dword - int13_handler)

	jmp	3b		/* try again, ignoring the reset failure */
2:
	stc			/* failure */
	jmp	2f
3:
	clc			/* success */
2:
	pushfw
#ifdef CDROM_INIT
	cmpb	$0, %cs:(force_int13 - int13_handler)
	jnz	2f	/* we have not touched the int 13 vector */
#endif
	call	restore_int13
2:
	popfw
	jmp	*%cs:(int13_retry_IP - int13_handler)	//ret

backup_int13:

	/* backup the current int 13 to tmp_int13, then install ROM_int13 */
	cli
	pushw	%ds
	pushl	%eax
	xorw	%ax, %ax
	movw	%ax, %ds
	movl	0x4C, %eax	/* current int 13 vector */
	movl	%eax, %cs:(tmp_int13 - int13_handler)
	movl	%cs:(EXT_C(ROM_int13) - int13_handler), %eax
	movl	%eax, 0x4C
	popl	%eax
	popw	%ds
	ret

restore_int13:

	/* restore the current int 13 from tmp_int13 */
	cli
	pushw	%ds
	pushl	%eax
	xorw	%ax, %ax
	movw	%ax, %ds
	movl	%cs:(tmp_int13 - int13_handler), %eax
	movl	%eax, 0x4C
	popl	%eax
	popw	%ds
	ret

retry_count:
	.byte	5

int13_retry_ah:
	.byte	0
#ifdef CDROM_INIT
force_int13:
	.byte	0
#endif
	.align	2
int13_retry_IP:
	.word	0
int13_retry_ds:
	.word	0
int13_retry_es:
	.word	0

	.align	4
int13_retry_eax:
	.long	0
int13_retry_ebx:
	.long	0
int13_retry_ecx:
	.long	0
int13_retry_edx:
	.long	0
int13_retry_esi:
	.long	0
int13_retry_edi:
	.long	0
int13_retry_ebp:
	.long	0
tmp_int13:
	.long	0
//sector_last_dword:
//	.long	0

/****************************************************************************/
int13_simple:

	/* input:	CF=0	reset disk	*/

	/* save return address to memory */
	popw	%cs:(int13_simple_IP - int13_handler)

	movl	%eax, %cs:(int13_retry_eax - int13_handler)
	movl	%ebx, %cs:(int13_retry_ebx - int13_handler)
	movl	%ecx, %cs:(int13_retry_ecx - int13_handler)
	movl	%edx, %cs:(int13_retry_edx - int13_handler)
	movl	%esi, %cs:(int13_retry_esi - int13_handler)
	movl	%edi, %cs:(int13_retry_edi - int13_handler)
	movl	%ebp, %cs:(int13_retry_ebp - int13_handler)
	movw	%ds, %cs:(int13_retry_ds - int13_handler)
	movw	%es, %cs:(int13_retry_es - int13_handler)

	//sti
#ifdef CDROM_INIT
	cmpb	$0, %cs:(force_int13 - int13_handler)
	jz	3f
	/* read 1 cdrom sector using our builtin cdrom driver. */

	/* backup old variables to bak variables */
	movl	%cs:(int13_old_eax - int13_handler), %eax
	movl	%eax, %cs:(int13_bak_eax - int13_handler)
	movl	%cs:(int13_old_ebx - int13_handler), %eax
	movl	%eax, %cs:(int13_bak_ebx - int13_handler)
	movl	%cs:(int13_old_ecx - int13_handler), %eax
	movl	%eax, %cs:(int13_bak_ecx - int13_handler)
	movl	%cs:(int13_old_edx - int13_handler), %eax
	movl	%eax, %cs:(int13_bak_edx - int13_handler)
	movl	%cs:(int13_old_esi - int13_handler), %eax
	movl	%eax, %cs:(int13_bak_esi - int13_handler)
	movl	%cs:(int13_old_edi - int13_handler), %eax
	movl	%eax, %cs:(int13_bak_edi - int13_handler)
	movl	%cs:(int13_old_ebp - int13_handler), %eax
	movl	%eax, %cs:(int13_bak_ebp - int13_handler)
	movw	%cs:(int13_old_ds - int13_handler), %ax
	movw	%ax, %cs:(int13_bak_ds - int13_handler)
	movw	%cs:(int13_old_es - int13_handler), %ax
	movw	%ax, %cs:(int13_bak_es - int13_handler)
	movw	%cs:(int13_old_flags - int13_handler), %ax
	movw	%ax, %cs:(int13_bak_flags - int13_handler)
	movl	%cs:(int13_old_cs_ip - int13_handler), %eax
	movl	%eax, %cs:(int13_bak_cs_ip - int13_handler)

	/* setup old variables for use by edd30_for_cdrom */

	/* edd30_for_cdrom far return address */
	movw	%cs, %cs:(int13_old_cs_ip + 2 - int13_handler)
	movw	$(2f - int13_handler), %cs:(int13_old_cs_ip - int13_handler)
	/* edd30_for_cdrom flags */
	pushfw
	popw	%cs:(int13_old_flags - int13_handler)
	movw	%cs:(int13_retry_es - int13_handler), %ax
	movw	%ax, %cs:(int13_old_es - int13_handler)
	movw	%cs:(int13_retry_ds - int13_handler), %ax
	movw	%ax, %cs:(int13_old_ds - int13_handler)
	movl	%cs:(int13_retry_ebp - int13_handler), %eax
	movl	%eax, %cs:(int13_old_ebp - int13_handler)
	movl	%cs:(int13_retry_edi - int13_handler), %eax
	movl	%eax, %cs:(int13_old_edi - int13_handler)
	movl	%cs:(int13_retry_esi - int13_handler), %eax
	movl	%eax, %cs:(int13_old_esi - int13_handler)
	movl	%cs:(int13_retry_edx - int13_handler), %eax
	movl	%eax, %cs:(int13_old_edx - int13_handler)
	movl	%cs:(int13_retry_ecx - int13_handler), %eax
	movl	%eax, %cs:(int13_old_ecx - int13_handler)
	movl	%cs:(int13_retry_ebx - int13_handler), %eax
	movl	%eax, %cs:(int13_old_ebx - int13_handler)
	movl	%cs:(int13_retry_eax - int13_handler), %eax
	movl	%eax, %cs:(int13_old_eax - int13_handler)

	movb	%cs:(int13_retry_ah - int13_handler), %ah
	jmp	edd30_for_cdrom		/* will return at 2f */
2:
	/* restore old variables from bak variables */
	movl	%cs:(int13_bak_eax - int13_handler), %eax
	movl	%eax, %cs:(int13_old_eax - int13_handler)
	movl	%cs:(int13_bak_ebx - int13_handler), %eax
	movl	%eax, %cs:(int13_old_ebx - int13_handler)
	movl	%cs:(int13_bak_ecx - int13_handler), %eax
	movl	%eax, %cs:(int13_old_ecx - int13_handler)
	movl	%cs:(int13_bak_edx - int13_handler), %eax
	movl	%eax, %cs:(int13_old_edx - int13_handler)
	movl	%cs:(int13_bak_esi - int13_handler), %eax
	movl	%eax, %cs:(int13_old_esi - int13_handler)
	movl	%cs:(int13_bak_edi - int13_handler), %eax
	movl	%eax, %cs:(int13_old_edi - int13_handler)
	movl	%cs:(int13_bak_ebp - int13_handler), %eax
	movl	%eax, %cs:(int13_old_ebp - int13_handler)
	movw	%cs:(int13_bak_ds - int13_handler), %ax
	movw	%ax, %cs:(int13_old_ds - int13_handler)
	movw	%cs:(int13_bak_es - int13_handler), %ax
	movw	%ax, %cs:(int13_old_es - int13_handler)
	movw	%cs:(int13_bak_flags - int13_handler), %ax
	movw	%ax, %cs:(int13_old_flags - int13_handler)
	movl	%cs:(int13_bak_cs_ip - int13_handler), %eax
	movl	%eax, %cs:(int13_old_cs_ip - int13_handler)
	/* eax changed but no problem */

	jmp	2f
#endif
3:
	movb	%cs:(int13_retry_ah - int13_handler), %ah
	xorb %al, %al				//Correction write  mapping device fail in real machine     2017.05.10 yaya   
	int	$0x13
2:
	movw	%cs:(int13_retry_ds - int13_handler), %ds
	movw	%cs:(int13_retry_es - int13_handler), %es
	movl	%cs:(int13_retry_eax - int13_handler), %eax
	movl	%cs:(int13_retry_ebx - int13_handler), %ebx
	movl	%cs:(int13_retry_ecx - int13_handler), %ecx
	movl	%cs:(int13_retry_edx - int13_handler), %edx
	movl	%cs:(int13_retry_esi - int13_handler), %esi
	movl	%cs:(int13_retry_edi - int13_handler), %edi
	movl	%cs:(int13_retry_ebp - int13_handler), %ebp

	jmp	*%cs:(int13_simple_IP - int13_handler)	//ret

	.align	2
int13_simple_IP:
	.word	0

	.align	4
int13_bak_cs_ip:	.long	0
int13_bak_eax:		.long	0
int13_bak_ebx:		.long	0
int13_bak_ecx:		.long	0
int13_bak_edx:		.long	0
int13_bak_esi:		.long	0
int13_bak_edi:		.long	0
int13_bak_ebp:		.long	0
int13_bak_ds:		.word	0
int13_bak_es:		.word	0
int13_bak_flags:	.word	0

/****************************************************************************/
modify_boot_sectors:

	movw	%cs:(int13_old_eax - int13_handler), %ax
//	/* check CHS read */
//	cmpb	$0x02, %ah	/* is it read? */
//	jne	3f
//	cmpw	$0x0001, %cx	/* read from cylinder 0, sector 1? */
//	je	5f
//3:
	/* check LBA read */
	//cmpb	$0x42, %ah	/* is it extended read? */
	testb	$0x01, %ah	/* is it extended read? */
	jne	4f		/* no, do nothing. */
	cmpl	$0, 12(%si)	/* read from LBA_high=0? */
	jne	4f		/* no, do nothing. */

	movb	$0, %dh		/* simulate CHS read of head 0 */
	cmpl	$0, 8(%si)	/* read from LBA_low=0? */
	je	3f		/* yes, continue. */

	movzbl	%cs:3(%bp), %eax	/* AL=Smax */
	andb	$63, %al	/* EAX=sectors per track */
	movb	$1, %dh		/* simulate CHS read of head 1 */
	cmpl	%eax, 8(%si)	/* read from LBA_low=sectors per track? */
	jne	4f		/* no, exit. */
3:
	//movw	4(%si), %bx	/* simulate CHS read buffer offset */
	//movw	6(%si), %es	/* simulate CHS read buffer segment */
	lesw	4(%si), %bx

5:
	/* CHS mode read from cylinder 0, sector 1 */

#;	testb	%dl, %dl	/* The TO_DRIVE is hard drive? */
#;	jns	4f		/* no, do nothing */
	cmpw	$0xaa55, %es:0x1fe(%bx)	/* is it a valid boot sector? */
	jne	4f			/* no, skip */
	movl	%cs:16(%bp), %eax	/* S_count_Lo */
	shrl	$1, %eax	/* map a whole drive? */
	jz	4f		/* yes, nothing need to change */
	movl	%cs:8(%bp), %eax	/* StartLBA_Lo */
	testl	%eax, %eax	/* geometry translation only? */
	jz	4f		/* yes, needn't change */
	movl	%cs:(%bp), %eax	/* FROM_DRIVE, TO_DRIVE, H, S */

	testb	%al, %al	/* The FROM_DRIVE is hard drive? */
	jns	modify_floppy	/* no, goto floppy boot record modification */

	testb	%dh, %dh	/* read from head 0? */
	jnz	modify_HD_DOS	/* no, goto HD DOS boot record modification */

	/* we have read an MBR, and we need to modify the partition table */

	/* the partition table could be an extended partition table. if so,
	 * we need to turn it to be a primary partition table.
	 */

	/* if all start_sectors are not sectors_per_track, then it is not
	 * an extended partition table.
	 */

	/* first, the MS magic number should be non-zero */
	cmpl	$0, %es:0x1b8(%bx)
	jne	5f
	movb	%al, %es:0x1b8(%bx)	/* let it be the FROM_DRIVE */
5:
	shrl	$16, %eax	/* AL=Hmax, AH=Smax */
	andb	$63, %ah	/* AH=Smax */
//	cmpb	$1, %ah
//	jbe	4f /* do not modify partition table when disable CHS mode */

	/* an extended partition table should have an entry with
	 * StartSector=Smax.
	 */
	movzbl	%ah, %eax
	movw	$0x1c6, %si		/* SI=0x1c6 */
	cmpl	%eax, %es:(%bx, %si)
	jz	5f
	addw	$16, %si		/* SI=0x1d6 */
	cmpl	%eax, %es:(%bx, %si)
	jz	5f
	addw	$16, %si		/* SI=0x1e6 */
	cmpl	%eax, %es:(%bx, %si)
	jz	5f
	addw	$16, %si		/* SI=0x1f6 */
	cmpl	%eax, %es:(%bx, %si)
	jnz	4f	/* not an extended partition table */
5:
	/* check if it is a primary partition table */

	/* now the StartSector=Smax. if it is a primary partition table entry,
	 * it must have C/H/S=0/1/1.
	 */

	movl	%es:-8(%bx, %si), %eax
	shrl	$8, %eax	/* hi word:cylinder/sector, lo word:head */
	cmpl	$0x000101, %eax		/* 0x00=C, 0x01=S, 0x01=H */
	je	4f	/* primary partition table, nothing to do */

	/* now we are sure this is an extended partition table */

	/* compose a master boot record routine */

	cld
#if 1
	movw	%bx, %di
	movb	$0xFA, %al		/* cli */
	stosb
	movl	$0xD08EC033, %eax	/* xor AX,AX; mov SS,AX */
	stosl
	movl	$0xFB7C00BC, %eax	/* mov SP,7C00 ; sti */
	stosl
	movl	$0x07501F50, %eax	/* push AX; pop DS; push AX; pop ES */
	stosl
	movl	$0x7C1CBEFC, %eax	/* cld; mov SI,7C1C */
	stosl
	movl	$0x50061CBF, %eax	/* mov DI,061C ; push AX */
	stosl
	movl	$0x01E4B957, %eax	/* push DI ; mov CX, 01E4 */
	stosl
	movl	$0x1ECBA4F3, %eax	/* repz movsb; retf; push DS */
	stosl
	movl	$0x537C00BB, %eax	/* mov BX,7C00 ; push BX */
	stosl
	movl	$0x520180BA, %eax	/* mov DX,0180 ; push DX */
	stosl
	movl	$0x530201B8, %eax	/* mov AX,0201 ; push BX */
	stosl
	movl	$0x5F13CD41, %eax	/* inc CX; int 13; pop DI */
	stosl
	movl	$0x5607BEBE, %eax	/* mov SI,07BE ; push SI */
	stosl
	movl	$0xCBFA5A5D, %eax	/* pop BP; pop DX; cli; retf */
	stosl
#endif

	/* empty all other entries except this one by 2 steps: */

	/* step 1. move this entry onto the first one (overwrite it) */
	movl	%es:-8(%bx, %si), %eax
	movl	%eax, %es:0x1be(%bx)
	movl	%es:-4(%bx, %si), %eax
	movl	%eax, %es:0x1c2(%bx)
	movl	%es:(%bx, %si), %eax
	movl	%eax, %es:0x1c6(%bx)
	movl	%es:4(%bx, %si), %eax
	movl	%eax, %es:0x1ca(%bx)

#if 1
	/* step 2. empty the last 3 entries */
	xorw	%ax, %ax
	movw	%bx, %di
	addw	$0x1ce, %di
	movw	$24, %cx
	repz stosw		/* DI=BX+0x1fe */
#else
	xorw	%ax, %ax
	movw	%bx, %di
	addw	$0x1d1, %di
	stosw			/* DI=BX+0x1d3 */
	addw	$2, %di		/* DI=BX+0x1d5 */
	stosb			/* DI=BX+0x1d6 */
	addw	$0x28, %di	/* DI=BX+0x1fe */
#endif

	/* modify the start_CHS of the first entry */
	subw	$0x40, %di	/* DI=BX+0x1be */
	movb	$0x80, %al	/* set boot indicator */
	stosb			/* DI=BX+0x1bf */
	movb	$0x01, %al	/* AX=1 */
	stosb			/* DI=BX+0x1c0, H=0x01 */
	stosw			/* DI=BX+0x1c2, S=0x01, C=0x00 */

#if 0
	movb	$0x0B, %al
	stosb			/* DI=BX+0x1c3 */
#else
	/* modify the end_CHS of the first entry */
	incw	%di		/* DI=BX+0x1c3 */
#endif

	/* get total sectors */
	movl	%es:7(%di), %eax	/* DI+7=BX+0x1ca */

	/* calculate the end sector number */
	addl	%es:3(%di), %eax	/* DI+3=BX+0x1c6 */
	//decl	%eax

	pushl	%eax		/* EAX-1=end sector number */
	movzwl	%cs:2(%bp), %eax	/* Hmax, Smax */
	andb	$63, %ah
	movzbl	%ah, %ecx
	mulb	%cl
	addw	%cx, %ax	/* EAX=sectors per cylinder */
	pushl	%eax		/* EAX=sectors per cylinder */
	movw	$1024, %cx
	mull	%ecx		/* EAX=CHS addressible total sectors */
				/* EDX=0 */
	popl	%edx		/* EDX=sectors per cylinder */
	popl	%ecx		/* ECX-1=end sector number */
	cmpl	%eax, %ecx
	jb	5f

	/* assign max end sector number */
	movl	%eax, %ecx
	//subl	$0x3EC1, %ecx	/* XXX: 0x3EC1=255*63 */
5:
	pushl	%ecx		/* ECX-1=end sector number */
	movl	%edx, %ecx	/* ECX=sectors per cylinder */
	xorl	%edx, %edx
	popl	%eax		/* EAX-1=end sector number */
	decl	%eax

	divl	%ecx		/* EAX=cylinder number */
				/* EDX=sector number in the last cylinder */
				/* EAX hi=0, EDX hi=0 */
	xchgw	%ax, %cx	/* CX=cylinder number */
	xchgw	%ax, %dx	/* AX=sector number in the last cylinder */
	movb	%cs:3(%bp), %dl	/* DL=Smax */
	andb	$63, %dl
	divb	%dl		/* AL=head number, AH=sector number - 1 */
	stosb			/* DI=BX+0x1c4 */
	movb	%ah, %al
	incw	%ax		/* AL=sector number */
	movb	%cl, %ah	/* cylinder lo 8 bits */
	shlb	$6, %ch		/* cylinder hi 2 bits */
	orb	%ch, %al
	//movb	$0, %ah		/* XXX: let cylinder lo 8 bits=0 */
	stosw			/* DI=BX+0x1c6 */

4:
	/* end partition table modification */
	/* needn't restore registers */
	ret

/****************************************************************************/
modify_floppy:

	/* AL=FROM_DRIVE is the floppy drive number. */

	cmpb	$0x00, %dh	/* read from head 0? */
	jne	4b

	testw	$0x0800, %cs:4(%bp)	/* TO_C bit 11=bootsector has known filesystem */
	jz	4b		/* unknown boot sector */

	pushl	%ecx
	pushw	%si

	/* FAT12/FAT16/NTFS drive number is at offset 0x24. */
	movw	$0x24, %si

	xorl	%ecx, %ecx

	/* check if it is FAT32. */

	/* FAT32 should have 0 root_dir_entries and total_sectors_short. */
	cmpl	%ecx, %es:0x11(%bx)
	jne	5f			/* not FAT32 */

	/* FAT32 should have 0 sectors_per_fat. */
	cmpw	%cx, %es:0x16(%bx)
	jne	5f			/* not FAT32 */

	/* FAT32 should have non-zero total_sectors_long. */
	cmpl	%ecx, %es:0x20(%bx)
	je	5f			/* not FAT32 */

	/* FAT32 should have non-zero sectors_per_fat32. */
	cmpl	%ecx, %es:0x24(%bx)
	je	5f			/* not FAT32 */

	/* Now it is FAT32, and the drive number is at offset 0x40. */
	movw	$0x40, %si
5:
	movb	%al, %es:(%bx, %si)	/* modify the boot drive number. */

	movl	%ecx, %es:0x1c(%bx)	/* let number of hidden sectors=0 */
	/*movb	$0xf0, %es:0x15(%bx)*/	/* set floppy media descriptor */

	popw	%si
	popl	%ecx

	jmp	4b

/****************************************************************************/
modify_HD_DOS:

	cmpb	$0x01, %dh		/* read from head 1? */
	jne	4b
	movl	%cs:8(%bp), %eax		/* StartLBA_Lo */
	testl	%eax, %eax
	jz	4b
	cmpl	%eax, %es:0x1c(%bx)	/* Number of hidden sectors */
	jbe	4b
	subl	%eax, %es:0x1c(%bx)
	xorl	%eax, %eax
	cmpl	%eax, %es:0x1c(%bx)
	jne	4b
	movb	%cs:3(%bp), %al		/* AL=Smax */
	andb	$63, %al
	movb	%al, %es:0x1c(%bx)
	//cmpl	%eax, %es:0x1c(%bx)	/* Number of hidden sectors */
	//jnb	4b
	//movl	%eax, %es:0x1c(%bx)
	jmp	4b

/****************************************************************************/
modify_in_situ:

	movb	%al, %cs:(int13_in_situ_sectors - int13_handler)

	cld

	/* if FROM is not harddrive, do nothing. */
	testb	$0x80, %cs:(%bp)	/* FROM drive */
	jz	4f

	cmpb	$0x02, %ah	/* is it CHS read? */
	jne	3f

	/* AL=sectors read */

	/* translate CHS to LBA */

	movw	%dx, %di	/* save DX to DI */
	movw	%cx, %si	/* save CX to SI */

	movb	%cs:7(%bp), %al	/* TO_S */
	andb	$63, %al
	movzbw	%al, %dx
	mulb	%cs:6(%bp)	/* TO_H */
	addw	%dx, %ax	/* AX=sectors_per_cylinder */

	/* get current cylinder number */

	shrb	$6, %cl
	xchgb	%cl, %ch

	mulw	%cx		/* DX:AX=sectors */

	pushw	%dx
	pushw	%ax

	/* restore DX */

	movw	%di, %dx

	/* get current head number */

	movb	%cs:7(%bp), %al	/* TO_S */
	andb	$63, %al
	mulb	%dh		/* AX=sectors */

	popl	%edx

	movzwl	%ax, %eax
	addl	%edx, %eax

	movw	%si, %cx	/* restore CX */
	andb	$0x3F, %cl
	decw	%cx
	movzbl	%cl, %ecx
	addl	%ecx, %eax	/* EAX=start sector number */
	movzbl	%cs:(int13_in_situ_sectors - int13_handler), %ecx
	jmp	6f

3:
	cmpb	$0x42, %ah	/* is it LBA read? */
	jne	4f
	cmpl	$0, 12(%si)	/* read from LBA_high=0? */
	jne	4f
	movl	8(%si), %eax	/* start sector number */
	movzbl	2(%si), %ecx	/* sectors read */
	//movw	4(%si), %bx	/* simulate CHS read buffer offset */
	//movw	6(%si), %es	/* simulate CHS read buffer segment */
	lesw	4(%si), %bx
6:
#;	testb	%dl, %dl	/* The TO_DRIVE is hard drive? */
#;	jns	4f		/* no, do nothing */
	/* ECX=sectors read */
	cmpl	$0, %eax	/* start sector number 0 is for MBR */
	je	5f

	/* if EAX <= startLBA_Lo < EAX + ECX, then the boot sector is read. */

	cmpl	%cs:8(%bp), %eax	/* startLBA_Lo */
	ja	4f		/* boot sector was not read */
	addl	%eax, %ecx
	cmpl	%cs:8(%bp), %ecx
	jbe	4f		/* boot sector was not read */

	movl	%cs:8(%bp), %ecx
	subl	%eax, %ecx	/* sectors between ES:BX and boot record */

	/* modify hidden sectors of the partition boot record */
	cmpb	$0x07, %cs:4(%bp)	/* NTFS */
	je	3f
	cmpb	$0x0C, %cs:4(%bp)	/* FAT32(LBA) */
	je	3f
	cmpb	$0x0E, %cs:4(%bp)	/* FAT12/16 */
	je	3f
	cmpb	$0x83, %cs:4(%bp)	/* EXT2 */
	jne	4f
3:
	movw	%es, %ax
	shlw	$5, %cx		/* 1 sector is 32 paragraphs */
	addw	%cx, %ax
	movw	%ax, %es
	cmpw	$0xaa55, %es:0x1fe(%bx)
	jne	4f
	movl	%cs:8(%bp), %eax	/* startLBA_Lo */
	movl	%eax, %es:0x1c(%bx)

	jmp	4f
5:
	/* Modify partition table. Note that there are 4 partition entries.
	 * The first one is called entry 0, and the last is entry 3. */

	cmpw	$0xaa55, %es:0x1fe(%bx)
	jne	4f

	/* first, the MS magic number should be non-zero */
	cmpl	$0, %es:0x1b8(%bx)
	jne	5f
	movb	%cs:(%bp), %al	/* FROM drive */
	movb	%al, %es:0x1b8(%bx)	/* let it be the FROM_DRIVE */
5:
	leaw	0x1be(%bx), %di	/* DI=BX+0x1be */

	movw	%di, %si

	/* if the first entry is empty, we simply put our new entry on it. */

	//cmpb	$0, %es:4(%si)	/* consider partition type 00 as empty */
	//jz	5f
	testb	$63, %es:2(%si)	/* invalid start sector number of 0 */
	jz	5f
	testb	$63, %es:6(%si)	/* invalid end sector number of 0 */
	jz	5f
	cmpl	$0, %es:8(%si)	/* invalid start LBA of 0 */
	jz	5f
	cmpl	$0, %es:12(%si)	/* invalid sector count of 0 */
	jz	5f

	/* Now that the first entry is not empty, we should find an empty one
	 * in entries 1, 2, 3(the last 3 partition entries). */

7:
	addw	$0x10, %si	/* SI=BX+0x1ce or BX+0x1de or BX+0x1ee */

	//cmpb	$0, %es:4(%si)	/* consider partition type 00 as empty */
	//jz	7f
	testb	$63, %es:2(%si)	/* invalid start sector number of 0 */
	jz	7f
	testb	$63, %es:6(%si)	/* invalid end sector number of 0 */
	jz	7f
	cmpl	$0, %es:8(%si)	/* invalid start LBA of 0 */
	jz	7f
	cmpl	$0, %es:12(%si)	/* invalid sector count of 0 */
	jz	7f

	leaw	0x1ee(%bx), %bx
	cmpw	%bx, %si
	leaw	-0x1ee(%bx), %bx
	jb	7b		/* try next entry */

	/* Now SI=BX+0x1ee points to the last entry */

7:
	/* SI points to empty entry(or the last entry), we move old entry 0
	 * onto this one.
	 */

	movl	%es:(%di), %eax
	movl	%eax, %es:(%si)
	movl	%es:4(%di), %eax
	movl	%eax, %es:4(%si)
	movl	%es:8(%di), %eax
	movl	%eax, %es:8(%si)
	movl	%es:12(%di), %eax
	movl	%eax, %es:12(%si)

5:
	/* build our new entry 0 */

	/* DI=BX+0x1be */

	movl	%cs:8(%bp), %eax	/* startLBA_Lo */

	call	lba_to_chs

	movb	$0x80, %al	/* set boot indicator */
	//movb	$0x00, %al	/* clear boot indicator */
	stosl			/* DI=BX+0x1c2 */

	movl	%cs:16(%bp), %eax
	addl	%cs:8(%bp), %eax
	decl	%eax		/* endLBA */

	call	lba_to_chs

	/* if in situ, TO_C holds the partition type:
	 * 	0x07(NTFS), 0x0C(FAT32), 0x0E(FAT12/16), 0x83(EXT2/3)
	 */
	movb	%cs:4(%bp), %al	/* TO_C */
	stosl			/* DI=BX+0x1c6 */

	/* modify start sector and sector count */

	movl	%cs:8(%bp), %eax
	stosl			/* DI=BX+0x1ca */
	movl	%cs:16(%bp), %eax
	stosl			/* DI=BX+0x1ce */

	/* bit 8 of TO_C means clearing other partition entries. */
	testb	$1, %cs:5(%bp)	/* TO_C hi word for in_situ_flags */
	jz	3f

	/* empty the last 3 entries */
	xorw	%ax, %ax
	movw	%bx, %di
	addw	$0x1ce, %di
	movw	$24, %cx
	repz stosw		/* DI=BX+0x1fe */
	ret
3:
#if 1
	/* clear other boot indicators in the last 3 entries */

	movb	$0, %al		/* DI=BX+0x1ce */
	stosb			/* DI=BX+0x1cf */

	leaw	0x1de(%bx), %di	/* DI=BX+0x1de */
	stosb			/* DI=BX+0x1df */

	leaw	0x1ee(%bx), %di	/* DI=BX+0x1ee */
	stosb			/* DI=BX+0x1ef */
#endif

//#if 0
//	/* hide entry 3 (test only) */
//	orb	$0x10, %es:0x1f2(%bx)
//#endif

#if 0
	/* move extended partition entries to last */

	cmpb	$0x0F, %es:0x1f2(%bx)
	je	4f		/* entry 3 is extended, do nothing */

	cmpb	$0x05, %es:0x1f2(%bx)
	je	4f		/* entry 3 is extended, do nothing */

	cmpb	$0x0F, %es:0x1e2(%bx)
	je	7f		/* entry 2 is extended */

	cmpb	$0x05, %es:0x1e2(%bx)
	je	7f		/* entry 2 is extended */

	/* both entry 2 and entry 3 are not extended */

	cmpb	$0x0F, %es:0x1d2(%bx)
	je	8f		/* entry 1 is extended */

	cmpb	$0x05, %es:0x1d2(%bx)
	jne	4f	/* all entries 1, 2, 3 are not extended, do nothing */
8:
	/* entry 1 is extended, but entry 2 and entry 3 are not extended.
	 * so exchange entry 1 and entry 3. */

	xchgl	%eax, %es:0x1ce(%bx)
	xchgl	%eax, %es:0x1ee(%bx)
	xchgl	%eax, %es:0x1ce(%bx)

	xchgl	%eax, %es:0x1d2(%bx)
	xchgl	%eax, %es:0x1f2(%bx)
	xchgl	%eax, %es:0x1d2(%bx)

	xchgl	%eax, %es:0x1d6(%bx)
	xchgl	%eax, %es:0x1f6(%bx)
	xchgl	%eax, %es:0x1d6(%bx)

	xchgl	%eax, %es:0x1da(%bx)
	xchgl	%eax, %es:0x1fa(%bx)
	xchgl	%eax, %es:0x1da(%bx)

	jmp	4f

7:
	/* entry 2 is extended, but entry 3 is not. if entry 1 is extended,
	 * we exchange entry 1 and 3, else, we exchange entry 2 and 3. */
	 */

	cmpb	$0x0F, %es:0x1d2(%bx)
	je	8b		/* entry 1 is extended */

	cmpb	$0x05, %es:0x1d2(%bx)
	je	8b		/* entry 1 is extended */

	/* exchange entry 2 and 3 */
	xchgl	%eax, %es:0x1de(%bx)
	xchgl	%eax, %es:0x1ee(%bx)
	xchgl	%eax, %es:0x1de(%bx)

	xchgl	%eax, %es:0x1e2(%bx)
	xchgl	%eax, %es:0x1f2(%bx)
	xchgl	%eax, %es:0x1e2(%bx)

	xchgl	%eax, %es:0x1e6(%bx)
	xchgl	%eax, %es:0x1f6(%bx)
	xchgl	%eax, %es:0x1e6(%bx)

	xchgl	%eax, %es:0x1ea(%bx)
	xchgl	%eax, %es:0x1fa(%bx)
	xchgl	%eax, %es:0x1ea(%bx)
#endif

//	/* empty the last 3 entries */
//	xorw	%ax, %ax
//	movw	%bx, %di
//	addw	$0x1ce, %di
//	movw	$24, %cx
//	repz stosw		/* DI=BX+0x1fe */

4:
	/* end partition table and BPB modification */
	ret

int13_in_situ_sectors:
	.byte	0

/****************************************************************************/

lba_to_chs:

	/* input:
	 *
	 *	EAX:	LBA
	 *
	 * output:
	 *
	 *	EAX:	CHS
	 *
	 *
	 *
	 */

	pushl	%eax		/* EAX=end sector number */
	movzwl	%cs:6(%bp), %eax	/* TO_H, TO_S */
	andb	$63, %ah
	movzbl	%ah, %ecx
	mulb	%cl
	addw	%cx, %ax	/* EAX=sectors per cylinder */
	pushl	%eax		/* EAX=sectors per cylinder */
	movw	$1024, %cx
	mull	%ecx		/* EAX=CHS addressible total sectors */
				/* EDX=0 */
	popl	%edx		/* EDX=sectors per cylinder */
	popl	%ecx		/* ECX=end sector number */
	decl	%eax
	cmpl	%eax, %ecx
	jb	5f

	/* assign max end sector number */
	movl	%eax, %ecx
	//subl	$0x3EC1, %ecx	/* XXX: 0x3EC1=255*63 */
5:
	pushl	%ecx		/* ECX=end sector number */
	movl	%edx, %ecx	/* ECX=sectors per cylinder */
	xorl	%edx, %edx
	popl	%eax		/* EAX=end sector number */

	divl	%ecx		/* EAX=cylinder number */
				/* EDX=sector number in the last cylinder */
				/* EAX hi=0, EDX hi=0 */
	xchgw	%ax, %cx	/* CX=cylinder number */
	xchgw	%ax, %dx	/* AX=sector number in the last cylinder */
	movb	%cs:7(%bp), %dl	/* TO_S */
	andb	$63, %dl
	divb	%dl		/* AL=head number, AH=sector number - 1 */
	//stosb			/* DI=BX+0x1c4 */

	xchgb	%ah, %al	/* AH=head number, AL=sector number - 1 */

	pushw	%ax

	//movb	%ah, %al
	incw	%ax		/* AL=sector number */
	movb	%cl, %ah	/* cylinder lo 8 bits */
	shlb	$6, %ch		/* cylinder hi 2 bits */
	orb	%ch, %al
	//movb	$0, %ah		/* XXX: let cylinder lo 8 bits=0 */
	//stosw			/* DI=BX+0x1c6 */
	shll	$16, %eax

	popw	%ax

	ret

/****************************************************************************/
#ifdef CDROM_INIT
max_cdrom_id:		.byte	0xE0
#endif
	.align	4
ENTRY(e820cycles)
	.long	-1	/* no limit. set to 0 if int15 will unhook */
ENTRY(int15nolow)
	.long	0	/* set to 1 if int15 will not handle low memory */
ENTRY(memdisk_raw)
	.long	1	/* set to 0 if accessing memdrives using int15/87h */
ENTRY(a20_keep_on)
	.long	1	/* set to 0 if a20 should be auto-off */
ENTRY(lba_cd_boot)
	.long	0	/* LBA of no-emu boot image, in 2048-byte sectors */

minimum_mem_lo_in_map:
	.long	0	/* min ram drive base below 16M */
ENTRY(minimum_mem_hi_in_map)
	.long	0	/* min ram drive base above 16M */
#if (repair_memory_holes)
ENTRY(prime_end)
	.long	0,0
ENTRY(new_end)
	.long	0,0
#endif
#if 0
int15_e820_saved_ebx:
	.long	0
int15_e820_range0_addr:
	.long	0, 0
int15_e820_range0_length:
	.long	((int13_handler_end - int13_handler + 0x3ff)&(-0x400)), 0
int15_e820_range0_type:
	.long	2
#endif

/****************************************************************************/
int15_e820_handler:

	/* Comments are mostly gotten from Ralf Brown's Interrupt List. */

	cmpw	$0xe820, %ax		//cmpl	$0x0000e820, %eax
	jne	1f

	/* Newer BIOSes - GET SYSTEM MEMORY MAP */

	//AX = E820h
	//EAX = 0000E820h
	//EDX = 534D4150h ('SMAP')
	//EBX = continuation value or 00000000h to start at beginning of map
	//ECX = size of buffer for result, in bytes (should be >= 20 bytes)
	//ES:DI -> buffer for result

	//Return:
	//CF clear if successful
	//EAX = 534D4150h ('SMAP')
	//ES:DI buffer filled
	//EBX = next offset from which to copy or 00000000h if all done
	//ECX = actual length returned in bytes
	//CF set on error
	//AH = error code (86h)

	/* Notes: Originally introduced with the Phoenix BIOS v4.0, this
	 * function is now supported by most newer BIOSes, since various
	 * versions of Windows call it to find out about the system memory.
	 * A maximum of 20 bytes will be transferred at one time, even if ECX
	 * is higher; some BIOSes (e.g. Award Modular BIOS v4.50PG) ignore the
	 * value of ECX on entry, and always copy 20 bytes. Some BIOSes expect
	 * the high word of EAX to be clear on entry, i.e. EAX=0000E820h. If
	 * this function is not supported, an application should fall back to
	 * AX=E802h, AX=E801h, and then AH=88h. The BIOS is permitted to return
	 * a nonzero continuation value in EBX and indicate that the end of the
	 * list has already been reached by returning with CF set on the next
	 * iteration. This function will return base memory and ISA/PCI memory
	 * contiguous with base memory as normal memory ranges; it will
	 * indicate chipset-defined address holes which are not in use and
	 * motherboard memory-mapped devices, and all occurrences of the system
	 * BIOS as reserved; standard PC address ranges will not be reported
	 */

//Format of Phoenix BIOS system memory map address range descriptor:
//
//Offset Size	Description
//------ -----	----------------------
//  00h  QWORD	base address
//  08h  QWORD	length in bytes
//  10h  DWORD	type of address range

//Values for System Memory Map address type:
//01h memory, available to OS
//02h reserved, not available (e.g. system ROM, memory-mapped device)
//03h ACPI Reclaim Memory (usable by OS after reading ACPI tables)
//04h ACPI NVS Memory (OS is required to save this memory between NVS sessions)
//other not defined yet -- treat as Reserved

	cmpl	$0x534D4150, %edx	/* "SMAP" */
	jne	99f	/* not a valid e820 call. But could be another */
			/* unknown call. So pass through to original BIOS. */

	cmpl	$20, %ecx
	jb	2f	/* treat this as error: invalid call. */

#if (repair_memory_holes)	
	orl	%ebx, %ebx
	jne	3f
	movl	%ebx, %cs:(prime_end - int13_handler)
	movl	%ebx, %cs:(prime_end + 4 - int13_handler)
	movl	%ebx, %cs:(new_end - int13_handler)
	movl	%ebx, %cs:(new_end + 4 - int13_handler)
#endif	
3:
#if 0
	cmpl	$1, %ebx
	jb	8f	/* EBX = 0 : start at beginning */
	ja	3f	/* input EBX > 1 : invalid */
	/* input EBX = 1 : continue */
	movl	%cs:(int15_e820_saved_ebx - int13_handler), %ebx
	test	%ebx, %ebx
	jz	9f	/* BIOS ranges ended */
8:
#endif
	//pushfw
	//lcall	%cs:*(EXT_C(ROM_int15) - int13_handler)
	call	safe_ROM_int15

	jc	3f	/* it signals end of a cycle of the e820 calls */
	cmpl	$0x534D4150, %eax	/* "SMAP" */
	jne	3f	/* unknown failure. Also end the cycle for safety. */
#if 0
	movl	%ebx, %cs:(int15_e820_saved_ebx - int13_handler)
	movl	$1, %ebx
#endif
	#####################################################################
	pushal
	#####################################################################

	xorl	%eax, %eax
	cdq				/* let EDX=0 */
	movw	%cs, %ax
	shll	$4, %eax

	/* EDX:EAX=linear address of int13_handler */

#if 1
	/* get 0x413 memory size */
	pushw	%ds
	movw	%dx, %ds	/* DS = 0 */
	movzwl	0x413, %ebx
	shll	$10, %ebx
	jz	5f		/* invalid mem size */
	cmpl	%ebx, %eax
	jbe	5f
	xchgl	%eax, %ebx	/* movl %ebx, %eax */
5:
	popw	%ds
#endif

	pushl	%eax		/* available low mem size in bytes */

	movl	%es:16(%di), %eax	/* Type: 1 = usable memory. */
	decl	%eax			/* is it usable? */
	jz	5f			/* yes, modify usable mem range */
#if (!repair_memory_holes)
	/* it is reserved memory. only modify low mem range at this moment. */
	cmpl	%edx, %cs:(EXT_C(int15nolow) - int13_handler) /* EDX=0 */
	jnz	4f	/* int15 will not handle low mem */
	cmpl	%edx, %es:4(%di)	/* BaseAddrHigh */
	jnz	4f			/* not low mem */
	cmpl	%edx, %es:12(%di)	/* LengthHigh */
	jnz	4f			/* not low mem */
	movl	%es:(%di), %eax		/* BaseAddrLow */
	addl	%es:8(%di), %eax	/* LengthLow, EAX=EndAddrLow */
	jc	4f
	cmpl	$0xA0000, %eax		/* EndAddrLow */
	jne	4f			/* not a top range, will not touch */

	/* Yes, modify this range */
	popl	%eax		/* available low mem size in bytes */
	pushl	%eax		/* available low mem size in bytes */
	movl	%eax, %es:(%di)
	movl	$0xA0000, %es:8(%di)
	subl	%eax, %es:8(%di)
	jmp	4f	/* done modifying reserved low mem range */
#else
	andb	$0xfe,	%cs:(prime_end - int13_handler)
	movl	%es:4(%di), %edx
	cmpl	%edx, %cs:(prime_end + 4 - int13_handler)	//0
	jne 4f
	movl	%es:(%di), %eax
	cmpl	%eax, %cs:(prime_end - int13_handler)
	jne 4f

	subl	%cs:(new_end - int13_handler), %eax
	sbbl	%cs:(new_end + 4 - int13_handler), %edx
	addl	%eax, %es:8(%di)
	adcl	%edx, %es:12(%di)

	movl	%cs:(new_end - int13_handler), %eax
	movl	%cs:(new_end + 4 - int13_handler), %edx
	movl	%eax, %es:(%di)
	movl	%edx, %es:4(%di)
	jmp 4f
#endif
5:
	/* it is usable memory. */

	/* check if overlapping with int13_handler and memdrives. */

#if	MAP_NUM_16
	movw	$(EXT_C(hooked_drive_map_1)-int13_handler-DRIVE_MAP_SLOT_SIZE), %si
#else
	movw	$(EXT_C(hooked_drive_map)-int13_handler-DRIVE_MAP_SLOT_SIZE), %si
#endif

#if (!repair_memory_holes)
	movw	$(DRIVE_MAP_SIZE + 1), %cx
#else
	movw	$(DRIVE_MAP_SIZE + 1), %bp
#endif
	popl	%eax		/* available low mem size in bytes */
	pushl	%eax		/* available low mem size in bytes */

#if (!repair_memory_holes)
	cmpl	%edx, %cs:(EXT_C(int15nolow) - int13_handler) /* EDX=0 */
#else
	cmpl	$0, %cs:(EXT_C(int15nolow) - int13_handler)
#endif
	jz	7f	/* int15 will handle low mem */
	jmp	5f	/* this "jmp" instruction can be omitted. */
6:
#if	MAP_NUM_16
	cmpw	$EXT_C(hooked_drive_map_1) - int13_handler + DRIVE_MAP_SIZE * DRIVE_MAP_SLOT_SIZE / 2, %si		//hooked_drive_map_1 end
	jne	8f
	movw	$(EXT_C(hooked_drive_map_2) - int13_handler), %si
8:
#endif
	/* the code should not change SI, DI and CX! */
	cmpb	$0xff, %cs:1(%si)
	jne	5f			/* not memdrive */
	testb	$0x40, %cs:5(%si)	/* TO_C bit 14=(TO is cdrom) */
	jnz	5f			/* not memdrive */
	movl	%cs:12(%si), %edx	/* start_sector hi */
	movl	%cs:8(%si), %eax	/* start_sector lo */
	shldl	$9, %eax, %edx
	shll	$9, %eax		/* EDX:EAX=start_address of mem drive*/
7:
	subl	%es:(%di), %eax
	sbbl	%es:4(%di), %edx	/* EDX:EAX=new length */
	jb	5f			/* start_address is below the base */
	ja	7f
	/* now EDX=0 */
	testl	%eax, %eax		/* EAX also be zero? ZF holds it! */
	jne 7f
	/* Change memory address type from 1 (usable) to 2 (reserved). Note
	 * that original value is long integer of 1 (all higher 3 bytes = 0).
	 * So only the lowest byte needs to be incremented. */
	incb	%es:16(%di)		/* memory address type */
	jmp	5f
7:
	subl	%es:8(%di), %eax
	sbbl	%es:12(%di), %edx	/* EDX:EAX=difference */
	jnb	5f			/* new length is too big */
#if (!repair_memory_holes)
	addl	%eax, %es:8(%di)
	adcl	%edx, %es:12(%di)	/* apply new length */
#else
	movl	%es:8(%di), %ebx
	movl	%es:12(%di), %ecx
	addl	%ebx, %eax
	adcl	%ecx, %edx
	movl	%eax, %es:8(%di)
	movl	%edx, %es:12(%di)
	addl	%es:(%di), %eax
	adcl	%es:4(%di), %edx
	movl	%eax, %cs:(new_end - int13_handler)
	movl	%edx, %cs:(new_end + 4 - int13_handler)

	testb	$1,	%cs:(prime_end - int13_handler)
	jne 5f
	addl	%es:(%di), %ebx
	adcl	%es:4(%di), %ecx
	movl	%ebx, %cs:(prime_end - int13_handler)
	movl	%ecx, %cs:(prime_end + 4 - int13_handler)
	orb	$1, %cs:(prime_end - int13_handler)
#endif
5:
	/* try next slot */
	addw	$DRIVE_MAP_SLOT_SIZE, %si
#if (!repair_memory_holes)
	loop	6b
#else
	dec %bp
	jne 6b
#endif
	/* done modifying usable mem range */
4:
	popl	%eax		/* available low mem size in bytes */
	#####################################################################
	popal
	#####################################################################
	testl	%ebx, %ebx
	jnz	5f
	call	dec_e820cycles
5:
	clc
	lret	$2
#if 0
9:
	pushfw
	pushw	%si
	pushw	%di
	cld
	movw	$(int15_e820_range0_addr - int13_handler), %si
	movw	$(20/2), %cx
	rep movsw %cs:(%si), %es:(%di)
	movl	$20, %ecx	/* number of byte */
	xorl	%ebx, %ebx	/* no continuation */
	movl	%edx, %eax	/* "SMAP" */
	popw	%di
	popw	%si
	popfw
	clc
	lret	$2
#endif
3:
	call	dec_e820cycles
2:
	movb	$0x86, %ah	/* function not supported */
	stc
	lret	$2
#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
1:
	cmpw	$0xe801, %ax	//cmpl	$0x0000e801, %eax
	je	1f
	cmpw	$0xe881, %ax	//cmpl	$0x0000e881, %eax
	je	1f
	cmpw	$0xda88, %ax
	je	1f
//	cmpb	$0xc7, %ah
//	je	1f
	cmpb	$0x8a, %ah
	je	1f
	cmpb	$0x88, %ah
	je	1f
99:
	ljmp	%cs:*(EXT_C(ROM_int15) - int13_handler)
#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
1:

	pushal

	/* find minimum mem ever used in the drive map slots */

	movl	%cs:(EXT_C(minimum_mem_hi_in_map) - int13_handler), %eax
	/* set %si to the drive map */
#if	MAP_NUM_16
	movw	$(EXT_C(hooked_drive_map_1) - int13_handler), %si
#else
	movw	$(EXT_C(hooked_drive_map) - int13_handler), %si
#endif
	movw	$(DRIVE_MAP_SIZE), %cx
6:
#if	MAP_NUM_16
	cmpw	$EXT_C(hooked_drive_map_1) - int13_handler + DRIVE_MAP_SIZE * DRIVE_MAP_SLOT_SIZE / 2, %si		//hooked_drive_map_1 end
	jne	8f
	movw	$(EXT_C(hooked_drive_map_2) - int13_handler), %si
8:	
#endif
	cmpb	$0xff, %cs:1(%si)
	jne	5f
	testb	$0x40, %cs:5(%si)	/* TO_C bit 14=(TO is cdrom) */
	jnz	5f

	cmpl	$0, %cs:12(%si)
	jnz	5f			// too big, address exceeding 2048G!

	movl	%cs:8(%si), %ebx	/* StartLBA_Lo */
	testl	$0xFF800000, %ebx
	jnz	5f			// too big, address exceeding 4G!

	shll	$9, %ebx		/* convert to address in bytes */

	/* hi mem */
	cmpl	%ebx, %eax
	jbe	5f
	movl	%ebx, %eax
5:
	/* try next slot */
	addw	$DRIVE_MAP_SLOT_SIZE, %si
	loop	6b

	subl	$0x100000, %eax
	shrl	$10, %eax
	movl	%eax, %cs:(minimum_mem_lo_in_map - int13_handler)
	popal

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
1:
	cmpw	$0xe801, %ax	//cmpl	$0x0000e801, %eax
	jne	1f

	/* Phoenix BIOS v4.0 - GET MEMORY SIZE FOR >64M CONFIGURATIONS */

	//AX = E801h

	//Return:
	//CF clear if successful
	//AX = extended memory between 1M and 16M, in K (max 3C00h = 15MB)
	//BX = extended memory above 16M, in 64K blocks
	//CX = configured memory 1M to 16M, in K
	//DX = configured memory above 16M, in 64K blocks
	//CF set on error

	/* Notes: Supported by the A03 level (6/14/94) and later XPS P90
	 * BIOSes, as well as the Compaq Contura, 3/8/93 DESKPRO/i, and
	 * 7/26/93 LTE Lite 386 ROM BIOS. Supported by AMI BIOSes dated
	 * 8/23/94 or later. On some systems, the BIOS returns AX=BX=0000h;
	 * in this case, use CX and DX instead of AX and BX. This interface
	 * is used by Windows NT 3.1, OS/2 v2.11/2.20, and is used as a
	 * fall-back by newer versions if AX=E820h is not supported. This
	 * function is not used by MS-DOS 6.0 HIMEM.SYS when an EISA machine
	 * (for example with parameter /EISA) (see also MEM F000h:FFD9h), or
	 * no Compaq machine was detected, or parameter /NOABOVE16 was given.
	 */

	/* between 16M and 4G, we modify BX, DX */
4:
	movl	%cs:(minimum_mem_lo_in_map - int13_handler), %ebx
	subl	$0x3c00, %ebx	/* 16M */
	shrl	$6, %ebx	/* AX=mem above 16M in 64K blocks */
	movw	%bx, %dx

	/* between 1M and 16M, we modify AX, CX */

	movw	$0x3c00, %ax
	movw	%ax, %cx

	clc
	lret	$2

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
1:
	cmpw	$0xe881, %ax	//cmpl	$0x0000e881, %eax
	jne	1f

	/* Phoenix BIOS v4.0 - GET MEMORY SIZE FOR >64M CONFIGURATIONS (32-bit) */

	//AX = E881h

	//Return:
	//CF clear if successful
	//EAX = extended memory between 1M and 16M, in K (max 3C00h = 15MB)
	//EBX = extended memory above 16M, in 64K blocks
	//ECX = configured memory 1M to 16M, in K
	//EDX = configured memory above 16M, in 64K blocks
	//CF set on error

	/* Notes: Supported by AMI BIOSes dated 8/23/94 or later. This
	 * interface is used by Windows NT 3.1, OS/2 v2.11/2.20, and is used
	 * as a fall-back by newer versions if AX=E820h is not supported
	 */

	//movl	$0x3c00, %eax	/* 1-16M mem in K (0x3c00 = 15MB) */
	//movl	$0x0300, %ebx	/* mem above 16M in 64K blocks */

	jmp	4b
#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
1:
	cmpw	$0xda88, %ax
	jne	1f

	/* AMI PCI BIOS - GET EXTENDED MEMORY SIZE */

	//AX = DA88h

	//Return:
	//CF clear (successful)
	//AX = 0000h
	//CL:BX = extended memory size in KBytes

	/* 63M = 3f00000 Bytes = 0xfc00 K */
	//movw	$0xfc00, %bx

#;	pushfw
#;	lcall	%cs:*(EXT_C(ROM_int15) - int13_handler)
#;
#;	jc	3f

	pushl	%eax

	movl	%cs:(minimum_mem_lo_in_map - int13_handler), %eax
	xchgw	%ax, %bx
	shrl	$16, %eax
	movb	%al, %cl

	popl	%eax

	xorw	%ax, %ax
	clc
	lret	$2
#;3:
#;	stc
#;	lret	$2

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
#if 0
	/* comment out this function because it uses DS register on call and
	 * thus it cannot call safe_ROM_int15
	 */
1:
	cmpb	$0xC7, %ah
	jne	1f

	/* SYSTEM - later PS/2s - RETURN MEMORY-MAP INFORMATION */

	//AH = C7h
	//DS:SI -> user supplied memory map table

	//Return:
	//CF set on error
	//CF clear if successful

	/* Notes: Call AH=C0h and examine bit 4 of feature byte 2 to check if
	 * this function is supported. IBM classifies this function as
	 * optional. Windows95 OSR2 reportedly does not support this function
	 * even though INT 15/AH=C0h indicates that it is available (observed
	 * on several different systems)
	 */

//Format of memory-map table structure:
//
//Offset Size	Description
//------ -----	--------------------------------------------------------------
//  00h  WORD	length of table (excluding this word)
//  02h  DWORD	local memory between 1M and 16M, in 1K blocks
//  06h  DWORD	local memory between 16M and 4G, in 1K blocks
//  0Ah  DWORD	system memory between 1M and 16M, in 1K blocks
//  0Eh  DWORD	system memory between 16M and 4G, in 1K blocks
//  12h  DWORD	cacheable memory between 1M and 16M, in 1K blocks
//  16h  DWORD	cacheable memory between 16M and 4G, in 1K blocks
//  1Ah  DWORD	1K blocks before start of non-system memory between 1M and 16M
//  1Eh  DWORD	1K blocks before start of non-system memory between 16M and 4G
//  22h  WORD	start segment of largest free block from C0000h-DFFFFh
//  24h  WORD	size of largest free block
//  26h  DWORD	reserved

	//movw	$0x0028, (%si)
	pushw	%ds
	pushw	%si

	pushfw
	lcall	%cs:*(EXT_C(ROM_int15) - int13_handler)

	popw	%si
	popw	%ds

	jc	3f

	pushl	%eax

	/* between 16M and 4G */

	movl	%cs:(minimum_mem_hi_in_map - int13_handler), %eax
	cmpl	$-1, %eax		/* 4G - 1 */
	je	5f
	subl	$0x1000000, %eax	/* 16M */
	shrl	$10, %eax	/* AX=mem above 16M in 1K blocks */
	movl	%eax, 0x0e(%si)
	movl	%eax, 0x1e(%si)
5:

	/* between 1M and 16M */

	movl	%cs:(minimum_mem_lo_in_map - int13_handler), %eax
	cmpl	$0x1000000, %eax	/* 16M */
	je	5f
	subl	$0x0100000, %eax	/* 1M */
	shrl	$10, %eax	/* AX=1-16M mem in K */
	movl	%eax, 0x0a(%si)
	movl	%eax, 0x1a(%si)
5:

	popl	%eax

	clc
	lret	$2
3:
	stc
	lret	$2
#endif
#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
1:
	cmpb	$0x8a, %ah
	jne	1f

	/* Phoenix BIOS v4.0 - GET BIG MEMORY SIZE */

	//AH = 8Ah

	//Return:
	//DX:AX = extended memory size in K
	//movw	$0xfc00, %ax

#;	pushfw
#;	lcall	%cs:*(EXT_C(ROM_int15) - int13_handler)
#;
#;	jc	3f

	movl	%cs:(minimum_mem_lo_in_map - int13_handler), %edx
	movw	%dx, %ax
	shrl	$16, %edx

	clc
	lret	$2
#;3:
#;	stc
#;	lret	$2

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
1:
	cmpb	$0x88, %ah
	jne	1f

	/* SYSTEM - GET EXTENDED MEMORY SIZE (286+) */

	//AH = 88h

	//Return:
	//CF clear on success(Not all BIOSes correctly return the carry flag)
	//AX = number of contiguous KB starting at absolute address 100000h
	//CF set on error
	//AH = status
	//	80h invalid command (PC,PCjr)
	//	86h unsupported function (XT,PS30)
	//movw	$0xfc00, %ax

#;	pushfw
#;	lcall	%cs:*(EXT_C(ROM_int15) - int13_handler)
#;
#;	jc	3f

	movl	%cs:(minimum_mem_lo_in_map - int13_handler), %eax
	cmpl	$0x10000, %eax
	jb	5f
	movw	$0xFFFF, %ax
5:
	clc
	lret	$2
#;3:
#;	stc
#;	lret	$2

1:
	ljmp	%cs:*(EXT_C(ROM_int15) - int13_handler)
	
safe_ROM_int15:

	pushw	%ds
	pushw	$0
	popw	%ds		/* DS = 0 */
	pushl	0x54		/* backup original int 15 vector */
	pushl	%cs:(EXT_C(ROM_int15) - int13_handler)
	popl	0x54		/* install ROM_int15 */
	pushw	%ds		/* DS = 0 */
	int	$0x15
	popw	%ds		/* DS = 0 */
	popl	0x54		/* restore original int 15 vector */
	popw	%ds
	ret

dec_e820cycles:
	/* if e820cycles = -1, do nothing, else decrement */
	pushal
	movl	%cs:(EXT_C(e820cycles) - int13_handler), %ecx
	incl	%ecx
	jz	1f
	decl	%ecx
	jz	2f
	decl	%ecx
	jnz	3f
2:
	/* unhook int15 */
	pushw	%ds
	movw	%cx, %ds	/* DS = 0 */
	pushl	%cs:(EXT_C(ROM_int15) - int13_handler)
	popl	0x54		/* install ROM_int15 */
	popw	%ds
3:
	movl	%ecx, %cs:(EXT_C(e820cycles) - int13_handler)
1:
	popal
	ret

#;============================================================================

/****************************************************************************/

#include "a20.inc"

/****************************************************************************/

	.align	16

GDT_data:		/* used by int15/ah=87h */

/*00*/	.long	0, 0	/* unused descriptor, must be 0 */
/*08*/	.long	0, 0	/* GDT descriptor, must be 0 */

	/* source descriptor */
/*10*/	.word	0xffff	/* segment limit, 64K */
/*12*/	.byte	0, 0, 0	/* physical address(low 24 bits) */
/*15*/	.byte	0x93	/* access rights, 93h=readable/writable */
/*16*/	.byte	0	/* low 4 bits are high 4 bits of segment limit(here
			   should be 0). high 4 bits are flags, also should
			   be set to 0 for this function call */
/*17*/	.byte	0	/* physical address(high 8 bits) */

	/* destination descriptor */
/*18*/	.word	0xffff	/* segment limit, 64K */
/*1A*/	.byte	0, 0, 0	/* physical address(low 24 bits) */
/*1D*/	.byte	0x93	/* access rights, 93h=readable/writable */
/*1E*/	.byte	0	/* low 4 bits are high 4 bits of segment limit(here
			   should be 0). high 4 bits are flags, also should
			   be set to 0 for this function call */
/*1F*/	.byte	0	/* physical address(high 8 bits) */

/*20*/	.long	0, 0	/* code segment descriptor, must be 0 */
/*28*/	.long	0, 0	/* stack segment descriptor, must be 0 */
/*30*/



	.align	16

/* EBIOS_disk_address_packet should be at the end of the handler because some
 * buggy BIOSes could destroy the memory that immediately follows.
 */	
EBIOS_disk_address_packet:
	.byte	0x10	/* packet size, 16 or more */
	.byte	0	/* reserved, must be 0 */
	.byte	0	/* number of sectors, must be from 1 to 127 */
	.byte	0	/* reserved, must be 0 */
	.word	0	/* displacement of memory address */
	.word	0	/* segment of memory address */
	.long	0	/* 64bit, start logical sector number */
	.long	0	

edd30_disk_buffer:	.space	0x800

#if	MAP_NUM_16
ENTRY(hooked_drive_map_2)
	.space	(DRIVE_MAP_SIZE / 2 + 1) * DRIVE_MAP_SLOT_SIZE
#endif

//	.ascii	"FRAGMENT"
	
//ENTRY(hooked_fragment_map)	.space	FRAGMENT_MAP_SLOT_SIZE

	.ascii	"int13end"

cdrom_start:
#ifdef CDROM_INIT
/* EDD30 code imported from edd30.asm of Smart Boot Manager. */

/*
#; asmsyntax=nasm
#;
#; CD-ROM Boot Extension v 1.1 for Smart Boot Manager
#;
#; Copyright (C) 2000, Christopher Li <chrisl@gnuchina.org>.
#; Copyright (C) 2000, James Su <suzhe@gnuchina.org>
#;
#; This is free software, you can redistribute it and/or modify it
#; under the terms of the GNU General Public License version 2 or above.
#;
#; The ATAPI driver is based on the source code of atadrv written by
#; Hale Landis <hlandis@ibm.net>, Thanks him a lot!
#;
#; Without his great program, we could not implement the CD-ROM Boot feature
#; so quickly.
#;
*/


//#define	EDD_3_0

#define	sane_check
#define	check_extra_fail

//#define CB_DATA	0	//; data reg         in/out pio_base_addr1+0
//#define CB_FR		1	//; feature reg         out pio_base_addr1+1
//#define CB_SC		2	//; sector count     in/out pio_base_addr1+2
//#define CB_SN		3	//; sector number    in/out pio_base_addr1+3
//#define CB_CL		4	//; cylinder low     in/out pio_base_addr1+4
//#define CB_CH		5	//; cylinder high    in/out pio_base_addr1+5
//#define CB_DH		6	//; device head      in/out pio_base_addr1+6
//#define CB_STAT	7	//; primary status   in     pio_base_addr1+7
//#define CB_CMD	7	//; command             out pio_base_addr1+7
//#define CB_DC		8	//; device control      out pio_base_addr2+6
//#define CB_ASTAT	8	//; alternate status in     pio_base_addr2+6
//#define CB_DC_SRST	0x04	//; soft reset
#define	CB_STAT_BSY	0x80	//; busy
#define	CB_STAT_RDY	0x40	//; ready
#define	CB_STAT_SKC	0x10	//; seek complete

#define	CB_STAT_DRQ	0x08	//; data request
#define	CB_STAT_ERR	0x01	//; error

#define	CB_SC_P_TAG	0xf8	//; ATAPI tag (mask)
#define	CB_SC_P_REL	0x04	//; ATAPI release
#define	CB_SC_P_IO	0x02	//; ATAPI I/O
#define	CB_SC_P_CD	0x01	//; ATAPI C/D

#define	FAILBIT8	0x0100	//; SC( CD/IO bits) wrong at end of cmd
#define	FAILBIT6	0x0040	//; byte count wrong at data packet xfer time
#define	FAILBIT5	0x0020	//; SC (IO bit) wrong at data packet xfer time
#define	FAILBIT4	0x0010	//; SC (CD bit) wrong at data packet xfer time
#define	FAILBIT3	0x0008	//; byte count wrong at cmd packet xfer time
#define	FAILBIT2	0x0004	//; SC wrong at cmd packet xfer time
#define	FAILBIT0	0x0001	//; slow setting BSY=1 or DRQ=1 after AO cmd

#define	CB_DC_HD15	0x08	//; bit should always be set to one
#define	CB_DC_NIEN	0x02	//; disable interrupts
#define	cmd_DC	CB_DC_HD15
#define cmd_DC_ni CB_DC_HD15 | CB_DC_NIEN

/****************************************************************************/
			
__reg_select_dev:
#; ax = dev
#; simple version of the select dev
	addb	$0x0A, %al	#; AL= 0x0A or 0x0B
	shlb	$4, %al		#; AL= 0xA0 or 0xB0
	movw	reg_addr - int13_handler + (6 * 2), %dx	#; CB_DH = drive select
	outb	%al, %dx

	call	delay400ns
	ret


reg_select_dev:
#; input: ax = bx = dev
#; set_timeout first
//	movw	%ax, %bx
	cmpb	$2, (reg_dev_info - int13_handler)(%bx)	# REG_CONFIG_TYPE_ATA
	jb	__reg_select_dev	# not ATA

	/* The device is ATA */

//	pushw	%bx

	call	reg_poll_busy		# change AL, DX

#if 0
	orw	%ax, %ax
	jnz	1f	# return if,{or ax,ax},nz
#else
	ja	1f	# timeout, failure
#endif

	movw	%bx, %ax
	call	__reg_select_dev	# change AL, DX

#ifndef USE_ATA
	call	reg_poll_busy		# change AL, DX
#if 0
	jmp	1f
#else
#if 0
	orw	%ax, %ax
	jz	1f
#else
	jna	1f
#endif
#endif
#else	//; USE_ATA
3:
	movw	reg_addr - int13_handler + (7 * 2), %dx		#; CB_STAT
	inb	%dx, %al

	cmpb	$2, (reg_dev_info - int13_handler)(%bx)	# REG_CONFIG_TYPE_ATA
	jne	4f
	andb	$(CB_STAT_BSY | CB_STAT_RDY | CB_STAT_SKC), %al
	# return if,{cmp al,CB_STAT_RDY|CB_STAT_SKC},e
	cmpb	$(CB_STAT_RDY | CB_STAT_SKC), %al
	je	1f
	jmp	3b
4:
	# return if,{test al,CB_STAT_BSY},z
	testb	$CB_STAT_BSY, %al
	jz	1f
	jmp	3b
#endif	//; USE_ATA


	call	__reg_select_dev
1:
//	popw	%bx
	ret


select_atapi:
	cmpw	%cs:(atapi_cur_dev - int13_handler), %ax	# device serial number
	jne	select_atapi_force
	//clc	#; CF already cleared
	ret


select_atapi_force:
#; input: ax = device serial number
#; return: cf =0 success, cf =1 failed
	pushaw
	pushw	%es

	pushw	%cs
	popw	%es

	cmpb	%cs:(EXT_C(atapi_dev_count) - int13_handler), %al
	cmc
	jb	2f

	movw	%ax, %cs:(atapi_cur_dev - int13_handler)	# device serial number

	movw	$atapi_dev_base - int13_handler, %si	# array of ATAPI reg pointer and dev
	shlw	$2, %ax					# each element of array has 4 bytes
	addw	%ax, %si
	movw	%cs:(%si), %bx				# BX is the reg pointer of the current device

	call	reg_setup_base_addr			# fill the 10-word reg_addr area
							# change no registers

	movw	%cs:2(%si), %bx				# BX is the dev number of the current device
	movw	%bx, %ax

	call	set_timeout				# set timeout for reg_select_dev
							# change no registers

	call	reg_select_dev

	addb	$0x0A, %bl		#; BL= 0x0A or 0x0B
	shlb	$4, %bl			#; BL= 0xA0 or 0xB0
	movb	%bl, %cs:(reg_cur_dev - int13_handler)

	clc
2:
	pop	%es
	popaw
	ret


read_atapi:
#;input: es:di -> buffer, cx = sector count, edx = lba address
#;return: cf =0 success, cx = number of bytes actually read
	pushaw
	orw	%cx, %cx
	jz	3f

1:
	pushw	%cx
	pushl	%edx

	call	clear_atapi_buffer
	movb	$0x28, atapi_cmd_buffer - int13_handler

	bswapl	%edx
	movl	%edx, atapi_cmd_buffer - int13_handler + 2
	movb	$1, atapi_cmd_buffer - int13_handler + 8

	# invoke reg_packet,byte, 0, es, di, REG_ATAPI_MAX_BYTES
	pushw	$0x8000			#;   REG_ATAPI_MAX_BYTES
	pushw	%di
	pushw	%es
	pushw	$0
	call	reg_packet
	addw	$8, %sp

	orw	%ax, %ax
	jnz	2f
	cmpw	$0x800, %cx		#; CDSECTOR_SIZE
	jne	2f
	popl	%edx
	incl	%edx
	addw	%cx, %di
	popw	%cx
	loop	1b

	clc
	jmp	4f

2:
	popl	%edx
	popw	%cx

3:
	stc
4:
	popaw
	ret


read_bios_time:
#;	return EAX as the long time
	pushw	%ds
	xorw	%ax, %ax
	movw	%ax, %ds
	movl	0x46c, %eax
	popw	%ds
	ret


edd30_read_cdrom:
#;return cf=0 success, cf=1 fail, ah = fail code
#;will change all registers, including ES!!

	call	test_atapi_ready
	movb	$0xAA, %ah	# error code if CF=1
	jc	4f

	movw	%ds, %ax	# save DS

	# get DS for the disk address packet
	movw	%cs:(int13_old_ds - int13_handler), %ds
	movw	2(%si), %cx	# struc_int13ext.blk_count
	movw	4(%si), %di	# struc_int13ext.buf_addr_off
	movw	6(%si), %bx	# struc_int13ext.buf_addr_seg
	movl	8(%si), %edx	# struc_int13ext.blk_num_low1
	cmpb	$16, (%si)	# valid packet size is 16

	movw	%ax, %ds	# restore DS

	setne	%ah		# AH=1 for invalid function call
	stc			# error out if ...
	jne	4f		# ... invalid packet size

	movw	%bx, %es	# ES changed!!

	call	read_atapi

	movb	$0x0C, %ah	# error code if CF=1
4:
//	/* debug print AX and FLAGS */
//	pushfw
//	pushw	%ax
//	pushw	$read_cdrom_ah_flags - int13_handler	# the format string
//	call	realmode_printf
//	popw	%ax
//	popw	%ax
//	popfw
	ret

//read_cdrom_ah_flags:
//	.ascii	"edd30_read_cdrom: AX=%X, FLAGS=%X\r\n\0"


reg_reset:
#; call after reg_probe_exist
	pushaw

#;	mov_ax 0
#;	call __reg_select_dev

#if 0
	/* The ATA software reset mechanism, SRST, (bit 2 in the Device Control
	 * Register) cannot be used for ATAPI Device, because resets issued by
	 * the ATAPI driver would also reset any attached hard disk and vice
	 * versa. To solve this, ATAPI defines an ATAPI Soft Reset command
	 * using a reserved ATA opcode which could be decoded by the interface
	 * controller hardware.
	 */

	movb	$cmd_DC, %al
	orb	$0x04, %al	#; CB_DC_SRST = soft reset

	movw	reg_addr - int13_handler + (8 * 2), %dx		#; CB_DC
	outb	%al, %dx

	call	delay400ns
#endif

#if 0
	movb	$0x08, %al		#; ATAPI Soft Reset
	movw	reg_addr - int13_handler + (7 * 2), %dx		#; CB_CMD
	outb	%al, %dx

	call	delay400ns
#endif

#if 1
	/* set features (0xEF to command port) */
	/* set transfer mode (0x03 in feature register) */
	/* mode value in Sector Count register (0 for PIO default mode) */

	movb	$0, %al		#; PIO default transfer mode
	movw	reg_addr - int13_handler + (2 * 2), %dx	#; CB_SC = interrupt reason register
	outb	%al, %dx

	call	delay400ns

	movb	$0x03, %al	#; 0x03 = set transfer mode based on value in sector count register
	movw	reg_addr - int13_handler + (1 * 2), %dx	#; CB_FR = Feature Register
	outb	%al, %dx

	call	delay400ns

	movb	$0xEF, %al		#; Set Features Command
	movw	reg_addr - int13_handler + (7 * 2), %dx		#; CB_CMD
	outb	%al, %dx

	call	delay400ns
#endif

#if 1
	movb	$0x08, %al		#; ATAPI Soft Reset
	movw	reg_addr - int13_handler + (7 * 2), %dx		#; CB_CMD
	outb	%al, %dx

	call	delay400ns
#endif

#; Test only: do a hard reset and see if it works
#if 1
	/* The ATA software reset mechanism, SRST, (bit 2 in the Device Control
	 * Register) cannot be used for ATAPI Device, because resets issued by
	 * the ATAPI driver would also reset any attached hard disk and vice
	 * versa. To solve this, ATAPI defines an ATAPI Soft Reset command
	 * using a reserved ATA opcode which could be decoded by the interface
	 * controller hardware.
	 */

	movb	$cmd_DC, %al
	orb	$0x04, %al	#; CB_DC_SRST = soft reset

	movw	reg_addr - int13_handler + (8 * 2), %dx		#; CB_DC
	outb	%al, %dx

	call	delay400ns
#endif

	movb	$cmd_DC, %al
	movw	reg_addr - int13_handler + (8 * 2), %dx		#; CB_DC
	outb	%al, %dx

	call	delay400ns

	call	set_timeout

	cmpb	$0, reg_dev_info - int13_handler
	je	1f			#; master not exist

	call	sub_atapi_delay

	call	reg_poll_busy

1:
	cmpb	$0, reg_dev_info - int13_handler + 1
	je	1f			#; slave not exist

	call	sub_atapi_delay

	//call	reg_poll_busy		#; added recently for test

	//jmp	1f			#; added recently for test

2:
	movw	$1, %ax

	call	__reg_select_dev

	movw	reg_addr - int13_handler + (2 * 2), %dx	#; CB_SC = interrupt reason register
	inb	%dx, %al
	movb	%al, %ah

	movw	reg_addr - int13_handler + (3 * 2), %dx	#; CB_SN = reserved for SAM TAG byte
	inb	%dx, %al

	cmpw	$0x0101, %ax
	je	1f

	call	check_timeout
	jna	2b
1:
	popaw
	ret


test_atapi_ready:
#; return: cf =0 ready, cf =1 not ready
	pushaw
	movw	$2, %cx

1:
	pushw	%cx

	call	clear_atapi_buffer

	#; TEST UNIT READY Command
	#;
	#; The TEST UNIT READY command provides a means to check if the Device
	#; is ready. This is not a request for a self-test. If the Device would
	#; accept an appropriate medium-access command without returning CHECK
	#; CONDITION status, this command shall return a GOOD status. If the
	#; Device cannot become operational or is in a state such that a Host
	#; Computer action(e.g. START/STOP UNIT command with LoEj = 0 and Start
	#; = 1) is required to make the unit ready, the ATAPI CD-ROM Drive
	#; shall return CHECK CONDITION status with a sense key of NOT READY.
	#;
	#; Byte
	#;   0			Operation code (00h)
	#;   1			Reserved
	#;   2			Reserved
	#;   3			Reserved
	#;   4			Reserved
	#;   5			Reserved
	#;   6			Reserved
	#;   7			Reserved
	#;   8			Reserved
	#;   9			Reserved
	#;  10			Reserved
	#;  11			Reserved
	#;
	#; Using the TEST UNIT READY Command
	#;
	#; The TEST UNIT READY command is useful in that it allows a Host
	#; Computer to poll a Device until it is ready without the need to
	#; allocate space for returned data. It is especially useful to check
	#; cartridge status. ATAPI CD-ROM Drives are expected to respond
	#; promptly to indicate the current status of the device.

	# invoke reg_packet,byte,0,cs, atapi_tmp_buffer, 128
	pushw	$128
	pushw	$atapi_tmp_buffer - int13_handler
	pushw	%cs
	pushw	$0
	call	reg_packet
	addw	$8, %sp

//	/* debug print AX */
//	pushw	%ax
//	pushw	$test_atapi_ax - int13_handler	# the format string
//	call	realmode_printf
//	popw	%ax
//	popw	%ax

	orw	%ax, %ax
	jnz	2f

	call	get_atapi_sense

//	/* debug print AL, FLAGS */
//	pushfw
//	pushw	%ax
//	pushw	$test_atapi_al_flags - int13_handler	# the format string
//	call	realmode_printf
//	popw	%ax
//	popw	%ax
//	popfw

	jc	2f
	orb	%al, %al
	jnz	2f

	popw	%cx
	clc
	popaw
	ret
2:
	call	start_stop_unit		#; start unit

	popw	%cx
	loop	1b			#; try again

	stc
	popaw
	ret

//test_atapi_ax:
//	.ascii	"test_atapi_ax: AX=%X\r\n\0"

//test_atapi_al_flags:
//	.ascii	"test_atapi_al_flags: AX=%X, FLAGS=%X\r\n\0"


start_stop_unit:
#; return: cf =0 ready, cf =1 not ready
	pushaw
	movw	$1, %cx

1:
	pushw	%cx

	call	clear_atapi_buffer

	#; START/STOP UNIT Command
	#;
	#; The START/STOP UNIT command requests that the ATAPI CD-ROM Drive
	#; enable or disable media access operations.
	#;
	#;      |  bit     bit     bit     bit     bit     bit     bit     bit
	#; Byte |   7       6       5       4       3       2       1       0
	#; -----+--------------------------------------------------------------
	#;   0  |                    Operation code (1Bh)
	#; -----+-------------------------------------------------------+------
	#;   1  |                   Reserved                            | Immed
	#; -----+-------------------------------------------------------+------
	#;   2  |                        Reserved
	#; -----+--------------------------------------------------------------
	#;   3  |                        Reserved
	#; -----+-----------------------------------------------+-------+------
	#;   4  |                Reserved                       | LoEj  | Start
	#; -----+-----------------------------------------------+-------+------
	#;   5  |
	#; -----+--------------------------------------------------------------
	#;   6  |                        Reserved
	#; -----+--------------------------------------------------------------
	#;   7  |                        Reserved
	#; -----+--------------------------------------------------------------
	#;   8  |                        Reserved
	#; -----+--------------------------------------------------------------
	#;   9  |                        Reserved
	#; -----+--------------------------------------------------------------
	#;  10  |                        Reserved
	#; -----+--------------------------------------------------------------
	#;  11  |                        Reserved
	#; -----+--------------------------------------------------------------

	movb	$0x1B, atapi_cmd_buffer - int13_handler
	movb	$0x01, atapi_cmd_buffer - int13_handler + 4	#; start unit

	# invoke reg_packet,byte,0,cs, atapi_tmp_buffer, 128
	pushw	$128
	pushw	$atapi_tmp_buffer - int13_handler
	pushw	%cs
	pushw	$0
	call	reg_packet
	addw	$8, %sp

	orw	%ax, %ax
	jnz	2f

//	call	get_atapi_sense
//
//	jc	2f
//	orb	%al, %al
//	jnz	2f

	popw	%cx
	clc
	popaw
	ret
2:
	popw	%cx
	loop	1b

	stc
	popaw
	ret




reg_setup_base_addr:
#; input:  bx = base addr pointer
	pushw	%ax
	pushw	%di
	cld			# store upward
	movw	$reg_addr - int13_handler, %di	# points to 10-word space
	movw	%cs:(%bx), %ax	# get base address
	# store 8 ports(command block registers)

#if 0
	pushw	18(%di)
	pushw	16(%di)
	pushw	14(%di)
	pushw	12(%di)
	pushw	10(%di)
	pushw	8(%di)
	pushw	6(%di)
	pushw	4(%di)
	pushw	2(%di)
	pushw	(%di)
	pushw	2(%bx)
	pushw	%ax
	pushw	$reg_addresses - int13_handler		# the format string
	call	realmode_printf
	addw	$26, %sp			# adjust the stack pointer
#endif

	stosw
	incw	%ax
	stosw
	incw	%ax
	stosw
	incw	%ax
	stosw
	incw	%ax
	stosw
	incw	%ax
	stosw
	incw	%ax
	stosw
	incw	%ax
	stosw

	movw	%cs:2(%bx), %ax		# get base address
	stosw				# store 2 ports(control block registers)
	incw	%ax
	stosw
	popw	%di
	popw	%ax
	ret

#if 0
reg_addresses:
	.ascii	"reg_addresses: Data=%X, Ctrl=%X, di: %X, %X, %X, %X, %X, %X, %X, %X, %X, %X\r\n\0"
#endif

set_timeout:
	pushl	%eax
	call	read_bios_time
	addl	$(5 * 18), %eax	#; 20 seconds
				# not 20 seconds, but 5 seconds
	 			#; FIXME: Midnight overflow
	 			# Midnight overflow is handled in check_timeout
	movl	%eax, %cs:(time_out - int13_handler)
	popl	%eax
	ret


check_timeout:
#; you need to setup the timeout first
	pushl	%eax
	pushl	%ebx
	pushl	%edx
	call	read_bios_time
	movl	time_out - int13_handler, %ebx
	movl	%ebx, %edx
	subl	$(5 * 18), %ebx		# the time when we set_timeout
	cmpl	%ebx, %eax
	jnb	1f
	/* the next day */
	addl	$0x001800B0, %eax	/* 24 hours = 0x1800B0 ticks */
1:
	cmpl	%edx, %eax
	popl	%edx
	popl	%ebx
	popl	%eax
	ret


clear_atapi_buffer:
	pushaw
	pushw	%es
	pushw	%cs
	popw	%es
	cld
	movw	$atapi_tmp_buffer - int13_handler, %di
	movw	$128, %cx	# but note the buffer is 256-byte long!
	xorb	%al, %al
	rep stosb		# added the lost rep prefix
	movw	$atapi_cmd_buffer - int13_handler, %di
	movw	$16, %cx
	rep stosb		# added the lost rep prefix
	popw	%es
	popaw
	ret


reg_packet:
#;proc	reg_packet,withlocal,dir,packet_seg,packet_off,packet_len
#; input:
#; return ax = 0 noerror, ah = error code al= error bit
#;	 cx = len
	pushaw

	movw	%sp, %bp

	call	set_timeout
	movb	$0, _pre_fail_bit7_  - int13_handler

#;	outbytes CB_DC,cmd_DC_ni,CB_FR,0,CB_SC,0,CB_SN,0,
	# outbytes CB_DC,cmd_DC,CB_FR,0,CB_SC,0,CB_SN,0,
	movb	$cmd_DC, %al
	movw	reg_addr - int13_handler + (8 * 2), %dx	#; CB_DC
	outb	%al, %dx

	call	delay400ns	#; added recently

	call	sub_atapi_delay	#; added recently

	movb	$0, %al
	movw	reg_addr - int13_handler + (1 * 2), %dx	#; CB_FR = Feature Register
	outb	%al, %dx

	movb	$0, %al
	movw	reg_addr - int13_handler + (2 * 2), %dx	#; CB_SC = interrupt reason register
	outb	%al, %dx

	movb	$0, %al
	movw	reg_addr - int13_handler + (3 * 2), %dx	#; CB_SN = reserved for SAM TAG byte
	outb	%al, %dx


	# outbytes CB_CL,[.packet_len],CB_CH,[.packet_len+1]
	movb	24(%bp), %al		#; packet_len low byte
	movw	reg_addr - int13_handler + (4 * 2), %dx	#; CB_CL = byte count low
	outb	%al, %dx

	movb	25(%bp), %al		#; packet_len high byte
	movw	reg_addr - int13_handler + (5 * 2), %dx	#; CB_CH = byte count high
	outb	%al, %dx

	# outbyte CB_DH,[reg_cur_dev]
	movb	reg_cur_dev - int13_handler, %al
	movw	reg_addr - int13_handler + (6 * 2), %dx	#; CB_DH = drive select
	outb	%al, %dx

	# outbyte CB_CMD,CMD_PACKET
	movb	$0xA0, %al		#; CMD_PACKET
	movw	reg_addr - int13_handler + (7 * 2), %dx		#; CB_CMD
	outb	%al, %dx

	call	delay400ns

	call	sub_atapi_delay

	subw	%bx, %bx		#; clear error number

	# while {inbyte CB_ASTAT},{test al,CB_STAT_BSY|CB_STAT_ERR|CB_STAT_DRQ},z
3:
	# inbyte CB_ASTAT
	movw	reg_addr - int13_handler + (8 * 2), %dx		#; CB_ASTAT=8, CB_STAT=7
	inb	%dx, %al

	testb	$(CB_STAT_BSY | CB_STAT_DRQ | CB_STAT_ERR), %al
	jnz	3f

	orb	$FAILBIT0, %bl		#; error
	call	check_timeout
	jna	3b
3:
	# endwhile

	# while {inbyte CB_ASTAT},{test al, CB_STAT_BSY}, nz
3:
	# inbyte CB_ASTAT
	movw	reg_addr - int13_handler + (8 * 2), %dx		#; CB_ASTAT=8, CB_STAT=7
	inb	%dx, %al

	testb	$CB_STAT_BSY, %al
	jz	3f

	call	check_timeout
	jna	3b
	movw	$-1, 18(%bp)
	movb	$51, %bh		#; error
3:
	# endwhile

	cmpb	$0, %bh
	jnz	2f			#; error

	# inbyte CB_STAT,[.status]
	movw	reg_addr - int13_handler + (7 * 2), %dx		#; CB_STAT
	inb	%dx, %al
	movb	%al, _status_ - int13_handler

	# inbyte CB_SC, [.reason]
	movw	reg_addr - int13_handler + (2 * 2), %dx	#; CB_SC = interrupt reason register
	inb	%dx, %al
	movb	%al, _reason_ - int13_handler

	# inbyte CB_CL, [.bcnt]
	movw	reg_addr - int13_handler + (4 * 2), %dx	#; CB_CL = byte count low
	inb	%dx, %al
	movb	%al, _bcnt_ - int13_handler

	# inbyte CB_CH, [.bcnt+1]
	movw	reg_addr - int13_handler + (5 * 2), %dx	#; CB_CH = byte count high
	inb	%dx, %al
	movb	%al, _bcnt_ - int13_handler + 1

	movb	_status_ - int13_handler, %al
	andb	$(CB_STAT_BSY | CB_STAT_DRQ), %al
	cmpb	$CB_STAT_DRQ, %al
	jz	3f
	movb	$52, %bh		#; error

	jmp	2f
3:

#ifdef sane_check
	movb	_reason_ - int13_handler, %al
	testb	$(CB_SC_P_TAG | CB_SC_P_REL | CB_SC_P_IO), %al
	jnz	3f
	testb	$CB_SC_P_CD, %al
	jnz	4f
3:
	orb	$FAILBIT2, %bl		#; error
4:
	movw	_bcnt_ - int13_handler, %ax
	cmpw	24(%bp), %ax
	jz	3f
	orb	$FAILBIT3, %bl		#; error

3:

#endif	//; sane_check

	movw	$atapi_cmd_buffer - int13_handler, %si
#if 0
	movw	$12, %cx		#; cmd_buff_len is 12
	shrw	$1, %cx
#else
	movw	$6, %cx			#; cmd_buff_len is 12
#endif
	movw	reg_addr - int13_handler + (0 * 2), %dx		#; CB_DATA = 16-bit data port
	cld
	repz outsw

	call	delay400ns

	subw	%cx, %cx
3:
	# while

	call	sub_atapi_delay
4:
	# while {inbyte CB_ASTAT},{test al,CB_STAT_BSY},nz
	# inbyte CB_ASTAT
	movw	reg_addr - int13_handler + (8 * 2), %dx		#; CB_ASTAT=8, CB_STAT=7
	inb	%dx, %al

	testb	$CB_STAT_BSY, %al
	jz	4f
	call	check_timeout
	jna	4b
	movb	$54, %bh		#; error
	jmp	2f
	# endwhile
4:

	#; Data transfer loop
	#; read the primary state register

	# inbyte CB_STAT,[.status]
	movw	reg_addr - int13_handler + (7 * 2), %dx		#; CB_STAT
	inb	%dx, %al
	movb	%al, _status_ - int13_handler

	# inbyte CB_SC, [.reason]
	movw	reg_addr - int13_handler + (2 * 2), %dx	#; CB_SC = interrupt reason register
	inb	%dx, %al
	movb	%al, _reason_ - int13_handler

	# inbyte CB_CL, [.bcnt]
	movw	reg_addr - int13_handler + (4 * 2), %dx	#; CB_CL = byte count low
	inb	%dx, %al
	movb	%al, _bcnt_ - int13_handler

	# inbyte CB_CH, [.bcnt+1]
	movw	reg_addr - int13_handler + (5 * 2), %dx	#; CB_CH = byte count high
	inb	%dx, %al
	movb	%al, _bcnt_ - int13_handler + 1

	testb	$(CB_STAT_BSY | CB_STAT_DRQ), _status_ - int13_handler
	jnz	4f
	orb	$0x80, 18(%bp)		#; NON_DATA
	jmp	2f
4:
	movb	_status_ - int13_handler, %al
	andb	$(CB_STAT_BSY | CB_STAT_DRQ), %al
	cmpb	$CB_STAT_DRQ, %al
	jz	4f
	movb	$55, %bh		#; error
	jmp	2f
4:

#ifdef sane_check
	testb	$(CB_SC_P_TAG | CB_SC_P_REL | CB_SC_P_CD), _reason_ - int13_handler
	jz	4f
	orb	$FAILBIT4, %bl		#; error
4:
	testb	$CB_SC_P_IO, _reason_ - int13_handler
	jz	4f
	cmpb	$0, 18(%bp)
	jz	4f
	orb	$FAILBIT5, %bl		#; error
4:
#endif	//; sane_check

	#; do the slow data transfer

	call	sub_atapi_delay

	movw	_bcnt_ - int13_handler, %ax
	orw	%ax, %ax
	jnz	4f
	movb	$60, %bh		#; error
	orb	$0x80, 18(%bp)		#; NON_DATA
	jmp	2f
4:

#ifdef sane_check
	cmpw	$0x8000, %ax		#; REG_ATAPI_MAX_BYTES
	jna	4f
	orb	$FAILBIT6, %bl		#; error
4:
#endif	//; sane_check
	movb	_pre_fail_bit7_ - int13_handler, %dl
	orb	%dl, %bl		#; error
	testb	$1, %al
	setnz	_pre_fail_bit7_ - int13_handler

	movw	%cx, %dx
	addw	%ax, %dx
	cmpw	24(%bp), %dx
	jna	4f
	movb	$59, %bh		#; error
	orb	$0x80, 18(%bp)		#; NON_DATA
	jmp	2f
4:

	pushw	%dx
	movw	%ax, %cx
	incw	%cx
	shrw	$1, %cx
	movw	reg_addr - int13_handler + (0 * 2), %dx		#; CB_DATA = 16-bit data port
	cld
	cmpb	$0, 18(%bp)
	jz	4f
	pushw	%ds
	movw	20(%bp), %ds		#; packet_seg
	movw	22(%bp), %si		#; packet_off
	repz outsw
	popw	%ds
	jmp	5f
4:
	pushw	%es
	movw	20(%bp), %es		#; packet_seg
	movw	22(%bp), %di		#; packet_off
	repz insw
	popw	%es
5:
	popw	%cx

	addw	%ax, 22(%bp)		#; packet_off
	call	delay400ns
	jmp	3b
	# endwhile
3:
	testb	$0x80, 18(%bp)		#; NON_DATA
	jnz	3f
	call	sub_atapi_delay

	call	check_timeout		# !!added recently!!
	jna	3f

	movb	$57, %bh		#; error: time is out
	jmp	2f
3:

	# inbyte CB_STAT, #; [.status]
	movw	reg_addr - int13_handler + (7 * 2), %dx		#; CB_STAT
	inb	%dx, %al

	testb	$(CB_STAT_BSY | CB_STAT_DRQ | CB_STAT_ERR), %al
	jz	1f

	movb	$58, %bh		#; error
	jmp	2f
1:
	# inbyte CB_SC, [.reason]
	movw	reg_addr - int13_handler + (2 * 2), %dx	#; CB_SC = interrupt reason register
	inb	%dx, %al
	movb	%al, _reason_ - int13_handler
2:

#ifdef check_extra_fail
	movb	_reason_ - int13_handler, %al
	testb	$(CB_SC_P_TAG | CB_SC_P_REL), %al
	jnz	1f
	testb	$CB_SC_P_IO, %al
	jz	1f
	testb	$CB_SC_P_CD, %al
	jnz	2f
1:
	orw	$FAILBIT8, %bx		#; error
2:
#endif	//; check_extra_fail

#;	outbyte CB_DC,cmd_DC

	movw	%bx, 14(%bp)	# __AX	#; error
	movw	%cx, 12(%bp)	# __CX

	popaw
	ret

#	local status,1,reason,1,bcnt,2,pre_fail_bit7,1
_status_:		.byte	0
_reason_:		.byte	0
_bcnt_:			.byte	0, 0
_pre_fail_bit7_:	.byte	0
# //; end of function reg_packet


delay400ns:

	pushw	%cx
	movw	$0x0040, %cx
1:
	pause
	loop	1b
	popw	%cx
	ret

	# inbyte CB_ASTAT
	movw	reg_addr - int13_handler + (8 * 2), %dx		#; CB_ASTAT=8, CB_STAT=7
	inb	%dx, %al
	inb	%dx, %al
	inb	%dx, %al
	inb	%dx, %al
	ret


sub_atapi_delay:
#; delay a few clicks
	cmpl	$0, EXT_C(slow_atapi_device) - int13_handler
	jz	1f

	# delay 1 millisecond

	pushl	%ecx
	//pushfw

	//sti		#; sti should already be done by the caller

	movl	delay_repeat_num - int13_handler, %ecx	# loops per millisecond
2:
	call	read_bios_time
	cmpw	%ax, %ax	# for more accurate timing
	addr32 loope	2b

	//popfw
	popl	%ecx

1:
	ret


reg_poll_busy:
#; need to setup the timeout first
#;return ax=0	ok
#;	ax = 1	timeout

1:
	movw	reg_addr - int13_handler + (7 * 2), %dx		#; CB_STAT
	inb	%dx, %al

	andb	$0x80, %al	# CB_STAT_BSY
	jz	1f
	call	check_timeout
	jna	1b
	# timeout
1:
	ret


get_atapi_sense:
#; return: cf =0 success, al = sense key, bl = asc, bh = ascq
#;	  cf =1 failed

	call	clear_atapi_buffer

	#; REQUEST SENSE Command
	#;
	#; The REQUEST SENSE command requests that the ATAPI CD-ROM Drive
	#; transfer sense data to the Host Computer.
	#;
	#; Byte
	#;   0			Operation code (03h)
	#;   1			Reserved
	#;   2			Reserved
	#;   3			Reserved
	#;   4			Allocation Length
	#;   5			Reserved
	#;   6			Reserved
	#;   7			Reserved
	#;   8			Reserved
	#;   9			Reserved
	#;  10			Reserved
	#;  11			Reserved

	movb	$0x03, atapi_cmd_buffer - int13_handler
	movb	$32, atapi_cmd_buffer - int13_handler + 4

	# invoke reg_packet,byte,0,cs, atapi_tmp_buffer, 128
	pushw	$128
	pushw	$atapi_tmp_buffer - int13_handler
	pushw	%cs
	pushw	$0
	call	reg_packet
	addw	$8, %sp

	orw	%ax, %ax
	jnz	1f

	movb	atapi_tmp_buffer - int13_handler, %al
	/* bit 7 is `Valid' bit, and should be 1
	 * bit 0-6 is the error code and can be 70h or 71h
	 */
	andb	$0x7f, %al
	cmpb	$0x70, %al
	je	2f
	cmpb	$0x71, %al
	jne	1f
2:
	movb	atapi_tmp_buffer - int13_handler + 2, %al	#; get sense key
	andb	$0x0f, %al	#; low 4 bits are sense key
	xorw	%bx, %bx
	cmpb	$0x06, atapi_tmp_buffer - int13_handler + 7	#; additional sense length
	jb	3f
	movw	atapi_tmp_buffer - int13_handler + 12, %bx
3:
	clc
	ret
1:
	stc
	ret


edd30_for_cdrom:
//	; Stack layout:
//	;   +10 INT flags
//	;   +8  INT CS
//	;   +6  INT IP
//	;   +2  EAX
//	; BP+0 BP
//	;   -2 ax
//	;   -4 cx
//	;   -6 dx
//	;   -8 bx
//	;   -10 sp
//	;   -12 bp
//	;   -14 si
//	;   -16 di
//	;   -18 ds
//	;   -20 es
//	;   -24 edx

	/* will read these variables:
	 *	int13_old_eax
	 *	int13_old_ebx
	 *	int13_old_ecx
	 *	int13_old_edx
	 *	int13_old_esi
	 *	int13_old_edi
	 *	int13_old_ebp
	 *	int13_old_ds
	 *	int13_old_es
	 *	int13_old_flags
	 *	int13_old_cs_ip
	 * will write these variables:
	 *	int13_old_eax
	 *	int13_old_ebx
	 *	int13_old_ecx
	 *	int13_old_flags
	 */

	sti

	cld

	pushw	%cs
	popw	%ds

	movzbw	%dl, %ax
	subb	%cs:(EXT_C(min_cdrom_id) - int13_handler), %al
	call	select_atapi	#; select_atapi_force
	movw	%cs:(int13_old_eax - int13_handler), %ax

	jnc	1f
	movb	$0xAA, %ah
	jmp	2f	//fail_out
1:
	cmpb	$0, %ah
	je	6f	//reset
	cmpb	$1, %ah
	je	7f	//get_last_stat
	cmpb	$0x41, %ah
	je	8f	//install_check
	cmpb	$0x42, %ah
	je	9f	//ext_read
	cmpb	$0x43, %ah
	je	10f	//ext_write
	cmpb	$0x44, %ah		/* verify sectors */
	je	5f	//success_out
	cmpb	$0x47, %ah		/* extended seek */
	je	5f	//success_out
	cmpb	$0x48, %ah
	je	11f	//get_drv_param
	cmpb	$0x4B, %ah
	jne	3f	//invalid_cmd

	//stop_disk_emu:
	cmpb	$1, %al			/* only 0x4B01 supported */
	jne	3f	//invalid_cmd

	movw	%si, %di
	movw	%cs:(int13_old_ds - int13_handler), %es
	movw	$0x0013, %ax	/* packet size=13h, boot type=0 (no-emu) */
	cld
	stosw
	//movb	%dl, %al	/* drive=DL, controller=0 */
	movb	%cs:(int13_old_edx - int13_handler), %al	/* drive=DL, controller=0 */
	stosw
	xorw	%ax, %ax
	stosw
	stosw			/* LBA for no-emu image=0 */
	stosw			/* device specification */
	stosw			/* user buffer segment */
	stosw			/* load segment */
	movb	$4, %al
	stosw			/* sector count=4 */
				/* CHS makes no sense for no-emu */
	jmp	5f	//success_out

11:	//get_drv_param:
	movw	%cs:(int13_old_ds - int13_handler), %ds
	cmpw	$26, (%si)
	jb	3f		//invalid_cmd

	movw	$26, (%si)
//	movw	$0x74, 2(%si)		#; struc_extparam.flags: removable, lock, chg line
	movw	$0x00, 2(%si)		# none, tinybit 2007-11-15
	movw	$0x800, 24(%si)		# bytes per sect=2048
	xorl	%eax, %eax
	decw	%ax
	movl	%eax, 4(%si)		# cylinders=0xFFFF
	movb	$0, %ah
	movl	%eax, 8(%si)		# heads=0xFF
	movb	$15, %al
	movl	%eax, 12(%si)		# sectors per track=15
	movl	%eax, 20(%si)		# total sectors hi dword=0
	xorw	%ax, %ax		# CF cleared
	decl	%eax			# EAX=0xFFFFFFFF
	movl	%eax, 16(%si)		# total sectors lo dword
					# CF is cleared
	jmp	5f	//success_out

10:	//ext_write:
	movb	$0x03, %ah	# error code of write-protection
	jmp	2f	//fail_out

9:	//ext_read:
	call	edd30_read_cdrom
	jnc	5f	//success_out
	jmp	2f	//fail_out

8:	//install_check:
	cmpw	$0x55AA, %bx	# added by tinybit 2007-11-15
	jne	3f	//invalid_cmd

	movw	$0xAA55, %cs:(int13_old_ebx - int13_handler)	#; bx=0xaa55
	movb	$0x21, %ah	# ah=0x21  edd-1.1
	movb	$0x01, %cs:(int13_old_ecx - int13_handler)	# cx=0x01, ext disk access ok -- tinybit 2007-11-15
	jmp	4f	//success_out_no_ah

7:	//get_last_stat:
	movb	%cs:(int13_last_stat - int13_handler), %ah
	jmp	4f	//success_out_no_ah

6:	//reset:
	call	reg_reset
	movw	%cs:(atapi_cur_dev - int13_handler), %ax	# device serial number
	call	select_atapi_force

5:	//success_out:
	xorb	%ah, %ah

4:	//success_out_no_ah:
	andb	$0xFE, %cs:(int13_old_flags - int13_handler)
	jmp	1f

3:	//invalid_cmd:
	movb	$0x01, %ah

2:	//fail_out:
	orb	$1, %cs:(int13_old_flags - int13_handler)	// set CF=1 for ERROR

1:
	/* set error code AH */
	movb	%ah, %cs:(int13_last_stat - int13_handler)
	movb	%ah, %cs:(int13_old_eax + 1 - int13_handler)

	movl	%cs:(int13_old_eax - int13_handler), %eax
	movl	%cs:(int13_old_ebx - int13_handler), %ebx
	movl	%cs:(int13_old_ecx - int13_handler), %ecx
	movl	%cs:(int13_old_edx - int13_handler), %edx
	movl	%cs:(int13_old_esi - int13_handler), %esi
	movl	%cs:(int13_old_edi - int13_handler), %edi
	movl	%cs:(int13_old_ebp - int13_handler), %ebp
	movw	%cs:(int13_old_ds - int13_handler), %ds
	movw	%cs:(int13_old_es - int13_handler), %es

	/* restore old flags */
	pushw	%cs:(int13_old_flags - int13_handler)
	popfw
	/* transfer control to caller */
	ljmp	*%cs:(int13_old_cs_ip - int13_handler)

#;============================================================================

int13_last_stat:	.byte	0			#; the error code in AH

start_of_atapi_data:

atapi_cur_dev:		.word	0	#; device serial number
reg_cur_dev:		.byte	0	#; 0xA0 for master, 0xB0 for slave
time_out:		.long	0
reg_dev_info:		.byte	0, 0


atapi_dev_base:		.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device

atapi_dev_base_bak:	.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device
			.word	0, 0	#; base pointer, device

reg_addr:		.word	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
atapi_cmd_buffer:	.space	16
atapi_tmp_buffer:	.space	256
//atapi_devinfo:	.space	32	# SIZE_OF_ATAPI_DEVINFO
delay_repeat_num:	.long	0	# loops per millisecond

end_of_atapi_data:

ENTRY(slow_atapi_device)
			.long	0

	.align	16

	#;*******************************************************************
	#;			ATA Registers Layout
	#;*******************************************************************
	#;
	#; Address		Read			Write
	#; ---------	---------------------	-----------------------------
	#; A N 0 0 0	16-bit data port for both read and write
	#; ------------------------------------------------------------------
	#; A N 0 0 1	ATAPI/ATA Error(Read)	ATAPI/ATA Feature(Write)
	#; ------------------------------------------------------------------
	#; A N 0 1 0	ATAPI Interrupt Reason(Read) / ATA Sector Count
	#; ------------------------------------------------------------------
	#; A N 0 1 1	Reserved for SAM TAG Byte / ATA Sector Number
	#; ------------------------------------------------------------------
	#; A N 1 0 0	ATAPI Byte Count LSB(R/W) / ATA Cylinder Low(R/W)
	#; ------------------------------------------------------------------
	#; A N 1 0 1	ATAPI Byte Count MSB(R/W) / ATA Cylinder High(R/W)
	#; ------------------------------------------------------------------
	#; A N 1 1 0	ATAPI Drive Select(R/W) / ATA Drive/Head Select(R/W)
	#; ------------------------------------------------------------------
	#; A N 1 1 1	ATAPI/ATA Status(Read)	ATA Command(Write)
	#; ------------------------------------------------------------------
	#; N A 1 1 0	Alternate Status(Read)	Device Control(Write)
	#; ------------------------------------------------------------------

reg_base_addr:
			.word	0x01f0, 0x03f6
			.word	0x0170, 0x0376
			.word	0x00f0, 0x02f6
			#;.word	0x0070, 0x0276	//port 70 and 71 is for CMOS
			.word	0x0180, 0x0386
			.word	0x6b00, 0x6f00
			.word	0x7300, 0x7700
ENTRY(reg_base_addr_append)
			.word	0, 0
			.word	0, 0		// ending null

#endif	//CDROM_INIT
#;============================================================================

/* void realmode_printf(const char *format, ...)
 *
 * input:	format is offset in CS segment
 *
 * Usage example:
 *
 * 		pushw	IntegerN
 *		 ... ... ... ...
 * 		pushw	Integer2
 * 		pushw	Integer1
 * 		pushw	$format_string - int13_handler
 *		call	realmode_printf
 * 		addw	$(2*(N+1)), %sp
 *
 * where int13_handle should be the base of the CS segment,
 * and format_string like this:
 *
 * format_string:
 *		 .string "Int1=%x, Int2=%x, ..., IntN=%x\r\n"
 *
 * Currently only %d, %x and %X are implemented.
 */

realmode_printf:
	pushaw
	movw	%sp, %bp
	# bp+18:	format
	# bp+20:	variables
	addw	$18, %bp
	movw	(%bp), %si		# points to format string
	addw	$2, %bp			# (%bp) is the first variable
1:
	cs lodsb
	testb	%al, %al
	jz	1f
	cmpb	$'%', %al
	jne	2f

	#; %d, %u, %x, %X, %4x, %4X
	xorw	%cx, %cx
7:
	cs lodsb
	testb	%al, %al
	jz	1f
	movw	$10, %bx		# base 10
	cmpb	$'d', %al
	jz	4f
	cmpb	$'u', %al
	jz	4f
	cmpb	$'x', %al
	jz	3f
	cmpb	$'X', %al
	jz	3f
	cmpb	$'4', %al
	jne	1b			# unkown directive, continue
	movw	$4, %cx
	jmp	7b
3:
	/* print hexa number */
	movw	$16, %bx		# base 16
4:
	/* print decimal or hexa number */
	pushl	%edi

	movl	$1, %edi
	movw	(%bp), %ax
5:
	xorw	%dx, %dx
	divw	%bx			# AX=quo, DX=rem
	shll	$4, %edi
	orw	%dx, %di
	jcxz	7f
	loop	5b
7:
	testw	%ax, %ax		# end?
	jnz	5b

	/* print the digits in EDI */
	xorw	%bx, %bx	/* video page 0 */
5:
	movw	%di, %ax		# get digit in AL
	shrl	$4, %edi
	jz	7f
	andb	$0x0f, %al
	cmpb	$9, %al
	jbe	6f
	addb	$7, %al			# A, B, C, D, E, F
6:
	addb	$0x30, %al
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	jmp	5b
7:
	popl	%edi

	addw	$2, %bp			# (%bp) is the next variable
	jmp	1b			# continue
2:
	/* print char in AL */
	xorw	%bx, %bx	/* video page 0 */
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	jmp	1b			# continue
1:
	popaw
	ret


#;============================================================================				
			
			
/* Don't insert code or data here! Buggy BIOSes could overwrite this area! */

//	.align	16

//	.space	0x1000	/* 4KB stack */

int13_handler_end:

	/* Total size of int13_handler should better not exceed 12KB. */

//	. = . - (int13_handler_end - int13_handler)/0x3001

	// this is used to check the current size of the handler code
	//. = . - (int13_handler_end - int13_handler)/0x2F01

#;============================================================================
ENTRY(defer)

	.code32

	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%ecx
	pushl	%edx

	movl	8(%ebp), %ebx
	movw  EXT_C(count_ms), %dx

	call	EXT_C(prot_to_real)
	.code16

	movw %bx, %ax
2:
	movw %dx, %cx
3:
	cmpw %cs:0x046C, %ax
	jmp 1f
1:
	pushw %dx
	pushw %ax
	movw $0xcf8, %dx
	inw %dx, %ax
	popw %ax
	popw %dx
	decw %cx
	cmpw $0, %cx
	jne 3b
	decw %ax
	jne 2b

	call	EXT_C(real_to_prot)
	.code32

	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp
	ret

/* unsigned long
 * init_usb() : return number of usb devices.
 */

ENTRY(init_usb)

	.code32

	pushl	%ebp
	pushl	%ebx

	call	EXT_C(prot_to_real)	/* enter real mode */

	.code16

	sti		/* enable interrupt for USB */

	cld

	/* set CS base to be USB2DRI */
	.byte	0xEA
	.word	ABS(1f)
	.word	0
1:
	pushaw
	pushw	%es
	pushw	%ds

	pushw	%cs
	popw	%ds
	pushw	%cs
	popw	%es

	//Load the usb driver
	movw	$ABS(USB2DRI), %ax
	shrw	$4, %ax
	movw	%ax, ABS(2f+2)
	lcall	*ABS(2f)

	popw	%ds
	popw	%es
	popaw

	ljmp	$0, $ABS(1f)
2:	
	.word 0
	.word 0
1:

	call	EXT_C(real_to_prot)
	.code32

	sti
	popl	%ebx
	popl	%ebp
	ret

/* ********************  USB2DRI start  ****************************** */
	.align	32
#define cd 1
ENTRY(USB2DRI)
USB2DRI:
	.code16
	
	jmp Initialization
	
	.ascii  "USB2DRI "
ENTRY(usb_delay)
	.byte   0
map_num:
	.byte   0	
bios_dl:
	.byte   0
fd_num:
	.byte   0
hd_num:
	.byte   0
max_dev:
	.byte   0	
ENTRY(max_dri)
	.byte   0	
Count_1ms:
	.word   0
ENTRY(One_transfer)
	.word   0x4000
bus_device_function:
	.word   0
Register_pci:
	.long   0
old_int13:
	.long   0
	
	. = USB2DRI + 0x20



qh:
	.fill 0x90,1,0x00                      //QH,qTD

//qTD1    db      40h dup(0)

//QH0     db      40h dup(0)
//				db		  10h dup(0)    

cbw_qq:
dCBWSignature:
	.long   0x43425355
dCBWTag:
	.long   0x12345678
dCBWDataTransferLength:
	.long   0
bmCBWFlags:
	.byte   0
bCBWLUN:
	.byte   0
bCBWCBLength:
	.byte   0x0c
CBWCB:
	.long   0
	.long   0
	.long   0
	.long   0
	.byte   0 

csw_qq:
	.fill 0x20,1,0x00

//---------------------------------------------------------------------------- 

GDT:

	.word	0, 0
	.byte	0, 0, 0, 0
	

LimitL:
	.word   0xffff
BaseL:
	.word   0
BaseM:
	.byte   0 
Attributes:
	.byte   0x92
LimitH:
	.byte   0
BaseH:
	.byte   0

aaaa:
#define	GDTLen	aaaa - GDT  


VGDTR:
	.word	GDTLen - 1
Base:        
	.long	0
	

//---------------------------------------------------------------------------- 
	.ascii  "DEV"

Dev:
DevID:
	.byte   0
ConProperty:
	.byte   0
ConAddress:
	.byte   0
ConSize:
	.word   0
InProperty:
	.byte   0
InAddress:
	.byte   0
InSize:
	.word   0
OutProperty:
	.byte   0
OutAddress:
	.byte   0
OutSize:
	.word   0
MaxLUN:
	.byte   0
Port_Num_Reg:
	.byte   0
HUB_id:
	.byte   0
	
bbbb:
#define	DevLen	(bbbb - Dev)

	.fill (DevLen*7),1,0x00

//---------------------------------------------------------------------------- 
	.ascii  "DRI"

Dri:
Number:
	.byte   0
DriID:
	.byte   0
DriLUN:
	.byte   0
#if cd
DriType:
	.byte   0	
#endif
SectorSize:
	.word   0
Capacity:
	.long   0
StartSector:
	.long   0       

cccc:        
#define	DriveLen  (cccc - Dri)

	.fill (DriveLen*3),1,0x00

//----------------------------------------------------------------------------
	.align	8

Get_device:
	.byte   0x80,06,00,01,00,00,0x12,00
Get_Configuration:
	.byte   0x80,06,00,02,00,00,0x20,00
Get_lun:
	.byte   0xa1,0xfe,00,00,00,00,01,00
Set_Configuration:
	.byte   00,0x09,01,00,00,00,00,00
Set_device_Address:
	.byte   00,05,00,00,00,00,00,00 
Clear_endpoint:
	.byte   02,01,00,00,00,00,00,00
Bulk_Reset:
	.byte   0x21,0xff,00,00,00,00,00,00
Get_Hub:
	.byte   0xa0,06,00,0x29,00,00,0x10,00
Set_HubPort_Power:
	.byte   0x23,03,0x08,00,00,00,00,00
Get_HubPort_Status:
	.byte   0xa3,00,00,00,00,00,04,00
Clear_HubPort_Conne:
	.byte   0x23,01,0x10,00,00,00,00,00
Set_HubPort_Reset:
	.byte   0x23,03,04,00,00,00,00,00
Clear_HubPort_Reset:
	.byte   0x23,01,0x14,00,00,00,00,00

_id:
	.byte   1
_lun:
	.byte   0
#if cd
_type:
	.byte   0	
Media_types:	
	.byte   0
cd_num:	
	.byte   0xf8
#endif
AsyncListAddr:
	.long   0
qTD1:
	.word   0
qTD1_Linear:
	.long   0
ds_Linear:
	.long   0  	
Sector_Size:
	.word   0
Disk_sector:
	.long   0
Start_sector:
	.long   0
Status:
	.byte   0
Read_write_repeat_Number:
	.byte   0
Stack_sp:
	.word   0
Stack_ss:
	.word   0
Ability_Register_Size:
	.word   0
_eax:
	.long   0
_bx:
	.word   0
_cl:
	.byte   0
_es:
 	.word   0
Direction1:
	.byte   0
R_W_Mark:
	.byte   0
Save_ah:
	.byte   0	
Return:
	.byte   0	
TSize:
	.long   0
TOffset:
	.word   0
TSection:
	.word   0
Single_size:
	.word   0
	
Entry_int13:
	call Find_drive_info
	jnb 1f
	testb $1, %cs:(map_num - USB2DRI)
	je 2f	
	cmpb $0x80, %dl
	jb 2f
	incb %dl
2:	
	ljmp *%cs:(old_int13 - USB2DRI)
1:        
	cld               
	pushal                          //       eax ecx edx ebx esp ebp esi edi  cf 
	movw %sp, %bp                   //bp+    1c  18  14  10  0c  08  04  00   24
	pushw %ds
	pushw %es
	pushw %fs
	pushw $0
	popw %fs
        //ss=ds=0 es=2000 cs=9e00  fs=0000
	cmpb $00, %ah
	je int13_00_04_16_17_18_44_47
	cmpb $01, %ah
	je int13_01
	cmpb $02, %ah
	je int13_02_03        
	cmpb $03, %ah
	je int13_02_03
	cmpb $0x08, %ah
	je int13_08
	cmpb $0x15, %ah
	je int13_15
	cmpb $0x16, %ah
	je int13_00_04_16_17_18_44_47
	cmpb $0x41, %ah
	je int13_41                
	cmpb $0x42, %ah
	je int13_42_43     
	cmpb $0x43, %ah
	je int13_42_43
	cmpb $0x48, %ah
	je int13_48
#if cd
	cmpw $0x4b01, %ax
	je int13_4b01	
#endif
qtml:        
	movb $01, %ah
	jmp 4f        
int13_00_04_16_17_18_44_47:
	xorb %ah, %ah         
4:  
	testb $0x80, %dl
	jne 1f
	movb %ah, %fs:0x0441
	jmp 2f
1:	
	movb %ah, %fs:0x0474
2:	
	andb $0xfe, 0x24(%bp)        		//cf=0
	movb %ah, 0x1d(%bp)             //ah
	orb %ah, %ah
	je 3f
	orb $01, 0x24(%bp)              //cf=1
3: 
	popw %fs
	popw %es
	popw %ds
	popal
	iret        
        

int13_01:  
	testb $0x80, %dl
	jne 1f
	xchgb %fs:0x0441, %ah
	jmp 2b
1:
	xchgb %fs:0x0474, %ah        
	jmp 2b    


int13_02_03:
	movb %ah, %cs:(Save_ah - USB2DRI)
	call chs_lba
	cmpl %cs:(Disk_sector - USB2DRI), %eax
	ja 2f
	movb $01, %cl
	cmpb $02, %cs:(Save_ah - USB2DRI)
	je 1f
	xorb %cl, %cl
1:     
	call Read_write_usb
	jnb int13_00_04_16_17_18_44_47
2:
	jmp qtml        

int13_08:
	pushw %dx
	movw $0x3ec1, %cx        
	movw %cs:(Disk_sector - USB2DRI), %ax
	movw %cs:(Disk_sector + 2 - USB2DRI), %dx 
	cmpw %cx, %dx
	jae 1f
	divw %cx
	decw %ax
	cmpw $0x03FF, %ax
	jb 2f
1:        
	movw $0x03FF, %ax
2:
	movw %ax, %cx
	xchgb %ch, %cl
	shlb $06, %cl
	orb $0x3f, %cl
	movb $0xfe, %dh
	movb %fs:0x0475, %dl
	movw %dx, 0x14(%bp)
	movw %cx, 0x18(%bp)
	movb $0x10, 0x10(%bp)
	popw %dx
	jmp int13_00_04_16_17_18_44_47        
         
int13_15:
	movb %al, %fs:0x0441
	movb $02, %ah        
	testb $0x80, %dl
	je 1f
	movb $03, %ah
1:        
	andb $0xfe,0x24(%bp)            //cf=0
	movb %ah, 0x1d(%bp)             //ah
	jmp 3b


Find_drive_info:
	pushal
	pushw %ds
	pushw %cs
	popw %ds
	movb (EXT_C(max_dri) - USB2DRI), %cl
	movw $(Dri - USB2DRI), %si
1:        
	cmpb (Number - Dri)(%si), %dl
	je 1f
	decb %cl
	orb %cl, %cl
	je 2f
	addw $DriveLen, %si 
	jmp 1b
1:
	movb (DriID - Dri)(%si), %al 
	movb %al, (_id - USB2DRI)
	movb (DriLUN - Dri)(%si), %al 
	movb %al, (_lun - USB2DRI)
#if cd
	movb (DriType - Dri)(%si), %al
	movb %al, (_type - USB2DRI)
#endif
	movw (SectorSize - Dri)(%si), %ax
	movw %ax, (Sector_Size - USB2DRI)
	movl (Capacity - Dri)(%si), %eax 
	movl %eax, (Disk_sector - USB2DRI)
	movl (StartSector - Dri)(%si), %eax 
	movl %eax, (Start_sector - USB2DRI)      
	clc
1:
	popw %ds
	popal
	ret
2:
	stc
	jmp 1b                 


chs_lba:
	movw %bx, %di
	movzbw %al, %bx

	movb %cl, %al
	andb $0x3f, %al
	movzbl %al, %eax
	pushl %eax 

	xchgb %ch, %cl
	andb $0xc0, %ch
	rolb $02, %ch

	movl $0xff, %eax
	movzwl %cx, %ecx
	imull %ecx, %eax
	movzbl %dh, %ecx
	addl %ecx, %eax 
	movl $0x3f, %ecx
	imull %ecx, %eax
	popl %ecx
	addl %ecx, %eax        
	decl %eax
   
	ret
   



int13_41:   
	cmpw $0x55aa, %bx
	jne qtml 
	movw $0x2100, %ax               //EDD-2.1
	notw %bx                        //bx=AA55
	movw $5, %cx 
	movw %bx, 0x10(%bp)
	movw %cx, 0x18(%bp)         
	movw %ax, 0x1c(%bp) 
	andb $0xfe, 0x24(%bp)           //cf=0
	jmp 3b


int13_42_43:
	testl $0xffffffff, 12(%si)
	jne qtml
	movb %ah, %cs:(Save_ah - USB2DRI)     
	call Disk_package
	movb $01, %cl
	cmpb $0x42, %cs:(Save_ah - USB2DRI)
	je 1f
	xorb %cl, %cl       
1:
	call Read_write_usb
	jnb int13_00_04_16_17_18_44_47
2:
	jmp qtml


Disk_package:
	movw 02(%si), %bx
	movw 04(%si), %di
	movw 06(%si), %es
	movl 0x08(%si), %eax       
	ret

int13_48:
	pushw %ds
	popw %es
	pushw %dx
	movw %si, %di 
	xorb %al, %al
	movw $0x1a, %cx
	rep stosb
	movb $0x1a, (%si)
	movb $06, 02(%si)
	movb $0x3f, 0x0c(%si)
	movb $0xff, 0x08(%si)
	movl $0x3ec1, %ecx
	movl %cs:(Disk_sector - USB2DRI), %eax
	movl %eax, 0x10(%si)
	xorl %edx, %edx
	divl %ecx
	movl %eax, 04(%si)        
	movw %cs:(Sector_Size - USB2DRI), %ax
	movw %ax, 0x18(%si)
	popw %dx
	jmp int13_00_04_16_17_18_44_47
#if cd	
int13_4b01:			
	movb $0x13, (%si)
	movb %cs:(Media_types - USB2DRI), %al
	movb %al, 1(%si)
	movb %cs:(cd_num - USB2DRI), %al
	movb %al, 2(%si)
	movb $1, 3(%si)
	movl %cs:(Start_sector - USB2DRI), %eax
	movl %eax, 4(%si)
	movw %cs:(bus_device_function - USB2DRI), %ax
	movb %cs:(_lun - USB2DRI) ,%al
	movw %ax, 8(%si)
	movw $0, 0x0a(%si)
	movw %cs:(Disk_sector - USB2DRI), %ax
	movw %ax, 0x0c(%si)
	movw %cs:(Disk_sector + 2 - USB2DRI), %ax
	movw %ax, 0x0e(%si)
	movw $0, 0x10(%si)
	movb $0, 0x12(%si)	
	jmp int13_00_04_16_17_18_44_47
#endif	
//------------------------------------------------------------------------------------------


Allow_A20:
	call observe
	orw %ax, %ax 
	jne 1f
	mov $0x2401, %ax
	int $0x15
	call observe
	orw %ax, %ax
	jne 1f
	pushfl
	cli

2:
	inb $0x64, %al
	testb $02, %al
	jne 2b     
	mov $0xd1, %al
	outb %al, $0x64
2:
	inb $0x64, %al
	testb $02, %al
	jne 2b
	movb $0xdf, %al
	outb %al, $0x60
2:
	inb $0x64, %al
	testb $02, %al
	jne 2b
	movb $0xff, %al
	outb %al, $0x64
2:
	inb $0x64, %al
	testb $02, %al
	jne 2b
	popfl
1:
	ret



observe:
	pushw %bx
	pushw %dx
	pushw %gs
	pushw $0xffff
	popw	%gs
	pushfl
	cli
	movw %fs:0x0000, %ax
	movw %gs:0x0010, %bx
	notw %ax
	movw %ax, %gs:0x0010
	movw %fs:0x0000, %dx
	movw %bx, %gs:0x0010 
	popfl
	xorw %dx, %ax
	popw %gs
	popw %dx
	popw %bx
	ret

        

Delay:
	pushl %ecx
	pushw %dx
	movw (Count_1ms - USB2DRI), %dx
2:        
	movw %dx, %cx

3:
	cmpw %cs:0x046C, %ax 
	jmp 1f
1:
	pushw %dx
	pushw %ax
	movw $0xcf8, %dx
	inw %dx, %ax
	popw %ax
	popw %dx
	decw %cx
	cmpw $0, %cx
	jne 3b

	decw %ax
	jne 2b
	popw %dx
	popl %ecx
	ret


Remove_qTD:				
	pushaw
  xorw	%ax, %ax
	movw	$0x20, %cx
	rep	stosw
	popaw
	ret	
	
	
Control_transfer_hub:
	movb %cl, 4(%si)
Control_transfer:
	pushaw

	call Determine_device_address

	movw (qTD1 - USB2DRI), %di
	movw $0x8e80, %ax
	movw $8, %cx
	movzwl %si, %edx
	addl (ds_Linear - USB2DRI), %edx
	call Con_mon
	call Set_QH
              
	movw 6(%si), %cx
	orw %cx, %cx 
	je 1f
	movb $0, %ah
	testb $0x80, (%si)
	je 2f
	movb $01, %ah
2:
	movb $0x80, %al
	orb $0x8c, %ah
	orw $0x8000, %cx
	movl $(Cache - USB2DRI), %edx
	addl (ds_Linear - USB2DRI), %edx
	call Con_mon
	call Set_QH
	cmpb $0, (Status - USB2DRI)
	jne 2f
1:
	movb $0x81, %ah
	testb $0x80, (%si) 
	je 1f 
	movb $0x80, %ah
1:
	movb $0x80, %al
	orb $0x0c, %ah
	movw $0x8000, %cx
	xorl %edx, %edx
	call Con_mon
	call Set_QH
2:							
	popaw
	ret

Con_mon:
	call Remove_qTD
	movw %ax, 8(%di)
	movw %cx, 0xa(%di)
	movl %edx, 0xc(%di)
	movl $0x1000, %eax
	andl $0xfffff000, %edx
	addl %eax, %edx
	movl %edx, 0x10(%di)
	movb $1,(%di)
	movb $1,4(%di)
	movw $0, %cx
	ret

Port_Reset:
	movl $0x00001100, %eax      
	call Write_Register
	movw $100, %ax
	call Delay

	movl $0x00001000, %eax  
	call Write_Register 
	movw $200, %ax
	call Delay
	
	movl $0x00001007, %eax    
	call Write_Register

	call Read_Register
	ret
	
	
Determine_device_address:
	movb	(_id - USB2DRI), %bl
	decb	%bl
	movb	$DevLen, %al
	mulb	%bl
	movw	%ax, %bx 
	ret	
	


Set_QH:
	pushw %si
	pushw %di
	addw $0x40, %di
	movb $2, %dl
	
	leaw (ConAddress - USB2DRI)(%bx), %si
	addw %cx, %si
	movb (%si), %ch
	andb $0x0f, %ch 
	shlb $04, %dl
	orb %dl, %ch
	movw 01(%si), %ax
	orb %cl, %cl
	jne 1f
	orw $0x0800, %ax
	orb $0x40, %ch 
1:
	orb $0x80, %ch
	movb (DevID - USB2DRI)(%bx), %cl
	andb $0x7f, %cl 
	movb %cl, 04(%di)
	movb %ch, 05(%di)
	movw %ax, 06(%di)
	
	movb $0x40, 0xb(%di)

	movl (qTD1_Linear - USB2DRI), %eax
	movl %eax, 0x0c(%di)
	movw (qTD1 - USB2DRI), %si
	pushw %di
	addw $0x10, %di
	movw $0x20, %cx
	rep movsw
	popw %di			
	movb $00, (Status - USB2DRI) 

	movl $0x80021, %eax
	movw $0, %si	
	call Write_Register
	movl $0x00100000, %ecx 
1:	
	movw $04, %si
	call Read_Register
	testb $03, %al
	jne 2f
	loopl 1b
	orb $2, (Status - USB2DRI)
	jmp 1f
2:
	movw $04, %si
	call Write_Register
	test $01, %al
	jne 2f
	orb $4, (Status - USB2DRI)
	jmp 1f
2:
	movl $0x00100000, %ecx 
2:
	testb $0x80, 0x18(%di)
	je 2f      
	loopl 2b
	orb $8, (Status - USB2DRI)
2:
	testb $8, 7(%di)
	je 1f
	movw $20, %ax
	call Delay	
1:
	pushw %ax
	movl $0x80001, %eax
	movw $0, %si
	call Write_Register 
	popw %ax
	cmpb $0, (Status - USB2DRI)
	jne 6f

	movb 0x18(%di), %al
	movb 0x19(%di), %ah	
	andb $0x0c, %ah 
	orb %ah, %ah
	je 5f
	testb $0x40, %al
	jne 5f 
	testb $0x20, %al
	jne 5f                   
3:
	movzbl (usb_delay - USB2DRI), %ecx
	orb %cl, %cl
	je 1f
	shll $14, %ecx
2:
	loopl 2b
1:
	popw %di
	popw %si
	ret
5:
	orb $01, (Status - USB2DRI)
6:
	jmp 3b

	
Bulk_transfer:
	pushw %bx
	movw (qTD1 - USB2DRI), %di
	call Remove_qTD
	
	decb %al
	je 1f
	decb %al
	je 2f
	
	movw $0x0d, %cx
	movl $(csw_qq - USB2DRI), %eax
	movb $01, (Direction1 - USB2DRI)  
	jmp 3f
1:
	movw $0x1f, %cx
	movl $(cbw_qq - USB2DRI), %eax
	movb $0, (Direction1 - USB2DRI)
3:
	movw %cs, %dx
	movzwl %dx, %edx
	shll $4, %edx
	addl %eax, %edx
	jmp 3f
2:
	movb (bmCBWFlags - USB2DRI), %al
	shrb $7, %al
	movb %al, (Direction1 - USB2DRI)
	movl (TOffset - USB2DRI), %edx
	movw (Single_size - USB2DRI), %cx
3:
	movb $01, (%di)
	movb $01, 04(%di) 
	movb $0x80, 0x08(%di) 
	cmpw $0, %cx
	je 1f
	movl $0x1000, %eax    
	movl %edx, 0x0c(%di)
	andl $0x0fffff000, %edx
	addl %eax, %edx
	movl %edx, 0x10(%di) 
	addl %eax, %edx
	movl %edx, 0x14(%di)
	addl %eax, %edx
	movl %edx, 0x18(%di)
	addl %eax, %edx
	movl %edx, 0x1c(%di)
1:        
	leaw (InProperty - USB2DRI)(%bx), %si
	movb (Direction1 - USB2DRI), %al
	testb $01, %al 
	jne 1f
	addw $4, %si
1:
	testb $0x80, (%si)
	je 1f
	orw $0x8000, %cx
1:
	orb $0x80, %al
	orb $0x0c, %al
	movb %al, 0x09(%di)
	movw %cx, 0x0a(%di)
	movw $4, %cx
	testb $01, %al     
	jne 1f

	movb $0x81, 0x08(%di)
	movw $8, %cx
1:  
	call Set_QH 
	cmpb $0, (Status - USB2DRI)
	jne 1f

	addw $0x40, %di
	andb $0x7f, (%si)
	testb $0x80, 0x1b(%di)
	je 1f 
	orb $0x80, (%si)
1:
	popw %bx
	ret


Reset_Recovery:
	movw $(Bulk_Reset - USB2DRI), %si
	call Control_transfer	

	movw $(Clear_endpoint - USB2DRI), %si
	movb (InAddress - USB2DRI)(%bx), %cl
	call Control_transfer_hub		

	mov $(Clear_endpoint - USB2DRI), %si
	mov (OutAddress - USB2DRI)(%bx), %cl
	call Control_transfer_hub	
	
	andb $0x7f, (InProperty - USB2DRI)(%bx)
	andb $0x7f, (OutProperty - USB2DRI)(%bx)			
	ret			
	

SCSI_Command:
	pushaw
	
	call Determine_device_address        
	movzwl (TOffset - USB2DRI), %eax   
	movzwl (TSection - USB2DRI), %edx
	shll $04, %edx
	addl %eax, %edx
	movl %edx, (TOffset - USB2DRI)
	movl (dCBWDataTransferLength - USB2DRI), %eax
	movl %eax, (TSize - USB2DRI)
	movw (One_transfer - USB2DRI), %ax
	movw %ax, (Single_size - USB2DRI)
	movb (_lun - USB2DRI), %al
	movb %al, (bCBWLUN - USB2DRI)

	movb $01, %al
	call Bulk_transfer
	cmpb $0, (Status - USB2DRI)
	jne 4f
1:
	movl (TSize - USB2DRI), %eax 
	orl %eax, %eax
	je 2f
	movzwl (Single_size - USB2DRI), %ecx
	cmpl %eax, %ecx
	jbe 3f
	movw %ax, (Single_size - USB2DRI)
3:      
	movb $02, %al
	call Bulk_transfer 
	cmpb $0, (Status - USB2DRI)
	jne 2f
 
	movzwl (Single_size - USB2DRI), %eax 
	addl %eax, (TOffset - USB2DRI)
	subl %eax, (TSize - USB2DRI)
	jmp 1b
2:  
	movb $03, %al
	call Bulk_transfer
	cmpb $0, (Status - USB2DRI)
	je 3f
	movw $(Clear_endpoint - USB2DRI), %si
	movb (InAddress - USB2DRI)(%bx), %cl
	call Control_transfer_hub 
	andb $0x7f, (InProperty - USB2DRI)(%bx)		
	movb $3, %al
	call Bulk_transfer
  cmpb $0, (Status - USB2DRI)
	jne 4f	
3:  
	movw $(csw_qq - USB2DRI), %si
	cmpl $0x53425355, (%si)
	jne 4f
	cmpl $0x12345678, 04(%si)
	jne 4f
	cmpb $0, 0x0c(%si)
	je 1f 
	cmpb $1, 0x0c(%si)
	je 3f 
4:
	call Reset_Recovery
3:
	movb $1, (Return - USB2DRI)
	jmp 2f
1:  
	movb $0, (Return - USB2DRI)
2:
	popaw
	ret


Read_write_usb:
	cli
	movw %ss, %cs:(Stack_sp - USB2DRI) 
	movw %sp, %cs:(Stack_ss - USB2DRI) 
	pushw %cs
	popw %ss
	movw $(Stack - USB2DRI), %sp
	sti
        
	pushal 
	pushw %ds
	pushw %es

	pushw %cs
	popw %ds
	pushw %cs
	popw %es 

	call Remove_CBWCB
        
	popw %es 
	pushw %es
	
	movb $3, (Read_write_repeat_Number - USB2DRI) 
	movl %eax, (_eax - USB2DRI)
	movw %bx, (_bx - USB2DRI)
	movw %es, (_es - USB2DRI)
	movb %cl, (_cl - USB2DRI)
5:
	movb $0x80, (bmCBWFlags - USB2DRI)
	movb $0x28, (CBWCB - USB2DRI)
	orb %cl, %cl
	jne 1f
	movb $0, (bmCBWFlags - USB2DRI)
	movb $0x2a, (CBWCB - USB2DRI)
1:        
	movb %al, (CBWCB - USB2DRI + 5) 
	movb %ah, (CBWCB - USB2DRI + 4)
	shrl $0x10, %eax
	movb %al, (CBWCB - USB2DRI + 3)
	movb %ah, (CBWCB - USB2DRI + 2)        
	movb %bl, (CBWCB - USB2DRI + 8)
	movb %bh, (CBWCB - USB2DRI + 7)  
	movw (Sector_Size - USB2DRI), %ax
	mulw %bx
	movw %dx, (dCBWDataTransferLength - USB2DRI + 2)
	movw %ax, (dCBWDataTransferLength - USB2DRI) 
	movw %di, (TOffset - USB2DRI)      
	movw %es, (TSection - USB2DRI)
	pushw %cs
	popw %es 
	call Allow_A20  
	movw $0x18, %si
	call Read_Register
	cmpl (AsyncListAddr - USB2DRI), %eax
	je 1f
	call init_QH_AsyncListAddr
1:
	call SCSI_Command 
	cmpb $0, (Return - USB2DRI) 
	je 6f 
	movl (_eax - USB2DRI), %eax
	movw (_bx - USB2DRI), %bx
	movw (_es - USB2DRI), %es
	movb (_cl - USB2DRI), %cl
				
	decb (Read_write_repeat_Number - USB2DRI)
	cmpb $0, (Read_write_repeat_Number - USB2DRI)
	jne 5b	
	
	stc        
3:
	popw %es
	popw %ds
	popal    
	cli
	movw %cs:(Stack_sp - USB2DRI), %ss 
	movw %cs:(Stack_ss - USB2DRI), %sp
	sti       
	ret

6:
	clc
	jmp 3b

  
Read_Base_address:
	pushl %esi
	movb $01, (R_W_Mark - USB2DRI) 
	jmp 1f

Read_Register:
	pushl %esi
	movb $01, (R_W_Mark - USB2DRI)
2:
	addw (Ability_Register_Size - USB2DRI), %si
1:
	movzwl %si, %esi
	call Protected_Mode 
	popl %esi
	ret


Write_Register:
	pushl %esi
	movb $0, (R_W_Mark - USB2DRI) 
	jmp 2b


Protected_Mode:  
	pushw %es
	pushfl
	cli
	pushw %ds
	pushw %es
	pushl %ecx
	pushw %cs        
	lgdt %cs:(VGDTR - USB2DRI)
	movl %cr0, %ecx
	pushl %ecx
	orb $01, %cl
	movl %ecx, %cr0 
	jmp 1f
1:
	movw $0x08, %cx
	movw %cx, %es
	testb $01, (R_W_Mark - USB2DRI) 
	jne 1f
	movl %eax, %es:(%si)
	jmp 2f
1:        
	movl %es:(%si), %eax
2:        
	popl %ecx
	andb $0xFE, %cl
	movl %ecx, %cr0 
	jmp 1f
1:      
	pushw $(1f - USB2DRI)
	retf
1:
	popl %ecx
	popw %es
	popw %ds
	popfl
	popw %es
	ret

Remove_CBWCB:
	pushaw
	movw $(dCBWDataTransferLength - USB2DRI), %di
	xorw %ax, %ax
	movw $12, %cx
	repz stosw
	movb $0xc, (bCBWCBLength - USB2DRI)
	popaw
	ret	
	
Port_Reset_hub:	
	movw $(Clear_HubPort_Conne - USB2DRI), %si
	call Control_transfer_hub
	mov $(Set_HubPort_Reset - USB2DRI), %si
	call Control_transfer_hub	
	movw $200, %ax
	call Delay
	movw $(Clear_HubPort_Reset - USB2DRI), %si
	call Control_transfer_hub
	ret
	
szsbdz:				     
	movw $(Set_device_Address - USB2DRI), %si
	movb (_id - USB2DRI), %al
	movb %al, 2(%si)
	call Control_transfer
	movb %al, (DevID - USB2DRI)(%bx)
	ret
	
szpz:
	movw $(Set_Configuration - USB2DRI), %si
	call Control_transfer
	movw $30, %ax
	call Delay		
	ret	

init_QH_AsyncListAddr:
	movl (qTD1_Linear - USB2DRI), %eax
	addl $0x40, %eax 
	pushw %ax
	orb $02, %al
	movw (qTD1 - USB2DRI), %si
	movl %eax, 0x40(%si) 
	popw %ax
	movl %eax, (AsyncListAddr - USB2DRI)
	movw $0x18, %si
	call Write_Register
	ret


Determine_delay_units:   
	xorw %ax, %ax
	movw %ax, %fs
	sti 
	movw %fs:0x046c, %dx
	xorl %ecx, %ecx
1:
	movw %fs:0x046c, %ax 
	cmpw %dx, %ax                
	je 1b                      
1:
	cmpw %fs:0x046c, %ax
	jne 2f 
	pushw %dx
	pushw %ax
	movw $0xcf8, %dx
	inw %dx, %ax
	popw %ax
	popw %dx
	incl %ecx 
	cmpl $0, %ecx
	jmp 1b
2:
	xorl %edx, %edx
	movl %ecx, %eax
	movl $0x37, %ecx
	divl %ecx
	movw %ax, (Count_1ms - USB2DRI) 
	ret
	
	
xh_hd:
  call read_dri
  decb %cl
  je 1f
  addw $0x200, %bx
  incb %dl
  jmp xh_hd
1:
  ret
  
	
Check_bus: 
	xorl %edi, %edi
	movw $0xB101, %ax 
	int $0x1A
	cmpl $0x20494350, %edx
	jne 1f
	movw $0, %si
	movw $0xB103, %ax 
	movl $0x000C0320, %ecx 
	int $0x1A 
	jb 1f
	movw %bx, (bus_device_function - USB2DRI)
	clc
	ret
1:
	stc       
	ret	
	


Stack:
	.ascii " DATA "
Cache:
Ending:
//=================================================================================================================
#define mem_mbr         0
#define mem_pbr         mem_mbr + 0x1000
#define mem_fd          mem_mbr + 0x2000
#define mem_hd          mem_mbr + 0x2400

Disk_Info:
	pushw %ds
	pushw %es
	pushw %cs:(Init_cs - USB2DRI)
	popw %es
	pushw %es
	popw %ds
	call Get_dri
	movb %dl, %dh
	orb %dl, %dl
	je 1f
	andb $0x3e, %fs:0x0410
	jmp bf_hd
1:	
  movb %fs:0x410, %cl
  testb $1, %cl
  jne 1f
  xorb %cl, %cl
	call set_410_1
1:
  shrb $6, %cl
  incb %cl
  cmpb $2, %cl
  jbe 1f
  movb $2, %cl
1:
  movb %cl, %cs:(fd_num - USB2DRI)
  xorb %dl, %dl
	movw $mem_fd, %bx
	call xh_hd
bf_hd:
	movb $8, %cl 
  cmpb $8, %fs:0x0475
	ja 1f
  movb %fs:0x0475, %cl
1:
	movb %cl, %cs:(hd_num - USB2DRI)
  cmpb $0, %cl
  je 1f
  movb $0x80, %dl
	movw $mem_hd, %bx
	call xh_hd					
	cmpb $0x80, %dh
	jne 1f
	movw $mem_hd, %di
	call Check_PartitionTable
	jnc 1f
	orb $1, %cs:(map_num - USB2DRI)
	call set_410_1	
1:
	popw %es
	popw %ds
	ret

host_initialization:                     
	call Allow_A20 
             
	pushw $0x10 
	pushw (bus_device_function - USB2DRI)
	movb $02, %al
	call Read_Write_Configuration
	addw $04, %sp
	movl %eax, (Register_pci - USB2DRI)      

	movw %ds, %ax
	movzwl %ax, %eax
	shll $04, %eax
	movl %eax, (ds_Linear - USB2DRI) 
	call Fill_descriptor

	call Define_qTD1

	movw $0, %si
	call Read_Base_address 
	movb %al, (Ability_Register_Size - USB2DRI)

	movw $04, %si 
	call Read_Base_address
	andw $0x0F, %ax
	movw %ax, (n_ports - USB2DRI)
 
	movl $02, %eax
	movw $0, %si
	call Write_Register
1:
	movw $3, %ax 
	call Delay
    
	movw $0, %si 
	call Read_Register
	testw $02, %ax
	jne 1b 

	movw $0x08, %si 
	call Read_Base_address
	xorw %cx, %cx
	movb %ah, %cl
	jcxz 1f
2:				
	pushw %cx
	pushw (bus_device_function - USB2DRI)
	movb $1, %al
	call Read_Write_Configuration
	addw $4, %sp
	cmpb $1, %al
	je 2f
	orb %ah, %ah
	je 1f
	addb %ah, %cl
	jmp 2b
1:
	jmp 3f		
2:
	movw %cx, %si
	leaw 02(%si), %dx 
	
	pushw %dx 
	pushw (bus_device_function - USB2DRI)
	movb $01, %al 
	call Read_Write_Configuration 
	addw $04, %sp

	orw $0x100, %ax
	pushw %ax 
	pushw %dx 
	pushw (bus_device_function - USB2DRI)
	movb $03, %al
	call Read_Write_Configuration
	addw $06, %sp
         
	movw $0x01F4, %cx        
2:
	pushw %dx 
	pushw (bus_device_function - USB2DRI)
	movb $01, %al 
	call Read_Write_Configuration 
	addw $04, %sp
	testw $01, %ax
	je 1f 
	movw $10, %ax
	call Delay 
	loop 2b
1:
 
	leaw 04(%si), %dx 
	
	pushw $00 
	pushw %dx
	pushw (bus_device_function - USB2DRI) 
	movb $03, %al 
	call Read_Write_Configuration
	addw $06, %sp               

3:
	call init_QH_AsyncListAddr

	movl $0x00080001, %eax
	movw $0, %si
	call Write_Register
	
	movl $3, %eax
  movw $8, %si
	call Write_Register

	movw (n_ports - USB2DRI), %cx
	movw $0x44, %di 
	jmp 2f
3: 
	movl $0x00001000, %eax
	movw %di, %si
	call Write_Register
	addw $04, %di
	decw %cx
2:
	cmpw $0, %cx 
	ja 3b
	movw $100, %ax
	call Delay
1:        

	movl $0x00000001, %eax
	movw $0x40, %si   
	call Write_Register 
	movw $500, %ax
	call Delay 
	ret
//=================================================================================================================
n_ports:
	.word   0       
Init_cs:
	.word   0
Config_Length:
	.byte		0
Num_Endpoint:
	.byte		0
usb_count:
	.byte		0
Interface_ok:
	.byte		0


Initialization:
	pushw %ds
	pushw %es
	pushw	%fs
	pushal

	movw $0x8000, %cs:(Init_cs - USB2DRI)

	pushw %cs
	popw %ds
	pushw %cs
	popw %es
	
	call Determine_delay_units
	call Check_bus
	movb $0x80,%fs:ABS(EXT_C(usb_count_error))
	jb 3f
	call Disk_Info 
	call host_initialization
	call Device_enumerate
	movb $0x81,%fs:ABS(EXT_C(usb_count_error))
	cmpb $0, (usb_count - USB2DRI)
	je 3f	
	call Drive_enumerate
	movb $0x82, %fs:ABS(EXT_C(usb_count_error))			//usb_error
	
	movb (EXT_C(max_dri) - USB2DRI), %al
	orb %al, %al
	je 3f
	movb %al, %fs:ABS(EXT_C(usb_count_error))				//usb_count
	movw $(Number - USB2DRI), %si
	movw $ABS(EXT_C(usb_drive_num)), %bx
1:				
	movb (%si), %ah
	movb %ah, %fs:(%bx)
	decb %al
	je 1f
	addw $DriveLen, %si 
	incw %bx
	jmp 1b
1:       	
	
	movw %fs:0x0413, %ax             
	subw $3, %ax     
	movw %ax, %fs:0x0413
	shlw $6, %ax
	pushw %ax
	movzwl %ax, %eax
	shll $4, %eax
	movl %eax, (ds_Linear - USB2DRI)
	call Define_qTD1
	call Fill_descriptor
	popw %ax	

	movw $(Ending - USB2DRI), %cx 
	xorw %si, %si     
	xorw %di, %di
	movw %ax, %es
	rep movsb 
	movw %ax, %ds
	shrw $5, %ax
	movw %ax, %fs:ABS(EXT_C(usb_md_address))
	call init_QH_AsyncListAddr
	call Set_int13 
	clc
4:	
	popal
	popw %fs
	popw %es
	popw %ds
	lret
3:
	stc
	jmp 4b
	



	
Read_Write_Configuration:  
	pushw %bp
	movw %sp, %bp
	pushw %bx
	pushl %ecx
	pushw %di
	movw 04(%bp), %bx
	movw 06(%bp), %di 
	movw 0x08(%bp), %cx
	decb %al
	jne 1f
	movw $0xB109, %ax 
	int $0x1A
	jmp 2f
1:
	decb %al
	jne 1f        
	movw $0xB10A, %ax 
	int $0x1A
	jmp 2f
1:
	movw $0xB10C, %ax
	int $0x1A                          
2:
	movl %ecx, %eax
	popw %di
	popl %ecx
	popw %bx
	popw %bp
	ret	
	
	
Set_int13:
	cli
	movl %fs:0x004c, %ebx
	movl %ebx, %ds:(old_int13 - USB2DRI) 

	movw $(Entry_int13 - USB2DRI), %bx
	movw %bx, %fs:0x004c
	movw %ds, %fs:0x004e 
	sti
	ret 	


Sector_comparison:
  pushw %si
  pushw %di
  pushw %cx
	movw $0x200, %cx
1:				
	repz cmpsb
	je 2f
	cmpw $0x01db, %cx
	je 1b
	cmpw $0x01bf, %cx
	je 1b
	cmpw $0x01e3, %cx
	jne 3f
	subw $3, %cx
	addw $3, %si
	addw $3, %di
	jmp 1b
2:
	clc
1:
	popw %cx
  popw %di
  popw %si
  ret
3:
	stc
	jmp 1b

	
	
	
Search_drive:
	movb %fs:0x0475, %cl
	orb $0x80, %cl
	incb %fs:0x0475
	ret	
	
	
read_hdd:
	pushw %bx
	movw $1, %bx
	movb $1, %cl
	call Read_write_usb
	popw %bx
	ret
	

read_int13:				
	pushw %ds
	pushw %es
	int $0x13
	popw %es
	popw %ds
	ret			
	
	

read_dri:
	pushaw
	movw $2, %si
1:        
	movw $0x0201, %ax    
	xorb %dh, %dh
	movw $1, %cx
	call read_int13  
	jnb 1f
	decw %si
	je 1f
	movb $0, %ah
	call read_int13
	jmp 1b
1:
	popaw
	ret
				

Get_dri:
	movb %fs:0x8280, %dl 
	cmpb $0x23, %dl
	jne 1f
	movb %fs:0x21af, %dl
1:	
	movb %dl, %cs:(bios_dl - USB2DRI)		
	ret	


Check_PartitionTable:
	pushaw
	xorb %bl, %bl
	leaw 0x1be(%di), %di
	movw %di, %si
	addw $0x40, %di
4:       
	cmpw %di, %si
	jnb	3f		        					/* partition table is OK */
	pushw %si
	movw $4, %cx	
2:
	lodsl
	negl %eax
	jc 2f
	loop 2b	
	popw %ax
	incb %bl
	cmpb $4, %bl
	jb 4b                      	/* empty entry, check next */	
	jmp 5f                      /* Error: empty table */
2:
	/* non-empty entry */
	popw %si
	lodsw
	shlb $1, %al
	jnz 5f
	lodsw
	andb $63, %al
	jz 5f
	lodsw
	lodsw
	andb $63, %al
	jz 5f
	lodsl
	negl %eax
	jnc	5f
	lodsl
	negl %eax
	jc 4b
	jmp 5f	
3:      
	lodsw
	cmpw $0xAA55, %ax
	je 1f
5:
	stc		                /* invalid partition table */        
1:
	popaw
	ret     
	
	
Define_disk_No: 
	pushaw
	pushw %ds
	pushw %es
	pushw (Init_cs - USB2DRI)      
	popw %es 
	pushw %es
	popw %ds

	call Get_dri
	movw %si, %bx	
#if cd
	cmpb $5, %cs:(_type - USB2DRI)
	jne 1f
							
	cmpb $0x9f, %dl
  jb 2f
  movb %dl, %cs:(Number - USB2DRI)(%bx)
	movb %dl, %cs:(cd_num - USB2DRI)
  jmp 3f
2:     
  movb $0xf8, %cs:(Number - USB2DRI)(%bx)
3:
	movl $0x11, %eax
	movw $mem_mbr, %di
	call read_hdd
	jc cssq_err
	cmpl $0x54204c45, 7(%di)
	jne cssq_err
	movl 0x47(%di), %eax
	call read_hdd
	cmpw $0xAA55, 0x1e(%di)
	jne cssq_err
	cmpw $0x88, 0x20(%di)
	jne cssq_err
	movb 0x21(%di), %al
	movb %al, %cs:(Media_types - USB2DRI)
	movw 0x22(%di), %ax
	movw %ax, %cs:(Capacity - USB2DRI)(%bx)
	movw 0x26(%di), %ax
	movw %ax, %cs:(Capacity + 2 - USB2DRI)(%bx)
	movl 0x28(%di), %eax
	movl %eax, %cs:(StartSector - USB2DRI)(%bx)			
	jmp 5f
#endif 
1:   
	xorl %eax, %eax
	movw $mem_mbr, %di
	call read_hdd
	jc cssq_err
	call Check_PartitionTable
	jc 2f

	movw $4, %cx
1:
	cmpb $0x80, 0x1be(%di)
	je 1f
	addw $0x10, %di
	loop 1b
	
	movw $mem_mbr, %di
	movw $4, %cx
1:
	movl 0x1c6(%di), %eax
	orl %eax, %eax
	jne 1f
	addw $0x10, %di
	loop 1b
	jmp 2f
1:
	subw $mem_mbr, %di
	shrw $4, %di
	movw %di, %bp
  movw $mem_pbr, %di
  call read_hdd
  jmp 1f
2:
  movw $mem_mbr, %di
  movl 0x1c(%di), %eax
1:
	movl %eax, %cs:(StartSector - USB2DRI)(%bx)

  orb %dl, %dl
	jne 2f
	movb %cs:(fd_num - USB2DRI), %ch
  cmpb $0, %ch
  je 2f
  xorb %cl, %cl
  movw $mem_fd, %si
1:
  movw $mem_mbr, %di
	call Sector_comparison
	jnc 3f
  movw $mem_pbr, %di
  call Sector_comparison
  jc 4f
  movw %bp, %fs:(0x8208+2)
  movw %bp, %fs:(0x829c+2)
  movw %bp, %fs:(0x831c+2)
  jmp 3f
4:
  decb %ch
  je 2f
  addw $0x200, %si
  incb %cl
  jmp 1b
2:
  movb %cs:(hd_num - USB2DRI), %ch
  cmpb $0, %ch
  je 2f
  movb $0x80, %cl
  movw $mem_hd, %si
  movw $mem_mbr, %di
1:
  call Sector_comparison
  jnc 3f
  decb %ch
  je 2f
  addw $0x200, %si
  incb %cl
  jmp 1b
2:
  call Search_drive
3:
	testb $1, %cs:(map_num - USB2DRI)
	je not_map
	cmpb $0x80, %cl
	jne not_map_80
	movb $0, %cs:(Number - USB2DRI)(%bx)
	movb $0, %fs:0x8280
	movb $0, %fs:0x82a0
	movb $0, %fs:0x8318
	jmp 5f
not_map_80:
	decb %cl
not_map:
	movb %cl, %cs:(Number - USB2DRI)(%bx)
5:
	testb $1, %cs:(map_num - USB2DRI)
	je cssq_end_1
	decb %fs:(0x0475)
cssq_end_1:
	clc
cssq_end_2:
	popw %es
	popw %ds
	popaw
	ret	
cssq_err:	
	stc
	jmp cssq_end_2
	
	
	
Drive_enumerate:   
	xorw %bx, %bx
	xorw %si, %si
	movb $0, (_lun - USB2DRI)
	movb $0, (EXT_C(max_dri) - USB2DRI)
	movzbw (max_dev - USB2DRI), %bp
0:
	cmpb $0xf0, (MaxLUN - USB2DRI)(%bx)
	jb 1f
5:
	decw %bp
	je 4f
	addw $DevLen, %bx
	jmp 0b
1:
	movb (DevID - USB2DRI)(%bx), %al
	movb %al, (_id - USB2DRI)
#if cd
	call Remove_CBWCB
	movb $0x80, (bmCBWFlags - USB2DRI)			
	movb $0x24, (dCBWDataTransferLength - USB2DRI)			
	movw $(Cache - USB2DRI), %ax		
	movw %ax, (TOffset - USB2DRI)	
	movw %ds, (TSection - USB2DRI)
	movb $0x12, (CBWCB - USB2DRI)	
	movb $0x24, (CBWCB + 4 - USB2DRI)
	call SCSI_Command
	cmpb $0, (Return - USB2DRI)
	jne 1f
	movb (Cache - USB2DRI), %al	
	andb $0x1f, %al
	cmpb $0x1f, %al
	je 1f		
	movb %al, (_type - USB2DRI)
#endif
	pushw %si
#if cd
	movw $900, %si
	cmpb $5, %al
	je csdyd
#endif
	movw $5, %si
csdyd:	
	call Remove_CBWCB
	call SCSI_Command
	cmpb $0, (Return - USB2DRI)
	je 2f
	decw %si
	je 3f
	movw $100, %ax
	call Delay
	jmp csdyd	
3:
	popw %si
	jmp 1f

2:
	popw %si
	call Remove_CBWCB
	movb $0x80, (bmCBWFlags - USB2DRI)			
	movb $8, (dCBWDataTransferLength - USB2DRI)			
	movw $(Cache - USB2DRI), %ax		
	movw %ax, (TOffset - USB2DRI)	
	movw %ds, (TSection - USB2DRI)
	movb $0x25, (CBWCB - USB2DRI)	
	call SCSI_Command
	cmpb $0, (Return - USB2DRI)
	jne 1f
	
	movl (Cache - USB2DRI), %eax
	orl %eax, %eax
	je 1f
	xchgb %ah, %al
	rorl $0x10, %eax
	xchgb %ah, %al
	movl %eax, (Capacity - USB2DRI)(%si) 
	
	movw (Cache - USB2DRI + 6), %ax
	xchgb %ah, %al
	movw %ax, (SectorSize - USB2DRI)(%si)
	movw %ax, (Sector_Size - USB2DRI)

	call Define_disk_No	
	jc 1f
	
	movb (_id - USB2DRI), %al
	movb %al, (DriID - USB2DRI)(%si) 

	movb (_lun - USB2DRI), %al
	movb %al, (DriLUN - USB2DRI)(%si)
#if cd
	movb (_type - USB2DRI), %al
	movb %al, (DriType - USB2DRI)(%si)
#endif	
	incb (EXT_C(max_dri) - USB2DRI)
	cmpb $4, (EXT_C(max_dri) - USB2DRI)
	jae 4f
	addw $DriveLen, %si     
1:
	movb (_lun - USB2DRI), %al
	cmpb (MaxLUN - USB2DRI)(%bx), %al 
	jae 1f
	incb (_lun - USB2DRI)
	jmp 0b 
1:
	movb $0, (_lun - USB2DRI)
	jmp 5b
4:        
	ret       	

HUB_PORT:
	.byte	0
HUB__ID:
	.byte	0

usb_probe:
	pushw %cx
	movb (DevID - USB2DRI)(%bx), %al 
	movb %al, (_id - USB2DRI)
	movw $(Cache - USB2DRI), %di	
	
	movw $(Get_Hub - USB2DRI), %si
	call Control_transfer
	movzbw 2(%di), %cx
	movw %cx, (n_ports - USB2DRI)
2:
	movw $(Set_HubPort_Power - USB2DRI), %si
	call Control_transfer_hub 
	movw $200, %ax
	call Delay
	loop 2b
	movw $200, %ax
	call Delay
	movw (n_ports - USB2DRI), %cx
2:
	movw $(Get_HubPort_Status - USB2DRI), %si
	call Control_transfer_hub 
	testw $1, (%di)
	jne 4f		
3:
	decw %cx
	jne 2b
	jmp 1f
4:
	call Port_Reset_hub
	movw $(Get_HubPort_Status - USB2DRI), %si
	call Control_transfer_hub
	testw $2, (%di)
	je 3b
	testw $0x400,(%di)
	je 3b
				
	movb %cl, (HUB_PORT - USB2DRI)
	movb (_id - USB2DRI), %al
	movb %al, (HUB__ID - USB2DRI)
	pushw	%bx
	pushw (_id - USB2DRI)
	call Fill_device_structure
	popw (_id - USB2DRI)
	popw %bx
	
	movw $(Cache - USB2DRI), %di	
	movb (_id - USB2DRI), %al
	jmp 3b
1:
	popw %cx
	ret

	
Fill_device_structure:
	pushw %cx
	cmpb $8, (max_dev - USB2DRI)
	jae jscl	
	movb (max_dev - USB2DRI), %dl
	incb %dl
	movb %dl, (_id - USB2DRI)
	call Determine_device_address

	movb (HUB_PORT - USB2DRI), %al
	orb %al, %al
	je 1f
	movb %al, (Port_Num_Reg - USB2DRI)(%bx)
	movb (HUB__ID - USB2DRI), %al
	movb %al, (HUB_id - USB2DRI)(%bx)
	jmp 2f
1:	
	movw %si, (Port_Num_Reg - USB2DRI)(%bx)
2:
	movw $(Cache - USB2DRI), %di
	movw $(Get_device - USB2DRI), %si
	movb $0x40, (ConSize - USB2DRI)(%bx)
	call Control_transfer
	cmpb $0, (Status - USB2DRI)
	jne jscl
	
	call szsbdz
	incb (max_dev - USB2DRI)

	pushw 4(%di)
	movw $(Get_Configuration - USB2DRI), %si  
	call Control_transfer
	popw %cx
	cmpb $0, (Status - USB2DRI)
	jne 5f
	movb 2(%di), %al
	movb %al, (Config_Length - USB2DRI)
pz_cl:
	movb 5(%di), %al
	movb %al, (Set_Configuration + 2 - USB2DRI)
	cmpb $0, %cl
	je pdlx
	cmpb $9, %cl
	je pdlx
	call szpz
	jmp 5f
pdlx:				
	movzbw (%di), %ax
	subb %al, (Config_Length - USB2DRI)
	je 5f
	addw %ax, %di
	movb 1(%di), %al
	cmpb $5, %al
	je dd_cl
	cmpb $2, %al
	je pz_cl
	cmpb $4, %al
	jne pdlx
	movb $0, (Interface_ok - USB2DRI)
jk_cf:
	cmpw $9, 5(%di)
	je jk_cf_0
	cmpw $0x0608, 05(%di)
	jne pdlx
	cmpb $0x50, 07(%di)
	jne pdlx
jk_cf_0:
	movb $1, (Interface_ok - USB2DRI)
	movb 4(%di), %al
	movb %al, (Num_Endpoint - USB2DRI)
	jmp pdlx
dd_cl:
	cmpb $1, (Interface_ok - USB2DRI)
	jne pdlx
	movb 02(%di), %dl
	movw 04(%di), %bp
	movb 03(%di), %dh
	cmpb $2, %dh
	jne 3f

	testb $0x80, %dl 
	je 2f 

	movb %dh, (InProperty - USB2DRI)(%bx) 
	movb %dl, (InAddress - USB2DRI)(%bx)
	movw %bp, (InSize - USB2DRI)(%bx)
	jmp 3f
2:     
	mov %dh, (OutProperty - USB2DRI)(%bx)
	mov %dl, (OutAddress - USB2DRI)(%bx)
	mov %bp, (OutSize - USB2DRI)(%bx) 
3:
	decb (Num_Endpoint - USB2DRI)
	jne pdlx									
4: 
	call szpz
	cmpb $0, (OutProperty - USB2DRI)(%bx) 
	jne getlun
	movb $0xff, (MaxLUN - USB2DRI)(%bx)
	jmp jscl
getlun:		
	mov $(Get_lun - USB2DRI), %si 
	call Control_transfer
	movb (Cache - USB2DRI), %al
	movb %al, (MaxLUN - USB2DRI)(%bx) 
	incb (usb_count - USB2DRI)
jscl:
	popw %cx
	ret	
5:	
	movb $0xf0, (MaxLUN - USB2DRI)(%bx)
	jmp jscl
				
Device_enumerate:   
	movw $0x44, %si
	movw (n_ports - USB2DRI), %cx 
1:        
	call Read_Register
	testb $01, %al
	jne 1f 
2:       
	decw %cx 
	je 4f
	addw $04, %si
	jmp 1b
1: 
	testb $04, %ah
	jne 1f
	call Port_Reset    
	testb $04, %al
	jne 3f 
1:
	movl $0x00003002, %eax  
	call Write_Register
	movw $03, %ax
	call Delay 
	jmp 2b
3:  
	pushw %si
	movb $0, (HUB_PORT - USB2DRI)
	call Fill_device_structure
	popw %si
	jmp 2b
4:
	xorw %bx, %bx
	movw $8, %cx
cl_jxq:
	cmpb $0xff, (MaxLUN - USB2DRI)(%bx)
	je cl_jxq_1
cl_jxq_0:
	addw $DevLen, %bx
	decw %cx
	jne cl_jxq
	jmp cl_jxq_end
cl_jxq_1:
	call usb_probe
	jmp cl_jxq_0
cl_jxq_end:				   
	ret        	
	
	
Fill_descriptor:  
	movl $(GDT - USB2DRI), %eax
	addl (ds_Linear - USB2DRI), %eax
	movl %eax, (Base - USB2DRI)

	movl (Register_pci - USB2DRI), %eax 
	movw %ax, (BaseL - USB2DRI)
	shrl $0x10, %eax
	movb %al, (BaseM - USB2DRI)
	movb %ah, (BaseH - USB2DRI)
	ret	

Define_qTD1:   
	movl $(qh - USB2DRI), %eax 
	addl (ds_Linear - USB2DRI), %eax
	testb $0x1f, %al
	je 1f
	andl $0xffffffe0, %eax
	addl $0x20, %eax
1:
	movl %eax, (qTD1_Linear - USB2DRI) 
	subl (ds_Linear - USB2DRI), %eax
	movw %ax, (qTD1 - USB2DRI)
	ret 	

set_410_1:
	orb $1, %fs:0x0410
	andb $0x3f, %fs:0x0410
	ret
	
	. = USB2DRI + 0x13c0
			
/* ********************  USB2DRI end  ****************************** */ 



//#define CDROM_INIT
#ifdef CDROM_INIT

/* unsigned long
 * init_atapi() : return number of atapi cdrom devices.
 */

ENTRY(init_atapi)

	.code32

	pushl	%ebp
	pushl	%ebx

	call	EXT_C(prot_to_real)	/* enter real mode */

	.code16

	sti		/* enable interrupt for ATAPI */

	cld

	/* set CS base to be int13_handler */
#if 0
	ljmp	$((ABS(int13_handler)) >> 4), $(1f - int13_handler)
#else
	.byte	0xEA
	.word	1f - int13_handler
	.word	(ABS(int13_handler)) >> 4
#endif
1:
	pushaw
	pushw	%es
	pushw	%ds

	pushw	%cs
	popw	%ds
	pushw	%cs
	popw	%es
	xorw	%ax, %ax

	# initialize variables

	movw	$(start_of_atapi_data - int13_handler), %di
	movw	$(end_of_atapi_data - start_of_atapi_data), %cx
	repz stosb

	decw	atapi_cur_dev - int13_handler	# device serial number initialized to 0xFFFF

	#; begin init_timer

	call	read_bios_time
	movw	%ax, %bx		#; store initial timer in BX
1:
	call	read_bios_time
	cmpw	%ax, %bx		#; timer just changed?
	je	1b			#; no, continue.

	/* now AX=BX+1 */

	movw	%ax, %bx		#; store initial timer in BX
	xorl	%ecx, %ecx
1:
	call	read_bios_time
	cmpw	%ax, %bx		#; timer changed?
	addr32 loope	1b

	negl	%ecx

	/* ECX loops=a tick=1/18.2second=1000/18.2(=54.9) milliseconds */

	movl	%ecx, %eax
	xorl	%edx, %edx
	xorl	%ecx, %ecx
	movw	$55, %cx		# a tick is about 55 milliseconds
	divl	%ecx

	/* EAX=loops per millisecond */

	movl	%eax, delay_repeat_num - int13_handler

	#; end init_timer

	call	init_atapi_cdroms

	//movw	$-1, atapi_cur_dev - int13_handler	# device serial number initialized to 0xFFFF

	popw	%ds
	popw	%es

	movw	%sp, %bp
	movw	%cx, 8(%bp)	#; 8(%bp) is old BX on stack !
	popaw

	ljmp	$0, $ABS(1f)
1:

	call	EXT_C(real_to_prot)
	.code32

	sti

	movzwl	%bx, %eax

	popl	%ebx
	popl	%ebp
	ret

init_atapi_cdroms:

	.code16

#; return: CF=0 success, CX=number of cdroms
#;         CF=1 failed, no cdrom found
#;	push cs
#;	pop es

	call	reg_probe	#; return cx = number of atapi devices

	orw	%cx, %cx
	jnz	1f

	call	reg_probe	#; return cx = number of atapi devices

	orw	%cx, %cx
	jnz	1f

	stc
	ret

1:
#if 1
	movw	$atapi_dev_base - int13_handler, %si	#; array of ATAPI reg pointer and dev
	movw	$atapi_dev_base_bak - int13_handler, %di	#; for CDROMs

	cld

	pushw	%si
	pushw	%di

	xorw	%ax, %ax
	xorw	%bx, %bx
1:
	call	select_atapi_force	#;input: ax = device serial number
					#;return: cf=0 success, cf=1 failed


	jc	2f

	call	check_atapi_cdrom	#;return: cf=0 is cdrom, cf=1 not cdrom

	jc	2f

	/* It is CDROM */

	incw	%bx			#;count the cdroms
	movsw				#;store CDROM base pointer
	movsw				#;store CDROM device number
	subw	$4, %si

	# debug print the reg and dev
	pushw	%bx

	movw	(%si), %bx		# BX=reg group pointer

	pushw	2(%si)			# CDROM device number
	pushw	2(%bx)			# base port of control block regs, i.e., device control port
	pushw	(%bx)			# base port of command block regs, i.e., data port
	pushw	$cdrom_reg_dev - int13_handler		# the format string
	call	realmode_printf
	addw	$8, %sp			# adjust the stack pointer

	popw	%bx

2:
	addw	$4, %si
	incw	%ax			#; try next ATAPI device
	loop	1b

	popw	%si			#; points to atapi_dev_base_bak, which is for CDROMs
	popw	%di			#; points to atapi_dev_base, which is for ATAPIs

	movw	$64, %cx		#; overwrite ATAPIs with CDROMs
	repz movsb

	movb	%bl, EXT_C(atapi_dev_count) - int13_handler
	movb	EXT_C(min_cdrom_id) - int13_handler, %cl
	addb	%bl, %cl
	decw	%cx
	movb	%cl, max_cdrom_id - int13_handler
	movw	%bx, %cx
#endif

	clc
	ret

cdrom_reg_dev:
	.ascii	"CDROM device found: Data=%X, Ctrl=%X, dev=%X(Note: 0=master, 1=slave)\r\n\0"


/* Called by init_atapi_cdroms
 * Calls reg_setup_base_addr, reg_reset
 */
reg_probe:

	.code16

#; return cx = number of atapi devices
	pushw	%ax
	pushw	%bx
	pushw	%si
	pushw	%di
	cld
	movb	$0, EXT_C(atapi_dev_count) - int13_handler		# reset counter
	movw	$reg_base_addr - int13_handler, %bx		# BX points to base address array
	movw	$atapi_dev_base - int13_handler, %di		# will store base pointers and device numbers
1:
	call	reg_setup_base_addr	# setup the base reg addresses(reg_addr) for reg base address BX

	#; begin reg_probe_exist
	movb	$cmd_DC, %al
	movw	reg_addr - int13_handler + (8 * 2), %dx		#; CB_DC
	outb	%al, %dx

	movw	$0, %ax

	call	reg_probe_dev_exist

	movb	%al, reg_dev_info - int13_handler	# master device existence
	movw	$1, %ax

	call	reg_probe_dev_exist

	movb	%al, reg_dev_info - int13_handler + 1	# slave device existence

	#; end reg_probe_exist

	call	reg_reset

	xor	%si, %si		# SI=0 for master
2:
//	movw	%si, %ax
//	call	reg_probe_dev_exist

	cmpb	$0, (reg_dev_info - int13_handler)(%si)		# check existence
//	cmpb	$0, %al						# check existence
	je	3f						# device not exist, so skip

	/* The device exists, so do a further check for device type.  */

	movw	%si, %ax

	#; begin reg_check_dev_type

	#; call after a reset

	call	__reg_select_dev

	movw	reg_addr - int13_handler + (2 * 2), %dx	#; CB_SC = interrupt reason register
	inb	%dx, %al
	movb	%al, %ah

	movw	reg_addr - int13_handler + (3 * 2), %dx	#; CB_SN = reserved for SAM TAG byte
	inb	%dx, %al

	cmpw	$0x0101, %ax		# success?
	movw	$1, %ax			# REG_CONFIG_TYPE_UNKN
	jne	4f			# No. The device type is unknown.

	movw	reg_addr - int13_handler + (4 * 2), %dx	#; CB_CL = byte count low
	inb	%dx, %al
	movb	%al, %ah

	movw	reg_addr - int13_handler + (5 * 2), %dx	#; CB_CH = byte count high
	inb	%dx, %al

	cmpw	$0x14EB, %ax		# Is ATAPI?
	je	5f			# yes.

	testb	%al, %al		# 0 is for ATA
	movw	$1, %ax			# REG_CONFIG_TYPE_UNKN
	jnz	4f
					# AX=1

	movw	reg_addr - int13_handler + (7 * 2), %dx		#; CB_STAT
	inb	%dx, %al
					# AH=0
	//if AL==0, then AX=1=REG_CONFIG_TYPE_UNKN, else AX=2=REG_CONFIG_TYPE_ATA
	testb	%al, %al
	setne	%al
	incw	%ax
	jmp	4f
5:
	movw	$3, %ax			# REG_CONFIG_TYPE_ATAPI
4:

	#; end reg_check_dev_type


	movb	%al, (reg_dev_info - int13_handler)(%si)	#; added (%si) recently
	cmpb	$3, %al			# Is ATAPI?
	jne	3f			# no, ignore it.

	#; It is ATAPI, so add it to the list at atapi_dev_base
	incb	EXT_C(atapi_dev_count) - int13_handler	# count it
	movw	%bx, %ax		# store the pointer to the base reg pair
	stosw
	movw	%si, %ax		# store the device number(master/slave)
	stosw

	# debug print the reg and dev
	pushw	%si
	pushw	2(%bx)			# base port of control block regs, i.e., device control port
	pushw	(%bx)			# base port of command block regs, i.e., data port
	pushw	$atapi_reg_dev - int13_handler		# the format string
	call	realmode_printf
	addw	$8, %sp			# adjust the stack pointer
3:
	incw	%si			# SI=1 for slave
	cmpw	$2, %si
	jb	2b

	addw	$4, %bx			# try next group of base reg
	#; incw	%bx			# try next group of base reg
	#; incw	%bx
	cmpw	$0, (%bx)		# end?
	jnz	1b			# no, probe next reg group

	movzbw	EXT_C(atapi_dev_count) - int13_handler, %cx
	movw	%cx, %bx
	decw	%bx
	movb	EXT_C(min_cdrom_id) - int13_handler, %al
	addb	%bl, %al
	movb	%al, max_cdrom_id - int13_handler
	popw	%di
	popw	%si
	popw	%bx
	popw	%ax
	ret

atapi_reg_dev:
	.ascii	"ATAPI device found: Data=%X, Ctrl=%X, dev=%X(Note: 0=master, 1=slave)\r\n\0"


check_atapi_cdrom:

	.code16

#;return: cf =0 is cdrom, cf =1 not cdrom
	pushaw
#;	push es
#;	push cs
#;	pop es
#;	mov di, atapi_devinfo
	call	inquiry_atapi
	jc	1f
	clc

#define	ATATYPE_CDR	0x4	//#; Write-once device
#define	ATATYPE_CD	0x5	//#; CD-ROM device

	cmpb	$ATATYPE_CD, %al
	je	1f
	cmpb	$ATATYPE_CDR, %al
	je	1f
	stc
1:
#;	pop es
	popaw
	ret


inquiry_atapi:

	.code16

#;input: es:di -> atapi_devinfo
#;return: cf =0 success, al = device type,
#;        cf =1 fail
#;	save si, di, cx
	call	clear_atapi_buffer
	movb	$0x12, atapi_cmd_buffer - int13_handler
	movb	$128, atapi_cmd_buffer - int13_handler + 4

	# invoke reg_packet,byte,0,cs, atapi_tmp_buffer, 128
	pushw	$128
	pushw	$atapi_tmp_buffer - int13_handler
	pushw	%cs
	pushw	$0
	call	reg_packet
	addw	$8, %sp

	orw	%ax, %ax
	jnz	1f

	movb	atapi_tmp_buffer - int13_handler, %al
	testb	$0xe0, %al
	jnz	1f

	andb	$0x1f, %al

#if 0
	mov [es:di + struc_atapi_devinfo.dev_type], al

	mov ah, [atapi_tmp_buffer+7]
	mov [es:di + struc_atapi_devinfo.dev_flags], ah

	add di, struc_atapi_devinfo.vender_id
	mov si, atapi_tmp_buffer + 8
	mov cx, 24
	cld
	rep movsb
#endif
	clc
	ret
1:
	stc
	ret


reg_probe_dev_exist:

	.code16

#; input ax=dev
#; return ax = 1: exist		# <----- "ax=1" should be "al=1"
	call	__reg_select_dev

	# outbytes CB_SC,0x55,CB_SN,0xaa,CB_SC,0xaa,CB_SN,0x55,CB_SC,0x55
	movb	$0x55, %al
	movw	reg_addr - int13_handler + (2 * 2), %dx	#; CB_SC = interrupt reason register
	outb	%al, %dx

	movb	$0xAA, %al
	movw	reg_addr - int13_handler + (3 * 2), %dx	#; CB_SN = reserved for SAM TAG byte
	outb	%al, %dx

	movb	$0xAA, %al
	movw	reg_addr - int13_handler + (2 * 2), %dx	#; CB_SC = interrupt reason register
	outb	%al, %dx

	movb	$0x55, %al
	movw	reg_addr - int13_handler + (3 * 2), %dx	#; CB_SN = reserved for SAM TAG byte
	outb	%al, %dx

	movb	$0x55, %al
	movw	reg_addr - int13_handler + (2 * 2), %dx	#; CB_SC = interrupt reason register
	outb	%al, %dx

	movb	$0xAA, %al
	movw	reg_addr - int13_handler + (3 * 2), %dx	#; CB_SN = reserved for SAM TAG byte
	outb	%al, %dx

	movw	reg_addr - int13_handler + (2 * 2), %dx	#; CB_SC = interrupt reason register
	inb	%dx, %al
	movb	%al, %ah

	movw	reg_addr - int13_handler + (3 * 2), %dx	#; CB_SN = reserved for SAM TAG byte
	inb	%dx, %al

	cmpw	$0x55AA, %ax
	sete	%al
	ret

#endif	/* CDROM_INIT */


/*
 * chain_stage1(segment, offset, part_table_addr)
 *
 *  This starts another stage1 loader, at segment:offset.
 */

ENTRY(chain_stage1)

	.code32

	/* no need to save anything, just use %esp */

	/* store %ESI, presuming %ES is 0 */
	movl	0xc(%esp), %esi

	/* store new offset */
	movl	0x8(%esp), %eax
	movl	%eax, offset

	/* store new segment */
	movw	0x4(%esp), %ax
	movw	%ax, segment

	/* set up to pass boot drive */
	movb	EXT_C(boot_drive), %dl

	call	EXT_C(prot_to_real)

	.code16

	//sti		/* it is not bad keeping interrupt off */
	sti		/* for hardware interrupt or watchdog */

#ifdef ABSOLUTE_WITHOUT_ASTERISK
	DATA32	ADDR32	ljmp	(offset)
#else
	DATA32	ADDR32	ljmp	*(offset)
#endif


/*
 *  These next two routines, "real_to_prot" and "prot_to_real" are structured
 *  in a very specific way.  Be very careful when changing them.
 *
 *  NOTE:  Use of either one messes up %eax and %ebp.
 */

ENTRY(real_to_prot)

	.code16

	cli

	/* load the GDT register */
	xorw	%ax, %ax
	movw	%ax, %ds
	lgdt	gdtdesc

	/* turn on protected mode */
	movl	%cr0, %eax
	andl	$0x0000FFF3, %eax
	orb	$1, %al
	movl	%eax, %cr0

	/* jump to relocation, flush prefetch queue, and reload %cs */
	ljmp	$PROT_MODE_CSEG, $protcseg

	/*
	 *  The ".code32" directive only works in GAS, the GNU assembler!
	 *  This gets out of "16-bit" mode.
	 */

	.code32

protcseg:
	/* reload other segment registers */
	movw	$PROT_MODE_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

#;	/* put the return address in a known safe location */
#;	movl	(%esp), %eax
#;	movl	%eax, STACKOFF

#;	/* get protected mode stack */
#;	movl	protstack, %eax
#;	movl	%eax, %esp
#;	movl	%eax, %ebp

#;	/* get return address onto the right stack */
#;	movl	STACKOFF, %eax
#;	movl	%eax, (%esp)

	lidt	pmode_idtdesc

	/* zero %eax */
	xorl	%eax, %eax

	/* return on the old (or initialized) stack! */
	.byte	0x66	# data32	/* YES!! only a 16-bit RET!! */
	ret


ENTRY(prot_to_real)

	.code32

	cli

	/* just in case, set GDT */
	lgdt	gdtdesc

#;	/* save the protected mode stack */
#;	movl	%esp, %eax
#;	movl	%eax, protstack

#;	/* get the return address */
#;	movl	(%esp), %eax
#;	movl	%eax, STACKOFF

#;	/* set up new stack */
#;	movl	$STACKOFF, %eax
#;	movl	%eax, %esp
#;	movl	%eax, %ebp

	/* set up segment limits */
	movw	$PSEUDO_RM_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* this might be an extra step */
	ljmp	$PSEUDO_RM_CSEG, $tmpcseg	/* jump to a 16 bit segment */

tmpcseg:
	.code16

	/* clear the PE bit of CR0 */
	movl	%cr0, %eax
	//andl 	$CR0_PE_OFF, %eax
	andl 	$0x0000FFF2, %eax
	movl	%eax, %cr0

	/* flush prefetch queue, reload %cs */
	ljmp	$0, $realcseg

realcseg:
	/* we are in real mode now
	 * set up the real mode segment registers : DS, SS, ES
	 */
	/* zero %eax */
	xorl	%eax, %eax

	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* restore interrupts */
	/* oh, don't enable interrupt when we are controlling gateA20 */
	//sti
	lidt	realmode_idtdesc

	/* return on new stack! */
	DATA32	ret		/* 32-bit RET!! */

	
	/*********** begin initialising page maps ***********/
	/* PML4 base, with only one entry(=512G) */
	
mem64_paging_initialized:
	.byte	0
	
mem64_paging_init:	
	.code32

	movl	ABS(EXT_C(page_map_start)), %ebx
	movl	%ebx, %edi
	movl	%ebx, %eax
	addl	$0x1007, %eax
	stosl					# first PML4 table entry, lo
	xorl	%eax, %eax
	stosl					# first PML4 table entry, hi

	/* PDP table, with 512 entries(=512G) */
	/* entry 0x000 starts at EBX+0x002000 */
	/* entry 0x001 starts at EBX+0x003000 */
	/* .................................. */
	/* entry 0x1FF starts at EBX+0x201000 */

	movl	%ebx, %edi
	addl	$0x1000, %edi		# PDP table starting at EBX+0x1000
	movl	$512, %ecx
1:
	movl	$(512+2), %eax
	subl	%ecx, %eax		# EAX=(entry number + 2)
	shll	$12, %eax
	addl	%ebx, %eax
	orb	$0x07, %al
	stosl					# PDP table entry, lo
	xorl	%eax, %eax
	stosl					# PDP table entry, hi
	loop	1b

	/* PD table, identity mapping, 2M page size, 512*512entries=512G */
	movl	%ebx, %edi
	addl	$0x2000, %edi		# PD table starting at EBX+0x2000
	movl	$(512*512), %ecx
1:
	xorl	%edx, %edx
	movl	$(512*512), %eax
	subl	%ecx, %eax
	shldl	$21, %eax, %edx
	shll	$21, %eax
	orb	$0x87, %al
	stosl					# PD table entry, lo
	movl	%edx, %eax
	stosl					# PD table entry, hi
	loop	1b
	orb $1, ABS(mem64_paging_initialized)
	ret
	/***********  end  initialising page maps ***********/


/*
 *   int mem64 (int func, __u64 dest, __u64 src, __u64 len)
 *
 *	SRC and DEST should better align 8 for efficiency.
 *
 *   input:
 *		func = 1 for memmove, 2 for memcmp, 3 for memset
 *
 */

ENTRY(mem64)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	#; +28	len
	#; +20	src
	#; +12	dest
	#;  +8	func
	#;  +4	EIP
	#; ebp	EBP
	#;  -4	ESI
	#;  -8	EDI
	#; -12	EBX

	pushl	%esi
	pushl	%edi
	pushl	%ebx
	pushfl
	cli

	/* backup cr3, cr4 */
	movl	%cr3, %eax
	movl	%eax, ABS(old_cr3)		# save cr3
	movl	%cr4, %eax
	movl	%eax, ABS(old_cr4)		# save cr4

	cmpb $1, ABS(mem64_paging_initialized)
	je 1f
	call mem64_paging_init
1:
#if 0
	/* XXX: page maps can initialise only once for efficiency. */

	/*********** begin initialising page maps ***********/
	/* PML4 base, with only one entry(=512G) */
	cld
	movl	ABS(EXT_C(page_map_start)), %ebx
	movl	%ebx, %edi
	movl	%ebx, %eax
	addl	$0x1007, %eax
	stosl					# first PML4 table entry, lo
	xorl	%eax, %eax
	stosl					# first PML4 table entry, hi

	/* PDP table, with 512 entries(=512G) */
	/* entry 0x000 starts at EBX+0x002000 */
	/* entry 0x001 starts at EBX+0x003000 */
	/* .................................. */
	/* entry 0x1FF starts at EBX+0x201000 */

	movl	%ebx, %edi
	addl	$0x1000, %edi		# PDP table starting at EBX+0x1000
	movl	$512, %ecx
1:
	movl	$(512+2), %eax
	subl	%ecx, %eax		# EAX=(entry number + 2)
	shll	$12, %eax
	addl	%ebx, %eax
	orb	$0x07, %al
	stosl					# PDP table entry, lo
	xorl	%eax, %eax
	stosl					# PDP table entry, hi
	loop	1b

	/* PD table, identity mapping, 2M page size, 512*512entries=512G */
	movl	%ebx, %edi
	addl	$0x2000, %edi		# PD table starting at EBX+0x2000
	movl	$(512*512), %ecx
1:
	xorl	%edx, %edx
	movl	$(512*512), %eax
	subl	%ecx, %eax
	shldl	$21, %eax, %edx
	shll	$21, %eax
	orb	$0x87, %al
	stosl					# PD table entry, lo
	movl	%edx, %eax
	stosl					# PD table entry, hi
	loop	1b

	/***********  end  initialising page maps ***********/
#endif
	movl	ABS(EXT_C(page_map_start)), %eax
	movl	%eax, %cr3			# load new PML4 base

	movl	%cr4, %eax
	orb	$0x30, %al			# 0x80=PGE, 0x20=PAE, 0x10=PSE
	movl	%eax, %cr4			# load new cr4

	/* rdmsr will change EDX:EAX */
	movl	$0xC0000080, %ecx		# specify EFER MSR
	rdmsr					# enable long mode(EFER.LME=1)
	orb	$0x1, %ah
	wrmsr

	movl	%cr0, %eax
	orl	$0x80000000, %eax	# Activate long mode by enabling paging
	movl	%eax, %cr0

	ljmp	$32, $ABS(1f)		# ljmp to enter 64-bit mode

	.code64
1:

	/* 28(%ebp) = len */
	/* 20(%ebp) = src */
	/* 12(%ebp) = dest */
	/*  8(%ebp) = func */

	movl	%ebp, %ebp		# clear upper 32-bit of %rbp
	movl	8(%rbp), %eax
	testl	%eax, %eax
	jz	1f
	cmpl	$3, %eax
	ja	1f
	cld
	movq	12(%rbp), %rdi
	movq	20(%rbp), %rsi
	movq	28(%rbp), %rcx
	/* AL=1, 2, 3 */
	cmpb	$1, %al
	jne	2f
	/* AL=1, memmove */
	cmpq	%rdi, %rsi
	jb	3f
	movb	%cl, %al
	shrq	$3, %rcx
	repz movsq			/* RCX=0 */
	andb	$7, %al
	jz	1f
	movb	%al, %cl
	repz movsb
	jmp	1f
3:
	std
	addq	%rcx, %rsi
	addq	%rcx, %rdi
	decq	%rsi
	decq	%rdi
	movq	%rcx, %rax		/* save RCX to RAX */
	xorq	%rcx, %rcx
	movb	%al, %cl
	andb	$7, %cl			/* if RCX=0, rep will do nothing */
	repz movsb			/* RCX=0 */
	movq	%rax, %rcx		/* restore RCX from RAX */
	shrq	$3, %rcx
	subq	$7, %rsi		/* align 8 */
	subq	$7, %rdi		/* align 8 */
	repz movsq			/* RCX=0 */
	cld
	jmp	1f
2:
	cmpb	$2, %al
	jne	2f
	/* AL=2, memcmp */
	movb	%cl, %al
	shrq	$3, %rcx
	repz cmpsq
	jnz	1f
	andb	$7, %al			/* RCX=0 */
	jz	1f
	movb	%al, %cl
	repz cmpsb
	jmp	1f
2:
	/* AL=3, memset */
	movl	%esi, %eax	/* get AL for the char to write */
	movzbq	%al, %rax
	testb	%al, %al
	jz	2f
	movb	%al, %ah
	movw	%ax, %si
	shll	$16, %eax
	movw	%si, %ax
	movl	%eax, %esi
	shlq	$32, %rax
	movl	%esi, %eax
2:
	movb	%cl, %dl
	shrq	$3, %rcx
	repz stosq			/* RCX=0 */
	andb	$7, %dl
	jz	1f
	movb	%dl, %cl
	repz stosb
1:
	setnz	%al
	movzbl	%al, %ebx		# EBX=return value

	/* back to protected mode */
	ljmp	*ABS(1f)
1:
	.long	ABS(1f)
	.word	40			# 32-bit code 4GB limit

	.code32
1:

	/* now in 32-bit compatability mode */

	movl	%cr0, %eax
	andl	$0x7FFFFFFF, %eax	# disable paging, leaving long mode
	movl	%eax, %cr0

	movl	%cs:ABS(old_cr3), %eax
	movl	%eax, %cr3		# restore cr3, flush the TLB

	/* now in 32-bit protected mode */

	/* rdmsr will change EDX:EAX */
	movl	$0xC0000080, %ecx	# specify EFER MSR
	rdmsr				# disable long mode(EFER.LME=0)
	andb	$0xFE, %ah
	wrmsr

	movl	%cs:ABS(old_cr4), %eax
	movl	%eax, %cr4		# restore cr4

	/* reload segment registers */

	movw	$16, %ax		# 32-bit data 4GB limit
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* ESP not touched, so it need not restore. */
	//movl	%ebp, %esp

	xchgl	%eax, %ebx		# EAX=return value
	popfl
	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp

	ret

/*
 *   int biosdisk_int13_extensions (unsigned ax, unsigned drive, void *dap, unsigned ssize)
 *
 *   Call IBM/MS INT13 Extensions (int 13 %ax=AX) for DRIVE. DAP
 *   is passed for disk address packet. If an error occurs, return
 *   non-zero, otherwise zero.
 */

ENTRY(biosdisk_int13_extensions)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	#; +20	ssize
	#; +16	dap
	#; +12	drive
	#;  +8	 ax
	#;  +4	EIP
	#; ebp	EBP

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	0x14(%ebp), %ecx	#; sector size(512 or 2048)

	/* compute the address of disk_address_packet */
	movl	0x10(%ebp), %eax	#; linear address of dap

	/* if DS can be 0x40, we can avoid AWARD BIOS bug of int13/AX=4B01 */
	subl	$0x400, %eax

	shll	$1, %eax
	movw	%ax, %si
	shrw	$1, %si			#; low 15-bit for offset

	xorw	%ax, %ax
	shrl	$5, %eax		#; segment value in AX
	addw	$0x40, %ax
	movw	%ax, %di	/* save the segment to di */

	/* drive */
	movb	0xc(%ebp), %dl
	/* ax */
	movw	0x8(%ebp), %bx
	/* enter real mode */
	call	EXT_C(prot_to_real)

	.code16

	sti	#; cli should also work for biosdisk_int13_extensions

	movw	%bx, %ax
	movw	%di, %ds

	/* set additional registers to serve buggy BIOSes. */
	pushw	%ds
	pushw	%si
	pushw	%dx
	pushw	%cx
	pushw	%bx
	//movw	%cx, %es
	//movw	%si, %di
	//movw	%si, %bx

	andb	$0xFE, %bh
	cmpb	$0x42, %bh	/* is it read/write? */
	jne	2f
#if 0
	testb	%cl, %cl	/* single sector access? */
	jz	2f		/* no */
	cmpb	$0x2, %ch	/* is it normal sector size? */
#else
	cmpw	$0x201, %cx
#endif
	stc
	je	1f
2:
#if 1
	int	$0x13
#else
	call	safe_int13
#endif

1:
	popw	%bx
	popw	%cx
	popw	%dx
	popw	%si
	popw	%ds

	jnc	1f
	movw	%bx, %ax	/* read(0x42) or write(0x43) */	
	andb	$0xFE, %ah
	cmpb	$0x42, %ah	/* is it read/write? */
	jne	2f
	cmpb	$0x2, %ch	/* is it normal sector size? */
	jne	2f
	/* try reading sectors one by one */
	movw	2(%si), %cx	/* number of sectors to read/write */
	cmpw	$2, %cx
	jb	2f
	cmpw	$0x7F, %cx
	ja	2f
	lesw	4(%si), %di	/* get buffer address in ES:DI */
	subw	$0x200, %di
	movl	8(%si), %eax	/* starting sector number lo */
	movl	12(%si), %ebp	/* starting sector number hi */
	decl	%eax
	cmpl	$-1, %eax
	jne	3f
	decl	%ebp
3:
	addw	$0x200, %di
	incl	%eax
	jnz	4f
	incl	%ebp
4:
	pushw	%ds
	pushw	%es
	pushal
	
	pushl	%ebp		/* sector number hi */
	pushl	%eax		/* sector number lo */
	pushw	%es		/* buffer segment */
	pushw	%di		/* buffer offset */
	pushw	$0x0001		/* read/write only one sector */
	pushw	$0x0010		/* packet size */

	xorw	%si, %si
	movw	%si, %ds	/* DS=SS=0 */
	movw	%sp, %si	/* DS:SI points to packet */

	movw	%bx, %ax	/* read(0x42) or write(0x43) */	
	/* set additional registers to serve buggy BIOSes. */
	movw	%di, %bx
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw			/* discard packet on the stack */

	popal
	popw	%es
	popw	%ds

	jc	1f
	loop	3b
	jmp	1f		/* CF=0, success */
2:
	stc
1:

#if 1
	/* some buggy USB BIOSes fail to clear AH on success */
	setc	%dl
#else
	movb	$1, %dl		/* set error */
	jc	1f
	movb	%ah, %dl	/* save return value */
1:
#endif

	/* clear the data segment */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es

	/* if it is not read/write operation, we can skip the A20 code. */

	andb	$0xFE, %bh
	cmpb	$0x42, %bh
	jne	1f

	/* ensure A20 is on when we come back to protected mode. */

	pushal

	movw	$0x00ff, %cx		# try so many times on failure
	movw	$0x0001, %dx		# DL=enable A20, DH=debug off

	cli	/* yes, keep interrupt off when controlling A20 */
	call	enable_disable_a20
	sti

	//sete	%dl			# DL=1 means success

	popal
1:
	/* back to protected mode */
	call	EXT_C(real_to_prot)
	.code32

	sti

	movzbl	%dl, %eax	/* return value in %eax */

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp

	ret

/*
 *   int biosdisk_standard (int ah, int drive, int coff, int hoff, int soff,
 *                          int nsec, int segment)
 *
 *   Call standard and old INT13 (int 13 %ah=AH) for DRIVE. Read/write
 *   NSEC sectors from COFF/HOFF/SOFF into SEGMENT. If an error occurs,
 *   return non-zero, otherwise zero.
 */

ENTRY(biosdisk_standard)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	/* set up CHS information */
	movl	0x10(%ebp), %eax	/* EAX = Cylinder */
	movb	%al, %ch		/* low 8 bits to CH */
	movb	0x18(%ebp), %al		/* AL = Sector */
	shlb	$2, %al			/* move low 2 bits of AH .. */
	shrw	$2, %ax			/* .. to high 2 bits of AL */
	movb	%al, %cl		/* CX is built OK */
	movb	0x14(%ebp), %dh		/* DH = Head */
	/* drive */
	movb	0xc(%ebp), %dl		/* DL = drive */
	/* segment */
	movw	0x20(%ebp), %bx		/* BX = segment */
	/* save nsec and ah to %di */
	movb	0x8(%ebp), %ah		/* AH = read/write */
	movb	0x1c(%ebp), %al		/* AL = nsec */
	movw	%ax, %di
	/* enter real mode */
	call	EXT_C(prot_to_real)

	.code16

	//sti	#; biosdisk_standard won't require sti
	sti	#; added 2006-11-30

	movw	%bx, %es
	movw	$3, %si		/* attempt at least 3 times */

1:
	pushw	%si
	pushw	%di
	pushw	%cx
	pushw	%dx
	xorw	%bx, %bx
	movw	%di, %ax
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popw	%dx
	popw	%cx
	popw	%di
	popw	%si

#if 1
	/* some buggy USB BIOSes fail to clear AH on success */
	setc	%bl
	jnc	2f
#else
	movb	%ah, %bl	/* save return value */
	jc	3f		/* check if successful */

	testb	%ah, %ah
	jz	2f
3:
#endif

	movw	%di, %ax	/* get function number */
	cmpb	$0x04, %ah	/* verify sectors? */
	je	4f		/* yes, do not retry */
	/* if fail, reset the disk system */
	pushw	%si
	pushw	%di
	pushw	%cx
	pushw	%dx
	xorw	%ax, %ax
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popw	%dx
	popw	%cx
	popw	%di
	popw	%si

	decw	%si
	jnz	1b		/* retry */
4:
	movb	$1, %bl		/* set error */
2:
	/* ensure A20 is on when we come back to protected mode. */

	pushal

	movw	$0x00ff, %cx		# try so many times on failure
	movw	$0x0001, %dx		# DL=enable A20, DH=debug off

	cli	/* yes, keep interrupt off when controlling A20 */
	call	enable_disable_a20
	sti

	//sete	%dl			# DL=1 means success

	popal
	/* back to protected mode */
	call	EXT_C(real_to_prot)
	.code32

	sti

	movzbl	%bl, %eax	/* return value in %eax */

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp

	ret


/*
 *   int check_int13_extensions (unsigned drive, unsigned lba1sector)
 *
 *   Check if LBA is supported for DRIVE. If it is supported, then return
 *   the major version and API support bits of extensions, otherwise zero.
 *   On return:
 *	EAX bit 0 = 1, LBA supported
 *		bit 8 = 0, with 127-sector readability
 *		bit 8 = 1, without 127-sector readability
 *	EAX bit 0 = 0, no LBA support
 */

ENTRY(check_int13_extensions)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movw	0xC(%ebp), %di		// lba1sector
	movb	0x8(%ebp), %dl		// drive
	/* enter real mode */
	call	EXT_C(prot_to_real)

	.code16

	sti	#; added 2006-11-30

	pushw	%dx
	pushw	%di

	movb	$0x41, %ah
	movw	$0x55aa, %bx
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popw	%di
	popw	%dx

	/* check the result */
	jc	1f
	cmpw	$0xaa55, %bx
	jne	1f

	movb	%ah, %bl	/* save the major version into %bl */

	/* check if AH=0x42 is supported if FORCE_LBA is zero */
	movb	ABS(EXT_C(force_lba)), %al
#if 0
	testb	%al, %al
#if 0
	jnz	2f
	andw	$1, %cx
	jnz	2f
#else
	setnz	%al
	orb	%al, %cl
	jmp	2f
#endif
#else
	andb	$0x01, %al	/* only use bit 0 for FORCE_LBA */
	orb	%al, %cl
	/*****************************************************************/
	/* workaround for ASRock's ConRoe865PE mainboard. See issue 153. */
	testb	$0x01, %cl
	jnz	3f
	/* read a sector with function 42h */
	/* no need to save segment registers */
	pushw	%di
	pushw	%bx
	pushw	%cx
	/* first, clear the buffer */
	movw	$0x2000, %cx	/* GEOMTUNE_SEG */
	movw	%cx, %ds
	movw	%cx, %es
	movw	$0x100, %cx	/* 1 sector */
	xorw	%ax, %ax	/* fill the first buffer with zeroes */
	xorw	%di, %di
	cld
	rep stosw
	movw	$0x100, %cx	/* 1 sector */
	decw	%ax		/* fill the second buffer with 0xFF's */
	rep stosw
	/* read the boot sector at 2000:0000 */
	/* disk address packet at 2000:8000 */
	movw	$0x8000, %di
	movw	%di, %si
	movw	$0x0010, (%di)	/* packet size and reserved byte */
	movw	$0x0001, 2(%di)	/* number of blocks to read */
	movw	$0x0000, 4(%di)	/* buffer offset */
	movw	$0x2000, 6(%di)	/* buffer segment = GEOMTUNE_SEG */
	movl	$0, 8(%di)	/* lo 32 bits of starting block number */
	movl	$0, 12(%di)	/* hi 32 bits of starting block number */
	movw	$0x4200, %ax
	pushw	%ds
	pushw	%es
	pushw	%dx
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popw	%dx
	popw	%es
	popw	%ds
	/* read the boot sector again at 2000:0200 */
	/* disk address packet at 2000:8000 */
	movw	$0x8000, %di
	movw	%di, %si
	movw	$0x0010, (%di)	/* packet size and reserved byte */
	movw	$0x0001, 2(%di)	/* number of blocks to read */
	movw	$0x0200, 4(%di)	/* buffer offset */
	movw	$0x2000, 6(%di)	/* buffer segment = GEOMTUNE_SEG */
	movl	$0, 8(%di)	/* lo 32 bits of starting block number */
	movl	$0, 12(%di)	/* hi 32 bits of starting block number */
	movw	$0x4200, %ax
	pushw	%ds
	pushw	%es
	pushw	%dx
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popw	%dx
	popw	%es
	popw	%ds
	/* compare the two buffers */
	movw	$0x0000, %si
	movw	$0x0200, %di
	movw	$0x100, %cx
	cld
	repe cmpsw
	/* if equal, account as "extended disk access functions supported" */
	sete	%al
	popw	%cx
	orb	%al, %cl
	popw	%bx
	popw	%di
3:
	/* end of workaround */
	/*****************************************************************/
	jmp	2f
#endif

1:
	xorw	%bx, %bx
	xorw	%cx, %cx
2:
	shll	$16, %ebx	#; version number in high word
	movb	%cl, %bl	#; API subset support bitmap in low word

	testb	$1, %bl		// support LBA?
	jz	1f		// no
	/* make sure if it could read 127 sectors at a time */
	/* first, check sector size */
	/* fill the 2048-byte buffer with FF's */
	movw	$0x2000, %cx	/* GEOMTUNE_SEG */
	movw	%cx, %ds
	movw	%cx, %es
	movw	$0x400, %cx	/* 4 sectors */
	xorw	%ax, %ax
	decw	%ax		/* fill the buffer with 0xFF's */
	pushw	%di
	xorw	%di, %di
	cld
	rep stosw
	popw	%di

	pushw	%ds
	pushw	%es
	pushal
	xorl	%eax, %eax
	pushl	%eax		/* sector hi */
	pushl	%eax		/* sector lo */
	pushw	$0x2000		/* buffer segment = GEOMTUNE_SEG */
	pushw	%ax		/* offset=0 */
	pushw	$0x0001		/* read 1 sector */
	pushw	$0x0010		/* packet size */
	movw	%ax, %ds	/* DS=SS=0 */
	movw	%sp, %si
	movw	$0x4200, %ax
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw			/* discard packet on the stack */
	popal
	popw	%es
	popw	%ds
	jnc	2f
	andb	$0xFE, %bl	/* no LBA */
	jmp	1f
2:
	pushw	%di
	movw	$0x600, %cx
	movw	$0x200, %di
	cld
	repz scasb
	popw	%di
	jne	1f		/* big 2048-byte sector */
	/* try again, fill the 2048-byte buffer with 00's */
	movw	$0x2000, %cx	/* GEOMTUNE_SEG */
	movw	%cx, %ds
	movw	%cx, %es
	movw	$0x400, %cx	/* 4 sectors */
	xorw	%ax, %ax	/* fill the buffer with 00's */
	pushw	%di
	movw	$0x8000, %di
	cld
	rep stosw
	popw	%di

	pushw	%ds
	pushw	%es
	pushal
	xorl	%eax, %eax
	pushl	%eax		/* sector hi */
	pushl	%eax		/* sector lo */
	pushw	$0x2000		/* buffer segment = GEOMTUNE_SEG */
	pushw	$0x8000		/* offset=0x8000 */
	pushw	$0x0001		/* read 1 sector */
	pushw	$0x0010		/* packet size */
	movw	%ax, %ds	/* DS=SS=0 */
	movw	%sp, %si
	movw	$0x4200, %ax
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw			/* discard packet on the stack */
	popal
	popw	%es
	popw	%ds
	jnc	2f
	andb	$0xFE, %bl	/* no LBA */
	jmp	1f
2:
	pushw	%di
	movw	$0x600, %cx
	movw	$0x8200, %di
	cld
	repz scasb
	popw	%di
	jne	1f		/* big 2048-byte sector */

	/* now it is 512-byte sector size */

	testw	%di, %di
	jz	2f
	testw	$0x0080, %di	/* lba127sector */
	jnz	2f
	movb	$1, %bh		/* failure, cannot read 127 sectors */
	jmp	1f
2:
#if 0
	/* detailed check for all possibilities */
	movw	$0x7F, %cx
	xorw	%di, %di

lba_check_loop:
	incw	%di		/* number of sectors to read */

	pushal
	pushw	%di
	pushw	$ABS(LBA_read_sectors_message)
	call	realmode_printf
	popw	%ax
	popw	%di
	popal
#else
	movw	$3, %cx		/* try it for 3 times. */
	/* only check 127-sector accessibility */
	movw	$127, %di	/* number of sectors to read */

lba_try_again:

	pushal
	xorb	%dh, %dh
	pushw	%dx		/* drive number */
	pushw	$ABS(LBA_read_sectors_message)
	call	realmode_printf
	popw	%ax
	popw	%dx
	popal
#endif

	pushw	%ds
	pushw	%es
	pushal
	xorl	%eax, %eax
	pushl	%eax		/* sector hi */
	pushl	%eax		/* sector lo */
	pushw	$0x2000		/* buffer segment = GEOMTUNE_SEG */
	pushw	%ax		/* offset=0 */
	pushw	%di		/* sectors to read */
	pushw	$0x0010		/* packet size */
	movw	%ax, %ds	/* DS=SS=0 */
	movw	%sp, %si
	movw	$0x4200, %ax
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw			/* discard packet on the stack */
	popal
	popw	%es
	popw	%ds
#if 0
	loop	lba_check_loop
	setc	%bh		/* failure, cannot read 127 sectors */
#else
	jnc	2f
	loop	lba_try_again
2:
	setc	%bh		/* failure, cannot read 127 sectors */
	pushal
	pushw	$ABS(LBA_read_sectors_message_wipe)
	call	realmode_printf
	popw	%ax
	popal
#endif
1:
	/* back to protected mode */
	call	EXT_C(real_to_prot)
	.code32

	sti

	xchgl	%eax, %ebx	/* return value in %eax */

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp

	ret

LBA_read_sectors_message:
#if 0
	.ascii "\r\nTry reading %d sectors ...   \0"
#else
	.ascii "\r\nIf hangup, use \'geometry --lba1sector (%d)\' at the beginning of preset menu. \0"
LBA_read_sectors_message_wipe:
	.ascii "\r                                                                               \r\0"
#endif

/*
 *   int get_diskinfo_standard (unsigned long drive, unsigned long *cylinders,
 *                              unsigned long *heads, unsigned long *sectors)
 *
 *   if bit 8-15 of drive(dh) != 0 on call, then geometry_tune will be used.
 *
 *   Return the geometry of DRIVE in CYLINDERS, HEADS and SECTORS. If an
 *   error occurs, then return non-zero, otherwise zero.
 */

ENTRY(get_diskinfo_standard)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	/* Heads */
	movl	0x10(%ebp), %edi
	movl	(%edi), %edi
	movl	%edi, ABS(Heads_passed_in)

	/* Sectors */
	movl	0x14(%ebp), %edi
	movl	(%edi), %edi
	movl	%edi, ABS(Sectors_passed_in)

	/* drive */
	movl	0x8(%ebp), %edx

	/* enter real mode */
	call	EXT_C(prot_to_real)

	.code16

	sti
	
	testb	%dh, %dh
	jz	2f

	call	geometry_tune

	movb	$1, %bh		/* geometry_tune indicator */
	jmp	1f
2:
	xorw	%cx, %cx
	movb	$0x8, %ah
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	movb	$0, %bh		/* geometry_tune indicator */
	jc	2f

	/* check if successful */
	testb	%ah, %ah
	jnz	1f		/* Error number in AH */

	/* bogus BIOSes may not return an error number */
	testb	$0x3f, %cl	/* 0 sectors means no disk */
	jnz	1f		/* if non-zero, then succeed */
2:
	/* failure */
	/* XXX 0x60 is one of the unused error numbers */
	movb	$0x60, %ah
1:
	movb	%ah, %bl	/* save return value in %bl */
	/* back to protected mode */
	call	EXT_C(real_to_prot)

	.code32

	sti

	testb	%bl, %bl	/* check failure */
	jnz	1f		/* failure */

	/* restore %ebp */
	leal	12(%esp), %ebp

	/* heads */
	xorl	%eax, %eax
	movb	%dh, %al
	incl	%eax		/* number of heads is counted from zero */
	movl	0x10(%ebp), %edi
	//movl	%eax, (%edi)
	stosl

	/* sectors */
	xorl	%eax, %eax
	movb	%cl, %al
	andb	$0x3f, %al
	movl	0x14(%ebp), %edi
	//movl	%eax, (%edi)
	stosl

	/* cylinders */

	testb	%bh, %bh
	jnz	1f		/* geometry_tune won't touch cylinders */

	xorl	%eax, %eax
	shrb	$6, %cl
	movb	%cl, %ah
	movb	%ch, %al
	incl	%eax		/* number of cylinders is counted from zero */
	movl	0x0C(%ebp), %edi
	//movl	%eax, (%edi)
	stosl
1:
	movzbl	%bl, %eax	/* return value in %eax */

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp

	ret

#if defined(USE_DEBUG_DUMP)
	.code16
debug_dump_regs_format:
	.ascii "di%4X si%4X bp%4X sp%4X bx%4X dx%4X cx%4X ax%4X f%4X ip%4X ln%u\r\n\0"
	/* debug_dump_regs_ln */
debug_dump_regs: 
	pushfw
	pushaw
	pushw	$ABS(debug_dump_regs_format)
	call	realmode_printf
	popw	%ax
	popaw
	popfw
	ret	$2
debug_pause:
	pushfw
	pushw	%ax
	movb	$0, %ah
	int	$0x16
	popw	%ax
	popfw
	ret
#define DEBUG_DUMP	pushw $__ ## LINE ## __ ; call debug_dump_regs
#define DEBUG_DUMP_P	pushw $__ ## LINE ## __ ; call debug_dump_regs ; call debug_pause
#else
#define DEBUG_DUMP
#define DEBUG_DUMP_P
#endif


#define GEOMTUNE_SEG 0x2000
	/* geometry_tune uses 64KiB buffer at this segment */
#define GEOMTUNE_BUFFERSEG BUFFERSEG
	/* geometry_tune uses 31.5KiB buffer at this segment */

	/* offset of variables in GEOM_TUNE_SEG */
#define GEOMTUNE_Smax_tuned    0x0000
#define GEOMTUNE_Hmax_tuned    0x0002
#define GEOMTUNE_Smax_readable 0x0004
	/* upperbound of Smax, cannot read past this sector number */
#define GEOMTUNE_Hmax_readable 0x0006
	/* upperbound of Hmax, cannot read past this head */
#define GEOMTUNE_read_ax       0x0008
	/* byte0 = number of sectors to read */
	/* byte1 = 2                         */
#define GEOMTUNE_cmps_cx       0x000A
	/* (tune Hmax) number of DWORD to compare      */
#define GEOMTUNE_next_read     0x000C
	/* (tune Hmax) boolean : 1 = next cylinder head 0 is read successfully */
	/* (tune Smax) boolean : 1 = next track sector 1 is read successfully  */
#define GEOMTUNE_equal 0x0100
	/* (tune Hmax) 
	 *   [0]     nonzero = There exist at least 1 track that have data equal to head 0. 
	 *   [1...254] 
	 *     bit0: 0=data from this head differ from data in head 0 same cylinder  1=no difference encountered
	 *     bit1: 0=data from this head differ from data in head 0 next cylinder  1=no difference encountered
	 *   [255]   padding
	 * (tune Smax)
	 *   [0]     nonzero = There exist at least 1 sector that have data equal to sector 1. 
	 *   [1...62] 
	 *     bit0: 0=data from sector i+1 differ from data in sector 1 same track  1=no difference encountered
	 *     bit1: 0=data from sector i+1 differ from data in sector 1 next track  1=no difference encountered
	 *   [63]    padding
	 */
#define GEOMTUNE_H_buf_cch0   0x0200
	/* 0200 - 7FFF buffer current cylinder head0 (upto 63 sectors) */
#define GEOMTUNE_H_buf_nch0   0x8000
	/* 8000 - FDFF buffer next    cylinder head0 (upto 63 sectors) */
#define GEOMTUNE_S_buf_ct     0x0200
	/* 0200 - 7FFF buffer current track */
#define GEOMTUNE_S_buf_nt     0x8000
	/* 8000 - FDFF buffer next track */

	/* offset of variables in BUFFERSEG */
#define GEOMTUNE_H_buf_ct     0x0000
	/* 0000 - 7DFF buffer current track          (upto 63 sectors) */

	.code16

geometry_tune:

	/* on call:
	 *		CS	0
	 *
	 * on return:
	 *		AH=0	success, otherwise failure
	 *		CL	max sector number
	 *		DH	max head number
	 *		DS	changed
	 *		ES	changed
	 *		EAX	changed
	 *		BX	changed
	 *		CX	changed
	 *		DX	changed
	 *		SI	changed
	 *		DI	changed
	 */
	
	/* check for fb status  */
	cmpb	$0, %cs: ABS(EXT_C(fb_status))
	jz	no_fb
	cmpb	%dl, %cs: ABS(EXT_C(fb_status)) + 1
	jnz	no_fb
	movb	%cs: ABS(EXT_C(fb_status)) + 2, %cl
	movb	%cs: ABS(EXT_C(fb_status)) + 3, %dh
	decb	%dh
	xorb	%ah, %ah
	ret
no_fb:
#ifdef USE_OLD_GEOMETRY_TUNE
//////////////////////////////////////////////////////////////////////////////

	/* find Max sector by reading each sector on the first track. */

#if 0
	pushal
	pushw	%ds
	pushw	%es
	movw	$0x0e41, %ax
	xorw	%bx, %bx
	int	$0x10
	popw	%es
	popw	%ds
	popal
#endif

	/* try the passed-in value first */
	movw	%cs:ABS(Sectors_passed_in), %cx	/* cylinder=0 */
	call	check_sector_readable
	jnc	1f
	xorw	%cx, %cx
1:
	cmpw	$63, %cx
	jnb	2f		/* this is the maximum value */

	incw	%cx
	call	check_sector_readable
	jc	1f
	cmpw	$63, %cx
	jb	1b
	jmp	2f
1:
	/* Max Sector = CX - 1 */
	decw	%cx
	cmpb	$2, %cl
	jnb	2f
	movb	$1, %ah		/* failure */
	ret

check_sector_readable:
	movw	$0x27E0, %ax	/* Don't use SCRATCHSEG */
	movw	%ax, %es
	movw	%ax, %ds
	xorw	%bx, %bx
	movw	$0x201, %ax	/* read 1 sector */
	movb	$0, %dh		/* head=0 */
	pushaw
	movw	%bx, %si
	movw	%bx, %di
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw
	ret

2:
	cmpw	$63, %cx
	jbe	2f
	movw	$63, %cx
2:
	/* CX=Max Sector */
	movw	%cx, %cs:ABS(Smax_tuned)

#if 0
	pushal
	pushw	%ds
	pushw	%es
	movw	$0x0e20, %ax
	addb	%cl, %al
	xorw	%bx, %bx
	int	$0x10
	popw	%es
	popw	%ds
	popal
#endif

//////////////////////////////////////////////////////////////////////////////
#if 0
	/* check if we can read sectors across the track boundary */

	/* first, read a track plus one sector */

	movb	$0, %cs:ABS(cross_track)

	movw	$1, %cx		/* sector 1, cylinder 0 */
	movb	$0, %dh		/* head 0 */
	movb	%cs:ABS(Smax_tuned), %al	/* sectors to read */
	incw	%ax		/* read 1 more sector */
	movb	$2, %ah		/* READ */
	movw	$0x2000, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw
	jc	1f		/* cross-track read is not supported */

	/* read again normally, only the track */

	movb	%cs:ABS(Smax_tuned), %al	/* sectors to read */
	movb	$2, %ah		/* READ */
	movw	$0x2800, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw
	jc	2b		/* failure */

	/* compare the two tracks */

	pushw	%cx
	pushw	%si
	pushw	%di
	movw	%cs:ABS(Smax_tuned), %cx	/* sectors */
	shlw	$7, %cx				/* dwords */
	movw	$0x2000, %ax
	movw	%ax, %ds
	movw	$0x2800, %ax
	movw	%ax, %es
	xorw	%si, %si
	xorw	%di, %di
	cld
	repz cmpsl
	popw	%di
	popw	%si
	popw	%cx
	jne	1f		/* cross-track read is not supported */
	movb	$1, %cs:ABS(cross_track)
1:
#endif
//////////////////////////////////////////////////////////////////////////////
#if 0
	/* find Max head by reading sector 1 on each track of cylinder 0. */

	movb	$0xFF, %dh	/* head=Max possible */
1:
	movw	$0x27E0, %ax	/* Don't use SCRATCHSEG */
	movw	%ax, %es
	movw	%ax, %ds
	xorw	%bx, %bx
	movw	$0x201, %ax	/* read 1 sector */
	movw	$1, %cx		/* cylinder=0, sector=1 */
	pushaw
	movw	%bx, %si
	movw	%bx, %di
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw
	jnc	1f		/* found Max head */
	decb	%dh
	cmpb	$0xFF, %dh
	jne	1b
	movb	$1, %ah		/* failure */
	ret
1:

	/* DH=Max head */
	movb	%dh, %cs:ABS(Hmax_tuned)
#endif
//////////////////////////////////////////////////////////////////////////////

	/* tune Hmax */

	/* First, try the passed-in value */
	movb	%cs:ABS(Heads_passed_in), %dh
	testb	%dh, %dh
	jz	1f		/* the passed-in heads = 0x100 */

	call	tune_heads
	jb	2f		/* failure */
	ja	4f		/* success */
1:
	movb	$1, %dh		/* Hmax: 1 - 255 */
1:
	call	tune_heads
	jb	2f		/* failure */
	ja	4f		/* success */

#if 0
	cmpb	%cs:ABS(Hmax_tuned), %dh
	ja	2f		/* this should not happen */
	je	5f
#endif

	incb	%dh		/* Next Hmax */
	jnz	1b

	/* Hmax=0xFF */
4:
	/* got Hmax=DH-1 */

	decb	%dh
	movb	%dh, %cs:ABS(Hmax_tuned)
5:
	/* Hmax is tuned ok. */

	cmpb	$0xFF, %dh
	jne	1f
	/* consider Hmax=0xFF as a failure! Use the passed-in value. */
	movb	%cs:ABS(Heads_passed_in), %dh
	testb	%dh, %dh
	jnz	4f
	decb	%dh
4:
	decb	%dh
	movb	%dh, %cs:ABS(Hmax_tuned)
1:

#if 0
	pushal
	pushw	%ds
	pushw	%es
	movw	$0x0e21, %ax
	xorw	%bx, %bx
	int	$0x10
	popw	%es
	popw	%ds
	popal
#endif

//////////////////////////////////////////////////////////////////////////////

	/* tune Smax */

	/* First, try the passed-in value */
	movb	%cs:ABS(Sectors_passed_in), %cl
//	cmpb	$0, %cs:ABS(cross_track)
//	jnz	4f
//	cmpb	%cs:ABS(Smax_tuned), %cl
//	jnb	1f
//	cmpb	$8, %cl
//	jb	1f
4:
	call	tune_sectors
	jb	2f		/* failure */
	ja	4f		/* success */
1:
	movb	$8, %cl		/* Smax: 8 - 63 */
1:
	call	tune_sectors
	jb	2f		/* failure */
	ja	4f		/* success */

	incw	%cx		/* Next Smax */
	cmpb	%cs:ABS(Smax_tuned), %cl
	jb	1b
	cmpb	$63, %cl
	jbe	4f
	movb	$63, %cl

4:
	/* got Smax=CL */

	movb	%cl, %cs:ABS(Smax_tuned)

	/* Smax is tuned ok. */

#if 0
	pushal
	pushw	%ds
	pushw	%es
	movw	$0x0e21, %ax
	xorw	%bx, %bx
	int	$0x10
	popw	%es
	popw	%ds
	popal
#endif

//////////////////////////////////////////////////////////////////////////////

	movw	%cs:ABS(Smax_tuned), %cx
	movb	%cs:ABS(Hmax_tuned), %dh
	movb	$0, %ah		/* success */
	ret
2:
	movb	$1, %ah		/* failure */
	ret

//////////////////////////////////////////////////////////////////////////////

tune_heads:

	/* input:	DH = MaxHead + 1 */

	movb	$0, %ch		/* cylinder: 0 - 4 */
2:
	/* read ending track of this cylinder */

#if 0
	pushal
	pushw	%ds
	pushw	%es
	movw	$0x0e22, %ax
	xorw	%bx, %bx
	int	$0x10
	popw	%es
	popw	%ds
	popal
#endif

	movb	$1, %cl		/* sector 1=the leading sector */
	movb	$2, %ah		/* READ */
	movb	%cs:ABS(Smax_tuned), %al	/* sectors to read */
	movw	$0x2000, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw
	//jc	2f		/* failure */
	incb	%ch
	jc	4f		/* considered OK */
	decb	%ch

	/* read beginning track of this cylinder */

	movb	$1, %cl		/* sector 1=the leading sector */
	movb	$2, %ah		/* READ */
	movb	%cs:ABS(Smax_tuned), %al	/* sectors to read */
	movw	$0x2800, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
	movb	$0, %dh
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw
	jc	2f		/* failure */

	incb	%ch		/* next cylinder */

	/* compare the two tracks */
	call	cmp_track
	je	4f		/* ok, try next cylinder */

	/* read beginning track of the next cylinder */

	movb	$1, %cl		/* sector 1=the leading sector */
	movb	$2, %ah		/* READ */
	movb	%cs:ABS(Smax_tuned), %al	/* sectors to read */
	movw	$0x2800, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
	movb	$0, %dh
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw
	jc	2f		/* failure */

	/* compare the two tracks */
	call	cmp_track
	jne	3f		/* Next Hmax */
4:
	cmpb	$5, %ch		/* cylinder: 0 - 4 */
	jb	2b		/* Next cylinder */

	/* all passed, DH-1 is the final Hmax */
	cmpb	$0, %dh
	je	2f		/* failure */
	ret			/* Flag: above */
3:
	cmpb	%dh, %dh	/* Flag: equal */
	ret
2:
	stc			/* Flag: below */
	ret

cmp_track:
	pushw	%cx
	pushw	%si
	pushw	%di
	movw	%cs:ABS(Smax_tuned), %cx	/* sectors */
	shlw	$7, %cx				/* dwords */
	movw	$0x2000, %ax
	movw	%ax, %ds
	movw	$0x2800, %ax
	movw	%ax, %es
	xorw	%si, %si
	xorw	%di, %di
	cld
	repz cmpsl
	popw	%di
	popw	%si
	popw	%cx
	ret

//////////////////////////////////////////////////////////////////////////////

tune_sectors:

	/* input:	CL = MaxSector */

	movw	$16, %cs:ABS(Smax_count)

	movb	$0, %ch		/* cylinder: 0 - 6 */
	movb	$0, %dh		/* head: 0 - Hmax */
6:
	/* read beginning sector of this track. */

	movw	$0x201, %ax	/* read 1 sector */
	movw	$0x2800, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
	movb	$1, %cl		/* beginning sector */
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw
	jc	2f		/* failure */

	/* read ending sector of this track. */

	movw	$0x202, %ax	/* read 2 sectors */
	movw	$0x2000, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw
	//jc	2f		/* failure */

	/* locate the next track. */

	pushfw			/* save CF */
	cmpb	%cs:ABS(Hmax_tuned), %dh
	jb	3f		/* Next track */
	movb	$0xFF, %dh	/* head 0 of ... */
	incb	%ch		/* ... the next cylinder. */
3:
	incb	%dh		/* next track */
	popfw			/* restore CF */
	jc	4f		/* considered OK */

	/* compare the 2 sectors */
	call	cmp_sector
	je	4f		/* ok, try next track */

	/* read beginning sector of the next track. */

	movw	$0x201, %ax	/* read 1 sector */
	movw	$0x2800, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
	movb	$1, %cl		/* sector 1=the leading sector */
#if 1
	int	$0x13
#else
	call	safe_int13
#endif
	popaw
	jc	2f		/* failure */

	/* compare the 2 sectors */
	call	cmp_sector
	jne	3f		/* Next Smax */
4:
	decw	%cs:ABS(Smax_count)
	jz	6f

	//cmpb	%cs:ABS(Hmax_tuned), %dh
	//jb	6b		/* Next track */
	//movb	$0, %dh		/* head 0 of ... */
	//incb	%ch		/* ... the next cylinder. */
	cmpb	$7, %ch		/* any cylinder remains to check? */
	jb	6b		/* yes, next track */
6:
	/* all passed, CL is the final Smax */
	cmpb	$1, %cl
	jbe	2f		/* failure */
	ret			/* Flag: above */
3:
	/* not Maximum sector number */
	cmpb	%cl, %cl	/* Flag: equal */
	ret
2:
	/* I/O error, sector tune failed */
	stc			/* Flag: below */
	ret

cmp_sector:
	pushw	%cx
	pushw	%si
	pushw	%di
	movw	$0x80, %cx	/* 1 sector == 0x80 dwords */
	movw	$0x2020, %ax
	movw	%ax, %ds
	movw	$0x2800, %ax
	movw	%ax, %es
	xorw	%si, %si
	xorw	%di, %di
	cld
	repz cmpsl
	popw	%di
	popw	%si
	popw	%cx
	ret

#else /* ndef USE_OLD_GEOMETRY_TUNE */

	//////////////////////////////////////////////////////////////////////////////
	/* find Max sector by reading each sector on the first track. */

	movw	$GEOMTUNE_SEG, %ax	/* Don't use SCRATCHSEG */
	movw	%ax, %es
	movw	%ax, %ds

	/* try the passed-in value first */
	movw	%cs:ABS(Sectors_passed_in), %cx	/* cylinder=0 */
	xorw	%ax, %ax
	cmpw	$63, %cx
	ja	1f
	call	check_sector_readable
	sbbw	%ax, %ax
	notw	%ax
1:
	andw	%ax, %cx	/* if read failure, cx=0 */
1:
	cmpb	$63, %cl
	jnb	2f		/* this is the maximum value */
	incw	%cx
	call	check_sector_readable
	jnc	1b

	/* Max Sector = CX - 1 */
	decw	%cx
	cmpb	$2, %cl
	jnb	2f		/* CX=Max Sector */
	movb	$1, %ah		/* failure */
	ret

check_sector_readable:
	xorw	%bx, %bx
	movw	$0x201, %ax	/* read 1 sector */
	movb	$0, %dh		/* head=0 */
	pushaw
	//movw	%bx, %si
	//movw	%bx, %di
	int	$0x13
	jc	1f
	decb	%al
	addb	$255, %al	/* set CF if returned AL != 1 */
1:
	popaw
	ret
2:
	/* CX=Max Sector */
	movw	%cx, GEOMTUNE_Smax_readable

	//////////////////////////////////////////////////////////////////////////////
	/* tune Hmax */

	/* invalidate disk cache */
	movl	$(-1),%cs:ABS(EXT_C(buf_track))

	movw	$GEOMTUNE_SEG, %bx
	movw	%bx, %ds
	movw	%bx, %es
	movw	$254, GEOMTUNE_Hmax_readable

	movb	$2,  %ch
	movw	%cx, GEOMTUNE_read_ax	/* number of readable sectors per track */
	shlw	$7,  %cx		/* x128 DWORDs per sector               */
	movw	%cx, GEOMTUNE_cmps_cx	/* number of DWORDs to compare          */
	/* fill array */
	cld
	movw	$GEOMTUNE_equal, %di
	movl	$0x03030303, %eax
	movw	$0x40, %cx	/* 0x100/4 = 0x40 */
	rep stosl
	
	/* cx is now 0 */	/* start with cylinder 0*/

10:	/* cylinder loop */
	movb	$1, %cl		/* sector 1 */
	xorb	%dh, %dh	/* head 0 */
	movw	GEOMTUNE_read_ax, %ax
	movw	%ds, %bx
	movw	%bx, %es	/* GEOMTUNE_SEG */

	/* read next cylinder, head 0 */
	pushaw
	incb	%ch	/* next cylinder */
	movw	$GEOMTUNE_H_buf_nch0, %bx
	int	$0x13
	setnc	%ah
	cmpb	GEOMTUNE_read_ax, %al
	sete	%al
	andb	%ah, %al	/* 1=no error and read all required sectors */
	movb	%al, GEOMTUNE_next_read	/* save boolean value for later use */
	popaw

	/* read current cylinder, head 0 */
	pushaw
	movw	$GEOMTUNE_H_buf_cch0, %bx
	int	$0x13
	setnc	%ah
	cmpb	GEOMTUNE_read_ax, %al
	sete	%al
	andb	%ah, %al	/* 1=no error and read all required sectors */
	popaw
	jz	14f	/* error, end cylinder loop */

	/* read current cylinder, head 1-254 */
	movw	$GEOMTUNE_BUFFERSEG, %bx
	movw	%bx, %es
	/* DH is now 0 */
	movb	%dh, GEOMTUNE_equal	/* GEOMTUNE_equal[0] = 0 */

11:	/* head loop */
	addb	$1,  %dh
	cmp	GEOMTUNE_Hmax_readable, %dh
	ja	13f	/* end head loop */

	movb	$(GEOMTUNE_equal >> 8),  %bh
	movb	%dh, %bl	
	/* DS:BX = address of GEOMTUNE_equal[DH] */
	movb	(%bx), %al
	testb	%al,  %al	/* 0=this head has already returned different data from head 0, no need to compare again */
	jz	11b		/* head loop */

	pushaw
	xorw	%bx, %bx	/* offset GEOMTUNE_H_buf_ct is 0 */
	movw	GEOMTUNE_read_ax, %ax
	int	$0x13
	setnc	%ah
	cmpb	GEOMTUNE_read_ax, %al
	sete	%al
	andb	%ah, %al	/* no error and read all required sectors */
	popaw
	jz	12f	/* read error head 1-254 */

	pushaw
	testb	$1,  %al
	jz	1f
	/* compare with same cylinder head 0 */
	movw	GEOMTUNE_cmps_cx, %cx
	movw	$GEOMTUNE_H_buf_cch0, %si
	xorw	%di, %di	/* offset GEOMTUNE_H_buf_ct is 0 */ /* ES is BUF_SEG */
	repe cmpsl
	je	1f
	/* different data */
	andb	$(0xFF-1), %al
1:
	testb	$2,  %al
	jz	1f
	/* compare with next cylinder head 0 */
	testb	$1, GEOMTUNE_next_read	/* skip if next cylinder head 0 has not been read successfully */
	jz	1f		
	movw	GEOMTUNE_cmps_cx, %cx
	movw	$GEOMTUNE_H_buf_nch0, %si
	xorw	%di, %di	/* offset GEOMTUNE_H_buf_ct is 0 */ /* ES is BUF_SEG */
	repe cmpsl
	je	1f
	/* different data */
	andb	$(0xFF-2), %al
1:
	movb	%al, (%bx)
	orb	%al, GEOMTUNE_equal	/* GEOMTUNE_equal[0] */
	popaw

	/* continue, next head */
	jmp	11b	/* head loop */

12:	/* read error head 1-254 */
	test	%ch, %ch /* Is this cylinder 0 ? */
	    		 /* If error on cylinder >0, end. */
	jnz	14f	/* end cylinder loop */
	decb	%dh		/* Error on cylinder 0 = end of cylinder. Adjust readable head number */
	movb	%dh, GEOMTUNE_Hmax_readable

13:	/* end head loop */
	movb	GEOMTUNE_equal, %al
	test	%al, %al	/* 0: all readable heads(>0) differ from head0 */
	jz	14f	/* end cylinder loop */

	/* next cylinder */
	incb	%ch
	cmpb	$5,  %ch	/* max number of cylinders to read */
	jb	10b	/* cylinder loop */

14:	/* end cylinder loop */
	movw	%ds, %bx
	movw	%bx, %es	/* GEOMTUNE_SEG */
	movw	%cx, %bx
	movw	$(GEOMTUNE_equal +1), %di
	movw	GEOMTUNE_Hmax_readable, %cx
	incw	%cx
	xorw	%ax, %ax
	repz	scasb		/* skip heads that have different data from head0 */
	DEBUG_DUMP
	subw	$(GEOMTUNE_equal +2), %di
	movw	%di, GEOMTUNE_Hmax_tuned

	//////////////////////////////////////////////////////////////////////////////
	/* tune Smax */

	/* DS = ES = GEOMTUNE_SEG */

	movw	$GEOMTUNE_equal, %di
	movl	$0x03030303, %eax
	movw	$(64/4), %cx
	rep stosl

	/* read first track */
	movw	$1, %cx 	/* cyl 0 sector 1 */
	xorb	%dh, %dh	/* head 0 */
	movw	GEOMTUNE_read_ax, %ax
	movw	$GEOMTUNE_S_buf_ct, %bx
	pushaw
	int	$0x13
	popaw
	jc	2f
	/* save data in disk cache */
	movw	%bx, %si
	xorw	%di, %di
	movw	$GEOMTUNE_BUFFERSEG, %bx
	movw	%bx, %es
	movw	$(63*512/4), %cx
	rep movsl
	movw	%ds, %bx
	movw	%bx, %es
#if 0
	/* update buf_track variable */
	movl	$0, %cs:ABS(EXT_C(buf_track))
	movl	$0, %cs:ABS(EXT_C(buf_track)) + 4
#else
	/* Invalidate the buffer, because the buffer is not exactly 1 track. */
	/* The buffer size is now 64K. */
	movl	$-1, %cs:ABS(EXT_C(buf_track))
	/* need not set the higher 32 bits of buf_track. */
#endif
	movw	$1, %cx 	/* cyl 0 sector 1 */

20:	/* track loop */
	/* compare sector 1 with sectors 2-63 */
	/* AL = number of sectors read */

	pushw	%cx
	pushw	%dx
	xorb	%ah, %ah
	movw	$GEOMTUNE_equal, %bx
	movw	$GEOMTUNE_S_buf_ct, %dx
21:	/* sector loop */
	incw	%bx
	cmpb	%al, %bl
	jae	22f /* end sector loop */
	addw	$512, %dx
	movw	%dx, %di
	movw	$GEOMTUNE_S_buf_ct, %si
	movw	$(512/4), %cx
	repe cmpsl
	setne	%cl		/* eq:0   dif:1     */
	notb	%cl		/* eq:255 dif:255-1 */
	andb	%cl, (%bx)	/* clear bit0 if dif */
	setnz	%cl		/* dif:0  eq:1      */
	orb	%cl, %ah	/* AH=1 if there is some sector that has equal data to sector 1 */
	jmp	21b /* sector loop */
22:	/* end sector loop */
	popw	%dx
	popw	%cx

	testb	$0xFF, %ah	/* AH=1 if there is sector that has equal data to sector 1 */
	jz	25f /* end track loop */ /* no more equal sector */
	/* next track */
	incb	%dh
	cmpb	GEOMTUNE_Hmax_tuned, %dh
	jna	1f
	/* next cylinder */
	xorb	%dh, %dh
	incb	%ch
	cmpb	$7, %ch		/* max cylinder number to read and compare */
	ja	25f /* end track loop */
1:	/* read new track */
	movw	GEOMTUNE_read_ax, %ax
	movw	$GEOMTUNE_S_buf_nt, %bx
	pushaw
	int	$0x13
	popaw
	jc	2f
	/* compare sector 1 with sectors 2-63 from previous track */
	/* AL = number of sectors read */
	pushaw
	movw	$GEOMTUNE_equal, %bx
	movw	$GEOMTUNE_S_buf_ct, %dx
23:	/* sector loop */
	incw	%bx
	cmpb	%al, %bl
	jae	24f /* end sector loop */
	addw	$512, %dx
	movw	%dx, %di
	movw	$GEOMTUNE_S_buf_nt, %si
	movw	$(512/4), %cx
	repe cmpsl
	setne	%cl		/* eq=>0   dif=>1     */
	addb	%cl, %cl	/* eq=>0   dif=>2     */
	notb	%cl		/* eq=>255 dif=>255-2 */
	andb	%cl, (%bx)	/* clear bit1 if dif */
	jmp	23b /* sector loop */
24:	/* end sector loop */
	/* copy next track buffer to current track buffer */
	movw	$GEOMTUNE_S_buf_nt, %si
	movw	$GEOMTUNE_S_buf_ct, %di
	movw	$(63*512/4), %cx
	rep movsl
	popaw
	jmp	20b /* track loop */
25:	/* end track loop */
	movw	$(GEOMTUNE_equal +1), %di
	movw	GEOMTUNE_Smax_readable, %cx
	xorw	%ax, %ax	/* AL = 0 */
	repe	scasb		/* skip 0 */
	DEBUG_DUMP
	subw	$(GEOMTUNE_equal +1), %di
	movw	%di, GEOMTUNE_Smax_tuned

	movw	%di, %cx
	movb	GEOMTUNE_Hmax_tuned, %dh
	movb	$0, %ah		/* success */
	ret
2:
	movb	$1, %ah		/* failure */
	ret
#endif /* ndef USE_OLD_GEOMETRY_TUNE */

//////////////////////////////////////////////////////////////////////////////

	.align	4

Sectors_passed_in:
	.long	0
Heads_passed_in:
	.long	0
#ifdef USE_OLD_GEOMETRY_TUNE
Smax_tuned:
	.word	0
Hmax_tuned:
	.word	0
Smax_count:
	.word	0
cross_track:
	.byte	0
#endif

//////////////////////////////////////////////////////////////////////////////


/*
 *
 * get_memsize(i) :  return the memory size in KB. i == 0 for conventional
 *		memory, i == 1 for extended memory
 *	BIOS call "INT 12H" to get conventional memory size
 *	BIOS call "INT 15H, AH=88H" to get extended memory size
 *		Both have the return value in AX.
 *
 */

ENTRY(get_memsize)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	0x14(%esp), %ebx

	call	EXT_C(prot_to_real)	/* enter real mode */

	.code16

	sti		/* for hardware interrupt or watchdog */

	cmpb	$0x1, %bl
	je	1f

	int	$0x12
	jmp	2f

1:
	movb	$0x88, %ah
	pushfw
	lcall	*ABS(EXT_C(ROM_int15))
2:
	movw	%ax, %bx

	call	EXT_C(real_to_prot)
	.code32

	sti

	movw	%bx, %ax
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 *
 * get_eisamemsize() :  return packed EISA memory map, lower 16 bits is
 *		memory between 1M and 16M in 1K parts, upper 16 bits is
 *		memory above 16M in 64K parts.  If error, return -1.
 *	BIOS call "INT 15H, AH=E801H" to get EISA memory map,
 *		AX = memory between 1M and 16M in 1K parts.
 *		BX = memory above 16M in 64K parts.
 *
 */

ENTRY(get_eisamemsize)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	call	EXT_C(prot_to_real)	/* enter real mode */

	.code16

	//sti		/* it is not bad keeping interrupt off */
	sti		/* for hardware interrupt or watchdog */

	movl	$0xe801, %eax
	//int	$0x15
2:
	pushl	%eax
	pushfw
	lcall	*ABS(EXT_C(ROM_int15))
	jc	1f
	testb	$0x80, %ah
	jnz	1f

	shll	$16, %ebx
	movw	%ax, %bx
	testl	%ebx, %ebx
	jnz	3f
	movw	%cx, %ax
	movw	%dx, %bx
	shll	$16, %ebx
	movw	%ax, %bx
3:
	popl	%eax
	jmp	2f
1:
	popl	%eax
	cmpw	$0xE881, %ax
	movw	$0xE881, %ax
	jne	2b
	//movl	$0xFFFFFFFF, %ebx
	/* call the old function 88h */
	movw	$0x8800, %ax
	pushfw
	lcall	*ABS(EXT_C(ROM_int15))
	movzwl	%ax, %ebx
2:
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es

	call	EXT_C(real_to_prot)
	.code32

	sti

	movl	%ebx, %eax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

/*
 *
 * get_mmap_entry(addr, cont) :  address and old continuation value (zero to
 *		start), for the Query System Address Map BIOS call.
 *
 *  Sets the first 4-byte int value of "addr" to the size returned by
 *  the call.  If the call fails, sets it to zero.
 *
 *	Returns:  new (non-zero) continuation value, 0 if done.
 *
 * NOTE: Currently hard-coded for a maximum buffer length of 1024.
 * Note: addr must be addressible by BIOS in the low memory. -Tinybit
 */

ENTRY(get_mmap_entry)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	/* place address (+4) in ES:DI */
	movl	0x14(%esp), %eax	/* addr */
	addl	$4, %eax
	movl	%eax, %edi
	andl	$0xf, %edi		/* offset */
	shrl	$4, %eax		/* segment */
	movl	%eax, %esi

	/* set continuation value */
	movl	0x18(%esp), %ebx	/* cont */

	pushl	%ecx		/* save ECX */

	/* set default maximum buffer size */
	movl	$0x14, %ecx

	/* set EDX to 'SMAP' */
	movl	$0x534d4150, %edx

	call	EXT_C(prot_to_real)	/* enter real mode */

	.code16

	//sti		/* it is not bad keeping interrupt off */
	sti		/* for hardware interrupt or watchdog */

	movw	%si, %es		/* ES=segment */

	/* set additional registers to serve buggy BIOSes. */
	movw	%si, %ds
	movw	%di, %si

	movl	$0xe820, %eax
	//int	$0x15
	pushfw
	lcall	%cs:*ABS(EXT_C(ROM_int15))

	jnc	1f
	movl	$0, %ebx	/* set end indicator */
1:
	cmpl	$0x534d4150, %eax
	jne	1f

	/* 20-byte length is currently standard. So others are considered
	 * invalid.
	 */

	cmpl	$0x14, %ecx
	je	2f

1:
	movl	$0, %ecx
2:

	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es

	call	EXT_C(real_to_prot)
	.code32

	sti

	popl	%eax		/* OLD ECX */

	/* write length of buffer (zero if error) into "addr" */
	movl	0x14(%esp), %edi
	xchgl	%eax, %ecx
	stosl

	/* set return value to continuation */
	movl	%ebx, %eax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

/*
 * get_rom_config_table()
 *
 * Get the linear address of a ROM configuration table. Return zero,
 * if fails.
 */

ENTRY(get_rom_config_table)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	/* zero %ebx for simplicity */
	xorl	%ebx, %ebx

	call	EXT_C(prot_to_real)

	.code16

	sti		/* for hardware interrupt or watchdog */

	movb	$0xc0, %ah
	int	$0x15

	jc	no_rom_table
	testb	%ah, %ah
	jnz	no_rom_table

	movw	%es, %dx
	jmp	found_rom_table

no_rom_table:
	xorw	%dx, %dx
	xorw	%bx, %bx

found_rom_table:

	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es

	call	EXT_C(real_to_prot)
	.code32

	sti

	/* compute the linear address */
	xorl	%eax, %eax
	movw	%dx, %ax
	shll	$4, %eax
	addl	%ebx, %eax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * int get_vbe_controller_info (struct vbe_controller *controller_ptr)
 *
 * Get VBE controller information.
 */

ENTRY(get_vbe_controller_info)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	/* Convert the linear address to segment:offset */
	movl	8(%ebp), %eax
	movl	%eax, %edi
	andl	$0x0000000f, %edi
	shrl	$4, %eax
	movl	%eax, %ebx

	call	EXT_C(prot_to_real)

	.code16

	sti		/* for hardware interrupt or watchdog */

	movw	%bx, %es

	/* set additional registers to serve buggy BIOSes. */
	movw	%bx, %ds
	movw	%di, %si

	movw	$0x4F00, %ax
	int	$0x10

	movw	%ax, %bx

	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es

	call	EXT_C(real_to_prot)
	.code32

	sti

	movzwl	%bx, %eax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * int get_vbe_mode_info (int mode_number, struct vbe_mode *mode_ptr)
 *
 * Get VBE mode information.
 */

ENTRY(get_vbe_mode_info)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	/* Convert the linear address to segment:offset */
	movl	0xc(%ebp), %eax
	movl	%eax, %edi
	andl	$0x0000000f, %edi
	shrl	$4, %eax
	movl	%eax, %ebx

	/* Save the mode number in %cx */
	movl	0x8(%ebp), %ecx

	call	EXT_C(prot_to_real)

	.code16

	sti		/* for hardware interrupt or watchdog */

	movw	%bx, %es
	movw	$0x4F01, %ax
	int	$0x10

	movw	%ax, %bx
	call	EXT_C(real_to_prot)
	.code32

	sti

	movzwl	%bx, %eax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * int set_vbe_mode (int mode_number)
 *
 * Set VBE mode. Don't support user-specified CRTC information.
 */

ENTRY(set_vbe_mode)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	/* Save the mode number in %bx */
	movl	0x8(%ebp), %ebx
	/* Clear bit D11 */
	andl	$0xF7FF, %ebx

	call	EXT_C(prot_to_real)

	.code16

	//sti		/* it is not bad keeping interrupt off */
	sti		/* for hardware interrupt or watchdog */

	movw	$0x4F02, %ax
	int	$0x10

	movw	%ax, %bx
	call	EXT_C(real_to_prot)
	.code32

	sti

	movzwl	%bx, %eax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


#ifdef SUPPORT_GFX

/*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * graphical menu functions
 *
 */

/*
 * int gfx_init_v1 (gfx_data_v1_t *gfx_data)
 *
 * init gfx things
 *
 * return vales:
 *   0: ok
 *   1: failed
 *   sets gfx_data->ok
 */

ENTRY(gfx_init_v1)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	8(%ebp),%edx
	movl	%edx,%edi
	andl	$0xf,%edi
	shrl	$4,%edx

	pushl	%ebp

	call	EXT_C(prot_to_real)
	.code16

	pushw	%ds

	movw	%dx,%ds
	leal	gfx_ofs_v1_sys_cfg(%di),%esi
	movl	gfx_ofs_v1_mem_file(%di),%eax
	movl	gfx_ofs_v1_mem_cur(%di),%ebx
	movl	gfx_ofs_v1_mem_max(%di),%ecx
	movw	%ds,%dx

	/* basically just a lcall, but we need %edi */
	pushw	%cs
	pushw	$gfx_init_50_v1
	pushl	gfx_ofs_v1_jmp_table + 4 * 0 (%di)

	movl	gfx_ofs_v1_mem_align(%di),%edi

	lret

gfx_init_50_v1:
	movl	$0,%ebx
	adcl	$0,%ebx

	popw	%ds

	call	EXT_C(real_to_prot)
	.code32

	sti

	popl	%ebp

	movl	%ebx,%eax
	negl	%ebx
	incl	%ebx
	movl	8(%ebp),%edx
	movl	%ebx,gfx_ofs_v1_ok(%edx)

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * int gfx_done_v1 (gfx_data_v1_t *gfx_data)
 *
 * shut down gfx things
 *
 * return vales:
 *   always 0
 *   sets gfx_data->ok
 */

ENTRY(gfx_done_v1)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	8(%ebp),%edx
	movl	%edx,%ebx
	andl	$0xf,%ebx
	shrl	$4,%edx

	pushl	%ebp

	call	EXT_C(prot_to_real)
	.code16

	pushw	%ds

	movw	%dx,%ds

	lcall	*gfx_ofs_v1_jmp_table + 4 * 1 (%bx)

	popw	%ds

	call	EXT_C(real_to_prot)
	.code32

	sti

	popl	%ebp

	xorl	%eax,%eax
	movl	8(%ebp),%edx
	movl	%eax,gfx_ofs_v1_ok(%edx)

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * int gfx_input_v1 (gfx_data_v1_t *gfx_data, int *menu_entry)
 *
 * let user enter a command line
 *
 * uses gfx_data->cmdline as buffer
 *
 * return values:
 *   1: abort
 *   2: boot
 *   menu_entry: selected entry
 */

ENTRY(gfx_input_v1)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%esi
	pushl	%ebx
	pushl	%edi

	movl	8(%ebp),%edx
	movl	%edx,%ebx
	andl	$0xf,%ebx
	shrl	$4,%edx

	pushl	%ebp

	call	EXT_C(prot_to_real)
	.code16

	pushw	%ds

	movw	%dx,%ds
	shll	$4,%edx
	movl	gfx_ofs_v1_cmdline(%bx),%edi
	subl	%edx,%edi
	movw	gfx_ofs_v1_cmdline_len(%bx),%cx
	movw	gfx_ofs_v1_timeout(%bx),%ax
	imulw	$18,%ax

	pushl	%ebp
	lcall	*gfx_ofs_v1_jmp_table + 4 * 2 (%bx)
	popl	%ebp
	movl	%eax,%ecx

	popw	%ds

	call	EXT_C(real_to_prot)
	.code32

	sti

	popl	%ebp

	movl	12(%ebp),%edx
	movl	%ebx,(%edx)

	movl	%ecx,%eax

	popl	%edi
	popl	%ebx
	popl	%esi
	popl	%ebp
	ret


/*
 * int gfx_setup_menu_v1 (gfx_data_v1_t *gfx_data)
 *
 * draw boot menu
 *
 * return values:
 *   always 0
 */

/* menu entry descriptor */
#define menu_v1_entries		0
#define menu_v1_default		2	/* seg:ofs */
#define menu_v1_ent_list	6	/* seg:ofs */
#define menu_v1_ent_size	10
#define menu_v1_arg_list	12	/* seg:ofs */
#define menu_v1_arg_size	16
#define sizeof_menu_v1_desc	18

ENTRY(gfx_setup_menu_v1)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	8(%ebp),%edx
	movl	%edx,%ebx
	andl	$0xf,%ebx
	shrl	$4,%edx

	call	EXT_C(prot_to_real)
	.code16

	pushw	%ds

	movw	%dx,%ds
	shll	$4,%edx

	subw	$sizeof_menu_v1_desc,%sp
	movw	%sp,%bp

	movl	gfx_ofs_v1_menu_entries(%bx),%eax
	movw	%ax,menu_v1_entries(%bp)

	movl	gfx_ofs_v1_menu_default_entry(%bx),%eax
	subl	%edx,%eax
	movw	%ax,menu_v1_default(%bp)
	movw	%ds,menu_v1_default+2(%bp)

	movl	gfx_ofs_v1_menu_list(%bx),%eax
	subl	%edx,%eax
	movw	%ax,menu_v1_ent_list(%bp)
	movw	%ds,menu_v1_ent_list+2(%bp)

	movl	gfx_ofs_v1_menu_entry_len(%bx),%eax
	movw	%ax,menu_v1_ent_size(%bp)

	movl	gfx_ofs_v1_args_list(%bx),%eax
	subl	%edx,%eax
	movw	%ax,menu_v1_arg_list(%bp)
	movw	%ds,menu_v1_arg_list+2(%bp)

	movl	gfx_ofs_v1_args_entry_len(%bx),%eax
	movw	%ax,menu_v1_arg_size(%bp)

	movw	%bp,%si
	pushw	%ss
	popw	%es

	lcall	%ds: *gfx_ofs_v1_jmp_table + 4 * 3 (%bx)

	addw	$sizeof_menu_v1_desc,%sp

	popw	%ds

	call	EXT_C(real_to_prot)
	.code32

	sti

	xorl	%eax,%eax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * int gfx_init_v2 (gfx_data_v2_t *gfx_data)
 *
 * init gfx things
 *
 * return vales:
 *   0: ok
 *   1: failed
 *   sets gfx_data->ok
 */

ENTRY(gfx_init_v2)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	8(%ebp),%edx
	movl	%edx,%edi
	leal	gfx_ofs_v2_sys_cfg(%edx),%esi
	andl	$0xf,%edi
	shrl	$4,%edx

	pushl	%ebp

	call	EXT_C(prot_to_real)
	.code16

	pushw	%ds
	movw	%dx,%ds

	lcall	*gfx_ofs_v2_jmp_table + 4 * 0 (%di)

	sbbl	%ebx,%ebx
	negl	%ebx

	popw	%ds

	call	EXT_C(real_to_prot)
	.code32

	sti

	popl	%ebp

	movl	%ebx,%eax
	xorl	$1,%ebx
	movl	8(%ebp),%edx
	movl	%ebx,gfx_ofs_v2_ok(%edx)

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * int gfx_done_v2 (gfx_data_v2_t *gfx_data)
 *
 * shut down gfx things
 *
 * return vales:
 *   always 0
 *   sets gfx_data->ok
 */

ENTRY(gfx_done_v2)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	8(%ebp),%edx
	movl	%edx,%ebx
	andl	$0xf,%ebx
	shrl	$4,%edx

	pushl	%ebp

	call	EXT_C(prot_to_real)
	.code16

	pushw	%ds

	movw	%dx,%ds

	lcall	*gfx_ofs_v2_jmp_table + 4 * 1 (%bx)

	popw	%ds

	call	EXT_C(real_to_prot)
	.code32

	sti

	popl	%ebp

	xorl	%eax,%eax
	movl	8(%ebp),%edx
	movl	%eax,gfx_ofs_v2_ok(%edx)

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * int gfx_input_v2 (gfx_data_v2_t *gfx_data, int *menu_entry)
 *
 * let user enter a command line
 *
 * uses gfx_data->cmdline as buffer
 *
 * return values:
 *   1: abort
 *   2: boot
 *   menu_entry: selected entry
 */

ENTRY(gfx_input_v2)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	8(%ebp),%edx
	movl	%edx,%ebx
	leal	gfx_ofs_v2_sys_cfg(%edx),%esi
	andl	$0xf,%ebx
	shrl	$4,%edx

	pushl	%ebp

	call	EXT_C(prot_to_real)
	.code16

	pushw	%ds

	movw	%dx,%ds

	movl	gfx_ofs_v2_cmdline(%bx),%edi
	movl	gfx_ofs_v2_cmdline_len(%bx),%ecx
	movl	gfx_ofs_v2_timeout(%bx),%eax
	imull	$18,%eax

	lcall	*gfx_ofs_v2_jmp_table + 4 * 2 (%bx)

	movl	%eax,%ecx

	popw	%ds

	call	EXT_C(real_to_prot)
	.code32

	sti

	popl	%ebp

	movl	12(%ebp),%edx
	movl	%ebx,(%edx)

	movl	%ecx,%eax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * int gfx_setup_menu (gfx_data_t *gfx_data)
 *
 * draw boot menu
 *
 * return values:
 *   always 0
 */

/* menu entry descriptor */
#define menu_v2_entries		0
#define menu_v2_default		2	/* seg:ofs */
#define menu_v2_ent_list		6	/* seg:ofs */
#define menu_v2_ent_size		10
#define menu_v2_arg_list		12	/* seg:ofs */
#define menu_v2_arg_size		16
#define sizeof_menu_v2_desc	18

ENTRY(gfx_setup_menu_v2)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	8(%ebp),%edx
	movl	%edx,%ebx
	andl	$0xf,%ebx
	shrl	$4,%edx

	call	EXT_C(prot_to_real)
	.code16

	pushw	%ds

	movw	%dx,%ds
	shll	$4,%edx

	subw	$sizeof_menu_v2_desc,%sp
	movw	%sp,%bp

	movl	gfx_ofs_v2_menu_entries(%bx),%eax
	movw	%ax,menu_v2_entries(%bp)

	movl	gfx_ofs_v2_menu_default_entry(%bx),%eax
	subl	%edx,%eax
	movw	%ax,menu_v2_default(%bp)
	movw	%ds,menu_v2_default+2(%bp)

	movl	gfx_ofs_v2_menu_list(%bx),%eax
	subl	%edx,%eax
	movw	%ax,menu_v2_ent_list(%bp)
	movw	%ds,menu_v2_ent_list+2(%bp)

	movl	gfx_ofs_v2_menu_entry_len(%bx),%eax
	movw	%ax,menu_v2_ent_size(%bp)

	movl	gfx_ofs_v2_args_list(%bx),%eax
	subl	%edx,%eax
	movw	%ax,menu_v2_arg_list(%bp)
	movw	%ds,menu_v2_arg_list+2(%bp)

	movl	gfx_ofs_v2_args_entry_len(%bx),%eax
	movw	%ax,menu_v2_arg_size(%bp)

	movl	%ss,%esi
	shll	$4,%esi
	addl	%ebp,%esi

	lcall	%ds: *gfx_ofs_v2_jmp_table + 4 * 3 (%bx)

	addw	$sizeof_menu_v2_desc,%sp

	popw	%ds

	call	EXT_C(real_to_prot)
	.code32

	sti

	xorl	%eax,%eax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 *
 * end graphics stuff
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 */
#endif /* SUPPORT_GFX */


/*
 * int gateA20(int linear)
 *
 * Gate address-line 20 for high memory.
 *
 * This routine is probably overconservative in what it does, but so what?
 *
 * It also eats any keystrokes in the keyboard buffer.  :-(
 *
 *	on call:	linear=0 for a20 off and 1 for on
 *
 *	return value:	0 for failure and 1 for success
 *
 */

ENTRY(gateA20)

	.code32

	pushl	%ebp
	movl	8(%esp), %edx		/* the value of `linear' */

#if 1
	/* first, check if A20 status is already what we desired. */

	/* disable CPU cache for the test to work reliably. */
	movl	%cr0, %eax
	pushl	%eax			/* save old cr0 */
//	andl	$0x00000011, %eax
	orl	$0x60000000, %eax	/* set CD and NW */
	movl	%eax, %cr0
	movl	%cr0, %eax
	testl	$0x60000000, %eax	/* check if we can use wbinvd. */
	jz	1f			/* CPU has no wbinvd instruction. */
	wbinvd
	andl	$0xDFFFFFFF, %eax	/* clear NW */
	movl	%eax, %cr0
1:
	movl	0x00000000, %eax
	pushl	%eax			/* save old int0 vector */

	cmpl	0x00100000, %eax
	jne	1f			/* A20 is on */

	notl	0x00000000
	movl	0x00000000, %eax
	cmpl	0x00100000, %eax
	notl	0x00000000		/* logical `NOT' won't touch flags */
1:
	/* ZF=0(means not equal) for A20 on, ZF=1(means equal) for A20 off. */
	sete	%al			/* save ZF to AL */
	testl	%edx, %edx
	sete	%ah			/* save ZF to AH */
	cmpb	%al, %ah

	/* ZF=1(means equal) for desired and we needn't do anything. */

	popl	%eax			/* restore int0 vector */
	movl	%eax, 0x00000000
	popl	%eax			/* restore cr0 */
	movl	%eax, %cr0
	movl	$1, %eax		/* return value 1 means success */

	jz	1f			/* success */

#endif

	/* failure, then call a real-mode function enable_disable_a20 */

	call	EXT_C(prot_to_real)

	.code16

	sti		/* for hardware interrupt or watchdog */

	movw	$0x00ff, %cx		# try so many times on failure
	movb	$0x01, %dh		# with a20 debug on

	cli	/* yes, keep interrupt off when controlling A20 */
	call	enable_disable_a20
	sti		/* for hardware interrupt or watchdog */

	sete	%dl			# DL=1 means success

#if 0	/* comment out to avoid hanging ONDA C51G board. */
	pushal

#if 0
	# qemu arrow keys will not work if we turn on NumLock in this way.

	/* Turn on Number Lock */
	orb	$0x20, 0x417
#endif

	/* reset mouse */
	movw	$0xC201, %ax
	int	$0x15

	/* disable mouse */
	movw	$0xC200, %ax
	xorw	%bx, %bx	/* BH=0 means disable */
	int	$0x15

	popal
#endif

	call	EXT_C(real_to_prot)

	.code32

	sti

	movzbl	%dl, %eax

1:
	popl	%ebp
	ret
chain_stage1_esp:
	.long	0

/****************************************************************************/

/*
 * long realmode_run(long regs_ptr)
 *
 */

ENTRY(realmode_run)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	movl	8(%ebp), %ebx		/* EBX=regs_ptr */

	/* check sanity */

	xorl	%eax, %eax		/* EAX=0 for failure */

	movl	0x20(%ebx), %ecx	/* GS */
	incl	%ecx
	jz	1f
	decl	%ecx
	shrl	$16, %ecx
	jnz	2f			/* failure */
1:

	movl	0x24(%ebx), %ecx	/* FS */
	incl	%ecx
	jz	1f
	decl	%ecx
	shrl	$16, %ecx
	jnz	2f			/* failure */
1:

	movl	0x28(%ebx), %ecx	/* ES */
	incl	%ecx
	jz	1f
	decl	%ecx
	shrl	$16, %ecx
	jnz	2f			/* failure */
1:

	movl	0x2C(%ebx), %ecx	/* DS */
	incl	%ecx
	jz	1f
	decl	%ecx
	shrl	$16, %ecx
	jnz	2f			/* failure */
1:

	movl	0x30(%ebx), %ecx	/* SS */
	movl	0x0C(%ebx), %edx	/* ESP */
	incl	%ecx
	jz	1f
	decl	%ecx
	orl	%edx, %ecx
	shrl	$16, %ecx
	jnz	2f			/* failure */
	jmp	3f
1:
	incl	%edx
	jnz	2f			/* failure */
3:

	movl	0x38(%ebx), %ecx	/* CS */
	movl	0x34(%ebx), %edx	/* EIP */
	incl	%ecx
	jz	1f
	decl	%ecx
	orl	%edx, %ecx
	shrl	$16, %ecx
	jnz	2f			/* failure */
	jmp	3f
1:
	cmpb	$0xCD, %dl
	jnz	2f			/* failure */
	shrl	$16, %edx
	incw	%dx
	jnz	2f			/* failure */
3:
	/* sanity check ok, run it! */

	pushal
	pushal
	pushal
	pushal
	movl	%esp, ABS(chain_stage1_esp)

	/* mov the struct to stack */
	subl	$64, %esp
	movl	%esp, %edi
	movl	%ebx, %esi
	movl	$16, %ecx
	cld
	repz movsl

	movl	%esp, %ebp	/* EBP point to struct on stack */

	call	EXT_C(prot_to_real)

	.code16

	sti

	movl	%ebp, %esp

	/* check CS:EIP */

	movl	0x38(%bp), %ecx		/* CS */
	movl	0x34(%bp), %edx		/* EIP */
	incl	%ecx			/* CS = -1? */
	jz	3f			/* yes, jump to INTxx code */

	/* run user defined code */
	decl	%ecx			/* CS */
	/* 0xEA might have been changed to 0xCD previously, so we must restore
	 * it back to 0xEA.
	 */
	movb	$0xEA, ABS(1f-1)	/* opcode for "far jump" */
	movw	%dx, ABS(1f)		/* IP in the far jump instruction */
	movw	%cx, ABS(1f + 2)	/* CS in the far jump instruction */

	/* set return address for user code to return */
	movl	$ABS(4f), %ecx
	movl	%ecx, 0x8201
	jmp	6f
3:
	/* run int xx */
	movw	%dx, ABS(1f - 1)	/* int XX */
	movb	$0xE9, ABS(1f + 1)	/* jmp */
	movw	$(4f - 1f - 4), ABS(1f + 2)
6:
	movw	%sp, ABS(8f)		/* initial SP */
	movw	%ss, ABS(7f)		/* initial SS */

	/* set DS, ES, FS, GS */
	movl	0x20(%bp), %ecx		/* GS */
	incl	%ecx
	jz	5f
	decl	%ecx
	movw	%cx, %gs
5:

	movl	0x24(%bp), %ecx		/* FS */
	incl	%ecx
	jz	5f
	decl	%ecx
	movw	%cx, %fs
5:

	movl	0x28(%bp), %ecx		/* ES */
	incl	%ecx
	jz	5f
	decl	%ecx
	movw	%cx, %es
5:

	movl	0x2C(%bp), %ecx		/* DS */
	incl	%ecx
	jz	5f
	decl	%ecx
	movw	%cx, %ds
5:

	movl	0x30(%bp), %ecx		/* SS */
	incl	%ecx
	jz	5f
	decl	%ecx
	movw	%cx, ABS(7f)
	movw	0x0C(%bp), %dx		/* SP */
	movw	%dx, ABS(8f)
5:
	movl	0x3C(%bp), %ecx		/* EFLAGS */
	incl	%ecx
	jz	5f
	decl	%ecx
	pushl	%ecx
	popfl
5:
	popal				/* setup general registers */

	/* move SS and SP */
	.byte	0xBC			/* MOV SP */
7:
	.word	0			/* SS */
	movw	%sp, %ss
	.byte	0xBC			/* MOV SP */
8:
	.word	0			/* SP */

	/* far jump to user code */
	.byte	0xEA			/* ljmp */
1:
	.word	0			/* IP */
	.word	0			/* CS */

4:	/* return from user code */

	/* CS:IP need not care */

	/* use the stack already built by the user */

	pushal
	pushfl
	pushl	%cs
	pushl	$0
	pushl	%ss
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	movl	%ss, %ebp
	shll	$4, %ebp
	xorl	%eax, %eax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%sp, %ax
	addl	%eax, %ebp		/* physical address of stack */

	call	EXT_C(real_to_prot)

	.code32

	sti

	cld
	movl	%ebp, %esi

	movl	ABS(chain_stage1_esp), %esp
	movl	16(%esp), %ebx		/* EBX=regs_ptr */

	leal	32(%ebx), %edi
	movl	$8, %ecx
	repz movsl

	leal	32(%ebp), %esi
	movl	%ebx, %edi
	movl	$8, %ecx
	repz movsl

	/* the stack should not change! */

	/* fix the pushed ESPs before checking */
	movl	$32, %ecx
	subl	%ecx, (32 + 12)(%esp)
	movl	$64, %ecx
	subl	%ecx, (64 + 12)(%esp)
	movl	$96, %ecx
	subl	%ecx, (96 + 12)(%esp)

	movl	%esp, %esi
	leal	32(%esp), %edi
	movl	$8, %ecx
	repz cmpsl
	jnz	1f
	movl	%esp, %esi
	leal	64(%esp), %edi
	movl	$16, %ecx
	repz cmpsl
	jnz	1f
	popal
	popal
	popal
	popal
	movl	$1, %eax		/* success */
	popl	%ebx
	popl	%ebp
	ret
1:
	popal
	popal
	popal
	popal
	xorl	%eax, %eax		/* failure */
2:
	popl	%ebx
	popl	%ebp
	ret


realmode_int:

	.code32

	pushl	%eax

	call	EXT_C(prot_to_real)

	.code16

	sti

	.byte	0xCD			/* opcode for "INT xx" */
VARIABLE(interrupt_number)
	.byte	0			/* interrupt number */

	call	EXT_C(real_to_prot)

	.code32

	sti

	popl	%eax

	iret

/*
 * linux_boot()
 *
 * Does some funky things (including on the stack!), then jumps to the
 * entry point of the Linux setup code.
 */

VARIABLE(linux_text_len)
	.long	0

VARIABLE(linux_data_tmp_addr)
	.long	0

VARIABLE(linux_data_real_addr)
	.long	0

VARIABLE(linux_bzimage_tmp_addr)
//	.long	SYSTEM_RESERVED_MEMORY
	.long	LINUX_TMP_MEMORY

ENTRY(linux_boot)

	.code32

	cli

	/* don't worry about saving anything, we're committed at this point */
	cld	/* forward copying */

	///* realmode_idtdesc is at memory address below 0x10000 */
	//lidt	realmode_idtdesc

	/* copy kernel */
	movl	EXT_C(linux_text_len), %ecx
	addl	$3, %ecx
	shrl	$2, %ecx
	movl	EXT_C(linux_bzimage_tmp_addr), %esi	# SYSTEM_RESERVED_MEMORY
	movl	$LINUX_ZIMAGE_ADDR, %edi		# 0x10000

	rep movsl

	jmp	linux_real_mode_startup

ENTRY(big_linux_boot)

	.code32

	cli
	cld

	///* realmode_idtdesc is at memory address below 0x10000 */
	//lidt	realmode_idtdesc

	/* copy kernel */
	movl	EXT_C(linux_text_len), %ecx
	addl	$3, %ecx
	shrl	$2, %ecx
	movl	EXT_C(linux_bzimage_tmp_addr), %esi	# SYSTEM_RESERVED_MEMORY
	movl	$LINUX_BZIMAGE_ADDR, %edi		# 0x100000

	rep movsl

linux_real_mode_startup:

	movl	EXT_C(linux_data_real_addr), %ebx

	/* copy the real mode part */
	movl	EXT_C(linux_data_tmp_addr), %esi
	movl	%ebx, %edi
	movl	$LINUX_SETUP_MOVE_SIZE, %ecx	# 0x9400
	cld
	rep movsb

	/* change %ebx to the segment address */
	shrl	$4, %ebx	#; CS of LINUX SETUP, high word = 0
	movl	%ebx, %eax
	addl	$0x20, %eax	#; one sector
	movw	%ax, 1f		// linux_setup_seg

	/* final setup for linux boot */

	call	EXT_C(prot_to_real)

	.code16

	/* final setup for linux boot */
	cli
	movw	%bx, %ss
	movw	$LINUX_SETUP_STACK, %sp		# 0x9000

	movw	%bx, %ds
	movw	%bx, %es
	movw	%bx, %fs
	movw	%bx, %gs

	/* jump to start */
	/* ljmp */
	.byte	0xea
	.word	0
1:				//linux_setup_seg:
	.word	0

#if 0
/*
 * void toggle_blinking (void)
 * BIOS call "INT 10H Function 1003h" to toggle intensity/blinking bit
 *	Call with	%ax = 0x1003
 *			%bl = new state:
 *				0x00 enable background intensity
 *				0x01 enable blinking
 *			%bh = 0x00 to avoid problems on some adapters
 *      Returns         nothing
 */


ENTRY(toggle_blinking)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	EXT_C(blinking), %ebx
	call	EXT_C(prot_to_real)

	.code16

	//sti		/* it is not bad keeping interrupt off */
	sti		/* for hardware interrupt or watchdog */

        xorb	%bh, %bh
	testb	%bl, %bl
	setnz	%bl
	movw	$0x1003, %ax
	int	$0x10

	call	EXT_C(real_to_prot)
	.code32

	sti

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
#endif

/*
 * unsigned int console_putchar (unsigned int c, unsigned int max_width)
 *
 * Put the character C on the console. Because GRUB wants to write a
 * character with an attribute, this implementation is a bit tricky.
 * If C is a control character (CR, LF, BEL, BS), use INT 10, AH = 0Eh
 * (TELETYPE OUTPUT). Otherwise, save the original position, put a space,
 * save the current position, restore the original position, write the
 * character and the attribute, and restore the current position.
 *
 * The reason why this is so complicated is that there is no easy way to
 * get the height of the screen, and the TELETYPE OUPUT BIOS call doesn't
 * support setting a background attribute.
 */
ENTRY(console_putchar)

	.code32

	movl	0x4(%esp), %edx
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movl	EXT_C(current_color), %ebx

	movb	EXT_C(fontx), %cl
	movb	EXT_C(fonty), %ch


	call	EXT_C(prot_to_real)
	.code16

	sti		/* for hardware interrupt or watchdog */

	movb	%dl, %al
	xorb	%bh, %bh
#if 0		//通常从0列开始打印，不会出现这些情况 2022-11-11
	cmpb	$80, %cl
	jb	5f
	movb	$0, %cl
	movw	%cx, %dx
	pushaw
	movb	$0x02, %ah
	int	$0x10
	popaw
	incb	%ch

	cmpb	$25, %ch
	jb	5f
	/* scroll up */
	pushaw
	movw	$0x0e0d, %ax
	int	$0x10
	movw	$0x0e0a, %ax
	int	$0x10
	popaw
	decb	%ch
	jmp	3f
#endif
5:
	movw	%cx, %dx
	pushaw
	movb	$0x02, %ah
	int	$0x10
	popaw
3:

	/* use teletype output if control character */
	cmpb	$0x07, %al
	je	1f
	cmpb	$0x08, %al
	je	1f
	cmpb	$0x0a, %al
	je	1f
	cmpb	$0x0d, %al
	je	1f

//	/* save the character and the attribute on the stack */
//	pushw	%ax
//	pushw	%bx
//
//	/* get the current position */
//	movb	$0x03, %ah
//	int	$0x10
//
//	/* check the column with the width */
//	cmpb	$79, %dl
//	jb	2f
//
//	/* print CR and LF, if next write will exceed the width */
//	movw	$0x0e0d, %ax
//	int	$0x10
//	movw	$0x0e0a, %ax
//	int	$0x10
//
//	/* get the current position */
//	movb	$0x03, %ah
//	int	$0x10
//
//2:
//	/* restore the character and the attribute */
//	popw	%bx
//	popw	%ax

	pushw	%cx

	/* write the character with the attribute */
	movb	$0x09, %ah
	movw	$1, %cx
	int	$0x10

	popw	%dx

	/* move the cursor forward */
	incb	%dl
//	movb	$0x02, %ah
//	int	$0x10

	/* if showcursor, print CRLF on needed */
	testb	$1, ABS(EXT_C(cursor_state))
	jz	3f

	cmpb	$80, %dl
	jb	5f
	movb	$0, %dl
	pushaw
	movb	$0x02, %ah
	int	$0x10
	popaw
	incb	%dh

	cmpb	$25, %dh
	jb	5f
	/* scroll up */
	pushaw
	movw	$0x0e0d, %ax
	int	$0x10
	movw	$0x0e0a, %ax
	int	$0x10
	popaw
	decb	%dh
	movw	$0x0920, %ax	//在BIOS环境下，在VM或VBOX的控制台中，如果一行首字符非标准色，滚屏时会花屏。2022-11-11
	movw	$80, %cx
	movb	$0, %bl
	int	$0x10
	jmp	3f
5:
	pushaw
	movb	$0x02, %ah
	int	$0x10
	popaw
	jmp	3f

1:
	movb	$0x0e, %ah
	int	$0x10
	/* update fontx, fonty */
	xorb	%bh, %bh                /* set page to 0 */
	movb	$0x3, %ah
	int	$0x10			/* get cursor position */

3:
	call	EXT_C(real_to_prot)
	.code32

	sti

	/* EAX = 0 */

	movb	%dl, %al
	movl	%eax, EXT_C(fontx)
	movb	%dh, %al
	movl	%eax, EXT_C(fonty)

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	movl	$0x00000001, %eax	/* always return 1 char printed. */
	ret


/*
 * remap_ascii_char remaps the ascii code %dl to another if the code is
 * contained in ASCII_KEY_MAP.
 */
remap_ascii_char:

	.code32

	//pushl	%esi

	movl	$ABS(EXT_C(ascii_key_map)), %esi
1:
	lodsl
	/* check if this is the end */
	testl	%eax, %eax
	jz	2f
	/* check if this matches the ascii code */
	cmpw	%ax, %dx
	jne	1b
	/* if so, perform the mapping */
	shrl	$16, %eax
	movw	%ax, %dx
2:
	//popl	%esi

	ret

	.align	4
ENTRY(ascii_key_map)
	.space	(KEY_MAP_SIZE + 1) * 4


/*
 * int console_getkey (void)
 * BIOS call "INT 16H Function 00H" to read character from keyboard
 *	Call with	%ah = 0x0
 *	Return:		%ah = keyboard scan code
 *			%al = ASCII character
 */

ENTRY(console_getkey)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

#if 0
	/* MacBook cannot work with this pmode code. Comment out 2012-10-15 */

	cld
	xorl	%eax, %eax
1:
	cli
	movl	$0x41A, %esi
	lodsw		/* EAX=buf_head, ESI=0x41C, point to buf_tail */
	cmpw	%ax, (%esi)	/* head == tail ? */
	sti		/* Enable interrupt. Wait for a key. */
	je	1b	/* No key. loop and continue to check buffer. */
1:
	/* Found a key code in buffer. */
	/* EAX = buf_head */
	/* get int16 key code */
	movzwl	0x400(%eax), %edx	/* DL=ASCII code, DH=scan code */
	/* clear ASCII code if needed */
	cmpb	$0xE0, %dl
	je	2f
	cmpb	$0xF0, %dl
	jne	1f
	cmpb	$0x00, %dh
	je	1f
2:
	movb	$0x00, %dl
1:
	/* set new buf_head */
	incl	%eax
	incl	%eax
#if 0
	movl	$0x480, %esi	/* point to buf_start and buf_end */
	cmpw	2(%esi), %ax	/* buf_head >= buf_end ? */
#else
	cmpw	$0x003E, %ax	/* buf_head >= buf_end ? */
#endif
	jb	1f
#if 0
	movw	(%esi), %ax	/* let buf_head = buf_start */
#else
	movw	$0x001E, %ax	/* let buf_head = buf_start */
#endif
1:
	/* update buf_head in the BIOS Data Area. */
	movw	%ax, 0x41A

	/* the int16 key code is in DX */
	call	remap_ascii_char	/* ESI changed */
	movzwl	%dx, %eax
#else
4:
	call	EXT_C(prot_to_real)

	.code16

1:
	sti		/* getkey needs interrupt on */
	// hlt
	call	conditional_hlt

	#; work around for Apple BIOS getkey bug
	#; check the keyboard buffer, until there is a keypress.

#if 0
	/* first, use normal check key function */
	movb	$0x01, %ah		#; check key
	int	$0x16
	jz	2f			#; no normal key, try enhanced key

	/* found a normal key */
	movb	$0x00, %ah		#; get the normal key
	int	$0x16
	jmp	1f
2:
#endif

#if 1
	/* then, use enhanced check key function */
	movb	$0x11, %ah		#; check enhanced key
	int	$0x16
//	jz	1b			#; no keypress, loop
	movl	$0xFFFFFFFF, %edx	/* value for no key-press */
	jz	3f			/* jump if no key-press */

	/* found an enhanced key */
	movb	$0x10, %ah		#; get the enhanced key
	int	$0x16
	/* clear ASCII code if needed */
	cmpb	$0xE0, %al
	je	2f
	cmpb	$0xF0, %al
	jne	1f
	cmpb	$0x00, %ah
	je	1f
2:
	movb	$0x00, %al
#else
	/* avoid calling enhanced functions */
	jmp	1b
#endif
1:
	movzwl	%ax, %edx
	//call	translate_keycode
3:
	call	EXT_C(real_to_prot)
	.code32

	sti

	cmpl	$0xFFFFFFFF, %edx
	jne	1f
  pushl $1
  call defer;
  popl %eax
  cmpb $0, beep_enable
  je 2f
  call beep_func
2:
	cmpb $0, DateTime_enable
	je 2f
	call DateTime_refresh
2:
	cmpl $0, graphics_inited
	je 4b
	cmpb $0, animated_enable
	je 4b
	call animated
	cmpw $0x3c00, %ax
	jne 4b
	movw %ax, %dx
1:
	call	remap_ascii_char	/* ESI changed */
	movzwl	%dx, %eax
#endif
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * int console_checkkey (void)
 *	if there is a character pending, return it; otherwise return -1
 * BIOS call "INT 16H Function 01H" to check whether a character is pending
 *	Call with	%ah = 0x1
 *	Return:
 *		If key waiting to be input:
 *			%ah = keyboard scan code
 *			%al = ASCII character
 *			Zero flag = clear
 *		else
 *			Zero flag = set
 */
ENTRY(console_checkkey)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	xorl	%edx, %edx

#if 0
	/* MacBook cannot work with this pmode code. Comment out 2012-10-15 */

	decl	%edx	/* EDX = -1 */	

	cld
	xorl	%eax, %eax
1:
	cli
	movl	$0x41A, %esi
	lodsw		/* EAX=buf_head */
			/* ESI=0x41C, point to buf_tail */
	cmpw	%ax, (%esi)	/* head == tail ? */
	sti		/* Enable interrupt. Wait for a key. */
	je	9f	/* no key press. return -1 immediately. */
1:
	/* Found a key code in buffer. */
	/* EAX = buf_head */
	/* get int16 key code */
	movzwl	0x400(%eax), %edx	/* DL=ASCII code, DH=scan code */
	/* clear ASCII code if needed */
	cmpb	$0xE0, %dl
	je	2f
	cmpb	$0xF0, %dl
	jne	1f
	cmpb	$0x00, %dh
	je	1f
2:
	movb	$0x00, %dl
1:
	/* return the key in DX immediately */
	//jmp	9f
#else

	call	EXT_C(prot_to_real)	/* enter real mode */

	.code16

	movl	$500, %ecx
1:
	movl	0x46c, %eax	/* get old timer counter */

	sti		/* checkkey needs interrupt on */
	// hlt
	call	conditional_hlt

	cmpl	0x46c, %eax	/* timer counter changed by int 8? */
	loopz	1b		/* no change, wait a timer interrupt */

	/* counter changed or timed out. */

#if 0
	/* first, use normal check key function */
	movb	$0x01, %ah		#; check key
	int	$0x16
	jnz	2f			/* there is a key-press. */
#endif

#if 1
	/* then, use enhanced check key function */
	movb	$0x11, %ah		#; check enhanced key
	int	$0x16
	movl	$0xFFFFFFFF, %edx	/* value for no key-press */
	jz	1f			/* jump if no key-press */
	/* clear ASCII code if needed */
	cmpb	$0xE0, %al
	je	3f
	cmpb	$0xF0, %al
	jne	4f
	cmpb	$0x00, %ah
	je	4f
3:
	movb	$0x00, %al
4:
#else
	/* avoid calling enhanced functions */
	movl	$0xFFFFFFFF, %edx	/* value for no key-press */
	jmp	1f
#endif
2:
	/* Yes, there is a key. */
	movzwl	%ax, %edx
	//call	translate_keycode

1:
	call	EXT_C(real_to_prot)
	.code32

	sti
#endif

9:
	cmpl	$0xFFFFFFFF, %edx
	jne 3f
	pushl $1
	call defer
	popl %eax
	cmpb $0, beep_enable
	jz 2f
	call beep_func
2:
	cmpb $0, DateTime_enable
	je 2f
	call DateTime_refresh
2:
	cmpl $0, graphics_inited
	je 5f
	cmpb $0, animated_enable
	je 5f
	call animated
5:
	movl $0xFFFFFFFF, %edx
	jmp 1f
3:
	call	remap_ascii_char	/* ESI changed */
1:
	mov	%edx, %eax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * int console_getxy (void)
 * BIOS call "INT 10H Function 03h" to get cursor position
 *	Call with	%ah = 0x03
 *			%bh = page
 *      Returns         %ch = starting scan line
 *                      %cl = ending scan line
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */


ENTRY(console_getxy)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	call	EXT_C(prot_to_real)

	.code16

	sti		/* for hardware interrupt or watchdog */

        xorb	%bh, %bh                /* set page to 0 */
	movb	$0x3, %ah
	int	$0x10			/* get cursor position */

	call	EXT_C(real_to_prot)
	.code32

	sti

	//movb	%dl, %ah
	//movb	%dh, %al
	movw	%dx, %ax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * void console_gotoxy(int x, int y)
 * BIOS call "INT 10H Function 02h" to set cursor position
 *	Call with	%ah = 0x02
 *			%bh = page
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */


ENTRY(console_gotoxy)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	0x14(%esp), %eax		/* x */
	movl	%eax, EXT_C(fontx)
	movb %al, %dl
	movl	0x18(%esp), %eax		/* y */
	movl %eax, EXT_C(fonty)
	movb %al, %dh

	call	EXT_C(prot_to_real)

	.code16

	sti		/* for hardware interrupt or watchdog */

        xorb	%bh, %bh                /* set page to 0 */
	movb	$0x2, %ah
	int	$0x10			/* set cursor position */

	call	EXT_C(real_to_prot)
	.code32

	sti

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * void console_cls (void)
 * BIOS call "INT 10H Function 09h" to write character and attribute
 *	Call with	%ah = 0x09
 *                      %al = (character)
 *                      %bh = (page number)
 *                      %bl = (attribute)
 *                      %cx = (number of times)
 */


ENTRY(console_cls)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movb	EXT_C(current_color), %bh
	call	EXT_C(prot_to_real)

	.code16

	sti		/* for hardware interrupt or watchdog */
/*
* BIOS call "INT 10H Function 06h" to Scroll up window
*	Call with	%AH = 0x06
*			%AL = lines to scroll (0 = clear, CH, CL, DH, DL are used)
*			%BH = attribute
*			%CH = Upper row number
*			%CL = Left column number
*			%DH = Lower row number
*			%DL = Right column number
*/
	movw $0x0600, %ax
	xorw %cx, %cx
	movb 0x44a, %dl //40:004a   Screen width in text columns
	movb 0x484, %dh //40:0084   maximum valid row value
	int	$0x10
	xorb	%bh, %bh
	/* move back the cursor */
	xorw	%dx, %dx
	movb	$0x02, %ah
	int	$0x10

	call	EXT_C(real_to_prot)
	.code32

	sti

	/* EAX = 0 */
	movl	%eax, EXT_C(fontx)
	movl	%eax, EXT_C(fonty)

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


ENTRY(console_beep)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	call	EXT_C(prot_to_real)
	.code16

	sti		/* for hardware interrupt or watchdog */
	cmpw $0, EXT_C(beep_frequency)
	je 1f
	movb $0xb6, %al
	outb %al, $0x43
	movw EXT_C(beep_frequency), %ax
	outb %al, $0x42
	xchgb %ah, %al
	outb %al, $0x42
	inb $0x61, %al
	orb $3, %al
	outb %al, $0x61
	jmp 2f
1:
	inb $0x61, %al
	andb $0xFC, %al
	outb %al, $0x61
2:
	call	EXT_C(real_to_prot)
	.code32

	sti
	movw	%dx, %ax

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
  

/*
 * void console_setcursor (unsigned long on)
 * BIOS call "INT 10H Function 01h" to set cursor type
 *      Call with       %ah = 0x01
 *                      %ch = cursor starting scanline
 *                      %cl = cursor ending scanline
 */

VARIABLE(cursor_state)
	.long	0 //bit 0=1 show cursor,bit 1=1 show splashimage
console_cursor_shape:
	.word	0

ENTRY(console_setcursor)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	/* check if the standard cursor shape has already been saved */
	movw	console_cursor_shape, %ax
	testw	%ax, %ax
	jne	1f

	call	EXT_C(prot_to_real)

	.code16

	sti		/* for hardware interrupt or watchdog */

	movb	$0x03, %ah
	xorb	%bh, %bh
	int	$0x10

	call	EXT_C(real_to_prot)
	.code32

	sti

	movw	%cx, console_cursor_shape
1:
	/* set %cx to the designated cursor shape */
	movw	$0x2000, %cx
	movl	0x14(%esp), %ebx	/* on */
	testl	%ebx, %ebx
	jz	2f
	movw	console_cursor_shape, %cx
2:
	call	EXT_C(prot_to_real)

	.code16

	sti		/* for hardware interrupt or watchdog */

	movb    $0x1, %ah
	int     $0x10

	call	EXT_C(real_to_prot)
	.code32

	sti

	//movb	cursor_state, %al
	//movb	%bl, cursor_state

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

/* graphics mode functions */
#ifdef SUPPORT_GRAPHICS
/*
 * int set_videomode(mode)
 * BIOS call "INT 10H Function 0h" to set video mode
 *	Call with	%ah = 0x0
 *			%al = video mode
 *      //Returns old videomode.
 *	Return the current mode value stored at 0040:0049 in BIOS Data Area
 */
ENTRY(set_videomode)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movb	0x14(%esp), %cl

	call	EXT_C(prot_to_real)

	.code16

	sti		/* for hardware interrupt or watchdog */

	//xorw	%bx, %bx
	//movb	$0xf, %ah
	//int	$0x10			/* Get Current Video mode */
	//movb	%al, %ch
	xorb	%ah, %ah
	movb	%cl, %al
	pushw	%cx
	pushw	%ds			// DS = 0
	pushw	%es			// ES = 0
	int	$0x10			/* Set Video mode */
	popw	%es			// ES = 0
	popw	%ds			// DS = 0
	popw	%cx

	cmpb	0x449, %cl		// DS = 0
	jne	1f
	movw	$0x8000, %di		// use 64-byte buffer at 0000:8000
	xorl	%eax, %eax
	movw	$0x11, %cx		// clear the buffer (64 + 4) bytes
	cld
	rep stosl
	movw	$0x8000, %di		// use 64-byte buffer at 0000:8000
	movw	$0x1B00, %ax		// get current mode infomation
	xorw	%bx, %bx		// BX=0 for return state infomation
	int	$0x10
	// the caller want to confirm AL=0x1B for a successful return.
	movb	%al, %cs:(0x8000+64)	// it is certain CS = 0
1:
	xorb	%bh, %bh
	movb	$0, %bl
	movw	$0x1003, %ax
	int	$0x10
	call	EXT_C(real_to_prot)
	.code32

	sti

	//xorb	%ah, %ah
	//movb	%ch, %al
	movb	0x449, %al

	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret


/*
 * unsigned char * graphics_get_font()
 * BIOS call "INT 10H Function 11h" to set font
 *      Call with       %ah = 0x11
 */
ENTRY(graphics_get_font)

	.code32

	push	%ebp
	push	%ebx
	push	%esi
	push	%edi

	call	EXT_C(prot_to_real)

	.code16

	sti		/* for hardware interrupt or watchdog */

	movw	$0x1130, %ax
	movb	$6, %bh		/* font 8x16 */
	int	$0x10
	movw	%bp, %dx
	movw	%es, %cx

	call	EXT_C(real_to_prot)
	.code32

	sti

	xorl	%eax, %eax
	movw	%cx, %ax
	shll	$4, %eax
	movw	%dx, %ax

	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ebp
	ret


/*
 * void bios_scroll_up()
 * BIOS call "INT 10H Function 6h" to scroll up window
 *
 * VIDEO - SCROLL UP WINDOW
 * AH = 06h
 * AL = number of lines by which to scroll up (00h = clear entire window)
 * BH = attribute used to write blank lines at bottom of window
 * CH,CL = row,column of window's upper left corner
 * DH,DL = row,column of window's lower right corner
 *
 * Return: Nothing
 *
 * Note: Affects only the currently active page (see AH=05h)
 *
 * BUGS: Some implementations (including the original IBM PC) have a bug
 * which destroys BP. The Trident TVGA8900CL (BIOS dated 1992/9/8) clears
 * DS to 0000h when scrolling in an SVGA mode (800x600 or higher)
 */
ENTRY(bios_scroll_up)

	.code32

	push	%ebp
	push	%ebx
	push	%esi
	push	%edi

	movb	EXT_C(x1), %dl	/* bytes per line */
	movb	EXT_C(y1), %dh	/* number of lines */

	call	EXT_C(prot_to_real)

	.code16

	sti

	decb	%dl
	decb	%dh
	movw	$0x0601, %ax	/* scroll up 1 line */
	movb	$0, %bh		/* use background color */
	xorw	%cx, %cx	/* upper left corner */
	int	$0x10

	call	EXT_C(real_to_prot)
	.code32

	sti

	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ebp
	ret



/*
 * graphics_set_palette(index, color)
 * BIOS call "INT 10H Function 10h" to set individual dac register
 *	Call with	%ah = 0x10
 *			%bx = register number
 *			%ch = new value for green (0-63)
 *			%cl = new value for blue (0-63)
 *			%dh = new value for red (0-63)
 */

ENTRY(graphics_set_palette)

	.code32

	push	%ebp
	push	%ebx
	push	%esi
	push	%edi

#if 0
	movw	$0x3c8, %bx		/* address write mode register */

	/* wait vertical retrace */

	movw	$0x3da, %dx
1:	inb	%dx, %al	/* wait vertical active display */
	test	$8, %al
	jnz	1b

2:	inb	%dx, %al	/* wait vertical retrace */
	test	$8, %al
	jnz	2b

	mov	%bx, %dx
	movb	0x14(%esp), %al		/* index */
	outb	%al, %dx
	inc	%dx

	movb	0x1A(%esp), %al		/* red */
	shrb	$2, %al
	outb	%al, %dx

	movb	0x19(%esp), %al		/* green */
	shrb	$2, %al
	outb	%al, %dx

	movb	0x18(%esp), %al		/* blue */
	shrb	$2, %al
	outb	%al, %dx
#endif
	movw	0x14(%esp), %bx
#if 1
	movl	0x18(%esp), %ecx
	movl	%ecx, %edx
	shrl	$10, %edx		/* DH = red */
	shrb	$2, %ch			/* CH = green */
	shrb	$2, %cl			/* CL = blue */
#endif

	call	EXT_C(prot_to_real)

	.code16

	sti		/* for hardware interrupt or watchdog */

#if 1
	// VIDEO - SET INDIVIDUAL DAC REGISTER (VGA/MCGA)
	//
	// AX = 1010h
	// BX = register number
	// CH = new value for green (0-63)
	// CL = new value for blue (0-63)
	// DH = new value for red (0-63)
	//
	// Return: Nothing 

	pushal
	movw	$0x1010, %ax
	int	$0x10
	popal
#endif

	// VIDEO - SET SINGLE PALETTE REGISTER (PCjr,Tandy,EGA,MCGA,VGA)
	//
	// AX = 1000h
	// BL = palette register number (00h-0Fh)
	//    = attribute register number (undocumented, see below)
	// BH = color or attribute register value
	//
	// Return: Nothing
	//
	// Notes: On MCGA, only BX = 0712h is supported. Under UltraVision,
	// the palette locking status (see AX=CD01h) determines the outcome
	//
	// (undocumented) values for attribute register number:
	// 10h	attribute mode control register (should let BIOS control this)
	// 11h	overscan color register
	// 12h	color plane enable register (bits 3-0 enable corresponding text attribute bit)
	// 13h	horizontal PEL panning register
	// 14h	color select register

	movb	%bl, %bh
	movw	$0x1000, %ax
	int	$0x10

	call	EXT_C(real_to_prot)
	.code32

	sti

	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ebp
	ret

#endif /* SUPPORT_GRAPHICS */

/*
 * getrtsecs()
 *	if a seconds value can be read, read it and return it (BCD),
 *      otherwise return 0xFF
 * BIOS call "INT 1AH Function 02H" to check whether a character is pending
 *	Call with	%ah = 0x2
 *	Return:
 *		If RT Clock can give correct values
 *			%ch = hour (BCD)
 *			%cl = minutes (BCD)
 *                      %dh = seconds (BCD)
 *                      %dl = daylight savings time (00h std, 01h daylight)
 *			Carry flag = clear
 *		else
 *			Carry flag = set
 *                         (this indicates that the clock is updating, or
 *                          that it isn't running)
 */
ENTRY(getrtsecs)

	.code32

	push	%ebp

	call	EXT_C(prot_to_real)	/* enter real mode */

	.code16

	#;sti		/* getrtsecs needs interrupt on */
	sti	#; added 2006-11-30

#if 0
	movb	$0x2, %ah

	/* Ralf Brown's Interrupt List says:
	 *
	 * BUG: Some BIOSes leave CF unchanged if successful,
	 * so CF should be cleared before calling this function
	 */

	clc
	int	$0x1a

	DATA32	jnc	gottime
	movb	$0xff, %dh

gottime:
#else
	/* The call int1A/ah=02 could fail all the time.
	 * So we should avoid using it. Instead, we use ticks at 0040:006C.
	 *					- Tinybit 2007-04-21
	 */
	pushl	%ecx
	movl	0x46C, %eax
//	movl	$5, %ecx	/* 5 seconds */
//	mull	%ecx
	xorl	%edx, %edx
//	movl	$91, %ecx	/* 91 ticks = 5 seconds */
	movl	$18, %ecx
	divl	%ecx
	xorl	%edx, %edx
	movl	$60, %ecx
	divl	%ecx		/* EDX=seconds (0 .. 59) */
	movb	%dl, %dh
	popl	%ecx

#endif
	call	EXT_C(real_to_prot)
	.code32

	sti

	movb	%dh, %al

	pop	%ebp
	ret


/*
 *  void get_datetime(unsigned long *date, unsigned long *time);
 */
ENTRY(get_datetime)

	.code32

	pushl	%ebp
	call	EXT_C(prot_to_real)

	.code16

	sti		/* for hardware interrupt or watchdog */
	movb	$2, %ah
	clc
	int	$0x1a
	jc	2f

	pushw	%cx
	pushw	%dx

	movb	$4, %ah
	clc
	int	$0x1a
	jc	3f

	pushw	%cx
	pushw	%dx
	popl	%edx
	popl	%ecx
	jmp	1f

3:
	popl	%eax

2:
	xorl	%ecx, %ecx
	xorl	%edx, %edx

1:
	call	EXT_C(real_to_prot)
	.code32

	sti

	movl	%esp, %ebp
	movl	8(%ebp), %eax
	movl	%edx, (%eax)
	movl	12(%ebp), %eax
	movl	%ecx, (%eax)

	popl	%ebp
	ret

#if 0
	/* This BIOS call should NOT be called since it will clear the byte at 0040:0070. */

/*
 * currticks()
 *	return the real time in ticks, of which there are about
 *	18-20 per second
 */
ENTRY(currticks)
	.code32
	pushl	%ebp

	call	EXT_C(prot_to_real)	/* enter real mode */

	.code16

	#;sti		/* currticks needs interrupt on */
	sti	#; added 2006-11-30

	/* %ax is already zero */
        int	$0x1a

	call	EXT_C(real_to_prot)
	.code32

	sti

	movl	%ecx, %eax
	shll	$16, %eax
	movw	%dx, %ax

	popl	%ebp
	ret
#endif

/*
 * multi_boot(int start, int mb_info, int, int, int, int, int)
 *
 *  This starts a kernel in the manner expected of the multiboot standard.
 */

ENTRY(multi_boot)

	.code32

	cli

	/* The protected-mode IDT at 3M will be destroyed. Although cli has
	 * been executed, the multiboot kernel might still need to call the
	 * real-mode BIOS functions(int15/E820, etc). So we need to load the
	 * default real-mode IDT descriptor.
	 */

	lidt	realmode_idtdesc

	/* Now the IDT is wrong for protected-mode code! Do not sti in
	 * pmode. Only do it in real-mode.
	 */

	/* before moving kernel, move module list(mll) down to 0x20000. */

	movl	$ABS(EXT_C(mll)), %esi
	movl	$0x20000, %edi	/* at physical address 128K. */
	movl	$0x200, %ecx	/* (0x200*4=)2K is enough for mll[99]. */
	cld
	repz movsl

	/* move image down to 1M. This will destroy code at above 1M. So this
	 * function must be at below 1M, together with real-mode functions.
	 */

//	movl	$(SYSTEM_RESERVED_MEMORY + 0x0100000), %esi
	movl	$(LINUX_TMP_MEMORY + 0x0100000), %esi
	movl	$0x0100000, %edi				/* 1M */
	movl	ABS(EXT_C(cur_addr)), %ecx
	subl	%esi, %ecx
	addl	$3, %ecx
	shrl	$2, %ecx
	cld
	repz movsl

	///* no need to save anything */
	//call	EXT_C(stop_floppy)

	movl	0x10(%esp), %ecx
	jecxz	1f

	movl	$0x2BADB002, %eax
	movl	0x8(%esp), %ebx

	/* boot kernel here (absolute address call) */
	call	*0x4(%esp)

	/* error */
	call	EXT_C(stop)

1:
	/* BSD boot */
	popl	%eax		/* discard return address */
	popl	%eax		/* EAX=entry_addr */
	call	*%eax

	/* error */
	call	EXT_C(stop)


/*
 *  This is the area for all of the special variables.
 */

	.align	4

#;protstack:
#;	.long	PROTSTACKINIT

	/* an address can only be long-jumped to if it is in memory, this
	   is used by multiple routines */
offset:
	.long	0x8000
segment:
	.word	0

#if 0
VARIABLE(apm_bios_info)
	.word	0	/* version */
	.word	0	/* cseg */
	.long	0	/* offset */
	.word	0	/* cseg_16 */
	.word	0	/* dseg_16 */
	.word	0	/* cseg_len */
	.word	0	/* cseg_16_len */
	.word	0	/* dseg_16_len */
#endif

/*
 * This is the Global Descriptor Table
 *
 *  An entry, a "Segment Descriptor", looks like this:
 *
 * 31          24         19   16                 7           0
 * ------------------------------------------------------------
 * |             | |B| |A|       | |   |1|0|E|W|A|            |
 * | BASE 31..24 |G|/|0|V| LIMIT |P|DPL|  TYPE   | BASE 23:16 |
 * |             | |D| |L| 19..16| |   |1|1|C|R|A|            |
 * ------------------------------------------------------------
 * |                             |                            |
 * |        BASE 15..0           |       LIMIT 15..0          |
 * |                             |                            |
 * ------------------------------------------------------------
 *
 *  Note the ordering of the data items is reversed from the above
 *  description.
 */

#if 0
	.align	16
gdt:
	.word	0, 0
	.byte	0, 0, 0, 0

	/* code segment */
	.word	0xFFFF, 0
	.byte	0, 0x9A, 0xCF, 0

	/* data segment */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0xCF, 0

	/* 16 bit real mode CS */
	.word	0xFFFF, 0
	.byte	0, 0x9E, 0, 0

	/* 16 bit real mode DS */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0, 0


/* this is the GDT descriptor */
gdtdesc:
	.word	0x27			/* limit */
	.long	gdt			/* addr */
#endif

pmode_idtdesc:
	.word	0x7FF
	.long	0x300000	/* at 3M */
	.word	0	// pad


/* this code will be moved to and get executed at HMA_ADDR=0x2B0000 */

/* our gdt starts at HMA_ADDR=0x2B0000 */

ENTRY(HMA_start)

	.code32

	/* the first entry of GDT, i.e., the default null entry,
	 * can be any value. it never get used. So we use these
	 * 8 bytes for our jmp and GDT descriptor.
	 */

	. = EXT_C(HMA_start) + 0	/* GDT entry: default null */

	jmp	1f		/* two-byte short jmp */

	. = EXT_C(HMA_start) + 2

	/* 6-byte GDT descriptor */
gdtdescHMA:
	.word	0x1F		/* limit */
	.long	HMA_ADDR	/* linear base address */

	. = EXT_C(HMA_start) + 8	/* 16-bit data 64K limit */

	/* real mode data segment base=0x200 */
	.word	0xFFFF, 0x0200
	.byte	0x00, 0x92, 0, 0

	. = EXT_C(HMA_start) + 0x10	/* 32-bit data 4GB limit */

	/* data segment, although it is no use here for now */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0xCF, 0

	. = EXT_C(HMA_start) + 0x18	/* 16-bit code 64K limit */

	/* 16-bit code segment base=0x2B0000 */
	.word	0xFFFF, 0x0000
	.byte	0x2B, 0x9E, 0, 0

realmode_idtdesc:
	.word	0x3FF
	.long	0
	.word	0	// pad

1:
	/* set up to pass boot drive */
	movb	EXT_C(boot_drive), %dl

	/* check if the --ebx option is given. */
	movl	(EXT_C(chain_ebx_set) - EXT_C(HMA_start) + HMA_ADDR), %eax
	testl	%eax, %eax
	jz	1f
	movl	(EXT_C(chain_ebx) - EXT_C(HMA_start) + HMA_ADDR), %ebx
1:

	/* check if the --edx option is given. */
	movl	(EXT_C(chain_edx_set) - EXT_C(HMA_start) + HMA_ADDR), %eax
	testl	%eax, %eax
	jz	1f
	movl	(EXT_C(chain_edx) - EXT_C(HMA_start) + HMA_ADDR), %edx
1:

	/* move new loader from extended memory to conventional memory.
	 * this will overwrite our GRUB code, data and stack, so we should not
	 * use instuctions like push/pop/call/ret, and we should not use
	 * functions like gateA20().
	 */

	/* the new loader is currently at 0x200000 */
	movl	$0x00200000, %esi
	xorl	%eax, %eax
	xorl	%edi, %edi
	movw	(EXT_C(chain_load_segment) - EXT_C(HMA_start) + HMA_ADDR), %di
	shll	$4, %edi
	movw	(EXT_C(chain_load_offset) - EXT_C(HMA_start) + HMA_ADDR), %ax
	addl	%eax, %edi
	//movl	$0x00007c00, %edi
	movl	(EXT_C(chain_load_length) - EXT_C(HMA_start) + HMA_ADDR), %ecx
	cld
	repz movsb

	/* switch to real mode */

	/* check if the --cx option is given. */
	movl	(EXT_C(chain_cx_set) - EXT_C(HMA_start) + HMA_ADDR), %eax
	testl	%eax, %eax
	jz	1f
	movw	(EXT_C(chain_cx) - EXT_C(HMA_start) + HMA_ADDR), %cx
1:
	/* check if the --bx option is given. */
	movl	(EXT_C(chain_bx_set) - EXT_C(HMA_start) + HMA_ADDR), %eax
	testl	%eax, %eax
	jz	1f
	movw	(EXT_C(chain_bx) - EXT_C(HMA_start) + HMA_ADDR), %bx
1:

	/* set new GDT */
	cli
	lgdt	(gdtdescHMA - EXT_C(HMA_start) + HMA_ADDR)
	lidt	(realmode_idtdesc - EXT_C(HMA_start) + HMA_ADDR)

	/* set up segment limits */
	movw	$PSEUDO_RM_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	movl	$0x200, %esp	/* points to end of interrupt vector table */
				/* SS base=0x200, so SS:SP=physical 0x400 */

	movl	$0x7c00, %ebp

	/* jump to a 16 bit segment, this might be an extra step:
	 * set up CS limit, also clear high word of EIP
	 */
	ljmp	$PSEUDO_RM_CSEG, $(1f - EXT_C(HMA_start))
1:
	.code16

	/* clear the PE bit of CR0 */
	movl	%cr0, %eax
	//andl	$CR0_PE_OFF, %eax
	andl 	$0x0000FFFE, %eax
	movl	%eax, %cr0

	/* setup DS, ES, SS, FS and GS before loading CS */
	xorl	%eax, %eax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movl	$0x400, %esp
	movw	%ax, %fs
	movw	%ax, %gs

	/* flush prefetch queue, reload %cs */

	.byte	0xEA		/* ljmp 0000:7c00 */
VARIABLE(chain_boot_IP)
	.word	0x7c00		/* offset */
VARIABLE(chain_boot_CS)
	.word	0x0000		/* segment */

	.align	4

VARIABLE(chain_load_offset)
	.word	0x7c00
VARIABLE(chain_load_segment)
	.word	0x0000
VARIABLE(chain_load_length)
	.long	0x200
VARIABLE(chain_ebx)
	.long	0
VARIABLE(chain_ebx_set)
	.long	0
VARIABLE(chain_edx)
	.long	0
VARIABLE(chain_edx_set)
	.long	0
VARIABLE(chain_enable_gateA20)
	.long	0
VARIABLE(chain_bx)
	.word	0
VARIABLE(chain_bx_set)
	.long	0
VARIABLE(chain_cx)
	.word	0
VARIABLE(chain_cx_set)
	.long	0

	/* max length of code is 1 sector */
	. = . - (. - EXT_C(HMA_start))/0x201

	.align	4

VARIABLE(mbi)
	.space	(22 * 4)

VARIABLE(end_of_low_16bit_code)

	/* ensure this resides in the first 64KB */
	. = . - (ABS(.) / 0x10001)

	/* Although real-mode code ends at this point, the dynamic data
	 * of multiboot will use a piece of memory here.
	 */


/****************************************************************************/
/************************* 32-bit functions follow **************************/
/****************************************************************************/


	.space	0x300000	/* !!!! insert 3M !!!! */

#if 0
/*
 * int check_64bit (void)
 *
 * Checks whether 64-bit mode is supported
 *
 * Stolen from a patch originaly intended for syslinux
 * (http://syslinux.zytor.com/archives/2007-January/007832.html)
 *
 * Copyright (C) 2007 Byron Stanoszek <gandalf@winds.org>
 *
 * Adapted to AT&T syntax by Robert Millan <rmh@aybabtu.com>
 */

ENTRY(check_64bit)

	.code32

	pushl	%ebp
	pushl	%ebx
	pushl	%edx

	/* Check if this CPU supports the CPUID command */
	pushfl
	pushfl
	popl	%eax
	movl	%eax, %ebx
	xorl	$(1 << 21), %eax	// CPUID bit
	pushl	%eax
	popfl
	pushfl
	popl	%eax
	popfl				// Restore the original flags
	xorl	%ebx, %eax
	jz	is_32bit

	/* Now check for the 64-bit flag in the CPU features byte ($0000_0001, edx)
	This is bit 30 for Intel CPUs, and bit 29 for AMD CPUs */
	movl	$0x00000000, %eax	// Find last Intel cpuid #
	cpuid
	cmpl	$0x00000000, %eax
	je	test_amd
	movl	$0x00000001, %eax	// Read Intel CPU flags
	cpuid
	btl	$30, %edx		// 64-bit if bit 30 is set
	jc	is_64bit

test_amd:
	movl	$0x80000000, %eax	// Find last AMD cpuid #
	cpuid
	cmpl	$0x80000000, %eax
	jbe	is_32bit
	movl	$0x80000001, %eax	// Read AMD CPU flags
	cpuid
	btl	$29, %edx		// 64-bit if bit 29 is set
	jnc	is_32bit

is_64bit:
	movl	$1, %eax
	popl	%edx
	popl	%ebx
	popl	%ebp
	ret
is_32bit:
	xorl	%eax, %eax
	popl	%edx
	popl	%ebx
	popl	%ebp
	ret
#endif

#if 0
/*
 * set_int15_handler(void)
 *
 * Set up int15_handler.
 */
ENTRY(set_int15_handler)

	.code32

	pushl	%edi

	/* save the original int15 handler */
	movl	$0x54, %edi
#if 0
	movw	(%edi), %ax
	movw	%ax, ABS(int15_offset)
	movw	2(%edi), %ax
	movw	%ax, ABS(int15_segment)

	/* save the new int15 handler */
	movw	$ABS(int15_handler), %ax
	movw	%ax, (%edi)
	xorw	%ax, %ax
	movw	%ax, 2(%edi)
#else
	movl	(%edi), %eax
	movl	%eax, ABS(int15_offset)

	/* set the new int15 handler */
	movl	$ABS(int15_handler), %eax
	stosl
#endif

	popl	%edi
	ret


/*
 * unset_int15_handler(void)
 *
 * Restore the original int15 handler
 */
ENTRY(unset_int15_handler)

	.code32

	pushl	%edi

	/* check if int15_handler is set */
	movl	$0x54, %edi
#if 0
	movw	$ABS(int15_handler), %ax
	cmpw	%ax, (%edi)
	jne	1f
	xorw	%ax, %ax
	cmpw	%ax, 2(%edi)
	jne	1f

	/* restore the original */
	movw	ABS(int15_offset), %ax
	movw	%ax, (%edi)
	movw	ABS(int15_segment), %ax
	movw	%ax, 2(%edi)
#else
	movl	$ABS(int15_handler), %eax
	cmpl	%eax, (%edi)
	jne	1f

	/* restore the original */
	movl	ABS(int15_offset), %eax
	stosl
#endif

1:
	popl	%edi
	ret
#endif


/*
 * set_int13_handler(map)
 *
 * Copy MAP to the drive map and set up int13_handler.
 */
ENTRY(set_int13_handler)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%edi
	pushl	%esi

#if	MAP_NUM_16
	/* copy boot_drive_map to maphooked_drive_map_1 */
	movl	$(DRIVE_MAP_SIZE * DRIVE_MAP_SLOT_SIZE / 8), %ecx
	pushl	%ecx
	movl	$ABS(EXT_C(hooked_drive_map_1)), %edi
	movl	8(%ebp), %esi
	cld
	repz movsl
	/* copy boot_drive_map to maphooked_drive_map_2 */
	popl %ecx
	movl	$ABS(EXT_C(hooked_drive_map_2)), %edi
	repz movsl
#else
	/* copy boot_drive_map to maphooked_drive_map */
	movl	$(DRIVE_MAP_SIZE * DRIVE_MAP_SLOT_SIZE / 4), %ecx
	movl	$ABS(EXT_C(hooked_drive_map)), %edi
	movl	8(%ebp), %esi
	cld
	repz movsl
#endif

	/* other variable(s) */
	movb	ABS(EXT_C(is64bit)), %dl
	movb	%dl, ABS(int13_is64bit)
	
	movl	0x4C, %eax
	movl	%eax, ABS(EXT_C(ROM_int13))
	movl	%eax, ABS(EXT_C(ROM_int13_dup))

//	Now initialized early at the beginning of this file
//
//	/* save the original int13 handler */
//	movl	$0x4c, %edi
//	movl	(%edi), %eax
//	movl	%eax, ABS(EXT_C(ROM_int13))

	/* decrease the lower memory size and set it to the BIOS memory */

	/* read available lower memory size */
	movzwl  0x413, %eax
#ifdef CDROM_INIT
	/* KBytes that int13 handler occupies */
	movl	$((int13_handler_end - int13_handler + 0x3ff) / 0x400), %edx	
//	cmpb	$0, %cs:(EXT_C(atapi_dev_count) - int13_handler)
	cmpb	$0, ABS(EXT_C(atapi_dev_count))
	jne	1f
#endif
	movl	$((cdrom_start - int13_handler + 0x3ff) / 0x400), %edx	
1:		
//	movzbl	ABS(int13_handler), %edx # EDX=KBytes used by new int13_handler
	movb	%dl, ABS(int13_handler)
	subl	%edx, %eax		 # EAX=new available low memory(KBytes)
	/* save remaining lower memory size in BIOS data area */
	movw	%ax, 0x413
	
	/* copy int13_handler to the reserved area */
	shll    $10, %eax	
	movl    %eax, %edi	# EDI=EAX=destination int13_handler address
	movl	$ABS(int13_handler), %esi
#ifdef CDROM_INIT
	movl	$((int13_handler_end - int13_handler + 3) / 4), %ecx
//	cmpb	$0, %cs:(EXT_C(atapi_dev_count) - int13_handler)
	cmpb	$0, ABS(EXT_C(atapi_dev_count))
	jne	1f
#endif
	movl	$((cdrom_start - int13_handler + 3) / 4), %ecx
1:
	// cld
	repz	movsl
	
	movl	%eax, %edi	# EDI=EAX=linear address of int13_handler

	///* save size (KBytes) of int13_handler in its header */
	//movb	%dl, (%edi)

#if 0
	movl	%eax, (int15_e820_range0_addr - int13_handler)(%edi)
#endif
	/* linear address of int13_handler (below 0x100000). yes, use 'orl'
	 * because we only write the lower 24 bits of the linear addess.
	 * The higher byte(8 bits) is on the highest byte of the gdt entry. 
	 */
	orl	%eax, (PM_DS16 + 2 + MyGDT - int13_handler)(%edi)
	orl	%eax, (PM_CS16 + 2 + MyGDT - int13_handler)(%edi)
	//orl	%eax, (PM_CS32 + 2 + MyGDT - int13_handler)(%edi)

	/* calculate linear address of MyGDT and save in gdtdesc+2 */
	addl	$(MyGDT - int13_handler), %eax	# EAX=linear address of MyGDT
	movl	%eax, (gdtdesc - int13_handler + 2)(%edi)

	/* calculate linear address of int13_lm64move_lm64_start */
	addl	$(int13_lm64move_lm64_start - MyGDT), %eax
	movl	%eax, (int13_lm64move_lm64_start_addr - int13_handler)(%edi)

	movl	%edi, %eax	# EDI=EAX=linear address of int13_handler

	/* real mode segment of int13_handler */
	shrl	$4, %eax
	movw	%ax, (int13_pgmove_to_rm_cs - int13_handler)(%edi)

	shll	$16, %eax	# move segment to high word
	movw	$(int13_handler_entry - int13_handler), %ax	# AX=0x100
	movl	%eax, 0x004C	# save the new int13 handler
	
#if 0	/* comment out so that int15 always hooks. */
	/* set ESI to the drive map */
	movl	$ABS(EXT_C(hooked_drive_map)), %esi
	movl	$(DRIVE_MAP_SIZE), %ecx
1:
	cmpb	$0xFF, 1(%esi)	# It is memdrive?
	jne	2f		# No. Try next slot.
	testb	$0x40, 5(%esi)	# To_DRIVE is CDROM?
	jz	1f		# No. Memdrive indeed. Hook int15.
2:
	/* try next slot */
	addl	$DRIVE_MAP_SLOT_SIZE, %esi
	loop	1b
	jmp	2f			/* no memdrives, don't hook int15 */
1:
#endif
	movl	(EXT_C(e820cycles) - int13_handler)(%edi), %ecx
	jecxz	2f

	/* save the new int15 handler. segment still in high word of EAX. */
	movw	$(int15_e820_handler - int13_handler), %ax	# offset
	movl	%eax, 0x54
2:
	popl	%esi
	popl	%edi
	popl	%ebp
	ret

/* int
 * unset_int13_handler(check_status_only)
 *
 * Restore the original int13 handler
 *
 * Return 0 for success and non-zero for failure.
 */
ENTRY(unset_int13_handler)

	.code32

	pushl	%ebp
	movl	%esp, %ebp

	pushl	%edi

	/* check if int13_handler is set */
	movl	$0x413, %edi
	movw	(%edi), %ax
	cmpw	$640, %ax
	jae	1f		#; needn't unset
//	cmpw	$632, %ax
//	jb	1f
	shll	$(16 + 6), %eax

	/* the offset(low word) should be 0x100 */
//	movw	$0x100, %ax
	movw $(int13_handler_entry - int13_handler), %ax

	cmpl	%eax, 0x4c
	jne	1f		#; not hooked, unset failure
	movl	%eax, %edi
	shrl	$12, %edi	/* segment base address */

	/* check our int13 signature "$INT13SFGRUB4DOS" */
//	cmpl	$0x544E4924, 0x103(%edi)	/* $INT */
	cmpl	$0x544E4924, (int13_handler_entry - int13_handler + 3)(%edi)	/* $INT */
	jnz	1f
//	cmpl	$0x46533331, 0x107(%edi)	/* 13SF */
	cmpl	$0x46533331, (int13_handler_entry - int13_handler + 7)(%edi)	/* 13SF */
	jnz	1f
//	cmpl	$0x42555247, 0x10B(%edi)	/* GRUB */
	cmpl	$0x42555247, (int13_handler_entry - int13_handler + 0xb)(%edi)	/* GRUB */
	jnz	1f
//	cmpl	$0x534F4434, 0x10F(%edi)	/* 4DOS */
	cmpl	$0x534F4434, (int13_handler_entry - int13_handler + 0xf)(%edi)	/* 4DOS */
	jnz	1f

	//cmpl	$0x9A000000, 0x1C(%edi)		/* old int 13 */
	cmpl	$0x5A000000, 0x1C(%edi)		/* old int 13 */
	jb	1f

	//cmpl	$0x9A000000, 0x0C(%edi)		/* old int 15 */
	cmpl	$0x5A000000, 0x0C(%edi)		/* old int 15 */
	jb	1f

	movl	ABS(EXT_C(ROM_int13)), %eax
	cmpl	0x1C(%edi), %eax
	jnz	1f

	movl	ABS(EXT_C(ROM_int15)), %eax
	cmpl	0x0C(%edi), %eax
	jnz	1f

	xorl	%eax, %eax

	cmpl	%eax, 8(%ebp)
	jnz	1f

	/* increase the lower memory size */

	movzbw	(%edi), %ax
	addw	%ax, 0x413

	/* restore the original int15 handler */
	movl	ABS(EXT_C(ROM_int15)), %eax
	movl	%eax, 0x54

	/* restore the original int13 handler */
	movl	ABS(EXT_C(ROM_int13)), %eax
	movl	%eax, 0x4c

	xorl	%eax, %eax			/* success */
1:
	/* return non-zero for failure */
	popl	%edi
	popl	%ebp
	ret

#if 0
/* memmove using SSE */
/*
 * void memmove_forward_SSE (void *dst, const void *src, unsigned int len)
 *
 */

ENTRY(memmove_forward_SSE)

	.code32

	pushl	%esi
	pushl	%edi

#if 1
	movl	%cr0, %eax
	orb	$2, %al		// set CR0.MP
	movl	%eax, %cr0
	movl	%cr4, %eax
	orb	$0x6, %ah	// set CR4.OSFXSR (bit 9) OSXMMEXCPT (bit 10)
	movl	%eax, %cr4
#endif

	movl	0x0C(%esp), %edi
	movl	0x10(%esp), %esi
	movl	0x14(%esp), %ecx
	shrl	$7, %ecx
1:
	movdqa	(%esi), %xmm0
	addl	$16, %esi
	movntps	%xmm0, (%edi)
	addl	$16, %edi
	movdqa	(%esi), %xmm1
	addl	$16, %esi
	movntps	%xmm1, (%edi)
	addl	$16, %edi
	movdqa	(%esi), %xmm2
	addl	$16, %esi
	movntps	%xmm2, (%edi)
	addl	$16, %edi
	movdqa	(%esi), %xmm3
	addl	$16, %esi
	movntps	%xmm3, (%edi)
	addl	$16, %edi
	movdqa	(%esi), %xmm4
	addl	$16, %esi
	movntps	%xmm4, (%edi)
	addl	$16, %edi
	movdqa	(%esi), %xmm5
	addl	$16, %esi
	movntps	%xmm5, (%edi)
	addl	$16, %edi
	movdqa	(%esi), %xmm6
	addl	$16, %esi
	movntps	%xmm6, (%edi)
	addl	$16, %edi
	movdqa	(%esi), %xmm7
	addl	$16, %esi
	movntps	%xmm7, (%edi)
	addl	$16, %edi
	loop	1b

	popl	%edi
	popl	%esi
	ret
#endif

/* get_code_end() :  return the address of the end of the code
 * This is here so that it can be replaced by asmstub.c.
 */

ENTRY(get_code_end)

	.code32

	/* will be the end of the bss */
# if defined(HAVE_END_SYMBOL)
	movl	$end, %eax
# elif defined(HAVE_USCORE_END_SYMBOL)
	movl	$_end, %eax
# endif
	shrl	$2, %eax		/* Round up to the next word. */
	incl	%eax
	shll	$2, %eax
	ret

int2_nmi:

	.code32

	movb	$0x02, EXT_C(interrupt_number)
	jmp	realmode_int


irq_8_to_15:	/* from the slave interrupt controller */

	.code32

	pushl	%eax
	movb	$0x20, %al	// End Of Interrupt(EOI) command
	outb	%al, $0xA0	// sent EOI to slave controller
	outb	%al, $0x20	// sent EOI to master controller
	popl	%eax
	iret

irq_0_to_7:	/* from the master interrupt controller */

	.code32

	pushl	%eax
	movb	$0x20, %al	// End Of Interrupt(EOI) command
	outb	%al, $0x20	// sent EOI to master controller
	popl	%eax
	iret

irq_0:	/* int 0x08, timer interrupt, also the CPU double fault */

	.code32

	/* double fault has error code 0 on stack.
	 * we never have the EIP = 0.
	 * So check the stack and we can distinguish these two.
	 */

	cmpl	$0, (%esp)
	jnz	1f
	cmpl	$40, 8(%esp)	// it should be the CS if it is double fault
	jnz	1f
	/* CPU double fault */
	addl	$4, %esp	// remove the error code
	/* simply return for now */
	iret
1:
	/* simulate the ROM real-mode routine */
	sti
	pushl	%eax
	movb	0x440, %al	// Diskette Motor shutoff counter
	testb	%al, %al
	jz	1f		// motor already stopped
	decb	%al
	movb	%al, 0x440	// decrement the counter
	jnz	1f		// motor is running, continue
	/* time is out, stop the motor now */
	pushl	%edx
	movw	$0x3F2, %dx	// diskette controller digit-out register
	inb	%dx, %al
	andb	$0xCF, %al	// shut off drive 0 and 1
	outb	%al, %dx
	andb	$0xFC, 0x43F	// Diskette motor status
				// set motor staus to be off for drive 0, 1
	popl	%edx
1:
	movl	0x46C, %eax	// Daily timer counter, cleared at midnight
	incl	%eax		// incremented
	cmpl	$0x1800B0, %eax
	jb	2f
	xorl	%eax, %eax	// cleared
	incb	0x470	//Clock rollover flag, set when 40:6C exceeds 24hrs
2:
	movl	%eax, 0x46C	// get incremented or cleared
	//int	0x1C	// could do it in protected mode, but no need yet

	cli
	movb	$0x20, %al	// End Of Interrupt(EOI) command
	outb	%al, $0x20	// sent EOI to master controller
	sti

	popl	%eax
	iret

irq_1:

	.code32

	movb	$9, EXT_C(interrupt_number)
	jmp	realmode_int

irq_2:

	.code32

	movb	$0x0A, EXT_C(interrupt_number)
	jmp	realmode_int

irq_3:

	.code32

	movb	$0x0B, EXT_C(interrupt_number)
	jmp	realmode_int

irq_4:

	.code32

	movb	$0x0C, EXT_C(interrupt_number)
	jmp	realmode_int

irq_5:

	.code32

	movb	$0x0D, EXT_C(interrupt_number)
	jmp	realmode_int

irq_6:

	.code32

	movb	$0x0E, EXT_C(interrupt_number)
	jmp	realmode_int

irq_7:

	.code32

	movb	$0x0F, EXT_C(interrupt_number)
	jmp	realmode_int

irq_8:

	.code32

	movb	$0x70, EXT_C(interrupt_number)
	jmp	realmode_int

irq_9:

	.code32

	movb	$0x71, EXT_C(interrupt_number)
	jmp	realmode_int

irq_10:

	.code32

	movb	$0x72, EXT_C(interrupt_number)
	jmp	realmode_int

irq_11:

	.code32

	movb	$0x73, EXT_C(interrupt_number)
	jmp	realmode_int

irq_12:

	.code32

	movb	$0x74, EXT_C(interrupt_number)
	jmp	realmode_int

irq_13:

	.code32

	movb	$0x75, EXT_C(interrupt_number)
	jmp	realmode_int

irq_14:

	.code32

	movb	$0x76, EXT_C(interrupt_number)
	jmp	realmode_int

irq_15:

	.code32

	movb	$0x77, EXT_C(interrupt_number)
	jmp	realmode_int

default_iret:

	.code32

	iret


/*
 * The C code for a grub4dos executable may have defines as follows:
 *

#define sprintf ((int (*)(char *, const char *, ...))((*(int **)0x8300)[0]))
#define printf(...) sprintf(NULL, __VA_ARGS__)
#define putstr ((void (*)(const char *))((*(int **)0x8300)[1]))
#define putchar ((void (*)(int))((*(int **)0x8300)[2]))
#define get_cmdline ((int (*)(char *))((*(int **)0x8300)[3]))
#define getxy ((int (*)(void))((*(int **)0x8300)[4]))
#define gotoxy ((void (*)(int, int))((*(int **)0x8300)[5]))
#define cls ((void (*)(void))((*(int **)0x8300)[6]))
#define setcursor ((int (*)(int))((*(int **)0x8300)[7]))
#define nul_terminate ((int (*)(char *))((*(int **)0x8300)[8]))
#define safe_parse_maxint ((int (*)(char **, unsigned long long *))((*(int **)0x8300)[9]))
#define substring ((int (*)(const char *, const char *, int))((*(int **)0x8300)[10]))
#define strstr ((char *(*)(const char *, const char *))((*(int **)0x8300)[11]))
#define strlen ((int (*)(const char *))((*(int **)0x8300)[12]))
#define strtok ((char *(*)(char *, const char *))((*(int **)0x8300)[13]))
#define strncat ((int (*)(char *, const char *, int))((*(int **)0x8300)[14]))
#define strcmp ((int (*)(const char *, const char *))((*(int **)0x8300)[15]))
#define strcpy ((char *(*)(char *, const char *))((*(int **)0x8300)[16]))
#define tolower ((int (*)(int))((*(int **)0x8300)[17]))
#define isspace ((int (*)(int))((*(int **)0x8300)[18]))
#define getkey ((int (*)(void))((*(int **)0x8300)[19]))
#define checkkey ((int (*)(void))((*(int **)0x8300)[20]))
#define sleep ((unsigned int (*)(unsigned int))((*(int **)0x8300)[21]))
#define memcmp ((int (*)(const char *, const char *, int))((*(int **)0x8300)[22]))
#define memmove ((void *(*)(void *, const void *, int))((*(int **)0x8300)[23]))
#define memset ((void *(*)(void *, int, int))((*(int **)0x8300)[24]))
#define mem64 ((int (*)(int, unsigned long long, unsigned long long, unsigned long long))((*(int **)0x8300)[74]))
#define open ((int (*)(char *))((*(int **)0x8300)[26]))
#define read ((unsigned long (*)(unsigned long long, unsigned long long, unsigned long))((*(int **)0x8300)[27]))
#define close ((void (*)(void))((*(int **)0x8300)[28]))

http://grubutils.googlecode.com/svn/trunk/src/include/grub4dos.h
 *
 */

VARIABLE(system_functions)
	.long	ABS(EXT_C(grub_sprintf))
	.long	ABS(EXT_C(grub_putstr))
VARIABLE(grub_putchar)
	.long	ABS(EXT_C(_putchar))
	.long	ABS(EXT_C(get_cmdline_obsolete))
	.long	ABS(EXT_C(getxy))
	.long	ABS(EXT_C(gotoxy))
	.long	ABS(EXT_C(cls))
	.long	ABS(EXT_C(wee_skip_to))	//was obsolete setcursor
	.long	ABS(EXT_C(nul_terminate))
	.long	ABS(EXT_C(safe_parse_maxint_with_suffix))
	.long	ABS(EXT_C(substring))
	.long	ABS(EXT_C(grub_strstr))
	.long	ABS(EXT_C(grub_strlen))
	.long	ABS(EXT_C(grub_strtok))
	.long	ABS(EXT_C(grub_strncat))
	.long	ABS(EXT_C(grub_strcmp))
	.long	ABS(EXT_C(grub_strcpy))
	.long 0				//reserved
	.long 0				//reserved
	.long	ABS(EXT_C(getkey))	/* getchar */
	.long	ABS(EXT_C(checkkey))
	.long	ABS(EXT_C(pxe_call))	//reserved  21
	.long	ABS(EXT_C(grub_memcmp))
	.long	ABS(EXT_C(grub_memmove))
	.long	ABS(EXT_C(grub_memset))
	.long 0				//reserved
	.long	ABS(EXT_C(grub_open))
	.long	ABS(EXT_C(grub_read))
	.long	ABS(EXT_C(grub_close))
	.long 0				//reserved
	.long 0				//reserved
  VARIABLE(disk_read_hook)
	.long 0//reserved
	.long	ABS(EXT_C(devread))
	.long	ABS(EXT_C(devwrite))
	.long	ABS(EXT_C(next_partition))
	.long	ABS(EXT_C(open_device))
	.long	ABS(EXT_C(real_open_partition))
	.long	ABS(EXT_C(set_device))
	.long 0				//reserved
	.long 0				//reserved
	.long 0				//reserved
	.long	ABS(EXT_C(parse_string))
	.long	ABS(EXT_C(hexdump))
	.long	ABS(EXT_C(skip_to))
	.long	ABS(EXT_C(builtin_cmd))
	.long	ABS(EXT_C(get_datetime))
	.long	ABS(EXT_C(find_command))
	.long 0				//reserved
	.long 0				//reserved
	.long	ABS(EXT_C(get_mmap_entry))
	.long	ABS(EXT_C(grub_malloc))
	.long	ABS(EXT_C(grub_free))//51
	.long  0             // list_partitions
	.long  ABS(EXT_C(realmode_run))             // realmode_run
	//reserved for wee
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	////////////////////////////////////////////61
	.long	ABS(EXT_C(dir))
	.long	ABS(EXT_C(print_a_completion))
	.long	ABS(EXT_C(print_completions))
	.long	ABS(EXT_C(lba_to_chs))
	.long	ABS(EXT_C(probe_bpb))
	.long	ABS(EXT_C(probe_mbr))
	.long	ABS(EXT_C(unicode_to_utf8))
	.long	ABS(EXT_C(rawread))
	.long	ABS(EXT_C(rawwrite))
	.long	ABS(EXT_C(setcursor))
	.long	ABS(EXT_C(grub_tolower))
	.long	ABS(EXT_C(grub_isspace))
	.long	ABS(EXT_C(grub_sleep))
	.long	ABS(EXT_C(mem64))
	.long ABS(EXT_C(envi_cmd))
	.long ABS(EXT_C(strncmpx))
#ifdef SUPPORT_GRAPHICS
	.long ABS(EXT_C(rectangle))
#else
	.long 0
#endif
	.long ABS(EXT_C(get_cmdline))

/*
 * The C code for a grub4dos executable may have defines as follows:
 *

#define next_partition_drive		((*(unsigned long **)0x8304)[0])
#define next_partition_dest		((*(unsigned long **)0x8304)[1])
#define next_partition_partition	((*(unsigned long ***)0x8304)[2])
#define next_partition_type		((*(unsigned long ***)0x8304)[3])
#define next_partition_start		((*(unsigned long long ***)0x8304)[4])
#define next_partition_len		((*(unsigned long long ***)0x8304)[5])
#define next_partition_offset		((*(unsigned long long ***)0x8304)[6])
#define next_partition_entry		((*(unsigned long ***)0x8304)[7])
#define next_partition_ext_offset	((*(unsigned long ***)0x8304)[8])
#define next_partition_buf		((*(char ***)0x8304)[9])

 *
 */

VARIABLE(system_variables)
VARIABLE(next_partition_drive)
	.long	0
VARIABLE(next_partition_dest)
	.long	0
VARIABLE(next_partition_partition)
	.long	0
VARIABLE(next_partition_type)
	.long	0
VARIABLE(next_partition_start)
	.long	0
VARIABLE(next_partition_len)
	.long	0
VARIABLE(next_partition_offset)
	.long	0
VARIABLE(next_partition_entry)
	.long	0
VARIABLE(next_partition_ext_offset)
	.long	0
VARIABLE(next_partition_buf)
	.long	0
VARIABLE(quit_print)
	.long	0
//VARIABLE(buf_drive)
	.long	-1				//reserved
//VARIABLE(buf_track)
	.long	-1				//reserved
VARIABLE(filesystem_type)
	.long	0
//VARIABLE(query_block_entries)//reserved
	.long	0				//reserved
//VARIABLE(map_start_sector)//reserved
	.long	0				//reserved
	.long	ABS(EXT_C(buf_geom))
	.long	ABS(EXT_C(tmp_geom))
	.long	ABS(EXT_C(term_table))
VARIABLE(current_term)
	.long	ABS(EXT_C(term_table))
	.long	ABS(EXT_C(fsys_table))
//VARIABLE(fsys_type)//reserved
	.long	NUM_FSYS		//reserved
	.long	NUM_FSYS		//reserved
VARIABLE(graphics_inited)
	.long	0
	.long	0			//reserved
VARIABLE(font8x16)
	.long	0
VARIABLE(fontx)
	.long	0
VARIABLE(fonty)
	.long	0
VARIABLE(graphics_CURSOR)
	.long	0
	.long	(EXT_C(menu_border))
VARIABLE(gzip_filemax)
	.long	0
	.long	0
	//reserved for wee
	.long	0	// reserved 1
	.long	0	// reserved 2
	.long	0	// reserved 3
	.long	0	// reserved 4
	.long	0	// reserved 5
	.long	0	// reserved 6
	.long	0	// reserved 7
	.long	0	// reserved 8
	.long	0	// reserved 9
	.long	0	// reserved 10
	//////////////////////////////
VARIABLE(current_color)
	.long A_NORMAL

VARIABLE(current_color_64bit)
	.long	0xAAAAAA	/* lo 32 bits for foreground */
	.long	0		/* hi 32 bits for background */
VARIABLE(p_get_cmdline_str)
	.long ABS(EXT_C(get_cmdline_str))
VARIABLE(splashimage_loaded)
	.long 0	
VARIABLE(putchar_hooked)
	.long 0
VARIABLE(init_free_mem_start)
	.long 0


/*
  flags for vbe splashimage
  splashimage_loaded & ~0xf   SPLASH_BASE_ADDR
  if image load success set bit 1=1;
  SPLASH_BASE_ADDR+4		image Width.
  SPLASH_BASE_ADDR+8		image height
  SPLASH_BASE_ADDR+16		image data
  the image data use 32bit colors.eg. width=4,height=2
  xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
  xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
*/

