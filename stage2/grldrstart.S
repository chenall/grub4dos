/*
 *  grldrstart.S -- Startup code for GRLDR
 *  Copyright (C) 2004-2007  Tinybit(tinybit@tom.com)
 *  Copyright (C) 2007  Bean(bean@windrv.net)
 *  Copyright (C) 2012  yaya(yaya2007_7777@126.com)
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
 * This program is used to generate the GRLDR file.
 *
 * Use the following shell command to generate the GRLDR file:
 *
 * 	cat grldrstart pre_stage2 > grldr
 *
 */

#define ASM_FILE
#include <shared.h>
#define ASM_FILE
	
#include <stage2_size.h>

#ifdef GRLDR_MBR
	.file	"mbrstart.S"	
#elif defined(GRLDR_INSTALL)
	.file	"bootlacestart.S"
#else
	.file	"grldrstart.S"	
#endif

#ifdef GRLDR_INSTALL
	//.data
#else
	.text
	.globl	start, _start
start:
_start:
#endif



_start1:


	/* Tell GAS to generate 16-bit real mode instructions */

	.code16
	
#if (! defined(GRLDR_INSTALL))	

	. = _start1 + 0x00

	jmp	1f	/* FAT32/NTFS routine comes to offset 0 */

	. = _start1 + 0x02

	.byte	0x90 
	
	/* 6 control bytes moved at offset 0x5A */


#if (! defined(GRLDR_MBR))

	/* filled in by mkisofs using the -boot-info-table option */

#;bi_pvd:	.long 0xDEADBEEF	/* LBA of primary volume descript */
#;bi_file:	.long 0xDEADBEEF	/* LBA of boot file */
#;bi_length:	.long 0xDEADBEEF	/* Length of boot file */
#;bi_csum:	.long 0xDEADBEEF	/* Checksum of boot file */
#;bi_reserved:	.space (10*4)		/* Reserved */

	. = _start1 + 0x40
        
#else

	/* filled in with BPB in case the drive(typically USB) is treated as floppy by buggy BIOSes */

	. = _start1 + 0x5A

mbr_disable:
        # bit0=1: disable GRLDR search on floppy
	# bit1=1: disable booting previous MBR with invalid partition table
	# bit2=1: disable unconditional entrance to the command-line
	# bit3=1: disable geometry tune
	# bit7=1: disable booting previous MBR prior to GRLDR search

	.byte	0x80

	
mbr_time:	
	# time in seconds to wait, 0xFF means waiting forever

	.byte	0	// use 0 instead of 5 for grldr.mbr to work as usual.

	/* a key press to wait. if AX returned from int16 equals this word,
	 * the desired action will occur. */


mbr_hotkey:
	.word	0x3920		/* the space bar */


mbr_boot_number:
	.byte	0xff	# preferred boot drive number, 0xff means undefined
	.byte	0xff	# preferred partition number, 0xff for whole drive

#endif	/* ! defined(GRLDR_MBR) */

1:
//	cli
	xorw	%bx, %bx
	movw	%bx, %ss
	movw	$0x580, %sp		/* temp safe stack space */
//	sti	
	call	1f
1:
	popw	%bx			/* Instruction Pointer of 1b */
	subw	$(1b - _start1), %bx	/* CS:BX=_start1 */

	shrw	$4, %bx
	movw	%cs, %ax
	addw	%ax, %bx		/* BX:0000=_start1 */

	/* we are booted from BOOT.INI, or whole GRLDR image already loaded */

	/* Let CS:0000=_start1 */
	pushw	%bx			/* BX:0000=_start1 */

        pushw	$(1f - _start1)
	lret

1: 
 
#if (! defined(GRLDR_MBR))
     
	addw	$((grldr_signature - _start1 + 4 + STAGE2_SIZE - 4) >> 4), %bx
	movw	%bx, %ds
	cmpl	$0xCE1A02B0, ((STAGE2_SIZE - 4) & 0x0F)
	pushw   %cs
        popw    %ds
	je	grldr_real_start	/* whole image loaded. boot it! */ 

        cmpb    $0X80, %dl                              /* cdrom? */
	jbe     1f                                      /* not */
	call	cdrom_check		                /* cdrom_check present */ 
        andb    $0x80, %dl	

#endif //(! defined(GRLDR_MBR))
	/* CS:0000=DS:0000=_start1 */

	/* we are loaded by BIOS or another boot loader */

#define	MONITOR	0x8410		/* for 4-sector NTFS boot record */
#define GRLDR_CS 0x2000		/* grldr code segment */
1:      
//      cli
	pushw	$GRLDR_CS
	popw	%ss
	movw	$0x9000, %sp	/* SS:SP=0x9d000, keep away from EBDA data */
//	sti
	
	/* Extended BIOS Data Area should not take up space below 0x9d000 */
	
	/* 
	 * 0x07c00-0x07dff	This sector. Another boot loader load us here
	 * 0x0d000-0x0d7ff	partition/floppy boot track(bootsector,etc)
	 * 0x20000-0x27dff	master boot track(MBR,etc,usually 63 sectors)
	 * 0x29000-0x29bff	6 sectors for stack
	 */

#define FS_BOOT	0xd00		/* segment of partition boot track */
        
        cmpb    $0x00, %dl
        je      1f
        movb    $0x80, %dl
1:        
	pushw	%ss		/* SS=CS */
	popw	%es		/* ES=CS */

//      movw	$(((pre_stage2_start - _start1) >> 9) + 0x200), %ax
        movb	$((pre_stage2_start - _start1) >> 9), %al
        xorb    %dh, %dh
        xorw    %bx, %bx
        call    read_disk_with_reset_and_dec_di
        jc      read_help_err

	pushw	%cs		/* DS=0 */
	popw	%ds		/* DS=CS */
	
	/* CS=DS=old segment. ES=SS=new segment. */

	/* Move the code and error messages from DS:0000 to 9400:0000, do not
	 * touch the partition table
	 */
	xorw	%si, %si
	xorw	%di, %di
	movw	$223, %cx	/* 223 words = 446 bytes = 0x1be bytes */
	cld
	repz movsw		/* SI=DI=0x1be, CX=0 */

	/********************************************************************/
	/*  At this moment we are still not sure whether the helper is ok.  */
	/********************************************************************/ 


disk_serial_number_structure:
	/********************************************************************/
	/* This piece of code is structured! It contains address 0x1FFC and */
	/* a disk serial number that can be created by grub4dos installers. */
	/********************************************************************/

	/* if the boot loader has loaded more than one sector, we use them */

        movw	$(grldr_signature - _start1), %bx	/* BX=0x1FFC */

	/* if the boot loader has loaded more than one sector, we use them */
	
	movl	$0xAA555247, %eax	/* "GR" 0x55 0xAA */

disk_serial_number_structure_end:

	cmpl	%eax, (%bx)		/* DS=old segment of 07C0:0000 */
	jne	1f

	/* The MOVE_HELPER code is in the old segment of 07C0:0000 */
	        
	call	move_helper	/* SI=0x1be, CX=0 */
1:

	/* Jump to new segment! */
#if 1
	ljmp	$GRLDR_CS, $(1f - _start1)
#else
	pushw	%ss		/* 0x9400 */

	//pushw	$(1f - _start1)
	.byte	0x6A, (1f - _start1)

	lret
#endif
1:

	/* We are at the new segment. CS=ES=SS=new segment. */

	/* But DS is still old segment. */

	pushw	%ss
	popw	%ds

	/* CS=DS=ES=SS=new segment. */

	//movw	$0x01be, %si

	/* check the existence of helper */
	cmpl	%eax, (%bx)
        je      helper_call_c

	/* try to load helper from floppy/hard */

read_help_err:

        xorb    $0x80, %dl
        pushw   $0x7c0
        popw    %es

//      movw	$(((pre_stage2_start - _start1) >> 9) + 0x200), %ax
        movb	$((pre_stage2_start - _start1) >> 9), %al
        xorb    %dh, %dh
        xorw	%bx, %bx	/* ES:BX immediately follow this sector */	
	call	read_disk_with_reset_and_dec_di
	jc      1f      
	ljmp	$0x7c0, $0 
endup:
1:	
        movw	$(message_string - _start1), %si       
        call	print_message	/* CS:SI points to message string */
1:        
        jmp 1b	
	

helper_call_c:        
        movw    $0x01be, %si

1:
helper_call:

	call	helper_start	/* change to jmp 6f if helper not present */ 
//      testb   $0x80, 0x0a
        testb   $0x80, (help_mark - _start1)
        je      filesystem_boot

6:

add_sub_si:

	/* extended partition check routine will adjust this to
	 *
	 *	0x83, 0xEE, 0x04 for "subw $4, %si"
	 *
	 *			 or
	 *
	 *	0x83, 0xC6, 0xFC for "addw $-4, %si" 
	 *
	 * so that SI keeps the value 0x1fe.
	 */
	addw	$12, %si	/* 0x83, 0xC6, 0x0C */

	. = add_sub_si + 3

	/* extended partition check routine will adjust the word 0x1fe at
	 * (add_sub_si + 5). The value 0x1ff or greater indicates there are
	 * entries need to be treated. The value 0x1fe indicates no entries
	 * left, and the floppy should be checked.
	 */

	cmpw	$0x01fe, %si	/* 0x81, 0xFE, 0xfe, 0x01 */
				/* All entries checked done? */
	jb	1b		/* No, check the next entry */
	ja 2f

	testb $1, (Boot_dl - _start1)
	jne boot_prev_mbr
	orb $1, (Boot_dl - _start1)
	testb $2, (Boot_dl - _start1)
	jne 1b
	pushw	$FS_BOOT
	popw	%es
	movb	$4, %al
	xorw	%dx, %dx
	xorw	%bx, %bx	
	call	read_disk_with_reset_and_dec_di
	jnc helper_call_c
	movw $(No_floppy - _start1), %si       
	call print_message
	jmp boot_prev_mbr
2:
try_HD:
	testb $1, (Boot_dl - _start1)
	jne boot_prev_mbr
	orb $1, (Boot_dl - _start1)
	jmp	Error_modify

int13:
	pushw	%ds
	pushw	%es
//	pushw	%bx
	pushw	%dx
	pushw	%si
	pushw	%di
	pushw	%bp
	stc
	int	$0x13
	popw	%bp
	popw	%di
	popw	%si
	popw	%dx
//	popw	%bx
	popw	%es
	popw	%ds
	ret

try_next_partition:

	cli
	movw	$GRLDR_CS, %ax
	movw	%ax, %ss
	movw	$(0x9000-36), %sp
	sti

	/* restore the registers and continue */
	popal
	popw	%es
	popw	%ds
	jmp	add_sub_si

read_disk_with_reset_and_dec_di:
//2:
	movb    $2, %ah
	movw    $3, %di
	movw    $1, %cx
2:	
	pushaw
////////////////////////////////////////////////////////////
	/* ES:BX=buffer */
	/* AH=2, AL=sectors */
	/* CX=sector and cylinder numbers */
	/* DH=head number, DL=drive number */
3:
	pushw	%ax
	pushw	%bx
	pushw	%cx
	decb	%al
	addb	%al, %cl	/* sector number */
	shlb	$1, %al
	addb	%al, %bh	/* buffer */
	movb	$0x01, %al	/* AH=2 untouched */
	call	int13
	popw	%cx
	popw	%bx
	popw	%ax
	jc	3f
	decb	%al
	jnz	3b
3:
////////////////////////////////////////////////////////////
	popaw
	jnc	3f
	pushaw
	xorw	%ax, %ax
	call	int13
	popaw
	decw	%di
	jne     2b
	stc
3:
	ret

	/* prints string CS:SI (modifies AX BX SI) */
3:
	//xorw	%bx, %bx	/* video page 0 */
	movb	$0x0e, %ah	/* print char in AL */
	int	$0x10		/* via TTY mode */

print_message:

	lodsb	%cs:(%si), %al	/* get token */
	cmpb	$0, %al		/* end of string? */
	jne	3b
	ret 
	
message_string:

	.ascii	"\r\nMissing helper.\0"
	
help_mark:	
	.byte   0       
	
	/* helper detection flag
	 * bit2=1: EBIOS supported
	 * bit3=1: Partition Table OK
	 * bit7=1: Try next entry	
         */
        
Sectors_passed_in:
	.byte	0x3f
	
Heads_passed_in:
	.byte	0xff
	
Boot_dl:
	.byte	0x00		//00/80=FD/HD
	
#if (! defined(GRLDR_MBR))

cdrom_check:

	/* DS=CS points to the sector start. */

	/* BX segment points to near the end of GRLDR image. */

	/* The code must begin with 0x58, 0x8E, 0xD3, 0xBC. */

	popw	%ax	/* old return IP. code: 0x58 */

	/* set BX as the new safe stack. */
	//For compatibility Virtual PC
	movw	$0x6840, %bx
	movw	%bx, %ss
	movw	$0xFFF0, %sp

	pushw	%ax	/* old return IP */

	/* check if DL is no-emulation-mode bootable CDROM. */
	pushw	%ds

	cmpb	$0x80, %dl
	jb	1f	/* not a valid no-emulation-mode cdrom drive number */

	cmpw	$0xAA55, 0x3FE		/* 2048 bytes loaded? */
	jne	1f

	movw	$0x0180, %si
	movw	$0x4B01, %ax
	pushw	$0x0040
	popw	%ds
	movb	$0x13, (%si)
	call	int13

	/* ignore CF */
#;	jc	2f	/* not in emulation mode */
	xorl	%eax, %eax
	xorw	%bp, %bp
	testb	$0x0F, 1(%si)	/* boot media type is No Emulation? */
	jnz	2f	/* no, it simulates floppy or hard disk. */
	cmpb	%dl, 2(%si)	/* drive number */
	jnz	2f	/* invalid drive */

	/* OK! it is no-emulation-mode cdrom drive. */
	movl	4(%si), %eax	/* LBA of GRLDR */
	incw	%bp

2:
	jmp	cdrom_helper
1:
	popw	%ds
	ret

#endif //(! defined(GRLDR_MBR))
	

	/* Make sure the above code does not occupy the partition table */

	/* offset value here must be less than or equal to 0x1b8 */

#if (defined(GRLDR_MBR)) 
//       . = . - ((. - _start1) / 0x1b9)
        . = _start1 + 0x1fe	/* boot signature */
	.word	0xAA55

#else	

#endif //(defined(GRLDR_MBR))
	. = _start1 + 0x200 


/* if it is in the Master Boot Track, the second sector can be used to backup
 * the previously working MBR, typically, the MS MBR. if the backup copy of
 * the MBR cannot boot(because, e.g., it depends on another sector of code
 * that does not exist for now), then please do not set the ending signature
 * to 0xAA55, that is to say, if the signature is already 0xAA55, you should
 * change it to another value(for example, 0x0000).
 */

        .fill 0x6b,1,0x90
        	
//	. = _start1 + 0x256
        // NTLDR loads GRLDR entrance
        // Have verified that at least in Windows XP SP2 (5.1.2600.2180) and SP3 (5.1.2600.5512)

        movb	$0x80, %dl
	jmp	_start1   

//        . = _start1 + 0x270 

#if (! defined(GRLDR_MBR))

cdrom_helper:

	/* IP and old_DS is on the stack. */

	/* DS=40h */

	/* Stack is high and safe. */

	/* EAX is LBA. if EAX==0, LBA is unknown. */

	/* check if the first sector is the same as the current one */

	/* load the first sector onto the sector immediately follows */
1:
	pushw	%cs
	popw	%bx		/* BX=CS=old_DS=load_segment */
	addw	$0x0080, %bx	/* buffer segment */
	movw	%bx, %es	/* ES changed! */
	call	load_cd_sector

	/* compare the two sectors */
	movw	$0x200, %cx
	xorw	%si, %si
	xorw	%di, %di
	cld
	cs repz cmpsl
	je	load_the_rest	/* 1st sector is ok, continue */
not_grldr:
	testw	%bp, %bp
	jz	2f
	xorw	%bp, %bp
//	xorl	%eax, %eax
	movl	$0x12, %eax
2:
	incl	%eax
	jnz	1b		/* try next */

cd_no_grldr:
        pushw   %cs
	popw	%ds		/* DS=load_segment */

	# Here we use error message and routine in FAT32 boot sector
	# which is also inside the 2048-byte CD sector.

	movw	$(msg_BootError_cd - _start1), %si
1:
	lodsb	(%si), %al	/* get token */
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	cmpb	$0, %al		/* end of string? */
	jne	1b		/* until done */
1:	
	jmp	1b
        

		
load_cd_sector:
	/* input:	EAX	LBA
	 *		BX	buffer segment(buffer offset=0)
	 *		EAX	LBA lo 32 bits
	 * output:
	 *		16 bytes at DS:SI	destroyed
	 */

	pushaw
	pushw	%ds
	movw	$0x1A0, %si	
	pushw	$0x40
	popw	%ds

	/* disk address packet */
	movl	$0x00010010, (%si)	/* load 1 sector each time. */
	movw	$0, 4(%si)		/* buffer offset=0 */
	movw	%bx, 6(%si)		/* buffer segment */
	movl	%eax, 8(%si)		/* LBA lo 32 bits */
	movl	$0, 12(%si)		/* LBA hi 32 bits */
	
	movb	$0x42, %ah
	call	int13
	
	popw	%ds
	popaw
	ret

load_the_rest:

	/* load all sectors (except the first one) */

	/* EAX = first sector(LBA) of GRLDR */

	pushl	%eax

	pushw	%cs
	popw	%bx		/* BX=CS=old_DS=load_segment */
	pushw	%bx
	/* 6144 = 0x1800 = 3 sectors > 4KB, this is for the additional 4KB-preset-menu at the end of grldr */
//	movw	$((grldr_signature - _start1 + 4 + STAGE2_SIZE - 1 + 6144 ) / 2048), %cx	/* sectors to load */
	movw	$((grldr_signature - _start1 + 4 + STAGE2_SIZE - 4) / 0x800 + 16), %cx		//32KB-preset-menu
1:
	incl	%eax		/* next sector */
	addw	$0x0080, %bx	/* buffer segment */	
	call	load_cd_sector
	loop	1b
	/* loading is completed. BX=segment of the last sector. */

//	subw	$0x0181, %bx	/* decw	%bx */
	popw	%bx
	addw	$((grldr_signature - _start1 + 4 + STAGE2_SIZE - 4) >> 4), %bx
	movw	%bx, %ds

	popl	%eax
//	subl	$((grldr_signature - _start1 + 4 + STAGE2_SIZE - 1 + 6144) / 2048), %eax

	/* check the ending signature */
//	cmpl	$0xCE1A02B0, ((grldr_signature - _start1 + 4 + STAGE2_SIZE - 1) % 2048) + 13
	cmpl	$0xCE1A02B0, ((STAGE2_SIZE - 4) & 0x0F)
	jne	not_grldr
	        
grldr_real_start:
	cli
	//xorw	%ax, %ax
	//movw	%ax, %ss
	//movw	$0x0400, %sp	/* tmp use real-mode IDT as stack */
	movw	%cs, %bp	/* save CS to BP */
	call	1f
1:
	popw	%bx		/* BX=Instruction Pointer of 1b */
	subw	$(1b - _start1), %bx
	movw	%bx, %cx
	shrw	$4, %bx
	addw	%bp, %bx
	pushw	%bx		/* new CS */
	andw	$0x000f, %cx
	addw	$(1f - _start1), %cx
	pushw	%cx		/* new IP */
	lret
1:
//	movw	%ds, %cx	/* CX==BP==0x7C0 for pxe enabled */
	pushw	%cs
	popw	%ds

	/* CS=DS=BX, CS:0000 = _start1 */
	
        addw	$((pre_stage2_start - _start1) >> 4), %bx
	
	/* BX:0000 = pre_stage2_start */
	////////////////////////////////////////////////
	//old_stack	1                   0x580
	//old_stack	2                   0x68400                                     For Compatible with  Virtual PC 
	//new_stack                     0x2000
	//Buffer                        0x580 - 0x59f           0x20		
	//int13/AH=42 pack              0x5a0 - 0x5af           0x10
	//GRLDR head										0x5eo - 0x7df						0x200
	//pre_stage2                    0x8200 - 0x681ff        0x60000	
	////////////////////////////////////////////////

//	cmpw	$0x7C0, %bp
//	jne	1f
//	cmpw	%bp, %cx
//	je	2f
//1:
    //orb	$0x01, (pre_stage2_start - _start1 + 5) /* disable pxe */
2:      
	cmpw	$0x820, %bx       
	jb	2f

	movw	$((0x8200 - (pre_stage2_start - _start1) - 0x400) >> 4), %cx		//0x5e0

	movw	%cx, %es
	xorw	%di, %di
	xorw	%si, %si

	movw	$0x200, %cx	/* move 2 sectors */ 
	cld
	repz movsw
	
	pushw	%es		/* ES:0000 = _start */
	pushw	$(1f - _start1)
	lret			/* CS=ES, CS:0000 = _start1 */
1:

	/* move BX:0000 to 0820:0000 upward since BX >= 0x820 */

//	cld

	movw	%bx, %ds			//0x2200 or 0x9c0
	movw	$0x820, %bx
	movw	%bx, %es			//0x820

	xorw	%si, %si
	xorw	%di, %di

	movw	$6, %bx		/* 6*64K pages: 0x22000 - 0x81fff 	6*64K = 384K  */
1:
	movw	$0x8000, %cx
	repz movsw					//(0x22000 - 0x81fff) or (0x9c00 - 0x69bff) ->  (0x8200 - 0x681ff)
	movw	%ds, %ax
	addw	$0x1000, %ax
	movw	%ax, %ds
	movw	%es, %ax
	addw	$0x1000, %ax
	movw	%ax, %es
	decw	%bx
	jnz	1b

	jmp	3f
2:

	/* move BX:0000 to 0820:0000 downward since BX < 0x820 */

	std

	addw	$0x7000, %bx
	movw	%bx, %ds			//0x7b00
	movw	$0x7820, %bx
	movw	%bx, %es			//0x7820

	movw	$0xfffe, %si
	movw	%si, %di

	movw	$6, %bx		/* 6*64K pages: 0x0b000 - 0x6afff   6*64K = 384K   */
1:
	movw	$0x8000, %cx
	repz movsw					//(0x0b000 - 0x6afff) ->  (0x8200 - 0x681ff)
	movw	%ds, %ax
	subw	$0x1000, %ax
	movw	%ax, %ds
	movw	%es, %ax
	subw	$0x1000, %ax
	movw	%ax, %es
	decw	%bx
	jnz	1b

	cld

3:

#if 0
	/* put the config file name */
	xorw	%ax, %ax
	movw	%ax, %es
	movw	%ax, %ds

	xorl	%ebp, %ebp
	
	movb	%dh, 0x820A	/* this is the boot partition number */

	#; clear saved_entryno so that force_cdrom_as_boot_device be cleared
	#; later in common.c

	movl	%ebp, 0x820C	/* EBP=0, clear saved_entryno */

	movw    $0x0010, %cx	/* set max length of grub version string */
	movw    $0x8212, %di	/* version string */ 
	cld
	/* AL is already 0. Locate the end of version string */
	repnz scasb	/* find the location of the default config file name */

	jcxz	1f	/* failed, will not use the default config file name */

	movw    $0x4e, %cx	/* max length of config file name */

	movw	%cs, %si	/* CS:0000 = _start1 */
	shlw	$4, %si		/* 0000:SI = _start1 */

	addw	$(default_config_file - _start1), %si

	//movw	$(default_config_file + 0x8200 - pre_stage2_start), %si
	cld
	repz movsb	/* move file name to the config-file field of stage2 */ 
1:
        cli
	xorw	%ax, %ax
	movw	%ax, %ss
	movw	$0x2000, %sp
	sti
	
	movw	$0x0003, %ax	/* set display mode: 80*25 color text */
	int	$0x10
	
	ljmp	$0, $0x8200


default_config_file:
	.ascii	"/menu.lst"

	.byte	0		/* mark the end of ascii zero string */	
#else
	xorl	%eax, %eax
	movw	%ax, %es
	movw	%ax, %ds
	//movw	%ax, %ss
	//movw	$0x2000, %sp

	#; clear saved_entryno so that force_cdrom_as_boot_device be cleared
	#; later in common.c

	movl	%eax, 0x820C	/* EAX=0, clear saved_entryno */

	ljmp	$0, $0x8200
#endif
	
	. = _start1 + 0x3f0 	
	
msg_BootError_cd:
	.ascii	"No GRLDR\0"
	
        . = _start1 + 0x3fe	/* boot signature */
	.word	0xAA55	
        
#endif // (! defined(GRLDR_MBR))        

	. = _start1 + 0x400 		

#endif // (! defined(GRLDR_INSTALL))	 

#define ALTERNATIVE_KERNEL

#if     defined(GRLDR_INSTALL)

        .code16
        
/* ********************  fat32_dbr start  ****************************** */ 
/*
 * The following is based on FreeDOS, modified heavily by Tinybit in Feb, 2004
 *
 * Merges LBA and CHS boot sectors to ONE FAT32 boot sector!
 *
 * Memory layout for GRLDR FAT32 single stage boot process:
 *
 *	...
 *	|-------| 1FE0:7E00
 *	|BOOTSEC| (GRUB does not use this relocation area)
 *	|RELOC.	| (overwritten by kernel loaded)
 *	|-------| 1FE0:7C00
 *	...
 *	|-------|
 *	|KERNEL	| (overwrites bootsec reloc.)
 *	|LOADED	| (holds 1 sector directory buffer before kernel load)
 *	|-------| 2000:0000
 *	...
 *	|-------| 0000:7E00
 *	|BOOTSEC| GRUB always run inside this sector,
 *	|ORIGIN | no relocation.
 *	|-------| 0000:7C00
 *	...
 *	|-------| 0060:0200
 *	|  FAT  | (only 1 sector buffered)
 *	|-------| 0060:0000
 *	...
 *
 */

/*
; This is an LBA-enabled FreeDOS FAT32 boot sector (single sector!).
; You can use and copy source code and binaries under the terms of the
; GNU Public License (GPL), version 2 or newer. See www.gnu.org for more.

; Based on earlier work by FreeDOS kernel hackers, modified heavily by
; Eric Auer and Jon Gentle in 7 / 2003.
;
; Features: Uses LBA and calculates all variables from BPB/EBPB data,
; thus making partition move / resize / image-restore easier. FreeDOS
; can boot from FAT32 partitions which start > 8 GB boundary with this
; boot sector. Disk geometry knowledge is not needed for booting.
;
; Windows uses 2-3 sectors for booting (sector stage, statistics sector,
; filesystem stage). Only using 1 sector for FreeDOS makes multi-booting
; of FreeDOS and Windows on the same filesystem easier.
;
; Requirements: LBA BIOS and 386 or better CPU. Use the older CHS-only
; boot sector if you want FAT32 on really old PCs (problems: you cannot
; boot from > 8 GB boundary, cannot move / resize / ... without applying
; SYS again if you use the CHS-only FAT32 boot sector).
;
; FAT12 / FAT16 hints: Use the older CHS-only boot sector unless you
; have to boot from > 8 GB. The LBA-and-CHS FAT12 / FAT16 boot sector
; needs applying SYS again after move / resize / ... a variant of that
; boot sector without CHS support but with better move / resize / ...
; support would be good for use on LBA harddisks.


; Memory layout for the FreeDOS FAT32 single stage boot process:

;	...
;	|-------| 1FE0:7E00
;	|BOOTSEC|
;	|RELOC.	|
;	|-------| 1FE0:7C00
;	...
;	|-------| 2000:0200
;	|  FAT  | (only 1 sector buffered)
;	|-------| 2000:0000
;	...
;	|-------| 0000:7E00
;	|BOOTSEC| overwritten by the kernel, so the
;	|ORIGIN | bootsector relocates itself up...
;	|-------| 0000:7C00
;	...
;	|-------|
;	|KERNEL	| maximum size 134k (overwrites bootsec origin)
;	|LOADED	| (holds 1 sector directory buffer before kernel load)
;	|-------| 0060:0000
;	...
*/


#define LOADSEG		0x2000
#define FATSEG		0x0060

Entry_32:
	jmp	1f

	. = Entry_32 + 0x02

	.byte	0x90	/* for CHS. Another possible value is 0x0e for LBA */

	. = Entry_32 + 0x52

	.ascii	"FAT32   "	/* filesystem ID, 8 bytes. */

/*
;	bp is initialized to 7c00h
; %define bsOemName	bp+0x03	; OEM label (8)
%define bsBytesPerSec	bp+0x0b ; bytes/sector (dw)
%define bsSecPerClust	bp+0x0d	; sectors/allocation unit (db)
%define bsResSectors	bp+0x0e	; # reserved sectors (dw)
%define bsFATs		bp+0x10	; # of fats (db)
; %define bsRootDirEnts	bp+0x11	; # of root dir entries (dw, 0 for FAT32)
			; (FAT32 has root dir in a cluster chain)
; %define bsSectors	bp+0x13	; # sectors total in image (dw, 0 for FAT32)
			; (if 0 use nSectorHuge even if FAT16)
; %define bsMedia	bp+0x15	; media descriptor: fd=2side9sec, etc... (db)
; %define sectPerFat	bp+0x16	; # sectors in a fat (dw, 0 for FAT32)
			; (FAT32 always uses xsectPerFat)
%define sectPerTrack	bp+0x18	; # sectors/track
; %define nHeads	bp+0x1a	; # heads (dw)
%define nHidden		bp+0x1c	; # hidden sectors (dd)
; %define nSectorHuge	bp+0x20	; # sectors if > 65536 (dd)
%define xsectPerFat	bp+0x24	; Sectors/Fat (dd)
			; +0x28 dw flags (for fat mirroring)
			; +0x2a dw filesystem version (usually 0)
%define xrootClst	bp+0x2c	; Starting cluster of root directory (dd)
			; +0x30 dw -1 or sector number of fs.-info sector
			; +0x32 dw -1 or sector number of boot sector backup
			; (+0x34 .. +0x3f reserved)
%define drive		bp+0x40	; Drive number
			bp+0x41	; partition number for GRLDR

%define fat_sector	bp+0x44		; last accessed FAT sector (dd)
					; (overwriting unused bytes)
%define fat_start	bp+0x48		; first FAT sector (dd)
					; (overwriting unused bytes)
%define data_start	bp+0x4c		; first data sector (dd)
					; (overwriting unused bytes)

*/

	. = Entry_32 + 0x5a
1:
	cli
	cld
//	testb	$0x80, %dl		2014-01-26
//	jne	1f
//	movb	$0xff, %dh
//	jmp 2f
//1:
//	cmpw	$0x7c00, %sp
//	jne	2f
//	subw	$0x7be, %bp
//	xchgw	%bp, %ax
//	shrb	$4, %al
//	movb	%al, %dh
//2:
	xorw	%ax, %ax
	movw	$0x7c00, %bp
	movw	%bp, %sp        
	movw	%ax, %ss	/* stack and BP-relative moves up, too */
	sti			/* after stack setup, we can use stack */
	
	/* cs=ss=ds=es=0, bp=0x7C00 */
	pushw	%ax
	movw	%dx, 0x40(%bp)  /* BIOS passes drive number in DL */
	movb	$0x41, %ah
	movw	$0x55AA, %bx
	int	$0x13
	popw	%ds
	jc	1f		/* No EBIOS */
	cmpw	$0xAA55, %bx
	jne	1f		/* No EBIOS */
	rorb	$1, %cl
	jnc	1f              /* No EBIOS */
	/* EBIOS supported */
	movb	$0x42, (ebios_32 - 1 - Entry_32 + 0x7c00)
1:
//	movb	(Loadusb_32 - Entry_32 + 0x7c00 +1), %al
//	decb %al				//al=1,When dl = 00 load
//	jne 1f						
//	cmpb	%al, %dl	//dl=0?
//	je	3f
//1:
//	decb %al				//al=2,Forced load 
//	jne 2f							
//3:	//Load the usb driver
//	pushw $0x0d00
//	popw %es				//read to 0d00:0000
//	movl	0x7c1c, %eax
// 	incl	%eax
//	incl	%eax
//	movw	$0x0a, %cx 
//1:	
// 	call	readDisk_32
// 	loop	1b
//	call	$0x0d00,$0x0000
//	jc 2f
//	movb	%dl, 0x40(%bp)  /* BIOS passes drive number in DL */
//	movb	$0x42, (ebios_32 - 1 - Entry_32 + 0x7c00)
//2:

/* figure out where FAT and DATA area starts
 * (modifies EAX EDX, sets fat_start and data_start variables)
 */
  xorw %bx, %bx
	xorl	%eax, %eax
	movl	%eax, 0x44(%bp)	/* init buffer status */

	/* first, find fat_start */
	movw	0x0e(%bp), %ax	/* reserved sectors */
	addl	0x1c(%bp), %eax	/* hidden sectors */
	movl	%eax, 0x48(%bp)	/* first FAT sector */
	movl	%eax, 0x4c(%bp)	/* first data sector, initial value */

	/* next, find data_start */
	movl	0x10(%bp), %eax	/* number of fats, no movzbl needed: the */
				/* 2 words at 0x11(%bp) are 0 for fat32. */
	mull	0x24(%bp)	/* sectors per fat (EDX=0) */
	addl	%eax, 0x4c(%bp)	/* first DATA sector */

/* Searches for the file in the root directory.
 * Returns:	EAX = first cluster of file
 */

	movl	0x2c(%bp), %eax	/* root dir cluster */

1:
	pushl	%eax		/* save cluster */
	call	cluster_to_lba_32
		/* EDX is sectors per cluster, EAX is sector number */
	movw	$(msg_BootError_32 - Entry_32 + 0x7c00), %si
	jc	boot_error_32	/* EOC encountered */

2:
//	lesw	(loadseg_off_32 - Entry_32)(%bp), %bx	/* load to loadseg:0 */
	pushw	$LOADSEG
	popw	%es
	pushw	%es
	call	readDisk_32
	popw	%es
	xorw	%di, %di

	/* Search for kernel file name, and find start cluster */
3:
	movw	$11, %cx
	movw	$(filename_32 - Entry_32 + 0x7c00), %si
	repz cmpsb
	jz	1f	/* note that di now is at dirent+11 */

	addw	$0x20, %di
	andw	$-0x20, %di	/* 0xffe0 */
	cmp	0x0b(%bp), %di	/* bytes per sector */
	jnz	3b		/* next directory entry */

	decw	%dx	/* initially DX holds sectors per cluster */
	jnz	2b	/* loop over sectors in cluster */

	popl	%eax		/* restore current cluster */
	call	next_cluster_32
	jmp	1b		/* read next cluster */

1:
	/* kernel directory entry is found */
	pushw	%es:(0x14-11)(%di)	/* get cluster number HI */
	pushw	%es:(0x1a-11)(%di)	/* get cluster number LO */
	popl	%eax			/* convert to 32bit */


/* read kernel */

2:
	pushl	%eax
	call	cluster_to_lba_32
		/* EDX is sectors per cluster, EAX is sector number */
	jnc	1f
	
	/* EOC encountered - done */

	movw	0x40(%bp), %dx	/* boot_drive and boot_partition */
	pushw	%dx		/* for loading grub.exe */
//	ljmp	*(loadseg_off_32 - Entry_32)(%bp)
	ljmp	$LOADSEG,$0x0000

1:
	call	readDisk_32
	decw	%dx	/* initially DX holds sectors per cluster */
	jnz	1b	/* loop over sectors in cluster */

	popl	%eax
	call	next_cluster_32
	jmp	2b
		
/* given a cluster number, find the number of the next cluster in
 * the FAT chain. Needs fat_start.
 * input:	EAX - cluster
 *		EDX = 0
 * output:	EAX - next cluster
 *		EDX = undefined
 */

next_cluster_32:
	pushw	%es		
	shll	$2, %eax	/* 32bit FAT */
	movzwl	0x0b(%bp), %ebx	/* bytes per sector */
	divl	%ebx		/* residue is in EDX */
	addl	0x48(%bp), %eax	/* add the first FAT sector number.  */ 
				/* EAX=absolute sector number */
	movw	$FATSEG, %bx
	movw	%bx, %es
	xorw	%bx, %bx

	/* is it the last accessed and already buffered FAT sector? */
	cmpl	0x44(%bp), %eax
	jz	1f
	movl	%eax, 0x44(%bp)	/* mark sector EAX as buffered */
	pushw	%es
	call	readDisk_32	/* read sector EAX to buffer */
	popw	%es
1:
        andb	$0x0f, %es:3(%edx)	/* mask out top 4 bits */
        movl	%es:(%edx), %eax	/* read next cluster number */
	popw	%es
	ret

/* Convert cluster number to the absolute sector number
 * ... or return carry if EndOfChain! Needs data_start.
 * input:	EAX - target cluster
 * output:	EAX - absolute sector
 *		EDX - [bsSectPerClust] (byte)
 *		carry clear
 *		(if carry set, EAX/EDX unchanged, end of chain)
 */

cluster_to_lba_32:
	cmpl	$0x0ffffff8, %eax	/* check End Of Chain */
	cmc
	jb	1f			/* carry is stored if EOC */

	/* sector = (cluster-2) * clustersize + data_start */
	decl	%eax
	decl	%eax

	movzbl	0x0d(%bp), %edx		/* sectors per cluster */
	pushw	%dx			/* only DX would change */
	mull	%edx			/* EDX = 0 */
	popw	%dx
	addl	0x4c(%bp), %eax		/* data_start */
	/* here, carry is cleared (unless parameters are wrong) */
1:
	ret

/* Read a sector from disk, using LBA or CHS
 * input:	EAX - 32-bit DOS sector number
 *		ES:0000 - destination buffer
 *		(will be filled with 1 sector of data)
 * output:	ES:0000 points one byte after the last byte read.
 *		EAX - next sector
 */
 
readDisk_32:
	pushal
	xorl	%edx, %edx	/* EDX:EAX = LBA */
	pushl	%edx		/* hi 32bit of sector number */
	pushl	%eax		/* lo 32bit of sector number */
	pushw	%es		/* buffer segment */
	pushw	%dx		/* buffer offset */
	pushw	$1		/* 1 sector to read */
	pushw	$16		/* size of this parameter block */

	xorl	%ecx, %ecx
	pushl	0x18(%bp)	/* lo:sectors per track, hi:number of heads */
	popw	%cx		/* ECX = sectors per track */
	divl	%ecx		/* residue is in EDX */
				/* quotient is in EAX */
	incw	%dx		/* sector number in DL */
	popw	%cx		/* ECX = number of heads */
	pushw	%dx		/* push sector number into stack */
	xorw	%dx, %dx	/* EDX:EAX = cylinder * TotalHeads + head */
	divl	%ecx		/* residue is in EDX, head number */
				/* quotient is in EAX, cylinder number */
	xchgb	%dl, %dh	/* head number should be in DH */
				/* DL = 0 */
	popw	%cx		/* pop sector number from stack */
	xchgb	%al, %ch	/* lo 8bit cylinder should be in CH */
				/* AL = 0 */
	shlb	$6, %ah		/* hi 2bit cylinder ... */
	orb	%ah, %cl	/* ... should be in CL */
	
	movw	$0x201, %ax	/* read 1 sector */
ebios_32: 

	movw	%sp, %si	/* DS:SI points to disk address packet */
	movb	0x40(%bp), %dl	/* hard disk drive number */
	pushw	%es
	pushw	%ds
	int	$0x13
	popw	%ds
	popw	%bx
	jc	disk_error_32
	leaw	0x20(%bx), %bx
	movw	%bx, %es
	popaw			/* remove parameter block from stack */
	popal
	incl 	%eax		/* next sector */
	ret



disk_error_32:

	movw	$(msg_DiskReadError_32 - Entry_32 + 0x7c00), %si

boot_error_32:

/* prints string DS:SI (modifies AX BX SI) */

//print_32:
1:
	lodsb	(%si), %al	/* get token */
	//xorw	%bx, %bx	/* video page 0 */
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	cmpb	$0, %al		/* end of string? */
	jne	1b		/* until done */
1:	
	jmp	1b

        . = Entry_32 + 0x1e0

msg_BootError_32:
	.ascii	"No "
	
filename_32:
	.ascii	"GRLDR      \0"

msg_DiskReadError_32:
	.ascii	"Disk error\0" 

	. = Entry_32 + 0x1fc
	.word	0, 0xAA55 /* Win9x uses all 4 bytes as magic value here */

	. = Entry_32 + 0x200  
	
/* ********************  fat32_dbr end  ****************************** */

/* ********************  fat12_16_dbr start  ****************************** */	
	
	.arch	i486, nojumps

/*
 * The following is based on FreeDOS, modified heavily by Tinybit in Feb, 2004
 *
 * Merges FAT12 and FAT16 boot sectors to ONE FAT boot sector!
 *
 * Memory layout for GRLDR FAT single stage boot process:
 *
 *	+--------+
 *	|        |
 *	|GRLDR   | also used as max 128k FAT buffer
 *	|LOADED  | before GRLDR loading starts
 *	|--------| 2000:0000
 *	|        |
 *	|--------| 0000:7E00
 *	|BOOTSECT|
 *	|ORIGIN  |
 *	|--------| 0000:7C00
 *	|        |
 *	|--------| 0000:3000
 *	|CLUSTER |
 *	|LIST    |
 *	|--------| 0000:2000
 *	|        |
 *	+--------+
 */

/*
;
; File:
;                            boot.asm
; Description:
;                           DOS-C boot
;
;                       Copyright (c) 1997;
;                           Svante Frey
;                       All Rights Reserved
;
; This file is part of DOS-C.
;
; DOS-C is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version
; 2, or (at your option) any later version.
;
; DOS-C is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
; the GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public
; License along with DOS-C; see the file COPYING.  If not,
; write to the Free Software Foundation, 675 Mass Ave,
; Cambridge, MA 02139, USA.
;
;
;	+--------+ 1FE0:7E00
;	|BOOT SEC|
;	|RELOCATE|
;	|--------| 1FE0:7C00
;	|        |
;	|--------| 1FE0:3000
;	| CLUSTER|
;	|  LIST  |
;	|--------| 1FE0:2000
;	|        |
;	|--------| 0000:7E00
;	|BOOT SEC| overwritten by max 128k FAT buffer
;	|ORIGIN  | and later by max 134k loaded kernel
;	|--------| 0000:7C00
;	|        |
;	|--------|
;	|KERNEL  | also used as max 128k FAT buffer
;	|LOADED  | before kernel loading starts
;	|--------| 0060:0000
;	|        |
;	+--------+
*/


#define LOADSEG_12_16   0x2000
#define FATBUF          0x2000        /* offset of temp buffer for FAT chain */

Entry_12_16:
	jmp     1f

	. = Entry_12_16 + 0x02
	.byte	0x90

 	. = Entry_12_16 + 0x36

	.ascii	"FAT12/16"	/* filesystem ID */

/*
;       bp is initialized to 7c00h
%define bsOemName       bp+0x03      ; OEM label
%define bsBytesPerSec   bp+0x0b      ; bytes/sector
%define bsSecPerClust   bp+0x0d      ; sectors/allocation unit
%define bsResSectors    bp+0x0e      ; # reserved sectors
%define bsFATs          bp+0x10      ; # of fats
%define bsRootDirEnts   bp+0x11      ; # of root dir entries
%define bsSectors       bp+0x13      ; # sectors total in image
%define bsMedia         bp+0x15      ; media descrip: fd=2side9sec, etc...
%define sectPerFat      bp+0x16      ; # sectors in a fat
%define sectPerTrack    bp+0x18      ; # sectors/track
%define nHeads          bp+0x1a      ; # heads
%define nHidden         bp+0x1c      ; # hidden sectors
%define nSectorHuge     bp+0x20      ; # sectors if > 65536
%define drive           bp+0x24      ; drive number
			bp+0x25      ; partition number for GRLDR
%define extBoot         bp+0x26      ; extended boot signature
%define volid           bp+0x27
%define vollabel        bp+0x2b
%define filesys         bp+0x36

%define RootDirSecs     bp+0x26         ; # of sectors root dir uses
					; (overwriting unused bytes)
%define fat_start       bp+0x28         ; first FAT sector
					; (overwriting unused bytes)
%define root_dir_start  bp+0x2c         ; first root directory sector
					; (overwriting unused bytes)
%define data_start      bp+0x30         ; first data sector
					; (overwriting unused bytes)
%define data_clusters   bp+0x34         ; # of clusters in data area
					; (overwriting unused bytes)
			bp+0x36		; bytes per FAT( > 0x1800 means FAT16)
					; (overwriting unused bytes)
*/

	. = Entry_12_16 + 0x3e
1:
	cli
	cld
//	testb	$0x80, %dl    2014-01-26
//	jne	1f
//	movb	$0xff, %dh
//	jmp 2f
//1:
//	cmpw	$0x7c00, %sp
//	jne	2f
//	subw	$0x7be, %bp
//	xchgw	%bp, %ax
//	shrb	$4, %al
//	movb	%al, %dh
//2:
	xorw	%ax, %ax
	movw	$0x7c00, %bp
	movw	%bp, %sp        
	movw	%ax, %ss	/* stack and BP-relative moves up, too */
	movw	%ax, %ds
	movw	%ax, %es
	sti			/* after stack setup, we can use stack */ 

	/* cs=ss=ds=es=0, bp=0x7C00 */
 
	movw	%dx, 0x24(%bp)  /* BIOS passes drive number in DL */
	movb	$0x41, %ah
	movw	$0x55AA, %bx
	pushw	%ds
	pushw	%es
	int	$0x13
	popw	%es
	popw	%ds
	jc	1f		/* No EBIOS */
	cmpw	$0xAA55, %bx
	jne	1f		/* No EBIOS */
	rorb	$1, %cl
	jnc	1f              /* No EBIOS */
	/* EBIOS supported */
	movb	$0x42, (ebios_12_16 - 1 - Entry_12_16 + 0x7c00)
1:
	/* GET DRIVE PARMS: Calculate start of some disk areas */
	movzwl	0x0e(%bp), %ebx	/* reserved sectors */
	addl	0x1c(%bp), %ebx	/* hidden sectors */
	movl	%ebx, 0x28(%bp)	/* ---- FAT start */

	movzbl	0x10(%bp), %eax	/* number of FATs */
	movzwl	0x16(%bp), %ecx	/* sectors per FAT */
	mull	%ecx		/* EDX=0, EAX=total sectors for FAT */
	addl	%eax, %ebx
	movl	%ebx, 0x2c(%bp)	/* ---- RootDir start */

	/* Calculate how many sectors the root directory occupies */

	movw	0x11(%bp), %ax	/* max number of root dir entries */
	addw	$0x0f, %ax
	movw	$0x10, %cx
	divw	%cx		/* AX = sectors per root directory */
	cwde
	movw	%ax, %cx       
	addl	%eax, %ebx
	movl	%ebx, 0x30(%bp)	/* ---- DataArea start */

	/* First, read the whole root directory into the temporary buffer */

	movl	0x2c(%bp), %eax	/* root directory starting sector(lo) */
	lesw	(loadseg_off_12_16 - Entry_12_16)(%bp), %bx     /* ES:BX = 0x2000:0000 */
	
	/* cs=ss=ds=0 es=0x2000 */
	
	call	readDisk_12_16	/* CX=0, eax,ES changed */ 
	
	lesw	(loadseg_off_12_16 - Entry_12_16)(%bp), %di     /* ES:DI = 0x2000:0000 */


	/* Search for kernel file name, and find start cluster */

	/* BX=0, CX=0 */

1:
	movw	$(filename_12_16 - Entry_12_16 + 0x7c00), %si	/* filename */

	movb	$11, %cl	/* length of kernel filename */

	pushw	%di
	repz cmpsb
	popw	%di

	jz	1f

	addw	$0x20, %di	/* next entry */
	jz	2f		/* or jc 2f, exceeding 64K */

	/* if the entry begins in 0, this also ends the dir. */

	cmpb	%ch, %es:(%di)	/* CH=0 */
	jnz	1b
2:
	movw	$(msg_BootError_12_16 - Entry_12_16 + 0x7c00), %si
	jmp	boot_error_12_16	/* fail if not found */

1:

/****************************************************************************/

	/* BX=0, CX=0 */

	######################################################################
	# Reads the FAT chain and stores it in a temporary buffer in the
	# first 64KB.  The FAT chain is stored an array of 16-bit cluster
	# numbers, ending with 0.
	#
	# The file must fit in conventional memory, so it can't be larger
	# than 640KB. The sector size must be at least 512 bytes, so the
	# FAT chain can't be larger than around 3KB.
	######################################################################

	/********************************************************************/
	/* First, load the complete FAT into memory. The FAT can't be       */
	/* larger than 128KB, so it should fit in the temporary buffer.'     */
	/********************************************************************/

	pushw	%es:0x1a(%di)	/* save first cluster number of file */
	movw	0x16(%bp), %cx	/* sectors per FAT */
	movl	0x28(%bp), %eax	/* FAT start sector(lo) */

	pushw	%es		/* ES=0x2000 */
	call	readDisk_12_16	/* CX=0, eax,ES changed */	
	popw	%ds		/* DS=0x2000 */
	
	/* cs=ss=0 ds=0x2000 es=x */

	popw	%ax		/* restore first cluster number of file */

	/********************************************************************/
	/* Then, extract the clusters of the file from the FAT              */
	/********************************************************************/

	/* Set ES:DI to the temporary storage for the FAT chain */

	pushw	%ds		/* ES=0x2000 */

	pushw	%ss
	popw	%es
	
	/* cs=es=ss=0 ds=0x2000 */
	
	movw	$FATBUF, %di    /* cs=ss=es=0000 ds=di=0x2000 */

	/* BX=0, CX=0 */
2:
	stosw			/* store cluster number */
	movw	%ax, %si	/* SI = cluster number */

	//////////////////////////////////////////////////////////////
	//
	// FAT16 can occupy 128KB, so the segment must be adjusted
	//
	//////////////////////////////////////////////////////////////

	popw	%dx		/* DX=0x2000, segment for FAT16 */
	pushw	%dx
	addw	%si, %si	/* multiply cluster number by two */
	jnc	1f
	addb	$0x10, %dh	/* overflow. Add 0x1000 to segment value */
1:
	cmpw	$0x3231, 0x39(%bp)
	jne     3f
	/******** FAT12 ********/

	addw	%ax, %si	/* multiply cluster number by 3 ... */
	shrw	$1, %si		/* ... and divide by 2 */
	lodsw

	/* If the cluster number was even, the cluster value is now in
	 * bits 0-11 of AX. If the cluster number was odd, the cluster
	 * value is in bits 4-15, and must be shifted right 4 bits. If
	 * the number was odd, CF was set in the last shift instruction.
	 */

	jnc	1f
	shrw	$4, %ax
1:
	andb	$0x0f, %ah	/* mask off the highest 4 bits */
	cmpw	$0x0ff7, %ax	/* check for EOF */
	jmp	4f

3:
	/******** FAT16 ********/

	movw	%dx, %ds	/* DS:SI points to next cluster */
	lodsw			/* AX = next cluster */

	cmpw	$0xfff7, %ax	/* check for EOF */
4:
	jbe	2b		/* continue if not EOF */

	/* Mark end of FAT chain with 0, so we have a single
	 * EOF marker for both FAT12 and FAT16 systems.
	 */

	xorw	%ax, %ax
	stosw

/****************************************************************************/

	/* Load the file into memory, one cluster at a time */
	/* cs=es=ss=0 ds=0x2000 */

	popw	%es		/* ES=0x2000 */
	pushw	%ss
	popw	%ds		/* DS=SS */
	
	/* cs=ds=ss=0 es=0x2000 */
	
	movw	$FATBUF, %si	/* set DS:SI to the FAT chain */

1:
	/* CH=0 */

	lodsw			/* AX = next cluster to read */
	subw	$2, %ax		/* cluster numbers start with 2 */
	jc	1f		/* the cluster should be 0 for EOC */

	movzwl  %ax, %eax
	movb	0x0d(%bp), %cl	/* CH=0, CX=sectors per cluster */	
	mull	%ecx
	addl	0x30(%bp), %eax	/* data starting sector */
	
	call	readDisk_12_16	/* CX=0, eax,ES changed */
	jmp	1b		/* read next cluster */

1:
	/* EOC encountered - done */

	movw	0x24(%bp), %dx	/* boot_drive and boot_partition */
	pushw	%dx		/* for loading grub.exe */
	ljmp	*(loadseg_off_12_16 - Entry_12_16)(%bp)	/* boot it! */

/****************************************************************************/

/* Read a number of sectors into memory.
 *
 * Call with:	DS=SS=0
 *		eax = 32-bit DOS sector number
 *	        CX = number of sectors to read
 * 		ES:BX = destination buffer
 *
 * Returns:	CX=0
 *		ES increased, BX untouched
 *		ES:BX points one byte after the last byte read.
 * 		eax = next sector number after read
 *		All other registers preserved
 */
 
readDisk_12_16:
2:
	pushal            
	xorl	%edx, %edx
	pushl	%edx		/* hi 32bit of sector number */  //666A00 pushd 00
	pushl	%eax		/* lo 32bit of sector number */
	pushw	%es		/* buffer segment */
	pushw	%bx		/* buffer offset */
	pushw	$1		/* 1 sector to read */
	pushw	$16		/* size of this parameter block */
	cmpb	$0x42, (ebios_12_16 - 1 - Entry_12_16 + 0x7c00)
	je      1f
        
	xorl	%ecx, %ecx
	pushl	0x18(%bp)	/* lo:sectors per track, hi:number of heads */
	popw	%cx		/* ECX = sectors per track */
	divl	%ecx		/* residue is in EDX */
				/* quotient is in EAX */
	incw	%dx		/* sector number in DL */
	popw	%cx		/* ECX = number of heads */
	pushw	%dx		/* push sector number into stack */
	xorw	%dx, %dx	/* EDX:EAX = cylinder * TotalHeads + head */
	divl	%ecx		/* residue is in EDX, head number */
				/* quotient is in EAX, cylinder number */
	xchgb	%dl, %dh	/* head number should be in DH */
				/* DL = 0 */
	popw	%cx		/* pop sector number from stack */
	xchgb	%al, %ch	/* lo 8bit cylinder should be in CH */
				/* AL = 0 */
	shlb	$6, %ah		/* hi 2bit cylinder ... */
	orb	%ah, %cl	/* ... should be in CL */
1:	
	movw	$0x201, %ax	/* read 1 sector */
ebios_12_16: 

	movw	%sp, %si	/* DS:SI points to disk address packet */
	movb	0x24(%bp), %dl	/* hard disk drive number */
	pushw	%ds
	pushw	%es
	int	$0x13
	popw	%bx
	popw	%ds
	jc	disk_error_12_16	/* disk read error, jc 1f if caller handles */	
	leaw	0x20(%bx), %bx 	
	movw	%bx, %es	
	popaw			/* remove parameter block from stack */
	popal
	incl 	%eax		/* next sector */
	loop	2b

	ret


loadseg_off_12_16:	.word	0
loadseg_seg_12_16:	.word	LOADSEG_12_16


disk_error_12_16:

	movw	$(msg_DiskReadError_12_16 - Entry_12_16 + 0x7c00), %si

boot_error_12_16:

/* prints string DS:SI (modifies AX BX SI) */

//print_12_16:
1:
	lodsb	(%si), %al	/* get token */
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	cmpb	$0, %al		/* end of string? */
	jne	1b		/* until done */
1:	
	jmp	1b

        . = Entry_12_16 + 0x1e0

msg_BootError_12_16:
	.ascii	"No "

filename_12_16:
	.ascii	"GRLDR      \0"
	
msg_DiskReadError_12_16:
	.ascii	"Disk error\0"

	. = Entry_12_16 + 0x1fc
	.word	0, 0xAA55 /* Win9x uses all 4 bytes as magic value here */

	. = Entry_12_16 + 0x200 


/* ********************  fat12_16_dbr end  ****************************** */
	
/* ********************  ext2_3_4_dbr start  ****************************** */	

//	. = _start1 + 0x800




//	.arch	i486, nojumps

/*
 #; Ext2 boot sector for GRLDR
 */


#define	DEBUG	call	debug_print
#undef DEBUG

	//. = _start1 + 0x800
	
//Offset	Length	Description
//======	======	==============================================================
//00h			2				Machine code for short jump over the data.
//02h			1				0x90
//03h			8				Partitioning message "EXT2/3/4"
//0Bh			2				Bytes per sector. Must be 512.
//0Dh			1				Sectors per block. Valid values are 2, 4, 8, 16 and 32.
//0Eh			2				Bytes per block. Valid values are 0x400, 0x800, 0x1000, 0x2000 and 0x4000.
//10h			4				Pointers in pointers-per-block blocks, that is, number of blocks covered by a double-indirect block.
//								Valid values are 0x10000, 0x40000, 0x100000, 0x400000 and	0x1000000.
//14h			4				Pointers per block, that is, number of blocks covered by an	indirect block.
//								Valid values are 0x100, 0x200, 0x400, 0x800, 0x1000.
//18h			2				Sectors per track.
//1Ah			2				Number of heads/sides.
//1Ch			4				Number of hidden sectors (those preceding the boot sector).
//								Also referred to as the starting sector of the partition.	For floppies, it should be 0.
//20h			4				Total number of sectors in the filesystem(or in the partition).
//24h			1				BIOS drive number of the boot device.
//								Actually this byte is ignored for read. The boot code will write DL onto this byte. 
//								The BIOS or the caller should set	drive number in DL.
//								We assume all BIOSes pass correct drive number in DL.	Buggy BIOSes are not supported!!
//25h			1				Partition number of this partition on the boot drive.
//								0, 1, 2, 3 are primary partitions.
//								4, 5, 6, ... are logical partitions in the extended partition.
//								0xff is for whole drive. So for floppies, it should be 0xff.
//26h			2				inode size in bytes. (Notice! We use the formerly reserved word here for inode size!)
//28h			4				Number of inodes per group.	Normally a 1.44M floppy has only one group, and the total number of inodes is 184. 
//								So the value should be	184 or greater.
//2Ch			4				The block number for group descriptors.	Valid values are 2 for 1024-byte blocks, and 1 otherwise.
//								The value here is equal to (s_first_data_block + 1).	
	

Entry_ext2:

	jmp     1f

	. = Entry_ext2 + 0x02

	/* The default mode is CHS. This is for maximum compatiblity with
	 * small-sized disks, e.g., floppies.
	 *
	 * Valid values are 0x02 for CHS mode, or 0x42 for LBA mode.
	 *
	 * If the BIOS int13 supports LBA, this byte can be safely set to 0x42.
	 *
	 * Some USB BIOSes might have bugs when using CHS mode, so the format
	 * program should set this byte to 0x42. It seems that (generally) all
	 * USB BIOSes have LBA support.
	 *
	 * If the format program does not know whether the BIOS has LBA
	 * support, it may operate this way:
	 *
	 * if (partition_start + total_sectors_in_partition) exceeds the CHS
	 * addressing ability(especially when it is greater than 1024*256*63),
	 * the caller should set this byte to 0x42, otherwise, set to 0x02.
	 */

        .byte	0x90

	. = Entry_ext2 + 0x03

	.ascii	"EXT2/3/4"

	. = Entry_ext2 + 0x30
1:
	cld			/* 0xFC */

	xorw	%ax, %ax	/* 0x31, 0xC0; CF=0, ZF=1 */
	movw	%ax, %ss	/* constant SS=0 */
	movw	%ax, %ds
	movw	%ax, %fs
	
	movw	$0x7c00, %sp

	movw	%sp, %bp	/* constant BP=0x7c00 */

	pushw	%ax		/* 0x0000 at 0000:7bfe */
	movw	$0x1000, %bx
	pushw	%bx		/* 0x1000 at 0000:7bfc */
	pushw	%ax		/* 0x0000 at 0000:7bfa */
				/* SP=0x7bfa */

	/* the 6 bytes in the stack are used by read_block():
	 *	0000	----	-2(%bp)
	 *	1000	----	-4(%bp)
	 *	0000	----	-6(%bp)
	 * Don't touch them!'
	 */

	movb	%dl, 0x24(%bp)	/* BIOS passes drive number in DL */
	movw	$0x200, 0xb(%bp)
	
	movb	$0x41, %ah
	movw	$0x55AA, %bx
	pushw	%ds
	int	$0x13
	popw	%ds
	jc	1f		# No EBIOS
	rorb	%cl	        # ror cl: D0 C9
	jnc	1f		# No EBIOS
	movb	$0x42, (ebios_ext2 - 1 - Entry_ext2 + 0x7c00) 
1:
	pushw	%dx
	movb	$0x08, %ah      /* chs parameters obtained */
	pushw	%ds
	int	$0x13
	popw	%ds
	incb	%dh
	movb	%dh, 0x1a(%bp)
	andb	$0x3f, %cl
	movb	%cl, 0x18(%bp)
	popw	%dx
	
	xorl	%eax, %eax
	xorw	%bx, %bx
	pushw	$0x0600
	popw	%es
	movw	$1, %cx
	call	readDisk_ext2   /* Read MBR */
	
	movw	$0x61be, %si
	movw	$4, %cx
1:
	cmpb	$0x80, (%si)
	je	2f
	addw	$0x10, %si
	loop	1b
	movl	$0, 0x1c(%bp)
	xorl	%eax, %eax
	jmp	1f
2:
//	subw	$4, %cx		2014-01-26
//	negw	%cx
//	movb	%cl, 0x25(%bp)		//Partition number
//	movl    0xc(%si), %eax	//The total number of sectors	
//	movl    %eax, 0x20(%bp)
	movl    8(%si), %eax    /* First start logical sector */
	movl    %eax, 0x1c(%bp)
1:
	incl    %eax            /* Read from the second sector */ 
	pushw   $0x07e0
	popw    %es	
	movw    $3, %cx         /* Read 3 sectors */
	call    readDisk_ext2
	cmpw    $0xef53, 0x8038 /* Magic signature? */
//	je      1f              /* yes */		2014-01-26
	jne disk_error_ext2
//	addw    $0x10, %si      /* check the next partition table */
//	jmp     2b	
//1:	
	movw	$0x8000, %si
	movw	0x58(%si), %ax		/* AX=s_inode_size */
	testw	%ax, %ax 
        jnz     1f
        movw	$0x80, %ax		/* EXT2_GOOD_OLD_INODE_SIZE */	
1:
        movw    %ax, 0x26(%bp)	
	movb	0x18(%si), %cl		/* s_log_block_size */
	movl	$2, %eax
	shlw	%cl, %ax
	movb	%al, 0x0d(%bp)
	shlw	$9, %ax			/* block size is word wide */
	movw	%ax, 0x0e(%bp)
	shrw	$2, %ax
        movl	%eax, 0x14(%bp)
	addb	$8, %cl
	shll	%cl, %eax
        movl	%eax, 0x10(%bp)
        movl	0x28(%si), %eax		/* s_inodes_per_group */
        movl	%eax, 0x28(%bp)
	movl	4(%si), %eax
	movb	0x0d(%bp), %cl
	decb	%cl
	shll	%cl, %eax
	movl	%eax, 0x20(%bp)
        
        
        testb   $0x80, 0x60(%si)        /* 64-bit file system? */
        je      1f
        orb     $0x40, 2(%bp)           /* 64-bit file system marked */
1:
        testb   $0x10, 0x60(%si)        /* metablock group? */
        je      1f                      /* not */
        movl    0x104(%si), %eax        /* The first metablock group */
        movl    %eax, 0x2c(%bp)
        jmp     2f
1:                
	/* fix for ext2 partition: block number for group descriptors, offset 0x2c */
        /* At which block the group descriptors begin? */
	movl	0x14(%si), %eax		/* s_first_data_block */
	incw	%ax
	movl	%eax, 0x2c(%bp)
2:
        xorl	%ecx, %ecx
	
	xorl	%eax, %eax	/* CF=0, ZF=1 */

	/* MOV keeps all flags untouched, so it is better than INC */

	movb	$2, %al		/* EAX=2=inode number for root dir */


	/* CF=0, ZF=1 because MOV and PUSH do not touch Flags */

	/* read root dir to 0000:1000, and grldr to 1000:0000 */

	jmp 4f
         
/* Read sectors from disk, using LBA or CHS
 * input:	EAX   = 32-bit LBA sector number
 *		CX    = number of sectors to read
 *		ECX high word  = 0
 *		ES:BX = destination buffer
 *
 * output:	No return on error
 *		BX not changed
 *		ES    = ES + 0x20 * CX
 *		EAX   = EAX + CX
 *		ZF    = 0
 *		CF    = 0
 */
 
readDisk_ext2:
2:
	pushal
	xorl	%edx, %edx	/* EDX:EAX = LBA */
	pushl	%edx		/* hi 32bit of sector number */
	pushl	%eax		/* lo 32bit of sector number */
	pushw	%es		/* buffer segment */
	pushw	%bx		/* buffer offset */
	pushw	$1		/* 1 sector to read */
	pushw	$16		/* size of this parameter block */
	
	cmpb    $0x42, (ebios_ext2 - 1 - Entry_ext2 + 0x7c00)
	je      1f

	//xorl	%ecx, %ecx
	pushl	0x18(%bp)	/* lo:sectors per track, hi:number of heads */
	popw	%cx		/* ECX = sectors per track */
	divl	%ecx		/* residue is in EDX */
				/* quotient is in EAX */
				/* EDX high=0, DH=0 */
	incw	%dx		/* DL=sector number */
	popw	%cx		/* ECX = number of heads */
	pushw	%dx		/* push sector number into stack */
	xorw	%dx, %dx	/* EDX:EAX = cylinder * TotalHeads + head */
	divl	%ecx		/* residue is in EDX, head number */
				/* quotient is in EAX, cylinder number */
				/* EDX high=0, EAX high=0 */


	xchgb	%dl, %dh	/* head number should be in DH */
				/* DL = 0 */
	popw	%cx		/* pop sector number from stack */
	xchgb	%al, %ch	/* lo 8bit cylinder should be in CH */
				/* AL = 0 */
	shlb	$6, %ah		/* hi 2bit cylinder ... */
	orb	%ah, %cl	/* ... should be in CL */
	
	incw	%ax		/* AL=1, read 1 sector */

	/* Instead of 0x0e, the LBA indicator at 2(%bp) is 
	 *
	 *	0x42 for LBA
	 *
	 * and
	 *
	 *	0x02 for CHS
	 */
1:
	movb	$0x02, %ah
	/* ebios_ext2 - 1 points to 0x02 that can be changed to 0x42 */
ebios_ext2:

	//andb	2(%bp), %ah

	movw	%sp, %si	/* DS:SI points to disk address packet */
	movb	0x24(%bp), %dl	/* drive number */
	pushw	%es
	pushw	%ds
	int	$0x13
	popw	%ds
	popw	%bx
	jc	disk_error_ext2
	leaw	0x20(%bx), %bx
	movw	%bx, %es
	popaw			/* remove parameter block from stack */
	popal
	incl 	%eax		/* next sector, here ZF=0 */
	loop	2b
	ret
  
disk_error_ext2:

	movw	$(msg_DiskReadError_ext2 - Entry_ext2 + 0x7c00), %si

boot_error_ext2:

/* prints string DS:SI (modifies AX BX SI) */

//print_ext2:
1:
	lodsb	(%si), %al	/* get token */
	//xorw	%bx, %bx	/* video page 0 */
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	cmpb	$0, %al		/* end of string? */
	jne	1b		/* until done */

1:	jmp	1b 

	//. = . - (. - readDisk_ext2)/74
	
       . = Entry_ext2 + 0x1e0

msg_No_grldr_ext2:
	.ascii	"No "

filename_ext2:
	.ascii	"GRLDR\0\0\0\0\0\0\0\0"
	
msg_DiskReadError_ext2:
	.ascii	"Disk error\0"

  . = Entry_ext2 + 0x1fc       
	.word	0, 0xAA55

4:
	/* EAX holds the inode number: for root dir or grldr */

	/* These 3 PUSHes is intended to place 1000:0000 onto the stack for
	 * grldr. For root dir, the stack is not used since CF is cleared.
	 * Although there is no corresponding POPs, this is safe enough
	 * because the program comes here only twice: the first is for
	 * the root dir, and the second is for grldr.
	 *
	 * For root dir, CF=0 and ZF=1. For grldr, CF=1.
	 */
	movw	$0x1000, %di
	pushw	%di		/* 0x1000, see "jz 4b" below. */
	pushw	%ss		/* 0x0000 */
	pushfw

	/* SP=0x7bf4 for root dir, or 0x7bee for grldr */

	decl	%eax		/* EAX=(inode - 1) */

	/* inode numbers are far less than 0x7fffffff, so it is safe to
	 * initialise EDX with CDQ */

	cdq			/* let EDX=0 */

	divl	0x28(%bp)	/* s_inodes_per_group */
				/* EAX=group number */
	pushl	%edx		/* EDX=inode number in the group */

	/* group numbers are far less than 0x7fffffff, so it is safe to
	 * initialise EDX with CDQ */

	cdq			/* let EDX=0 */
	shll	$5, %eax	/* EAX=relative displacement of the group descriptor */ 
	
	testb   $0x40, 02(%bp)  /* filesystem 64? */
	je      1f              /* no */
	shll	$1, %eax        /* Group Descriptor Accounted for 64 bytes */
1:	
	
	divl	0x0e(%bp)	/* bytes per block */
				/* EAX=relative block number for the group descriptor */
				/* DX=displacement in the block */
				/* EDX high=0 */

	pushw	%dx		/* we don't care about EDX high word, because it is 0 '*/

	addl	0x2c(%bp), %eax	/* EAX=absolute block number for the group descriptor */
				/* CF=0, ZF=0 */

	call	read_block	/* 0000:1000 points to the block data containing the group descriptor */
				/* ES changed and > 0, BX=0x1000 */
				/* ECX=EDX=0 */
				/* CF=0, ZF=0 */

	popw	%si		/* DS:[BX+SI] points to the group descriptor */
				/* DS:[BX+SI+8] points to the starting block number of the group inode table */

	popl	%eax		/* inode number in the group */
	movw	0x26(%bp), %dx	/* EDX=inode size */
	mull	%edx		/* EDX:EAX=relative displacement of the inode struct */

	divl	0x0e(%bp)	/* bytes per block */
				/* EAX=relative block number for the inode struct */
	pushw	%dx		/* DX=displacement of the inode struct in the block */
				/* EDX high=0 */

	addl	8(%bx, %si), %eax	/* EAX=absolute block number for the inode struct */
					/* CF=0, ZF=0 */

	call	read_block	/* 0000:1000 points to the block data containing the inode struct */
				/* ES changed and > 0, BX=0x1000 */
				/* ECX=EDX=0 */
				/* CF=0, ZF=0 */

	popw	%si		/* DS:[BX+SI] points to the inode struct */

	addw	%bx, %si	/* DS:SI points to the inode struct */ 
	
	testl   $0x80000, 0x20(%si)     /* Node using the extents way to store data, */
	                                /* rather than the original direct_indirect index model to store data? */
	jne     extent                  /* yes */
	
	/* Move the inode struct to a known safe area(0000:0fa8 - 0000:0fff),
	 * that is, 0x58 bytes immediately before 0000:1000. We care about only
	 * the beginning 0x58 bytes of the 0x80-byte inode struct, the last
	 * 0x28 bytes are ignored. The area from 0xfa8+0x28 to 0xfa8+0x57
	 * stores 12 direct block pointers.
	 *  
	 *  
	 * At address   Initial value               Stores what?
	 * ==========   =============   ======================================
	 * 0xfa8+0x04      (const)           the size of the file in bytes
	 * 
	 * 0xfa8+0x08    total blocks           blocks left to read
	 * 
	 * 0xfa8+0x0c         0           serial number of the block to read
	 * 
	 */

	pushw	%ss
	popw	%es				/* ES=0 */

	leaw	-0x58(%bx), %di			/* BX=0x1000, so DI=0x0fa8 */

	movb	$0x2c, %cl			/* 0x2c words = 0x58 bytes */

	repz movsw				/* now ECX=0, BX=0x1000=DI */

	movl	%ecx, (0x0c - 0x58)(%di)	/* block serial number of the file */
						/* ECX=0 means first block */
						/* DI=0x1000 */

	movl	(0x04 - 0x58)(%di), %eax	/* i_size, the file size */
	decl	%eax
        
        cdq
        
	divl	0x0e(%bp)			/* bytes per block */
						/* EDX=various */
	incl	%eax
	movl	%eax, (0x08 - 0x58)(%di)	/* total blocks for file data */

	/*
	 * 0000:1000	trebly indirect block
	 * 0000:8000	indirect block
	 * 0000:c000	double indirect block
	 * 1000:0000	the file data
	 */

	/* now DS:SI points to indirect block number */

	lodsl					/* indirect block number */
	testl	%eax, %eax
	jz	1f	

	movb	$0x80, %bh			/* ES:BX=0000:8000 */
	call	read_block_c

						/* ES changed and > 0, BX=0x8000 */
						/* ECX=EDX=0 */
						/* ZF=0, CF=0 */

	/* now DS:SI points to double indirect block number */

	lodsl					/* double indirect block number */
	testl	%eax, %eax
	jz	1f

	movb	$0xc0, %bh			/* ES:BX=0000:c000 */
	call	read_block_c

						/* ES changed and > 0, BX=0xc000 */
						/* ECX=EDX=0 */
						/* ZF=0, CF=0 */
	
	/* now DS:SI points to trebly indirect block number */

	lodsl					/* trebly indirect block number */
	testl	%eax, %eax			/* CF=0, TEST always clears CF */
	jz	1f
						/* ZF=0 */
	call	read_block			/* 0000:1000 points to the block data */
						/* ES changed and > 0, BX=0x1000 */
						/* ECX=EDX=0 */
						/* ZF=0, CF=0 */


	/* the block at 0000:1000, which contains the indirect block numbers,
	 * is just overwritten by the trebly indirect block */
1:
	/* get absolute block number by block serial number */

	movl	(0x0c - 0x58)(%di), %ebx	/* block serial number of the file */
	subl	$12, %ebx
	jc	3f				/* direct block: block serial number < 12 */

	pushw	%bx
	subl	0x14(%bp), %ebx
	popw	%ax
	jnc	2f

	/* indirect block: 12 <= block serial number < 12 + 0x14(%bp) */

	addb	$(0x70 / 4), %ah
	jmp	8f

2:
	pushl	%ebx
	subl	0x10(%bp), %ebx
	jc	7f	/* EBX on the stack is < 0x10(%bp). double indirect block: */
			/* 12 + 0x14(%bp) <= block serial number < 12 + 0x14(%bp) + 0x10(%bp) */

	/* trebly indirect block: block serial number >= 12 + 0x14(%bp) + 0x10(%bp) */

	popl	%eax		/* discard the stack */
	xchgl	%eax, %ebx	/* move EBX to EAX */
				/* EDX=0 */
	divl	0x10(%bp)
				/* EAX=indirect block number, < 0x14(%bp) */
				/* EDX=block number, < 0x10(%bp) */

	pushl	%edx		/* EDX < 0x10(%bp) */
	testl	%edx, %edx
	jnz	7f

	/* EDX=0, so we need to load the double indirect block */

	shlw	$2, %ax
	xchgw	%ax, %bx

	/* get the double indirect block number from the trebly indirect
	 * block data */

	movl	(%bx, %di), %eax

	movw	$0xc000, %bx			/* ES:BX=0000:c000 */

	//pushw	%ss
	//popw	%es				/* ES=0 */
	//stc
	call	read_block_c	/* 0000:c000 points to the block data */
				/* ES changed and > 0, BX=0xc000 */
				/* ECX=EDX=0 */
				/* CF=0, ZF=0 */
7:
	popl	%eax		/* EAX < 0x10(%bp) */
	cdq			/* let EDX=0 (notice the above jc 7f and jnz 7f) */
	divl	0x14(%bp)
				/* EAX=indirect block number, < 0x14(%bp) */
				/* EDX=block number, < 0x14(%bp) */

	pushw	%dx		/* EDX < 0x14(%bp) */
	testw	%dx, %dx
	jnz	7f

	/* if DX=0, we need to load the indirect block */

	//addb	$(0xb0 / 4), %ah
	shlw	$2, %ax
	xchgw	%ax, %bx

	/* get the indirect block number from the double indirect block data */

	movl	0xb000(%bx, %di), %eax

	movw	$0x8000, %bx			/* ES:BX=0000:8000 */

	call	read_block_c	/* 0000:8000 points to the block data */
				/* ES changed and > 0, BX=0x8000 */
				/* ECX=EDX=0 */
				/* CF=0, ZF=0 */
7:
	popw	%ax		/* AX < 0x14(%bp) */
	
	addb	$(0x70 / 4), %ah
8:
	xchgw	%ax, %bx
3:
	shlw	$2, %bx
	movl	(%bx, %di), %eax

	/* got it! EAX=absolute block number */

	/* read block data to 1000:0000. For root dir, read each block to
	 * 1000:0000(overwrite the previous read). For grldr, read blocks
	 * one by one to the area starting at 1000:0000. 
	 */

	popfw
	popw	%bx
	popw	%es
	pushfw

	/* CF=0 and ZF=1 for reading root dir, CF=1 for reading grldr */

	call	read_block	/* 1000:0000 points to the block data */
				/* ES changed and > 0x1000, BX=0 */
				/* ECX=EDX=0 */
				/* CF=0, ZF=0 */

	popfw
	pushw	%es
	pushw	%bx
	pushfw

	jc	3f		/* CF=1, we are reading grldr */

	/* We have just read a block of the root dir to 1000:0000. 
	 * So we check all dir entries in the block to see if anyone
	 * matches grldr.
	 */
find_grldr:
	xorw	%si, %si
	pushw	%ss
	popw	%es		/* ES=0 */

2:
	pushw	%ds		/* DS=0 */
	movw	%di, %ds	/* DS=0x1000 */
	movw	$(filename_ext2 - Entry_ext2 + 0x7c00), %di
	
	pushw %di
102:
	cmpb $0, %es:(%di)
	je 101f
	cmpb $0x41, %es:(%di)
	jb 103f
	cmpb $0x5a, %es:(%di)
	ja 103f
	orb $0x20, %es:(%di)								/* Switch to lowercase */
103:
	incw %di
	jmp 102b
101:
	popw %di

	pushw	%si
	lodsl			/* This is possible inode number for grldr */
	pushl	%eax		/* This is possible inode number for grldr */
	lodsw
	xchgw	%ax, %dx	/* rec_len */
	lodsw			/* AL=name_len, should be 5 for grldr */
				/* AH=file_type(1 for regular file) */
        xorb    %ah, %ah
	xchgw	%ax, %cx	/* CX=name_len */
	repz cmpsb
	jnz	5f
	xchgw	%ax, %cx	/* movb	$0, %al */
	scasb
5:
	popl	%eax		/* This is possible inode number for grldr */
	popw	%si

	/* DS=0x1000, EAX=inode number */

	movw	%ds, %di	/* DI=0x1000 */
	popw	%ds		/* DS=0 */

	stc			/* indicates the new inode is for grldr */
	jz	4b		/* grldr is found with EAX=inode number */

	addw	%dx, %si
	cmpw	0x0e(%bp), %si	/* bytes per block */
	jb	2b
        
        

        
	/* file not found in this block, continue */

	/* We are lucky that CF=0, which indicates we are dealing with
	 * the root dir.
	 */
	movw	%fs, %ax
	orw	%ax, %ax
	jnz	1f

3:
        
	/* CF=1 for grldr, CF=0 for root dir. */

	incl	(0x0c - 0x58)(%di)
	decl	(0x08 - 0x58)(%di)
	jnz	1b
1:
	movw	$(msg_No_grldr_ext2 - Entry_ext2 + 0x7c00), %si

	jnc	boot_error_ext2		/* grldr not found in the root dir */

	/* All grldr blocks have been loaded to memory starting at 1000:0000,
	 * Before the boot, we pass boot_drive and boot_partition to grldr.
	 */

	/* ES>0x1000, BX=0, ECX=EDX=0, DI=0x1000, SS=0, SI>0x7c00, DS=0
	 * BP=0x7c00, SP<=0x7c00
	 */ 
	 
boot_now:
	movw	0x24(%bp), %dx

	/* boot it now! */

	pushw	%dx		/* for loading grub.exe */
	pushw	%di		/* 0x1000 */
	pushw	%ss		/* 0x0000 */
	lret

	
	  
	
	
extent:
        addw    $0x28, %si
1:        
        cmpw    $0xf30a, (%si)  /* Magic signature? */
        jne     disk_error_ext2 /* Not */
        movw    02(%si), %cx    /* number of effective index or leaves */
        cmpw    $0, 06(%si)     /* Leaf node? */
        je      2f              /* yes */
        /* ext4_extent_idx */
        addw    $0x0c, %si      /* Each leaf or index accounted 0x0c bytes */
        movb	$0x40, %ah				/* cf=0,zf=1 ES:BX=0000:1000 */
        sahf                    /* ah->flag, ES:BX=0000:1000 */
        movl    4(%si), %eax
        call    read_block
        movw    %bx, %si       
        jmp     1b        
2:      /* ext4_extent */
        lesw    -6(%bp), %bx    /* ES:BX=1000:0000 */
1:        
        addw    $0x0c, %si      /* Each leaf or index accounted 0x0c bytes */
        movl    8(%si), %eax    /* Lower 32 bits of physical block */
        pushw   %cx             /* number of effective leaves */
        movw    4(%si), %cx     /* Logic blocks */
        call    read_block_e
        popw    %cx
        loop    1b
        popfw
        pushfw
        jc	boot_now
        movw	%si, %fs
        jmp	find_grldr 

read_block_c:

	pushw	%ss
	popw	%es				/* ES=0 */
	stc
	
/* read_block - read a block
 * input:	CF	  - indicator for overlap or consecution
 *		EAX 	  = block number
 *		ES:BX	  - buffer
 *
 * output:	if CF is cleared on input, ES:BX is initialized to 0000:1000
 *		ES:BX	  - buffer filled with data
 *		ES, EAX	  - Changed
 *		ECX	  = 0
 *		EDX	  = 0
 *		ZF    = 0
 *		CF    = 0
 */

read_block:

	jc	1f

	.byte	0xC4, 0x5E, 0xFC	/* lesw -4(%bp), %bx */
					/* ES:BX=0000:1000 */
	jnz	1f

	//at this time, the gcc cannot generate 3 byte code
	.byte	0xC4, 0x5E, 0xFA	/* lesw -6(%bp), %bx */
					/* ES:BX=1000:0000 */
1:
	movw    $1, %cx
	
read_block_e:

	movzbl  0x0d(%bp), %edx /* sectors per block */         
	imulw   %dx, %cx        /* Number of sectors read */
	imull   %edx, %eax      /* Relative to the logical sector */
	
	
	
	addl	0x1c(%bp), %eax	/* EAX=absolute sector number */


	call	readDisk_ext2
	jb      disk_error_ext2
	ret
	
	. = Entry_ext2 + 0x3fc
	.word	0, 0xAA55	       	

       . = Entry_ext2 + 0x400
/* ********************  ext2_3_4_dbr end  ***************************** */	


/* ********************  exfat_dbr start  ****************************** */	

/*      Original exFAT Volume Boot Record
 *      Field Name                      Offset(byte)    Size(byte)      Description/Value
 *      Jump Boot                       0               3               0xEB7690
 *      File System Name                3               8               "EXFAT   "
 *      Must Be Zero                    11              53              Must be 0x00
 *      Partition Offset                64              8               Sector Address
 *      Volume Length                   72              8               Size of total volume in sectors
 *      FAT Offset                      80              4               Sector address of 1st FAT
 *      FAT Length                      84              4               Size of FAT in Sectors
 *      Cluster Heap offset             88              4               Sector address of the Data Region
 *      Cluster Count                   92              4               Number of clusters in the Cluster Heap
 *      Root Directory First Cluster    96              4               Cluster address of the Root Directory
 *      Volume Serial Number            100             4               Volume Serial Number
 *      File System Revision            104             2               VV.MM (01.00 for this release)
 *      Volume Flags                    106             2               Field                   Offset  bits Size       bits Description
 *                                                                      Active FAT              0       1               0-1st          1-2nd
 *                                                                      Volume Dirty            1       1               0-Clean        1-Dirty
 *                                                                      Media Failure           2       1               0-No Failures  1-Failures
 *                                                                      Reported Clear to Zero  3       1               No Meaning
 *                                                                      Reserved                4       12
 *      Bytes Per Sector                108             1               This is a power of 2. Range: min of 29 = 512 byte
 *                                                                      cluster size, and a max of 212 = 4096.
 *      Sectors Per Cluster             109             1               This is a power of 2. Range: Min of 21=512. The maximum Cluster size is 32 MiB,
 *                                                                      so the Values in Bytes per Sector + Sectors Per Cluster cannot exceed 25.
 *      Number of FATS                  110             1               This number is either 1 or 2, and is only 2 if TexFAT is in use.
 *      Drive Select                    111             1               Used by INT 13
 *      Percent In Use                  112             1               Percentage of Heap in use
 *      Reserved                        113             7
 *      Boot Code                       120             390             The Boot Program
 *      Boot Signature                  510             2               0xAA55
 *      Excess                          512                             If the sector is larger than 512 bytes, extra padding may exist beyond the signature
 */



/*      exFAT combined
        exFAT boot code provided by the Fan JianYe
        
        exFAT Current BPB Structure:
        Offset  Type    Description
        --------------------------------------------------------------------
        00      Word    EB 76   Jump instruction
        02      Byte    90
        03      DWord   "EXFAT   "
        0b      Word    Bytes per sector
        0d      byte    Retention
        0e      DWord   Data start Absolute sector
        12      DWord   The current cluster the absolute sector in fat table
        16      Word    EIOS Mark		Bit 7   EIOS
        18      Word    Sectors per track
        1a      Word    Number of heads
        1c      DWord   hidden sectors
        20      DWord   Retention
        24      Byte    Drive number
        25      Byte    Partition number
        26      Word    Sectors per cluster
        28      DWord   FAT table start absolute sector number
        2c      DWord   Retention 
 */        
                        
Entry_exfat:                                   
        jmp     1f                              /* cs=ds=es=0000 ss=2000 */
        
        .byte   0x90
        .ascii  "EXFAT   "
                
        . = Entry_exfat + 0x78        
1:        
        
        cli
	cld
//	testb	$0x80, %dl		2014-01-26
//	jne	1f
//	movb	$0xff, %dh
//	jmp 2f
//1:
//	cmpw	$0x7c00, %sp
//	jne	2f
//	subw	$0x7be, %bp
//	xchgw	%bp, %ax
//	shrb	$4, %al
//	movb	%al, %dh
//2:
	xorw	%ax, %ax
	movw	$0x7c00, %bp                    /* the base address */ 
        movw	%ax, %ss	                /* stack and BP-relative moves up, too */
//	movw	%ax, %ds
//	movw	%ax, %es
        movw    %bp, %sp                        /* ss:sp=0000:7c00 */
	sti
	pushw	%ax

	movw	%dx, 0x24(%bp)	/* BIOS passes drive number in DL */
	
	movb	0x6c(%bp), %cl                  /* Bytes per sector (power) */
	movw	$1, %ax
	shlw	%cl, %ax
	movw	%ax, 0x0b(%bp)                  /* Bytes per sector */
	
	movb	$0x41, %ah
	movw	$0x55AA, %bx
	int	$0x13
	jc	1f		# No EBIOS
	rorb	%cl   	        # ror cl: D0 C9
	jnc	1f		# No EBIOS
	orb	$0x80, 0x16(%bp)  /* EBIOS supported */
	jmp	2f
1:
	movb		$0x08, %ah
	int			$0x13
	movb    %dh, 0x1a(%bp)
	andb    $0x3f, %cl
	movb    %cl, 0x18(%bp)
2:
	popw	%ds
	        			
	movl    0x50(%bp), %eax
        addl    0x40(%bp), %eax
        testb   $1, 0x6a(%bp)
        je      2f
        addl    0x54(%bp), %eax
2:        
        movl    %eax, 0x28(%bp)                 /* FAT table start absolute sector number */ 
        movl    0x58(%bp), %eax
        addl    0x40(%bp), %eax                       
        movl    %eax, 0x0e(%bp)                 /* Data start Absolute sector */

        movb    0x6d(%bp), %cl                  /* Sectors per cluster (power) */
	movw $1, %ax
	shlw %cl, %ax
	movw %ax, 0x26(%bp)                  					/* Sectors per cluster */
        movl    0x40(%bp), %eax                 /* Sector Address */
				movl %eax, 0x1c(%bp)
        pushw   $0x07e0
        popw    %es	
        movw    $12, %cx
	call    readDisk_64
        jb      disk_error_64                   /* Read error display 'disk error'  */

        //Calculation exfat volume boot record checksum
        //si=the index entry (from 0 to cx) eax=checksum  cx=volume boot record check bytes (0x1600)
        xorw    %si, %si			//Initialize the index
        xorl    %eax, %eax			//Initialize the checksum
        movw    $0x1600, %cx           		//Bytes volume boot record check
1:
        rorl    $1, %eax                        //"Checksum" right Cycle 1 bit
        movzbl  0x7e00(%si), %ebx               //Check items
        addl    %ebx, %eax			//(("Checksum" Left Cycle 31 bit) or ("Checksum" right Cycle 1 bit)) + Check items
2:
        incw    %si				//index+1
        cmpw    $0x6a, %si                      //index=0x6a Exclude
        je      2b
        cmpw    $0x6b, %si                      //index=0x6b Exclude
        je      2b
        cmpw    $0x70, %si                      //index=0x70 Exclude
        je      2b
        cmpw    %cx, %si                        //index=Maximum?
        jb      1b			        //No, continue to
        //To determine the checksum
        movw    $0x9400, %di
        cmpl    (%di), %eax
        je      1f   
        //Filling Checksum
        pushw   %ds
        popw    %es               
        movw    $0x80, %cx
        repz    stosl                           //0x200 byte   
        //Boot sector write-back
        pushw   $0x07e0
        popw    %es
        pushw   %es
        movb	$0x03, (chs_64 + 2 - Entry_exfat + 0x7c00)
        movb	$0x43, (lba_64 + 1 - Entry_exfat + 0x7c00)
        movl    0x40(%bp), %eax
        movw    $12, %cx
        pushw   %cx
        call    readDisk_64 
        popw    %cx
        popw    %es
        call    readDisk_64     
        movb	$0x02, (chs_64 + 2 - Entry_exfat + 0x7c00)
        movb	$0x42, (lba_64 + 1 - Entry_exfat + 0x7c00)
1:
  pushw   %ds
  popw    %es
  movw    $0x8000, %si
  movw    $0x7e00, %di
  movw    $0x80, %cx
  repz    movsl                   //0x200 byte        
  movl    0x60(%bp), %eax         /* Root clusters */
	call    cluster_to_lba_64       /* Calculate the absolute sector from the cluster */
  jmp     exfat_next


/* Read a sector from disk, using LBA or CHS
 * input:	EAX - 32-bit DOS sector number
 *		ES:0000 - destination buffer
 *              CX -  Read number of sectors
 * output:	ES:0000 - next destination buffer
 *		EAX - next sector
 * warning:     EAX, ES Has changed
 */
readDisk_64:
3:
	pushal
	xorl	%edx, %edx	/* EDX:EAX = LBA */
	pushl	%edx		/* hi 32bit of sector number */
	pushl	%eax		/* lo 32bit of sector number */
	pushw	%es		/* buffer segment */
	pushw	$0		/* buffer offset */
	pushw	$1		/* 1 sector to read */
	pushw	$16		/* size of this parameter block */
        movw	%sp, %si	        /* DS:SI points to disk address packet */
        testb   $0x80, 0x16(%bp)           /* Support the expansion of reading? */
        jne     1f                      /* yes */
        /* C/H/S mode read */ 
        movzwl  0x18(%bp), %ecx /* sectors per track */
	divl	%ecx		/* Tracks is in EAX,Sectors in EDX */
	incw	%dx		/* sector number in DL */
	pushw	%dx		/* push sector number into stack */
				/* quotient is in EAX, cylinder number */        
        pushl	%eax            /* eax High 16 move to dx */
        popw	%ax     
        popw	%dx
        divw    0x1a(%bp)       /* ax=Tracks/Heads=cylinder number, dx=Head number */	
	xchgb	%dl, %dh	/* head number should be in DH,DL=0 */
	popw	%cx		/* pop sector number from stack */
	xchgb	%al, %ch	/* lo 8bit cylinder should be in CH,AL=0 */ 
        rorb	$2, %ah	        /* hi 2bit cylinder number moble from 0,1 to 6,7 bit */
	orb	%ah, %cl	/* hi 2bit cylinder number (7,6) and the sector number (5-0) are combined */
chs_64:		
	movw	$0x201, %ax	/* read 1 sector */
	jmp     2f
lba_64:
1:	
        movb    $0x42, %ah      /* Extended Mode Read */
2:      
        movb	0x24(%bp), %dl  /* hard disk drive number */
	pushw	%ds
	pushw	%es
	int	$0x13
	popw	%bx
	popw	%ds	
	pushfw
	leaw	0x20(%bx), %bx
	movw    %bx, %es
	popfw
	
	popaw			/* remove parameter block from stack */
	popal
        jc      2f              /* disk read error */
        incl 	%eax	        /* next Absolute logical sector */
        loop    3b
        clc                     /* Read complete */
2:	ret  

	. = Entry_exfat + 0x1e0
msg_BootError_64:
	.ascii	"No "
	
grldr_un:
        .ascii  "GRLDR\0\0\0\0\0\0\0\0"	

msg_DiskReadError_64:
	.ascii	"Disk error\0"  
	
	. = Entry_exfat + 0x1fc
	.word	0,0xAA55 
	

exfat_next: 
        /* Read all the root directory to 2000:0000 */
        lesw	(loadseg_off_64 - Entry_exfat)(%bp), %bx	/* load to loadseg:0 */ 
        movl    %eax, %esi
        jmp     2f
7:        
        movw    $0xffe0, %di            /* di initialization  ffe0+20=0000 */
        
6:      /* Determine the entry type */
        addw    $0x20, %di              /* Next Directory */
        cmpw    0x0b(%bp), %di          /* Bytes per sector*/
        jb      1f
2:        
        call    read_2_sectors          /* Read all the root directory to 2000:0000 */
        jb      disk_error_64           /* Read error display 'disk error'  */
        jmp     7b         
1:        
        cmpb    $0, %es:(%di)           /* Contents end? */
        je      grldr_error_64          /* yes */   
        cmpb    $0xc0, %es:(%di)        /* Stream Directory */
        jne     6b                      /* Not */
        /* Determine the length of the file name */
				movb %es:3(%di), %cl          	/* File name length */
        pushw   %di                     /* Save the current directory entry */
        addw    $0x22, %di              /* Filename address */                                                   
				pushl %esi
        movw    $(grldr_un - Entry_exfat + 0x7c00), %si /*'GRLDR' First place */
1:    
				cmpb $0x61, %es:(%di)
				jb 101f
				cmpb $0x7a, %es:(%di)
				ja 101f
        andb $0xdf, %es:(%di)						/* lowercase to Switch */
101:        
				cmpsb                           /* More 'grldr',Unicode file names */
        jne     1f                      /* NO,Restore directory entry, check the next entry */
        incw    %di
        loop    1b                      /* The same, compare the next character */
				cmpb $0, (%si)
				jne 1f
        /* GRLDR has been found */
				popl %esi
        popw	%di                     /* Restore directory entry */
        /* files stored continuously, Calculate the number of sectors the file */
        movl    %es:0x18(%di), %eax     /* 32-bit low byte file */
        movb    0x6c(%bp), %cl          /* Bytes per sector (power) */
        shrl    %cl, %eax               /* ax = file number of sectors */ 
        incw    %ax                     /* May be more than the number, file number of sectors + 1 */ 
        pushw   %ax                     /* File number of sectors */
        testb	$2, %es:1(%di)	        /* Bit 2 is equal to 1 indicates no FAT chain, files stored continuously */
        je 2f                           /* File storage is not continuous */
        /* Read file to 2000:0000 */        
        movl    %es:0x14(%di), %eax     /* Starting cluster file */
        call    cluster_to_lba_64       /* Calculate the absolute sector from the cluster */
        popw	%cx                     /* File number of sectors */
        call    readDisk_64             /* Read file GRLDR to 2000:0000 */
        jmp     5f        
1:      
        /* Restore directory entry, check the next entry */
				popl %esi
        popw	%di                     /* NO,Restore directory entry */
        jmp     6b                      /* Check the next entry */                         
2:      
        /* File storage is not continuous, read the file to 2000:0000 */
        popw    %cx
        movl    %es:0x14(%di), %eax     /* Starting cluster file */
        call    Read_Discontinuous      /* Read file GRLDR to 2000:0000 */
5:      movw    0x24(%bp), %dx          /* dh=Partition number (starting from 0)  dl=Drive number */
        pushw   %dx                     /* Application just to grub.exe */
        ljmp    $LOADSEG,$0             /* Go to 2000:0000 */  



	
/* Read 2 sectors
 * First read sector from the current cluster, 
 * followed by reading from the cluster of sectors. 
 * Encountered cluster terminator stop.
 * input:       es:0000=Buffer   esi=The first absolutely  logical sector
 * output:      0x60(%bp)=current cluster   esi=The second absolutely logical sector
 */
read_2_sectors:
        pushw   %es
        movl    %esi, %eax
        movw    $1, %cx                         /* Number of sectors read */    
        call    readDisk_64                     /* Read sectors */
        jb      4f
        pushl   %eax                            /* The second absolutely logical sector */
        subl    0x0e(%bp), %eax                 /* The first relatively logical sector */  /* Data start Absolute sector */
        xorl    %edx, %edx
        movzwl  0x26(%bp), %ecx                 /* sectors per cluster */
        divl    %ecx                            /* eax=Cluster number  edx=Sector offset */    
        cmpw    0x26(%bp), %dx                  /* In the current cluster? */
        /* In the next cluster */
        popl    %eax
        jb 3f																		/* Yes */
        movl    0x60(%bp), %eax                 /* current cluster */
        call    NextCluster                     /* Find the next cluster from the current cluster */
        movl    %eax, 0x60(%bp)                 /* current cluster */
        jb      1f                              /* Not the end of the cluster */ 
                
        xorw    %ax, %ax
        xorw    %di, %di
        xorl    %esi, %esi
        movw    $0x100, %cx        
        repz    stosw
        jmp     2f
1:        
        call    cluster_to_lba_64               /* Calculate the absolute sector from the cluster */
        jmp     1f        
3:  
        /* In the current cluster */ 
1:        
        movl    %eax, %esi                      /* The second absolutely logical sector */  
        movw    $1, %cx                         /* Number of sectors read */
        call    readDisk_64                     /* Read sectors */
        jnb     2f        
4:      stc        
2:
        popw    %es                
        ret        	
                
 
/* Read sectors from the cluster 
 * Input:       eax=Cluster number, es:0000=buffer, cx=File number of sectors 
 * Warning:     EAX Has changed
 */      
Read_Discontinuous:
        pushw   %es
        pushw   %di
        movw 0x26(%bp), %di 
1:             
        pushl	%eax                    /* Cluster number */
        pushw   %cx
        call    cluster_to_lba_64       /* Calculate the absolute sector from the cluster */
        popw    %cx        
        cmpw    %di, %cx
        jb      2f
        subw    %di, %cx
        pushw   %cx                
        movw    %di, %cx                /* sectors per cluster */
        jmp     3f
2:                
        pushw   $0               
3:
        call    readDisk_64             /* Read Sector */ 
        popw    %cx
        popl    %eax                    /* Cluster number */
        jb      1f                      /* Reading failure or end */
        orw     %cx, %cx
        je      1f
        pushw   %cx
        call    NextCluster             /* Find the next cluster from the current cluster */
        popw    %cx
        jb      1b                      /* Not the end of the cluster, continue to read */              
1:              
        popw    %di
        popw    %es
        ret 



/* Find the next cluster from the current cluster */
/* Input: eax=current cluster number */
/* Otput: eax=next cluster number, Less than=Normal */
/* [bp+2d]=The current cluster in the absolute sector fat table */
NextCluster:    
        pushw   %bx
        pushw   %es
        lesw	(FAT_Address - Entry_exfat)(%bp), %bx           /* es:bx=0060:0000 */
        /* Computing bytes per cluster */
        shll    $2, %eax                                        /* 4 bytes per cluster */     
        /* Computing cluster in the FAT table sector offset / byte offset */ 
        xorl    %edx, %edx
        movzwl  0x0b(%bp), %ecx                                 /* Bytes per sector */
        divl    %ecx                                            /* eax=sector offset, edx=byte offset */      
        /* Calculate the absolute sector of clusters */
        addl    0x28(%bp), %eax                                 /* FAT table start absolute sector number */
        pushw   %dx                                             /* byte offset */
        /* Determine whether or not to read the fat table */
        cmpl    %eax, 0x12(%bp)																	//The current cluster the absolute sector in fat table
        je      1f                                              /* Does not read the fat table */
        /* read the fat table */
        movl    %eax, 0x12(%bp)                                 /* Save the current address of fat table */ //The current cluster the absolute sector in fat table               
        movw    $1, %cx                                         /* Read 1 sector */ 
        pushw   %es
        call    readDisk_64                                     /* Reading FAT table */
        popw    %es
1:             
        popw	%bx                                             /* byte offset */
        movzwl  %es:(%bx), %eax                                 /* fat12/16 Next cluster number */
        cmpl    $0xfffffff8, %eax
        popw	%es 
        popw	%bx
        ret
   
/* Calculate the absolute sector from the cluster
 * input:       eax=Cluster number
 * output:      eax=Absolute sector
 */
cluster_to_lba_64:
	/* sector = (cluster-2) * clustersize + data_start */
	decl	%eax
	decl	%eax 
        movb    0x6d(%bp), %cl          /* Sectors per cluster (power) */
        shll    %cl, %eax 
        addl	0x0e(%bp), %eax         /* Absolute sector */   /* Data start Absolute sector */
	ret


grldr_error_64:
        movw	$(msg_BootError_64 - Entry_exfat + 0x7c00), %si
        jmp     print_64        
disk_error_64:
	movw	$(msg_DiskReadError_64 - Entry_exfat + 0x7c00), %si
	        
        
print_64:
1:
	lodsb	(%si), %al	/* get token */
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	cmpb	$0, %al		/* end of string? */
	jne	1b		/* until done */

1:      jmp    1f 

loadseg_off_64:
	.word	0
	.word	LOADSEG 	
FAT_Address:
        .word	0
	.word	FATSEG  
       
	. = Entry_exfat + 0x3fc	
	.word	0,0xAA55         

	. = Entry_exfat + 0x0400
	
/* ********************  exfat_dbr end  ****************************** */ 

#endif    /* defined(GRLDR_INSTALL) */

/* ********************  ntfs_dbr_mbr start  ****************************** */	
	
	

#define INSIDE_GRLDR

//#include "ntfsbs.S"
//-----------------begin of "ntfsbs.S"-----------------------
/*
 *  GRUB Utilities --  Utilities for GRUB Legacy, GRUB2 and GRUB for DOS
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

/* NTFS boot sector for loading GRLDR , written by bean
 *
 * This file can be compiled as standalone boot sector, or it can be embeded in
 * GRLDR.MBR at 0xA00 , right after the ext2 boot sector
 *
 * To compile the standalone ntfsbs.bin:
 *     gcc -c -o ntfsbs.o ntfsbs.S
 *     gcc -nostdlib -Wl,-N -Wl,-Ttext -Wl,7C00 -o ntfsbs_exec ntfsbs.o
 *     objcopy -O binary ntfsbs_exec ntfsbs.bin
 *
 * To install the standalone ntfsbs.bin:
 *     grubinst --restore=ntfsbs.bin DEVICE_OR_FILE
 *
 * Where DEVICE_OR_FILE specify a NTFS partition
 *
 * Limitations:
 *  1. Don't support >1K MFT record size, >4K INDEX record size
 *  2. Don't support encrypted file
 *  3. Don't support >4K non-resident attribute list and $BITMAP '
 *
 */

#ifndef INSIDE_GRLDR

	.text

	.code16
#endif

#define AT_STANDARD_INFORMATION	0x10
#define AT_ATTRIBUTE_LIST	0x20
#define AT_FILENAME		0x30
#define AT_OBJECT_ID		0x40
#define AT_SECURITY_DESCRIPTOR	0x50
#define AT_VOLUME_NAME		0x60
#define AT_VOLUME_INFORMATION	0x70
#define AT_DATA			0x80
#define AT_INDEX_ROOT		0x90
#define AT_INDEX_ALLOCATION	0xA0
#define AT_BITMAP		0xB0
#define AT_SYMLINK		0xC0
#define AT_EA_INFORMATION	0xD0
#define AT_EA			0xE0

#define MAX_MFT_SIZE	1		// 1<<(1+9) = 1024
#define MAX_IDX_SIZE	3		// 1<<(3+9) = 4096

#define LOADSEG_NT	0x2000

#define MMFT_BASE	0x2000
#define MMFT_EMFT	(MMFT_BASE + 1024)
#define MMFT_EBUF	(MMFT_BASE + 2048)

#define CMFT_BASE	(MMFT_BASE + 6144)
#define CMFT_EMFT	(CMFT_BASE + 1024)
#define CMFT_EBUF	(CMFT_BASE + 2048)

#define INDX_BASE	(CMFT_BASE + 6144)

#define SBUF_BASE	(INDX_BASE + 4096)

#define NTFS_Large_Structure_Error_Code	1
#define NTFS_Corrupt_Error_Code		2
#define NTFS_Run_Overflow_Error_Code	3
#define NTFS_No_Data_Error_Code		4
#define NTFS_Decompress_Error_Code	5

#define NT_FG_COMP	1
#define NT_FG_MMFT	2
#define NT_FG_ALST	4
#define NT_FG_GPOS	8

#define nt_boot_drive	-2(%bp)
#define nt_blocksize	-4(%bp)
#define nt_spc		-5(%bp)
#define nt_mft_size	-6(%bp)
#define nt_idx_size	-7(%bp)
#define nt_mft_start	-12(%bp)
#define nt_remain_len	-16(%bp)
//#define nt_file_count	-18(%bp)

#define nt_flag		(%di)
#define nt_attr_cur	2(%di)
#define nt_attr_nxt	4(%di)
#define nt_attr_end	6(%di)
#define nt_curr_vcn	8(%di)
#define nt_curr_lcn	0x10(%di)
#define nt_attr_ofs	0x14(%di)
#define nt_target_vcn	0x18(%di)
#define nt_read_count	0x1C(%di)
#define nt_vcn_offset	0x20(%di)

#define nt_emft_buf	1024(%di)
#define nt_edat_buf	2048(%di)

	.arch	i586

Entry_nt:
	jmp	1f

	. = Entry_nt + 0x02

	.byte	0x90	/* for CHS. Another possible value is 0x0e for LBA */

	.ascii	"NTFS    "

	.word	0	/* 0B - Bytes per sector */
	.byte	0	/* 0D - Sectors per cluster */
	.word	0	/* 0E - reserved sectors, unused */
	.byte	0	/* 10 - number of FATs, unused */
	.word	0	/* 11 - Max dir entries for FAT12/FAT16, unused */
	.word	0	/* 13 - total sectors for FAT12/FAT16, unused */
	.byte	0xF8	/* 15 - Media descriptor */
	.word	0	/* 16 - sectors per FAT for FAT12/FAT16, unused */
	.word	255	/* 18 - Sectors per track */
	.word	63	/* 1A - Number of heads */
nt_part_ofs:
	.long	0	/* 1C - hidden sectors */
	.long	0	/* 20 - total sectors for FAT32, unused */
	.long	0x800080
			/* 24 - Usually 80 00 80 00, A value of 80 00 00 00 has
			 * been seen on a USB thumb drive which is formatted
			 * with NTFS under Windows XP. Note this is removable
			 * media and is not partitioned, the drive as a whole
			 * is NTFS formatted.
		 	 */
	.long	0,0	/* 28 - Number of sectors in the volume */
	.long	0,0	/* 30 - LCN of VCN 0 of the $MFT */
	.long	0,0	/* 38 - LCN of VCN 0 of the $MFTMirr */
	.long	0	/* 40 - Clusters per MFT Record */
	.long	4	/* 44 - Clusters per Index Record */
	.long	0,0	/* 48 - Volume serial number */
	.long	0	/* 50 - Checksum, usually 0 */

1:

	. = Entry_nt + 0x54

	cli
	cld

	. = Entry_nt + 0x56

	/* the byte at offset 0x57 stores the real partition number for read.
	 * the format program or the caller should set it to a correct value.
	 * For floppies, it should be 0xff, which stands for whole drive.
	 */

	movb	$0xff, %dh	/* boot partition number */

	xorw	%ax, %ax
	movw	$0x7c00, %bp

	movw	%ax, %ss	/* stack and BP-relative moves up, too */
	leaw	-0x20(%bp), %sp
	sti

	movw	%dx, nt_boot_drive

	pushw	%ax		/* AX=0 */

	/* Test if your BIOS support LBA mode */
	movb	$0x41, %ah
	movw	$0x55AA, %bx
	int	$0x13

	popw	%ds		/* DS=0 */

	jc	1f		/* No EBIOS */
	cmpw	$0xAA55, %bx
	jne	1f		/* No EBIOS */
//	testb	$1, %cl
//	jz	1f		/* No EBIOS */
	rorb	%cl
	jnc	1f		# No EBIOS
	/* EBIOS supported */
	movb	$0x42, (ebios_nt - 1 - Entry_nt)(%bp)
1:

	pushw	%ss		/* SS=0 */
	popw	%es		/* ES=0 */

	cmpl	$0x42555247, (nt_sector_mark - Entry_nt)(%bp)
	jz	1f			// Must be called from GRLDR.MBR

	movw	$0x7E00, %bx
	movl	(nt_part_ofs - Entry_nt)(%bp), %eax
	incl	%eax
	call	readDisk_nt		// Load the second sector from disk
	call	readDisk_nt		// Load the third sector from disk
	call	readDisk_nt
1:

	xorl	%eax, %eax
	movw	0xb(%bp), %ax		// Bytes per sector (blocksize)
	movw	%ax, nt_blocksize

	call	convert_to_power_2
	movb	%cl, %bl
	movb	0xd(%bp), %al		// Sectors per cluster
	call	convert_to_power_2
	movb	%cl, %ch
	addb	%bl, %ch
	subb	$9, %ch			// 1<<ch = sectors per cluster
	movb	%ch, nt_spc
	movb	0x44(%bp), %al 		// Index record size (high bits of eax is 0)
	call	convert_size

	cmpb	$MAX_IDX_SIZE, %cl
	jbe	1f

NTFS_Large_Structure_Error:
	movb	$NTFS_Large_Structure_Error_Code, %al
	jmp	NTFS_Error

1:
	movb	%cl, nt_idx_size

	movb	0x40(%bp), %al 		// MFT record size
	call	convert_size

	cmpb	$MAX_MFT_SIZE, %cl
	jnz	NTFS_Large_Structure_Error

	movb	%cl, nt_mft_size

	movl	0x30(%bp), %eax
	//movl	0x34(%bp), %edx

	movb	%ch, %cl		// ch still contains nt_spc

	//shldl	%cl, %eax, %edx
	//orl	%edx, %edx
	//jnz	NTFS_Large_Structure_Error

	shll	%cl, %eax
	addl	(nt_part_ofs - Entry_nt)(%bp), %eax
	movl	%eax, nt_mft_start

	movw	$1, %dx
	movb	nt_mft_size, %cl
	shlw	%cl, %dx
	movw	%dx, %cx

	movw	$MMFT_BASE, %bx
	pushw	%bx
1:
	call	readDisk_nt
	loop	1b

	popw	%bx
	cmpw	$0x4946, (%bx)		// "FI"
	jnz	NTFS_Corrupt_Error

	// dx should still contain the number of sectors in the MFT record
	movw	%dx, %cx
	call	ntfs_fixup

	movw	%bx, %di
	movb	$AT_DATA, %al		// find $DATA

	call	ntfs_locate_attr
	jc	NTFS_Corrupt_Error

	movw	$CMFT_BASE, %bx
	xorl	%eax, %eax
	movb	$0x5, %al
	call	ntfs_read_mft
	movw	%bx, %di

	jmp	ntfs_search

// Convert the size of MFT and IDX block
// Input:
//     eax: size
//     ch: spc
// Output:
//     cl: convert value
convert_size:
	orb	%al, %al
	js	1f
	movb	%ch, %cl
	jmp	2f			// Jump to 2 in convert_to_power_2
1:
	negb	%al
	subb	$9, %al
	movb	%al, %cl
	ret

// Convert number to a power of 2
// Input:
//     eax
// Output:
//     cl: 1<<cl = eax
//     eax: 0

convert_to_power_2:
	xorb	%cl, %cl
2:
	incb	%cl
	shrl	$1, %eax
	jnc	2b
	decb	%cl
	ret

// Fixup the "FILE" and "INDX" record
// Input:
//     DS:BX - data buffer
//     CX - buffer length in sectors
//

ntfs_fixup:
	push	%bx
	push	%di
	movw	%bx, %di

	movw	6(%bx), %ax		// Size of Update Sequence
	decw	%ax
	movw	%ax, %bx

	mulw	nt_blocksize
	shlw	$9, %cx
	cmpw	%ax, %cx
	jnz	NTFS_Corrupt_Error	// blocksize * count != size

	movw	%bx, %cx		// cx = count

	movw	%di, %bx
	addw	4(%bx), %bx		// Offset to the update sequence
	movw	(%bx), %ax		// Update Sequence Number
	subw	$2, %di

1:
	addw	nt_blocksize, %di
	addw	$2, %bx
	cmpw	(%di), %ax
	jnz	NTFS_Corrupt_Error
	movw	(%bx), %dx
	movw	%dx, (%di)
	loop	1b

	popw	%di
	popw	%bx
	ret

NTFS_Corrupt_Error:
	movb	$NTFS_Corrupt_Error_Code, %al
	jmp	NTFS_Error

/* Read a sector from disk, using LBA or CHS
 * input:	EAX - 32-bit DOS sector number
 *		ES:BX - destination buffer
 *		(will be filled with 1 sector of data)
 * output:	ES:BX points one byte after the last byte read.
 *		EAX - next sector
 */

readDisk_nt:

	pushal
	xorl	%edx, %edx	/* EDX:EAX = LBA */
	pushl	%edx		/* hi 32bit of sector number */
	pushl	%eax		/* lo 32bit of sector number */
	pushw	%es		/* buffer segment */
	pushw	%bx		/* buffer offset */
	pushw	$1		/* 1 sector to read */
	pushw	$16		/* size of this parameter block */

	xorl	%ecx, %ecx
	pushl	0x18(%bp)	/* lo:sectors per track, hi:number of heads */
	popw	%cx		/* ECX = sectors per track */
	divl	%ecx		/* residue is in EDX */
				/* quotient is in EAX */
	incw	%dx		/* sector number in DL */
	popw	%cx		/* ECX = number of heads */
	pushw	%dx		/* push sector number into stack */
	xorw	%dx, %dx	/* EDX:EAX = cylinder * TotalHeads + head */
	divl	%ecx		/* residue is in EDX, head number */
				/* quotient is in EAX, cylinder number */
	xchgb	%dl, %dh	/* head number should be in DH */
				/* DL = 0 */
	popw	%cx		/* pop sector number from stack */
	xchgb	%al, %ch	/* lo 8bit cylinder should be in CH */
				/* AL = 0 */
	shlb	$6, %ah		/* hi 2bit cylinder ... */
	orb	%ah, %cl	/* ... should be in CL */

	movw	$0x201, %ax	/* read 1 sector */
ebios_nt: /* ebios_nt - 1 points to 0x02 that can be changed to 0x42 */

//	cmpb	$0x0e, 2(%bp)	/* force LBA? */
//	jnz	1f		/* no, continue */
//	movb	$0x42, %ah	/* yes, use extended disk read */
//1:
	movw	%sp, %si	/* DS:SI points to disk address packet */
	movb	nt_boot_drive, %dl	/* hard disk drive number */

	int	$0x13

	popaw			/* remove parameter block from stack */
	popal
	jc	disk_error_nt	/* disk read error, jc 1f if caller handles */
	incl 	%eax		/* next sector */
	addw	0x0b(%bp), %bx	/* bytes per sector */
	jnc	1f		/* 64K bound check */
	pushw	%dx
	movw	%es, %dx
	addb	$0x10, %dh	/* add 1000h to ES */
				/* here, carry is cleared */
	movw	%dx, %es
	popw	%dx
1:
	/* carry stored on disk read error */
	ret




NTFS_Error:
	addb	%al, (msg_DiskReadError_nt - Entry_nt)(%bp)
	jmp	disk_error_nt


// Kernel load address, located at 0x1E8
//	. = Entry_nt + 0x1e8

//nt_boot_image_end:

nt_loadseg_off:
	.word	0
	.word	LOADSEG_NT

// Boot image offset and length, located at 0x1EE
// Lower 11 bit is offset, higher 5 bit is length
//	. = Entry_nt + 0x1ec

//nt_boot_image_ofs:
//	.word (nt_boot_image - Entry_nt)+(nt_boot_image_end - nt_boot_image-1)*2048

//	. = Entry_nt + 0x1ee

disk_error_nt:

	movw	$(msg_DiskReadError_nt - Entry_nt + 0x7c00), %si

boot_error_nt:

/* prints string DS:SI (modifies AX BX SI) */

//print_32:
print_ntfs:
1:
	lodsb	(%si), %al	/* get token */
	//xorw	%bx, %bx	/* video page 0 */
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	cmpb	$0, %al		/* end of string? */
	jne	1b		/* until done */

	/* The caller will change this to
	 *	ljmp	$0x9400, $(try_next_partition - _start1)
	 */
	jmp 1f

        . = Entry_nt + 0x1e0

msg_NTFS_Not_Found_Error:
	.ascii "No "

nt_boot_image:
	.ascii "GRLDR\0\0\0\0\0\0\0\0"

msg_DiskReadError_nt:
	.ascii	"0\0" 

	. = Entry_nt + 0x1f4
	
error_go_back:
1:
	jmp	1b
	
	. = Entry_nt + 0x1fc

	.word	0, 0xAA55

// Here starts sector #2

// Input:
//     DI - current mft
ntfs_search:
	//movw	$0, nt_file_count
	call	ntfs_init_attr
	movb	$AT_INDEX_ROOT, %al

1:
	call	ntfs_find_attr
	jc	NTFS_Not_Found_Error

	cmpl	$0x180400,  8(%si)	// resident
					// namelen = 4
					// name offset = 0x18
	jnz	1b
	//cmpl	$0x490024, 0x18(%si)	// "$I"
	//jnz	1b
	//cmpl	$0x300033, 0x1C(%si)
	//jnz	1b			// "30"
	//testw	$0xC001, 12(%si)	// not compressed, encrypted or sparse
	//jnz	1b

	addw	0x14(%si), %si		// jump to attribute
	cmpb	$0x30, (%si)
	jnz	1b			// test if it index filenames

	addw	$0x10, %si		// skip the index root
	addw	(%si), %si

	call	ntfs_find_grldr
	jnc	ntfs_final

	call	ntfs_init_attr
	movb	$AT_BITMAP, %al
1:
	call	ntfs_find_attr
	jc	NTFS_Not_Found_Error
	movw	9(%si), %bx
	cmpb	$4, %bl
	jnz	1b
	//shrw	$4, %bx
	//cmpl	$0x490024, (%bx, %si)	// "$I"
	//jnz	1b
	cmpb	$0, 8(%si)
	jnz	1f
	pushw	0x10(%si)
	addw	0x14(%si), %si
	pushw	%si
	jmp	2f
1:
	pushw	0x30(%si)
	xorl	%edx, %edx
	movl	0x28(%si), %ecx
	cmpw	$4096, %cx
	ja	NTFS_Not_Found_Error
	shrl	$9, %ecx
	movw	$SBUF_BASE, %bx
	pushw	%bx
	call	ntfs_read_data
2:

	movb	$AT_INDEX_ALLOCATION, %al

1:
	call	ntfs_locate_attr
	jc	NTFS_Not_Found_Error

	cmpl	$0x400401, 8(%si)	// non-resident
					// namelen = 4
					// name offset = 0x40
	jnz	1b
	//cmpl	$0x490024, 0x40(%si)	// "$I"
	//jnz	1b
	//cmpl	$0x300033, 0x44(%si)
	//jnz	1b			// "30"
	//testw	$0xC001, 12(%si)	// not compressed, encrypted or sparse
	//jnz	1b

	movb	nt_idx_size, %cl
	xorl	%ebx, %ebx
	movb	$1, %bl
	shll	%cl, %ebx		// ebx - index size
	xorl	%edx, %edx		// edx - index offset


	popw	%si
	popw	%cx

1:
	pushw	%cx
	lodsb	(%si), %al

	movw	$8, %cx
2:
	pushw	%cx
	pushw	%ax
	testb	$1, %al
	jz	3f
	pushw	%si
	pushl	%edx
	pushl	%ebx

	movl	%ebx, %ecx
	movw	$INDX_BASE, %bx
	call	ntfs_read_attr
	jc	NTFS_Not_Found_Error
	cmpw	$0x4E49, (%bx)		// "IN"
	jnz	NTFS_Not_Found_Error
	call	ntfs_fixup
	movw	%bx, %si
	addw	$0x18, %si
	addw	(%si), %si

	call	ntfs_find_grldr
	jnc	ntfs_final_0

	popl	%ebx
	popl	%edx
	popw	%si

3:
	addl	%ebx, %edx

	popw	%ax
	shrb	$1, %al
	popw	%cx
	loop	2b

	popw	%cx
	loop	1b

	//pushw	nt_file_count
	//call	hex_out

NTFS_Not_Found_Error:
	leaw	(msg_NTFS_Not_Found_Error - Entry_nt)(%bp), %si
	jmp	boot_error_nt

ntfs_final_0:
	//addw	$16, %sp

// Input:
//     DI - current mft
//     SI - index entry
ntfs_final:
	cmpw	$0, 4(%si)
	jnz	NTFS_Large_Structure_Error

	movl	(%si), %eax
	movw	%di, %bx
	call	ntfs_read_mft

	movb	$AT_DATA, %al
	call	ntfs_locate_attr
	jc	NTFS_No_Data_Error

	cmpb	$1, 8(%si)		// non-resident / resident
	jz	1f

	movw	0x10(%si), %cx		// Resident
	lesw	(nt_loadseg_off - Entry_nt)(%bp), %di
	addw	0x14(%si), %si
	rep	movsb	(%si), %es:(%di)
	jmp	2f

1:

	xorl	%edx, %edx
	movl	0x28(%si), %ecx		// Use allocate size instead of real size
	shrl	$9, %ecx

	lesw	(nt_loadseg_off - Entry_nt)(%bp), %bx
	call	ntfs_read_data


2:

	//movb	$1, (do_pause - Entry_nt)(%bp)
	//call	pause

	movw	nt_boot_drive, %dx
	pushw	%dx		/* for loading grub.exe */
	ljmp	*(nt_loadseg_off - Entry_nt)(%bp)

NTFS_No_Data_Error:
	movb	$NTFS_No_Data_Error_Code, %al
	jmp	NTFS_Error

// Try to find GRLDR in the index
// Input:
//     DS:SI - points to index entry
// Output:
//     CF - status

ntfs_find_grldr:
	movw	%si, %bx
	testb	$2, 0xC(%bx)
	jz	1f
	stc
	ret
1:
	//incw	nt_file_count

//	xorb	%ch, %ch
	xorw %cx, %cx

	pushw	%si
	leaw	(nt_boot_image - Entry_nt)(%bp), %si
	addw	$0x52, %bx		// The value at 0xA(%bx) is wrong sometimes (0x4C)
	movb	-2(%bx), %cl
1:
	lodsb	(%si), %al
	movb	(%bx), %ah
//	cmpb	$'A', %ah
	cmpb	$'a', %ah
	jb	2f
//	cmpb	$'Z', %ah
	cmpb	$'z', %ah
	ja	2f
//	addb	$('a'-'A'), %ah		// Convert to lowercase
	subb	$('a'-'A'), %ah		// Convert to Switch
2:

	cmpb	%ah, %al
	jnz	3f			// Not match

	incw	%bx
	incw	%bx
	loop	1b

	cmpb	$0,(%si)
	jnz	3f

	popw	%si
	clc
	ret				// Match found

3:

	popw	%si
	addw	8(%si), %si

	jmp	ntfs_find_grldr

// Locate an attribute
// Input:
//     DI - pointer to buffer
//     AL - attribute
ntfs_locate_attr:
	call	ntfs_init_attr
	call	ntfs_find_attr
	jc	1f
2:
	testb	$NT_FG_ALST, nt_flag
	jnz	2f
	call	ntfs_find_attr
	jnc	2b
	call	ntfs_init_attr
	call	ntfs_find_attr
2:
	clc
1:
	ret

// Prepare to find attribute
// Input:
//     DI - pointer to buffer
ntfs_init_attr:
	pushw	%ax
	xorw	%ax, %ax
	movw	%ax, nt_flag
	movw	%ax, nt_attr_end
	movw	nt_attr_ofs, %ax
	addw	%di, %ax
	movw	%ax, nt_attr_nxt
	popw	%ax
	cmpw	$MMFT_BASE, %di
	jnz	1f
	orb	$NT_FG_MMFT, nt_flag
1:
	ret

// Find an attribute
// Input:
//     DI - pointer to buffer
//     AL - attribute
// Output:
//     SI - current item
//     CF - status
ntfs_find_attr:
	movw	nt_attr_nxt, %bx
	testb	$NT_FG_ALST, nt_flag
	jnz	6f
1:
	movw	%bx, %si
	cmpb	$0xFF, (%si)
	jz	3f

	cmpb	$AT_ATTRIBUTE_LIST, (%si)
	jnz	2f
	movw	%si, nt_attr_end
2:
	addw	4(%bx), %bx
	cmpb	%al, (%si)
	jnz	1b
	movw	%bx, nt_attr_nxt
	movw	%si, nt_attr_cur
2:
	ret
3:
	cmpw	$1, nt_attr_end
	jb	2b
	movw	nt_attr_end, %si
	cmpb	$0, 8(%si)
	jnz	4f
	movw	%si, %bx
	addw	0x14(%bx), %bx
	addw	4(%si), %si
	jmp	5f
4:
	movl	0x28(%si), %ecx
	shrl	$9, %ecx
	cmpw	$8, %cx
	ja	NTFS_Corrupt_Error
	leaw	nt_edat_buf, %bx
	pushw	%ax
	xorl	%edx, %edx
	call	ntfs_read_data
	popw	%ax
	jc	2b
	movw	0x30(%si), %si
	addw	%bx, %si
5:
	movw	%si, nt_attr_end
	orb	$NT_FG_ALST, nt_flag
	testb	$NT_FG_MMFT, nt_flag
	jz	6f
	call	ntfs_fix_mmft
6:
	movw	%bx, %si
	cmpw	nt_attr_end, %bx
	jb	1f
7:
	stc
	ret
1:
	addw	4(%bx), %bx
	cmpb	%al, (%si)
	jnz	6b

	pushw	%ax
	pushw	%es
	pushw	%ds
	popw	%es
	movw	%si, nt_attr_cur
	movw	%bx, nt_attr_nxt
	movl	0x10(%si), %eax
	leaw	nt_emft_buf, %bx
	testb	$NT_FG_MMFT, nt_flag
	jnz	2f
	call	ntfs_read_mft
	jmp	3f
2:
	pushw	%bx
	call	readDisk_nt
	movl	0x14(%si), %eax
	call	readDisk_nt
	popw	%bx
	cmpw	$0x4946, (%bx)			// "FI"
	jnz	NTFS_Corrupt_Error
	movw	$2, %cx
	call	ntfs_fixup
3:
	popw	%es
	popw	%ax
	addw	0x14(%bx), %bx
4:
	cmpb	$0xFF, (%bx)
	jz	7b
	cmpb	%al, (%bx)
	jz	5f
	addw	4(%bx), %bx
	jmp	4b
5:
	movw	%bx, %si
	ret

// Fix $MFT
// Input:
//     DI - pointer to buffer
//     BX - attr cur
ntfs_fix_mmft:
	pushw	%ax
	orb	$NT_FG_GPOS, nt_flag

1:
	cmpw	nt_attr_end, %bx
	jae	NTFS_Corrupt_Error
	cmpb	%al, (%bx)
	jz	2f
	addw	4(%bx), %bx
	jmp	1b
2:

	movw	%bx, nt_attr_cur

	movl	nt_mft_start, %eax
	movl	%eax, 0x10(%bx)
	incl	%eax
	movl	%eax, 0x14(%bx)
1:
	addw	4(%bx), %bx

	cmpw	nt_attr_end, %bx
	jae	2f
	cmpb	$AT_DATA, (%bx)
	jnz	2f

	movl	0x10(%bx), %edx
	movb	nt_mft_size, %cl
	shll	%cl, %edx

	call	ntfs_read_attr

	orl	%eax, %eax
	jz	NTFS_Corrupt_Error
	movl	%eax, 0x10(%bx)
	movl	%edx, 0x14(%bx)
	jmp	1b
2:
	movw	nt_attr_cur, %bx
	andb	$(~NT_FG_GPOS), nt_flag
	popw	%ax

	ret

// Read MFT record
// Input:
//     DS:BX - buffer
//     EAX - mft number
ntfs_read_mft:
	pushw	%di
	movw	$MMFT_BASE, %di
	movb	nt_mft_size, %cl
	shll	%cl, %eax
	movl	%eax, %edx
	movl	$1, %eax
	shll	%cl, %eax
	movl	%eax, %ecx
	call	ntfs_read_attr
	jc	NTFS_Corrupt_Error
	cmpw	$0x4946, (%bx)			// "FI"
	jnz	NTFS_Corrupt_Error
	call	ntfs_fixup
	popw	%di
	ret

// Read attribute
// Input:
//     DI - pointer to buffer
//     ES:BX - buffer
//     EDX - start sector
//     ECX - sector count
// Output:
//     CF - status
ntfs_read_attr:
	pushw	nt_attr_cur
	pushl	%edx
	pushl	%ecx
	pushw	%bx

	movw	nt_attr_cur, %si
	movb	(%si), %al

	testb	$NT_FG_ALST, nt_flag
	jz	2f
	movw	%si, %bx
	movb	nt_spc, %cl
	shrl	%cl, %edx

1:
	cmpw	nt_attr_end, %bx
	jae	2f
	cmpb	%al, (%bx)
	jnz	2f
	cmpl	%edx, 8(%bx)
	ja	2f
	movw	%bx, %si
	addw	4(%bx), %bx
	jmp	1b
2:

	movw	%si, nt_attr_nxt
	call	ntfs_find_attr

	popw	%bx
	popl	%ecx
	popl	%edx
	jc	1f
	call	ntfs_read_data
	clc
1:
	popw	nt_attr_cur
	ret

// Read data
// Input:
//     DI: pointer to buffer
//     SI: current item
//     ES:BX: buffer
//     EDX: start sector
//     ECX: sector count
ntfs_read_data:
	pushw	%cx
	pushw	%bx
	testb	$1, 8(%si)
	jz	NTFS_Corrupt_Error
	movb	0xC(%si), %al
	andb	$1, %al
	orb	%al, nt_flag

	movl	%ecx, nt_read_count
	movb	nt_spc, %cl

	movl	%edx, %eax
	shrl	%cl, %eax
	movl	%eax, nt_target_vcn
	shll	%cl, %eax
	subl	%eax, %edx
	movl	%edx, nt_vcn_offset

	xorw	%dx, %dx		// edx - next VCN
	movl	%edx, nt_curr_lcn

	movl	0x10(%si), %edx

	addw	0x20(%si), %si
1:
	call	ntfs_runlist_read_block

	cmpl	nt_target_vcn, %edx
	jbe	1b
1:
	movb	nt_spc, %cl

	orl	%eax, %eax		// sparse
	jz	2f

	movl	nt_target_vcn, %eax
	subl	nt_curr_vcn, %eax
	addl	nt_curr_lcn, %eax

	shll	%cl, %eax
	addl	nt_vcn_offset, %eax

	testb	$NT_FG_GPOS, nt_flag
	jz	3f
	pushl	%eax
	incl	%eax
	subl	nt_curr_vcn, %edx
	addl	nt_curr_lcn, %edx
	shll	%cl, %edx
	cmpl	%eax, %edx
	jnz	4f
	pushw	%cx
	call	ntfs_runlist_read_block
	popw	%cx
	movl	nt_curr_lcn, %eax
	shll	%cl, %eax
4:
	movl	%eax, %edx
	popl	%eax
	addl	(nt_part_ofs - Entry_nt)(%bp), %edx
3:

	addl	(nt_part_ofs - Entry_nt)(%bp), %eax

2:
	testb	$NT_FG_GPOS, nt_flag
	jnz	1f

	pushl	%ebx
	movl	%edx, %ebx
	subl	nt_target_vcn, %ebx
	shll	%cl, %ebx
	movl	%ebx, %ecx
	popl	%ebx

	subl	nt_vcn_offset, %ecx
	movl	$0, nt_vcn_offset
	cmpl	nt_read_count, %ecx
	jbe	2f
	movl	nt_read_count, %ecx
2:

	pushl	%ecx

	orl	%eax, %eax
	jnz	3f
	call	ntfs_sparse_block
	jmp	4f

3:
	call	readDisk_nt
	loop	3b

4:
	popl	%ecx
	subl	%ecx, nt_read_count
	jbe	1f

	movl	%edx, nt_target_vcn
	call	ntfs_runlist_read_block
	jmp	1b

1:
	popw	%bx
	popw	%cx
	ret

// Read run list data
// Input:
//     CL = number of bytes
// Output:
//     EAX = read bytes
//     SI points to the next unhandled byte

ntfs_runlist_read_data:
#if 0
	pushw	%cx
	orb	%cl, %cl
	jnz	1f
	popw	%cx
	xorl	%eax, %eax
	ret
1:
	lodsb	(%si), %al
	rorl	$8, %eax
	decb	%cl
	jnz	1b

	popw	%cx
	negb	%cl
	add	$4, %cl
	shlb	$3, %cl
	ret
#else
	xorl	%eax, %eax
	testb	%cl, %cl
	jz	2f
	pushw	%cx
1:
	lodsb	(%si), %al
	rorl	$8, %eax
	decb	%cl
	jnz	1b

	popw	%cx
	negb	%cl
	add	$4, %cl
	shlb	$3, %cl
2:
	ret
#endif

NTFS_Run_Overflow_Error:
	movb	$NTFS_Run_Overflow_Error_Code, %al
	jmp	NTFS_Error

// Read run list block
// Output:
//     EDX = Next VCN
//     SI points to the next unhandled byte

ntfs_runlist_read_block:
	lodsb	(%si), %al
	movb	%al, %cl
	movb	%cl, %ch
	andb	$0xF, %cl		// cl - Size of length field
	jz	1f
	shrb	$0x4, %ch		// ch - Size of offset field

	call	ntfs_runlist_read_data
	shrl	%cl, %eax

	movl	%edx, nt_curr_vcn
	addl	%eax, %edx

	movb	%ch, %cl
	call	ntfs_runlist_read_data
	sarl	%cl, %eax

	addl	%eax, nt_curr_lcn

	ret

1:
	testb	$NT_FG_ALST, nt_flag
	jz	NTFS_Run_Overflow_Error

	pushl	%edx
	pushw	%bx
	movw	nt_attr_cur, %si
	movb	(%si), %al
	call	ntfs_find_attr
	jc	NTFS_Run_Overflow_Error
	cmpb	$0, 8(%si)
	jz	NTFS_Run_Overflow_Error
	movl	$0, nt_curr_lcn
	popw	%bx
	popl	%edx
	addw	0x20(%si), %si
	jmp	ntfs_runlist_read_block

// Convert seg:ofs to linear address
// Input:
//     On stack: seg:ofs
// Output:
//     eax:
seg_to_lin:
	pushw	%bp
	movw	%sp, %bp
	xorl	%eax, %eax
	xchgw	6(%bp), %ax
	shll	$4, %eax
	addl	4(%bp), %eax
	popw	%bp
	ret	$4

// Convert linear address to seg:ofs
// Input:
//     on stack: linear address
// Output:
//     On stack: seg:ofs
lin_to_seg:
	pushw	%bp
	movw	%sp, %bp
	shll	$12, 4(%bp)
	shrw	$12, 4(%bp)
	popw	%bp
	ret

fix_segs:
	pushw	%ds
	pushw	%si
	call	seg_to_lin
	pushl	%eax
	call	lin_to_seg
	popw	%si
	popw	%ds

fix_es_di:
	pushw	%es
	pushw	%di
	call	seg_to_lin
	pushl	%eax
	call	lin_to_seg
	popw	%di
	popw	%es
	ret

// Handle sparse block
//     DI: points to buffer
//     ES:BX: points to buffer
//     ECX: number of sectors
//     EDX: next VCN

ntfs_sparse_block:
	pushw	%di
	pushl	%edx

	shll	$9, %ecx		// ecx - totel number of bytes

	testb	$1, nt_flag		// Not compressed
	jz	2f

	xorl	%edx, %edx
	movb	nt_target_vcn, %dl
	andb	$0xF, %dl
	jz	2f

	movw	%bx, %di

	pushw	%cx

	movb	nt_spc, %cl
	addb	$9, %cl
	shll	%cl, %edx		// edx: offset from the start of cluster

	push	%es
	push	%di
	call	seg_to_lin
	subl	%edx, %eax		// eax: linear address

	movl	$16, nt_remain_len
	shll	%cl, nt_remain_len

	popw	%cx

	addl	%edx, %ecx
	subl	nt_remain_len, %ecx

	pushl	%ecx
	call	ntfs_decomp_block
	popl	%ecx

	addl	nt_remain_len, %ecx

	jecxz	1f

	movw	%di, %bx

2:
	movw	%bx, %di
	movl	%ecx, %edx
	xorl	%eax, %eax
	movl	%eax, %ecx
	call	fix_es_di

3:
	movw	$0x8000, %cx
	cmpl	%edx, %ecx
	jbe	4f
	movw	%dx, %cx
4:
	pushw	%cx
	shrw	$2, %cx

	rep	stosl	%eax, %es:(%di)
	call	fix_es_di
	popw	%cx
	subl	%ecx, %edx
	jnz	3b

1:
	movw	%di, %bx

	popl	%edx
	popw	%di

	ret

// Decompress block
// Input:
//     eax: linear address at the beginning of the compressed block
// Output:
//     ES:DI: points to the end of the block
ntfs_decomp_block:
	pushw	%ds
	pushw	%si

	cld		# added 2009-10-06 by tinybit
	pushl	%eax
	call	lin_to_seg
	popw	%si
	popw	%ds
	movl	nt_remain_len, %edx
	addl	%edx, %eax
	pushl	%eax
	call	lin_to_seg
	popw	%di
	popw	%es

	pushw	%es
	pushw	%di
	pushw	%ds
	pushw	%si

	xorl	%ecx, %ecx

1:
	movw	$0x8000, %cx
	cmpl	%edx, %ecx
	jbe	2f
	movw	%dx, %cx
2:
	pushw	%cx
	shrw	$2, %cx
	rep	movsl	(%si), %es:(%di)
	call	fix_segs
	popw	%cx
	subl	%ecx, %edx
	jnz	1b

	popw	%di	# Why these POPs should be in reverse order?
	popw	%es	# Seems a comment needed.  -- tinybit
	popw	%si	#
	popw	%ds	#

1:
	xorl	%edx, %edx			// edx - copied bytes

	lodsw	(%si), %ax
	testb	$0x80, %ah
	jnz	2f
	movw	$0x800, %cx
	rep	movsw	(%si), %es:(%di)
	movw	$0x1000, %dx
	jmp	7f				// The block is not compressed

2:
	movw	%ax, %cx
	andw	$0xFFF, %cx
	incw	%cx				// ecx = block length
	addw	%si, %cx			// cx: end marker
	xorb	%bh, %bh

3:
	cmpw	$0x1000, %dx
	ja	NTFS_Decompress_Error

	orb	%bh, %bh
	jnz	4f
	lodsb	(%si), %al
	movb	%al, %bl			// bl: tag, bh: count
	movb	$8, %bh
4:

	testb	$1, %bl
	jz	5f

	movw	%dx, %ax
	decw	%ax

	pushw	%cx
	pushw	%bx

	movb	$12, %cl
6:
	cmpw	$0x10, %ax
	jb	6f
	shrw	$1, %ax
	decb	%cl
	jmp	6b
6:

	lodsw	(%si), %ax
	movw	%ax, %bx
	shrw	%cl, %bx			// bx: delta

	pushw	%dx
	movw	$1, %dx
	shlw	%cl, %dx
	decw	%dx
	andw	%dx, %ax
	popw	%dx

	addw	$3, %ax
	movw	%ax, %cx			// cx: length
	negw	%bx
	decw	%bx

6:
	movb	%es:(%bx, %di), %al
	stosb	%al, %es:(%di)
	incw	%dx
	loop	6b

	popw	%bx
	popw	%cx
	jmp	4f

5:
	movsb	(%si), %es:(%di)
	incw	%dx
4:
	shrb	$1, %bl
	decb	%bh

	cmpw	%cx, %si
	jb	3b

7:
	call	fix_segs

	subl	%edx, nt_remain_len	// End of block
	jz	1f

	cmpw	$0x1000, %dx
	je	1b

1:

	popw	%si
	popw	%ds
	ret

NTFS_Decompress_Error:
	pushw	%ss
	popw	%ds
	movb	$NTFS_Decompress_Error_Code, %al
	jmp	NTFS_Error

/*
do_pause:
	.byte	0

pause:
	cmpb	$0, (do_pause - Entry_nt)(%bp)
	jnz	1f
	ret
1:
	xorw	%bp, %bp
1:
	jmp	1b
*/

/*
hex_out:
	pushw	%bp
	movw	%sp, %bp
	pushaw
	movb	$0xE, %ah
 	movw	$7, %bx
	movw	$4, %cx
	movw	4(%bp), %dx
1:
	rol	$4, %dx
	movb	%dl, %al
	andb	$0xF, %al
	cmpb	$10, %al
	jb	2f
	subb	$('0'-'A'+10), %al
2:
	addb	$'0', %al
	int	$0x10
	loop	1b
	movb	$' ', %al
	int	$0x10
	popaw
	popw	%bp
	ret	$2
*/




	. = Entry_nt + 0x7fc
	
                                                      
nt_sector_mark:
	.long	0x42555247		// "GRUB" 
	
//----------------- end  of "ntfsbs.S"-----------------------  

/* ********************  ntfs_dbr_mbr end  ****************************** */



#if (! defined(GRLDR_INSTALL))

/* *************** FAT12_16_FAT32_exFAT_EXT2_3_4_mbr start ****************** */


 

#define BOOTGRUB	/* undef this if compiled for loading FreeDOS */

#define LOADSEG         0x2000
#define FATSEG		0x0060



/*      FAT12/16/32/exFAT/EXT2 combined
        exFAT boot code provided by the Fan JianYe
        
        FAT12/16/32/exFAT Current BPB Structure:
        Offset  Type    Description
        --------------------------------------------------------------------
        00      Word    EB 2E   Jump instruction
        02      Byte    Partition type / EIOS Mark
                        Bit 0   FAT12
                        Bit 1   FAT16
                        Bit 2   FAT32
                        Bit 3   exFAT
                        Bit 4   EXT2
                        Bit 5   Retention
                        Bit 6   EXT2  filesystem size of 2^64 blocks
                        Bit 7   EIOS
        03      DWord   Root directory
        07      DWord   Home directory of the absolute starting sector (fat12/16)
        0b      Word    Bytes per sector
        0d      byte    Sectors per cluster (ext2)
        0e      DWord   Data start Absolute sector 
        12      DWord   The current cluster the absolute sector in fat table
        16      Word    12 64
        18      Word    Sectors per track
        1a      Word    Number of heads
        1c      DWord   Partition start Absolute sector
        20      DWord   The total number of sectors partition
        24      Byte    Drive number
        25      Byte    Partition number
        26      Word    Sectors per cluster	(fat12/16/32/64)
        28      DWord   FAT table start absolute sector number
        2c      DWord   Root cluster  
 */        
                        
Entry_12_64_ext:                                    /* cs=ds=es=0000 ss=2000 */

        cli
	cld
	xorw	%ax, %ax
	movw	$0x7c00, %bp                    /* the base address */ 
        movw	%ax, %ss	                /* stack and BP-relative moves up, too */
        movw    %bp, %sp                        /* ss:sp=0000:7c00 */
	movw	%ax, %ds
	movw	%ax, %fs
	sti
        /* Save the drive letter */	
        /* Read all the root directory to 2000:0000 */
        lesw	(loadseg_off_12_64_ext - Entry_12_64_ext + 0x30)(%bp), %bx	/* load to loadseg:0 */ 
        testb   $0x10, 02(%bp)                                  /* ext2? */          
        jne     Entry_ext2_mbr                                  /* yes */ 
				movw		$0x6412, 0x16(%bp)															//12 64
        movb    $0xff, 0x12(%bp)                                /* The current cluster  the absolute sector in fat table */   //The current cluster the absolute sector in fat table    
        testb   $1, 02(%bp)                                     /* fat12? */          
        jne 1f                                                  /* yes */
        testb   $2, 02(%bp)                                     /* fat16? */
        jne 1f                                                  /* yes */
        /* Read fat32/exfat root directory */
        movl    0x2c(%bp), %eax                                 /* Root clusters */
        call    cluster_to_lba_12_64_ext                        /* Calculate the absolute sector from the cluster */
        movl    %eax, 03(%bp)
7:        
        call    read_2_sectors_12_64_ext                        /* Read all the root directory to 2000:0000 */
        jb      disk_error_12_64_ext                            /* Read error display 'disk error'  */
        jmp     2f                                              /* ok */
1:      /* Read fat12/16 Home directory */
        movl    0x07(%bp), %eax                                 /* Home directory of the absolute starting sector (fat12/16) */
        movl    0x0e(%bp), %ecx                                 /* Data start Absolute sector */
        subl    %eax, %ecx                                      /* sectors  number of  the main directory */
        pushw   %es
        call    readDisk_12_64_ext                              /* Read all the Home directory to 2000:0000 */
        popw    %es
        jb      disk_error_12_64_ext                            /* Read error display 'disk error'  */
2:      /* Search file GRLDR */
        testb   $8, 02(%bp)                                     /* exfat? */
        jne 3f                                                  /* yes */
        /* in fat12/16/32 Search file GRLDR */
        movw    $0xffe0, %di            /* di initialization  ffe0+20=0000 */
1:      addw    $0x20, %di              /* Next Directory */
        andw    $0xffe0, %di            /* Adjusted to 20,40,60, ... */
        testb   $0x04, 02(%bp)          /* fat32? */
        je      2f                      /* not */
        cmpw    0x0b(%bp), %di          /* Bytes per sector*/
        jnb     7b 
2:          
        cmpb    $0, %es:(%di)           /* Contents end? */
        je      grldr_error_12_64_ext   /* yes */ 
        movw    $0xb, %cx               /* More bytes*/
        movw    $(filename_12_64_ext - Entry_12_64_ext + 0x7c30), %si      /*'GRLDR' First place */         
				repz    cmpsb                   /* More 'GRLDR' */
        jne     1b                      /* NO,Restore directory entry, check the next entry */

        /* GRLDR has been found */
        movw    %es:0x11(%di), %ax      /* 16-bit low byte file */
        movw    %es:0x13(%di), %dx      /* 16-bit high byte file */
        divw    0x0b(%bp)               /* ax = file length / bytes per sector = file number of sectors */             
        incw    %ax                     /* May be more than the number, file number of sectors + 1 */
        xchgw   %ax, %cx        
        pushw	%es:0x9(%di)            /* File the initial cluster of high 16-bit */
        pushw	%es:0xf(%di)            /* File the initial cluster of low 16-bit */
        popl	%eax                    /* File the initial cluster 0f 32-bit */
4:      //xorw    %bx, %bx                /* Target offset */
        call    Read_Discontinuous_12_64_ext      /* Read file GRLDR to 2000:0000 */
5:      movw    0x24(%bp), %dx          /* dh=Partition number (starting from 0)  dl=Drive number */
        pushw   %dx                     /* Application just to grub.exe */
        ljmp    $LOADSEG,$0             /* Go to 2000:0000 */


3:      /* in exfat Search file GRLDR */
        movw    $0xffe0, %di            /* di initialization  ffe0+20=0000 */
6:      /* Determine the entry type */
        addw    $0x20, %di              /* Next Directory */
        cmpw    0x0b(%bp), %di          /* Bytes per sector*/
        jnb     7b         
1:        
        cmpb    $0, %es:(%di)           /* Contents end? */
        je      grldr_error_12_64_ext   /* yes */   
        cmpb    $0xc0, %es:(%di)        /* Stream Directory */
        jne     6b                      /* Not */
        /* Determine the length of the file name */
//        cmpb    $5, %es:3(%di)          /* File name length is equal to 5? */
//        jne     6b                      /* NO, Check next entry */
				movzbw  %es:3(%di), %cx        	/* File name length */
        pushw   %di                     /* Save the current directory entry */
        addw    $0x22, %di              /* Filename address */                                                   
//        movw    $5, %cx                 /* More bytes */
        movw    $(grldr_un_12_64_ext - Entry_12_64_ext + 0x7c30), %si /*'GRLDR' First place */
1:      
				cmpb $0x61, %es:(%di)
				jb 101f
				cmpb $0x7a, %es:(%di)
				ja 101f
        andb $0xdf, %es:(%di)        		/* lowercase to lowercase */
101:        
				cmpsb
        jne     1f                      /* NO,Restore directory entry, check the next entry */
        incw    %di
        loop    1b                      /* The same, compare the next character */
				cmpb $0, (%si)
				jne 1f
        /* GRLDR has been found */
        popw	%di                     /* Restore directory entry */
        /* files stored continuously, Calculate the number of sectors the file */
        movw    %es:0x18(%di), %ax      /* 16-bit low byte file */
        movw    %es:0x1a(%di), %dx      /* 16-bit high byte file */
        divw    0x0b(%bp)               /* ax = file length / bytes per sector = file number of sectors */             
        incw    %ax                     /* May be more than the number, file number of sectors + 1 */ 
        pushw   %ax                     /* File number of sectors */
        testb	$2, %es:1(%di)	        /* Bit 2 is equal to 1 indicates no FAT chain, files stored continuously */
        je 2f                           /* File storage is not continuous */
        /* Read file to 2000:0000 */        
        movl    %es:0x14(%di), %eax     /* Starting cluster file */
        call    cluster_to_lba_12_64_ext/* Calculate the absolute sector from the cluster */
        xorw    %bx, %bx                /* Target offset */
        popw	%cx                     /* File number of sectors */
        call    readDisk_12_64_ext      /* Read file GRLDR to 2000:0000 */
        jmp     5b
                

        
1:      
        /* Restore directory entry, check the next entry */
        popw	%di                     /* NO,Restore directory entry */
        jmp     6b                      /* Check the next entry */                                                   
                         
2:      
        /* File storage is not continuous, read the file to 2000:0000 */
        popw    %cx
        movl    %es:0x14(%di), %eax     /* Starting cluster file */
        jmp     4b


        
/* Calculate the absolute sector from the cluster
 * input:       eax=Cluster number
 * output:      eax=Absolute sector
 */
cluster_to_lba_12_64_ext:
	/* sector = (cluster-2) * clustersize + data_start */
	decl	%eax
	decl	%eax 
	movzwl	0x26(%bp), %edx		/* sectors per cluster */
	mull	%edx			/* EDX = 0 */
        addl	0x0e(%bp), %eax         /* Absolute sector */  /* Data start Absolute sector */
	ret


grldr_error_12_64_ext:
        movw	$(msg_BootError_12_64_ext - Entry_12_64_ext + 0x7c30), %si
        jmp     printr_12_64_ext        
disk_error_12_64_ext:
	movw	$(msg_DiskReadError_12_64_ext - Entry_12_64_ext + 0x7c30), %si
	        
        
printr_12_64_ext:
1:
	lodsb	(%si), %al	/* get token */
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	cmpb	$0, %al		/* end of string? */
	jne	1b		/* until done */

        ljmp    $0, $MONITOR    /* 0:0x8410 */

                
loadseg_off_12_64_ext:
	.word	0
	.word	LOADSEG
	 	
FAT_Address_12_64_ext:
        .word	0
	.word	FATSEG


/* Read a sector from disk, using LBA or CHS
 * input:	EAX - 32-bit DOS sector number
 *		ES:BX - destination buffer
 *              CX -  Read number of sectors
 * output:	ES:BX - next destination buffer
 *		EAX - next sector
 * warning:     EAX, ES Has changed
 */
readDisk_12_64_ext:
3:
	pushal
	xorl	%edx, %edx	/* EDX:EAX = LBA */
	pushl	%edx		/* hi 32bit of sector number */
	pushl	%eax		/* lo 32bit of sector number */
	pushw	%es		/* buffer segment */
	pushw	%bx		/* buffer offset */
	pushw	$1		/* 1 sector to read */
	pushw	$16		/* size of this parameter block */
        movw	%sp, %si	        /* DS:SI points to disk address packet */
        testb   $0x80, 2(%bp)           /* Support the expansion of reading? */
        jne     1f                      /* yes */
        /* C/H/S mode read */ 
        movzwl  0x18(%bp), %ecx /* sectors per track */
	divl	%ecx		/* Tracks is in EAX,Sectors in EDX */
	incw	%dx		/* sector number in DL */
	pushw	%dx		/* push sector number into stack */
				/* quotient is in EAX, cylinder number */        
        pushl	%eax            /* eax High 16 move to dx */
        popw	%ax     
        popw	%dx
        divw    0x1a(%bp)       /* ax=Tracks/Heads=cylinder number, dx=Head number */	
	xchgb	%dl, %dh	/* head number should be in DH,DL=0 */
	popw	%cx		/* pop sector number from stack */
	xchgb	%al, %ch	/* lo 8bit cylinder should be in CH,AL=0 */ 
        rorb	$2, %ah	        /* hi 2bit cylinder number moble from 0,1 to 6,7 bit */
	orb	%ah, %cl	/* hi 2bit cylinder number (7,6) and the sector number (5-0) are combined */	
	movw	$0x201, %ax	/* read 1 sector */
	jmp     2f
1:	movb    $0x42, %ah      /* Extended Mode Read */
2:      movb	0x24(%bp), %dl  /* hard disk drive number */
	pushw	%ds
	pushw	%es
	int	$0x13
	popw	%bx
	popw	%ds
	pushfw
	leaw	0x20(%bx), %bx
	movw	%bx, %es
	popfw	
	popaw								/* remove parameter block from stack */
	popal
        jc      2f              /* disk read error */
        incl 	%eax	        /* next Absolute logical sector */
        loop    3b
        clc                     /* Read complete */
2:	ret  



/* Read 2 sectors
 * First read sector from the current cluster, 
 * followed by reading from the cluster of sectors. 
 * Encountered cluster terminator stop.
 * input:       es:bx=Buffer   03(%bp)=The first absolutely logical sector
 * output:      0x2c(%bp)=current cluster   03(%bp)=The second absolutely logical sector
 */
read_2_sectors_12_64_ext:
        pushw   %es
        movl    03(%bp), %eax 
        movw    $1, %cx                         /* Number of sectors read */    
        call    readDisk_12_64_ext              /* Read sectors */
        jb      4f
        pushl   %eax                            /* The second absolutely logical sector */
        subl    0x0e(%bp), %eax                 /* The first relatively logical sector */  /* Data start Absolute sector */
        xorl    %edx, %edx
        movzwl  0x26(%bp), %ecx                 /* sectors per cluster */
        divl    %ecx                            /* eax=Cluster number  edx=Sector offset */
        cmpw    0x26(%bp), %dx                  /* In the current cluster? */
        /* In the next cluster */
        popl    %eax
        jb 3f																		/* Yes */	
        movl    0x2c(%bp), %eax                 /* current cluster */
        call    NextCluster_12_64_ext           /* Find the next cluster from the current cluster */
        movl    %eax, 0x2c(%bp)                 /* current cluster */
        jb      1f                              /* Not the end of the cluster */ 
                
        xorw    %ax, %ax
        xorw    %di, %di
        xorl    %esi, %esi
        movw    $0x100, %cx        
        repz    stosw
        jmp     2f
1:        
        call    cluster_to_lba_12_64_ext        /* Calculate the absolute sector from the cluster */
        jmp     1f        
3:  
        /* In the current cluster */ 
1:        
        movl    %eax, 03(%bp)  
        movw    $1, %cx                         /* Number of sectors read */
        call    readDisk_12_64_ext              /* Read sectors */
        jnb     2f        
4:      stc        
2:
        popw    %es                
        ret        


                
 
/* Read sectors from the cluster 
 * Input:       eax=Cluster number, es:bx=buffer, cx=File number of sectors 
 * Warning:     EAX Has changed
 */      
Read_Discontinuous_12_64_ext:
        pushw   %es
        pushw   %di
        movw 0x26(%bp), %di
1:             
        pushl	%eax                    /* Cluster number */
        call    cluster_to_lba_12_64_ext/* Calculate the absolute sector from the cluster */
        cmpw    %di, %cx
        jb      2f                                                                                 
        subw    %di, %cx
        pushw   %cx
        movw    %di, %cx                /* sectors per cluster */
        jmp     3f
2:                
        pushw   $0               
3:
        call    readDisk_12_64_ext      /* Read Sector */ 
        popw    %cx
        popl    %eax                    /* Cluster number */
        jb      1f                      /* Reading failure or end */
        orw     %cx, %cx
        je      1f 
        pushw   %cx
        call    NextCluster_12_64_ext   /* Find the next cluster from the current cluster */
        popw    %cx
        jb      1b                      /* Not the end of the cluster, continue to read */              
1:              
        popw    %di
        popw    %es
        ret



/* Find the next cluster from the current cluster */
/* Input: eax=current cluster number */
/* Otput: eax=next cluster number, Less than=Normal */
/* [bp+2d]=The current cluster in the absolute sector fat table */
NextCluster_12_64_ext:    
        pushw   %bx
        pushw   %es
        lesw	(FAT_Address_12_64_ext - Entry_12_64_ext + 0x30)(%bp), %bx    /* es:bx=0060:0000 */
        /* Computing bytes per cluster */
        testb   $1, 02(%bp)                                     /* fat12? */
        jne     3f                                              /* yes */ 
        testb   $2, 02(%bp)                                     /* fat16? */
        jne     2f                                              /* yes */
        /* fat32/exfat Computing bytes per cluster */
        shll    $2, %eax                                        /* 4 bytes per cluster */     
        jmp     1f                               
2:      
        /* fat16 Computing bytes per cluster */
        shlw    $1, %ax                                         /* 2 bytes per cluster */ 
        jmp     1f                
3:              
        /* fat12 Computing bytes per cluster */
        imulw   $3, %ax
        shrw    $1, %ax                                         /* 1.5 bytes per cluster */ 
        pushfw
                
1:              
        /* Computing cluster in the FAT table sector offset / byte offset */ 
        xorl    %edx, %edx
        movzwl  0x0b(%bp), %ecx                                 /* Bytes per sector */
        divl    %ecx                                            /* eax=sector offset, edx=byte offset */      
        /* Calculate the absolute sector of clusters */
        addl    0x28(%bp), %eax                                 /* FAT table start absolute sector number */
        pushw   %dx                                             /* byte offset */
        /* Determine whether or not to read the fat table */
        cmpl    %eax, 0x12(%bp)																	//The current cluster the absolute sector in fat table
        je      1f                                              /* Does not read the fat table */
        /* read the fat table */
        movl    %eax, 0x12(%bp)                                 /* Save the current address of fat table */  //The current cluster the absolute sector in fat table                                                      
        movw    $2, %cx                                         /* Read 2 sector */
        pushw   %es
        call    readDisk_12_64_ext                              /* Reading FAT table */
        popw    %es
1:             
        popw	%bx                                             /* byte offset */
        movzwl  %es:(%bx), %eax                                 /* fat12/16 Next cluster number */
        testb   $1, 02(%bp)                                     /* fat12? */
        jne     3f                                              /* yes */
        testb   $2, 02(%bp)                                     /* fat16? */
        jne     2f                                              /* yes */
        movl    %es:(%bx), %eax                                 /* fat32/exfat Next cluster number */
        testb   $4, 02(%bp)                                     /* fat32? */
        jne 1f                                                  /* yes */
        /* exfat Determine terminator */
        cmpl    $0xfffffff8, %eax
        jmp     4f                                
1:              
        /* fat32 Determine terminator */
        cmpl    $0x0ffffff8, %eax
        jmp     4f                 
2:              
        /* fat16 Determine terminator */ 
        cmpw    $0xfff8, %ax
        jmp     4f                
3:              
        /* fat12 Determine terminator */
        popfw                           /* 0f ff ff 12 34 78   ax=3412(No remainder) or ax=7834(remainder) */
        jnb     1f                      /* No remainder */
        shrw    $4, %ax                 /* 7834/10=0783 */               
1:              
        andb    $0xf, %ah               /* ax=3412->0412 or  ax=0783->0783 */
        cmpw    $0x0ff8, %ax  
4:             
        popw	%es 
        popw	%bx
        ret


Entry_ext2_mbr:

	pushw	%ax		/* 0x0000 at 0000:7bfe */ 
	movw	$0x1000, %bx
	pushw	%bx		/* 0x1000 at 0000:7bfc */
	pushw	%ax		/* 0x0000 at 0000:7bfa */
				/* SP=0x7bfa */ 		
				

	/* the 6 bytes in the stack are used by read_block():
	 *	0000	----	-2(%bp)
	 *	1000	----	-4(%bp)
	 *	0000	----	-6(%bp)
	 * Don't touch them!'
	 */

	xorl	%eax, %eax	/* CF=0, ZF=1 */


	/* MOV keeps all flags untouched, so it is better than INC */
	movb	$2, %al		/* EAX=2=inode number for root dir */

	/* CF=0, ZF=1 because MOV and PUSH do not touch Flags */

	/* read root dir to 0000:1000, and grldr to 1000:0000 */

4:
	/* EAX holds the inode number: for root dir or grldr */

	/* These 3 PUSHes is intended to place 1000:0000 onto the stack for
	 * grldr. For root dir, the stack is not used since CF is cleared.
	 * Although there is no corresponding POPs, this is safe enough
	 * because the program comes here only twice: the first is for
	 * the root dir, and the second is for grldr.
	 *
	 * For root dir, CF=0 and ZF=1. For grldr, CF=1.
	 */
	movw	$0x1000, %di
	pushw	%di		/* 0x1000, see "jz 4b" below. */
	pushw	%ss		/* 0x0000 */ 
	pushfw

	/* SP=0x7bf4 for root dir, or 0x7bee for grldr */

	decl	%eax		/* EAX=(inode - 1) */

	/* inode numbers are far less than 0x7fffffff, so it is safe to
	 * initialise EDX with CDQ */

	cdq			/* let EDX=0 */

	divl	0x28(%bp)	/* s_inodes_per_group */
				/* EAX=group number */
	pushl	%edx		/* EDX=inode number in the group */

	/* group numbers are far less than 0x7fffffff, so it is safe to
	 * initialise EDX with CDQ */

	cdq			/* let EDX=0 */
	shll	$5, %eax	/* EAX=relative displacement of the group descriptor */
	
	testb   $0x40, 02(%bp)  /* filesystem 64? */
	je      1f              /* no */
	shll	$1, %eax        /* Group Descriptor Accounted for 64 bytes */
1:	
	divl	0x0e(%bp)	/* bytes per block */
				/* EAX=relative block number for the group descriptor */
				/* DX=displacement in the block */
				/* EDX high=0 */

	pushw	%dx		/* we don't care about EDX high word, because it is 0' */

	addl	0x2c(%bp), %eax	/* EAX=absolute block number for the group descriptor */
				/* CF=0, ZF=0 */

	call	read_block_mbr	/* 0000:1000 points to the block data containing the group descriptor */
				/* ES changed and > 0, BX=0x1000 */
				/* ECX=EDX=0 */
				/* CF=0, ZF=0 */

	popw	%si		/* DS:[BX+SI] points to the group descriptor */
				/* DS:[BX+SI+8] points to the starting block number of the group inode table */

	popl	%eax		/* inode number in the group */

	movw	0x26(%bp), %dx	/* EDX=inode size */
	mull	%edx		/* EDX:EAX=relative displacement of the inode struct */

	divl	0x0e(%bp)	/* bytes per block */
				/* EAX=relative block number for the inode struct */
	pushw	%dx		/* DX=displacement of the inode struct in the block */
				/* EDX high=0 */

	addl	8(%bx, %si), %eax	/* EAX=absolute block number for the inode struct */
					/* CF=0, ZF=0 */

	call	read_block_mbr	/* 0000:1000 points to the block data containing the inode struct */
				/* ES changed and > 0, BX=0x1000 */
				/* ECX=EDX=0 */
				/* CF=0, ZF=0 */

	popw	%si		/* DS:[BX+SI] points to the inode struct */

	addw	%bx, %si	/* DS:SI points to the inode struct */ 
	
	
	testl   $0x80000, 0x20(%si)     /* Node using the extents way to store data, */
	                                /* rather than the original direct_indirect index model to store data? */
	jne     extent_mbr              /* yes */
	
	/* Move the inode struct to a known safe area(0000:0fa8 - 0000:0fff),
	 * that is, 0x58 bytes immediately before 0000:1000. We care about only
	 * the beginning 0x58 bytes of the 0x80-byte inode struct, the last
	 * 0x28 bytes are ignored. The area from 0xfa8+0x28 to 0xfa8+0x57
	 * stores 12 direct block pointers.
	 *  
	 *  
	 * At address   Initial value               Stores what?
	 * ==========   =============   ======================================
	 * 0xfa8+0x04      (const)           the size of the file in bytes
	 * 
	 * 0xfa8+0x08    total blocks           blocks left to read
	 * 
	 * 0xfa8+0x0c         0           serial number of the block to read
	 * 
	 */

	pushw	%ss
	popw	%es				/* ES=0 */

	leaw	-0x58(%bx), %di			/* BX=0x1000, so DI=0x0fa8 */
	movb	$0x2c, %cl			/* 0x2c words = 0x58 bytes */

	repz movsw				/* now ECX=0, BX=0x1000=DI */

	movl	%ecx, (0x0c - 0x58)(%di)	/* block serial number of the file */
						/* ECX=0 means first block */
						/* DI=0x1000 */

	movl	(0x04 - 0x58)(%di), %eax	/* i_size, the file size */
	decl	%eax
	
        cdq
	
	divl	0x0e(%bp)			/* bytes per block */
						/* EDX=various */
	incl	%eax
	movl	%eax, (0x08 - 0x58)(%di)	/* total blocks for file data */

	/*
	 * 0000:1000	trebly indirect block
	 * 0000:8000	indirect block
	 * 0000:c000	double indirect block
	 * 1000:0000	the file data
	 */

	/* now DS:SI points to indirect block number */

	lodsl					/* indirect block number */
	testl	%eax, %eax
	jz	1f

	//pushw	%ss
	//popw	%es				/* ES=0 */
//	movb	$0x80, %bh			/* ES:BX=0000:8000 */
        movb	$0x20, %bh			/* ES:BX=0000:1000 */

	call	read_block_c_mbr
						/* ES changed and > 0, BX=0x8000 */
						/* ECX=EDX=0 */
						/* ZF=0, CF=0 */

	/* now DS:SI points to double indirect block number */

	lodsl					/* double indirect block number */
	testl	%eax, %eax
	jz	1f

	movb	$0xc0, %bh			/* ES:BX=0000:c000 */
	call	read_block_c_mbr
						/* ES changed and > 0, BX=0xc000 */
						/* ECX=EDX=0 */
						/* ZF=0, CF=0 */
	
	/* now DS:SI points to trebly indirect block number */

	/* the block at 0000:1000, which contains the indirect block numbers,
	 * is just overwritten by the trebly indirect block */
	
1:
	/* get absolute block number by block serial number */

	movl	(0x0c - 0x58)(%di), %ebx	/* block serial number of the file */
	subl	$12, %ebx
	jc	3f				/* direct block: block serial number < 12 */

	pushw	%bx
	subl	0x14(%bp), %ebx
	popw	%ax
	jnc	2f

	/* indirect block: 12 <= block serial number < 12 + 0x14(%bp) */

//	addb	$(0x70 / 4), %ah
        addb	$(0x10 / 4), %ah
	//xchgw	%ax, %bx
	jmp	8f

2:
	pushl	%ebx
	subl	0x10(%bp), %ebx

7:
	popl	%eax		/* EAX < 0x10(%bp) */
	cdq			/* let EDX=0 (notice the above jc 7f and jnz 7f) */
	divl	0x14(%bp)
				/* EAX=indirect block number, < 0x14(%bp) */
				/* EDX=block number, < 0x14(%bp) */

	pushw	%dx		/* EDX < 0x14(%bp) */
	testw	%dx, %dx
	jnz	7f

	/* if DX=0, we need to load the indirect block */

	shlw	$2, %ax
	xchgw	%ax, %bx

	/* get the indirect block number from the double indirect block data */

	movl	0xb000(%bx, %di), %eax

	movw	$0x2000, %bx			/* ES:BX=0000:1000 */

	call	read_block_c_mbr/* 0000:8000 points to the block data */
				/* ES changed and > 0, BX=0x8000 */
				/* ECX=EDX=0 */
				/* CF=0, ZF=0 */		
				
7:
	popw	%ax		/* AX < 0x14(%bp) */
//	addb	$(0x70 / 4), %ah 
        addb	$(0x10 / 4), %ah
8:
	xchgw	%ax, %bx
3:
	shlw	$2, %bx
	movl	(%bx, %di), %eax

	/* got it! EAX=absolute block number */

	/* read block data to 1000:0000. For root dir, read each block to
	 * 1000:0000(overwrite the previous read). For grldr, read blocks
	 * one by one to the area starting at 1000:0000. 
	 */

	popfw
	popw	%bx
	popw	%es
	pushfw

	/* CF=0 and ZF=1 for reading root dir, CF=1 for reading grldr */

	call	read_block_mbr	/* 1000:0000 points to the block data */
				/* ES changed and > 0x1000, BX=0 */
				/* ECX=EDX=0 */
				/* CF=0, ZF=0 */

	popfw
	pushw	%es
	pushw	%bx
	pushfw

	jc	3f		/* CF=1, we are reading grldr */

	/* We have just read a block of the root dir to 1000:0000. 
	 * So we check all dir entries in the block to see if anyone
	 * matches grldr.
	 */
find_grldr_mbr:
	xorw	%si, %si
	pushw	%ss
	popw	%es		/* ES=0 */

2:
	pushw	%ds		/* DS=0 */
	movw	%di, %ds	/* DS=0x1000 */
	movw	$(filename_ext2_mbr - Entry_12_64_ext + 0x7c30), %di
	
	pushw %di
102:
	cmpb $0, %es:(%di)
	je 101f
	cmpb $0x41, %es:(%di)
	jb 103f
	cmpb $0x5a, %es:(%di)
	ja 103f
	orb $0x20, %es:(%di)								/* Switch to lowercase */
103:
	incw %di
	jmp 102b
101:
	popw %di

	pushw	%si
	lodsl			/* This is possible inode number for grldr */
	pushl	%eax		/* This is possible inode number for grldr */
	lodsw
	xchgw	%ax, %dx	/* rec_len */
	lodsw			/* AL=name_len, should be 5 for grldr */
				/* AH=file_type(1 for regular file) */
        xorb    %ah, %ah
	xchgw	%ax, %cx	/* CX=name_len */
	repz    cmpsb
	jnz	5f
	xchgw	%ax, %cx	/* movb	$0, %al */
	scasb	
5:
	popl	%eax		/* This is possible inode number for grldr */
	popw	%si

	/* DS=0x1000, EAX=inode number */

	movw	%ds, %di	/* DI=0x1000 */
	popw	%ds		/* DS=0 */

	stc			/* indicates the new inode is for grldr */
	jz	4b		/* grldr is found with EAX=inode number */

	addw	%dx, %si
	cmpw	0x0e(%bp), %si	/* bytes per block */
	jb	2b

	/* file not found in this block, continue */

	/* We are lucky that CF=0, which indicates we are dealing with
	 * the root dir.
	 */
	movw	%fs, %ax
	orw	%ax, %ax
	jnz	1f

3:

	/* CF=1 for grldr, CF=0 for root dir. */

	incl	(0x0c - 0x58)(%di)
	decl	(0x08 - 0x58)(%di)
	jnz	1b
1:
        jnc     grldr_error_12_64_ext

	/* All grldr blocks have been loaded to memory starting at 1000:0000,
	 * Before the boot, we pass boot_drive and boot_partition to grldr.
	 */

	/* ES>0x1000, BX=0, ECX=EDX=0, DI=0x1000, SS=0, SI>0x7c00, DS=0
	 * BP=0x7c00, SP<=0x7c00
	 */

boot_now_mbr:

	movw	0x24(%bp), %dx

	/* boot it now! */

	pushw	%dx		/* for loading grub.exe */
	pushw	%di		/* 0x1000 */
	pushw	%ss		/* 0x0000 */
	lret

read_block_c_mbr:

	pushw	%ss
	popw	%es				/* ES=0 */
	stc
	
/* read_block - read a block
 * input:	CF	  - indicator for overlap or consecution
 *		EAX 	  = block number
 *		ES:BX	  - buffer
 *
 * output:	if CF is cleared on input, ES:BX is initialized to 0000:1000
 *		ES:BX	  - buffer filled with data
 *		ES, EAX	  - Changed
 *		ECX	  = 0
 *		EDX	  = 0
 *		ZF    = 0
 *		CF    = 0
 */

read_block_mbr:

	jc	1f

	.byte	0xC4, 0x5E, 0xFC	/* lesw -4(%bp), %bx */
					/* ES:BX=0000:1000 */
	jnz	1f

	//at this time, the gcc cannot generate 3 byte code
	.byte	0xC4, 0x5E, 0xFA	/* lesw -6(%bp), %bx */
					/* ES:BX=1000:0000 */
	//. = . - (. - read_block) / 6
1:
	
	movw    $1, %cx
	
read_block_e_mbr:

	movzbl  0x0d(%bp), %edx /* sectors per block */
	imulw   %dx, %cx        /* Number of sectors read */
	imull   %edx, %eax      /* Relative to the logical sector */	
	addl	0x1c(%bp), %eax	/* EAX=absolute sector number */
        call    readDisk_12_64_ext
        jb      disk_error_12_64_ext
        ret  
       
        
extent_mbr:
        addw    $0x28, %si
1:        
        cmpw    $0xf30a, (%si)  /* Magic signature? */
        jne     disk_error_12_64_ext   /* Not */
        movw    02(%si), %cx    /* number of effective index or leaves */
        cmpw    $0, 06(%si)     /* Leaf node? */
        je      2f              /* yes */
        /* ext4_extent_idx */
        addw    $0x0c, %si      /* Each leaf or index accounted 0x0c bytes */
        movb	$0x40, %ah				/* cf=0,zf=1 ES:BX=0000:1000 */
        sahf
        movl    4(%si), %eax    /* Lower 32 bits of physical block */
        call    read_block_mbr
        movw    %bx, %si       
        jmp     1b        
2:      /* ext4_extent */
        lesw    -6(%bp), %bx    /* ES:BX=1000:0000 */
1:        
        addw    $0x0c, %si      /* Each leaf or index accounted 0x0c bytes */
        movl    8(%si), %eax    /* Lower 32 bits of physical block */
        pushw   %cx             /* number of effective leaves */
        movw    4(%si), %cx     /* Logic blocks */
        call    read_block_e_mbr
        popw    %cx
        loop    1b
        popfw
        pushfw
        jc	boot_now_mbr
        movw	%si, %fs
        jmp	find_grldr_mbr

        .align	16


	
msg_BootError_12_64_ext:
	.ascii	"No "
	
grldr_un_12_64_ext:	
filename_12_64_ext:
filename_ext2_mbr:
        .ascii	"           \0\0"

msg_DiskReadError_12_64_ext:
	.ascii	"Disk error\0" 
	
        .fill 6,1,0

/* *************** FAT12_16_FAT32_exFAT_EXT2_3_4_mbr end ****************** */

//	. = _start1 + 0x1200  

end_12_64_ext:

	.arch	i586, jumps 
		

#ifdef DEBUG

	. = Entry_ext2 + 0x201

debug_print:

	pushfl
	pushal
	movl	%eax, %ebp
	call	2f
#if 0
	popal
	pushal
	movl	%ebx, %ebp
	call	2f
	popal
	pushal
	movl	%ecx, %ebp
	call	2f
	popal
	pushal
	movl	%edx, %ebp
	call	2f
	popal
	pushal
	movl	%esi, %ebp
	call	2f
	popal
	pushal
	movl	%edi, %ebp
	call	2f
	popal
	popfl
	pushfl
	pushal
	pushfl
	popl	%ebp		/* flags */
	call	2f
	movw	%ds, %bp
	shll	$16, %ebp
	movw	%es, %bp
	call	2f
	movw	$0x0e0d, %ax	/* print CR */
	int	$0x10		/* via TTY mode */
	movw	$0x0e0a, %ax	/* print LF */
	int	$0x10		/* via TTY mode */
#endif
	popal
	popfl
	ret
2:
	movw	$7, %cx
1:
	xorw	%bx, %bx	/* video page 0 */
	movl	%ebp, %eax
	shrl	%cl, %eax
	shrl	%cl, %eax
	shrl	%cl, %eax
	shrl	%cl, %eax
	andb	$0x0f, %al
	addb	$0x30, %al
	movb	$0x0e, %ah	/* print char in AL */
	int	$0x10		/* via TTY mode */

	decw	%cx
	testw	%cx, %cx
	jns	1b

	movw	$0x0e20, %ax	/* print space */
	int	$0x10		/* via TTY mode */
	ret
      
        
#endif        
        
#if 1
	/* restore GRLDR_CS */

	/* this code is executed at 0000:MONITOR, which must be a 16-byte
	 * aligned address. The address 0000:MONITOR should be designed in
	 * a way that could avoid memory confliction with volume boot records
	 * (currently FAT12/16/32/NTFS/EXT2/3 are built in).
	 */

	/* CS=code */

	.align 16

restore_GRLDR_CS:
2:
	call	1f
1:
	popw	%bx		# instruction pointer of 1b
	movw	%cs, %ax
	shrw	$4, %bx
	addw	%ax, %bx	# BX=segment value of this code
	pushw	%bx
	pushw	$(1f - 2b)
	lret
1:
	/* modify gdt base */
	xorl	%eax, %eax
	movw	%bx, %ax
	shll	$4, %eax
	addl	$(gdt -2b), %eax
	movl	%eax, %cs:(gdt - 2b + 2)
	
	movw	$GRLDR_CS, %bx
	movw	%bx, %es
	movw	%ds, %bx	# save old DS to BX

	cli
	lgdt	%cs:(gdt - 2b)
	movl	%cr0, %eax
	orb	$1, %al
	movl	%eax, %cr0

	movw	$8, %si
	movw	%si, %ds

	xorl	%esi, %esi
	xorl	%edi, %edi
	movl	$(0x9000 / 4), %ecx

	cld
	repz movsl

	movw	$16, %si
	movw	%si, %ds

	andb	$0xfe, %al
	movl	%eax, %cr0

	movw	%bx, %ds	# restore DS from BX 
	
        ljmp	$GRLDR_CS, $(try_next_partition - _start1)
			

	

#endif

# Descriptor tables
#
# NOTE: The intel manual says gdt should be sixteen bytes aligned for
# efficiency reasons.  However, there are machines which are known not
# to boot with misaligned GDTs, so alter this at your peril!  If you alter
# GDT_ENTRY_BOOT_CS (in asm/segment.h) remember to leave at least two
# empty GDT entries (one for NULL and one reserved).
#
# NOTE:	On some CPUs, the GDT must be 8 byte aligned.  This is
# true for the Voyager Quad CPU card which will not boot without
# This directive.  16 byte aligment is recommended by intel.
#
	.align 16
gdt:
	/* this is the default null entry in GDT */
	.word	gdt_end - gdt - 1		# gdt limit

        .long	(GRLDR_CS * 16 + gdt - _start1)	# linear address of gdt
		

	.word	0				# pad 2 bytes

	/* real mode data segment base=0x200000 */
	.word	0xFFFF, 0
	.byte	0x20, 0x92, 0, 0

	/* real mode data segment base=0 */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0, 0

gdt_end:

// ===========================================================================================

helper_start:
	//cs=ss=2000	ds=0d00/07c0	es=2000/0d00
	pushw	%cs
	popw	%ds
	pushw	$FS_BOOT
	popw	%es
	//cs=ds=ss=2000  es=0d00
   
	addw    $4, %si
	andb    $0x7f, (help_mark - _start1)
	andb $0xfd, (Boot_dl - _start1)

	/* Format of partition information blocks.
	 *
	 * Offset   Length in bytes	Field
	 *  00h		1		Set to 80h if this partition is active.
	 *  01h		1		Partition's starting head.
	 *  02h		2		Partition's starting sector and track.
	 *  04h(SI)	1		Partition's ID number.
	 *  05h		1		Partition's ending head.
	 *  06h		2		Partition's ending sector and track.
	 *  08h		4		Starting LBA.
	 *  0Ch		4		Partition's length in sectors.
	 */

	pushw	%ds		/* DS=0x2000 */
	pushw	%es		/* ES=0x0d00 */
	pushal
        
        cmpw	$0x1c2, %si
	jne	5f
	movb	$0, (help_mark - _start1)
#if     (defined(GRLDR_MBR)) 	

        testb   $0x0c, %cs:(mbr_disable - _start1)
	jz	1f

	pushw	%es		/* ES=FS_BOOT */
	/* set the DUCE indicator */
	xorw	%ax, %ax
	movw	%ax, %es
	movw	$0x5FC, %di
	movl	$0x45435544, %eax
	movb    %cs:(mbr_disable - _start1), %al
	andb	$0x0C, %al
	orb	$0x40, %al
	stosl
	popw	%es
1:
	call    Clear_KeyboardBuffer
	
	testb   $0x80, %cs:(mbr_disable - _start1)/* boot previous MBR first? */
	jnz	2f		/* no, continue to find GRLDR */

	/* yes, call the routine for booting the previous MBR.
	 * it will not return on success.
	 * on failure, it will return here
	 */

	/* before we call the routine, we will check if the user want to
	 * skip this step and continue to find the GRLDR
	 */
	/* if timeout==0, don't display the message */
        cmpb    $0, %cs:(mbr_time - _start1)
  	je      1f
	/* Press ... to start GRUB, any other key to boot previous MBR ... */
	
        movw	$(press_hot_key_pre - _start1), %si
	
	
	call	print_message

        movw	$(press_hot_key_name - _start1), %si
	
		
	call	print_message
	
        movw	$(press_hot_key_sub - _start1), %si
	
	
	call	print_message
1:
	call	sleep_5_seconds
	jc	3f		/* desired hot-key pressed */
	call	boot_prev_mbr	//Error_modify
	jmp	3f
2:
	/* before we find GRLDR, give the user a chance to boot_prev_mbr. */
	/* if timeout==0, don't display the message */
  	cmpb    $0, %cs:(mbr_time - _start1)
  	je      1f
	/* Press ... to boot previous MBR, any other key to start GRUB ... */
	
        movw	$(press_hot_key_pre - _start1), %si
	 
	
	call	print_message
	
        movw	$(press_hot_key_name - _start1), %si
	
	
	call	print_message
	
        movw	$(press_hot_key_sub1 - _start1), %si
	
	
	call	print_message
1:
	call	sleep_5_seconds
	jnc	3f
	/* desired hot-key pressed */
	andb    $0x7f, %cs:(mbr_disable - _start1)
	call	boot_prev_mbr	//Error_modify
3:
	pushw	%cs
	popw	%ds
	pushw	$FS_BOOT
	popw	%es
	/* modify code at 2b to be "jmp 4f" */
	
        movw	$(0xEB | ((4f - 2b - 2) << 8)), (2b - _start1)
		 
	orb     $0x80, %cs:(mbr_disable - _start1)
4:

	popal
	pushal 
	
#endif // defined(GRLDR_MBR) 

	movb	$0x41, %ah	/* EBIOS check existence */
	movw	$0x55aa, %bx
	call	int13
	jc	1f		/* No EBIOS */
	cmpw	$0xaa55, %bx
	jnz	1f		/* No EBIOS */
	testb	$1, %cl
	jz	1f		/* No EBIOS */
//      orb     $0x04, 0x0a
        orb     $0x04, (help_mark - _start1)
1:      

	/* initialize partition number and partition entries end */                                 
	movw	$0xffff, 0x1bc		/* hd partition number */
	movw	$0x01fe, 0x1ba		/* partition entries end */
	
	movw	$0xFE3F, (Sectors_passed_in - _start1)	/* use a default of H=255, S=63 */

	movb	$8, %ah		/* read drive parameters changes DX,ES,DI,BX */

	/* should not use "call int13", because "int13" will restore DX,
	 * and the DH returned by "int $0x13" thus be destroyed.
	 */
	pushaw		/* <------ at least DL,SI need to be saved */ 
	pushw	%ds
	pushw	%es
	int	$0x13
	popw	%es
	popw	%ds
	jc	1f		/* failed */
	andb	$63, %cl	/* sectors per track */
	jz	1f		/* failed */

	movb	%dh, %ch	/* DH=Hmax */
	movw	%cx, (Sectors_passed_in - _start1)	/* CL=Smax, CH=Hmax */	
1:      
        popaw		/* <------ at least DL,SI need to be restored */
        
        /*  geometry_tune */
        testb   $0x04, (help_mark - _start1)
	jne	1f		/* yes, skip the tune */
	
#if     defined(GRLDR_MBR)	
	
        testb   $0x08, (mbr_disable - _start1)
	jnz	1f 

#endif // defined(GRLDR_MBR)
	
	/* check if the code is loaded completely */
	pushw	%ds
	pushw	%es
	pushaw
	call	geometry_tune
	popaw
	popw	%es
	popw	%ds
        
        movw    (Sectors_passed_in - _start1), %cx
	movb    %cl, 0x18
	incb    %ch
	movb    %ch, 0x1a
1:
	pushw	%si
	subw	$4, %si
	call	Check_PartitionTable
	popw	%si
	jc	5f
	orb	$8, (help_mark - _start1)
	call gpt_statr  
5:        
	xorl    %eax, %eax              
	testb   $8, (help_mark - _start1)/* Partition Table OK? */
	je      5f                      /* not */      
	cmpw	$0x1f2, %si		/* is it a primary partition? */
	ja	3f			/* no, it is an extended partition */
	movl	4(%si), %eax
	movl	%eax, 8(%si)		/* parent part_start saved here */
	xorl	%eax, %eax
	movl	%eax, 4(%si)		/* current part_start(0) saved here */

3:
	movl	4(%si), %eax
	addl	8(%si), %eax	/* EAX=start_LBA */
	testl	%eax, %eax
	je	2f
	call	read_lba
	jnb	5f
2:
	orb	$0x80, (help_mark - _start1)
5:

	/*******************************************************/
	/* load partition boot track to FS_BOOT using LBA mode */
	/*******************************************************/

	pushw	%si
	pushw	%es
//---------------------------------------------------------
	/* print "Try (hd0,n): " or "Try (fd0): "*/
	pushw	%ds
	popw	%es
	//cs=ds=es=ss=2000
	cld			/* for stosb */
	xorw	%ax, %ax
        testb   $8, (help_mark - _start1)
        je      1f              /* not */
	/* hard drive */
	incw	0x1bc		/* logical partition number */
	movw	0x1bc, %ax
	movb    %al, %dh
	cmpb	$4, %al
	jb	1f
	
        movb	$0xFC, (add_sub_si + 2 - _start1)
1:
	/* AL=partition number, AH=0 */
	pushw	%ax

        movw	$(partition_message - _start1 + 7), %di	/* drive type */
	
	
	movb	%dl, %al
	shrb	$7, %al		/* drive type: floppy=0, harddrive=1 */
	shlb	$1, %al
	addw	$0x6466, %ax	/* "fd" or "hd" */
	stosw
	movb	%dl, %al
	andb	$0x7f, %al	/* drive number */
	aam		/* convert binary to decimal, AH=high, AL=low */
	testb	%ah, %ah
	jz	1f
	addb	$0x30, %ah
	movb	%ah, (%di)
	incw	%di
1:
	addb	$0x30, %al
	stosb

	popw	%ax
        testb   $8, (help_mark - _start1)
        je      2f              /* not */    
	/* this is a hard drive, the partition number is in AL */
	movb	$0x2c, (%di)	/* "," */
	incw	%di
	aam		/* convert binary to decimal, AH=high, AL=low */
	testb	%ah, %ah
	jz	1f
	addb	$0x30, %ah
	movb	%ah, (%di)
	incw	%di
1:
	addb	$0x30, %al
	stosb
2:
	movl	$0x00203a29, (%di)	/* "): \0" */ 
	
        movw	$(partition_message - _start1), %si
	
	
	call	print_message	/* CS:SI points to message string */
//-------------------------------------------------------------------------------------
	popw	%es 
	//cs=ds=ss=2000  es=0d00
        testb   $0x80, (help_mark - _start1)
        jne     invalid_or_null

	xorw	%si, %si
	pushw	%es
	popw	%ds
	

	//cs=ss=2000  ds=es=0d00
	
	//Installation fat12/16/32/exfat/ext2/ntfs partition
	/* First, check for ext2 filesystem */
       
        andb    $0, 0x02                /* Initialization mark */
	testb $0x04, %cs:(help_mark - _start1)
	je 1f
	orb $0x80, 02(%si)    /* force LBA */
1:
	cmpw $0xEF53, 0x438		/* Magic signature */
	jnz fat
	
/* ext2 Patch start */	
	xorl	%eax, %eax
	cmpl	%eax, 0x420		/* s_blocks_per_group */
	jz	fat
	cmpl	%eax, 0x428		/* s_inodes_per_group */
	jz	fat

	/* BX = 0 */
	/* EAX= 0 */

	movw	0x458, %ax		/* AX=s_inode_size */
	testw	%ax, %ax
	jnz	2f			/* invalid inode size */ 
        movw	$0x80, %ax		/* EXT2_GOOD_OLD_INODE_SIZE */

2:
        movw    %ax, 0x26(%si)
	
	movb	0x418, %cl		/* s_log_block_size */
	movl	$2, %eax
	shlw	%cl, %ax
	movb	%al, 0x0d(%si)
	shlw	$9, %ax			/* block size is word wide */
	movw	%ax, 0x0e(%si)
	shrw	$2, %ax
        movl	%eax, 0x14(%si)
	addb	$8, %cl
	shll	%cl, %eax
        movl	%eax, 0x10(%si)

	
	/* fix for ext2 partition: s_inodes_per_group, offset 0x28 */
        movl	0x428, %eax		/* s_inodes_per_group */
        movl	%eax, 0x28(%si)
        
        
        testb   $0x80, 0x460(%si)       /* 64-bit file system? */
        je      1f                      /* not */
        orb     $0x40, 2(%si)           /* 64-bit file system marked */
1:
        testb   $0x10, 0x460(%si)       /* metablock group? */
        je      1f                      /* not */
        movl    0x504(%si), %eax        /* The first metablock group */
        movl    %eax, 0x2c(%si)
        jmp     2f
1:                

	/* fix for ext2 partition: block number for group descriptors, offset 0x2c */
        /* At which block the group descriptors begin? */
	movl	0x414, %eax		/* s_first_data_block */
	incw	%ax
	movl	%eax, 0x2c(%si)
2:

	/* fix for ext2 partition: on error go back to supervisor, offset 0x01fc */
        
        movw    $0x200, 0x0b(%si)       /* bytes per sector */
        orb     $0x10, 02(%si)          /* ext2 Mark */

fat:

	cld

	/* ds=es=0d00  cs=ss=2000 si=0 */
        cmpl    $0x5346544e, 3(%si)             /* ntfs Partition? */
        je      3f                              /* yes */ 

        //fat12/16/32/exfat/ext2
        movw	%cs:(Sectors_passed_in - _start1)(%si), %ax	/* SI=0 */
        movb	%al, 0x18(%si)		/* DS=ES */
        shrw	$8, %ax
        incw	%ax
        movw	%ax, 0x1a(%si)        
2:      
        /* BUG fix for extended FAT12/16/32/64/ext2 partition */
        popw	%di			/* DI points to old entry in MBR */
        pushw	%di
        xorl	%eax, %eax		/* let hidden_sectors=0 for floppy */
        testb   $8, %cs:(help_mark - _start1)
        je	2f			/* not */       
        movl	%cs:4(%di), %eax
        addl	%cs:8(%di), %eax
2:      
				movl	%eax, 0x1c(%si)		/* adjust hidden_sectors for FAT */
        
        cmpl    $0x33544146, 0x52(%si)  /* fat32 Partition? */
        jne     1f                      /* not */
        cmpl    $0x20202032, 0x56(%si)  /* fat32 Partition? */
        je      7f                      /* yes */
1: 
        //fat12/16/exfat/ext2
				movw		%dx, 0x24(%si)
				testb   $0x10, 02(%si)          /* ext2 Mark */
        jne     9f                      /* yes */
				//fat12/16/exfat
        cmpl    $0x41465845, 03(%si)    /* exfat Partition? */
        jne     2f                      /* not */
        cmpl    $0x20202054, 07(%si)    /* exfat Partition? */
        je      6f                      /* yes */
2:     
        //fat12/16
				cmpl    $0x31544146, 0x36(%si)  /* fat12/16 Partition? */
        jne     1f                      /* not */
        cmpl    $0x20202032, 0x3a(%si)  /* fat12 Partition? */
        je      4f                      /* yes */
				//fat16
        cmpl    $0x20202036, 0x3a(%si)  /* fat16 Partition? */
        je      5f                      /* yes */
3:      
        cmpl    $0x20202020, 07(%si)    /* ntfs Partition? */
        jne     1f                      /* not */
        

	/* NTFS */
	movl	0x20(%si), %eax		/* FAT32 total sectors */
	testl	%eax, %eax
	jnz	1f
	movw	0x0e(%si), %ax		/* reserved sectors */
	testw	%ax, %ax
	jnz	1f

	/* fix for ntfs partition: sectors per track, offset 0x18 */
	/* fix for ntfs partition: number of heads, offset 0x1a */
	movw	%cs:(Sectors_passed_in - _start1)(%si), %ax	/* SI=0 */
	movb	%al, 0x18(%si)		/* DS=ES */
	shrw	$8, %ax
	incw	%ax
	movw	%ax, 0x1a(%si)
	/* BUG fix for extended NTFS partition */
	popw	%si			/* SI points to old entry in MBR */
	pushw	%si
	xorl	%eax, %eax		/* let hidden_sectors=0 for floppy */ 		           
	testb   $8, %cs:(help_mark - _start1)/* Partition Table OK */
        je	3f			/* not */   
	movl	%cs:4(%si), %eax
	addl	%cs:8(%si), %eax
3:
	movl	%eax, 0x1c		/* adjust hidden_sectors for NTFS */

	movb	%dl, 0x24		/* adjust drive number for NTFS */

	// Load NTFS using internal boot sector at 0xA00

        movw	$(NTFS5_message - _start1), %si
	
	
	call	print_message	/* CS:SI points to message string */

        movw    $(Entry_nt - _start1), %bx
	
	movw	$0x52, %cx

	pushw	%cs
	popw	%ds

	/* DS=SS=0x2000 */
	/* ES=FS_BOOT */

	movw	%bx, %si
	xorw	%di, %di
	lodsw
	stosw
	addw	%cx, %si
	addw	%cx, %di
	movw	$0x800, %cx
	subw	%di, %cx

	repz movsb

	/* modify the boot partition number */
	movb	%es:1, %al
	addb	$5, %al			/* AL is less than 0x80 */
	cbw				/* AH=0 */
	xchgw	%ax, %di		/* move AX to DI */
	movb	0x1bc, %al		/* partition number */
	stosb

	/* fix for NTFS partition: on error go back to supervisor, offset 0x01fa */

  movw $(error_go_back - Entry_nt), %di
	movw	%es:(%di), %ax
	cmpw	$0xFEEB, %ax /* EB FE is jmp back to itself(infinite loop) */
	jnz	3f
	decw	%ax		/* AL=0xEA, ljmp */
	stosb
	movw	$MONITOR, %ax
	stosw
	xorw	%ax, %ax
	stosw			/* DI=0x01ff */
3:
	clc
	jmp	move_entries_and_return
 

6:      /* exFAT */
        /* Parameters calculated by the bpb */
        orb     $8, 02(%si)                     /* exfat Mark */ 
				movl    0x48(%si), %eax
				movl    %eax, 0x20(%si)									//The total number of sectors partition
        movl    0x50(%si), %eax
        addl    0x1c(%si), %eax
        testb   $1, 0x6a(%si)
        je      6f
        addl    0x54(%si), %eax
6:        
        movl    %eax, 0x28(%si)                 /* FAT table start absolute sector number */ 
        movl    0x58(%si), %eax
        addl    0x1c(%si), %eax                       
        movl    %eax, 0x0e(%si)                 /* Data start Absolute sector */
        movb    0x6c(%si), %cl                  /* Bytes per sector (power) */
        movw    $1, %ax
        pushw   %ax
        shlw    %cl, %ax
        movw    %ax, 0x0b(%si)                  /* Bytes per sector */
        movb    0x6d(%si), %cl                  /* Sectors per cluster (power) */
        popw	%ax
        shlw    %cl, %ax
        movw    %ax, 0x26(%si)                  /* Sectors per cluster */
        movl    0x60(%si), %eax
        movl    %eax, 0x2c(%si)                 /* Root clusters */ 
        
        movw	$(exfat_message - _start1), %si
        jmp     3f         
4:      /* fat12 */ 
        orb     $1, 02(%si)                     /* fat12 Mark */
2:        
        pushw   %dx
        movzbw 0x0d(%si), %ax
        movw %ax, 0x26(%si)
				movzwl  0x0e(%si), %ecx                 /* Reserved sector */
        addl    0x1c(%si), %ecx
        movl    %ecx, 0x28(%si)                 /* FAT table start absolute sector number */
        movzbw  0x10(%si), %ax                  /* Number of FAT table */
        mulw    0x16(%si)
        pushw   %dx
        pushw   %ax
        popl	%eax                            /* FAT table number of sectors */
        addl    %ecx, %eax                      
        movl    %eax, 0x07(%si)                 /* Home directory of the absolute starting sector (fat12/16) */ 
        pushl   %eax         
        movw    0x0b(%si), %cx                  /* Bytes per sector */
        shrw    $5, %cx                         /* (Bytes per sector)/20h=10h= Contents of each sector */
        movw    0x11(%si), %ax                  /* File number of Master Contents */
        addw    %cx, %ax
        decw    %ax
        divw    %cx                             /* sectors  number of  the main directory */ 
        movzwl  %ax, %eax                       /* sectors  number of  the main directory */
        popl    %ecx                            /* Home directory of the absolute starting sector (fat12/16) */ 
        addl    %eax, %ecx
        popw    %dx
        movl    %ecx, 0x0e(%si)                 /* Data start Absolute sector */ 
        testb   $1, 02(%si)                     /* fat12? */
        je      8f                              /* not */
        
        movw	$(FAT12_message - _start1), %si       
        jmp     3f
8:      
        movw	$(FAT16_message - _start1), %si
        jmp     3f
5:      /* fat16 */
        orb     $2, 02(%si)                     /* fat16 Mark */
        jmp     2b
9:			/* ext2 */
        movw	$(EXT2_message - _start1), %si
     
        jmp     3f
7:      /* fat32 */
        orb     $4, 02(%si)                     /* fat32 Mark */
        movzwl  0x0e(%si), %ecx                 /* Reserved sector */
        addl    0x1c(%si), %ecx
        movl    %ecx, 0x28(%si)                 /* FAT table start absolute sector number */
        movzbl  0x10(%si), %eax
        pushw   %dx
        mull    0x24(%si)
        addl    %ecx, %eax
        movl    %eax, 0x0e(%si)                 /* Data start Absolute sector */
        popw    %dx
        movw    %dx, 0x24(%si)        
        movzbw 0x0d(%si), %ax
        movw %ax, 0x26(%si)
        movw	$(FAT32_message - _start1), %si      
3:     
        pushw %si
				pushw %ds
				pushw %es
				pushw %di
				movw $(nt_boot_image - _start1), %si 
				movw $(filename_12_64_ext - _start1), %di
				pushw %cs
				popw %es
				testb $7, 02
				pushw %cs
				popw %ds
				jne 101f
				
				movw $12, %cx
				repz movsb
				jmp 105f
101:				
				pushw %di
102:				
				movsb
103:				
				cmpb $0, (%si)
				je 104f
				cmpb $0x2e, (%si)
				jne 102b
				incw %si
				popw %di
				pushw %di
				addw $8, %di
				jmp 103b
104:
				popw %di
105:				
				popw %di
				popw %es
				
				/* From 2000 to  0d00 move the boot code */
        /* cs=ds=ss=2000  es=0d00 */
        movw    $(Entry_12_64_ext - _start1), %si       /* 0a00 */       
        movw    $0x30, %di
        movw    $((end_12_64_ext - Entry_12_64_ext) / 4 +1), %cx      /* move Double word */
        repz    movsl                           /* 2000:0400->0d00:0030 */
        popw	%ds
        popw	%si
        /* cs=ss=2000  ds=es=0d00  */
        movw    $0x2eeb, 0x00                   /* jmp 7c30 */
      
        call    print_message                   /* CS:SI points to message string */
        jmp     move_entries_and_return
        /* fat12/16/32/64/ext2 Partition disposed */


1:
	#; It is not FAT12/16/32/NTFS. Check for extended partition.

	/* cs=ss=2000  ds=es=0d00 */
        orb     $0x80, %cs:(help_mark - _start1)
	pushw	%cs
	popw	%es

	//cs=es=ss=2000  ds=0d00

	popw	%si
	pushw	%si
	cmpb	$0x05, %es:(%si)	/* extended */
	je	1f
	cmpb	$0x0f, %es:(%si)	/* Win95 extended (LBA) */
	je	1f
	cmpb	$0x15, %es:(%si)	/* hidden extended */
	je	1f
	cmpb	$0x1f, %es:(%si)	/* hidden win95 extended (LBA) */
	je	1f
	cmpb	$0x85, %es:(%si)	/* Linux extended */
	je	1f

        movw	$(non_MS_message - _start1), %si
		
4:
	call	print_message	/* CS:SI points to message string */
	jmp	move_entries_and_return
	
1:
	/* extended partition entry */
	cmpw	$0x1fe, %si
	jb	1f
	decw	%es:0x1bc	/* count the partitions in extended zone */
1:	
        movw	$(extended_message - _start1), %si
	
	
	call	print_message	/* CS:SI points to message string */
	movw	$0x1be, %si
	call    Check_PartitionTable
        jb      1f
        
	movw	$4, %cx
	popw	%bx	/* the old SI points to extended partition ID in MBR */
	pushw	%bx
	pushw   %dx
5:
	movl	(%si), %eax
	cmpw	2(%si), %ax	/* Is EAX high word equal to AX? */
	jnz	2f
	cmpb	%al, %ah	/* Is AL=AH? */
	jnz	2f

	/* now all 4 bytes in EAX are equal to each other. */
	cmpl	%eax, 4(%si)
	jnz	2f
	cmpl	%eax, 8(%si)
	jnz	2f
	cmpl	%eax, 12(%si)
	jz	3f	/* entry with 16 dups of a byte means empty entry */
2:
	/* now it is an acceptable entry */
	movw	%es:0x1ba, %di	/* partition entries end */

	addw	$16, %es:0x1ba	/* increment partition entries end */

	lodsl
	stosl
	lodsl
	stosl

	xchgw	%ax, %dx	/* save AL(the partition ID)to DL */

	lodsl
        xchgl	%eax, %edx	/* restore AL from DL(the partition ID), */
				/* and save EAX to EDX */
	cmpb	$0x05, %al
	je	6f
	cmpb	$0x0f, %al
	je	6f
	cmpb	$0x15, %al
	je	6f
	cmpb	$0x1f, %al
	je	6f
	cmpb	$0x85, %al
	je	6f
	/* normal partition, copied to 0x941fe-0x943fb */
	addl	%es:4(%bx), %edx	/* current partition start */
6:
	/* extended partition, copied to 0x941fe-0x943fb */
        xchgl	%eax, %edx	/* restore or update EAX from EDX */
	stosl
	lodsl				/* adjust SI only */
	movl	%es:8(%bx), %eax	/* parent partition start ... */
	stosl				/* ... stored here */
	jmp	2f
3:
	addw	$16, %si
2:
	loop	5b
	
	popw    %dx

	/* extended partition is not a normal one, so set carry to try next */
	jmp	move_entries_and_return
//cs=ss=2000  ds,es?
invalid_or_null:
1:
        movw	$(invalid_message - _start1), %si	
	call	print_message	/* CS:SI points to message string */

move_entries_and_return:
	popw	%si
	pushw	%cs
	popw	%ds
	pushw	%cs
	popw	%es
	//cs=ds=es=ss=2000
	pushw	%si
	cmpw	$0x202, %si
	jne	1f
	/* move entries backward 1 entry */
	movw	$0x1fe, %di
	movw	$0x20e, %si
	movw	$0xf8, %cx		/* 0x1f0 bytes = 0xf8 words */
	cld				/* move upward */
	repz movsw
	subw	$0x10, 0x1ba	
1:
	popw	%si
	movw    %sp, %bp	
	testb	%dl, %dl
	js	3f
	movb	$0xff, %dh	/* partition # for floppy is "whole drive" */
3:	
	movw	$0x1fe, %ax	
        movw	$(add_sub_si + 5 - _start1), %di	
	movw	%ax, (%di)		/* 0x1fe */
        testb   $8, (help_mark - _start1)
        je      4f
	incw	(%di)			/* 0x1ff */
	cmpw	%ax, 0x1ba		/* AX=0x1fe */
	jne	1f
	decw	(%di)			/* 0x1fe */
	cmpw	$0x1f2, %si
	ja	2f			/* logical partition */
	jb	1f			/* primary partition 0, 1, 2 */
	/* primary partition 3 */
	cmpw	$0x0003, 0x1bc		/* are there any logical partitions? */
        jne     1f
2:
inc_hard_drive:
//cs=ds=es=ss=2000
	/* all partitions on the drive have been checked, try next drive.
	 *
	 * the current stack is:
	 *
	 * SP + 36	: DS
	 * SP + 32	: ES
	 * SP + 28	: EAX
	 * SP + 24	: ECX
	 * SP + 20	: EDX
	 * SP + 16	: EBX
	 * SP + 12	: ESP_temp
	 * SP + 08	: EBP
	 * SP +  4	: ESI
	 * SP   	: EDI
	 *
	 */

	/* get total hard drives */
	orb     %dl, %dl
	je      4f
	xorw	%ax, %ax
	movw	%ax, %ds
	movb	0x475, %dh
	pushw	%cs
	popw	%ds
	orb	$0x80, %dh	/* CF=0, DH=Max harddrive number + 1 */
2:
	jnc	3f
	call	print_message	/* CS:SI points to message string */ 	
        movw	$(drive_number_string - _start1), %si	
	movb	%dl, %al
	andb	$0x7f, %al
	aam			/* AH=high decimal, AL=low decimal */
	addw	$0x3030, %ax
	xchgb	%al, %ah
	movw	%ax, 32(%si)
	call	print_message	/* CS:SI points to message string */
3:
	incw	%dx		/* !!!! Next drive !!!! */
	cmpb	%dh, %dl
	jnb	4f		/* all drives checked, try floppy finally */
9:
        pushw	%dx
        movb    $1, %al
	movw	$0x7e00, %bx	/* read MBR to 9400:7e00 */
	xorb	%dh, %dh
	call    read_disk_with_reset_and_dec_di
	popw	%dx 
        movw	$(Error_while_reading_string - _start1), %si	
	jc	2b		/* read failure, try next hard drive */

	/* The new partition table might be empty or invalid.
	 * Move the new partition table onto the old one while checking
	 */
	movw	$0x7fbe, %si
	call    Check_PartitionTable
        jb      2b                     
	movw	$0x01be, %di
	movw    $0x21, %cx
	repz    movsw

	/* Now the partition table is OK */

	movw	%dx, 8(%bp)
	orb $2, (Boot_dl - _start1)

	movw	$0x1b2, 4(%bp)	/* adjust SI in the stack */

	/* temp change the code:	call	self_modify_once
	 *
	 * "call self_modify_once" at add_sub_si is:
	 *
	 *	.byte	0xE8
	 *	.word	(self_modify_once - add_sub_si - 3)
	 *
	 */
	 
        movb	$0xE8, (add_sub_si - _start1)
	movw	$(self_modify_once - add_sub_si - 3), (add_sub_si + 1 - _start1)

	/* initialize partition number and partition entries end */
	movw	$0xffff, 0x1bc		/* hd partition number */
	movw	$0x01fe, 0x1ba		/* partition entries end */
	jmp	1f
4:

	/* floppy search disabled ? */
        orb     %dl, %dl
        jne     1f
        movw    %sp, %bp
        movw    $0x300, 4(%bp)	/* adjust SI in the stack */
1:
	popal
	popw	%es
	popw	%ds
	//cs=ds=ss=2000  es=0d00
	ret

//input	eax, es:bx
read_lba:
	pushw	0x02
	pushw	0x0b
	pushw	0x24
	pushw	%bp
	xorw	%bp, %bp
	andb	$0, 0x02
	testb	$0x04, (help_mark - _start1)
	je	1f
	orb	$0x80, 0x02
1:
	movb	%dl, 0x24
	movw	$0x200, 0x0b
	xorw	%bx, %bx
	movw	$4, %cx
	pushw	%es
	call	readDisk_12_64_ext     /* CX=0, eax,ES changed */
	popw	%es
1:
	popw	%bp 
	popw	0x24
	popw	0x0b
	popw	0x02
	ret
	
	
	
//cs=ds=ss=2000	es=0d00
gpt_statr:
	pushaw
	cmpb	$0xee, 0x1c2
	jne	1f
	
	movl	$1, %eax
	call	read_lba
	jc	1f

	cmpl	$0x20494645, %es:0		//"EFI PART"?
	jne	1f											//no
	movl	%es:72, %eax
	call	read_lba
	
	movw	$0x100, %cx
	movw	$0x1be, %di
2:
	movb	$0, (%di)
	incw	%di
	loop 2b
	
	movw	$0x1c2, %si						//mbr
	movw	$0x20, %di						//gpt
	movw	$16, %cx
2:	
	cmpl	$0xe3c9e316, %es:-0x20(%di)//"microsoft reserved partition"?
	je	3f											//no
	cmpl	$0, %es:4(%di)
	jne	2f
	movl	%es:(%di), %eax
	orl	%eax, %eax
	je 2f
	movb	$0xEE, (%si)
	movl	%eax, 4(%si)
3:	
	addw	$0x10,	%si
	addw	$0x80,	%di
	loop	2b
2:
	negw	%cx
	addw	$16, %cx
	cmpw	$4, %cx
	jbe	1f
	subw	$4, %cx
	shlw	$4, %cx
	addw	%cx, 0x1ba
1:
	popaw
	ret	

self_modify_once:
	/* when we get here, SI should be 0x1b2, and BP high holds DL */ 
	addw	$12, %si	/* 0x83, 0xC6, 0x0C */
	movw	%bp, %ax
	movb	%al, %dl
	/* note: DS=0x9400 */

	/* restore the original code:	addw	$12, %si */ 
	
        movw	$0xC683, (add_sub_si  - _start1)	/* 0x83, 0xC6 */
	movb	$0x0C, (add_sub_si + 2 - _start1)	/* 0x0C */	
	ret

Error_modify:

	/* we will do the second pass, from drive 0x80. */
	movb	$0x7f, %dl	/* this will become 0x80 after inc. */

	/* pass "error" to PUSHF, simulating a load failure, in order
	 * to try the first entry after return from the helper function.
	 */

//      orb     $0x80, 0x0a
        orb     $0x80, (help_mark - _start1)
        
        pushw	$(helper_call + 3 - _start1)	/* return address */
	        
	
	pushw	%cs		/* 0x9400, it is for DS. */
	pushw	$FS_BOOT	/* 0x0d00, it is for ES. */
	pushal

	pushw	%cs
	popw	%es		/* ES=0x9400 */
	movw	%sp, %bp

	/* redo from start: DL will be 0x80 after inc. */
	jmp	inc_hard_drive
	
	
1:
boot_prev_mbr:

	/* prepare to boot the previous MBR */

	/* at this moment DS=0x9400, ES=$FS_BOOT or ES=0x9400 */

#if     defined(GRLDR_MBR) 	
	call    Clear_KeyboardBuffer
#endif // defined(GRLDR_MBR) 	
		
	xorw	%ax, %ax	/* AX=0, for the segment of 0000:7c00 */
	movw	%ax, %es	/* ES=0x0000 */
	movw	%ax, %ds	/* DS=0x0000 */
//	movw	$0x0202, %ax	/* read 2 sectors ... */
        movw	$0x0201, %ax	/* read 1 sectors ... */
//	movw	$0x7A00, %bx	/* ... to 0000:7A00 */
        movw	$0x7C00, %bx	/* ... to 0000:7C00 */
//	movw	$0x0001, %cx	/* from the first sector ... */
	
	movw	$0x0002, %cx	/* 0-0-2 */
	movw	$0x0080, %dx	/* ... of the first hard drive */
	call	int13
	sti
	jc	1f
	testb	%ah, %ah
	jnz	1f
	cmpw	$0xAA55, 0x7dfe
	jne	1f

	/* has a valid partition table ? */
	movw	$0x7dbe, %si
	call    Check_PartitionTable
	
//	pushfw

	/* disable the boot of non-MBR bootsector ? */ 	
	
//	testb	$2, %cs:0x02		/* test bit1 of the third byte */

//	testb   $2, %cs:(mbr_disable - _start1)
//	jz	2f			/* zero means non-MBR enabled */
 	
//	popfw
	jc	1f	/* invalid partition table, print "Error" */	

	/* the partition table is valid */
//	pushfw

//2:
	/* the check passed, and the boot is permitted */
//	popfw

//	jc	2f	/* invalid partition table */

	/* use partition table in MBR instead */

	/* copy 72 bytes at 0000:7bb8 to 0000:7db8 */

//2:
#if     defined(GRLDR_MBR)
 
//	testb	$0x80, %cs:0x02		/* test bit 7 of the third byte */
        testb   $0x80, %cs:(mbr_disable - _start1)
	jz	2f			/* zero means boot prev-MBR first */

	/* Cannot find GRLDR. Press space bar to hold the screen, any other key to boot previous MBR ... */ 
	
        movw	$(Cannot_find_GRLDR_string - _start1), %si
	
	
	call	print_message	        /* CS:SI points to message string */
	
        movw	$(nt_boot_image - _start1), %si
	
	
	call	print_message	        /* CS:SI points to message string */ 
	
        movw	$(press_space_bar_string - _start1), %si
	
	
//	movw	$0x3920, %cs:0x04	/* reset hot-key to space bar */
        movw    $0x3920, %cs:(mbr_hotkey - _start1)
//	movb	$15, %cs:0x03		/* reset time out to 15 seconds */
        movb    $15, %cs:(mbr_time - _start1)
	call	print_message	        /* CS:SI points to message string */ 
	
        movw	$(prev_MBR_string - _start1), %si
	
	
	call	print_message	        /* CS:SI points to message string */
3:
	call	sleep_5_seconds		/* the hot-key is SPACE */
	/* if hot-key is pressed, wait forever until another key is pressed. */
//	movb	$0xff, %cs:0x03
	movb    $0xff, %cs:(mbr_time - _start1)
	jc	3b		        /* desired hot-key is pressed */ 

#endif // defined(GRLDR_MBR) 
	
2:
	/* boot the previous MBR */

	/* clear the DUCE indicator */
	movl	$0, 0x5FC	        /* DS=ES=0 */

	//movb	$0x80, %dl
	xorb    %dh, %dh
	ljmp	$0, $0x7c00
1:

#if     defined(GRLDR_MBR)
 
	/* no previous MBR, print "Error" */

//	testb	$0x80, %cs:0x02	        /* are we called prior to the GRLDR search? */
        testb   $0x80, %cs:(mbr_disable - _start1)
	jnz	1f		        /* no, it is a failure at last */
	/* yes, so return to the caller */
	/* Invalid previous MBR. Press any key to start GRUB ... */
	
        movw	$(continue_string - _start1), %si	
	call	print_message	        /* CS:SI points to message string */
	call	sleep_5_seconds
	ret

#endif // defined(GRLDR_MBR)

1:	
        movw	$(message_string_helper - _start1), %si	
	call	print_message	        /* CS:SI points to message string */
	
        movw	$(nt_boot_image - _start1), %si	
	call	print_message	        /* CS:SI points to message string */
	
        movw	$(ctrl_alt_del_string - _start1), %si	
	call	print_message	        /* CS:SI points to message string */	
1:	
        jmp	1b	/* hang */ 


#if     defined(GRLDR_MBR) 

Clear_KeyboardBuffer:
        pushw   %ds
        xorw    %ax, %ax
        movw    %ax, %ds
        movw    0x41c, %ax
        movw    %ax, 0x041A
        pop     %ds
        ret
#endif // defined(GRLDR_MBR) 

// in: ds:si
Check_PartitionTable:
        pushaw
        xorb    %bl, %bl
        movw    %si, %di
        addw    $0x40, %di
4:       
        cmpw	%di, %si
	jnb	3f		        /* partition table is OK */
	pushw   %si
	movw	$4, %cx	
2:
	lodsl
	negl	%eax
	jc	2f
	loop	2b	
	popw	%ax
	incb    %bl
	cmpb    $4, %bl
	jb	4b                      /* empty entry, check next */	
	jmp     5f                      /* Error: empty table */
2:
	/* non-empty entry */
	popw	%si
	lodsw
	shlb	$1, %al
	jnz	5f
	lodsw
	andb	$63, %al
	jz	5f
	lodsw
	lodsw
	andb	$63, %al
	jz	5f
	lodsl
	negl	%eax
	jnc	5f
	lodsl
	negl	%eax
	jc	4b
	jmp 	5f	
3:      
        lodsw
        cmpw    $0xAA55, %ax
        je      1f
5:
	stc		                /* invalid partition table */        
1:
        popaw
        ret        

#if     defined(GRLDR_MBR)  

sleep_5_seconds:
	/* sleep 5 seconds */

	/* sleep forever if %cs:0x03 is 0xff */

	/* calculate the timeout ticks */

	pushw	%ds
	pushl	%esi
	pushl	%edx

	movl	$0xffffffff, %edx
//	movzbl	%cs:0x03, %eax
	movzbl  %cs:(mbr_time - _start1), %eax
	cmpb	$0xff, %al
	je	1f
	movl	$18, %edx	/* 18.2 ticks per second. We simply use 18. */
	mulw	%dx		/* EDX=0, EAX=ticks */
	xchgw	%ax, %dx	/* EAX=0, EDX=ticks */
1:
	xorw	%ax, %ax
	movw	%ax, %ds
	movl	0x46c, %eax	/* initial tick */
	movl	%eax, %ecx	/* ECX=initial tick */
	testl	%edx, %edx
	js	1f
	addl	%edx, %eax	/* EAX=timeout tick */
	pushl	%eax

//	movzbl	%cs:0x03, %eax
	movzbl  %cs:(mbr_time - _start1), %eax
	orl	%eax, %eax
	jz	3f

        movw	$(hot_key_timeout_pre - _start1), %si
	
	
	pushl	%eax
	call	print_message
	popl	%eax

        movw	$(hot_key_timeout_num - _start1), %si
	
	
	call	print_decimal
3:
	movl	%ecx, %esi
	addl	$18, %esi

	popl	%eax
	jmp	3f
1:
	movl	%edx, %eax	/* EAX=0xffffffff */
	movl	%edx, %esi
3:
	movl	0x46c, %ebx	/* EBX=current tick */
	cmpl	%ecx, %ebx
	jnb	2f

	/* current tick is less than initial tick, this means the ticks have
	 * overflowed to the next day, and EBX is rather small. */
	xorl	%ecx, %ecx
	movl	%edx, %eax
	movl	$18, %esi
2:
	/* check if there is any key press. */
	pushl	%eax
	movb	$1, %ah
	int	$0x16
	pushw	%ax
	pushfw

	movb	$0x11, %ah
	int	$0x16
	jnz	1f
	popfw
	jnz	2f

	/* no, there is no key press. */

	popw	%ax
	popl	%eax

	cmpl	%esi, %ebx
	jb	4f
	pushl	%esi
	pushl	%eax
	pushl	%edx


	subl	%esi, %eax
	xorl	%edx, %edx
	movl	$18, %esi
	divl	%esi

        movw	$(hot_key_timeout_num - _start1), %si


	pushl	%ebx
	call	print_decimal
	popl	%ebx

	popl	%edx
	popl	%eax
	popl	%esi
	addl	$18, %esi
4:
	cmpl	%eax, %ebx	/* timeout? */
	jbe	3b		/* no, continue to wait */

	/* timeout reached, CF=0, no key pressed. */
	popl	%edx
	popl	%esi
	popw	%ds
	ret
1:
	popfw
2:
	/* yes, there is a key press. */
#if 0
	/* clear the keyboard buffer */
	movb	$1, %ah
	int	$0x16
	jz	1f	        /* no keys, end */
	movb	$0, %ah
	int	$0x16	        /* discard the key */
	jmp	1b
1:
#endif

	/* check if it is the desired key. */

//	xorw	%cs:0x04, %ax	/* CF=0 */     
	xorw	%cs:(mbr_hotkey - _start1), %ax
	popw	%ax
	je	1f
//	xorw	%cs:0x04, %ax	/* CF=0 */
	xorw	%cs:(mbr_hotkey - _start1), %ax
	jne	2f		/* not desired, return CF=0 */

	/* remove the desired key from the keyboard buffer. */

	movb	$0, %ah
	int	$0x16	        /* discard the key */
	jmp	3f
1:
	/* remove the desired key from the keyboard buffer. */

	movb	$0x10, %ah
	int	$0x16	        /* discard the key */
3:
	stc	/* CF=1, the desired key pressed */
2:
	popl	%eax
	popl	%edx
	popl	%esi
	popw	%ds
	ret

out_decimal:
	/*
	 * input: EAX = number, CS:SI = buffer
	 */

	pushl	%edx
	pushl	%ecx
	pushw	%bx

	movl	$10, %ecx
	movw	%si, %bx

1:
	xorl	%edx, %edx
	divl	%ecx
	addb	$'0', %dl
	movb	%dl, %cs:(%si)
	incw	%si
	orl	%eax, %eax
	jnz	1b

	pushw	%si

1:
	decw	%si
	cmpw	%bx, %si
	jbe	1f
	movb	%cs:(%si), %al
	xchgb	%al, %cs:(%bx)
	movb	%al, %cs:(%si)
	incw	%bx
	jmp	1b
1:

	popw	%si

	popw	%bx
	popl	%ecx
	popl	%edx
	ret

print_decimal:
	pushw	%si
	call	out_decimal

1:
	cmpb	$'\b', %cs:(%si)
	jz	2f
	movb	$' ', %cs:(%si)
	incw	%si
	jmp	1b
2:
	popw	%si
	call	print_message
	ret

#endif // defined(GRLDR_MBR) 

#if 0
modify_NTFS_boot_record:

	/* before the call:
	 *		AH= partition number
	 *		AL= 0xB6	; 0xB6 is opcode of "MOV DH,imm8"
	 *		DL= drive number
	 *
	 * on return:	CF=0 if there is NTFS boot record;
	 *		CF=1 otherwise. 
	 *		CF of flags_orig on the stack will set if CF=1
	 */

	/* 
	 *
	 * the current stack is:
	 *
	 * SP + 40	: DS
	 * SP + 38	: ES
	 * SP + 34	: EAX
	 * SP + 30	: ECX
	 * SP + 26	: EDX
	 * SP + 22	: EBX
	 * SP + 18	: ESP_temp
	 * SP + 14	: EBP
	 * SP + 10	: ESI
	 * SP +  6	: EDI
	 * SP +  4	: flags_orig
	 * SP +  2	: SI		; SI points to old entry in MBR
	 * SP		: return_IP
	 *
	 */

	/* DS=ES=FS_BOOT */

	/* change NTLDR to GRLDR */

	/* check GR or NT or anything else */

	pushw	%ax

	movw	$0x200, %si
	lodsw
	cmpw	$5, %ax
	jne	1f		/* failure */
	lodsw
	testb	%ah, %ah	/* high byte of unicode ASCII should be 0 */
	jne	1f		/* failure */

	/* 'N' should be a capital letter */

	cmpb	$0x41, %al	/* Less than 'A' */
	jb	1f		/* failure */
	cmpb	$0x5A, %al	/* Greater than 'Z'*/
	ja	1f		/* failure */

	xchgw	%ax, %cx	/* save AX to CX. CL='N' */

	lodsw
	testb	%ah, %ah	/* high byte of unicode ASCII should be 0 */
	jne	1f		/* failure */

	/* 'T' should be a capital letter */

	cmpb	$0x41, %al	/* Less than 'A' */
	jb	1f		/* failure */
	cmpb	$0x5A, %al	/* Greater than 'Z'*/
	ja	1f		/* failure */

	movb	%al, %ch	/* save AL to CH. CH='T' */

	lodsw
	cmpw	$0x4C, %ax	/* 'L' */
	jne	1f		/* failure */
	lodsw
	cmpw	$0x44, %ax	/* 'D' */
	jne	1f		/* failure */
	lodsw
	cmpw	$0x52, %ax	/* 'R' */
	jne	1f		/* failure */
	lodsw
	cmpw	$0x04, %ax	/* length of "$I30" */
	jne	1f		/* failure */
	lodsw
	cmpw	$0x24, %ax	/* '$' */
	jne	1f		/* failure */
	lodsw
	cmpw	$0x49, %ax	/* 'I' */
	jne	1f		/* failure */
	lodsw
	cmpw	$0x33, %ax	/* '3' */
	jne	1f		/* failure */
	lodsw
	cmpw	$0x30, %ax	/* '0' */
	jne	1f		/* failure */

	
	/* assume it is NT bootsector. first, find "NTLDR". CX holds "NT" */
	movw	$0x0100, %di
	movb	%cl, %al	/* AL="N" */
	movb	$1, %ah		/* AH=Carry for SAHF below */
	movl	$0x52444c00, %ebx	/* "LDR" */
	movb	%ch, %bl		/* 'T' */
	movw	$0x00fa, %cx

	/* now AL holds 'N' and BL holds 'T' */

	//cld			/* already upward */
3:
	repnz scasb		/* find "N" */
	jcxz	4f		/* gets the end, exit */
	cmpl	%ebx, (%di)	/* is it "NTLDR"? */
	jnz	3b		/* no, continue to find */

	/* "NTLDR" is found, so we believe it is NT boot sector. */

	movw	$0x5247, -1(%di)	/* change "NT" to "GR" */

	/* CF=0 for now */

	lahf			/* Load Flags into AH */
				/* AH = SF:ZF:xx:AF:xx:PF:xx:CF */
				/* AH = binary xxxxxxx0 */
	jmp	3b
4:
	sahf		/* Store AH into flags SF ZF xx AF xx PF xx CF */

	/* CF=0 means "NTLDR" is found, CF=1 means "NTLDR" is not found. */

	jc	1f		/* failure */

	movl	$0x00520047, 0x202	/* change to "G R L D R" */

	/* check NT 4.0 */

	movw	$0x406, %si
	movl	(%si), %ebx		/* NT 4.0 */
	cmpl	$0x03E8B800, %ebx	/* MOV AX, 03E8 */
	jnz	3f

	movl	0x84, %ebx
	cmpl	$0x680007E8, %ebx	/* call 008e; push (0D00) */
	jnz	3f


	movl	0xE8, %ebx
	cmpl	$0x13CD80B2, %ebx	/* "B2 80"="mov DL, 80", "CD 13"="int 13" */
	jnz	3f

	popw	%ax
	movw	%ax, 4(%si)

	movl	$0x68909090, %ebx	/* nop;nop;nop;push (0D00) */
	movl	%ebx, 0x84

	movb	%dl, 0xE9		/* modify drive number */

	/* modify NTFS boot record */
	movb	$0xea, %al	        /* ljmp, hand over the control to supervisor */
	movb	%al, 0x122

	movw	$MONITOR, %ax	        /* offset for ljmp */
	movw	%ax, 0x123
	xorw	%ax, %ax
	movw	%ax, 0x125

        movw	$(NTFS4_message - _start1), %si

	
	call	print_message	        /* CS:SI points to message string */
	clc
	ret
3:
	/* check NT 5.0 */

	movw	$0x44b, %si
	movl	(%si), %ebx		/* NT 5.0 */
	cmpl	$0x03E8B800, %ebx	/* MOV AX, 03E8 */
	jz	2f

	movw	$0x479, %si
	movl	(%si), %ebx		/* NT 5.1 SP2 */
	cmpl	$0x03E8B800, %ebx	/* MOV AX, 03E8 */
	jnz	1f
2:
	movl	0x71, %ebx
	cmpl	$0x680053E8, %ebx	/* call 00C7; push (0D00) */
	jnz	1f

	//movw	0x183, %bx		/* CR LF at begin of "A disk read error occurred." */
	movb	0x1F8, %bl
	movb	$1, %bh
	movw	(%bx), %bx
	cmpw	$0x0A0D, %bx		/* CR LF */
	jnz	1f

	movb	0x1F9, %bl
	movb	$1, %bh
	movw	(%bx), %bx
	cmpw	$0x0A0D, %bx		/* CR LF */
	jnz	1f

	movb	0x1FA, %bl
	movb	$1, %bh
	movw	(%bx), %bx
	cmpw	$0x0A0D, %bx		/* CR LF */
	jnz	1f

	popw	%ax
	movw	%ax, 4(%si)

	movl	$0x68909090, %ebx	/* nop;nop;nop;push (0D00) */
	movl	%ebx, 0x71

	/* change CRLF in NTFS error messages to spaces */
	movw	$0x2020, %ax
	movb	0x1F8, %bl
	movb	$1, %bh
	movw	%ax, (%bx)	        // 0x183
	movb	0x1F9, %bl
	movb	$1, %bh
	movw	%ax, (%bx)	        // 0x1A0
	movb	0x1FA, %bl
	movb	$1, %bh
	movw	%ax, (%bx)	        // 0x1B3

	/* modify NTFS boot record */
	movb	$0xEA, %al	        /* ljmp, hand over the control to supervisor */
	movb	%al, 0x167

	movw	$MONITOR, %ax	        /* offset for ljmp */
	movw	%ax, 0x168

	xorw	%ax, %ax
	movw	%ax, 0x16A

	cmpw	$0x44b, %si
	jne	2f
	
        movw	$(NTFS5_message - _start1), %si
	
	
	jmp	3f
2:

        movw	$(NTFS5p_message - _start1), %si

	
3:
	call	print_message	/* CS:SI points to message string */
	clc
	ret
1:
	/* NTFS boot record not found. */

        movw	$(NTFS_no_boot_record_message - _start1), %si

	
	call	print_message	/* CS:SI points to message string */

	popw	%ax
	popl	%eax			/* return_IP and SI */
	popfw
	stc
	pushfw
	pushl	%eax			/* return_IP and SI */
	ret
#endif

move_helper:
	
	/* called only once and only when the boot loader loaded this code */
	pushw	%si


	movw	$0x200, %si	        /* move from the 2nd sector */
	movw	%si, %di

        movw    $((pre_stage2_start - _start1 - 0x200) / 2), %cx 
 
        
	cld
	repz movsw

	popw	%si
	ret

filesystem_boot:
	/* The partition boot record successfully modified, just boot it */

	/*
	 * The boot might fail, but we want to take back the control.
	 * So we save the registers now.
	 */
	pushw	%ds
	pushw	%es
	pushal

	/* DS=CS=GRLDR_CS, ES=FS_BOOT */

	/* save GRLDR_CS */

	movw	%es, %bx	# save old ES to BX

	cli
	
        lgdt	gdt - _start1
 	
	
	movl	%cr0, %eax
	orb	$1, %al
	movl	%eax, %cr0

	movw	$8, %si
	movw	%si, %es

	xorl	%esi, %esi
	xorl	%edi, %edi
	movl	$(0x9000 / 4), %ecx

	cld
	repz movsl

	movw	$16, %si
	movw	%si, %es

	andb	$0xfe, %al
	movl	%eax, %cr0

	movw	%bx, %es	# restore ES from BX

	/* move FS_BOOT:0000 to 0:7c00 */
#if 0
	/* for single sector boot record */
	movw	$0x0200, %cx	/* move 2 sectors, the old FS_BOOT:0000 will keep untouched.  */
#else
	/* for 4-sector NTFS boot record */
	movw	$0x0400, %cx	/* move 4 sectors, the old FS_BOOT:0000 will keep untouched.  */
#endif
	xorw	%si, %si
	pushw	%si	/* SI=0, for the segment of 0000:7c00 */
	movw	$0x7c00, %di
	pushw	%di	/* DI=0x7c00, for the offset of 0000:7c00 */
	pushw	%es	/* ES=FS_BOOT */
	popw	%ds	/* DS=FS_BOOT */
	pushw	%si	/* SI=0 */
	popw	%es	/* ES=0 */
	cld
	repz movsw

	movw	$MONITOR, %di
	
        movw	$(restore_GRLDR_CS - _start1), %si
 	
	
	movw	$((gdt_end - restore_GRLDR_CS) / 4), %cx
	cld
	repz cs movsl		/* CS segment override prefix(=0x2E) */

	pushw	%es	/* ES=0 */
	popw	%ds	/* DS=0 */
	sti
	lret	//ljmp	$0, $0x7c00


press_space_bar_string:
	.ascii	"\r\nPress space bar\0"

press_hot_key_pre:
	.ascii "\r\nPress \0"

press_hot_key_sub:
	.ascii	" to start GRUB, any other key to boot previous MBR ...\0"
press_hot_key_sub1:
	.ascii	" to boot previous MBR, any other key to start GRUB ...\0"

hot_key_timeout_pre:
	.ascii "\r\nTimeout: \0"

hot_key_timeout_num:
	.ascii "   \b\b\b\0"

continue_string:
	.ascii	"\r\nInvalid previous MBR. Press any key to start GRUB ...\0"


prev_MBR_string:
	.ascii	" to hold the screen, any other key to boot previous MBR ...\0"

Error_while_reading_string:
	.ascii	"\r\nError while reading MBR of \0"

drive_number_string:
	.ascii	" in partition table of drive (hd0 ) \0"

Cannot_find_GRLDR_string:
message_string_helper:
	.ascii	"\r\nCannot find \0"
ctrl_alt_del_string:
	.ascii	" in all drives. Press Ctrl+Alt+Del to restart.\0"

partition_message:
	.ascii	"\r\nTry (hd0,0 ) : \0"

EXT2_message:
	.ascii	"EXT2: \0"

NTFS5_message:
	.ascii	"NTFS5: \0" 
exfat_message:
	.ascii	"EXFAT: \0"	
FAT32_message:
	.ascii	"FAT32: \0"
FAT16_message:
	.ascii	"FAT16: \0"
FAT12_message:
	.ascii	"FAT12: \0"
non_MS_message:
	.ascii	"non-MS: skip \0"
extended_message:
	.ascii	"Extended: \0"
invalid_message:
	.ascii	"invalid or null \0"
No_floppy:
	.ascii	"\r\nNo floppy \0"

press_hot_key_name:

	/* hot key name, the address is (grldr_signature - 16) */

	.ascii  "hot-key\0"

	/* grldr.mbr is larger than 8K. */

	######################################################################
	# The installer should setup the long integer at offset 0x1FFC as
	# the unique disk signature. The same signature value must be also
	# placed into the disk_serial_number_structure in the first sector(the
	# MBR sector). You can easily locate the structure through the pointer
	# at offset 0x1FF8.
	#
	# For GRLDR.MBR the default disk serial number is the grldr.mbr
	# signature(0x93cb4d05). You should change it according to the
	# date-time value just at the time when you run the installer.
	######################################################################

geometry_tune:

	/* on call:
	 *		CS=DS=SS
	 *		ES=FS_BOOT
	 *
	 * on return:
	 *		CL	max sector number
	 *		DH	max head number
	 *		DS	changed
	 *		ES	changed
	 *		AX	changed
	 *		BX	changed
	 *		CX	changed
	 *   byte at CS:[08]	updated on success(Smax)
	 *   byte at CS:[09]	updated on success(Hmax)
	 */

#define GEOMTUNE_SEG 0x5000
	/* geometry_tune uses 64KiB buffer at this segment */
#define GEOMTUNE_BUFFERSEG 0x6000
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
//////////////////////////////////////////////////////////////////////////////

	/* initialize passed-in values */

	/* print BIOS geometry */

        movzbw	%cs:(Sectors_passed_in - _start1), %cx
	
	pushw	%cx
	
        movzbw	%cs:(Heads_passed_in - _start1), %cx
	incw    %cx
	
	pushw	%cx
	movzbw	%dl, %cx
	pushw	%cx
	
        pushw	$(BIOS_geom_string - _start1)
	
	
	call	realmode_printf
	addw	$8, %sp

#ifdef USE_OLD_GEOMETRY_TUNE
//////////////////////////////////////////////////////////////////////////////

	/* find Max sector by reading each sector on the first track. */

	/* try the passed-in value first */

        movzbw	%cs:(Sectors_passed_in - _start1), %cx	/* cylinder=0 */
	
	
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
	movw	$0x5000, %ax
	movw	%ax, %es
	movw	%ax, %ds
	xorw	%bx, %bx
	movw	$0x201, %ax	/* read 1 sector */
	movb	$0, %dh		/* head=0 */
	pushaw
	movw	%bx, %si
	movw	%bx, %di
//        int $0x13
	call	int13
	popaw
	ret

2:
	cmpw	$63, %cx
	jbe	2f
	movw	$63, %cx
2:
	/* CX=Max Sector */
//	movw	%cx, %cs:(Smax_tuned - _start1)

        movb	%cl, %cs:(Smax_tuned - _start1)

        

//////////////////////////////////////////////////////////////////////////////
#if 0
	/* check if we can read sectors across the track boundary */

	/* first, read a track plus one sector */

        movb	$0, %cs:(cross_track - _start1)

	

	movw	$1, %cx		/* sector 1, cylinder 0 */
	movb	$0, %dh		/* head 0 */
	
        movb	%cs:(Smax_tuned - _start1), %al	/* sectors to read */
	
	
	incw	%ax		/* read 1 more sector */
	movb	$2, %ah		/* READ */
	movw	$0x5000, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
//	int	$0x13
	call	int13
	popaw
	jc	1f		/* cross-track read is not supported */

	/* read again normally, only the track */

        movb	%cs:(Smax_tuned - _start1), %al	/* sectors to read */
	
	
	movb	$2, %ah		/* READ */
	movw	$0x5800, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
//	int	$0x13
	call	int13
	popaw
	jc	2b		/* failure */

	/* compare the two tracks */

	pushw	%cx
	pushw	%si
	pushw	%di

        movzbw	%cs:(Smax_tuned - _start1), %cx	/* sectors */
	
	
	shlw	$7, %cx				/* dwords */
	movw	$0x5000, %ax
	movw	%ax, %ds
	movw	$0x5800, %ax
	movw	%ax, %es
	xorw	%si, %si
	xorw	%di, %di
	cld
	repz cmpsl
	popw	%di
	popw	%si
	popw	%cx
	jne	1f		/* cross-track read is not supported */ 
	
        movb	$1, %cs:(cross_track - _start1)
		
	
1:
#endif
//////////////////////////////////////////////////////////////////////////////
#if 0
	/* find Max head by reading sector 1 on each track of cylinder 0. */

	movb	$0xFF, %dh	/* head=Max possible */
1:
	movw	$0x5000, %ax
	movw	%ax, %es
	movw	%ax, %ds
	xorw	%bx, %bx
	movw	$0x201, %ax	/* read 1 sector */
	movw	$1, %cx		/* cylinder=0, sector=1 */
	pushaw
	movw	%bx, %si
	movw	%bx, %di
//	int	$0x13
	call	int13
	popaw
	jnc	1f		/* found Max head */
	decb	%dh
	cmpb	$0xFF, %dh
	jne	1b
	movb	$1, %ah		/* failure */
	ret
1:

	/* DH=Max head */
	
        movb	%dh, %cs:(Hmax_tuned - _start1)
		
	
#endif
//////////////////////////////////////////////////////////////////////////////

	/* tune Hmax */

	/* First, try the passed-in value */ 
        movb	%cs:(Heads_passed_in - _start1), %dh
	incb    %dh	
	
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

        cmpb	%cs:(Hmax_tuned - _start1), %dh

	
	ja	2f		/* this should not happen */
	je	5f
#endif

	incb	%dh		/* Next Hmax */
	jnz	1b

	/* Hmax=0xFF */
4:
	/* got Hmax=DH-1 */

	decb	%dh
	
        movb	%dh, %cs:(Hmax_tuned - _start1)
		
	
5:
	/* Hmax is tuned ok. */

	cmpb	$0xFF, %dh
	jne	1f
	/* consider Hmax=0xFF as a failure! Use the passed-in value. */
	
        movb	%cs:(Heads_passed_in - _start1), %dh
	incb    %dh	
	
	testb	%dh, %dh
	jnz	4f
	decb	%dh
4:
	decb	%dh
	
        movb	%dh, %cs:(Hmax_tuned - _start1)
		

1:
//////////////////////////////////////////////////////////////////////////////

	/* tune Smax */

	/* First, try the passed-in value */
	
        movb	%cs:(Sectors_passed_in - _start1), %cl

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
	

        cmpb	%cs:(Smax_tuned - _start1), %cl
		
	
	jb	1b
	cmpb	$63, %cl
	jbe	4f
	movb	$63, %cl

4:
	/* got Smax=CL */
        movb    %cl, %cs:(Smax_tuned - _start1)
	
	

	/* Smax is tuned ok. */

//////////////////////////////////////////////////////////////////////////////

	/* print tuned geometry */

        movzbw	%cs:(Smax_tuned - _start1), %cx
	
	
	pushw	%cx 
	
        movzbw	%cs:(Hmax_tuned - _start1), %cx
		
	
	incw	%cx
	pushw	%cx
	movzbw	%dl, %cx
	pushw	%cx
	
        pushw	$(TUNE_geom_string - _start1)
		
	
	call	realmode_printf
	addw	$8, %sp

	/* return with success */
	
	movw	%cx, %cs:(Sectors_passed_in - _start1)	/* Smax */
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

	movb	$1, %cl		/* sector 1=the leading sector */
	movb	$2, %ah		/* READ */
	
        movb	%cs:(Smax_tuned - _start1), %al	/* sectors to read */
		
	
	movw	$0x5000, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
//	int	$0x13
	call	int13
	popaw
	//jc	2f		/* failure */
	incb	%ch
	jc	4f		/* considered OK */
	decb	%ch

	/* read beginning track of this cylinder */

	movb	$1, %cl		/* sector 1=the leading sector */
	movb	$2, %ah		/* READ */
	
        movb	%cs:(Smax_tuned - _start1), %al	/* sectors to read */
	
	
	movw	$0x5800, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
	movb	$0, %dh
//	int	$0x13
	call	int13
	popaw
	jc	2f		/* failure */

	incb	%ch		/* next cylinder */

	/* compare the two tracks */
	call	cmp_track
	je	4f		/* ok, try next cylinder */

	/* read beginning track of the next cylinder */

	movb	$1, %cl		/* sector 1=the leading sector */
	movb	$2, %ah		/* READ */ 
	
        movb	%cs:(Smax_tuned - _start1), %al	/* sectors to read */
		

	movw	$0x5800, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
	movb	$0, %dh
//	int	$0x13
	call	int13
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

        movzbw	%cs:(Smax_tuned - _start1), %cx	/* sectors */
	
	
	shlw	$7, %cx				/* dwords */
	movw	$0x5000, %ax
	movw	%ax, %ds
	movw	$0x5800, %ax
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

        movb	$16, %cs:(Smax_count - _start1)
	movb	$0, %ch		/* cylinder: 0 - 6 */
	movb	$0, %dh		/* head: 0 - Hmax */
6:
	/* read beginning sector of this track. */

	movw	$0x201, %ax	/* read 1 sector */
	movw	$0x5800, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movb	$1, %cl		/* beginning sector */
	movw	%bx, %si
	movw	%bx, %di
//	int	$0x13
	call	int13
	popaw
	jc	2f		/* failure */

	/* read ending sector of this track. */

	movw	$0x202, %ax	/* read 2 sectors */
	movw	$0x5000, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
//	int	$0x13
	call	int13
	popaw
	//jc	2f		/* failure */

	/* locate the next track. */

	pushfw			/* save CF */
	
        cmpb	%cs:(Hmax_tuned - _start1), %dh
	
	
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
	movw	$0x5800, %bx
	movw	%bx, %es
	movw	%bx, %ds
	xorw	%bx, %bx
	pushaw
	movw	%bx, %si
	movw	%bx, %di
	movb	$1, %cl		/* sector 1=the leading sector */
//	int	$0x13
	call	int13
	popaw
	jc	2f		/* failure */

	/* compare the 2 sectors */
	call	cmp_sector
	jne	3f		/* Next Smax */
4:
        decb	%cs:(Smax_count - _start1)       
	jz	6f

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
	movw	$0x5020, %ax
	movw	%ax, %ds
	movw	$0x5800, %ax
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
	movzbw	%cs:(Sectors_passed_in - _start1), %cx	/* cylinder=0 */
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
//	int	$0x13
	call	int13
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
//	int	$0x13
	call	int13
	setnc	%ah
	cmpb	GEOMTUNE_read_ax, %al
	sete	%al
	andb	%ah, %al	/* 1=no error and read all required sectors */
	movb	%al, GEOMTUNE_next_read	/* save boolean value for later use */
	popaw

	/* read current cylinder, head 0 */
	pushaw
	movw	$GEOMTUNE_H_buf_cch0, %bx
//	int	$0x13
	call	int13
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
//	int	$0x13
	call	int13
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
	decb	%dh		/* Error on cylinder 0 = end of head. Adjust readable head number */
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
	movw	$(GEOMTUNE_equal +1), %di
	movw	GEOMTUNE_Hmax_readable, %cx
	incw	%cx
	xorw	%ax, %ax
	repz	scasb		/* skip heads that have different data from head0 */
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
//	int	$0x13
	call	int13
	popaw
	jc	2f
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
//	int	$0x13
	call	int13
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
	subw	$(GEOMTUNE_equal +1), %di
        movw	%di, %cx
        movb    %cl, %cs:(Sectors_passed_in - _start1)
        pushw   %cx
	movb	GEOMTUNE_Hmax_tuned, %cl
	movb    %cl, %cs:(Heads_passed_in - _start1)
	incb    %cl
	pushw   %cx
	movzbw	%dl, %cx
	pushw	%cx
	
        pushw	$(TUNE_geom_string - _start1)
		
	
	call	realmode_printf
	addw	$8, %sp
	movb	$0, %ah		/* success */
	ret
2:
	movb	$1, %ah		/* failure */
	ret
#endif /* ndef USE_OLD_GEOMETRY_TUNE */

//////////////////////////////////////////////////////////////////////////////



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
 * 		pushw	$format_string - _start1
 *		call	realmode_printf
 * 		addw	$(2*(N+1)), %sp
 * 
 * where _start1 should be the base of the CS segment,
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

	#; %d, %x, %X

	cs lodsb
	testb	%al, %al
	jz	1f
	cmpb	$'d', %al
	movw	$10, %bx		# base 10
	jz	4f
	cmpb	$'x', %al
	jz	3f
	cmpb	$'X', %al
	jne	1b			# unkown directive, continue
3:
	/* print hexa number */
	movw	$16, %bx		# base 16
4:
	/* print decimal or hexa number */
	pushl	%edi

	xorl	%edi, %edi
	xorw	%cx, %cx		# count the digits
	movw	(%bp), %ax
5:
	xorw	%dx, %dx
	divw	%bx			# AX=quo, DX=rem
	movw	%dx, %di
	rorl	$4, %edi
	incw	%cx
	testw	%ax, %ax		# end?
	jnz	5b

	/* print the digits in EDI */
	xorw	%bx, %bx	/* video page 0 */
5:
	roll	$4, %edi
	movw	%di, %ax		# get digit in AL
	andb	$0x0f, %al
	cmpb	$9, %al
	jbe	6f
	addb	$7, %al			# A, B, C, D, E, F
6:
	addb	$0x30, %al
	movb	$0x0e, %ah	/* print it */
	int	$0x10		/* via TTY mode */
	loop	5b

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
//////////////////////////////////////////////////////////////////////////////

BIOS_geom_string:
        .string	"\r\nBIOS: Drive=0x%X, H=%d, S=%d\r\n"
TUNE_geom_string:
	.string	"TUNE: Drive=0x%X, H=%d, S=%d\r\n"

#ifdef USE_OLD_GEOMETRY_TUNE	
Smax_tuned:
	.byte	0
Hmax_tuned:
	.byte	0	
Smax_count:
        .byte	0
cross_track:
	.byte	0
#endif

//////////////////////////////////////////////////////////////////////////////

	######################################################################
	# External modifiers may setup a long integer at offset 0x1FFC as the
	# unique disk signature. The same signature value must be also placed
	# into the disk_serial_number_structure in the first sector(the MBR
	# sector). You can easily locate the structure through the pointer at
	# offset 0x1FF8.
	#
	# For GRLDR the default disk serial number is the grldr signature
	# ("GR" 0x55 0xAA). Generally you needn't change it, though you are'
	# allowed to change it through an external modifier.
	###################################################################### 
	
                          
	. = _start1 + 0x1FF8

	/* point to disk serial number in the first sector, i.e., the MBR
	 * sector. The program never access this pointer. It can be used by
	 * an external program to easily locate the disk serial number at MBR.
	 */

	.word	disk_serial_number_structure - _start1 + 5 

	
                                                                          
	. = _start1 + 0x1FFA 
	

	/* version word of grldr.mbr, the address is (grldr_signature - 2) */

	.word   4

grldr_signature:
	.byte	0x47, 0x52, 0x55, 0xaa	/* signature for helper */

	.align	0x200

	. = _start1 + 0x2000

#endif  //(! defined(GRLDR_INSTALL))  

#if  defined(GRLDR_INSTALL)


/* ********************  CDROM_bin start  ****************************** */ 

Entry_cdrom:

	cli
	xorw	%bx, %bx
	movw	%bx, %ss
	movw	$0x7c00, %sp            /* temp safe stack space */
	sti	
	call	1f
1:
	popw	%bx			/* Instruction Pointer of 1b */
	subw	$(1b - Entry_cdrom), %bx        /* CS:BX=Entry_cdrom */
	shrw	$4, %bx
	movw	%cs, %ax
	addw	%ax, %bx		/* BX:0000=Entry_cdrom */
	/* Let CS:0000=Entry_cdrom */
	pushw	%bx
	 pushw	$(1f - Entry_cdrom)
	lret
1: 
	movw	%bx, %ds
	movw	%bx, %es       
        
        //Judgment udf partition
	movw	$0x1000, %bx                            /* buffer offset */
	movl	$0x10, %eax                             /* lba of sectors */
1:
	call	load_cdrom
	movw	$5, %cx
	leaw	1(%bx), %si
	movw	$(udf_tag - Entry_cdrom), %di
	repz cmpsb
	je	1f
	incl	%eax
	cmpl	$0x20, %eax
	jae	iso     
	jmp	1b

1:      //udf        
	movl	$0x100, %eax                            //AnchorVolume
	call	load_cdrom
	movl	0x14(%bx), %eax                         //MainVolume_ExtentLocation
1:
	call	load_cdrom
	cmpw	$5, (%bx)                               //Partition
	je	1f
	incl	%eax
	jmp	1b
1:        
	movl	0xbc(%bx), %eax                         //FileSet
	movl	%eax, (udf_partition_start - Entry_cdrom)
	call	load_cdrom
	movl	0x194(%bx), %eax
	addl	(udf_partition_start - Entry_cdrom), %eax       //RootDirectoryLocation 
	call	load_cdrom
	movw	0xa8(%bx), %si
	movl	0xb4(%bx, %si), %eax
	addl	(udf_partition_start - Entry_cdrom), %eax       //FileIdentifier         
	call	load_cdrom
5:
	testb	$2, 0x12(%bx)                           //Directory?
	jne	1f                                      	//yes
	movl	0x18(%bx), %eax                         //lba
	movw	0x24(%bx), %si                          //LengthofImplementationUse
	movb	0x13(%bx), %cl                          //NameLength
	orb	%cl, %cl
	je	1f
	decb	%cl
	movb	$0, (utf16_tag - Entry_cdrom)           //utf8
	cmpb	$8, 0x26(%bx, %si)                      //utf8?
	je	2f                                      	//yes
	orb	$1, (utf16_tag - Entry_cdrom)           	//utf16
	shrb	$1, %cl                                 //NameLength/2
2:        
	leaw	0x27(%bx, %si), %si                    	//Name Address
	cmpb	$1, (utf16_tag - Entry_cdrom)           //utf16?
	jne	2f                                      	//no
	incw	%si
2:        
	movw	$(cdrom_name - Entry_cdrom), %di       
3:	
	cmpb	$0, (%di)
	je 2f
	andb	$0xdf, (%si)                            /* Changed to upper case */
	cmpsb
	jne	1f
	cmpb	$1, (utf16_tag - Entry_cdrom)           //utf16?
	jne	3b                                      	//no
	incw	%si
	jmp	3b
2:                                           

	//Found files
	addl	(udf_partition_start - Entry_cdrom), %eax       //FileEntry
	movw	$0x1000, %bx                            /* buffer offset */
	call	load_cdrom
	movw	0xa8(%bx), %si
	movl	0xb0(%bx, %si), %ecx                    // ecx = File length in bytes */               
	movl	0xb4(%bx, %si), %eax
	addl	(udf_partition_start - Entry_cdrom), %eax       //lba of grldr
	jmp	cdrom_ok        

1:      //File not found
	movzbw  0x13(%bx), %cx                      	//NameLength
	addw	0x24(%bx), %cx                          //+LengthofImplementationUse
	addw	%cx, %bx                                //+NameBaseAddress
	addw	$0x26, %bx
	movw	$5, %cx
1: 
	cmpw	$0x101, (%bx)                           //UDF_FileIdentifier
	je	5b                                      	//Continue
	incw	%bx
	loop	1b
	jmp	4f

iso:
	movw	$0x1000, %bx                            /* buffer offset */
	movl	$0x10, %eax                             /* Read the main sector descriptor */
	call	load_cdrom
	movl	0x9e(%bx), %eax                         /* Root */
	call	load_cdrom
5:	                                                
	cmpb	$0, (%bx)                               /* Record length in bytes */
	je	4f                                      	/* End */
	cmpb	$0, 0x19(%bx)                           /* type */
	jne	3f
	xorw	%si, %si
	movw	$5, %cx                                 /* File name length */	
	cmpb	$5, 0x20(%bx)                           /* grldr file name length */
	je	1f
	cmpb	$7, 0x20(%bx)
	je	2f
	cmpb	$8, 0x20(%bx)
	jne	3f
	cmpb	$0x2e, 0x26(%bx)
	jne	3f
	incw	%si
2:
	cmpw	$0x313b, 0x26(%bx,%si)
	jne	3f
1:        
	leaw	0x21(%bx), %si                          /* file name */
	movw	$(cdrom_name - Entry_cdrom), %di        /* "GRLDR.;1" */
1:        
	andb	$0xdf, (%si)                            /* Changed to upper case */
	cmpsb
	jne	3f
	loop	1b
	movl	0x0a(%bx), %ecx                         /* File length in bytes */
	movl	2(%bx), %eax                            /* File logical sector */
	jmp	cdrom_ok
3:
	movzbw  (%bx), %cx
	addw	%cx, %bx                                /* First record */
	jmp	5b


4:
	movw	$(msg_BootError_cdrom - Entry_cdrom), %si
1:	
	lodsb	(%si), %al	/* get token */
	cmpb	$0, %al		/* end of string? */
	je	1f
	movb	$0x0e, %ah	/* print char in AL */
	int	$0x10		/* via TTY mode */	
	jmp	1b
	
1:      
	jmp	1b


cdrom_ok:
	/* EAX = first sector(LBA) of GRLDR */
	/* ECX = File number of byte */

	xorw	%bx, %bx
	movw	$0x2000, %di
	movw	%di, %es
1:	
	pushw	%es
	call	load_cdrom
	popw	%di
	leaw	0x80(%di), %di
	movw	%di, %es
	incl	%eax
	subl	$0x800, %ecx
	jnl	1b
	ljmp	$0x2000, $0  
	
	
load_cdrom:
	/* input:	EAX	LBA
	 * output:      ES:BX
	 */
	pushaw 
	movw	$0xe00, %si
	movl	$0x00010010, (%si)     /* number of sectors */
	movw	%bx, 4(%si)            /* buffer offset */
	movw	%es, 6(%si)            /* buffer segment */
	movl	%eax, 8(%si)           /* LBA lo 32 bits */
	movl	$0, 12(%si)            /* LBA hi 32 bits */ 
	movb	$0x42, %ah
	pushw	%ds
	pushw	%es
	int	$0x13
	popw	%es
	popw	%ds
	popaw
	ret	

        
        
        
	. = Entry_cdrom + 0x1e0       
        
msg_BootError_cdrom:
	.ascii	"No "        

cdrom_name:
	.ascii	"GRLDR\0\0\0\0\0\0\0\0"

udf_tag:
	.ascii  "BEA01"  
        
utf16_tag:
	.byte   0         
        
udf_partition_start:
	.long   0                
       

	. = Entry_cdrom + 0x1fe

	.word	0xAA55	
        
	. = Entry_cdrom + 0x200 	
	
/* ********************  CDROM_bin end  ****************************** */ 

/* ********************  HD_UDF_bin start  ****************************** */ 
Entry_hd_udf:
 
	jmp 1f
	
	. = Entry_hd_udf + 2
	.byte	0x90
	.ascii  "UDF"
	
hd_utf16_tag:
	.byte   0     
        
hd_udf_partition_start:
	.long   0  	

	. = Entry_hd_udf + 0xc	
partition_start:
 	.ascii  "hide" 	
	
1:
	cli
	xorw	%bx, %bx
	movw	%bx, %ss
	movw	$0x7c00, %sp            /* temp safe stack space */
	sti	
	call	1f
1:
	popw	%bx			/* Instruction Pointer of 1b */
	subw	$(1b - Entry_hd_udf), %bx        /* CS:BX=Entry_hd_udf */
	shrw	$4, %bx
	movw	%cs, %ax
	addw	%ax, %bx		/* BX:0000=Entry_hd_udf */
	/* Let CS:0000=Entry_hd_udf */
	pushw	%bx
	pushw	$(1f - Entry_hd_udf)
	lret
1: 
	movw	%bx, %ds
	movw	%bx, %es
	movw	$1, %bp
	movw	$0x1000, %bx
      
	movl	(partition_start - Entry_hd_udf), %eax
	testl	%eax, %eax
	jne 1f
	movb	$0xff, %dh
1:
	addl	$0x100, %eax                            //AnchorVolume
	call	load_hd_udf
	movl	0x14(%bx), %eax                         //MainVolume_ExtentLocation
	addl	(partition_start - Entry_hd_udf), %eax
1:
	call	load_hd_udf
	cmpw	$5, (%bx)                               //Partition
	je	1f
	incl	%eax
	jmp	1b
1:        
	movl	0xbc(%bx), %eax                         //FileSet
	addl	(partition_start - Entry_hd_udf), %eax
	movl	%eax, (hd_udf_partition_start - Entry_hd_udf)
	call	load_hd_udf
	movl	0x194(%bx), %eax
	addl	(hd_udf_partition_start - Entry_hd_udf), %eax       //RootDirectoryLocation 
	call	load_hd_udf
	
	movw	$0x09c0, %di
	call	read_fileEntry
	jc	4f
	
//(101)
	movw	$0x2000, %bx
5:
	testb	$2, 0x12(%bx)                           //Directory?
	jne	1f                                      	//yes
	movl	0x18(%bx), %eax                         //lba
	movw	0x24(%bx), %si                          //LengthofImplementationUse
	movb	0x13(%bx), %cl                          //NameLength
	orb	%cl, %cl
	je	1f
	decb	%cl
	movb	$0, (hd_utf16_tag - Entry_hd_udf)				//utf8
	cmpb	$8, 0x26(%bx, %si)              				//utf8?
	je	2f                               					//yes
	orb	$1, (hd_utf16_tag - Entry_hd_udf)     		//utf16
	shrb	$1, %cl                                 //NameLength/2
2:        
	leaw	0x27(%bx, %si), %si                    	//Name Address
	cmpb	$1, (hd_utf16_tag - Entry_hd_udf)   		//utf16?
	jne	2f                               					//no
	incw	%si
2:        
	movw	$(hd_udf_name - Entry_hd_udf), %di			//"GRLDR\0"      
3:	
	cmpb	$0, (%di)
	je 2f
	andb	$0xdf, (%si)                            /* Changed to upper case */
	cmpsb
	jne	1f
	cmpb	$1, (hd_utf16_tag - Entry_hd_udf)   		//utf16?
	jne	3b                                				//no
	incw	%si
	jmp	3b
1:
	//File not found
	movzbw  0x13(%bx), %cx                      	//NameLength
	addw	0x24(%bx), %cx                          //+LengthofImplementationUse
	addw	%cx, %bx                                //+NameBaseAddress
	addw	$0x26, %bx
	movw	$5, %cx
1: 
	cmpw	$0x101, (%bx)                           //UDF_FileIdentifier
	je	5b                                      	//Continue
	incw	%bx
	loop	1b
	jmp	4f
	
2:
	//Found files
	addl	(hd_udf_partition_start - Entry_hd_udf), %eax       //FileEntry
	movw	$0x1000, %bx                            /* buffer offset */
	call	load_hd_udf
	movw	$0x2000, %di
	call	read_fileEntry
	jc	4f
	ljmp	$0x2000, $0  


load_hd_udf:
	/* input:		EAX	LBA
	 * output: 	ES:BX
	 */
	pushaw 
	movw	%es, %di
1:
	pushl	%eax
	movw	$0xe00, %si
	movw	$0x10, (%si)     		 	 /* Packet length */
	movw	$1, 2(%si)						 /* number of sectors */
	movw	%bx, 4(%si)            /* buffer offset */
	movw	%di, 6(%si)            /* buffer segment */
	movl	%eax, 8(%si)           /* LBA lo 32 bits */
	movl	$0, 12(%si)            /* LBA hi 32 bits */ 
	movb	$0x42, %ah
	pushw	%ds
	pushw	%es
	int	$0x13
	popw	%es
	popw	%ds
	popl	%eax
	decw	%bp
	je	1f
	leaw	0x20(%di), %di
	incl	%eax
	jmp 1b
1:
	popaw
	ret		

4:
	movw	$(msg_BootError_hd_udf - Entry_hd_udf), %si
1:	
	lodsb	(%si), %al	/* get token */
	cmpb	$0, %al		/* end of string? */
	je	1f
	movb	$0x0e, %ah	/* print char in AL */
	int	$0x10		/* via TTY mode */	
	jmp	1b
	
1:      
	jmp	1b

//(105) or (10a)
read_fileEntry:
	pushw	%es
	cmpw	$0x105, (%bx)
	jne	1f
	//105
	movw	0xa8(%bx), %si
	movw	0xac(%bx), %cx
	pushw	%dx
	movw	$0, %dx
	jmp	2f
1:
	//10a
	movw	0x22(%bx), %ax
	andw	$7, %ax
	cmpw	$3, %ax
	jne	1f
	movw	0xd0(%bx), %si
	addw	$0x10d8, %si
	movw	$0x2000, %di
	movw	$0x80, %cx
	rep movsd
	jmp	4f
1:
	cmpw	$0, %ax
	jne	3f
	pushw	%dx
	movw	0xd0(%bx), %si
	movw	0xd4(%bx), %cx
	movw	$0x28, %dx
2:	
	addw	%dx, %si
	shrw	$3, %cx
	popw	%dx
1:
	movzwl	0xb4(%bx, %si), %eax
	addl	(hd_udf_partition_start - Entry_hd_udf), %eax       //FileIdentifier

	pushl	%eax
	movl	0xb0(%bx, %si), %eax                    //File length in bytes */  
	andl	$0x3fffffff, %eax
	addl	$0x1ff, %eax
	shrl	$9, %eax
	movw	%ax, %bp
	popl	%eax
	pushw	%bx
	xorw	%bx, %bx
	movw	%di, %es
	call	load_hd_udf
	popw	%bx
	leaw	0x20(%di), %di
	addw	$8, %bx    
	loop	1b   
	clc
	jmp	4f
3:
	stc
4:	
	popw	%es
  ret      
 
	. = Entry_hd_udf + 0x1f0       
       
msg_BootError_hd_udf:
	.ascii	"No "        

hd_udf_name:
	.ascii	"GRLDR\0\0\0\0\0\0"
       
	. = Entry_hd_udf + 0x1fe
	.byte   0x55, 0xaa
/* ********************  HD_UDF_bin end  ****************************** */ 
#endif   //defined(GRLDR_INSTALL))
    		      
       
pre_stage2_start:

