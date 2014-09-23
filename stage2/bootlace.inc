/*
 *  bootlace.inc -- code shared by 16-bit DOS and 32-bit Linux
 *  Copyright (C) 2005,2006,2007,2012  Tinybit(tinybit@tom.com)
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

	.file	"bootlace.inc"

#ifdef __DOS_16

#undef ABS
#define ABS(x) (x - _start + 0x100)

#undef iBX
#define iBX	%bx

#undef iSI
#define iSI	%si

#undef iDI
#define iDI	%di

#undef iDX
#define iDX	%dx

#undef rBP
#define rBP

#undef LEAL
#define LEAL	ADDR32 leal

#undef MOVZWL
#define MOVZWL	ADDR32 movzwl

#undef MOVL
#define MOVL	ADDR32 movl

#undef MOVW
#define MOVW	ADDR32 movw

#undef CMPL
#define CMPL	ADDR32 cmpl

#undef CMPW
#define CMPW	ADDR32 cmpw

#undef SUBL
#define SUBL	ADDR32 subl

#undef SBBL
#define SBBL	ADDR32 sbbl

#undef NEGL
#define NEGL	ADDR32 negl

#undef NOTL
#define NOTL	ADDR32 notl

#undef ADCL
#define ADCL	ADDR32 adcl

#undef ADDL
#define ADDL	ADDR32 addl

#undef DIVL
#define DIVL	ADDR32 divl

	. = _start + 0x40		// dos entry point invalid_dos_exec 

#if 0
	movl	$ABS(msg_invalid_dos), %ecx
	call	8f

	movw	$0x4c01, %ax		// EXIT - TERMINATE WITH RETURN CODE 1
	int	$0x21			// call DOS
#else
	jmp	1f
#endif


	//////////////////////////////////////////////////////////////////////
	
	. = _start + 0x47		// dos entry point

1:
	/* dos entry point */

	/* read stdin at _ph_offset + 1 */
	movl	$(_ph_offset + 1 - _start + 0x100), %edi
2:
	movw	$10, %cx
1:
	/* check if there is a char */
	movb	$0x0B, %ah
	pushw	%cx
	pushl	%edi
	int	$0x21
	popl	%edi
	popw	%cx
	testb	%al, %al
	jnz	1f
	loopz	1b
	/* no chars to read */
	jmp	2f		/* end of stdin */
1:
	/* read the char, direct char input without echo */
	movb	$0x07, %ah
	pushl	%edi
	int	$0x21		/* AL=char read */
	popl	%edi
	cmpb	$0x0D, %al
	jne	1f
	movb	$0x20, %al
1:
	cmpb	$0x0C, %al
	jne	1f
	movb	$0x20, %al
1:
	cmpb	$0x0A, %al
	jne	1f
	movb	$0x20, %al
1:
	cmpb	$0x09, %al
	jne	1f
	movb	$0x20, %al
1:
	cmpb	$0x20, %al
	jb	2f		/* end input */
	cld
	stosb
	cmpl	$(_ph_offset + 1 - _start + 0x100 + 512), %edi
	jb	2b		/* read next char */

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_stdin_too_long), %ecx
	jmp	4f	/* error */

2:

	cld
	movb	$0x20, %al
	stosb			/* store one more space */

	/* move command line */

	movl	$0x81, %esi
	movl	$0x7f, %ecx
	repz movsb

	/* the new long command line */

	//movl	$(_ph_offset + 1 - _start + 0x100), %esi
	//movl	$0x81, %esi
	//subl	%esi, %edi
	subl	$0x7f, %edi
	movl	%edi, %esi
	movl	$0x7f, %ecx
	movb	$0x0d, %al	/* CR */

	cld
	repnz scasb

	decl	%edi		/* points to CR */
	std
	movb	$0, %al		/* change CR to NULL */
	stosb

	movl	%edi, %esi
	xorl	%ecx, %ecx
	pushl	%ecx		/* 0 is the end of argv */
	xorl	%edx, %edx	/* find spaces */
3:
	cmpl	$(_ph_offset + 1 - _start + 0x100), %esi
	//cmpl	$0x81, %edi
	jb	3f		/* args all done, exit */
	lodsb
	cmpb	$0x20, %al	/* skip spaces */
	je	1f
	cmpb	$0x09, %al	/* skip Tabs */
	jne	2f		/* found non-space char */
1:
	/* it is a space */
	testl	%edx, %edx	/* we are finding spaces? */
	jz	1f		/* yes, simply change to NULL */

	notl	%edx		/* no, we will find spaces */
	incl	%edi
	pushl	%edi		/* argv */
	decl	%edi
1:
	movb	$0, %al		/* change spaces to NULL */
	stosb
	jmp	3b		/* next */
2:
	/* it is a non-space */
	testl	%edx, %edx	/* we are finding spaces? */
	jnz	1f		/* no, simply save the char */
	incl	%ecx		/* yes, one more arg was found */
	notl	%edx		/* now we will find non-spaces */

	cmpl	$50, %ecx
	jb	1f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_too_many_parameters), %ecx
	jmp	4f	/* error */

1:
	stosb			/* store the non-space char */
	jmp	3b		/* next */
3:
	testl	%edx, %edx
	jz	1f
	incl	%edi
	pushl	%edi
1:
	pushl	%esi		/* dummy for program name */
	incl	%ecx
	pushl	%ecx		/* argc */
	cld
#else

#undef ABS
//#define ABS(x) (x)
#define ABS(x) (x - _start + 0x00400000)

#undef iBX
#define iBX	%ebx

#undef iSI
#define iSI	%esi

#undef iDI
#define iDI	%edi

#undef iDX
#define iDX	%edx

#undef rBP
#define rBP	(%ebp)

#undef LEAL
#define LEAL	leal

#undef MOVZWL
#define MOVZWL	movzwl

#undef MOVL
#define MOVL	movl

#undef MOVW
#define MOVW	movw

#undef CMPL
#define CMPL	cmpl

#undef CMPW
#define CMPW	cmpw

#undef SUBL
#define SUBL	subl

#undef SBBL
#define SBBL	sbbl

#undef NEGL
#define NEGL	negl

#undef NOTL
#define NOTL	notl

#undef ADCL
#define ADCL	adcl

#undef ADDL
#define ADDL	addl

#undef DIVL
#define DIVL	divl

#endif

	popl	%ecx		# argc

	/* argc must be > 1 */

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	movl	%ebx, %ebp	# EBP = 0

	decl	%eax		// it is the rex prefix 0x48
	cmpl	$1, %ecx	// rcx
	movl	$ABS(msg_no_args), %ecx
	jbe	4f	/* error */

	/* Note: ECX does not hold the argc now. */

	xorl	%eax, %eax	# EAX will point to file name
	//movl	%ebx, ABS(filename)	# save 0 to filename
	popl	%ecx		# discard argv[0], the program name
1:
	popl	%ecx		# argv
	pushl	%ecx
	popl	%ebx

	jecxz	3f		/* end of arguments */
	jmp	2f
3:
	jmp	1f
2:

	/* check if it is an option */
	cmpb	$0x2D, (iBX)	# "-"
	jz	2f

	/* it is a filename */
	pushl	%eax
	popl	%ecx

	jecxz	3f		# filename not yet specified, continue

	/* error: only one file should be specified. */
	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_too_many_files), %ecx
	jmp	4f	/* error */

3:
	/* encountered first filename */
	pushl	%ebx		# EBX = filename
	popl	%eax		# save filename to EAX
	jmp	1b		# continue

2:

	/* it is an option */

	/* check --restore-mbr */

	movl	$14, %ecx
	movl	$ABS(option_restore_mbr), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	testl	$0x80000000, ABS(restore_mbr) rBP
	movl	$ABS(msg_option_restore_mbr_with_other), %ecx
	jnz	4f	/* error */
	testb	$1, ABS(restore_mbr) rBP
	movl	$ABS(msg_option_restore_mbr), %ecx
	jnz	4f	/* error */
	orb	$1, ABS(restore_mbr) rBP
	jmp	1b

3:

	/* check --read-only */

	movl	$12, %ecx
	movl	$ABS(option_read_only), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$2, ABS(read_only) rBP
	movl	$ABS(msg_option_read_only), %ecx
	jnz	4f	/* error */

	movl	$0, ABS(read_only) rBP
	jmp	1b

3:
	orl	$0x80000000, ABS(restore_mbr) rBP	# first option is not --restore-mbr/--read-only
	testb	$1, ABS(restore_mbr) rBP
	movl	$ABS(msg_option_restore_mbr_with_other), %ecx
	jnz	4f	/* error */

	/* check --no-backup-mbr */

	movl	$16, %ecx
	movl	$ABS(option_no_backup_mbr), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$1, ABS(backup_mbr) rBP
	movl	$ABS(msg_option_backup_mbr), %ecx
	jnz	4f	/* error */
	movl	$0, ABS(backup_mbr) rBP
	jmp	1b

3:
	/* check --force-backup-mbr */

	movl	$19, %ecx
	movl	$ABS(option_force_backup_mbr), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$1, ABS(backup_mbr) rBP
	movl	$ABS(msg_option_backup_mbr), %ecx
	jnz	4f	/* error */
	movl	$2, ABS(backup_mbr) rBP
	jmp	1b

3:

	/* check --mbr-enable-floppy */

	movl	$20, %ecx
	movl	$ABS(option_mbr_enable_floppy), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0, ABS(mbr_floppy) rBP
	movl	$ABS(msg_option_mbr_floppy), %ecx
	jnz	4f	/* error */
	movl	$2, ABS(mbr_floppy) rBP
	jmp	1b

3:

	/* check --mbr-disable-floppy */

	movl	$21, %ecx
	movl	$ABS(option_mbr_disable_floppy), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0, ABS(mbr_floppy) rBP
	movl	$ABS(msg_option_mbr_floppy), %ecx
	jnz	4f	/* error */
	movl	$1, ABS(mbr_floppy) rBP
	jmp	1b

3:

	/* check --mbr-enable-osbr */

	movl	$18, %ecx
	movl	$ABS(option_mbr_enable_osbr), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0, ABS(mbr_osbr) rBP
	movl	$ABS(msg_option_mbr_osbr), %ecx
	jnz	4f	/* error */
	movl	$2, ABS(mbr_osbr) rBP
	jmp	1b

3:

	/* check --mbr-disable-osbr */

	movl	$19, %ecx
	movl	$ABS(option_mbr_disable_osbr), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0, ABS(mbr_osbr) rBP
	movl	$ABS(msg_option_mbr_osbr), %ecx
	jnz	4f	/* error */
	movl	$1, ABS(mbr_osbr) rBP
	jmp	1b

3:

	/* check --mbr-no-bpb */

	movl	$13, %ecx
	movl	$ABS(option_mbr_no_bpb), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0, ABS(mbr_bpb) rBP
	movl	$ABS(msg_option_mbr_bpb), %ecx
	jnz	4f	/* error */
	movl	$1, ABS(mbr_bpb) rBP
	jmp	1b

3:

	/* check --duce */

	movl	$7, %ecx
	movl	$ABS(option_duce), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0, ABS(duce) rBP
	movl	$ABS(msg_option_duce), %ecx
	jnz	4f	/* error */
	movl	$1, ABS(duce) rBP
	jmp	1b

3:

	/* check --chs-no-tune */

	movl	$14, %ecx
	movl	$ABS(option_chs_no_tune), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0, ABS(chs_no_tune) rBP
	movl	$ABS(msg_option_chs_no_tune), %ecx
	jnz	4f	/* error */
	movl	$1, ABS(chs_no_tune) rBP
	jmp	1b

3:

	/* check --boot-prevmbr-first */

	movl	$21, %ecx
	movl	$ABS(option_boot_prevmbr_first), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0, ABS(boot_prevmbr) rBP
	movl	$ABS(msg_option_boot_prevmbr), %ecx
	jnz	4f	/* error */
	movl	$1, ABS(boot_prevmbr) rBP
	jmp	1b

3:

	/* check --boot-prevmbr-last */

	movl	$20, %ecx
	movl	$ABS(option_boot_prevmbr_last), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0, ABS(boot_prevmbr) rBP
	movl	$ABS(msg_option_boot_prevmbr), %ecx
	jnz	4f	/* error */
	movl	$2, ABS(boot_prevmbr) rBP
	jmp	1b

3:

	/* check --preferred-drive= */

	movl	$18, %ecx
	movl	$ABS(option_preferred_drive), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0xff, ABS(preferred_drive) rBP
	movl	$ABS(msg_option_preferred_drive), %ecx
	jnz	4f	/* error */

	movl	$0xfe, %ebx
	call	7f	/* parse_number */
	movl	%ebx, %edi	# save EBX to EDI
	movl	$0, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_invalid_number), %ecx
	jc	4f	/* error */
	movl	%edi, ABS(preferred_drive) rBP
	jmp	1b

3:

	/* check --preferred-partition= */

	movl	$22, %ecx
	movl	$ABS(option_preferred_partition), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0xff, ABS(preferred_partition) rBP
	movl	$ABS(msg_option_preferred_partition), %ecx
	jnz	4f	/* error */

	movl	$0xfe, %ebx
	call	7f	/* parse_number */
	movl	%ebx, %edi	# save EBX to EDI
	movl	$0, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_invalid_number), %ecx
	jc	4f	/* error */
	movl	%edi, ABS(preferred_partition) rBP
	jmp	1b

3:

	/* check --serial-number= */

	movl	$16, %ecx
	movl	$ABS(option_serial_number), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0, ABS(serial_number) rBP
	movl	$ABS(msg_option_serial_number), %ecx
	jnz	4f	/* error */

	movl	$0xffffffff, %ebx
	call	7f	/* parse_number */
	movl	%ebx, %edi	# save EBX to EDI
	movl	$0, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_invalid_number), %ecx
	jc	4f	/* error */
	cmpl	$0, %edi
	je	4f	/* error */
	movl	%edi, ABS(serial_number) rBP
	jmp	1b

3:

	/* check --time-out= */

	movl	$11, %ecx
	movl	$ABS(option_time_out), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
//	cmpl	$0xffffff05, ABS(time_out) rBP
	cmpl	$0xffffff00, ABS(time_out) rBP
	movl	$ABS(msg_option_time_out), %ecx
	jnz	4f	/* error */

	movl	$0xff, %ebx
	call	7f	/* parse_number */
	movl	%ebx, %edi	# save EBX to EDI
	movl	$0, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_invalid_number), %ecx
	jc	4f	/* error */
	movl	%edi, ABS(time_out) rBP
	jmp	1b

3:

	/* check --hot-key= */

	movl	$10, %ecx
	movl	$ABS(option_hot_key), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0xffff3920, ABS(hot_key) rBP
	movl	$ABS(msg_option_hot_key), %ecx
	jnz	4f	/* error */

	movl	$0xffff, %ebx
	call	7f	/* parse_number */
	movl	%ebx, %edi	# save EBX to EDI
	movl	$0, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_invalid_number), %ecx
	jc	4f	/* error */
	movl	%edi, ABS(hot_key) rBP
	jmp	1b

3:

	/* check --install-partition= */

	movl	$20, %ecx
	movl	$ABS(option_install_partition), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0xffffffff, ABS(install_partition) rBP
	movl	$ABS(msg_option_install_partition), %ecx
	jnz	4f	/* error */

	movl	$0xffff, %ebx
	call	7f	/* parse_number */
	movl	%ebx, %edi	# save EBX to EDI
	movl	$0, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_invalid_number), %ecx
	jc	4f	/* error */
	movl	%edi, ABS(install_partition) rBP
	jmp	1b

3:

	/* check --sectors-per-track= */

	movl	$20, %ecx
	movl	$ABS(option_sectors_per_track), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0xffffff3f, ABS(sectors_per_track) rBP
	movl	$ABS(msg_option_sectors_per_track), %ecx
	jnz	4f	/* error */

	movl	$0x3f, %ebx	# maximum is 63
	call	7f	/* parse_number */
	movl	%ebx, %edi	# save EBX to EDI
	movl	$0, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_invalid_number), %ecx
	jc	4f	/* error */
	cmpl	$0, %edi
	je	4f	/* error */
	movl	%edi, ABS(sectors_per_track) rBP
	jmp	1b

3:

	/* check --heads= */

	movl	$8, %ecx
	movl	$ABS(option_heads), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0xffff00ff, ABS(heads) rBP
	movl	$ABS(msg_option_heads), %ecx
	jnz	4f	/* error */

	movl	$0x100, %ebx	# maximum is 256
	call	7f	/* parse_number */
	movl	%ebx, %edi	# save EBX to EDI
	movl	$0, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_invalid_number), %ecx
	jc	4f	/* error */
	cmpl	$0, %edi
	je	4f	/* error */
	movl	%edi, ABS(heads) rBP
	jmp	1b

3:

	/* check --start-sector= */

	movl	$15, %ecx
	movl	$ABS(option_start_sector), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0xffffffff, ABS(start_sector) rBP
	movl	$ABS(msg_option_start_sector), %ecx
	jnz	4f	/* error */

	movl	$0xfffffffe, %ebx
	call	7f	/* parse_number */
	movl	%ebx, %edi	# save EBX to EDI
	movl	$0, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_invalid_number), %ecx
	jc	4f	/* error */
	movl	%edi, ABS(start_sector) rBP
	jmp	1b

3:

	/* check --total-sectors= */

	movl	$16, %ecx
	movl	$ABS(option_total_sectors), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0, ABS(total_sectors) rBP
	movl	$ABS(msg_option_total_sectors), %ecx
	jnz	4f	/* error */

	movl	$0xffffffff, %ebx
	call	7f	/* parse_number */
	movl	%ebx, %edi	# save EBX to EDI
	movl	$0, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_invalid_number), %ecx
	jc	4f	/* error */
	cmpl	$0, %edi
	je	4f	/* error */
	movl	%edi, ABS(total_sectors) rBP
	jmp	1b

3:

	/* check --floppy= */

	movl	$9, %ecx
	movl	$ABS(option_floppy_partition), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0xffffffff, ABS(floppy) rBP
	movl	$ABS(msg_option_floppy), %ecx
	jnz	4f	/* error */

	movl	$0xfe, %ebx
	call	7f	/* parse_number */
	movl	%ebx, %edi	# save EBX to EDI
	movl	$0, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_invalid_number), %ecx
	jc	4f	/* error */
	movl	%edi, ABS(floppy) rBP
	jmp	1b

3:

	/* check --floppy */

	movl	$8, %ecx
	movl	$ABS(option_floppy), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0xffffffff, ABS(floppy) rBP
	movl	$ABS(msg_option_floppy), %ecx
	jnz	4f	/* error */

	movl	$0xff, ABS(floppy) rBP
	jmp	1b
3:
	/* check --gpt */

	movl	$5, %ecx
	movl	$ABS(option_gpt), %esi
	pushl	%ebx
	popl	%edi

	cld
	repz cmpsb

	jnz	3f

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0xffffffff, ABS(gpt) rBP
	movl	$ABS(msg_option_gpt), %ecx
	jnz	4f	/* error */

	movl	$0xff, ABS(gpt) rBP
	jmp	1b

3:
	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	movl	$ABS(msg_invalid_option), %ecx
	jmp	4f	/* error */

1:

//----------------------------------------------------------------------------
	/* end of arguments */

	/* ECX=EBX=0 */

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	pushl	%eax
	popl	%ecx

	jecxz	3f		# no file name error
	jmp	1f
3:
	movl	$ABS(msg_no_file), %ecx
	jmp	4f	/* error */
1:

	pushl	%eax
	popl	%ebx		# EBX = filename

	/* check if preferred_partition matches preferred_drive */

	movb	ABS(preferred_drive) rBP, %al
	movb	ABS(preferred_partition) rBP, %ah

	cmpb	$0xff, %al
	jne	1f

	#; movl	%ebx, %edi	# save EBX to EDI
	pushl	%ebx
	popl	%edi

	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	cmpb	$0xff, %ah
	movl	$ABS(msg_partition_without_drive), %ecx
	jne	4f	/* error */

	#; movl	%edi, %ebx	# restore EBX from EDI
	pushl	%edi
	popl	%ebx
1:

	/* EBX points to the pathname of the file. */

	#; movl	%ebx, %edi	# save EBX to EDI
	pushl	%ebx
	popl	%edi
	
	xorl	%ebx, %ebx	# invalid file descriptor for CLOSE below
	
	cmpl	$0xffffffff, ABS(floppy) rBP
	je	1f		# --floppy is not specified

	/* if --floppy is specified, should not specify the options for MBR. */
	cmpl	$1, ABS(backup_mbr) rBP
	movl	$ABS(msg_option_backup_mbr_with_floppy), %ecx
	jnz	4f	/* error */

	cmpl	$0, ABS(mbr_floppy) rBP
	movl	$ABS(msg_option_mbr_floppy_with_floppy), %ecx
	jnz	4f	/* error */
	
	cmpl	$0, ABS(mbr_osbr) rBP
	movl	$ABS(msg_option_mbr_osbr_with_floppy), %ecx
	jnz	4f	/* error */
	
	cmpl	$0, ABS(mbr_bpb) rBP
	movl	$ABS(msg_option_mbr_bpb_with_floppy), %ecx
	jnz	4f	/* error */
	
	cmpl	$0, ABS(duce) rBP
	movl	$ABS(msg_option_duce_with_floppy), %ecx
	jnz	4f	/* error */
	
	cmpl	$0, ABS(chs_no_tune) rBP
	movl	$ABS(msg_option_chs_no_tune_with_floppy), %ecx
	jnz	4f	/* error */
	
	cmpl	$0, ABS(boot_prevmbr) rBP
	movl	$ABS(msg_option_boot_prevmbr_with_floppy), %ecx
	jnz	4f	/* error */
	
	cmpl	$0xff, ABS(preferred_drive) rBP
	movl	$ABS(msg_option_preferred_drive_with_floppy), %ecx
	jnz	4f	/* error */

	cmpl	$0xff, ABS(preferred_partition) rBP
	movl	$ABS(msg_option_preferred_partition_with_floppy), %ecx
	jnz	4f	/* error */

	cmpl	$0, ABS(serial_number) rBP
	movl	$ABS(msg_option_serial_number_with_floppy), %ecx
	jnz	4f	/* error */

	cmpl	$0xffffffff, ABS(install_partition) rBP
	movl	$ABS(msg_option_install_partition_with_floppy), %ecx
	jnz	4f	/* error */

//	cmpl	$0xffffff05, ABS(time_out) rBP
	cmpl	$0xffffff00, ABS(time_out) rBP
	movl	$ABS(msg_option_time_out_with_floppy), %ecx
	jnz	4f	/* error */

	cmpl	$0xffff3920, ABS(hot_key) rBP
	movl	$ABS(msg_option_hot_key_with_floppy), %ecx
	jnz	4f	/* error */
	
	/* for a floppy, start_sector should be 0, and
	 * for a partition, start_sector should be non-zero
	 */
	cmpl	$0xff, ABS(floppy) rBP
	je	2f		# a real floppy

	/* a single partition */
	cmpl	$0, ABS(start_sector) rBP
	movl	$ABS(msg_partition_start), %ecx
	jz	4f	/* error */

	jmp	3f	
2:
	/* a real floppy */
	/* */
	cmpl	$0xffffffff, ABS(start_sector) rBP
	je	3f
	cmpl	$0, ABS(start_sector) rBP
	movl	$ABS(msg_floppy_start), %ecx
	jnz	4f	/* error */

3:
	jmp	2f
	
1:
	/* EBX=0 */

	cmpl	$0xffffffff, ABS(install_partition) rBP
	je	2f		# --install-partition is not specified

	/* should not specify --install-partition with options for MBR. */
	cmpl	$1, ABS(backup_mbr) rBP
	movl	$ABS(msg_option_backup_mbr_with_partition), %ecx
	jnz	4f	/* error */

	cmpl	$0, ABS(mbr_floppy) rBP
	movl	$ABS(msg_option_mbr_floppy_with_partition), %ecx
	jnz	4f	/* error */
	
	cmpl	$0, ABS(mbr_osbr) rBP
	movl	$ABS(msg_option_mbr_osbr_with_partition), %ecx
	jnz	4f	/* error */
	
	cmpl	$0, ABS(mbr_bpb) rBP
	movl	$ABS(msg_option_mbr_bpb_with_partition), %ecx
	jnz	4f	/* error */
	
	cmpl	$0, ABS(duce) rBP
	movl	$ABS(msg_option_duce_with_partition), %ecx
	jnz	4f	/* error */
	
	cmpl	$0, ABS(chs_no_tune) rBP
	movl	$ABS(msg_option_chs_no_tune_with_partition), %ecx
	jnz	4f	/* error */
	
	cmpl	$0, ABS(boot_prevmbr) rBP
	movl	$ABS(msg_option_boot_prevmbr_with_partition), %ecx
	jnz	4f	/* error */
	
	cmpl	$0xff, ABS(preferred_drive) rBP
	movl	$ABS(msg_option_preferred_drive_with_partition), %ecx
	jnz	4f	/* error */

	cmpl	$0xff, ABS(preferred_partition) rBP
	movl	$ABS(msg_option_preferred_partition_with_partition), %ecx
	jnz	4f	/* error */

	cmpl	$0, ABS(serial_number) rBP
	movl	$ABS(msg_option_serial_number_with_partition), %ecx
	jnz	4f	/* error */

//	cmpl	$0xffffff05, ABS(time_out) rBP
	cmpl	$0xffffff00, ABS(time_out) rBP
	movl	$ABS(msg_option_time_out_with_partition), %ecx
	jnz	4f	/* error */

	cmpl	$0xffff3920, ABS(hot_key) rBP
	movl	$ABS(msg_option_hot_key_with_partition), %ecx
	jnz	4f	/* error */

	cmpl	$0xffffff3f, ABS(sectors_per_track) rBP
	movl	$ABS(msg_option_sectors_per_track_with_partition), %ecx
	jnz	4f	/* error */

	cmpl	$0xffff00ff, ABS(heads) rBP
	movl	$ABS(msg_option_heads_with_partition), %ecx
	jnz	4f	/* error */

	cmpl	$0xffffffff, ABS(start_sector) rBP
	movl	$ABS(msg_option_start_sector_with_partition), %ecx
	jnz	4f	/* error */

	cmpl	$0, ABS(total_sectors) rBP
	movl	$ABS(msg_option_total_sectors_with_partition), %ecx
	jnz	4f	/* error */

2:

	/* EBX=0, EDI points to the pathname of the file. */

//----------------------------------------------------------------------------
	#; movl	%edi, %ebx	# restore EBX from EDI
	pushl	%edi
	popl	%ebx
	
	/* EBX=EDI points to the pathname of the file. */

#ifdef __DOS_16

	xorl	%eax, %eax
	movl	$0xfe, %ebx
	call	7f	/* parse_number */
	jc	2f	/* invalid number */
	/* EBX holds the bios drive number */
	movl	%ebx, ABS(bios_drive_number)
	jmp	1f
2:
	//AH = 3Dh	//OPEN EXISTING FILE
	//AL = access and sharing modes (see #01402)
	//DS:DX -> ASCIZ filename
	//CL = attribute mask of files to look for (server call only)

	//Return:
	//CF clear if successful AX = file handle
	//CF set on error AX = error code (01h,02h,03h,04h,05h,0Ch,56h)

	movzbl	ABS(read_only), %eax
	movb	$0x3d, %ah	// open file for read/write
	movl	%edi, %edx	// DS:DX points to ASCIZ filename
	int	$0x21
	jnc	1f
	negl	%eax		/* EAX < 0 */
1:
#else

	/* int open(const char *pathname, int flags, mode_t mode) */
		// mode is not used since we are not creating a new file

	/* check if in 32-bit or 64-bit mode */
	xorl	%eax, %eax
	decl	%eax		// rex prefix of 0x48
	testl	%eax, %eax
	/* EAX = 0 for 64-bit mode, -1 for 32-bit mode */
	jz	1f
	
	/* in 32-bit mode */
	movl	$5, %eax		// sys_open
	movzbl	ABS(read_only) rBP, %ecx	// O_RDWR = 02
	pushl	%ebx
	popl	%edi
	pushl	%ecx
	popl	%esi
	int	$0x80
	jmp	2f
1:
	/* in 64-bit mode */
	movl	$2, %eax		// sys_open
	pushl	%ebx
	popl	%edi
	movzbl	ABS(read_only) rBP, %esi	// O_RDWR = 02
		// mode is not used since we are not creating a new file 
	syscall
2:
#endif
	xorl	%ebx, %ebx	// invalid file descriptor for CLOSE below
	movl	$ABS(msg_open_file_rw), %ecx
	cmpl	$2, ABS(read_only) rBP
	je	1f
	movl	$ABS(msg_open_file_ro), %ecx
1:
#if 1
	testl	%eax, %eax
	js	4f		/* error */
#else
	/* if needed, use 64-bit value for 64-bit Linux */
	pushl	%eax		# rax
	popl	%ecx		# rcx
	pushl	%ecx
	decl	%eax		# rex prefix of 0x48
	addl	%ecx, %ecx	# addq %rcx, %rcx
	popl	%eax
	jc	4f
#endif

//----------------------------------------------------------------------------
	/* open file succeeded */

#ifdef __DOS_16

#undef PRE
#define PRE(x) %x:
#undef ABS
#define ABS(x) (x - _start + 0x100)
#undef MBR
#define MBR(x) (x)

#else

#undef PRE
#define PRE(x) 
#undef ABS
//#define ABS(x) (x)
#define ABS(x) (x - _start + 0x00400000)
#undef MBR
//#define MBR(x) (mbr_63_sectors + x)
#define MBR(x) (grldr_mbr_start + 0x2000 + x - _start + 0x00400000)

#endif
	pushl	%eax		// file descriptor, or 0 for BIOS drive
	pushl	%eax
	popl	%ebx

	/* EBX and the stack holds the file descriptor number or 0 */
	
#ifdef __DOS_16

	/* check if there is 64K available for mbr_63_sectors and stack. */

	xorw	%ax, %ax
	movw	%ax, %ds
	movw	0x413, %ax	/* low memory in KBytes */
	shlw	$6, %ax		/* low memory in paragraphs */

	movw	%cs, %cx
	movw	%cx, %ds
	addw	$0x2000, %cx
	cmpw	%ax, %cx
	movl	$ABS(msg_no_enough_memory), %ecx
	ja	4f

	/* move stack to next 64K to avoid memory conflict. */
	movw	%cs, %ax
	addw	$0x1000, %ax
	movw	%ax, %ss
	movw	%ax, %ds
	movw	%ax, %es
	popl	%eax		// dummy pop
	pushl	%ebx		// file descriptor, or 0 for BIOS drive

	xorl	%eax, %eax
	cmpl	$0xff, PRE(cs)ABS(bios_drive_number)
	je	1f		// not bios drive

	/* check EBIOS support */
	call 100f	
	movb	$0x41, %ah
	movw	$0x55AA, %bx
	movb	PRE(cs)ABS(bios_drive_number), %dl
	int	$0x13
	movw	%ss, %ax
	movw	%ax, %ds
	movw	%ax, %es
	jc	3f		/* No EBIOS */
	cmpw	$0xAA55, %bx
	jne	3f		/* No EBIOS */
	testb	$1, %cl
	jz	3f		/* No EBIOS */
	/* EBIOS supported */
	movl	$1, PRE(cs)ABS(ebios_support)
//	movb	PRE(cs)ABS(bios_drive_number), %dl
	movw	$0, %si
	movb	$0x48, %ah
	int $0x13
	jc	2f
	cmpb	$0x42, 0x00
	jb	2f
	movw	$12, %cx
	movw	$0x24, %si
	movl	$ABS(msg_Information), %edi
	pushw	%es
	pushw	%cs
	popw	%es
	repz	movsb
	popw	%es
2:
	movw	$3, %si
2:
	pushw	%si	
	/* try read using EBIOS */
	pushl	$0		// hi 32bit of startLBA
	pushl	$0		// startLBA=0
	pushw	%es		// buffer segment
	pushw	$0		// buffer offset
	pushw	$0x0060		// sectors=96
	pushw	$0x0010		// packet length
	movw	%sp, %si
	movw	$0x4200, %ax	/* LBA read */
//	movb	PRE(cs)ABS(bios_drive_number), %dl
	int	$0x13
	movw	%ss, %ax
	movw	%ax, %ds
	movw	%ax, %es
	popl	%eax
	popl	%eax
	popl	%eax
	popl	%eax		# EAX=0
	popw	%si
	jnc	2f		// EBIOS read success
	decw	%si
	je	sb	
	xorw	%ax, %ax
//	movb	PRE(cs)ABS(bios_drive_number), %dl
	int	$0x13
	jmp	2b
sb:	
	/* A read failure means actually no EBIOS support */
	movl	$0, PRE(cs)ABS(ebios_support)
	movl	$ABS(msg_ebios_read_failure), %ecx
	jmp	10f
3:
	movl	$ABS(msg_no_ebios_support), %ecx
10:
	call	8f	/* linux_print */

	xorw	%bx, %bx	// BX=0
	movw	$0x204, %ax	// read 4 sectors of the MBR
	movw	$1, %cx
	movb	PRE(cs)ABS(bios_drive_number), %dl
	movb	$0, %dh
	int	$0x13
	movzwl	%ax, %eax
	movw	%ss, %cx
	movw	%cx, %ds
	movw	%cx, %es
	jc	2f		// stop on error
	movw	$0x7E00, %bx
	movw	$1, %cx
	movb	PRE(cs)ABS(bios_drive_number), %dl
	movb	$1, %dh
	movw	$0x201, %ax	// read 1 sector of track 1(the 2nd track)
	int	$0x13
	movzwl	%ax, %eax
	movw	%ss, %cx
	movw	%cx, %ds
	movw	%cx, %es
	clc			// ignore error
	jmp	2f
1:
	//READ FROM FILE OR DEVICE	
	call 100f
	
	//AH = 3Fh
	//BX = file handle
	//CX = number of bytes to read
	//DS:DX -> buffer for data

	//Return:
	//CF clear if successful AX = number of bytes actually read (0 if at EOF before call)
       	//CF set on error AX = error code (05h,06h)
	xchgw	%ax, %dx	// DX=0
	movw	$0x3f00, %ax
	movw	$((63+32+1)*512), %cx
	//movl	$MBR(0), %edx
	int	$0x21
2:
	jnc	1f
	negl	%eax
1:
#else
	/* ssize_t read(int fd, void *buf, size_t count) */
	//Clear buffer
	call	100f

	/* check if in 32-bit or 64-bit mode */
	xorl	%eax, %eax
	decl	%eax		// rex prefix of 0x48
	testl	%eax, %eax
	/* EAX = 0 for 64-bit mode, -1 for 32-bit mode */
	jz	1f
	
	/* in 32-bit mode */
	movl	$3, %eax		// sys_read
	movl	$MBR(0), %ecx
	movl	$((63+32+1)*512), %edx	// read 63+1 sectors
	int	$0x80
	jmp	2f
1:
	/* in 64-bit mode */
	movl	$0, %eax		// sys_read
	pushl	%ebx
	popl	%edi
	movl	$MBR(0), %esi
	movl	$((63+32+1)*512), %edx	// read 63+1 sectors
	syscall
2:
#endif

	popl	%ebx		// file descriptor, or 0 for BIOS drive
	testl	%eax, %eax
	movl	$ABS(msg_read_file), %ecx
	js	4f	/* error */

//----------------------------------------------------------------------------

	/* rewind the file for write! important! */

	movl	$0, %ecx
	call	400f

	testl	%eax, %eax
	movl	$ABS(msg_lseek_file), %ecx
	js	4f		/* error */

//----------------------------------------------------------------------------
	/* remember EBX holds file descriptor number, and don't touch it */

	/* check if the partition table is valid. */

#ifdef __DOS_16
	xorl	%esi, %esi
	cmpw	$0xAA55, 0x1fe(iSI)
#else
	movl	$MBR(0), %esi
	pushl	%eax
	movl	0x1fe(iSI), %eax
	andl	$0xFFFF, %eax
	cmpl	$0xAA55, %eax
	popl	%eax
#endif
	movl	$ABS(msg_boot_signature_mbr), %ecx
	//jne	4f	/* error */
	clc
	jne	2f

	call	9f	/* probe_geometry */
	jnc	1f

2:

	/* should be floppy since no partition table or no boot signature. */

	/* CF=0 means no boot signature, CF=1 means no partition table. */

	jnc	2f

	movl	$ABS(msg_invalid_partition_table), %ecx
2:
	cmpl	$0xffffffff, PRE(cs)ABS(floppy) rBP	/* Is --floppy specified? */
	je	4f	/* error */

	/* Yes, --floppy is specified */
	/* ESI points to MBR(0) */
	call	5f	/* floppy routine */
	movl	$0, %ecx
	jc	4f	/* error */
	
	jmp	2f	/* write file */
1:

//----------------------------------------------------------------------------
	/* the image type is MBR. So, should not specify --floppy */

	/* check empty partition table */
	movl	$MBR(0) + 0x1be, %edi
	movb	$0, %al
	xorl	%ecx, %ecx
	movb	$64, %cl
	cld
	repz scasb
	movl	$ABS(msg_invalid_partition_table), %ecx
	je	2b	/* empty partition table, always assume floppy. */
	
	cmpl	$0xffffffff, PRE(cs)ABS(floppy)	rBP /* Is --floppy specified? */
	movl	$ABS(msg_floppy_partition_table), %ecx
	jne	4f	/* error */
	
	cmpl	$0xffffff3f, PRE(cs)ABS(sectors_per_track) rBP
	movl	$ABS(msg_option_sectors_per_track_for_mbr), %ecx
	jnz	4f	/* error */

	cmpl	$0xffff00ff, PRE(cs)ABS(heads) rBP
	movl	$ABS(msg_option_heads_for_mbr), %ecx
	jnz	4f	/* error */

	cmpl	$0xffffffff, PRE(cs)ABS(start_sector) rBP
	movl	$ABS(msg_option_start_sector_for_mbr), %ecx
	jnz	4f	/* error */

	cmpl	$0, PRE(cs)ABS(total_sectors) rBP
	movl	$ABS(msg_option_total_sectors_for_mbr), %ecx
	jnz	4f	/* error */

	cmpl	$0xff, PRE(cs)ABS(gpt) rBP
	jz	1f
	
//	cmpl	$((pre_stage2_start - _start1) / 512), PRE(cs)ABS(mbr_free_sectors)	rBP
	cmpl	$(0x2000 / 0x200), PRE(cs)ABS(mbr_free_sectors)	rBP
	movl	$ABS(msg_sectors_per_track), %ecx
	jb	4f	/* error */

	cmpl	$0xffffffff, PRE(cs)ABS(install_partition) rBP
	je	1f	/* --install-partition not specified */

#ifdef __DOS_16
	/*****************************************/
	/* piece of code for --install-partition */ 
	/*****************************************/
	/* EBX holds the file descriptor number or 0 if it is BIOS drive */
	/* move partition table forward to the beginning of the 17th sector */
	cmpb	$0, PRE(cs)ABS(ebios_support)	//Support the expansion of reading?
	movl	$ABS(msg_no_ebios_support), %ecx
	je	4f	//no

	movl	$MBR(0x01be), %esi
	movl	$MBR(0x2000), %edi
	movl	$0x10, %ecx	# 0x10 dwords = 0x40 bytes
	cld
	repz movsl  //Move the partition table

	/* load the next partition and modify the boot record */

	/* Detection of extended partitions start logical sector */
	call	0f	/* partition */
	/* Display disk information */
	movb	PRE(cs)ABS(bios_drive_number), %al	//bios id
	subb	$0x50, %al
	movb	%al, PRE(cs)ABS(msg_disk)
	movw	$0xffff, %bx
	movl	$MBR(0x2004), %esi
	pushw	%bx
33:
	popw	%bx
	incw	%bx
	cmpb	PRE(cs)ABS(total_partition_entries), %bl //The total number of partitions
	ja	34f	
	pushw	%bx
	shlw	$4, %bx
	movl	$MBR(0x2004), %esi
	addw	%bx, %si
	cmpb $0, (iSI)
	je	33b
	movl 0x08(iSI), %eax
	popw	%bx
3:
	pushw	%bx	
	addb	$0x30, %bl
	movb	%bl, PRE(cs)ABS(msg_partition)
	call	66f
	jmp	33b
34:	
	//Wait for key
	movw	PRE(cs)ABS(install_partition), %bx	//Installation partition
	cmpb	PRE(cs)ABS(total_partition_entries), %bl //The total number of partitions
	movl	$ABS(msg_install_partition_not), %ecx	
	ja	4f
	shlw	$4, %bx
	movl	$MBR(0x2004), %esi
	addw	%bx, %si
	cmpb $0, (iSI)
	je	4f	
	movl	$ABS(msg_Wait_for_key), %ecx
	call	8f	/* linux_print */
3:	
	movb	$1, %ah	//Get keyboard input
	int	$0x16
	je	3b	//not
	xorl	%ecx, %ecx    	
	cmpb	$0x79, %al	//"y"?
	jne	4f	//not	
	//Install the boot code to disk
	//24 sectors to read the specified partition es: 0000

	movw	PRE(cs)ABS(install_partition), %bx	  //Partition number
	shlw	$4, %bx
	movl	$MBR(0x2000), %esi	
	addw	%bx, %si
	movl	0x08(iSI), %eax
	addl	(iSI), %eax
	pushl	%eax
	pushl	$0
	pushl	%eax
	pushw	%es
	pushw	$0
	pushw $0x04
	pushw $0x10
	movb	$0x42, %ah		
	movw	%sp, %si
	movb	PRE(cs)ABS(bios_drive_number), %dl //bios id	
	int $0x13
	popaw
	popl	%eax
	movl	$0, %ecx
	jc 4f
  //Determine the partition type, copy the file
  xorl	%esi, %esi
	call	5f	/* floppy routine */
	movl	%ecx, %edx
	movl	$0, %ecx
	jc	4f	/* error */
	//Write from memory to the boot sector file
	cmpw	$2, PRE(cs)ABS(read_only)	#; 0=read only, 2=read/write
	jne	4f		/* read only */
	//Determine whether ext2 partition usb drive installed
3:		
	pushl	$0
	pushl	%eax
	pushw	%es
	pushw	$0
	pushw PRE(cs)ABS(sectors_to_write)
	pushw $0x0010
	movb	$0x43, %ah		
	movw	%sp, %si
	movb	PRE(cs)ABS(bios_drive_number), %dl		//bios id
	int $0x13
	popaw
	movl	$0, %ecx
	jc 4f
	movl	$ABS(msg_success), %ecx	
	call	8f	/* linux_print */

	/* all partitions have been installed. */
	
	xorl	%eax, %eax	# exit code = 0
	movb	$0x4c, %ah	// EXIT - TERMINATE WITH RETURN CODE in AL
	int	$0x21		// call DOS

#else
	jmp	4f	/* error since --install-partition not implemented */
#endif
		
//	ret

	/*************************************************/
	/* end the piece of code for --install-partition */
	/*************************************************/

1:
//----------------------------------------------------------------------------
	cmpl	$0xff, PRE(cs)ABS(gpt) rBP
	jnz	1f
	
	movl	$MBR(0), %esi
	movl	$ABS(msg_gpt_partition_not), %ecx
	cmpb	$0xee, 0x1c2(iSI)
	jne	4f
	cmpl	$0x20494645, 0x200(iSI)	//"EFI PART"?
	jne	4f												//no
	
	movl	0x228(iSI), %edi
	movl	0x248(iSI), %eax
	shll	$9, %eax
	addl	%eax, %esi
	movl	0x20(iSI), %ecx
3:
	cmpl	$0, 0x2c(iSI)
	jne	210f
	cmpl	$0, 0xa4(iSI)
	jne	210f
	movl	0xa0(iSI), %eax
	movl	0x28(iSI), %edx
	subl	$16, %eax
	cmpl	%edx, %eax
	ja 3f
	addl	$0x80, %esi
	cmpl	$0, 0x20(iSI)
	jne	3b
	
210:
	subl	$16, %ecx
	movl	%edi, %edx
	cmpl	%edx, %ecx
	movl	$ABS(msg_cannot_install_gpt_partition), %ecx
	jbe 4f

3:
	addl	$1, %edx
	movl	$ABS(msg_cannot_install_gpt_partition), %ecx
	cmpl	$0x800000, %edx
	jae	4f
	movl	%edx, PRE(cs)ABS(201f) rBP
	movl	$ABS(200f), %esi
	movl	$MBR(0), %edi
	movl	$(1f - 200f)/4, %ecx
#ifdef __DOS_16
	cs repz movsl
#else
	repz movsl
#endif
	movl	%edx, PRE(cs)ABS(install_sectors) rBP
	movl	$1, PRE(cs)ABS(sectors_to_write) rBP
	jmp	2f

	.code16	
200:
	cli
	xorl	%ecx, %ecx
	movw	%cx, %ss
	movw	$0x7C00, %sp        	//ss:sp=0000:7c00
	pushw	%cx
	popw	%ds                  	//ds=0000
	sti

	cmpb	$0xee, 0x7dc2
	jne	204f
	
	movw	$3, %di       
3:
	pushaw
	pushl	%ecx
	.byte	0x66,0x68							//pushl
201:
	.long	0x22
	pushw	$0xd00
	pushw	%cx
	pushw	$16
	pushw	$16
	movw	%sp, %si
	movb	$0x42, %ah
	int	$0x13
	popaw
	popaw
	jnb	202f
	xorw	%ax, %ax
	int	$0x13
	decw	%di
	jne	3b
204:
	movw	$(203f - 200b + 0x7c00), %si
3:
	lodsb
	cmpb	$0, %al
	je	3f
	movb	$0xe, %ah
	int	$0x10
	jmp	3b

3:
	jmp	3b

202:
	pushw	$0xd00
	pushw	$0x256
	retf

203:
	.ascii "Error loading grldr.mbr"
	.byte	0,0,0,0,0
	
#ifdef __DOS_16
	.code16
#else
	.code32
#endif
	
1:
//---------------------------------------------------------------------------

	/* begin restore mbr */
	testb	$1, PRE(cs)ABS(restore_mbr) rBP
	jz	1f			/* not --restore-mbr */

	/* check if previous mbr is valid. */
#ifdef __DOS_16
	xorl	%esi, %esi
	movw	$0x200, %si
	cmpw	$0xAA55, 0x1fe(iSI)
#else
	movl	$MBR(0x200), %esi  /* point to 2nd sector */
	pushl	%eax
	movl	0x1fe(iSI), %eax
	andl	$0xFFFF, %eax
	cmpl	$0xAA55, %eax
	popl	%eax
#endif
	movl	$ABS(msg_invalid_prev_mbr), %ecx
	jnz	4f	/* error */

	call	9f	/* probe_geometry */
	movl	$ABS(msg_invalid_prev_mbr), %ecx
	jc	4f	/* error */

	/* check if previous mbr is grldr */
	/* 2nd is not empty. if it is grldr, consider it is empty */
	movb	$0xBB, %al
#ifdef __DOS_16
	xorl	%edi, %edi
	movw	$0x260, %di
#else
	movl	$MBR(0x260), %edi	// in 2nd sector
#endif
	movl	$0x150, %ecx
	cld
	repnz scasb
	jnz	3f		// 2nd is not grldr, restore it
	movl	$ABS(msg_invalid_prev_mbr), %ecx
	cmpl	$0xC38E0820, (%edi)
	jz	4f	/* error */

3:
	/* copy byte 0000 to 01b7 in 2nd sector to 1st sector */
#ifdef __DOS_16
	movl	$0x200, %esi  /* point to 2nd sector */
	xorl	%edi, %edi
#else
	movl	$MBR(0x200), %esi  /* point to 2nd sector */
	movl	$MBR(0), %edi
#endif
	movl	$0x6e, %ecx	// 0x6e dwords = 0x1b8 bytes

	cld
	repz movsl

	movl	$1, PRE(cs)ABS(sectors_to_write) rBP
	/* completed. write file. */
	jmp	2f

	/* end restore mbr */
1:
//----------------------------------------------------------------------------
	/* begin backup mbr */

	/* auto backup */
	cmpl	$1, PRE(cs)ABS(backup_mbr) rBP
	jnz	1f

	/* check if the second sector begins in 106 dups of one byte */
#ifdef __DOS_16
	movb	0x200, %al
	movl	$0x200, %edi
#else
	movb	MBR(0x200) rBP, %al
	movl	$MBR(0x200), %edi
#endif
	movl	$0x6A, %ecx	// 0x6A=106

	cld
	repz scasb

	jz	10f		// the second sector is empty.
	/* 2nd is not empty. if it is grldr, consider it is empty */
	movb	$0xBB, %al
#ifdef __DOS_16
	xorl	%edi, %edi
	movw	$0x260, %di
#else
	movl	$MBR(0x260), %edi	// in 2nd sector
#endif
	movl	$0x150, %ecx
	cld
	repnz scasb
	jnz	3f		# 2nd is not grldr, do not overwrite it
	cmpl	$0xC38E0820, (%edi) 
	jnz	3f	/* error */
10:
	/* 2nd is empty. but if MBR is grldr, we still skip the backup. */

	movb	$0xBB, %al
	movl	$MBR(0x60), %edi	// in 1st sector
	movl	$0x150, %ecx
	cld
	repnz scasb
	jnz	1f		# MBR is not grldr, enable backup
	cmpl	$0xB8661FFC, (%edi)
	jz	3f		# MBR is grldr, do not backup

1:

//----------------------------------------------------------------------------
	cmpl	$0, PRE(cs)ABS(backup_mbr) rBP
	jz	3f		# will not backup

#ifdef __DOS_16
	xorl	%esi, %esi
	movl	$0x200, %edi
#else
	movl	$MBR(0), %esi
	movl	$MBR(0x200), %edi
#endif
	movl	$0x80, %ecx	# 0x80 dwords = 0x200 bytes

	cld
	repz movsl

	/* end backup mbr */
3:

//----------------------------------------------------------------------------
#if 0
	/* copy byte 0000 to 01b7 of grldr.mbr to MBR(0) */

	movl	$ABS(grldr_mbr_start), %esi
#ifdef __DOS_16
	xorl	%edi, %edi
#else
	movl	$MBR(0), %edi
#endif
	movl	$0x6e, %ecx	# 0x6e dwords = 0x1b8 bytes

	cld
#ifdef __DOS_16
	cs repz movsl
#else
	repz movsl
#endif
	/* end copy boot record in sector 1 */
#endif
//----------------------------------------------------------------------------
	#################################################
	####       copy and modify BPB ... ...       ####
	#################################################

	/*************************/
	/* locate the BPB sector */
	/*************************/

#ifdef __DOS_16
	testb	$1, PRE(cs)ABS(mbr_bpb)
	jnz	1f		/* mbr disable bpb */
	testl	%ebx, %ebx
	jnz	1f
	/* BIOS drive. Need to load all 64 sectors. */

	cmpl	$0, PRE(cs)ABS(ebios_support)
	jne	1f		/* EBIOS succeeded previously, need not do anything more. */
	/* no EBIOS, so try BIOS with a single cross-track read. */
	movw	$0x23C, %ax	// read 60 sectors in the MBR track
	movw	$0x800, %bx	// skip 4 sectors
	movw	$5, %cx		// skip 4 sectors to load sector 5
	movb	PRE(cs)ABS(bios_drive_number), %dl
	movb	$0, %dh
	int	$0x13
	movw	%ss, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movl	$0, %ebx
	jnc	1f
	/* cross-track read failed, so try normal BIOS read track by track */
	movl	$0x3B, %ecx
3:
	pushl	%ecx
	movb	$0x02, %ah	// read sectors
	movb	%cl, %al
	movw	$0x800, %bx	// skip 4 sectors
	movw	$5, %cx		// skip 4 sectors to load sector 5
	movb	PRE(cs)ABS(bios_drive_number), %dl
	movb	$0, %dh
	int	$0x13
	movw	%ss, %ax
	movw	%ax, %ds
	movw	%ax, %es
	popl	%ecx
	jnc	3f
	loop	3b
3:
	/* CX + 4 = sectors per track */
	addw	$4, %cx
	cmpw	$32, %cx
	movl	$0, %ebx
	jb	1f

	movw	$0x240, %ax	// read sectors
	subw	%cx, %ax
	movw	%cx, %bx
	shlw	$9, %bx
	movw	$1, %cx		// load sector 1
	movb	PRE(cs)ABS(bios_drive_number), %dl
	movb	$1, %dh		// head 1
	int	$0x13
	movw	%ss, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movl	$0, %ebx
	/* ignore error */
1:
#endif

	movl	PRE(cs)ABS(mbr_free_sectors) rBP, %eax
	movl	%eax, %esi
	cmpl	$(63+32), %eax
	je	3f		/* we can embed triple MBR */
	cmpl	$32, %eax
	jne	10f		/* continue */

	/* check if it is triple MBR previously embedded */
	movl	$0xAA55, %ecx
	#; cmpw	%cx, MBR(0x5FE)
	cmpb	%cl, MBR(0x5FE) rBP
	jne	10f
	cmpb	%ch, MBR(0x5FF) rBP
	jne	10f
	#; cmpw	%cx, MBR(0x7FE)
	cmpb	%cl, MBR(0x7FE) rBP
	jne	10f
	cmpb	%ch, MBR(0x7FF) rBP
	jne	10f
	#; cmpw	%cx, MBR(0x9FE)
	cmpb	%cl, MBR(0x9FE) rBP
	jne	10f
	cmpb	%ch, MBR(0x9FF) rBP
	jne	10f
	#; cmpw	%cx, MBR(0xBFE)
	cmpb	%cl, MBR(0xBFE) rBP
	jne	10f
	cmpb	%ch, MBR(0xBFF) rBP
	jne	10f
	//cmpl	$0xAA555247, MBR(pre_stage2_start - _start1 - 4)
	//jne	10f
	xorl	%ecx, %ecx
	cmpl	%ecx, MBR(0x0000 + 0x1C) rBP	/* hidden sectors */
	jne	10f
	movb	$0x20, %cl
	cmpl	%ecx, MBR(0x4000 + 0x1C) rBP	/* hidden sectors */
	jne	10f
	cmpl	%ecx, MBR(0xBE00 + 0x1C) rBP	/* hidden sectors */
	jne	10f
	subl	$1, %ecx	#; decl	%ecx	#; ECX=0x1F
	cmpl	%ecx, MBR(0x7E00 + 0x1C) rBP	/* hidden sectors */
	jne	10f
	#; movw	MBR(0x1FF8), %cx	/* ECX hi word=0 */
	movb	MBR(0x1FF8) rBP, %cl	/* ECX hi word=0 */
	movb	MBR(0x1FF9) rBP, %ch	/* ECX hi word=0 */
	cmpl	$0x60, %ecx
	jb	10f
	cmpl	$0x1b8, %ecx
	jnb	10f
	//movzwl	%cx, %ecx
	movl	$MBR(0), %edi
	addl	%ecx, %edi
	cmpl	$0x661FFCBB, -5(iDI)
	jne	10f
	cmpb	$0xB8, -1(iDI)
	jne	10f
	movl	MBR(0x1FFC) rBP, %ecx
	cmpl	%ecx, (iDI)
	jne	10f

	movl	$MBR(0x4060), %esi	#; 0x4000 + 0x60
	movl	$MBR(0x7E60), %edi	#; 0x7E00 + 0x60
	movl	$((0x1b8 - 0x60) / 4), %ecx
	cld
	repz cmpsl
	movl	%eax, %esi	#; 32
	jne	10f

	movl	$MBR(0x4400), %esi	#; 0x4000 + 0x400
	movl	$MBR(0x8200), %edi	#; 0x7E00 + 0x400
	movl	$((0x2000 - 0x400) / 4), %ecx
	cld
	repz cmpsl
	movl	%eax, %esi	#; 32
	jne	10f

	/* find partition with start_sector=0x20 */
	movl	$0, PRE(cs)ABS(restore_partition_layout) rBP
	movl	$0x20, %eax
	movl	$MBR(0), %esi
	movl	%ebx, %edi		#; save file descriptor
	movl	$0x1B6, %ebx
11:
	incl	PRE(cs)ABS(restore_partition_layout) rBP
	addl	$0x10, %ebx
	cmpl	$0x1F6, %ebx
	ja	11f
	cmpl	$0x3F, 4(iBX, iSI)	#; LBA total sectors
	jna	11b
	cmpl	%eax, (iBX, iSI)	#; 0x20
	jne	11b
	#; cmpw	$0x21, -6(iBX, iSI)	#; CHS start sector number
	cmpb	$0x21, -6(iBX, iSI)	#; CHS start sector number
	jne	11b
	cmpb	$0x00, -5(iBX, iSI)	#; CHS start sector number
	jne	11b

	/* found partition with start_sector=0x20 */

	/* yes, it is triple MBR */

	movl	%edi, %ebx		#; restore file descriptor

	/* restore original partition layout */
	movl	$(63+32), %esi
	movl	$32, %eax	#; EAX=0x20, hidden sectors in BPB
	jmp	3f		#; continue to check BPB
11:
	/* not found partition with start_sector=0x20 */
	movl	$0, PRE(cs)ABS(restore_partition_layout) rBP
	movl	$32, %eax	#; mbr_free_sectors
	movl	%eax, %esi
	movl	%edi, %ebx		#; restore file descriptor
10:
	cmpl	$63, %eax
	ja	1f		/* too many sectors before partition */
	cmpl	$(0x2000 / 0x200), %eax
	jb	1f		/* too few sectors before partition */
3:
	shll	$9, %esi
#ifndef __DOS_16
	addl	$MBR(0), %esi
#endif

	/* ESI points to BPB sector */

	/* check if it has a valid FAT BPB */
	#; cmpw	$512, 0x0B(iSI)	// bytes per sector
	cmpb	$0x00, 0x0B(iSI)	// bytes per sector, lo
	jne	1f
	cmpb	$0x02, 0x0C(iSI)	// bytes per sector, hi
	jne	1f
	cmpl	%eax, 0x1C(iSI)	// hidden_sectors = part_start?
	jne	1f
	movl	0x18(iSI), %eax	// AX = sectors per track
	movl	%eax, %ecx
	shrl	$16, %ecx	// CX = heads
	andl	$0xFFFF, %eax
	subl	$1, %eax	#; decl	%eax
	cmpl	$62, %eax
	ja	1f

	subl	$1, %ecx	#; decl	%ecx
	cmpl	$255, %ecx
	ja	1f
	movb	0x0D(iSI), %cl	// sectors per cluster
	testb	%cl, %cl
	jz	1f
	movl	$128, %eax
	divb	%cl
	testb	%ah, %ah
	jnz	1f
	movl	0x0E(iSI), %ecx	// CX = reserved sectors
	andl	$0xFFFF, %ecx
	testl	%ecx, %ecx
	jz	1f
	movb	0x10(iSI), %al	// number of FATs
	decb	%al
	cmpb	$1, %al
	ja	1f
	movb	0x15(iSI), %al	// media descriptor
	cmpb	$0xF0, %al
	jb	1f

	/* Yes, it has a valid FAT BPB */

	/* it is time to restore original partition layout */
	xorl	%ecx, %ecx
	movl	PRE(cs)ABS(restore_partition_layout) rBP, %edi
	cmpl	%ecx, %edi
	je	3f
	movb	$(63+32), %cl
	movl	%ecx, MBR(0xBE00 + 0x1C) rBP	/* hidden sectors */

	shll	$4, %edi		#; EDI=EDI*16
	addl	$MBR(0x1B6), %edi	#; partition_start
	subl	$0x3F, 4(iDI)		#; partition_length
	addl	$0x3F, (iDI)		#; partition_start
	movb	$0x01, -7(iDI)		#; CHS start head number
	movl	$(63+32+1), PRE(cs)ABS(sectors_to_write) rBP
	jmp	2f
1:
	stc
3:
	pushfl
	/*********************************************************/
	/* copy byte 0000 to 01b7 of grldr.mbr to MBR(0) ... ... */
	/*********************************************************/

	pushl	%esi
	movl	$ABS(grldr_mbr_start), %esi
#ifdef __DOS_16
	xorl	%edi, %edi
#else
	movl	$MBR(0), %edi
#endif
	movl	$0x6e, %ecx	# 0x6e dwords = 0x1b8 bytes

	cld
#ifdef __DOS_16
	cs repz movsl
#else
	repz movsl
#endif
	popl	%esi
	popfl
	jc	1f

	/********************************************/
	/* ... ... end copy boot record in sector 1 */
	/********************************************/

	testb	$1, PRE(cs)ABS(mbr_bpb) rBP
	jnz	1f		/* mbr disable bpb */

	/**********************************/
	/* copy the BPB to MBR(0) ... ... */
	/**********************************/

	addl	$0x0B, %esi
#ifdef __DOS_16
	pushw	%ss
	popw	%es
	movl	$0x0B, %edi
#else
	movl	$MBR(0x0B), %edi
#endif
	movl	$(0x5A - 0x0B), %ecx

	cld
	repz movsb
	/**********************************/
	/* ... ... end copy BPB to MBR(0) */
	/**********************************/

	/* modify reserved sectors ... ... */
#ifdef __DOS_16
	xorl	%esi, %esi
#else
	movl	$MBR(0), %esi
#endif
	movl	0x1C(iSI), %eax			#; EAX=hidden sectors
	#; addw	%ax, 0x0E(iSI)			#; reserved sectors
	addl	0x0E(iSI), %eax
	movb	%al, 0x0E(iSI)
	movb	%ah, 0x0F(iSI)
	/* ... ... end modify reserved sectors */

	/* modify total sectors word ... ... */
	movl	0x13(iSI), %eax
	andl	$0xFFFF, %eax
	testl	%eax, %eax
	jz	3f
	addl	0x1C(iSI), %eax
	movb	$0, 0x13(iSI)			#; clear on overflow, lo
	movb	$0, 0x14(iSI)			#; clear on overflow, hi
	jc	3f
	movb	%al, 0x13(iSI)			#; total sectors short, lo
	movb	%ah, 0x14(iSI)			#; total sectors short, hi
3:
	/* ... ... end modify total sectors word */

	/* modify total sectors dword ... ... */
	movl	0x20(iSI), %eax
	testl	%eax, %eax
	jz	3f
	addl	0x1C(iSI), %eax
	movl	$0, 0x20(iSI)			#; clear on overflow
	jc	3f
	movl	%eax, 0x20(iSI)			#; total sectors long
3:
	/* ... ... end modify total sectors dword */

	/* clear the hidden sectors ... ... */
	xorl	%eax, %eax
	movl	%eax, 0x1C(iSI)
	/* ... ... end clear the hidden sectors */

1:
	#####################################################
	####       ... ... end copy and modify BPB       ####
	#####################################################
//----------------------------------------------------------------------------
	/* modify PRE(cs)ABS(mbr_floppy) bit */

	movb	PRE(cs)ABS(mbr_floppy) rBP, %al
	andb	$0x01, %al
//	movb	%al, MBR(2) rBP
	movb	%al, MBR(0x5a) rBP

	/* modify PRE(cs)ABS(mbr_osbr) bit */

	movb	PRE(cs)ABS(mbr_osbr) rBP, %al
	andb	$0x01, %al
	shlb	$1, %al
//	orb	%al, MBR(2) rBP
	orb	%al, MBR(0x5a) rBP

	/* modify PRE(cs)ABS(duce) bit */

	movb	PRE(cs)ABS(duce) rBP, %al
	andb	$0x01, %al
	shlb	$2, %al
//	orb	%al, MBR(2) rBP
	orb	%al, MBR(0x5a) rBP

	/* modify PRE(cs)ABS(chs_no_tune) bit */

	movb	PRE(cs)ABS(chs_no_tune) rBP, %al
	andb	$0x01, %al
	shlb	$3, %al
//	orb	%al, MBR(2) rBP
	orb	%al, MBR(0x5a) rBP

	/* modify PRE(cs)ABS(boot_prevmbr) bit */

	movb	PRE(cs)ABS(boot_prevmbr) rBP, %al
	notb	%al
	andb	$0x01, %al
	rorb	$1, %al
//	orb	%al, MBR(2) rBP
	orb	%al, MBR(0x5a) rBP

	/* modify PRE(cs)ABS(time_out) byte */

	movb	PRE(cs)ABS(time_out) rBP, %al
//	movb	%al, MBR(3) rBP
	movb	%al, MBR(0x5b) rBP

	/* modify PRE(cs)ABS(hot_key) byte */

	movl	PRE(cs)ABS(hot_key) rBP, %eax
//	movw	%ax, MBR(4) rBP
	movw	%ax, MBR(0x5c) rBP

	/* modify PRE(cs)ABS(preferred_drive) byte */

	movb	PRE(cs)ABS(preferred_drive) rBP, %al
//	movb	%al, MBR(6) rBP
	movb	%al, MBR(0x5e) rBP

	/* modify PRE(cs)ABS(preferred_partition) byte */

	movb	PRE(cs)ABS(preferred_partition) rBP, %al
//	movb	%al, MBR(7) rBP
	movb	%al, MBR(0x5f) rBP

	/* modify disk serial number */

	movl	PRE(cs)ABS(serial_number) rBP, %eax
	testl	%eax, %eax
	jz	1f
	movl	PRE(cs)ABS(grldr_mbr_start + 0x1FF8) rBP, %edi
	andl	$0xFFFF, %edi
#ifndef __DOS_16
	addl	$MBR(0), %edi	/* in the 1st sector */
#endif
	stosl
	movl	%eax, PRE(cs)ABS(grldr_mbr_start + 0x1FFC) rBP
1:

	/* copy byte 0400 and the rest of grldr.mbr to MBR(0) */

	movl	$ABS(grldr_mbr_start + 0x400), %esi
	movl	$MBR(0x400), %edi
	movl	$((0x2000 - 0x400) / 4), %ecx

	cld
#ifdef __DOS_16
	cs repz movsl
#else
	repz movsl
#endif

	movl	PRE(cs)ABS(mbr_free_sectors) rBP, %eax
				/* EAX=old part_start */
	cmpl	$(63+32), %eax
	jne	2f

	movl	$(0x2000 / 0x200), %ecx
	cmpl	$31, %ecx
	ja	2f

	/* copy MBR to sector 32 */
	movl	$MBR(0), %esi
	movl	$MBR(0x4000), %edi
	movl	$(0x2000 / 4), %ecx
	cld
	repz movsl
	/* end copy MBR to sector 32 */

	/* copy BPB to sector 32 */
	movl	$MBR(0xBE0B), %esi
	movl	$MBR(0x400B), %edi
	movl	$(0x5A - 0x0B), %ecx
	cld
	repz movsb
	/* end copy BPB to sector 32 */

	/* copy MBR and BPB to sector 63 */
	movl	$MBR(0x4000), %esi
	movl	$MBR(0x7E00), %edi
	movl	$(0x2000 / 4), %ecx
	cld
	repz movsl
	/* end copy MBR and BPB to sector 63 */

	/* modify partition entry for sector 0(MBR) */
	movl	%ebx, %edi		#; save file descriptor
	movl	$MBR(0), %esi
	movl	$0x1B6, %ebx
1:
	movl	PRE(cs)ABS(mbr_free_sectors) rBP, %eax
	addl	$0x10, %ebx
	cmpl	$0x1F6, %ebx
	ja	1f
	cmpl	(iBX, iSI), %eax	#; mbr_free_sectors
	jne	3f
	movb	$0, -7(iBX, iSI)	#; CHS start head number
	movb	$0x21, -6(iBX, iSI)	#; CHS start sector number, lo
	movb	$0x00, -5(iBX, iSI)	#; CHS start sector number, hi
	subl	$0x20, 4(iBX, iSI)	#; LBA total sectors
	addl	%eax, 4(iBX, iSI)	#; LBA total sectors
	xorl	%edx, %edx
	movl	%edx, -3(iBX, iSI)	#; clear end CHS
	movl	$0x20, (iBX, iSI)	#; LBA start sector number
	
	movl	(iBX, iSI), %eax
	addl	4(iBX, iSI), %eax
	subl	$1, %eax #; decl %eax	#; EDX:EAX=LBA end sector number
	/* translate LBA to CHS */
	movl	$63, %ecx		#; sectors per track=63
	divl	%ecx			#; EAX=track number, EDX+1=sector number
	addl	$1, %edx		#; incl	%edx
	movb	%dl, -2(iBX, iSI)	#; CHS end sector number
	xorl	%edx, %edx		#; EDX:EAX=track number
	movl	$255, %ecx		#; heads=255
	divl	%ecx			#; EAX=cylinder number, EDX=head number
	movb	%dl, -3(iBX, iSI)	#; CHS end head number
	movb	%al, -1(iBX, iSI)	#; CHS end cylinder lo byte
	shlb	$6, %ah			#; CHS end cylinder hi 2 bits
	orb	%ah, -2(iBX, iSI)	#; put in the byte for CHS end sector number
	jmp	1b
3:
	/* start LBA */
	xorl	%edx, %edx
	movl	(iBX, iSI), %eax	#; EDX:EAX=LBA start sector number
	cmpl	$0x20, %eax
	jb	1b
	/* translate start LBA to CHS */
	movl	$63, %ecx		#; sectors per track=63
	divl	%ecx			#; EAX=track number, EDX+1=sector number
	addl	$1, %edx		#; incl	%edx
	movb	%dl, -6(iBX, iSI)	#; CHS start sector number
	xorl	%edx, %edx		#; EDX:EAX=track number
	movl	$255, %ecx		#; heads=255
	divl	%ecx			#; EAX=cylinder number, EDX=head number
	movb	%dl, -7(iBX, iSI)	#; CHS start head number
	movb	%al, -5(iBX, iSI)	#; CHS start cylinder lo byte
	shlb	$6, %ah			#; CHS start cylinder hi 2 bits
	orb	%ah, -6(iBX, iSI)	#; put in the byte for CHS start sector number
	/* end LBA */
	xorl	%edx, %edx
	movl	(iBX, iSI), %eax	#; EDX:EAX=LBA start sector number
	addl	4(iBX, iSI), %eax
	subl	$1, %eax #; decl %eax	#; EDX:EAX=LBA end sector number
	/* translate end LBA to CHS */
	movl	$63, %ecx		#; sectors per track=63
	divl	%ecx			#; EAX=track number, EDX+1=sector number
	addl	$1, %edx		#; incl	%edx
	movb	%dl, -2(iBX, iSI)	#; CHS end sector number
	xorl	%edx, %edx		#; EDX:EAX=track number
	movl	$255, %ecx		#; heads=255
	divl	%ecx			#; EAX=cylinder number, EDX=head number
	movb	%dl, -3(iBX, iSI)	#; CHS end head number
	movb	%al, -1(iBX, iSI)	#; CHS end cylinder lo byte
	shlb	$6, %ah			#; CHS end cylinder hi 2 bits
	orb	%ah, -2(iBX, iSI)	#; put in the byte for CHS end sector number
	jmp	1b
1:
	/* end modify partition entry for sector 0(MBR) */

	/* modify BPB for sector 32 */
	movl	$MBR(0x4000), %esi
	movl	0x0E(iSI), %eax
	addl	$0x3F, %eax
	movb	%al, 0x0E(iSI)
	movb	%ah, 0x0F(iSI)

	movl	$0x20, 0x1C(iSI)	#; EAX=hidden sectors

	movl	0x13(iSI), %eax		#; AX = total sectors short
	andl	$0xFFFF, %eax
	jz	3f
	addl	$0x3F, %eax
	movb	%al, 0x13(iSI)
	movb	%ah, 0x14(iSI)
	shrl	$16, %eax
	jz	3f
	#; movw	$0, 0x13(iSI)		#; clear on overflow
	movb	%ah, 0x13(iSI)		#; AH = 0
	movb	%ah, 0x14(iSI)		#; AH = 0
3:

	cmpl	$0, 0x20(iSI)		#; total sectors long
	jz	3f
	addl	$0x3F, 0x20(iSI)
	jnc	3f
	movl	$0, 0x20(iSI)		#; clear on overflow
3:
	/* end modify BPB for sector 32 */

	/* modify partition entry for sector 32 */
	movl	$MBR(0x4000), %esi
	movl	$0x1B6, %ebx
1:
	movl	PRE(cs)ABS(mbr_free_sectors) rBP, %eax
	addl	$0x10, %ebx
	cmpl	$0x1F6, %ebx
	ja	1f
	cmpl	(iBX, iSI), %eax	#; mbr_free_sectors
	jne	3f
	movb	$0, -7(iBX, iSI)	#; CHS start head number
	movb	$0x20, -6(iBX, iSI)	#; CHS start sector number, lo
	movb	$0x00, -5(iBX, iSI)	#; CHS start sector number, hi
	subl	$0x3F, 4(iBX, iSI)	#; LBA total sectors
	addl	%eax, 4(iBX, iSI)	#; LBA total sectors
	xorl	%edx, %edx
	movl	%edx, -3(iBX, iSI)	#; clear end CHS
	movl	$0x1F, (iBX, iSI)	#; LBA start sector number
	
	movl	(iBX, iSI), %eax
	addl	4(iBX, iSI), %eax
	subl	$1, %eax #; decl %eax	#; EDX:EAX=LBA end sector number
	/* translate LBA to CHS */
	movl	$63, %ecx		#; sectors per track=63
	divl	%ecx			#; EAX=track number, EDX+1=sector number
	addl	$1, %edx		#; incl	%edx
	movb	%dl, -2(iBX, iSI)	#; CHS end sector number
	xorl	%edx, %edx		#; EDX:EAX=track number
	movl	$255, %ecx		#; heads=255
	divl	%ecx			#; EAX=cylinder number, EDX=head number
	movb	%dl, -3(iBX, iSI)	#; CHS end head number
	movb	%al, -1(iBX, iSI)	#; CHS end cylinder lo byte
	shlb	$6, %ah			#; CHS end cylinder hi 2 bits
	orb	%ah, -2(iBX, iSI)	#; put in the byte for CHS end sector number
	jmp	1b
3:
	/* start LBA */
	xorl	%edx, %edx
	movl	(iBX, iSI), %eax	#; EDX:EAX=LBA start sector number
	subl	$0x20, %eax
	jb	1b
	movl	%eax, (iBX, iSI)
	/* translate start LBA to CHS */
	movl	$63, %ecx		#; sectors per track=63
	divl	%ecx			#; EAX=track number, EDX+1=sector number
	addl	$1, %edx		#; incl	%edx
	movb	%dl, -6(iBX, iSI)	#; CHS start sector number
	xorl	%edx, %edx		#; EDX:EAX=track number
	movl	$255, %ecx		#; heads=255
	divl	%ecx			#; EAX=cylinder number, EDX=head number
	movb	%dl, -7(iBX, iSI)	#; CHS start head number
	movb	%al, -5(iBX, iSI)	#; CHS start cylinder lo byte
	shlb	$6, %ah			#; CHS start cylinder hi 2 bits
	orb	%ah, -6(iBX, iSI)	#; put in the byte for CHS start sector number
	/* end LBA */
	xorl	%edx, %edx
	movl	(iBX, iSI), %eax	#; EDX:EAX=LBA start sector number
	addl	4(iBX, iSI), %eax
	subl	$1, %eax #; decl %eax	#; EDX:EAX=LBA end sector number
	/* translate end LBA to CHS */
	movl	$63, %ecx		#; sectors per track=63
	divl	%ecx			#; EAX=track number, EDX+1=sector number
	addl	$1, %edx		#; incl	%edx
	movb	%dl, -2(iBX, iSI)	#; CHS end sector number
	xorl	%edx, %edx		#; EDX:EAX=track number
	movl	$255, %ecx		#; heads=255
	divl	%ecx			#; EAX=cylinder number, EDX=head number
	movb	%dl, -3(iBX, iSI)	#; CHS end head number
	movb	%al, -1(iBX, iSI)	#; CHS end cylinder lo byte
	shlb	$6, %ah			#; CHS end cylinder hi 2 bits
	orb	%ah, -2(iBX, iSI)	#; put in the byte for CHS end sector number
	jmp	1b
1:
	/* end modify partition entry for sector 32 */

	/* modify BPB for sector 63 */
	movl	$MBR(0x7E00), %esi

	#; addw	$0x20, 0x0E(iSI)	#; reserved sectors
	movl	0x0E(iSI), %eax
	addl	$0x20, %eax
	movb	%al, 0x0E(iSI)
	movb	%ah, 0x0F(iSI)

	movl	$0x1F, 0x1C(iSI)	#; EAX=hidden sectors

	movl	0x13(iSI), %eax		#; AX = total sectors short
	andl	$0xFFFF, %eax
	jz	3f
	addl	$0x20, %eax
	movb	%al, 0x13(iSI)
	movb	%ah, 0x14(iSI)
	shrl	$16, %eax
	jz	3f
	#; movw	$0, 0x13(iSI)		#; clear on overflow
	movb	%ah, 0x13(iSI)		#; AH = 0
	movb	%ah, 0x14(iSI)		#; AH = 0
3:

	cmpl	$0, 0x20(iSI)		#; total sectors long
	jz	3f
	addl	$0x20, 0x20(iSI)
	jnc	3f
	movl	$0, 0x20(iSI)		#; clear on overflow
3:
	/* end modify BPB for sector 63 */

	/* modify partition entry for sector 63 */
	movl	$MBR(0x7E00), %esi
	movl	$0x1B6, %ebx
1:
	movl	PRE(cs)ABS(mbr_free_sectors) rBP, %eax
	addl	$0x10, %ebx
	cmpl	$0x1F6, %ebx
	ja	1f
	cmpl	(iBX, iSI), %eax	#; mbr_free_sectors
	jne	3f
	movb	$0, -7(iBX, iSI)	#; CHS start head number
	movb	$0x21, -6(iBX, iSI)	#; CHS start sector number, lo
	movb	$0x00, -5(iBX, iSI)	#; CHS start sector number, hi
	//subl	$0x5F, 4(iBX, iSI)	#; LBA total sectors
	//addl	%eax, 4(iBX, iSI)	#; LBA total sectors
	xorl	%edx, %edx
	movl	%edx, -3(iBX, iSI)	#; clear end CHS
	movl	$0x20, (iBX, iSI)	#; LBA start sector number
	
	movl	(iBX, iSI), %eax
	addl	4(iBX, iSI), %eax
	subl	$1, %eax #; decl %eax	#; EDX:EAX=LBA end sector number
	/* translate LBA to CHS */
	movl	$63, %ecx		#; sectors per track=63
	divl	%ecx			#; EAX=track number, EDX+1=sector number
	addl	$1, %edx		#; incl	%edx
	movb	%dl, -2(iBX, iSI)	#; CHS end sector number
	xorl	%edx, %edx		#; EDX:EAX=track number
	movl	$255, %ecx		#; heads=255
	divl	%ecx			#; EAX=cylinder number, EDX=head number
	movb	%dl, -3(iBX, iSI)	#; CHS end head number
	movb	%al, -1(iBX, iSI)	#; CHS end cylinder lo byte
	shlb	$6, %ah			#; CHS end cylinder hi 2 bits
	orb	%ah, -2(iBX, iSI)	#; put in the byte for CHS end sector number
	jmp	1b
3:
	/* start LBA */
	xorl	%edx, %edx
	movl	(iBX, iSI), %eax	#; EDX:EAX=LBA start sector number
	subl	$0x3f, %eax
	jb	1b
	movl	%eax, (iBX, iSI)
	/* translate start LBA to CHS */
	movl	$63, %ecx		#; sectors per track=63
	divl	%ecx			#; EAX=track number, EDX+1=sector number
	addl	$1, %edx		#; incl	%edx
	movb	%dl, -6(iBX, iSI)	#; CHS start sector number
	xorl	%edx, %edx		#; EDX:EAX=track number
	movl	$255, %ecx		#; heads=255
	divl	%ecx			#; EAX=cylinder number, EDX=head number
	movb	%dl, -7(iBX, iSI)	#; CHS start head number
	movb	%al, -5(iBX, iSI)	#; CHS start cylinder lo byte
	shlb	$6, %ah			#; CHS start cylinder hi 2 bits
	orb	%ah, -6(iBX, iSI)	#; put in the byte for CHS start sector number
	/* end LBA */
	xorl	%edx, %edx
	movl	(iBX, iSI), %eax	#; EDX:EAX=LBA start sector number
	addl	4(iBX, iSI), %eax
	subl	$1, %eax #; decl %eax	#; EDX:EAX=LBA end sector number
	/* translate end LBA to CHS */
	movl	$63, %ecx		#; sectors per track=63
	divl	%ecx			#; EAX=track number, EDX+1=sector number
	addl	$1, %edx		#; incl	%edx
	movb	%dl, -2(iBX, iSI)	#; CHS end sector number
	xorl	%edx, %edx		#; EDX:EAX=track number
	movl	$255, %ecx		#; heads=255
	divl	%ecx			#; EAX=cylinder number, EDX=head number
	movb	%dl, -3(iBX, iSI)	#; CHS end head number
	movb	%al, -1(iBX, iSI)	#; CHS end cylinder lo byte
	shlb	$6, %ah			#; CHS end cylinder hi 2 bits
	orb	%ah, -2(iBX, iSI)	#; put in the byte for CHS end sector number
	jmp	1b
1:
	/* end modify partition entry for sector 63 */

	/* modify BPB for sector 95 */
	movl	$MBR(0xBE00), %esi
	movl	$0x20, 0x1C(iSI)	#; EAX=hidden sectors
	/* end modify BPB for sector 95 */

	movl	%edi, %ebx		#; restore file descriptor
	movl	$(63+32+1), PRE(cs)ABS(sectors_to_write) rBP

//----------------------------------------------------------------------------
2:	/* label for the above floppy code to jump */

	/* OK. Now write file! */

	xorl	%eax, %eax
	
	cmpl	$2, PRE(cs)ABS(read_only) rBP	#; 0=read only, 2=read/write
	jne	3f		/* read only */

	/* read/write */

#ifdef __DOS_16

	cmpl	$0xff, PRE(cs)ABS(bios_drive_number)
	je	1f		/* not bios */

	/* BIOS drive */
	/* Display disk information */
	movb	PRE(cs)ABS(bios_drive_number), %al	//bios id
	subb	$0x50, %al
	movb	%al, PRE(cs)ABS(msg_disk)
	movw	$0x2020, %ax
	movw	%ax, PRE(cs)ABS(msg_d)
	movl	0x1ca, %eax
	addl	0x1da, %eax
	addl	0x1ea, %eax
	addl	0x1fa, %eax
	call	66f
	//Wait for key
	movl	$ABS(msg_Wait_for_key), %ecx
	call	8f	/* linux_print */
2:	
	movb	$1, %ah	//Get keyboard input
	int	$0x16
	je	2b	//not
	xorl	%ecx, %ecx      	
	cmpb	$0x79, %al	//"y"
	jne	4f	//not
1:
#endif
	
	call 300f
3:
	testl	%eax, %eax
	movl	$ABS(msg_write_file), %ecx
	js	4f	/* error */

//=======================================================================================	
	cmpl	$0xff, PRE(cs)ABS(gpt) rBP
	jnz	1f

	movl	$ABS(grldr_mbr_start), %esi
	movl	$MBR(0), %edi
	movl	$0x2000/4, %ecx
#ifdef __DOS_16
	cs repz movsl
#else
	repz movsl
#endif	
	movl	PRE(cs)ABS(install_sectors) rBP, %ecx
	shll	$9, %ecx
	call	400f
	testl	%eax, %eax
	movl	$ABS(msg_lseek_file), %ecx
	js	4f		/* error */
	movl	$16, PRE(cs)ABS(sectors_to_write) rBP
	movl	PRE(cs)ABS(install_sectors) rBP, %ecx
	call 300f
	testl	%eax, %eax
	movl	$ABS(msg_write_file), %ecx
	js	4f	/* error */

1:
//----------------------------------------------------------------------------
	cmpl	$0xffffffff, PRE(cs)ABS(floppy) rBP
	jne	3f	/* floppy, skip geometry */

	/* hard drive */

	cmpl	$0, PRE(cs)ABS(probed_sectors_per_track) rBP
	je	2f

	cmpl	$0, PRE(cs)ABS(probed_heads) rBP
	je	2f

	movl	$ABS(msg_geometry_S), %ecx
	call	8f	/* linux_print */

	movl	PRE(cs)ABS(probed_sectors_per_track) rBP, %eax
	call	6f	/* print decimal number */

	movl	$ABS(msg_geometry_H), %ecx
	call	8f	/* linux_print */

	movl	PRE(cs)ABS(probed_heads) rBP, %eax
	call	6f	/* print decimal number */
2:
	cmpl	$(63+32+1), PRE(cs)ABS(sectors_to_write) rBP
	jne	3f
	movl	$ABS(msg_success_create_triple_mbr), %ecx	/* create triple MBR */
	cmpl	$0, PRE(cs)ABS(restore_partition_layout) rBP
	je	10f
	movl	$ABS(msg_success_cancel_triple_mbr), %ecx	/* cancel triple MBR */
	jmp	10f
3:
	movl	$ABS(msg_success), %ecx
10:
	call	8f	/* linux_print */
//3:
//success:

	# EBX=file descriptor, or 0 if specified BIOS drive
	
	xorl	%eax, %eax	# exit code = 0

	jmp	1f
	
//error:
4:
	# EBX=file descriptor, or 0 if specified BIOS drive
	
	testl	%ecx, %ecx
	jz	2f
	call	8f	/* linux_print */
2:
	movl	$ABS(msg_usage), %ecx
	call	8f	/* linux_print */

	movl	$1, %eax	# exit code = 1

1:
	testl	%ebx, %ebx
	jz	1f

	pushl	%eax		# exit code

#ifdef __DOS_16

	movb	$0x3e, %ah	// close file (BX = file handle)
	int	$0x21
	jnc	2f
	negl	%eax		/* EAX < 0 */
2:
#else
	/* int close(int fd); */

	/* check if in 32-bit or 64-bit mode */
	xorl	%eax, %eax
	decl	%eax		// rex prefix of 0x48
	testl	%eax, %eax
	/* EAX = 0 for 64-bit mode, -1 for 32-bit mode */
	jz	2f
	
	/* in 32-bit mode */
	movl	$6, %eax	# sys_close
	int	$0x80
	jmp	3f
2:
	/* in 64-bit mode */
	movl	$3, %eax	# sys_close
	pushl	%ebx
	popl	%edi
	syscall
3:
#endif
	movl	$ABS(msg_close_file), %ecx
	testl	%eax, %eax
	
	popl	%eax		# exit code

	jns	1f

	call	8f	/* linux_print */

//exit:
1:
#ifdef __DOS_16
	
	movb	$0x4c, %ah	// EXIT - TERMINATE WITH RETURN CODE in AL
	int	$0x21		// call DOS

#else
	pushl	%eax		// save exit code
	/* check if in 32-bit or 64-bit mode */
	xorl	%eax, %eax
	decl	%eax		// rex prefix of 0x48
	testl	%eax, %eax
	/* EAX = 0 for 64-bit mode, -1 for 32-bit mode */
	popl	%eax		// restore exit code
	jz	2f
	
	/* in 32-bit mode */
	xchgl	%eax, %ebx	# move exit code in EAX to EBX
	movl	$1, %eax	# sys_exit
	int	$0x80
	jmp	3f
2:
	/* in 64-bit mode */
	xchgl	%eax, %edi	# move exit code in EAX to EDI
	movl	$60, %eax	# sys_exit
	syscall
3:
#endif
		
	ret

//----------------------------------------------------------------------------
//floppy:
5:

	#; pushal
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	pushl	%ebx
	pushl	%ebp
	pushl	%esi
	pushl	%edi

	/* Beforehand, try MBR */

	/* ESI=$MBR(0) */

	cld
	cmpw	$0xAA55, 0x1fe(iSI)
	jne	1f

	pushl	%esi
	pushl	%edi
	pushl	%ebx	

	/* has a valid partition table ? */
	movb	$0, %bl		/* count valid partition entries */
	addl	$0x1be, %esi
3:
	cmpl	$MBR(0x1fe), %esi
	jnb	3f		/* partition table is OK */
	movl	$4, %ecx

	movl	%esi, %edi
2:
	lodsl
	negl	%eax
	jc	2f
	loop	2b
	/* empty entry, check next */
	jmp	3b
2:
	/* non-empty entry */
	movl	%edi, %esi

	lodsl
	shlb	$1, %al		/* boot indicator */
	jnz	2f
	shrl	$16, %eax
	andb	$63, %al
	jz	2f
	lodsl
	shrl	$16, %eax
	andb	$63, %al
	jz	2f
	lodsl
	negl	%eax
	jnc	2f
	lodsl
	negl	%eax
	jnc	2f
	incb	%bl
	jmp	3b
2:
	stc		/* invalid partition table */
3:
	popl	%eax
	popl	%edi
	popl	%esi

	xchgl	%eax, %ebx

	jc	1f
	testb	%al, %al
	jz	1f

	/* MBR write not allowed */
	
	movl	$0, PRE(cs)ABS(sectors_to_write) rBP
	
	movl	$ABS(msg_fstype_mbr_deny), %ecx
	call	8f	/* linux_print */
	
	stc
	#; popal
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
	popl	%edx
	popl	%ecx
	popl	%eax
	ret
1:
	/* First, try ext2 */

	cmpw	$0xEF53, 0x438(iSI)	/* Magic signature, lo */
	jnz	1f
	xorl	%eax, %eax
	cmpl	%eax, 0x400(iSI)	/* s_inodes_count */
	jz	1f
	cmpl	%eax, 0x404(iSI)	/* s_blocks_count */
	jz	1f
	cmpl	%eax, 0x420(iSI)	/* s_blocks_per_group */
	jz	1f
	cmpl	%eax, 0x428(iSI)	/* s_inodes_per_group */
	jz	1f
	movl	0x414(iSI), %eax	/* s_first_data_block */
	movl	%eax, %ebx		/* BX=1 for 1K block, 0 otherwise */
	shrl	$1, %eax		/* must be 0 */
	jnz	1f
	movl	0x418(iSI), %ecx	/* s_log_block_size */
	cmpl	$4, %ecx		/* max size of block is 16K */
	ja	1f
	negl	%ecx			/* CF=0 for 1K block, CF=1 otherwise */
	adcl	%eax, %ebx		/* EAX=0 */
	subl	$1, %ebx		#; decl %ebx
	jnz	1f

	/* BX = 0 */
	/* write to ext2 boot area */

	cld	
	//To install EXT2 PBR boot code
	movl	$ABS(grldr_pbr_start + 0x400), %esi
	movl	$MBR(0), %edi
	movl	$(0x400/4), %ecx
	repz	movsl PRE(cs)(iSI), (iDI)
	movl	$2, PRE(cs)ABS(sectors_to_write) rBP
	movl	$ABS(msg_fstype_ext2_allow), %ecx
	call	8f	/* linux_print */
	
	clc
	#; popal
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

1:
	/* Secondly, try FAT12/16/32/NTFS */
	
	cld
	cmpw	$0xAA55, 0x1fe(iSI)	/* boot signature */
	jne	1f			/* not a normal BPB */
	cmpw	$0x200, 0x0b(iSI)	/* bytes per sector */
	jne	1f			/* not a normal BPB */
	movb	0x0d(iSI), %al		/* sectors per cluster */
	testb	%al, %al
	jz	1f			/* invalid if = 0 */
	movb	%al, %cl
	movl	$128, %eax
	divb	%cl			/* quo=AL, rem=AH */
	testb	%ah, %ah
	jnz	1f			/* invalid if not 2^n */
	movl	0x18(iSI), %eax		/* AX = sectors per track */
	andl	$0xFFFF, %eax
	jz	1f			/* invalid if = 0 */
	cmpl	$63, %eax
	ja	1f			/* invalid if > 63 */
	movl	0x1a(iSI), %eax		/* AX = number of heads */
	subl	$1, %eax		#; decl %eax
					/* AX = Max head number, should be a byte */
	testb	%ah, %ah		/* should be 0 */
	jnz	1f			/* invalid if number of heads > 256 */
	cmpb	$0xf0, 0x15(iSI)	/* media descriptor */
	jb	1f

	movb	0x10(iSI), %al		/* number of FATs(NTFS:0, FAT:1,2) */
	cmpb	$2, %al
	ja	1f			/* abnormal FAT */
	movl	0x11(iSI), %eax		/* AX = max root entries */
	andl	$0xFFFF, %eax
	jnz	2f			/* FAT12/FAT16 */

	/* FAT32 or NTFS */
	movl	0x13(iSI), %eax		/* AX = total sectors(small) */
	andl	$0xFFFF, %eax
	jnz	1f			/* invalid FAT32 BPB */
	movl	0x16(iSI), %eax		/* sectors per FAT(small) */
	andl	$0xFFFF, %eax
	jnz	1f			/* invalid FAT32 BPB */
	movb	0x10(iSI), %al		/* number of FATs(NTFS:0, FAT:1,2) */
	testb	%al, %al
	jz	4f			/* NTFS */

	/* FAT32 */
	movl	0x20(iSI), %eax		/* FAT32 total sectors */
	testl	%eax, %eax
	jz	1f
	movl	0x24(iSI), %eax		/* FAT32 sectors per FAT */
	testl	%eax, %eax
	jz	1f
	
	/* sure it is FAT32 */	
	cld
	//To install FAT32 PBR boot code
	movl	$ABS(grldr_pbr_start), %esi
	movl	$MBR(0), %edi
	movsw	PRE(cs)(iSI), (iDI)
	movb	$0x90, %al
	stosb
	movl	$ABS(grldr_pbr_start + 0x5a), %esi
	movl	$MBR(0x5a), %edi
	movl	$((0x200-0x5a)/2), %ecx
	repz movsw PRE(cs)(iSI), (iDI)	
	movl	$1, PRE(cs)ABS(sectors_to_write) rBP		
	movl	$ABS(msg_fstype_fat32_allow), %ecx
	call	8f	/* linux_print */
	
	clc
	#; popal
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
	popl	%edx
	popl	%ecx
	popl	%eax
	ret
4:
	/* NTFS */
	movl	0x20(iSI), %eax		/* FAT32 total sectors */
	testl	%eax, %eax
	jnz	1f
	movl	0x0e(iSI), %eax		/* reserved sectors */
	andl	$0xFFFF, %eax
	jnz	1f
	/* sure it is NTFS */

	cld
//	movl	$ABS(_start1 + 0xA00), %esi	/* points to the 6th sector */
	movl	$ABS(grldr_pbr_start + 0x0c00), %esi
#ifdef __DOS_16
	xorl	%edi, %edi
	cs movsw
#else
	movl	$MBR(0), %edi
	#; lodsw PRE(cs)(iSI), %ax	/* The first 2 byte: short jmp */
	movsw
#endif

	movb	$0x90, %al		/* yes, it is chs. */
	stosb

	movl	$ABS(grldr_pbr_start + 0x0c54), %esi
	movl	$MBR(0x54), %edi
	movl	$((0x800 - 0x54)/ 4), %ecx	
	repz movsl PRE(cs)(iSI), (iDI)
	
	movl	$4, PRE(cs)ABS(sectors_to_write) rBP
	
	movl	$ABS(msg_fstype_ntfs_allow), %ecx
	call	8f	/* linux_print */
	
	clc
	#; popal
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

2:
	/* FAT12/FAT16 */
	movb	0x10(iSI), %al		/* number of FATs(NTFS:0, FAT:1,2) */
	testb	%al, %al
	jz	1f
	movl	0x16(iSI), %eax		/* AX = sectors per FAT(small) */
	andl	$0xFFFF, %eax
	jz	1f

	cld
	//Installation FAT12/16 PBR boot code
	movl	$ABS(grldr_pbr_start + 0x200), %esi
	movl	$MBR(0), %edi
	movsw PRE(cs)(iSI), (iDI)
	movb	$0x90, %al
	stosb
	movl	$ABS(grldr_pbr_start + 0x23e), %esi
	movl	$MBR(0x3e), %edi
	movl	$((0x200-0x3e)/2), %ecx	
	repz	movsw PRE(cs)(iSI), (iDI)	
	movl	$1, PRE(cs)ABS(sectors_to_write) rBP
	movl	$ABS(msg_fstype_fat16_allow), %ecx
	movw MBR(0x39), %ax
	cmpw	$0x3631, %ax
	je	2f	
	movl	$ABS(msg_fstype_fat12_allow), %ecx
2:	
	call	8f	/* linux_print */
	
	clc
	#; popal
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

1:
	/* Secondly, try EXFAT */
	cmpl	$0x41465845, 0x03(iSI)			/* EXFA */
	jnz	1f
	cmpl	$0x20202054, 0x07(iSI)			/* T    */
	jnz	1f

	/* write to EXFAT boot area */
	
	cld
	//Install exFAT PBR boot code
	movl	$ABS(grldr_pbr_start + 0x800), %esi
	movl	$MBR(0), %edi
	movsw PRE(cs)(iSI), (iDI)
	movb	$0x90, %al
	stosb
	movl	$ABS(grldr_pbr_start + 0x878), %esi
	movl	$MBR(0x78), %edi
	movl	$((0x400-0x78)/4), %ecx	
	repz movsl PRE(cs)(iSI), (iDI)
	//Fill sector
	xorl	%eax, %eax
	movl	$MBR(0x400), %edi
	movl	$0x480, %ecx
	repz	stosl
	movl	$MBR(0), %esi
	movw	$0xAA55, %ax
	movl	$7, %ecx
3:	
	movw	%ax, 0x5fe(%esi)
	addl	$0x200, %esi
	loop 3b

//Calculation exfat volume boot record checksum
//si=the index entry (from 0 to cx) eax=checksum  cx=volume boot record check bytes (0x1600)
//	xorw	%si, %si	//Initialize the index
	xorl	%eax, %eax	//Initialize the checksum
	movl	$MBR(0x1600), %ecx	//Bytes volume boot record check
3:
	rorl	$1, %eax	//"Checksum" right Cycle 1 bit
	movzbl	(%esi), %ebx	//Check items
	addl	%ebx, %eax	//(("Checksum" Left Cycle 31 bit) or ("Checksum" right Cycle 1 bit)) + Check items
2:
	addl	$1, %esi	//index+1
	cmpl	$MBR(0x6a), %esi                      //index=0x6a Exclude
	je	2b
	cmpl	$MBR(0x6b), %esi                      //index=0x6b Exclude
	je	2b
	cmpl	$MBR(0x70), %esi                      //index=0x70 Exclude
	je	2b
	cmpl	%ecx, %esi	//index=Maximum?
	jb	3b	//No, continue to
//Filling Checksum 	
	movl	$0x80, %ecx
	repz	stosl	//0x200 byte   
//Modify the backup area
	movl	$0x600, %ecx
	movl	$MBR(0), %esi
	repz movsl
	
	movl	$24, PRE(cs)ABS(sectors_to_write) rBP
	movl	$ABS(msg_fstype_exfat_allow), %ecx
	call	8f	/* linux_print */
	
	clc
	#; popal
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
	popl	%edx
	popl	%ecx
	popl	%eax
	ret	
1:
	movl	$ABS(msg_unsupported_fstype), %ecx
	movl	$0, PRE(cs)ABS(sectors_to_write) rBP		
	call	8f	/* linux_print */
	
	stc
	#; popal
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

//----------------------------------------------------------------------------
//print decimal number in EAX
6:
	/* print EAX value in decimal format */
	#; pushal
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	pushl	%ebx
	pushl	%ebp
	pushl	%esi
	pushl	%edi

#ifdef __DOS_16
	pushw	%cs
	popw	%es
#endif

#if 0
	cld
	movl	$ABS(number_digits), %edi
	xchgl	%eax, %edx	// save EAX to EDX
	movl	$0x20202020, %eax	// 4 spaces
	stosl
	stosl
	stosb
	stosb
	subl	$1, %edi		#; decl %edi
	xchgl	%eax, %edx	// restore EAX from EDX
#endif
	std
	movl	$ABS(number_digits + 9), %edi
	movl	$10, %ebp
1:
	xorl	%edx, %edx
	divl	%ebp		// quo=EAX, rem=EDX
	xchgl	%eax, %edx
	orb	$0x30, %al
	stosb
	xchgl	%eax, %edx
	testl	%eax, %eax
	jnz	1b

	/* all digits are stored */
	addl	$1, %edi		// points to the digit string

	movl	%edi, %ecx
	call	8f	/* linux_print */

	cld
#ifdef __DOS_16
	pushw	%ss
	popw	%es
#endif
	#; popal
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

//----------------------------------------------------------------------------
//parse_number:
7:

	/* input:	EDI	points to the number
	 *		EBX	upper limit of the number
	 * output:	EBX	the value of the number
	 *		EDX	changed
	 *		ESI	changed
	 *		CF=1	failure
	 *		CF=0	success
	 */

	pushl	%ebp
	pushl	%eax
	pushl	%ebx		/* upper limit */
	cld
	pushl	%edi
	popl	%esi
	movl	$0xffffffff, %edx
	xorl	%ebx, %ebx
	lodsw
	cmpw	$0x5830, %ax	/* 0X */
	je	1f
	cmpw	$0x7830, %ax	/* 0x */
	je	1f

	/* decimal */
	pushl	%edi
	popl	%esi
	movl	$10, %ebp
2:
	xorl	%eax, %eax
	lodsb
	cmpb	$0, %al
	je	4f		/* CF=0 */
	cmpb	$0x30, %al
	jb	3f
	cmpb	$0x39, %al
	ja	3f
	subb	$0x30, %al
	pushl	%eax
	movl	%ebx, %eax
	mull	%ebp
	movl	%eax, %ebx
	popl	%eax
	jc	3f
	addl	%eax, %ebx
	jc	3f
	popl	%eax
	pushl	%eax		/* upper limit */
	cmpl	%eax, %ebx
	ja	3f
	jmp	2b
1:

	/* hex */
	movl	$16, %ebp
2:
	xorl	%eax, %eax
	lodsb
	cmpb	$0, %al
	je	4f		/* CF=0 */
	cmpb	$0x30, %al
	jb	3f
	cmpb	$0x39, %al
	jbe	1f
	orb	$0x20, %al
	cmpb	$0x61, %al
	jb	3f
	cmpb	$0x66, %al
	ja	3f
	subb	$0x27, %al
1:
	subb	$0x30, %al
	pushl	%eax
	movl	%ebx, %eax
	mull	%ebp
	movl	%eax, %ebx
	popl	%eax
	jc	3f
	addl	%eax, %ebx
	jc	3f
	popl	%eax
	pushl	%eax		/* upper limit */
	cmpl	%eax, %ebx
	ja	3f
	jmp	2b

3:
	stc
	popl	%eax		/* upper limit */
	popl	%eax
	popl	%ebp
	ret
4:
	testl	%edx, %edx	/* CF=0 */
	jnz	3b
	popl	%eax		/* upper limit */
	popl	%eax
	popl	%ebp
	ret

//----------------------------------------------------------------------------
//linux_print:
8:

	/* print a message.
	 * input:	ECX	points to message string
	 * output:	message is displayed on the stdout device
	 */

	/* ssize_t write(int fd, const void *buf, size_t count) */
	#; pushal
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	pushl	%ebx
	pushl	%ebp
	pushl	%esi
	pushl	%edi

#ifdef __DOS_16

	cld
	movl	%ecx, %esi
1:      
	lodsb	%cs:(%si), %al		// get token
	cmpb	$0, %al			// end of string?
	je	1f
#if 0
	/* use BIOS independent of DOS */
	xorw	%bx, %bx		// video page 0
	movb	$0x0e, %ah		// print it
	int	$0x10			// via TTY mode
#else
	/* use stdout for redirection of the output */
	movb	%al, %dl		// character to write
	movb	$0x02, %ah		// WRITE CHARACTER TO STANDARD OUTPUT
	int	$0x21			// call DOS
#endif
	jmp	1b			// until done
1:

#else
	pushl	%ecx
	movl	%ecx, %edi	# scan the message string ...
	movl	$(msg_end - msg_start), %ecx
	movb	$0, %al		# ... find the ending zero byte

	cld
	repnz scasb

	movl	%edi, %edx
	popl	%ecx		# ECX points to the message
	subl	%ecx, %edx	# EDX=message length
	subl	$1, %edx	#; decl %edx
				# discard the ending zero byte

	/* check if in 32-bit or 64-bit mode */
	xorl	%eax, %eax
	decl	%eax		// rex prefix of 0x48
	testl	%eax, %eax
	/* EAX = 0 for 64-bit mode, -1 for 32-bit mode */
	jz	1f
	
	/* in 32-bit mode */
	movl	$4, %eax        # sys_write
	movl	$1, %ebx        # file descriptor = stdout
	int	$0x80
	jmp	3f
1:
	/* in 64-bit mode */
	movl	$1, %eax        # sys_write
	movl	$1, %edi        # file descriptor = stdout
	movl	%ecx, %esi
	syscall
3:
#endif

	#; popal
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

//----------------------------------------------------------------------------
//probe_geometry:
9:
	#; pushal
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	pushl	%ebx
	pushl	%ebp
	pushl	%esi
	pushl	%edi

	cmpl	$0xff, PRE(cs)ABS(gpt) rBP
	jz	88f
	
	addl	$0x200, %esi
	
	movl	$0, PRE(cs)ABS(mbr_free_sectors) rBP	# geometry ok
	movl	$0, PRE(cs)ABS(Cmax) rBP
	movl	$0, PRE(cs)ABS(Hmax) rBP
	movl	$0, PRE(cs)ABS(Smax) rBP

	movl	$0, PRE(cs)ABS(i) rBP			# partition number
1:
	cmpl	$4, PRE(cs)ABS(i) rBP
	jae	1f

	/* the boot indicator must be 0x80(bootable) or 0(non-bootable) */

	movl	PRE(cs)ABS(i) rBP, %ebx
	movl	PRE(cs)ABS(i) rBP, %edi
	shll	$4, %ebx		# EBX = i * 16
	movb	-66(iBX, iSI), %al	# boot_indicator
	shlb	$1, %al
	jnz	6f	/* geometry_probe_failed */

	/* check if the entry is empty, i.e., all the 16 bytes are 0 */

	movl	-66(iBX, iSI), %eax
	orl	-62(iBX, iSI), %eax
	orl	-58(iBX, iSI), %eax
	orl	-54(iBX, iSI), %eax
	jz	2f			# empty partition entry

	/* valid partitions never start at 0, because this is where the MBR
	 * lives; and more, the number of total sectors should be non-zero.
	 */

	cmpl	$0, -58(iBX, iSI)	# start_lba
	jz	6f	/* geometry_probe_failed */

	cmpl	$0, -54(iBX, iSI)	# total_sectors
	jz	6f	/* geometry_probe_failed */

	/* the partitions should not overlap each other */

	movl	$-1, PRE(cs)ABS(j) rBP		// j = -1
3:
	// for (j = 0; j < i; j++)
	incl	PRE(cs)ABS(j) rBP			// j++
	cmpl	%edi, PRE(cs)ABS(j) rBP		# i = EDI
	jae	3f			// j >= i, break.

	movl	-58(iBX, iSI), %ecx	# ECX=P[i].start_lba

	movl	PRE(cs)ABS(j) rBP, %edx
	shll	$4, %edx		# EDX = j * 16
	MOVL	-58(%edx, %esi), %eax	# EAX=P[j].start_lba

	// if ((P[j].start_lba <= P[i].start_lba) && (P[j].start_lba + P[j].total_sectors >= P[i].start_lba + P[i].total_sectors))

	cmpl	%ecx, %eax
	ja	4f

	pushl	%ecx
	pushl	%eax

	addl	-54(iBX, iSI), %ecx	// ECX = P[i].start_lba + P[i].total_sectors
	ADDL	-54(%edx, %esi), %eax	// EAX = P[j].start_lba + P[j].total_sectors

	cmpl	%ecx, %eax

	popl	%eax
	popl	%ecx

	jb	4f

	jmp	3b			//continue
4:
	// if ((P[j].start_lba >= P[i].start_lba) && (P[j].start_lba + P[j].total_sectors <= P[i].start_lba + P[i].total_sectors))
	
	cmpl	%ecx, %eax
	jb	4f

	pushl	%ecx
	pushl	%eax

	addl	-54(iBX, iSI), %ecx	// ECX = P[i].start_lba + P[i].total_sectors
	ADDL	-54(%edx, %esi), %eax	// EAX = P[j].start_lba + P[j].total_sectors

	cmpl	%ecx, %eax

	popl	%eax
	popl	%ecx

	ja	4f

	jmp	3b			//continue
4:

	cmpl	%ecx, %eax
	jae	4f

	// P[j].start_lba < P[i].start_lba
	subl	%eax, %ecx		// ECX = P[i].start_lba - P[j].start_lba
	CMPL	-54(%edx, %esi), %ecx	# ECX < P[j].total_sectors
	jmp	5f
4:
	// P[j].start_lba >= P[i].start_lba
	subl	%ecx, %eax		// EAX = P[j].start_lba - P[i].start_lba
	cmpl	-54(iBX, iSI), %eax	# EAX < P[i].total_sectors
5:
	jb	6f	/* geometry_probe_failed */

	jmp	3b			//continue
3:

	/* the start cylinder number */

	#; movzwl	-64(iBX, iSI), %eax	# start_sector_cylinder
	movl	-64(iBX, iSI), %eax	# start_sector_cylinder
	andl	$0xFFFF, %eax


	movl	%eax, %edx
	andl	$0x003f, %edx
	jz	6f	/* geometry_probe_failed */

	movl	%edx, PRE(cs)ABS(X) rBP			# the sector number

	cmpl	%edx, PRE(cs)ABS(Smax) rBP
	jae	3f
	movl	%edx, PRE(cs)ABS(Smax) rBP
3:
	movl	-58(iBX, iSI), %ecx	# start_lba
	addl	$1, %ecx		#; incl	%ecx
	subl	%edx, %ecx

	movl	$ABS(L), %edx
	MOVL	%ecx, PRE(cs)(%edx, %edi, 8)	# L[i]

	shrb	$6, %al
	xchgb	%al, %ah
	andl	$0xFFFF, %eax

	movl	$ABS(C), %edx
	MOVL	%eax, PRE(cs)(%edx, %edi, 4)	# C[i]

	cmpl	%eax, PRE(cs)ABS(Cmax) rBP
	jae	3f
	movl	%eax, PRE(cs)ABS(Cmax) rBP
3:

	movzbl	-65(iBX, iSI), %eax	# start_head
	movl	$ABS(H), %edx
	MOVL	%eax, PRE(cs)(%edx, %edi, 4)	# H[i]

	cmpl	%eax, PRE(cs)ABS(Hmax) rBP
	jae	3f
	movl	%eax, PRE(cs)ABS(Hmax) rBP
3:

	/* the end cylinder number */

	#; movzwl	-60(iBX, iSI), %eax	# end_sector_cylinder
	movl	-60(iBX, iSI), %eax	# end_sector_cylinder
	andl	$0xFFFF, %eax

	movl	%eax, %edx
	andl	$0x003f, %edx
	jz	6f	/* geometry_probe_failed */

	movl	%edx, PRE(cs)ABS(Y) rBP			# the sector number

	cmpl	%edx, PRE(cs)ABS(Smax) rBP
	jae	3f
	movl	%edx, PRE(cs)ABS(Smax) rBP
3:
	movl	-58(iBX, iSI), %ecx	# start_lba
	addl	-54(iBX, iSI), %ecx	# total_sectors
					# XXX: possible overflow!

	subl	%edx, %ecx
	jb	6f	/* geometry_probe_failed */

	addl	$4, %edi		# EDI=i+4

	movl	$ABS(L), %edx
	MOVL	%ecx, PRE(cs)(%edx, %edi, 8)	# L[i+4]

	shrb	$6, %al
	xchgb	%al, %ah
	andl	$0xFFFF, %eax

	movl	$ABS(C), %edx
	MOVL	%eax, PRE(cs)(%edx, %edi, 4)	# C[i+4]

	cmpl	%eax, PRE(cs)ABS(Cmax) rBP
	jae	3f
	movl	%eax, PRE(cs)ABS(Cmax) rBP
3:
	xchgl	%eax, %ecx		# save AX to CX
					# EAX = L[i+4]

	#####################################################################
	pushl	%esi			# save ESI before it changes
	#####################################################################

	pushl	%eax			# EAX = L[i+4]
	movzbl	-61(iBX, iSI), %eax	# end_head
	popl	%esi			# begin to use ESI(= L[i+4] initially)

	movl	$ABS(H), %edx
	MOVL	%eax, PRE(cs)(%edx, %edi, 4)	# H[i+4]

	cmpl	%eax, PRE(cs)ABS(Hmax) rBP
	jae	3f
	movl	%eax, PRE(cs)ABS(Hmax) rBP
3:

	/* Check the large disk partition -- Win98 */
	cmpl	$63, PRE(cs)ABS(Y) rBP
	jne	3f
	cmpl	%eax, PRE(cs)ABS(Hmax) rBP		# EAX=H[i+4]
	jne	3f
	cmpl	%ecx, PRE(cs)ABS(Cmax) rBP		# ECX=C[i+4]
	jne	3f

	cmpl	$254, %eax		# EAX=PRE(cs)ABS(Hmax)
	jnb	4f
	cmpl	$1022, %ecx		# CX=PRE(cs)ABS(Cmax)
	jb	3f
4:
	movl	$63, %edx		# EDX=PRE(cs)ABS(Y)
	addl	%edx, %esi
	addl	$1, %eax		#; incl	%eax
	mull	%edx			# EDX:EAX=product
					# EDX=0
					# EAX high word = 0
					# AX=(Hmax+1)*63
	pushl	%eax			# EAX=(Hmax+1)*63
	addl	$1, %ecx		#; incl	%ecx
	mull	%ecx			# EDX:EAX=product
					# EDX=0
	cmpl	%esi, %eax
	popl	%eax			# EAX=(Hmax+1)*63
	jnb	3f

	/* EDI=i+4 at this moment */

	pushl	%eax			# EAX=(Hmax+1)*63
	subl	$4, %edi		# EDI=i
	
	movl	$ABS(C), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %eax	# C[i]
	andl	$0xFFFF, %eax
	
	movl	PRE(cs)ABS(Hmax) rBP, %esi	# Hmax
	addl	$1, %esi		#; incl	%esi

	pushl	%eax			# EAX=C[i]

	movl	$ABS(H), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %ecx	# H[i]

	mull	%esi			# EDX=0, EAX=product
	addl	%ecx, %eax
	movl	$63, %edx
	mull	%edx			# EDX=0, EAX=product
	
	movl	$ABS(L), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 8), %esi	# L[i]

	cmpl	%esi, %eax
	popl	%eax			# EAX=C[i]

	jae	4f
	
	/* calculate CHS numbers from start LBA */

	//pushl	%eax			# EAX=C[i]

	xorl	%edx, %edx
	xchgl	%eax, %esi		# ESI=C[i], EDX:EAX=L[i]
	
	divl	PRE(cs)ABS(Y) rBP			# Y=63
					# quo=EAX, rem=EDX
	addl	$1, %edx		#; incl	%edx # sector number
	cmpl	%edx, PRE(cs)ABS(X) rBP
	je	15f
	cmpl	$63, PRE(cs)ABS(X) rBP
	jne	4f
15:
	xorl	%edx, %edx
	movl	PRE(cs)ABS(Hmax) rBP, %ebx
	addl	$1, %ebx		#; incl	%ebx # EBX=Hmax+1
	divl	%ebx			# quo=EAX=cylinder number
					# rem=EDX=head number
	cmpl	%edx, %ecx		# ECX=H[i]
	je	15f
	subl	$1, %ebx		#; decl %ebx
					# EBX=Hmax
	cmpl	%ebx, %ecx
	jne	4f
15:
	/* EAX=calculated cylinder number, ESI=C[i] */

	cmpl	PRE(cs)ABS(Cmax) rBP, %esi
	je	4f
	andl	$0x3ff, %eax		# low 10 bit of calculated cylinder
	cmpl	%eax, %esi
4:
	popl	%eax
	//##jne	6f	/* geometry_probe_failed */
	jmp	5f
3:

	/* Check the large disk partition -- Win2K */

	/* EDI=i+4 at this moment */
	cmpl	$63, PRE(cs)ABS(Y) rBP
	jne	3f
	movl	$ABS(H), %edx
	MOVL	PRE(cs)(%edx, %edi, 4), %eax	# H[i+4]
	cmpl	%eax, PRE(cs)ABS(Hmax) rBP		# EAX=H[i+4]
	jne	3f
	
	movl	$ABS(L), %edx
	MOVL	PRE(cs)(%edx, %edi, 8), %esi	# L[i+4]
	
	movl	$ABS(C), %edx
	MOVL	PRE(cs)(%edx, %edi, 4), %ecx	# C[i+4]
	andl	$0xFFFF, %ecx

	addl	$1, %ecx		#; incl	%ecx
	addl	$1, %eax		#; incl	%eax
	
	mull	%ecx			# EDX=0, EAX=product
	mull	PRE(cs)ABS(Y) rBP			# Y=63, EDX=0, EAX=product
	addl	PRE(cs)ABS(Y) rBP, %esi
	cmpl	%esi, %eax
	jnb	3f

	movl	PRE(cs)ABS(Hmax) rBP, %eax
	addl	$1, %eax		#; incl	%eax
	mull	PRE(cs)ABS(Y) rBP		# Y=63, EDX=0, EAX=product
	
	xorl	%edx, %edx
	xchgl	%eax, %esi		# ESI=(Hmax+1)*63
					# EAX=L[i+4]+Y	

	divl	%esi			# EAX=quo, EDX=rem
	testl	%edx, %edx
	jnz	3f
	subl	$1, %eax		#; decl %eax
	andl	$0x3ff, %eax
	subl	$1, %ecx		#; decl %ecx
	cmpl	%eax, %ecx
	jne	3f
	
	/* EDI=i+4 at this moment */

	subl	$4, %edi		# EDI=i

	movl	$ABS(C), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %eax	# C[i]

	pushl	%eax
	mull	%esi			# ESI=(Hmax+1)*63
					# EDX=0, EAX=product
	
	pushl	%esi			# ESI=(Hmax+1)*63
	
	xchgl	%eax, %esi		# ESI=product
	movl	$ABS(H), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %eax	# H[i]
	mull	PRE(cs)ABS(Y) rBP			# Y=63, EDX=0, EAX=H[i]*63

	pushl	%eax

	addl	%eax, %esi
	
	movl	$ABS(L), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 8), %eax	# L[i]
	cmpl	%eax, %esi

	popl	%ebx			# EBX=H[i]*63
	popl	%esi			# ESI=(Hmax+1)*63
	popl	%ecx			# ECX=C[i]
	xchgl	%eax, %esi

	jae	5f
	xchgl	%eax, %esi
	subl	%ebx, %eax
	xorl	%edx, %edx
	divl	%esi			# EAX=quo, EDX=rem
	testl	%edx, %edx
	//##jnz	6f	/* geometry_probe_failed */
	jnz	5f	/* geometry_probe_failed */
	andl	$0x3ff, %eax
	cmpl	%eax, %ecx
	//##jne	6f	/* geometry_probe_failed */
	jne	5f	/* geometry_probe_failed */
	
	xchgl	%eax, %esi
	jmp	5f
3:
	/* Maximum of C[n] * (H * S) + H[n] * S = 1023 * 255 * 63 + 254 * 63 = 0xFB03C1 */

	/* EDI=i+4 at this moment */
	movl	$ABS(L), %edx
	MOVL	PRE(cs)(%edx, %edi, 8), %esi	# L[i+4]
	cmpl	$0xFB03C1, %esi
	jbe	3f

	/* set H/S to max */
	cmpl	$254, PRE(cs)ABS(Hmax) rBP
	jae	4f
	movl	$254, PRE(cs)ABS(Hmax) rBP
4:
	movl	$63, PRE(cs)ABS(Smax) rBP
	xorl	%edx, %edx
	movl	%esi, %eax
	divl	PRE(cs)ABS(Smax) rBP
	testl	%edx, %edx
	//##jnz	6f	/* geometry_probe_failed */
	jnz	5f	/* geometry_probe_failed */

	/* EAX=L[i+4]/63, EDX=0 */

	movl	$ABS(H), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %ebx	# EBX=H[i+4]
	movl	PRE(cs)ABS(Hmax) rBP, %ecx
	addl	$1, %ecx		#; incl	%ecx	# ECX=Hmax+1
	divl	%ecx
	cmpl	%edx, %ebx
	je	15f
	cmpl	PRE(cs)ABS(Hmax) rBP, %ebx
	//##jne	6f	/* geometry_probe_failed */
	jne	5f	/* geometry_probe_failed */
15:
	/* EAX=L[i+4]/63/(Hmax+1) */
	andl	$0x3FF, %eax
	movl	$ABS(C), %edx
	MOVL	PRE(cs)(%edx, %edi, 4), %ebx	# C[i+4]
	andl	$0xFFFF, %ebx
	cmpl	%eax, %ebx
	je	15f
	cmpl	PRE(cs)ABS(Cmax) rBP, %ebx
	//##jne	6f	/* geometry_probe_failed */
	jne	5f	/* geometry_probe_failed */
15:
	movl	$63, %eax
	mull	%ecx

	/* EDI=i+4 at this moment */

	pushl	%eax			# EAX=(Hmax+1)*63
	subl	$4, %edi		# EDI=i
	
	movl	$ABS(C), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %eax	# C[i]
	andl	$0xFFFF, %eax
	
	movl	PRE(cs)ABS(Hmax) rBP, %esi		# Hmax
	addl	$1, %esi		#; incl	%esi

	pushl	%eax			# EAX=C[i]

	movl	$ABS(H), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %ecx	# H[i]

	mull	%esi			# EDX=0, EAX=product
	addl	%ecx, %eax
	movl	$63, %edx
	mull	%edx			# EDX=0, EAX=product
	
	movl	$ABS(L), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 8), %esi	# L[i]

	cmpl	%esi, %eax
	popl	%eax			# EAX=C[i]

	jae	4f
	
	/* calculate CHS numbers from start LBA */

	//pushl	%eax			# EAX=C[i]

	xorl	%edx, %edx
	xchgl	%eax, %esi		# ESI=C[i], EDX:EAX=L[i]
	
	divl	PRE(cs)ABS(Y) rBP		# Y=63
					# quo=EAX, rem=EDX
	addl	$1, %edx		#; incl	%edx	# sector number
	cmpl	%edx, PRE(cs)ABS(X) rBP
	je	15f
	cmpl	$63, PRE(cs)ABS(X) rBP
	jne	4f
15:
	xorl	%edx, %edx
	movl	PRE(cs)ABS(Hmax) rBP, %ebx
	addl	$1, %ebx		#; incl	%ebx	# EBX=Hmax+1
	divl	%ebx			# quo=EAX=cylinder number
					# rem=EDX=head number
	cmpl	%edx, %ecx		# ECX=H[i]
	je	15f
	subl	$1, %ebx		#; decl %ebx
					# EBX=Hmax
	cmpl	%ebx, %ecx
	jne	4f
15:
	/* EAX=calculated cylinder number, EBP=C[i] */

	cmpl	PRE(cs)ABS(Cmax) rBP, %esi
	je	4f
	andl	$0x3ff, %eax		# low 10 bit of calculated cylinder
	cmpl	%eax, %esi
4:
	popl	%eax
	//##jne	6f	/* geometry_probe_failed */
	//jmp	5f
5:
	je	5f
	movl	$-1, PRE(cs)ABS(mbr_free_sectors) rBP	# geometry fail
	jmp	3f
5:
	/* EAX = (Hmax+1)*63 */

	movl	$ABS(C), %ebx
	MOVL	$0, PRE(cs)(%ebx, %edi, 4)	# C[i]=0

	movl	$ABS(H), %ebx
	MOVL	$1, PRE(cs)(%ebx, %edi, 4)	# H[i]=1

	movl	$ABS(L), %ebx
	MOVL	$63, PRE(cs)(%ebx, %edi, 8)	# L[i]=63

	addl	$4, %edi		# EDI=i+4
	
	movl	$ABS(C), %ebx
	MOVL	$1, PRE(cs)(%ebx, %edi, 4)	# C[i+4]=1

	movl	$ABS(H), %ebx
	MOVL	$0, PRE(cs)(%ebx, %edi, 4)	# H[i+4]=0

	movl	$ABS(L), %ebx
	MOVL	%eax, PRE(cs)(%ebx, %edi, 8)	# L[i+4]=(Hmax+1)*63
3:
	#####################################################################
	popl	%esi			# restore ESI
	#####################################################################

	jmp	3f

2:	/* else */

	/* empty entry, zero all the coefficients */

	movl	$ABS(C), %ebx
	MOVL	$0, PRE(cs)(%ebx, %edi, 4)	# C[i]=0

	movl	$ABS(H), %ebx
	MOVL	$0, PRE(cs)(%ebx, %edi, 4)	# H[i]=0

	movl	$ABS(L), %ebx
	MOVL	$0, PRE(cs)(%ebx, %edi, 8)	# L[i]=0

	addl	$4, %edi
	movl	$ABS(C), %ebx
	MOVL	$0, PRE(cs)(%ebx, %edi, 4)	# C[i+4]=0

	movl	$ABS(H), %ebx
	MOVL	$0, PRE(cs)(%ebx, %edi, 4)	# H[i+4]=0

	movl	$ABS(L), %ebx
	MOVL	$0, PRE(cs)(%ebx, %edi, 8)	# L[i+4]=0

3:

	incl	PRE(cs)ABS(i) rBP
	jmp	1b
1:

	/* all coefficients are generated ok */

# --------------------------------------------------------------------

#if 0

	/* print the coefficients */
	
	xorl	%edi, %edi
1:
	cmpl	$4, %edi
	jae	1f

	/* print C[i] */
	movl	$ABS(C), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %eax		# C[i]

	call	6b	//print decimal number
	
	/* print spaces */
	movl	$ABS(delimit_space), %ecx
	call	8b	/* linux_print */
	
	/* print H[i] */
	movl	$ABS(H), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %eax		# H[i]

	call	6b	//print decimal number
	
	/* print spaces */
	movl	$ABS(delimit_space), %ecx
	call	8b	/* linux_print */
	
	/* print L[i] */
	movl	$ABS(L), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 8), %eax		# L[i]

	call	6b	//print decimal number
	
	/* print spaces */
	movl	$ABS(delimit_space), %ecx
	call	8b	/* linux_print */
	
	addl	$4, %edi

	/* print C[i] */
	movl	$ABS(C), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %eax		# C[i+4]

	call	6b	//print decimal number
	
	/* print spaces */
	movl	$ABS(delimit_space), %ecx
	call	8b	/* linux_print */
	
	/* print H[i] */
	movl	$ABS(H), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %eax		# H[i+4]

	call	6b	//print decimal number
	
	/* print spaces */
	movl	$ABS(delimit_space), %ecx
	call	8b	/* linux_print */
	
	/* print L[i] */
	movl	$ABS(L), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 8), %eax		# L[i+4]

	call	6b	//print decimal number
	
	/* print spaces */
	movl	$ABS(delimit_newline), %ecx
	call	8b	/* linux_print */
	
	subl	$4, %edi
	
	addl	$1, %edi		#; incl	%edi
	jmp	1b
1:

#endif
	
# --------------------------------------------------------------------

	/* mbr_free_sectors = 0 for geometry ok, -1 for failure */
	cmpl	$0, PRE(cs)ABS(mbr_free_sectors) rBP
	movl	$1, PRE(cs)ABS(mbr_free_sectors) rBP	# partition table ok
	jne	6f			/* -1 for failed geometry probing */

	//movl	$1, PRE(cs)ABS(mbr_free_sectors)	# partition table ok

	xorl	%edi, %edi			# equation number PRE(cs)ABS(i)
1:
	cmpl	$8, %edi
	jae	1f

	movl	$ABS(C), %ebx
	CMPL	$0, PRE(cs)(%ebx, %edi, 4)		# C[i]

	jne	1f

	addl	$1, %edi		#; incl	%edi
	jmp	1b
1:

	cmpl	$8, %edi
	jb	1f

	/* all C[i] == 0 */

	xorl	%edi, %edi			# equation number PRE(cs)ABS(i)
	movl	$ABS(H), %ebx
2:
	cmpl	$8, %edi
	jae	2f

	MOVL	PRE(cs)(%ebx, %edi, 4), %ecx		# H[i]
	testl	%ecx, %ecx

	jne	2f

	addl	$1, %edi		#; incl	%edi
	jmp	2b
2:

	cmpl	$8, %edi
	je	6f	/* geometry_probe_failed */	# all H[i] == 0

	movl	$1, PRE(cs)ABS(mbr_free_sectors) rBP	# partition table ok

	xorl	%edx, %edx			# equation number j
	movl	$ABS(L), %ebx
2:
	cmpl	%edi, %edx
	jae	2f

	CMPL	$0, PRE(cs)(%ebx, %edx, 8)		# L[j]

	jne	6f	/* geometry_probe_failed */

	addl	$1, %edx		#; incl	%edx
	jmp	2b
2:

	/* j == i */

	MOVL	PRE(cs)(%ebx, %edi, 8), %eax		# L[i]
	testl	%eax, %eax

	jle	6f	/* geometry_probe_failed */
						# OF=0; jmp if SF=1 or ZF=1

	cdq					# EDX=0
	divl	%ecx				# ECX=H[i]
						# EAX=quo=sectors_per_track
						# EDX=rem
	testl	%edx, %edx
	jnz	6f	/* geometry_probe_failed */

	movl	%eax, PRE(cs)ABS(probed_sectors_per_track) rBP

	cmpl	$63, %eax
	ja	6f	/* geometry_probe_failed */

	cmpl	PRE(cs)ABS(Smax) rBP, %eax
	jb	6f	/* geometry_probe_failed */

	movl	%eax, PRE(cs)ABS(Smax) rBP

	movl	%edi, %edx
	addl	$1, %edx		#; incl	%edx
					# equation number j=i+1
2:
	cmpl	$8, %edx
	jae	2f

	movl	$ABS(H), %ebx
	MOVL	PRE(cs)(%ebx, %edx, 4), %ecx		# H[j]

	movl	$ABS(L), %ebx
	MOVL	PRE(cs)(%ebx, %edx, 8), %eax		# L[j]

	jecxz	3f

	pushl	%edx

	xchgl	%eax, %ecx			# EAX=H[j], ECX=L[j]
	mull	PRE(cs)ABS(probed_sectors_per_track) rBP	# EAX=product
						# EDX=0
	popl	%edx

	cmpl	%eax, %ecx

	jnz	6f	/* geometry_probe_failed */

	jmp	4f
3:
	testl	%eax, %eax
	jnz	6f	/* geometry_probe_failed */
4:

	addl	$1, %edx		#; incl	%edx
	jmp	2b
2:

	movl	PRE(cs)ABS(Hmax) rBP, %eax
	addl	$1, %eax		#; incl	%eax
	movl	%eax, PRE(cs)ABS(probed_heads) rBP

	jmp	2f
1:
	movl	$1, PRE(cs)ABS(mbr_free_sectors) rBP	# partition table ok

	testl	%edi, %edi
	jz	1f

	/* xchgw C[i], C[0] */

	movl	$ABS(C), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %eax		# C[i]
	movl	PRE(cs)(iBX), %ecx			# C[0]

	movl	%eax, PRE(cs)(iBX)
	MOVL	%ecx, PRE(cs)(%ebx, %edi, 4)

	/* xchgw H[i], H[0] */

	movl	$ABS(H), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %eax		# H[i]
	movl	PRE(cs)(iBX), %ecx			# H[0]

	movl	%eax, PRE(cs)(iBX)
	MOVL	%ecx, PRE(cs)(%ebx, %edi, 4)

	/* xchgw L[i], L[0] */

	movl	$ABS(L), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 8), %eax		# L[i]
	movl	PRE(cs)(iBX), %ecx			# L[0]

	movl	%eax, PRE(cs)(iBX)
	MOVL	%ecx, PRE(cs)(%ebx, %edi, 8)

1:

	movl	$0, PRE(cs)ABS(H8) rBP			# will store sectors per track

	/* for (i = 1; i < 8; i++) */

	movl	$1, %edi
1:
	cmpl	$8, %edi
	jae	1f

	movl	$ABS(L), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 8), %eax		# L[i]

	movl	PRE(cs)ABS(C) rBP, %edx			# C[0]
	andl	$0xFFFF, %edx
	mull	%edx				# EDX:EAX=product

	MOVL	%eax, PRE(cs)(%ebx, %edi, 8)		# L[i] low
	MOVL	%edx, PRE(cs)4(%ebx, %edi, 8)		# L[i] high

	movl	$ABS(C), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %eax		# C[i]
	andl	$0xFFFF, %eax

	mull	PRE(cs)ABS(L) rBP			# L[0]

	movl	$ABS(L), %ebx
	SUBL	%eax, PRE(cs)(%ebx, %edi, 8)		# L[i] low
	SBBL	%edx, PRE(cs)4(%ebx, %edi, 8)		# L[i] high

	movl	$ABS(H), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %eax		# H[i]

	movl	PRE(cs)ABS(C) rBP, %edx			# C[0]
	andl	$0xFFFF, %edx
	mull	%edx				# EDX:EAX=product
						# EDX=0

	MOVL	%eax, PRE(cs)(%ebx, %edi, 4)		# H[i]

	movl	$ABS(C), %ebx
	MOVL	PRE(cs)(%ebx, %edi, 4), %eax		# C[i]
	andl	$0xFFFF, %eax

	mull	PRE(cs)ABS(H) rBP			# H[0]

	movl	$ABS(H), %ebx
	SUBL	%eax, PRE(cs)(%ebx, %edi, 4)		# H[i]

	/* if (H[i]) */

	jz	3f

	ja	4f

	movl	$ABS(H), %ebx
	NEGL	PRE(cs)(%ebx, %edi, 4)			# H[i]

	movl	$ABS(L), %ebx
	xorl	%eax, %eax
	NEGL	PRE(cs)(%ebx, %edi, 8)			# L[i] low
	cmc
	NOTL	PRE(cs)4(%ebx, %edi, 8)		# L[i] high
	ADCL	%eax, PRE(cs)4(%ebx, %edi, 8)

4:
	movl	$ABS(L), %ebx
	CMPL	$0, PRE(cs)4(%ebx, %edi, 8)		# L[i] high
	jne	6f	/* geometry_probe_failed */

	MOVL	PRE(cs)(%ebx, %edi, 8), %eax		# L[i] low
	testl	%eax, %eax

	jle	6f	/* geometry_probe_failed */
						# OF=0; jmp if SF=1 or ZF=1

	cdq					# EDX=0

	movl	$ABS(H), %ebx
	DIVL	PRE(cs)(%ebx, %edi, 4)			# H[i]
						# EAX=sectors per track
	testl	%edx, %edx
	jnz	6f	/* geometry_probe_failed */

	movl	%eax, PRE(cs)ABS(L8) rBP

	cmpl	$63, %eax
	ja	6f	/* geometry_probe_failed */

	cmpl	PRE(cs)ABS(Smax) rBP, %eax
	jb	6f	/* geometry_probe_failed */

	movl	%eax, PRE(cs)ABS(Smax) rBP

	cmpl	$0, PRE(cs)ABS(H8) rBP
	je	4f

	cmpl	%eax, PRE(cs)ABS(H8) rBP		# EAX=L[8]
	jne	6f	/* geometry_probe_failed */

	jmp	5f
4:
	movl	%eax, PRE(cs)ABS(H8) rBP		# EAX=L[8]
5:

	jmp	4f
3:	/* else */
	movl	$ABS(L), %ebx
	CMPL	$0, PRE(cs)(%ebx, %edi, 8)		# L[i] low
	jne	6f	/* geometry_probe_failed */

	CMPL	$0, PRE(cs)4(%ebx, %edi, 8)		# L[i] high
	jne	6f	/* geometry_probe_failed */

4:	/* end if (H[i]) */

	addl	$1, %edi		#; incl	%edi
	jmp	1b
1:	/* end for (i = 1; i < 8; i++) */

	/* if (PRE(cs)ABS(H)[8]) */

	cmpl	$0, PRE(cs)ABS(H8) rBP
	je	3f

	/* PRE(cs)ABS(H)[8] is sectors per track */

	movl	PRE(cs)ABS(H) rBP, %eax			# H[0]
	mull	PRE(cs)ABS(H8) rBP

	//movl	$ABS(L), %ebx
	subl	%eax, PRE(cs)ABS(L) rBP			# L[0] low
	sbbl	%edx, PRE(cs)ABS(L) + 4 rBP		# L[0] high

	jnz	6f	/* geometry_probe_failed */

	movl	PRE(cs)ABS(L) rBP, %eax
	testl	%eax, %eax

	jle	6f	/* geometry_probe_failed */
						# OF=0; jmp if SF=1 or ZF=1

	cdq					# EDX=0

	divl	PRE(cs)ABS(H8) rBP

	testl	%edx, %edx
	jnz	6f	/* geometry_probe_failed */

	movl	PRE(cs)ABS(C) rBP, %ecx		# C[0]
	andl	$0xFFFF, %ecx
	divl	%ecx

	testl	%edx, %edx
	jnz	6f	/* geometry_probe_failed */

	movl	%eax, PRE(cs)ABS(L8) rBP	# L[8] is number of heads

	cmpl	$256, %eax
	ja	6f	/* geometry_probe_failed */

	cmpl	PRE(cs)ABS(Hmax) rBP, %eax
	jbe	6f	/* geometry_probe_failed */

	movl	PRE(cs)ABS(H8) rBP, %eax
	movl	%eax, PRE(cs)ABS(probed_sectors_per_track) rBP

	jmp	4f
3:	/* else */

	/* fail to set L[8], this means all H[i]==0, i=1,2,3,4,5,6,7 */

	/* Now the only equation is: C[0] * H * S + H[0] * S = L[0] */

	/* for (i = 63; i >= Smax; i--) */

	movl	PRE(cs)ABS(L) rBP, %ebx		# EBX=L[0]
	movl	$63, %edi
3:
	cmpl	PRE(cs)ABS(Smax) rBP, %edi
	jb	3f

	movl	PRE(cs)ABS(H) rBP, %eax		# H[0]
	mull	%edi				# EDX=0
						# EAX=product
	//movl	%ebx, PRE(cs)ABS(L8)
	//subl	%eax, PRE(cs)ABS(L8)

	subl	%ebx, %eax
	negl	%eax

	jz	5f				# continue
	js	5f				# continue

	cdq					# EDX=0
	divl	%edi

	testl	%edx, %edx
	jnz	5f

	movl	PRE(cs)ABS(C) rBP, %ecx		# C[0]
	andl	$0xFFFF, %ecx
	divl	%ecx

	testl	%edx, %edx
	jnz	5f

	movl	%eax, PRE(cs)ABS(L8) rBP	# L[8] is number of heads

	cmpl	$256, %eax
	ja	5f

	cmpl	PRE(cs)ABS(Hmax) rBP, %eax
	ja	3f
5:
	subl	$1, %edi		#; decl %edi
	jmp	3b
3:	/* end for (i = 63; i >= Smax; i--) */

	cmpl	PRE(cs)ABS(Smax) rBP, %edi
	jb	6f	/* geometry_probe_failed */

	movl	%edi, PRE(cs)ABS(probed_sectors_per_track) rBP

4:	/* end if (PRE(cs)ABS(H)[8]) */

	movl	PRE(cs)ABS(L8) rBP, %eax
	movl	%eax, PRE(cs)ABS(probed_heads) rBP

2:	/* end if (PRE(cs)ABS(i) == 8) */

	/* geometry_probe_ok */

	clc
	jmp	2f

//geometry_probe_failed:
6:
	stc
	movl	$0, PRE(cs)ABS(probed_sectors_per_track) rBP
	movl	$0, PRE(cs)ABS(probed_heads) rBP
2:
	/* try to get mbr_free_sectors */
	cmpl	$1, PRE(cs)ABS(mbr_free_sectors) rBP	# partition table ok
	jl	2f					# failure

	/* find a minimum StartLBA in the partition table. */

	movl	$100, %eax		# greater than 63+32
	movl	-58(iSI), %ecx		# start_lba
	jecxz	1f
	cmpl	%ecx, %eax
	jbe	1f
	movl	%ecx, %eax
1:
	movl	-42(iSI), %ecx		# start_lba
	jecxz	1f
	cmpl	%ecx, %eax
	jbe	1f
	movl	%ecx, %eax
1:
	movl	-26(iSI), %ecx		# start_lba
	jecxz	1f
	cmpl	%ecx, %eax
	jbe	1f
	movl	%ecx, %eax
1:
	movl	-10(iSI), %ecx		# start_lba
	jecxz	1f
	cmpl	%ecx, %eax
	jbe	1f
	movl	%ecx, %eax
1:
	movl	%eax, PRE(cs)ABS(mbr_free_sectors) rBP
88:
	clc			/* success */
	#; popal
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
	popl	%edx
	popl	%ecx
	popl	%eax
	ret
2:
	movl	$0, PRE(cs)ABS(mbr_free_sectors) rBP	# partition table fail
	stc
	#; popal
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
	popl	%edx
	popl	%ecx
	popl	%eax
	ret


#ifdef __DOS_16
//partition:
0:
	/* load the next partition and modify the boot record */

	/* EBX holds the file descriptor number or 0 if it is BIOS drive */
	/* partition table is at MBR(0x2000) */
	/* current partition number is PRE(cs)ABS(current_partition) */

	pushal

	/* enumerate all partitions */
	
	/* initialize the partition table, move start_LBA(the 3rd dword)
	 * to the total_sectors field(the 4th dword) and clear the 3rd dword.
	 * The sum of the 3rd and 4th dwords gives the absolute start_LBA.
	 */
	movl	$MBR(0x2000), %ebp
	xorl	%eax, %eax
	movl	%eax, 0x00(%ebp)	//Clear the parent partition
	movl	%eax, 0x10(%ebp)
	movl	%eax, 0x20(%ebp)
	movl	%eax, 0x30(%ebp)
	
	movl	$0x04, PRE(cs)ABS(total_partition_entries)
	/* copy extended partition entries to the end of the partition table */
	xorl	%esi, %esi
1:
	movb	4(%ebp, %esi), %al	# partition id
	cmpb	$0x05, %al
	je	2f
	cmpb	$0x0f, %al
	je	2f
	cmpb	$0x15, %al
	je	2f
	cmpb	$0x1f, %al
	je	2f
	cmpb	$0x85, %al
	jne	4f
2:
	/* extended partition */
	movb	$0, 0x04(%ebp, %esi)	//Basic extended partition
	movl	0x08(%ebp, %esi), %eax
2:	
	/* read one sector of the partition to MBR(0) */
	pushal
	pushl	$0
	pushl	%eax
	pushw	%es
	pushw	$0
	pushw $0001
	pushw $0x0010
	movb	$0x42, %ah		
	movw	%sp, %si
	movb	PRE(cs)ABS(bios_drive_number), %dl //bios id
	int $0x13
	popaw
	popal
	/* check if the partition table is valid */
	movb	0x1c2, %bl	//Partition is marked?
	cmpb	$0x05, %bl  //Extended partition? 
	je	3f						//yes
	cmpb	$0x0f, %bl  //Extended partition? 
	je	3f						//yes
	cmpb	$0x15, %bl  //Extended partition?
	je	3f						//yes
	cmpb	$0x1f, %bl  //Extended partition?
	je	3f						//yes
	cmpb	$0x85, %bl  //Extended partition?
	jne	5f						//not
3:
	addl	0x1c6, %eax	//Partition start LBA address
	jmp	2b
5:	
	/* approve the partition entries and move onto the free entries */
	movl	PRE(cs)ABS(total_partition_entries), %edi
	shll	$4, %edi
	addl	%ebp, %edi		# EDI points to the first free entry
	/* move the extended partition entry onto the first free entry */
	movl	%eax, 0x00(%edi)	//parent partition
	movl	0x1c2, %eax
	movl	%eax, 0x04(%edi)
	movl	0x1c6, %eax
	movl	%eax, 0x08(%edi)	//The partition
	movl	0x1ca, %eax
	movl	%eax, 0x0c(%edi)	//The total number of sectors
	//Determine whether there is an extended partition
	cmpl	$0, 0x1d6
	je	2f	//no
	incl	PRE(cs)ABS(total_partition_entries)
	movl	0x08(%ebp, %esi), %eax	//parent partition
	addl	0x1d6, %eax	//LBA partition start address
	jmp	2b

4:
	addl	$0x10, %esi
	movl	PRE(cs)ABS(total_partition_entries), %edi
	shll	$4, %edi
	cmpl	%edi, %esi
	jb	1b
2:
	/* end of partition enumeration */
	popal
	ret


66:
	pushal
	movl	$ABS(msg_disk_Information), %ecx
	call	8b	/* linux_print */
  //Integer calculation                 
	xorl	%edx, %edx
	movl	$2, %ebx
	divl	%ebx	//Disk Bytes Kb
	movl	$1000, %ebx	//0x3e8
	xorl	%edx, %edx
	divl	%ebx	//Disk Bytes Mb
	cmpl	%ebx, %eax	//< 1Gb?
	jb	22f	//yes
	xorl 	%edx, %edx
	divl	%ebx	//Disk Bytes Gb
	cmpl	%ebx, %eax	//< 1Tb?
	jb	3f	//yes	
	xorl	%edx, %edx
	divl	%ebx	//Disk Bytes Tb
  //Setting unit'Tb'
	movb	$0x54, %cl	//'T'
	movb	%cl, PRE(cs)ABS(scM)
	jmp	22f
	//Setting unit'Gb' 
3: 
	movb	$0x47, %cl	//'G'
	movb	%cl, PRE(cs)ABS(scM)     
	//Handle decimal
22:   
	movl	$ABS(scS_c), %esi	//Store the last bit of pre-decimal address
	movw	$10, %bx	//0xa
	pushw	%ax	//Save integer
	movw	%dx, %ax	//Decimal
	movw	$2, %cx
23:   
	xorw	%dx, %dx
	divw	%bx
	orb		$0x30, %dl
	movb	%dl, PRE(cs)(iSI)
	decw	%si
	cmpw	$0, %ax
	jne 23b
	//Moving two decimal
	cmpb	$1, %cl
	jbe 3f
	movl	$ABS(scS_x), %edi
	movl	$ABS(scS_c_s), %esi
	movw	PRE(cs)(iSI), %ax
	movw	%ax, 	PRE(cs)(iDI)
	//Handle integer                               
3: 
	decb	%cl
	je 3f
	popw	%ax	//Integer
	movl 	$ABS(scS_z_w), %esi
	jmp 23b
3:
	movl	$ABS(scS_z_s), %ecx
	call	8b	/* linux_print */
	//Default units 'Mb'
	movb	$0x4d, %cl	//'M'
	movb	%cl, PRE(cs)ABS(scM)
	//The first 8 bytes from the integer filling spaces
	movl	$ABS(scS_z_s), %edi        
	movl	$0x20202020, %eax
	movl	%eax, PRE(cs)(iDI)
	movb	$0x30, PRE(cs)ABS(scS_c)
	movw	$0x3030, PRE(cs)ABS(scS_c_s)
	popal
	ret
	
#endif


//Clear_buffer
100:
	xorl	%eax, %eax
	movl	$MBR(0), %edi
	movl	$(0x60*0x200/4), %ecx	
	repz	stosl
	ret

//----------------------------------------------------------------------------------------
//input: ecx

400:
	pushl	%ebx		// file descriptor, or 0 for BIOS drive

#ifdef __DOS_16

	xorl	%eax, %eax
	cmpl	$0xff, PRE(cs)ABS(bios_drive_number)
	jne	1f

	movl	$0x4200, %eax
	movw	%cx, %dx		//dx=Æ«ÒÆÁ¿µÄµÍÎ»
	shrl	$16, %ecx	
	int	$0x21
	jnc	1f
	negl	%eax
1:
#else
	/* off_t lseek(int fildes, off_t offset, int whence) */

	/* check if in 32-bit or 64-bit mode */
	xorl	%eax, %eax
	decl	%eax		// rex prefix of 0x48
	testl	%eax, %eax
	/* EAX = 0 for 64-bit mode, -1 for 32-bit mode */
	jz	1f
	
	/* in 32-bit mode */
	movl	$19, %eax	// sys_lseek
	movl	$0, %edx 	// SEEK_SET = 0
	int	$0x80
	jmp	2f
1:
	/* in 64-bit mode */
	movl	$8, %eax	// sys_lseek
	pushl	%ebx
	popl	%edi
	movl	%ecx, %esi
	movl	$0, %edx 	// SEEK_SET = 0
	syscall
2:
#endif

	popl	%ebx		// file descriptor, or 0 for BIOS drive
	ret

//----------------------------------------------------------------------------------------
//input: ecx, es:0000, sectors_to_write

300:
	pushl	%ebx		# file descriptor, or 0 if specified BIOS drive 

#ifdef __DOS_16

	xorl	%eax, %eax
	cmpl	$0xff, PRE(cs)ABS(bios_drive_number)
	je	1f		/* not bios */

	/* BIOS drive */	
	cmpl	$0, PRE(cs)ABS(ebios_support)
	je	10f		/* EBIOS not present */
	movb	PRE(cs)ABS(bios_drive_number), %dl
	/* EBIOS supported */
	pushl	$0		// hi 32bit of startLBA
	pushl %ecx
	pushw	%es		// buffer segment
	pushw	$0		// buffer offset
	pushw	PRE(cs)ABS(sectors_to_write)
	pushw	$0x0010		// packet length
	movw	%sp, %si
	movw	$0x4300, %ax	/* LBA write */
	int	$0x13
	movw	%ss, %ax
	movw	%ax, %ds
	movw	%ax, %es
	popaw
	jc 77f
	xorl	%eax, %eax
	clc
	jmp 2f
77:
	movl	$ABS(msg_ebios_write_failure), %ecx
	call	8f	/* linux_print */

10:
	cmpb	$0xff, PRE(cs)ABS(gpt)
	jnz	10f
	stc
	jmp	2f
10:
	/* try CHS */

	movb	$0x03, %ah	// CHS write to disk
	movb	PRE(cs)ABS(sectors_to_write), %al

	cmpb	$32, %al
	stc
	ja	2f		/* should not write two many sectors with CHS */

	movw	$0, %bx
	movw	$1, %cx
	movw	PRE(cs)ABS(bios_drive_number), %dx
	int	$0x13
	movzwl	%ax, %eax
	movw	%ss, %cx			//0000
	movw	%cx, %ds
	movw	%cx, %es
	jmp	2f
	
1:
	//WRITE TO FILE OR DEVICE

	//AH = 40h
	//BX = file handle
	//CX = number of bytes to write
	//DS:DX -> data to write

	//Return:
	//CF clear if successful AX = number of bytes actually written
	//CF set on error AX = error code (05h,06h) 
	movl	$0x4000, %eax
	movl	PRE(cs)ABS(sectors_to_write), %ecx
	shll	$9, %ecx 
	movl	$0, %edx
	int	$0x21
2:
	jnc	1f
	negl	%eax
1:
#else
	/* ssize_t write(int fd, const void *buf, size_t count) */

	/* check if in 32-bit or 64-bit mode */
	xorl	%eax, %eax
	decl	%eax		// rex prefix of 0x48
	testl	%eax, %eax
	/* EAX = 0 for 64-bit mode, -1 for 32-bit mode */
	jz	2f
	
	/* in 32-bit mode */
	movl	$4, %eax	# sys_write
	movl	$MBR(0), %ecx
	movl	PRE(cs)ABS(sectors_to_write) rBP, %edx
	shll	$9, %edx
	int	$0x80       //bx=ÎÄ¼þÃèÊö·û
	jmp	1f
2:
	/* in 64-bit mode */
	movl	$1, %eax	# sys_write
	pushl	%ebx
	popl	%edi
	movl	$MBR(0), %esi
	movl	PRE(cs)ABS(sectors_to_write) rBP, %edx
	shll	$9, %edx
	syscall
1:
#endif

	popl	%ebx		# file descriptor, or 0 if specified BIOS drive
	ret
	
//----------------------------------------------------------------------------------------