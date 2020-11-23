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
//#define	ABS(x)	((x) - EXT_C(main) + 0x8200)
#define	ABS(x)	(x)


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

	.code64
	/* If BP=0x07C0, it indicates boot from pxe or no-emulation cdrom */

	/*
	 *  Guarantee that "main" is loaded at 0x0:0x8200 in stage2 and
	 *  at 0x0:0x2200 in stage1.5.
	 */
   
		jmp	real_codestart

	. = EXT_C(main) + 0x5																										//0x8205

	/* control byte: pxe, DUCE, tune																				//控制字节
	 * bit 0 = 1: disable pxe																								//禁用PXE
	 * bit 1 = 1: disable keyboard intervention in boot process							//禁用键盘干预引导过程中
	 * bit 2 = 1: disable the "unconditional command-line entrance" feature	//禁用“无条件命令行入口”功能
	 * bit 3 = 1: disable geometry tune																			//禁用几何协调
	 * bit 4 = 1: disable startup cdrom drive look-up.											//禁止启动光盘驱动器查找。
	 * bit 5 = 1: enable HLT instruction for checkkey idle loop.						//使能HLT指令，当执行checkkey空闲循环时。
	 */
	.byte	0

	. = EXT_C(main) + 0x6																										//0x8206  修改为保存DX  uefi未使用

	.byte	COMPAT_VERSION_MAJOR, COMPAT_VERSION_MINOR

	. = EXT_C(main) + 0x8                                                   //0x8208

VARIABLE(install_partition) //unsigned int              引导分区  0xffff=0分区;		0x3ffff=3分区;		0x0ffffff=整个磁盘
	.long	0x00FFFFFF

	. = EXT_C(main) + 0xC																										//0x820c

VARIABLE(saved_entryno) //int
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
	.long	0

	. = EXT_C(main) + 0x10																									//0x8210

VARIABLE(stage2_id)																												//stage2 id  00
	.byte	STAGE2_ID

VARIABLE(force_lba)																												//0x8211 强制LBA  uefi未使用
	/* only bit 0 is now used for force_lba. */
	/* bit 1=1 means we are booting as a Linux kernel. */
	/* bit 2=0 enable single-key-selection feature. */
	.byte	0x04

VARIABLE(version_string)                                                  //0x8212
	.string VERSION                                                         //0.97\0(hd0,0)

VARIABLE(config_file)																											//0x8217-0x8221		配置文件
	.string "/efi/grub/menu.lst"

	. = EXT_C(main) + 0x34																									//0x8234-0x8238	!!0x8244-0x8238 0x12字节未使用

	.word	0x14C	/* CPU type for i386 and compatibles */											//i386和兼容机CPU类型		0000
	.word	1	/* there is 1 section */																				//还有1节								0000		

	. = EXT_C(main) + 0x44																									//0x8244	!!0x826b-0x824e 0x30字节未使用

	.word	0x00	/* size of optional header */																//可选头尺寸										0x5244
	.word	3	/* flags: "no relocation" and "valid executable" */							//标志：“不搬迁”和“有效的执行”	0x0000
	.word	0x722E	/* section name for resources */													//名称资源节										0x0000
	.word	0x7273																														//															0x0000
	.word	0x63																															//															0x5eebaa55

#if !HOTKEY
	. = EXT_C(main) + 0x60

VARIABLE(external_hotkey)  //int*                                         //0x8260	外置热键功能
  .quad 0 
#endif

	. = EXT_C(main) + 0x68                                                  //0x826c
  
VARIABLE(min_con_mem_start)                                               //0x8268  最低常规内存起始(字节)  int 
	.long	0
VARIABLE(min_con_mem_size)                                                //0x826c  最低常规内存尺寸(字节)  int 
	.long	0

	. = EXT_C(main) + 0x70																									//0x8270  uefi未使用
 	.word	0
	.byte 0
  
	. = EXT_C(main) + 0x73
VARIABLE(debug_msg) //byte
	.byte 2
	/* internal variables follow */
	. = EXT_C(main) + 0x74																									//0x8274
VARIABLE(menu_num_ctrl) //unsigned char menu_num_ctrl[4]  4字节
	.long 0x2000                    //2000/2001/2d01  位0: 00/01/02=不显示自动菜单/显示自动菜单_引导标题/显示自动菜单_所有标题
                                  //                位1: 20/2d=自动菜单标号后显示' '/自动菜单标号后显示'-'
                                  //                位2: 返回当前菜单标号
                                  //由于传统菜单使用"write 0x8274 0x2001",是4字节,改变了0x8276,0x8277的值,所以使用这2字节时,要判断是否有影响.
																	//0x8276返回当前菜单项号
#include "grub4dos_int_ver.h"
#ifdef GRUB4DOS_INT_VER
	.long GRUB4DOS_INT_VER																									//0x8278		GRUB4DOS编译的日期十进制数. 2010-12-30日编译的是: 20121230=0x133068e
#else
	.long 0 
#endif
#undef GRUB4DOS_INT_VER

#if HOTKEY
VARIABLE(hotkey_func_enable)    //int                                     0x827c    内置热键功能允许
#endif
	.long 0
  
	. = EXT_C(main) + 0x80																									//0x8280

VARIABLE(boot_drive)                                                      //引导驱动器 int
	.long	0xFFFFFFFF	/* can be setup by external programs. */

VARIABLE(use_config_file)   //int
	.long	1
  
VARIABLE(configfile_opened) //int
	.long	0

VARIABLE(saved_pxe_ip)      //int
	.long	0

	. = EXT_C(main) + 0x90																									//0x8290

VARIABLE(filesize)                                                        //文件尺寸  long long
	.long	0, 0
//VARIABLE(console_mode)                                                  //0x8298  未使用
	.long	0
VARIABLE(saved_partition)																									//0x829c	int	根分区
	.long	0

	. = EXT_C(main) + 0xA0																									//0x82a0

VARIABLE(saved_drive)																											//根驱动器  int
	.long	0
VARIABLE(no_decompression)																								//0x82a4	int	不解压缩	0/1=开启/关闭压缩文件支持
	.long	0
VARIABLE(part_start)																											//0x82a8		分区开始  long long
	.long	0, 0

	. = EXT_C(main) + 0xB0																									//0x82b0

VARIABLE(part_length)																											//分区长度  long long
	.long	0, 0

VARIABLE(fb_status)																												//0x82b8	FB状态  int
	.long	0

VARIABLE(is64bit)																													//0x82bc	64位支持 int
	.long	0

	. = EXT_C(main) + 0xC0																									//0x82c0
  
VARIABLE(linux_text_len)                                                  //int
	.long	0

VARIABLE(cursor_state)	                                                  //0x82c4 鼠标状态  int
	.long	0 //bit 0=1 show cursor,bit 1=1 show splashimage		位0=1显示光标，位1=1显示图像
 
VARIABLE(menu_cfg)                                                        //0x82c8(原0x8308)		char menu_cfg[2] 2字节, 实际占用4字节
	.long 0x1110

VARIABLE(ram_drive)																												//0x82cc		RAM驱动器  int
	.long	0x7F	/* default is a floppy */

	. = EXT_C(main) + 0xD0																									//0x82d0

VARIABLE(rd_base)																													//RD基地址  long long
	.long	0, 0

VARIABLE(rd_size)																													//0x82d8		RD尺寸  long long
	.long	0, 1	/* default is 4G */

	. = EXT_C(main) + 0xE0																									//0x82e0

VARIABLE(return_value)                                                    //int       返回值
	.long	0

VARIABLE(QUOTE_CHAR)                                                      //int
	.long	0

VARIABLE(linux_bzimage_tmp_addr)                                          //char *
	.long	LINUX_TMP_MEMORY
  .long 0

	. = EXT_C(main) + 0xF0																									//0x82f0

VARIABLE(linux_data_tmp_addr)                                             //char *
	.long	0, 0

VARIABLE(linux_data_real_addr)                                            //char *
	.long	0, 0

	. = EXT_C(main) + 0x100																									//0x8300
  
VARIABLE(addr_system_functions)																						//0x8300    系统功能地址
	.quad	ABS(EXT_C(system_functions))

VARIABLE(addr_system_variables)																						//0x8308		系统变量地址(原0x8304）
	.quad	ABS(EXT_C(system_variables))	

  
	. = EXT_C(main) + 0x110																									//0x8310
  
VARIABLE(iso_types)                                                       //byte
	.byte 0xff
  .byte 0 //未使用
  .byte 0 //未使用
  .byte 0 //未使用

VARIABLE(errnum)																													//0x8314		错误号  grub_error_t  64位是8字节?
	.long	0

VARIABLE(current_drive)																										//0x8318		当前驱动器  int
	.long	0xFFFFFFFF

VARIABLE(current_partition)																								//0x831c		当前分区  int
	.long	0

	. = EXT_C(main) + 0x120																									//0x8320

VARIABLE(filemax)																													//文件最大  long long
	.long	0, 0

VARIABLE(filepos)																													//0x8328		文件指针  long long
	.long	0, 0

	. = EXT_C(main) + 0x130																									//0x8330

VARIABLE(debug)																														//调试  int
	/* 0 for silent, 1 for normal, 2..MAX_UINT for verbose.									//0=沉默，1=正常，2=冗长MAX_UINT。  MAX_UINT 是 0x7FFFFFFF
	 * if debug == MAX_UINT or debug < 0, then filesystem debug mode is on. //如果调试=MAX_UINT或调试<0，那么文件系统调试模式开。 
	 * (MAX_UINT is 0x7FFFFFFF)
	 */
	.long	0			

VARIABLE(current_slice)																										//0x8334		当前片  int
	.long	0
 
VARIABLE(saved_pxe_mac)																							      //0x8338		 unsigned char saved_pxe_mac[6];
	.byte	0, 0, 0, 0, 0, 0
  
  .byte	0, 0 //未使用

	. = EXT_C(main) + 0x140																									//0x8340

VARIABLE(buf_track)																												//缓存跟踪  long long
	.long	-1
	.long	0
VARIABLE(buf_drive)																												//缓存驱动器  int
	.long	-1
	.long	0 //未使用

	. = EXT_C(main) + 0x150																									//0x8350
	
VARIABLE(fd_devices)                                                      //软盘驱动器信息指针  grub_disk_data *
	.long	0, 0

VARIABLE(hd_devices)                                                      //0x8358 硬盘驱动器信息指针  grub_disk_data *
	.long	0, 0

	. = EXT_C(main) + 0x160																									//0x8360

VARIABLE(cd_devices)                                                      //光盘驱动器信息指针  grub_disk_data *
	.long	0, 0

VARIABLE(partition_info)                                                  //0x8368 分区信息指针  grub_part_data *
	.long	0, 0
	
	. = EXT_C(main) + 0x170																									//0x8370

VARIABLE(floppies_orig)			                                              //软盘数 byte
	.byte	0
VARIABLE(harddrives_orig)		                                              //硬盘数 byte
	.byte	0
VARIABLE(cdrom_orig)				                                              //光盘数 byte
	.byte	0	

	.byte	0	                                                                //未使用
  .long	0, 0, 0                                                           //未使用
	
	. = EXT_C(main) + 0x180																									//0x8370
	
ENTRY(disk_fragment_map)	.space	FRAGMENT_MAP_SLOT_SIZE                  //映射碎片插槽

	. = EXT_C(main) + 0x400																									//0x8670
	
ENTRY(disk_buffer)	.space	0x1000                                        //磁盘缓存

	. = EXT_C(main) + 0x1400																								//0x9600
		
ENTRY(disk_drive_map)	.space	[DRIVE_MAP_SIZE + 1] * DRIVE_MAP_SLOT_SIZE	//磁盘驱动器映射插槽  78*8=3c0

	. = EXT_C(main) + 0x1800	
real_codestart:

	.align	8
#if i386
//kern/i386/efi/startup.S
	.code32

	movl	4(%esp), %eax
	movl	%eax, EXT_C(grub_efi_image_handle)
	movl	8(%esp), %eax
	movl	%eax, EXT_C(grub_efi_system_table)
	call	EXT_C(grub_init)
  
#else
//------------------------------------------------------------------
//kern/x86_64/efi/startup.S
	.code64
	movq	%rcx, EXT_C(grub_efi_image_handle)(%rip)    //映像处理
	movq	%rdx, EXT_C(grub_efi_system_table)(%rip)    //系统表

	andq	$~0xf, %rsp
	call	EXT_C(grub_init)
	// Doesn't return. 
//-----------------------------------------------------------------------
//kern/x86_64/efi/callwrap.S
ENTRY(efi_wrap_0)
	subq $40, %rsp
	call *%rdi
	addq $40, %rsp
	ret

ENTRY(efi_wrap_1)
	subq $40, %rsp
	mov  %rsi, %rcx
	call *%rdi
	addq $40, %rsp
	ret

ENTRY(efi_wrap_2)
	subq $40, %rsp
	mov  %rsi, %rcx
	call *%rdi
	addq $40, %rsp
	ret

ENTRY(efi_wrap_3)
	subq $40, %rsp
	mov  %rcx, %r8
	mov  %rsi, %rcx
	call *%rdi
	addq $40, %rsp
	ret

ENTRY(efi_wrap_4)
	subq $40, %rsp
	mov %r8, %r9
	mov %rcx, %r8
	mov %rsi, %rcx
	call *%rdi
	addq $40, %rsp
	ret

ENTRY(efi_wrap_5)
	subq $40, %rsp
	mov %r9, 32(%rsp)
	mov %r8, %r9
	mov %rcx, %r8
	mov %rsi, %rcx
	call *%rdi
	addq $40, %rsp
	ret

ENTRY(efi_wrap_6)
	subq $56, %rsp
	mov 56+8(%rsp), %rax
	mov %rax, 40(%rsp)
	mov %r9, 32(%rsp)
	mov %r8, %r9
	mov %rcx, %r8
	mov %rsi, %rcx
	call *%rdi
	addq $56, %rsp
	ret

ENTRY(efi_wrap_7)
	subq $88, %rsp
	mov 88+16(%rsp), %rax
	mov %rax, 48(%rsp)
	mov 88+8(%rsp), %rax
	mov %rax, 40(%rsp)
	mov %r9, 32(%rsp)
	mov %r8, %r9
	mov %rcx, %r8
	mov %rsi, %rcx
	call *%rdi
	addq $88, %rsp
	ret

ENTRY(efi_wrap_10)
	subq $88, %rsp
	mov 88+40(%rsp), %rax
	mov %rax, 72(%rsp)
	mov 88+32(%rsp), %rax
	mov %rax, 64(%rsp)
	mov 88+24(%rsp), %rax
	mov %rax, 56(%rsp)
	mov 88+16(%rsp), %rax
	mov %rax, 48(%rsp)
	mov 88+8(%rsp), %rax
	mov %rax, 40(%rsp)
	mov %r9, 32(%rsp)
	mov %r8, %r9
	mov %rcx, %r8
	mov %rsi, %rcx
	call *%rdi
	addq $88, %rsp
	ret		
#endif
//---------------------------------------------------------------------------------------
	.code64
	.align	8
//VARIABLE(pxenv)       //ipxe
VARIABLE(pxe_tftp_open) //PXENV_TFTP_OPEN_t
	.word	0		# Status	状态
	.long	0		# ServerIPAddress			服务器IP地址 
	.long	0		# GatewayIPAddress		网关IP地址
	.space	128		# FileName[128];	文件名
	.word	0		# TFTPPort						TFTP端口
	.word	0		# PacketSize					包尺寸

//boot.c
	.align	8
VARIABLE(mbi) //multiboot_info
	.space	(22 * 4)
	

/****************************************************************************/
/************************* 32-bit functions follow **************************/
/****************************************************************************/

//	.space	0x300000	/* !!!! insert 3M !!!! */

/* get_code_end() :  return the address of the end of the code
 * This is here so that it can be replaced by asmstub.c.
 */

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
//#define mem64 ((int (*)(int, unsigned long long, unsigned long long, unsigned long long))((*(int **)0x8300)[74]))
#define open ((int (*)(char *))((*(int **)0x8300)[26]))
#define read ((unsigned long (*)(unsigned long long, unsigned long long, unsigned long))((*(int **)0x8300)[27]))
#define close ((void (*)(void))((*(int **)0x8300)[28]))
#define grub_malloc ((void *(*)(unsigned long))((*(int **)0x8300)[50]))

http://grubutils.googlecode.com/svn/trunk/src/include/grub4dos.h
 *
 */
 
//虽然 i386 32位系统在此处可以使用 long, 但是为了简化批处理, 统一使用 quad.
VARIABLE(system_functions)                            //IMG(0x8300)
	.quad ABS(EXT_C(grub_sprintf))                      //0
	.quad	ABS(EXT_C(grub_putstr))                       //1
//VARIABLE(grub_putchar)
//	.quad	ABS(EXT_C(_putchar))
	.quad	ABS(EXT_C(grub_putchar))                      //2
	.quad	ABS(EXT_C(get_cmdline_obsolete))              //3
	.quad	ABS(EXT_C(getxy))                             //4
	.quad	ABS(EXT_C(gotoxy))                            //5
	.quad	ABS(EXT_C(cls))                               //6
	.quad	ABS(EXT_C(wee_skip_to))	                      //7   was obsolete setcursor
	.quad	ABS(EXT_C(nul_terminate))                     //8
	.quad	ABS(EXT_C(safe_parse_maxint_with_suffix))     //9
	.quad	ABS(EXT_C(substring))                         //10
	.quad	ABS(EXT_C(grub_strstr))                       //11
	.quad	ABS(EXT_C(grub_strlen))                       //12
	.quad	ABS(EXT_C(grub_strtok))                       //13
	.quad	ABS(EXT_C(grub_strncat))                      //14
	.quad	ABS(EXT_C(grub_strcmp))                       //15
	.quad	ABS(EXT_C(grub_strcpy))                       //16
	.quad ABS(EXT_C(grub_efidisk_readwrite))			      //17
  .quad ABS(EXT_C(blockio_read_write))				        //18
	.quad	ABS(EXT_C(getkey))	                          //19
	.quad	ABS(EXT_C(checkkey))                          //20
	.quad	0				                                      //21  原ABS(EXT_C(pxe_call))	现在未使用
	.quad	ABS(EXT_C(grub_memcmp))                       //22
	.quad	ABS(EXT_C(grub_memmove))                      //23
	.quad	ABS(EXT_C(grub_memset))                       //24
	.quad 0				                                      //25  reserved
	.quad	ABS(EXT_C(grub_open))                         //26
	.quad	ABS(EXT_C(grub_read))                         //27
	.quad	ABS(EXT_C(grub_close))                        //28
	.quad 0				                                      //29  reserved
	.quad 0				                                      //30  reserved
  VARIABLE(disk_read_hook)
  .quad 0                                             //31
	.quad	ABS(EXT_C(devread))                           //32
	.quad	0                                             //33  原ABS(EXT_C(devwrite)) 现在未使用
	.quad	ABS(EXT_C(next_partition))                    //34
	.quad	ABS(EXT_C(open_device))                       //35
	.quad	ABS(EXT_C(real_open_partition))               //36
	.quad	ABS(EXT_C(set_device))                        //37
	.quad 0				                                      //38  reserved
	.quad 0				                                      //39  reserved
	.quad 0				                                      //40  reserved
	.quad	ABS(EXT_C(parse_string))                      //41
	.quad	ABS(EXT_C(hexdump))                           //42
	.quad	ABS(EXT_C(skip_to))                           //43
	.quad	ABS(EXT_C(builtin_cmd))                       //44
	.quad	ABS(EXT_C(get_datetime))                      //45
	.quad	ABS(EXT_C(find_command))                      //46
	.quad 0				                                      //47  reserved
	.quad 0				                                      //48  reserved
	.quad	0				                                      //49  原ABS(EXT_C(get_mmap_entry)) 现在未使用
	.quad	ABS(EXT_C(grub_malloc))                       //50
	.quad	ABS(EXT_C(grub_free))                         //51
	.quad 0                                             //52  原list_partitions 现在未使用
	.quad 0						                                  //53  原ABS(EXT_C(realmode_run)) realmode_run	reserved for wee  现在未使用
	.quad 0                                             //54  未使用
	.quad 0                                             //55  未使用
	.quad 0                                             //56  未使用
	.quad 0                                             //57  未使用
	.quad 0                                             //58  未使用
	.quad 0                                             //59  未使用
	.quad 0                                             //60  未使用
	////////////////////////////////////////////61
	.quad	ABS(EXT_C(dir))                               //61
	.quad	ABS(EXT_C(print_a_completion))                //62
	.quad	ABS(EXT_C(print_completions))                 //63
	.quad	0                                             //64  未使用
	.quad	ABS(EXT_C(probe_bpb))                         //65
	.quad	ABS(EXT_C(probe_mbr))                         //66
	.quad	ABS(EXT_C(unicode_to_utf8))                   //67
	.quad	ABS(EXT_C(rawread))                           //68
	.quad	ABS(EXT_C(rawwrite))                          //69
	.quad	ABS(EXT_C(setcursor))                         //70
	.quad	ABS(EXT_C(grub_tolower))                      //71
	.quad	ABS(EXT_C(grub_isspace))                      //72
	.quad	ABS(EXT_C(grub_sleep))                        //73
	.quad	0			                                        //74  原ABS(EXT_C(mem64)) 现在未使用
	.quad ABS(EXT_C(envi_cmd))                          //75
	.quad ABS(EXT_C(strncmpx))                          //76
#ifdef SUPPORT_GRAPHICS
	.quad ABS(EXT_C(rectangle))                         //77
#else
	.quad 0
#endif
	.quad ABS(EXT_C(get_cmdline))                       //78

/*
 * The C code for a grub4dos executable may have defines as follows:
 *

#define next_partition_drive		((*(unsigned long **)0x8308)[0])
#define next_partition_dest		((*(unsigned long **)0x8308)[1])
#define next_partition_partition	((*(unsigned long ***)0x8308)[2])
#define next_partition_type		((*(unsigned long ***)0x8308)[3])
#define next_partition_start		((*(unsigned long ***)0x8308)[4])
#define next_partition_len		((*(unsigned long ***)0x8308)[5])
#define next_partition_offset		((*(unsigned long ***)0x8308)[6])
#define next_partition_entry		((*(unsigned long ***)0x8308)[7])
#define next_partition_ext_offset	((*(unsigned long ***)0x8308)[8])
#define next_partition_buf		((*(char ***)0x8308)[9])

 *
 */

VARIABLE(system_variables)              //IMG(0x8308)
VARIABLE(next_partition_drive)          //0 int
	.quad 0
VARIABLE(next_partition_dest)           //1 int
	.quad 0
VARIABLE(next_partition_partition)      //2 int*
	.quad 0
VARIABLE(next_partition_type)           //3 int*
	.quad 0
VARIABLE(next_partition_start)          //4 long long *
	.quad 0
VARIABLE(next_partition_len)            //5 long long *
	.quad 0
VARIABLE(next_partition_offset)         //6 long long *
	.quad 0
VARIABLE(next_partition_entry)          //7 int*
	.quad 0
VARIABLE(next_partition_ext_offset)     //8 int*
	.quad 0
VARIABLE(next_partition_buf)            //9 char *
	.quad 0
VARIABLE(quit_print)                    //10  int
	.quad	0
VARIABLE(block_io_protocol_this)        //11
	.quad	0
//VARIABLE(buf_track)
	.quad	ABS(EXT_C(image))               //12  struct grub_efi_loaded_image
VARIABLE(filesystem_type)               //13  int
	.quad	0
//VARIABLE(query_block_entries)
	.quad	ABS(EXT_C(grub_efi_image_handle))   //14  void *
//VARIABLE(map_start_sector)
	.quad	ABS(EXT_C(grub_efi_system_table))   //15  struct grub_efi_system_table
	.quad	ABS(EXT_C(buf_geom))            //16  geometry
	.quad	ABS(EXT_C(tmp_geom))            //17  geometry
	.quad	0	//ABS(EXT_C(term_table))      //18  reserved
VARIABLE(current_term)
	.quad	ABS(EXT_C(term_table))          //19  term_entry
	.quad	ABS(EXT_C(fsys_table))          //20  fsys_entry
//VARIABLE(fsys_type)//reserved
	.quad	fsys_type		                    //21  int
	.quad	NUM_FSYS		                    //22  常数
VARIABLE(graphics_inited)	              //23  int
	.quad	0
	.quad	0			                          //24  reserved
//VARIABLE(font8x16)                    //25  reserved
	.quad	0
VARIABLE(fontx)	                        //26  int
	.quad	0
VARIABLE(fonty)	                        //27  int
	.quad	0
VARIABLE(graphics_CURSOR)               //28  void *
	.quad	0
	.quad	(EXT_C(menu_border))            //29  border 数组
VARIABLE(gzip_filemax)                  //30  long long
	.quad	0
	.quad	0                               //31  reserved
	//reserved for wee
	.quad	0                               //32  reserved
	.quad	0                               //33  reserved
	.quad	0                               //34  reserved
	.quad	0                               //35  reserved
	.quad	0                               //36  reserved
	.quad	0                               //37  reserved
	.quad	0                               //38  reserved
	.quad	0                               //39  reserved
	.quad	0                               //40  reserved
	.quad	0                               //41  reserved
	//////////////////////////////
VARIABLE(current_color)                 //42  int
	.quad A_NORMAL
VARIABLE(current_color_64bit)           //43  long long
	.quad	0xAAAAAA
  .quad 0                               //44  i386的32位背景色，x86_64没有用。
VARIABLE(p_get_cmdline_str)             //45  未使用
	.quad ABS(EXT_C(get_cmdline_str))
VARIABLE(splashimage_loaded)            //46  int
	.quad 0	
VARIABLE(putchar_hooked)                //47  char*
	.quad 0
//VARIABLE(init_free_mem_start)         //48  reserved
	.quad 0


	.space 0x200

