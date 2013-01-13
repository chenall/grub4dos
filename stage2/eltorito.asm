
;-----------------------------------------------------------------------------
; ElTorito.asm
;
; El Torito Bootable CD-ROM driver which does not reset the CD-ROM drive upon
; loading, but instead accesses the drive through BIOS system calls
;
; MIT License
;
; (c) 2000 by Gary Tong
; (c) 2001-2009 by Bart Lagerweij
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
;
;-----------------------------------------------------------------------------

; Imported from syslinux-5.00 with ScanDrives fixed by Tinybit, 2013-01-12

; Ported to NASM by H. Peter Anvin.

; To assemble and link, use these commands with NASM 2.x:
;   nasm -Ox -f bin -o eltorito.sys eltorito.asm

; To enable Trace markers uncomment the line below
; DEBUG_TRACERS=1

; To enable debug info uncomment the line below
; DEBUG=1

;-----------------------------------------------------------------------------
; Change log for eltorito.asm, by Bart Lagerweij
;
; Jun 25, 2009 - License source code under the MIT license
;
; Jun 6, 2002 - v1.2
; Eltorito.sys does now also finds the correct driver number for the booted
; CD-Rom on a Dell PC with very buggy BIOS. It does not clear the carry flag
; after a succesfull call to int13/ax=4b01h call. Other PC's also using Phoenix
; BIOS version 1.10 A14, or alike maybe also benefit from this "workaround".
;
; Mar 9, 2002
; - All read requests are now retried 5 times.
; - Bug fix, had...
;	cmp	ax, 3FFFh		;Too large?
;	ja	ReadLBad		;If yes
;   seperated from...
;	mov	ax,es:[bx+18]		;Get number of sectors to read
;	mov	word ptr [SpecPkt+2],ax
;   so, it was checking "wild" ax values...
; - Some cleanup and small changes
; - The tracers give trouble when using SHCD..
; - Reverted proc ReadL back to Rev. 0.15BETA
;
; Mar 5, 2002
; - Bug fix, when changing CD media some machines would "hang" in the
;   PriVolDesc routine.
; - Added printing of TRACER characters to trace the bug above.
; - Major cleanup and now using ASCIIZ strings.
;
; May 9, 2001
; - Fixed a "pad devicename with spaces" bug, this only happened when a device
;   name was used with less than 8 characters, for example, "MSCD000" became
;   "MSCD000(".
; - Bug fix, when eltorito.sys was called with invalid command line parameters,
;   garbage was printed and sometimes followed by "system halted" that has been
;   there since the very first version of eltorito.sys. I know that because I
;   had the bug back then. When loading eltorito.sys using a device loader,
;   for example "device.com eltorito.sys /test:123" garbage was printed instead
;   of "No device name found." "driver not installed".
; - Changed the error message to include a "usage" string.
;
; May 8, 2001
; - If diskemu.bin is loaded, eltorito.sys uses the drivenumber from diskemu.
;   A call is made to "diskemu/Get status" (INT13/AX=5400) and the drivenumber
;   is returned in CL. This should fix boot problems on Dell PCs (YES!).
;   When diskemu.bin is not loaded, eltorito still loops all drive numbers
;   using eltorito calls.
; - Removed "press Escape..."
; - When the Alt-key is pressed (and holded) more info is printed and
;   eltorito.sys halts.
;-----------------------------------------------------------------------------

%ifdef DEBUG_TRACERS
 %macro	TRACER	1
	call debug_tracer
	db %1
 %endmacro
%else
 %macro	TRACER	1
 %endmacro
%endif	; DEBUG_TRACERS

%define	Ver	'1.5'
%define CR	0DH, 0Ah
RPolyH		equ	0EDB8h
RPolyL		equ	08320h

		section .text align=16
		org	0

;=============================================================================

Cdrom:

NextDriver	dd	-1			;-+
Attributes	dw	0C800h			; |
Pointers	dw	Strategy		; |
		dw	Commands		; |   MSCDEX requires this
DeviceName	db	'ELTORITO'		; |  data in these locations
		dw	0			; |
DriveLetter	db	0			; |
NumUnitsSupp	db	1			;-+

DriverName	db	'El-Torito CD-ROM Device Driver',0
		align 4, db 0
ReqHdrLoc	dd	0
XferAddr	dd	0
Checksum	dd	-1
DriveNumber	db	0
ReadBytes	db	0			;0 --> 2048 bytes/sector
						;1 --> 1024 bytes/sector
						;2 -->  512 bytes/sector

Routines	dw	Init		;Init		;0
		dw	Unsupported	;MediaCheck	;1
		dw	Unsupported	;BuildBPB	;2
		dw	IoctlInput	;IoctlInput	;3
		dw	Unsupported	;Input		;4
		dw	Unsupported	;NonDesInput	;5
		dw	Unsupported	;InputStatus	;6
		dw	Unsupported	;InputFlush	;7
		dw	Unsupported	;Output		;8
		dw	Unsupported	;OutputVerify	;9
		dw	Unsupported	;OutputStatus	;10
		dw	Unsupported	;OutputFlush	;11
		dw	IoctlOutput	;IoctlOutput	;12
		dw	DoNothing	;DeviceOpen	;13
		dw	DoNothing	;DeviceClose	;14
		dw	ReadL		;ReadL		;128

IoctlICtrl	dw	Raddr		;Raddr		;0
		dw	Unsupported	;LocHead	;1
		dw	Unsupported	;(Reserved)	;2
		dw	Unsupported	;ErrStat	;3
		dw	Unsupported	;AudInfo	;4
		dw	DrvBytes		;DrvBytes	;5
		dw	DevStat		;DevStat	;6
		dw	SectSize		;SectSize	;7
		dw	VolSize		;VolSize	;8
		dw	MedChng		;MedChng	;9

SpecPkt		times	19	db	0	; offset 77h in 1.4
		times	13	db	0	; unknown extra 00s in 1.4

;Greeting	db	'El-Torito Bootable CD-ROM Driver for Dos v',Ver,', http://www.nu2.nu/eltorito/',CR
Greeting	db	'El-Torito Bootable CD-ROM Driver for Dos, built along with GRUB4DOS',CR
		db	'  (c) 2000 by Gary Tong',CR
		db	'  (c) 2001-2002 by Bart Lagerweij',CR,0
DblSpace	db	'  ',0

;=============================================================================

Strategy:

		mov	word [cs:ReqHdrLoc],bx
		mov	word [cs:ReqHdrLoc+2],es
		retf


;=============================================================================

Commands:

		push	ax
		push	bx
		push	cx
		push	dx
		push	si
		push	di
		push	bp
;		pushad
		push	ds
		push	es
		TRACER 'C'

		cld				;Clear direction
		sti				;Enable interrupts

		mov	ax, cs			;ds=cs
		mov	ds, ax

		les	bx,[ReqHdrLoc]	;seg:offset ptr into es:bx
		xor	ax,ax
		mov	al,[es:bx+2]		;Get Command code
%ifdef DEBUG
		call	print_hex8
%endif
		cmp	al,15
		jb	Mult2			;If 0-14
		cmp	al,128
		jb 	UnknownCmd		;If 15-127
		cmp	al,129
		jb	ShiftDown		;If 128
UnknownCmd:	mov	al,121			;8 = Unsupported (Reserved)
ShiftDown:	sub	al,113			;128 --> 15, 121 --> 8
Mult2:		shl	al,1			;Convert into offset (*2)
		mov	di,Routines
		add	di,ax
		call 	word [di]		;Execute desired command
		or	ax,100h			;Set Return Status's Done bit
		lds	bx,[ReqHdrLoc]		;seg:offset ptr into ds:bx
		mov	[bx+3],ax		;Save Status

%ifdef DEBUG
		cmp	byte [cs:buffer+2048], 96h
		je	buffer_ok
		mov	al, '!'
		call	print_char
		jmp	$
buffer_ok:
%endif

		TRACER 'c'
		pop	es
		pop	ds
;		popad
		pop	bp
		pop	di
		pop	si
		pop	dx
		pop	cx
		pop	bx
		pop	ax
		retf


;=============================================================================

Unsupported:			;Unsupported Command

		mov	ax,8003h		;Set Status Error bit,
		TRACER 'U'
		TRACER 'C'
		retn				;   Error 3 = Unknown Command


;=============================================================================

IoctlInput:			;IOCTL Input Routine

		mov	di,[es:bx+14]		;es:bx --> Request Header
		mov	es,[es:bx+16]		;Get Xfer Address into es:di
		xor	ax,ax			;Get Control Block Code
		mov	al,[es:di]
%ifdef DEBUG
	TRACER 'I'
	TRACER 'O'
	call	print_hex8
%endif
		cmp	al,10
		jb	UnkIoctlI		;If 0-9
		mov	al,2			;Map to Unsupported
UnkIoctlI:	shl	al,1			;Convert into offset (*2)
		mov	si,IoctlICtrl
		add	si,ax
		call 	word [si]		;Execute desired command
		retn


;=============================================================================

Raddr:			;Return Device Header Address

		TRACER 'A'
		mov	word [es:di+1],0
		mov	[es:di+3],cs
		xor	ax, ax			;Set Return Status = success
		TRACER 'a'
		retn


;=============================================================================

DrvBytes:			;Read Drive Bytes

		TRACER 'B'
		push	di			;Save original Xfer Addr
		add	di,2			;Point to 1st dest byte
		mov	si,Greeting	;Point to Greeting
DrvB:		movsb				;Copy over a byte
		cmp	byte [si],13	;Is next char a CR?
		jne	DrvB			;Loop if not

		sub	di,2			;Get #bytes copied into ax
		mov	ax,di
		pop	di			;Retrieve original Xfer Addr
		sub	ax,di
		mov	byte [es:di+1],al	;and save it
		mov	ax,0			;Set Return Status = success
		TRACER 'b'
		retn


;=============================================================================

DevStat:			;Return Device Status

		TRACER 'D'
		mov	word [es:di+1],202h	;Door closed
		mov	word [es:di+3],0	;Door unlocked
						;Supports only cooked reading
						;Read only
						;Data read only
						;No interleaving
						;No prefetching
						;No audio channel manipulation
						;Supports both HSG and Redbook
						;  addressing modes

		xor	ax, ax			;Set Return Status = success
		TRACER 'd'
		retn


;=============================================================================

SectSize:			;Return Sector Size

		TRACER 'S'
		mov	word [es:di+2],2048
		mov	ax,0			;Set Return Status = success
		TRACER 's'
		retn


;=============================================================================

VolSize:			;Return Volume Size

		TRACER 'V'
		call	PriVolDesc		;Get and Check Primary Volume
						;  Descriptor
		mov	ax,800Fh		;Assume Invalid Disk Change
		jc	VolExit			;If Read Failure

		mov	ax,word [Buffer+80]	;Read Successful
		mov	word [es:di+1],ax	;Copy over Volume Size
		mov	ax,word [Buffer+82]
		mov	word [es:di+3],ax
		mov	ax,0			;Set Return Status = success
VolExit:
		TRACER 'v'
		retn


;=============================================================================

MedChng:			;Return Media Changed Status

		TRACER 'M'
		call	PriVolDesc		;Get and Check Primary Volume
						;  Descriptor
		mov	byte [es:di+1],-1	;Assume Media Changed
		mov	ax,800Fh		;  and Invalid Disk Change
		jc	MedExit			;If Media Changed or Bad

		mov	byte [es:di+1],1	;Media has not changed
		mov	ax,0			;Set Return Status = success
MedExit:
		TRACER 'm'
		retn


;=============================================================================

PriVolDesc:			;Get and Check Primary Volume
						;  Descriptor
		TRACER 'P'
		mov	ax,cs			;Set ds:si --> SpecPkt
		mov	ds,ax

		mov	cx, 5
PriVolAgain:
		mov	byte [SpecPkt],16	;SpecPkt Size
		mov	byte [SpecPkt+1],0	;Reserved
		mov	word [SpecPkt+2],1	;Transfer one 2048-byte sector
		push	cx
		mov	cl,byte [ReadBytes]	;Multiply by 4 if reading 512
		shl	word [SpecPkt+2],cl	;  bytes at a time
		pop	cx
		mov	word [SpecPkt+6],cs	;Into our Buffer
		mov	word [SpecPkt+4], Buffer
		mov	word [SpecPkt+8],16	;From CD Sector 16
		mov	word [SpecPkt+10],0
		mov	word [SpecPkt+12],0
		mov	word [SpecPkt+14],0

		mov	si, SpecPkt
		mov	dl, [DriveNumber]
		mov	ah, 42h			;Extended Read
		int	13h
		jnc	PriVolPass		;If success

;		TRACER '1'
		; read error
		loop	PriVolAgain

		TRACER '2'
		; read retries exhausted
		; flow into below
		jmp	PriReadErr

PriVolPass:
		mov	si,Buffer	;Point input to Buffer
		mov	ax,-1			;Init Checksum registers
		mov	bx,ax			;  bx,ax = 0FFFFFFFFh
		jc	PriNew			;If Read Failure

		push	di			;Read Successful,
						;  so Calculate Checksum
		mov	di,1024			;Init Word counter
PriWord:	mov	dx,[cs:si]		;Grab next word from buffer
		mov	cx,16			;Init bit counter
PriBit:		shr	dx,1			;Shift everything right 1 bit
		rcr	bx,1
		rcr	ax,1
		jnc	NoMult			;If a zero shifted out

		xor	bx,RPolyH		;A one shifted out, so XOR
		xor	ax,RPolyL		;  Checksum with RPoly
NoMult:
		loop	PriBit

		add	si,2			;Inc Word Pointer
		dec	di
		ja	PriWord
		TRACER '3'

		pop	di			;Checksum calculation complete
		cmp	bx,[Checksum+2]		;Has Checksum changed?
		jne	PriNew			;If Checksum Changed

		cmp	ax,[Checksum]
		jne	PriNew			;If Checksum Changed

		clc				;Checksum not changed, CF=0
		mov	ax,0			;Status = success
		jmp	PriOld

PriReadErr:
		mov	WORD [Checksum+2],bx		;Save New Checksum
		mov	[Checksum],ax		;  or 0FFFFFFFFh if bad read
		stc				;Checksum change, CF=1
		mov	ax, 800bh		;Status = read fault
		jmp	PriOld

PriNew:		mov	WORD [Checksum+2],bx		;Save New Checksum
		mov	[Checksum],ax		;  or 0FFFFFFFFh if bad read
		stc				;Checksum Changed, CF=1
		mov	ax,800Fh		;Status = Invalid Media Change
PriOld:
		TRACER 'p'
		retn


;=============================================================================

IoctlOutput:			;IOCTL Output Routine

		TRACER 'O'
		mov	di,[es:bx+14]		;es:bx --> Request Header
		mov	es,[es:bx+16]		;Get Xfer Address into es:di
		xor	ax,ax			;Get Control Block Code
		mov	al,[es:di]
		cmp	al,2
		jne	UnkIoctlO		;If not 2 (ResetDrv)
		call	DoNothing		;Reset Drive
		jmp	IoctlODone
UnkIoctlO:
		call	Unsupported		;Unsupported command
IoctlODone:
		TRACER 'o'
		retn


;=============================================================================

DoNothing:			;Do Nothing Command

		mov	ax,0			;Set Return Status = success
		retn


;=============================================================================

ReadL:			;Read Long Command

		TRACER 'R'
		mov	ax,cs			;Set ds=cs
		mov	ds,ax
						;es:bx --> Request Header
		cmp	byte [es:bx+24],0	;Check Data Read Mode
		jne	ReadLErr		;If Cooked Mode

		cmp	byte [es:bx+13],2	;Check Addressing Mode
		jb	ReadLOK			;If HSG or Redbook Mode

ReadLErr:
		TRACER '8'
		mov	ax,8003h		;Set Return Status = Unknown
		jmp	ReadLExit		;  Command Error and exit

ReadLOK:
		mov	ax,[es:bx+20]		;Get Starting Sector Number,
		mov	dx,[es:bx+22]		;  Assume HSG Addressing Mode
		cmp	byte [es:bx+13],0	;Check Addressing Mode again
		je	ReadLHSG		;If HSG Addressing Mode

		TRACER '7'
		;Using Redbook Addressing Mode.  Convert to HSG format
		mov	al,dl			;Get Minutes
		mov	dl,60
		mul	dl			;ax = Minutes * 60
		add	al,byte [es:bx+21]	;Add in Seconds
		adc	ah,0
		mov	dx,75			;dx:ax =
		mul	dx			;  ((Min * 60) + Sec) * 75
		add	al,byte [es:bx+20]	;Add in Frames
		adc	ah,0
		adc	dx,0
		sub	ax,150			;Subtract 2-Second offset
		sbb	dx,0			;dx:ax = HSG Starting Sector

ReadLHSG:
		mov	word [SpecPkt+8], ax	;Store Starting
		mov	word [SpecPkt+10], dx	;  Sector Number
		mov	word [SpecPkt+12], 0	;  (HSG Format)
		mov	word [SpecPkt+14], 0

		mov	ax,[es:bx+14]		;Get Transfer Address
		mov	word [SpecPkt+4],ax
		mov	ax,[es:bx+16]
		mov	word [SpecPkt+6],ax

		mov	byte [SpecPkt],16	;Size of Disk Address Packet
		mov	byte [SpecPkt+1],0	;Reserved

		mov	cx, 5
ReadLAgain:
		mov	ax,[es:bx+18]		;Get number of sectors to read
		mov	word [SpecPkt+2],ax
		cmp	ax, 3FFFh		;Too large?
		ja	ReadLBad		;If yes

		push	cx
		mov	cl,byte [ReadBytes]	;Multiply by 4 if reading 512
		shl	word [SpecPkt+2],cl	;  bytes at a time
		pop	cx

%ifdef DEBUG
		push	ax
		push	cx
		push	si
		mov	cx, 16
		mov	si,SpecPkt
ReadDump:	mov	al, ' '
		call	print_char
		mov	al, byte [si]	;Hexdump a SpecPkt byte
		call	print_hex8
		inc	si			;Point to next byte
		loop	ReadDump
		pop	si
		pop	cx
		pop	ax
%endif
		mov	si,SpecPkt
		mov	dl,[DriveNumber]
		mov	ah,42h			;Extended Read
		int	13h
		jnc	ReadLGd			;If success

;hang:
;		jmp	hang
;		TRACER '1'
		loop	ReadLAgain
		TRACER '2'
		jmp short ReadLBad
ReadLGd:
		TRACER '3'
		xor	ax, ax 			;Status 0 = success
		jmp short ReadLExit

ReadLBad:
		TRACER '9'
		mov	ax, 800Bh		;Set Read Fault Error
		; flow into ReadLExit
ReadLExit:
		TRACER 'r'
		retn



%ifdef DEBUG_TRACERS
debug_tracer:	pushad
		pushfd

		mov	al, '['
		mov	ah,0Eh			;BIOS video teletype output
		xor	bh, bh
		int	10h			;Print it

		mov	bp,sp
		mov	bx,[bp+9*4]		; Get return address
		mov	al,[cs:bx]		; Get data byte
		inc	word [bp+9*4]	; Return to after data byte

		mov	ah,0Eh			;BIOS video teletype output
		xor	bh, bh
		int	10h			;Print it

		mov	al, ']'
		mov	ah,0Eh			;BIOS video teletype output
		xor	bh, bh
		int	10h			;Print it

		popfd
		popad
		retn
%endif

;-----------------------------------------------------------------------------
; PRINT_HEX4
;-----------------------------------------------------------------------------
; print a 4 bits integer in hex
;
; Input:
;	AL - 4 bits integer to print (low)
;
; Output: None
;
; Registers destroyed: None
;
print_hex4:

	push	ax
	and	al, 0fh		; we only need the first nibble
	cmp	al, 10
	jae	hex_A_F
	add	al, '0'
	jmp	hex_0_9
hex_A_F:
	add	al, 'A'-10
hex_0_9:
	call	print_char
	pop	ax
	retn


;-----------------------------------------------------------------------------
; print_hex8
;-----------------------------------------------------------------------------
; print	a 8 bits integer in hex
;
; Input:
;	AL - 8 bits integer to print
;
; Output: None
;
; Registers destroyed: None
;
print_hex8:

	push	ax
	push	bx

	mov	ah, al
	shr	al, 4
	call	print_hex4

	mov	al, ah
	and	al, 0fh
	call	print_hex4

	pop	bx
	pop	ax
	retn


;=============================================================================
; print_hex16 - print a 16 bits integer in hex
;
; Input:
;	AX - 16 bits integer to print
;
; Output: None
;
; Registers destroyed: None
;=============================================================================
print_hex16:

	push	ax
	push	bx
	push	cx

	mov	cx, 4
print_hex16_loop:
	rol	ax, 4
	call	print_hex4
	loop	print_hex16_loop

	pop	cx
	pop	bx
	pop	ax
	retn

;=============================================================================
; print_hex32 - print a 32 bits integer in hex
;
; Input:
;	EAX - 32 bits integer to print
;
; Output: None
;
; Registers destroyed: None
;=============================================================================
print_hex32:

	push	eax
	push	bx
	push	cx

	mov	cx, 8
print_hex32_loop:
	rol	eax, 4
	call	print_hex4
	loop	print_hex32_loop

	pop	cx
	pop	bx
	pop	eax
	retn

;=============================================================================
; print_string - print string at current cursor location
;
; Input:
;	DS:SI - ASCIIZ string to print
;
; Output: None
;
; Registers destroyed: None
;=============================================================================
print_string:
		push	ax
		push	si

print_string_again:
		mov	al, [si]
		or	al, al
		jz	print_string_exit
		call	print_char
		inc	si
		jmp	print_string_again

print_string_exit:
		pop	si
		pop	ax
		retn

;-----------------------------------------------------------------------------
; PRINT_CHAR
;-----------------------------------------------------------------------------
; Print's a character at current cursor position
;
; Input:
;	AL - Character to print
;
; Output: None
;
; Registers destroyed: None
;
print_char:

		push	ax
		push	bx

		mov	ah,0Eh			;BIOS video teletype output
		xor	bh, bh
		int	10h			;Print it

print_char_exit:
		pop	bx
		pop	ax
		retn


;=============================================================================

;This space is used as a 2048-byte read buffer plus one test byte.
;The 96h data is used for testing the number of bytes returned by an Extended
;  CD-ROM sector read

		align	16, db 0
Buffer		times	2049	db	96h

;=============================================================================

Init:			;Initialization Routine

		TRACER 'I'
		mov	ax,cs			;ds=cs
		mov	ds,ax

%ifdef DEBUG
; print CS value (load segment)
		call	print_hex16
%endif

		mov	si, Greeting	;Display Greeting
		call	print_string

		mov	ax,Unsupported	;Init is executed only once
		mov	[Routines],ax

		mov	ax, 5400h
		int	13h			; Get diskemu status
		jc	FindBoot		; If CF=1 no diskemu loaded

		mov	[DriveNumber], cl		; Store drive number

		call	keyflag
		and	al, 8			; alt key ?
		jz	extread

		mov	si, DrvNumMsg	; Display "drive number="
		call	print_string
		mov	al, [DriveNumber]
		call	print_hex8
		mov	si, LineEnd	; CR/LF
		call	print_string
		jmp	extread

; Diskemu is not loaded
; so loop to find drive number
		; *** start of 1.4 changes ***
		; ??? mov dl, 0ffh		;Start at Drive 0xff
		; *** FindBoot at c47 in 1.4, at c0c in 1.3 ***
FindBoot:	call	ScanDrives		; call new helper in 1.4
		jnc	FoundBoot		; ded*df3
;		mov	si,offset SpecPkt	;Locate booted CD-ROM drive
;		mov	[SpecPkt],0		;Clear 1st byte of SpecPkt
;		mov	ax,4B01h		;Get Bootable CD-ROM Status
;		int	13h
;		jnc	FindPass		;If booted CD found
;
; Carry is not cleared in buggy Dell BIOSes,
; so I'm checking packet size byte
; some bogus bioses (Dell Inspiron 2500) returns packet size 0xff when failed
; Dell Dimension XPsT returns packet size 0x14 when OK

;		cmp	[SpecPkt], 0
;		jne	FoundBoot

;		cmp	[SpecPkt], 13h	; anything between 13h and 20h should be OK
;		jb	FindFail
;		cmp	[SpecPkt], 20h
;		ja	FindFail
;		jmp	short FoundBoot
;
; FindFail:
;		dec	dl			;Next drive
;		cmp	dl, 80h
;		jae	FindBoot		;Check from ffh..80h
		; *** end of 1.4 changes ***

		mov	si,NoBootCD	;No booted CD found,
		call	print_string
		jmp	NoEndAddr		;Do not install driver

FoundBoot:
;		mov	dl, [SpecPkt+2]		; 1.4 change
		; *** next line at c57 in 1.4, at c3d in 1.3 ***
		mov	[DriveNumber],dl		;Booted CD-ROM found,
						;  so save Drive #

		call	keyflag
		and	al, 8			; alt key ?
		jz	extread

		mov	si, CDStat
		call	print_string
		mov	si, SpecPkt	;Point to returned CD SpecPkt
		mov	cx, 19			;  containing 19 bytes
StatDump:	mov	al, ' '			;Print a space
		call	print_char
		mov	al, byte [si]	;Hexdump a SpecPkt byte
		call	print_hex8
		inc	si			;Point to next byte
		loop	StatDump

		mov	si, LineEnd	;Print a CR/LF
		call	print_string

extread:
;See how many CD Sector bytes are returned by an Extended Read
		mov	byte [SpecPkt],16	;SpecPkt Size
		mov	byte [SpecPkt+1],0	;Reserved
		mov	word [SpecPkt+2],1	;Transfer one sector
		mov	word [SpecPkt+6],cs	;Into our Buffer
		mov	word [SpecPkt+4],Buffer
		mov	word [SpecPkt+8],16	;From CD Sector 16
		mov	word [SpecPkt+10],0
		mov	word [SpecPkt+12],0
		mov	word [SpecPkt+14],0

		mov	si, SpecPkt	;Set ds:si --> SpecPkt
		mov	dl, [DriveNumber]
		mov	ah, 42h			;Extended Read
		int	13h
		jnc	SecSize			;If success

		mov	ah, 42h			;Always make 2 read attempts
		int	13h
						;How many bytes did we get?
SecSize:	std				;Count down
		mov	ax,cs			;Point to end of Buffer
		mov	es,ax
		mov	di,Buffer+2047	;Find end of read data
		mov	si,Buffer+2048
		mov	cx,2049
		repe	cmpsb			;cx = number of bytes read

		cld				;Restore count direction to up
		mov	si,CDBytes	;Display number of bytes read
		call	print_string

		mov	al, [DriveNumber]
		call	print_hex8

		mov	si,CDBytesA	;Remainder A of message
		call	print_string

		mov	al,ch			;Hex-dump cx
		and	al,0Fh			;Second nibble
		call	print_hex8		;  (don't need the First)
		mov	al,cl
		call	print_hex8		;  (don't need the First)

		mov	si,CDBytesB	;Remainder B of message
		call	print_string

		cmp	cx,2048			;Did we read 2048 bytes?
		je	ParseParm		;If yes <-- O.K.

		mov	byte [ReadBytes],1
		cmp	cx,1024			;Did we read 1024 bytes?
		je	ParseParm		;If yes <-- O.K.

		mov	byte [ReadBytes],2
		cmp	cx,512			;Did we read 512 bytes?
		jne	NoEndAddr		;If not, do not load driver

ParseParm:	mov	bx,word [cs:ReqHdrLoc]	;Parse command line
		mov	es,word [cs:ReqHdrLoc+2]	;  parameters
		mov	si,[es:bx+18]		;Get BPB array ptr into DS:SI
		mov	ds,[es:bx+20]
FindParm:	inc	si
FindParm1:	cmp	byte [si],0Dh	;CR? (End of parameters)
		je	EndOfParms

		cmp	byte [si],0Ah	;LF?
		je	EndOfParms

		cmp	byte [si],'/'	;A parameter?
		jne	FindParm

		inc	si
		cmp	byte [si],'D'	;Device Name parameter?
		jne	FindParm1

		inc	si
		cmp	byte [si],':'
		jne	FindParm1

;bbb
		push	si
		mov	si, DevName	;Device Name is at ds:si
		push	ds			;Keep ptr to Device Name
		mov	ax, cs
		mov	ds, ax
		call	print_string
		pop	ds			;Retrieve Device Name ptr
		pop	si
		mov	cx, 8			;Get next 8 chars
		inc	si			;  = Device Name
		mov	ax, cs
		mov	es, ax
		mov	di, DeviceName
NextChar:	cmp	byte [si],' '
		ja	AboveSpace

		mov	ax,cs			;Pad end of Device Name with
		mov	ds,ax			;  spaces if necessary
		mov	si,DblSpace	;A space
AboveSpace:	mov	al, [si]
		call	print_char
		movsb				;ds:[si] --> es:[di]
		loop 	NextChar

		mov	si,LineEnd
		mov	ax,cs
		mov	ds,ax
		call	print_string

		mov	ax,Init-2	;Last byte of driver to keep
		jmp	EndAddr			;Install driver

EndOfParms:
		mov	ax, cs			; Restore segment registers (fix)
		mov	ds, ax
		mov	es, ax

		mov	si,NoDevName	;No Device Name Found
		call	print_string

NoEndAddr:	mov	ax,0			;Do not install driver

EndAddr:	mov	es,[ReqHdrLoc+2]		;Write End Address
		mov	bx,[ReqHdrLoc]
		mov	[es:bx+14],ax
		mov	[es:bx+16],cs
		mov	bx,ax			;Hold onto install status

		mov	si, DrvInst	;Display driver install status
		call	print_string
		mov	si, DrvInst1	;Assume driver installed
		cmp	bx,0			;Was driver installed?
		jne	DrvStatus		;If yes
		mov	si, NoDrvInst	;Driver not installed
DrvStatus:	call	print_string

		mov	ax,0			;Set Return Status = success
		cmp	bx,0			;Was INIT successful?
		jne	InitStat		;If yes
		mov	ax,800Ch		;Status = General Failure
InitStat:
		push	ax			;Save Return Status

		call	keyflag
		and	al, 8			; alt key ?
		jz	InitExit

WaitHere:
		mov	si, WaitMsg	;Display Halted message
		call	print_string

AltWait:
		call	keyflag
		and	al, 8			; Alt key?
		jnz	AltWait			; Pressed? yes -> wait

InitExit:
		pop	ax			;Retrieve Return Status
		TRACER 'i'
		retn				;That's it for Init!

		; *** start 1.4 changes at ded ***
SpecGo:		mov	si,SpecPkt
		int	13h
		retn

ScanDrives:	push	ax		; at df3 in 1.4
		push	si
		mov	dl, 0h		;Start at Drive 0xFF
NextDrv:	dec	dl
		clc
		mov	ax,4B01h	;Get Bootable CD-ROM Status
		mov	BYTE [SpecPkt],0	;Clear 1st byte of SpecPkt
		call	SpecGo
; Carry is not cleared in buggy Dell BIOSes,
; so I'm checking packet size byte
; some bogus bioses (Dell Inspiron 2500) returns packet size 0xff when failed
; Dell Dimension XPsT returns packet size 0x14 when OK

		cmp	BYTE [SpecPkt], 13h	; anything between 13h and 20h should be OK
		jb	FindFail
		cmp	BYTE [SpecPkt], 20h
		ja	FindFail	; in 1.4 at e16
		test	BYTE [SpecPkt+1], 0Fh	; media_type=0 means no-emulation mode
		jnz	FindFail
		cmp	dl, [SpecPkt+2]
		je	SendFound	; success with CF=0

FindFail:	cmp	dl, 80h		; Check from 80h..ffh
		jne	NextDrv		; Next drive
SendFail:	xor	dl,dl
		stc
		;jmp	short ThingDone	; yes, this jmp can be omitted.
SendFound:	;mov	dl, [SpecPkt+2]	; already equal on success
		;clc			; already cleared CF on success
ThingDone:	pop	si
		pop	ax
		retn
		; *** end 1.4 changes ***

;=============================================================================

;------------------------------------------------------------
; keyboard flags - return keyboard flags in AL
; bit 3 = ALT key
keyflag:	; at dbc in 1.3, at e2e in 1.4
	push	bx
	mov	ah, 2
	int	16h
	pop	bx
	retn

;=============================================================================

DrvNumMsg	db	'  Diskemxx.bin returned drive number=', 0
NoBootCD	db	'  No booted CD-ROM found.',CR,0

CDStat		db	'  INT 13h / AX=4B01h Specification Packet for '
		db	'Booted CD-ROM:',CR,'     ', 0

CDBytes		db	'  Drive ', 0
CDBytesA	db	' returns ', 0
CDBytesB	db	'h bytes per Sector.',CR,0

DevName		db	'  Device Name: ', 0
NoDevName	db	'  No Device Name found. '
		db	'Usage: device=eltorito.sys /D:<DevName>',CR,0

DrvInst		db	'  Driver ', 0
NoDrvInst	db	7,'not '		;7 = Ctrl-G = Beep
DrvInst1	db	'installed',CR,0

WaitMsg		db	'  Alt pressed, waiting...', CR, 0
;ContMsg		db	'  Continuing...'
LineEnd		db	CR,0


;=============================================================================
