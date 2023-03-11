/* shared.h - definitions used in all GRUB-specific code */
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

/*
 *  Generic defines to use anywhere
 */

#ifndef GRUB_SHARED_HEADER
#define GRUB_SHARED_HEADER	1

#include <config.h>

//UEFI 编译开关
#define GDPUP   0         //使用设备路径实用程序协议   低版本UEFI固件不支持
#define UNMAP   1         //卸载映像

/* Add an underscore to a C symbol in assembler code if needed. */
#ifdef HAVE_ASM_USCORE
# define EXT_C(sym) _ ## sym
#else
# define EXT_C(sym) sym
#endif

# define RAW_ADDR(x) (x)
# define RAW_SEG(x) (x)

/*
 *  Integer sizes
 */

#define MAXINT     0xFFFFFFFF

/*
 *  Reserved memory by grub4dos system kernel
 */

#define LINUX_TMP_MEMORY	0x2600000
#define LINUX_TMP_MEMORY_LEN  0x36000000
#define LINUX_INITRD_MAX_ADDRESSLINUX	0x38000000

//#define narrow_char_indicator	(*(unsigned int *)(UNIFONT_START + 'A'*num_wide*font_h))   //

/* Maximum command line size. Before you blindly increase this value,
   see the comment in char_io.c (get_cmdline).  */
#define MAX_CMDLINE 1600
#define NEW_HEAPSIZE 1500

/*
 *  This is the location of the raw device buffer.  It is 31.5K
 *  in size.
 */

#define BUFFERLEN   0x10000
#define BOOT_PART_TABLE RAW_ADDR  (0x07be)

/*
 *  BIOS disk defines		BIOS磁盘定义 
 */
#define BIOSDISK_READ			0x0							//读
#define BIOSDISK_WRITE			0x1						//写
#define BIOSDISK_ERROR_GEOMETRY		0x100		//几何错误
#define BIOSDISK_FLAG_LBA_EXTENSION	0x1		//LBA扩展标志
#define BIOSDISK_FLAG_CDROM		0x2					//CDROM标记
#define BIOSDISK_FLAG_BIFURCATE		0x4	/* accessibility acts differently between chs and lba */		//分叉标记
#define BIOSDISK_FLAG_GEOMETRY_OK	0x8			//几何ok
#define BIOSDISK_FLAG_LBA_1_SECTOR	0x10	//读1扇区标记

/* Command-line buffer for Multiboot kernels and modules. This area
   includes the area into which Stage 1.5 and Stage 1 are loaded, but
   that's no problem.  */
#define MB_CMDLINE_BUF		RAW_ADDR (0x7000)
#define MB_CMDLINE_BUFLEN	0x1000

/* The history buffer for the command-line.  */
#define HISTORY_SIZE		5

/* The size of the drive map.  */
#define DRIVE_MAP_SIZE		8

/* The size of the drive_map_slot struct.  */
//#define DRIVE_MAP_SLOT_SIZE		0x70

/* The fragment of the drive map.  */
#define DRIVE_MAP_FRAGMENT		0x27

#define FRAGMENT_MAP_SLOT_SIZE		0x280

/* The size of the key map.  */
#define KEY_MAP_SIZE		128

/*
 *  extended chainloader code address for switching to real mode
 */

//#define HMA_ADDR		0x2B0000

/*
 *  Linux setup parameters
 */

#define LINUX_MAGIC_SIGNATURE		0x53726448	/* "HdrS" */
#define LINUX_DEFAULT_SETUP_SECTS	4
#define LINUX_FLAG_CAN_USE_HEAP		0x80
#define LINUX_INITRD_MAX_ADDRESS	0x38000000
#define LINUX_MAX_SETUP_SECTS		64
#define LINUX_BOOT_LOADER_TYPE		0x71
#define LINUX_HEAP_END_OFFSET		(0x9000 - 0x200)

#define LINUX_BZIMAGE_ADDR		RAW_ADDR (0x100000)
#define LINUX_ZIMAGE_ADDR		RAW_ADDR (0x10000)
#define LINUX_OLD_REAL_MODE_ADDR	RAW_ADDR (0x90000)
#define LINUX_SETUP_STACK		0x9000

#define LINUX_FLAG_BIG_KERNEL		0x1

/* Linux's video mode selection support. Actually I hate it!  */
#define LINUX_VID_MODE_NORMAL		0xFFFF
#define LINUX_VID_MODE_EXTENDED		0xFFFE
#define LINUX_VID_MODE_ASK		0xFFFD

#define LINUX_CL_OFFSET			0x9000
#define LINUX_CL_END_OFFSET		0x93FF
#define LINUX_SETUP_MOVE_SIZE		0x9400
#define LINUX_CL_MAGIC			0xA33F

/*
 *  General disk stuff
 */

#define SECTOR_SIZE		0x200
#define SECTOR_BITS		9
#define BIOS_FLAG_FIXED_DISK	0x80

#define BOOTSEC_LOCATION		RAW_ADDR (0x7C00)
#define BOOTSEC_SIGNATURE		0xAA55
#define BOOTSEC_BPB_OFFSET		0x3
#define BOOTSEC_BPB_LENGTH		0x3B
#define BOOTSEC_BPB_SYSTEM_ID				0x3
#define BOOTSEC_BPB_BYTES_PER_SECTOR		0xB
#define BOOTSEC_BPB_SECTORS_PER_CLUSTER		0xD
#define BOOTSEC_BPB_RESERVED_SECTORS		0xE
#define BOOTSEC_BPB_MAX_ROOT_ENTRIES		0x11
#define BOOTSEC_BPB_MEDIA_DESCRIPTOR		0x15
#define BOOTSEC_BPB_SECTORS_PER_FAT			0x16
#define BOOTSEC_BPB_HIDDEN_SECTORS			0x1C
#define BOOTSEC_BPB_FAT32_SECTORS_PER_FAT	0x24
#define BOOTSEC_BPB_FAT32_ROOT				0x2C
#define BOOTSEC_BPB_FAT_NAME				0x36
#define BOOTSEC_BPB_FAT32_NAME				0x52
#define BOOTSEC_PART_OFFSET		0x1BE
#define BOOTSEC_PART_LENGTH		0x40
#define BOOTSEC_SIG_OFFSET		0x1FE
#define BOOTSEC_LISTSIZE		8

/* Not bad, perhaps.  */
#define NETWORK_DRIVE	0x20

#define PXE_DRIVE	0x21
#define IPXE_PART	0x45585069
#define INITRD_DRIVE	0x22
#define FB_DRIVE	0x23

/*
 *  GRUB specific information
 *    (in LSB order)
 */

#define COMPAT_VERSION_MAJOR	3
#define COMPAT_VERSION_MINOR	2
#define GRUB_INVALID_DRIVE	0xFFFFFFFF

#define STAGE2_VER_MAJ_OFFS	0x6
#define STAGE2_INSTALLPART	0x8
#define STAGE2_SAVED_ENTRYNO	0xc
#define STAGE2_STAGE2_ID	0x10
#define STAGE2_FORCE_LBA	0x11
#define STAGE2_VER_STR_OFFS	0x12

/* Stage 2 identifiers */
#define STAGE2_ID_STAGE2		0

#define STAGE2_ID	STAGE2_ID_STAGE2

/*
 *  defines for use when switching between real and protected mode
 */

#define CR0_PE_ON	0x1
#define CR0_PE_OFF	0xfffffffe
#define PROT_MODE_CSEG	40 /*0x8*/
#define PROT_MODE_DSEG  0x10
#define PSEUDO_RM_CSEG	0x18
#define PSEUDO_RM_DSEG	8 /*0x20*/
#define STACKOFF	MB_CMDLINE_BUF	/* (0x2000 - 0x10) */
#define PROTSTACKINIT   (FSYS_BUF - 0x10)

#define CR0_PE   0x00000001UL
#define CR0_WP   0x00010000UL
#define CR0_PG   0x80000000UL
#define CR4_PSE  0x00000010UL
#define CR4_PAE  0x00000020UL
#define CR4_PGE  0x00000080UL
#define PML4E_P  0x0000000000000001ULL
#define PDPTE_P  0x0000000000000001ULL
#define PDE_P    0x0000000000000001ULL
#define PDE_RW   0x0000000000000002ULL
#define PDE_US   0x0000000000000004ULL
#define PDE_PS   0x0000000000000080ULL
#define PDE_G    0x0000000000000100ULL
#define MSR_IA32_EFER 0xC0000080UL
#define IA32_EFER_LME 0x00000100ULL

/*
 * Assembly code defines
 *
 * "EXT_C" is assumed to be defined in the Makefile by the configure
 *   command.
 */

#define ENTRY(x) .globl EXT_C(x) ; EXT_C(x):
#define VARIABLE(x) ENTRY(x)


#define K_RDWR	0x60	/* keyboard data & cmds (read/write) */
#define K_STATUS	0x64	/* keyboard status */
#define K_CMD		0x64	/* keybd ctlr command (write-only) */

#define K_OBUF_FUL	0x01	/* output buffer full */
#define K_IBUF_FUL	0x02	/* input buffer full */

#define KC_CMD_WIN	0xd0	/* read  output port */
#define KC_CMD_WOUT	0xd1	/* write output port */
#define KB_OUTPUT_MASK  0xdd	/* enable output buffer full interrupt
				   enable data line
				   enable clock line */
#define KB_A20_ENABLE   0x02

/* Codes for getchar. */
#define ASCII_CHAR(x)   ((x) & 0xFF)
#define KEY_LEFT        0x4B00
#define KEY_RIGHT       0x4D00
#define KEY_UP          0x4800
#define KEY_DOWN        0x5000
#define KEY_IC          0x5200	/* insert char */
#define KEY_DC          0x5300	/* delete char */
#define KEY_BACKSPACE   0x0E08
#define KEY_HOME        0x4700
#define KEY_END         0x4F00
#define KEY_NPAGE       0x5100
#define KEY_PPAGE       0x4900
#define A_NORMAL        0x7
#define A_REVERSE       0xF				//0x70

/* In old BSD curses, A_NORMAL and A_REVERSE are not defined, so we
   define them here if they are undefined.  */
#ifndef A_NORMAL
# define A_NORMAL	0
#endif /* ! A_NORMAL */
#ifndef A_REVERSE
# ifdef A_STANDOUT
#  define A_REVERSE	A_STANDOUT
# else /* ! A_STANDOUT */
#  define A_REVERSE	0
# endif /* ! A_STANDOUT */
#endif /* ! A_REVERSE */

/* Define ACS_* ourselves, since the definitions are not consistent among
   various curses implementations.  */
#undef ACS_ULCORNER
#undef ACS_URCORNER
#undef ACS_LLCORNER
#undef ACS_LRCORNER
#undef ACS_HLINE
#undef ACS_VLINE
#undef ACS_LARROW
#undef ACS_RARROW
#undef ACS_UARROW
#undef ACS_DARROW

#define ACS_ULCORNER	'+'
#define ACS_URCORNER	'+'
#define ACS_LLCORNER	'+'
#define ACS_LRCORNER	'+'
#define ACS_HLINE	'-'
#define ACS_VLINE	'|'
#define ACS_LARROW	'<'
#define ACS_RARROW	'>'
#define ACS_UARROW	'^'
#define ACS_DARROW	'v'

/* Special graphics characters for IBM displays. */
#define DISP_UL		0x250c
#define DISP_UR		0x2510
#define DISP_LL		0x2514
#define DISP_LR		0x2518
#define DISP_HORIZ	0x2500
#define DISP_VERT		0x2502
#define DISP_LEFT	0x2190		//0x1b
#define DISP_RIGHT	0x2192	//0x1a
#define DISP_UP		0x2191		//0x18
#define DISP_DOWN	0x2193		//0x19

/* Remap some libc-API-compatible function names so that we prevent
   circularararity. */
#ifndef WITHOUT_LIBC_STUBS
#define memmove grub_memmove
#define memcpy grub_memmove	/* we don't need a separate memcpy */
#define grub_memcpy grub_memmove
#define memset grub_memset
#define isspace grub_isspace
#define printf grub_printf
#define sprintf grub_sprintf
#undef putchar
#define putchar grub_putchar
#define strncat grub_strncat
#define strstr grub_strstr
#define memcmp grub_memcmp
#define strcmp grub_strcmp
#define tolower grub_tolower
#define strlen grub_strlen
#define strcpy grub_strcpy
#endif /* WITHOUT_LIBC_STUBS */

#define PXE_TFTP_MODE	1
#define PXE_FAST_READ	1

/* see typedef gfx_data_t below */
#define gfx_ofs_v1_ok			0x00
#define gfx_ofs_v1_mem_start		0x04
#define gfx_ofs_v1_mem_cur		0x08
#define gfx_ofs_v1_mem_max		0x0c
#define gfx_ofs_v1_code_seg		0x10
#define gfx_ofs_v1_jmp_table		0x14
#define gfx_ofs_v1_sys_cfg		0x44
#define gfx_ofs_v1_cmdline		0x64
#define gfx_ofs_v1_cmdline_len		0x68
#define gfx_ofs_v1_menu_list		0x6c
#define gfx_ofs_v1_menu_default_entry	0x70
#define gfx_ofs_v1_menu_entries		0x74
#define gfx_ofs_v1_menu_entry_len	0x78
#define gfx_ofs_v1_args_list		0x7c
#define gfx_ofs_v1_args_entry_len	0x80
#define gfx_ofs_v1_timeout		0x84
#define gfx_ofs_v1_mem_file		0x88
#define gfx_ofs_v1_mem_align		0x8c

#define gfx_ofs_v2_ok			0x00
#define gfx_ofs_v2_code_seg		0x04
#define gfx_ofs_v2_jmp_table		0x08
#define gfx_ofs_v2_sys_cfg		0x38
#define gfx_ofs_v2_cmdline		0x6c
#define gfx_ofs_v2_cmdline_len		0x70
#define gfx_ofs_v2_menu_list		0x74
#define gfx_ofs_v2_menu_default_entry	0x78
#define gfx_ofs_v2_menu_entries		0x7c
#define gfx_ofs_v2_menu_entry_len	0x80
#define gfx_ofs_v2_args_list		0x84
#define gfx_ofs_v2_args_entry_len	0x88
#define gfx_ofs_v2_timeout		0x8c

//================================================================================================================================
#ifndef ASM_FILE


#define	IMG(x)	((x) - 0x8200 + g4e_data)
//---------------------------------------------------------------------------------------------------
//grub/term.h
#define GRUB_TERM_NO_KEY        0

/* Internal codes used by GRUB to represent terminal input. GRUB用来表示终端输入的内部代码 */
/* Only for keys otherwise not having shifted modification. 仅用于键，否则没有移位修改  */
#define GRUB_TERM_SHIFT         0x01000000
#define GRUB_TERM_CTRL          0x02000000
#define GRUB_TERM_ALT           0x04000000

/* Keys without associated character. 没有关联字符的键 */
#define GRUB_TERM_EXTENDED      0x00800000    //扩展
#define GRUB_TERM_KEY_MASK      0x00ffffff    //多重移幅键控

#define GRUB_TERM_KEY_LEFT      (GRUB_TERM_EXTENDED | 0x4b)
#define GRUB_TERM_KEY_RIGHT     (GRUB_TERM_EXTENDED | 0x4d)
#define GRUB_TERM_KEY_UP        (GRUB_TERM_EXTENDED | 0x48)
#define GRUB_TERM_KEY_DOWN      (GRUB_TERM_EXTENDED | 0x50)
#define GRUB_TERM_KEY_HOME      (GRUB_TERM_EXTENDED | 0x47)
#define GRUB_TERM_KEY_END       (GRUB_TERM_EXTENDED | 0x4f)
#define GRUB_TERM_KEY_DC        (GRUB_TERM_EXTENDED | 0x53)
#define GRUB_TERM_KEY_PPAGE     (GRUB_TERM_EXTENDED | 0x49)
#define GRUB_TERM_KEY_NPAGE     (GRUB_TERM_EXTENDED | 0x51)
#define GRUB_TERM_KEY_F1        (GRUB_TERM_EXTENDED | 0x3b)
#define GRUB_TERM_KEY_F2        (GRUB_TERM_EXTENDED | 0x3c)
#define GRUB_TERM_KEY_F3        (GRUB_TERM_EXTENDED | 0x3d)
#define GRUB_TERM_KEY_F4        (GRUB_TERM_EXTENDED | 0x3e)
#define GRUB_TERM_KEY_F5        (GRUB_TERM_EXTENDED | 0x3f)
#define GRUB_TERM_KEY_F6        (GRUB_TERM_EXTENDED | 0x40)
#define GRUB_TERM_KEY_F7        (GRUB_TERM_EXTENDED | 0x41)
#define GRUB_TERM_KEY_F8        (GRUB_TERM_EXTENDED | 0x42)
#define GRUB_TERM_KEY_F9        (GRUB_TERM_EXTENDED | 0x43)
#define GRUB_TERM_KEY_F10       (GRUB_TERM_EXTENDED | 0x44)
#define GRUB_TERM_KEY_F11       (GRUB_TERM_EXTENDED | 0x57)
#define GRUB_TERM_KEY_F12       (GRUB_TERM_EXTENDED | 0x58)
#define GRUB_TERM_KEY_INSERT    (GRUB_TERM_EXTENDED | 0x52)
#define GRUB_TERM_KEY_CENTER    (GRUB_TERM_EXTENDED | 0x4c)

/* Hex value is used for ESC, since '\e' is nonstandard. 十六进制值用于ESC，因为“\e”是非标准的 */
#define GRUB_TERM_ESC		0x1b
#define GRUB_TERM_TAB		'\t'
#define GRUB_TERM_BACKSPACE	'\b'

#define GRUB_PROGRESS_NO_UPDATE -1  //没有进行更新
#define GRUB_PROGRESS_FAST      0   //进展快速
#define GRUB_PROGRESS_SLOW      2   //进展缓慢

//-----------------------------------------------------------------------------------------------------
//grub/types.h



//#if defined(__i386__)
//# define GRUB_CPU_SIZEOF_LONG		  4
//# define GRUB_CPU_SIZEOF_VOID_P		4
//# define GRUB_TARGET_SIZEOF_VOID_P  4
//#else
//# define GRUB_CPU_SIZEOF_LONG		  8
//# define GRUB_CPU_SIZEOF_VOID_P		8
//# define GRUB_TARGET_SIZEOF_VOID_P  8
//#endif

#define GRUB_CPU_WORDS_BIGENDIAN	0			//小端
#define GRUB_PACKED __attribute__ ((packed))					

typedef unsigned long long  grub_uint64_t;
typedef long long           grub_int64_t;
typedef int                 grub_int32_t;
typedef unsigned int        grub_uint32_t;
typedef long long           grub_s64_t;
typedef unsigned long long  grub_u64_t;

//#if GRUB_CPU_SIZEOF_LONG == 8			//8		x86_64
#if !defined(__i386__)
typedef grub_uint64_t				grub_size_t;
typedef grub_int64_t				grub_ssize_t;
#else
typedef grub_uint32_t				grub_size_t;
typedef grub_int32_t				grub_ssize_t;
#endif

typedef grub_size_t grub_efi_uintn_t;
typedef grub_ssize_t grub_efi_intn_t;
typedef grub_size_t grub_addr_t;


/* Define various wide integers.  定义各种宽度整数 */
typedef char            grub_int8_t;
typedef short						grub_int16_t;
typedef unsigned char		grub_uint8_t;
typedef unsigned short	grub_uint16_t;
typedef grub_uint64_t 	grub_properly_aligned_t;
typedef grub_uint64_t		grub_off_t;				//表示文件偏移量的类型
typedef grub_uint64_t		grub_disk_addr_t;	//表示磁盘块地址的类型


typedef unsigned char 	grub_u8_t;
typedef unsigned short 	grub_u16_t;
typedef unsigned int		grub_u32_t;
typedef char            grub_s8_t;
typedef short						grub_s16_t;
typedef int							grub_s32_t;
#define PACKED			__attribute__ ((packed))

typedef unsigned long long grub_efi_uint64_t;
typedef long long grub_efi_int64_t;
typedef unsigned int grub_efi_uint32_t;
typedef int grub_efi_int32_t;
/* Types. 类型 */
typedef char grub_efi_boolean_t;
//#if GRUB_CPU_SIZEOF_VOID_P == 8
//#if !defined(__i386__)
//typedef long long grub_efi_intn_t;
//typedef unsigned long long grub_efi_uintn_t;
//#else
//typedef int grub_efi_intn_t;
//typedef unsigned int grub_efi_uintn_t;
//#endif

typedef char grub_efi_int8_t;
typedef unsigned char grub_efi_uint8_t;
typedef short grub_efi_int16_t;
typedef unsigned short grub_efi_uint16_t;
typedef unsigned char grub_efi_char8_t;
typedef unsigned short grub_efi_char16_t;

struct grub_efi_guid
{
  grub_uint32_t data1;
  grub_uint16_t data2;
  grub_uint16_t data3;
  grub_uint8_t data4[8];
} __attribute__ ((aligned(8)));
typedef struct grub_efi_guid grub_efi_guid_t;

typedef grub_efi_intn_t grub_efi_status_t;


typedef grub_int8_t         BOOLEAN;
typedef grub_efi_intn_t     INTN;
typedef grub_efi_uintn_t    UINTN;
typedef grub_int8_t         INT8;
typedef grub_uint8_t        UINT8;
typedef grub_int16_t        INT16;
typedef grub_uint16_t       UINT16;
typedef int                 INT32;
typedef unsigned int        UINT32;
typedef grub_int64_t        INT64;
typedef grub_uint64_t       UINT64;
typedef grub_uint8_t        CHAR8;
typedef grub_uint16_t       CHAR16;
typedef void                VOID;
typedef grub_efi_guid_t     EFI_GUID;
typedef UINTN               EFI_STATUS;
typedef void*               EFI_HANDLE;
typedef void*               EFI_EVENT;
typedef UINT64              EFI_LBA;
typedef UINTN               EFI_TPL;
typedef unsigned int        EFI_MAC_ADDRESS;
typedef unsigned int        EFI_IPv4_ADDRESS;
typedef grub_uint16_t       EFI_IPv6_ADDRESS;
typedef grub_uint16_t       EFI_IP_ADDRESS;

#define INT128  char[128]
#define UINT128 unsigned char[128]
#define TRUE  1
#define FALSE 0

/*
IN        基准传递给函数。
OUT       从该函数返回原点。
OPTIONAL  将数据传递给函数是可选的，如果未提供该值，则可以传递NULL。
CONST     基准是只读的。
EFIAPI    定义UEFI接口的调用约定。
*/

//=============================================================================

# define PRIxGRUB_UINT32_T	"x"
# define PRIuGRUB_UINT32_T	"u"

//#if GRUB_CPU_SIZEOF_LONG == 8			//8		x86_64
#if !defined(__i386__)
# define PRIxGRUB_UINT64_T	"lx"
# define PRIuGRUB_UINT64_T	"lu"
#else
# define PRIxGRUB_UINT64_T	"llx"
# define PRIuGRUB_UINT64_T	"llu"
#endif

/* Misc types. 其他类型 */

//#if GRUB_CPU_SIZEOF_VOID_P == 8		//8		x86_64
#if !defined(__i386__)
# define GRUB_SIZE_MAX 18446744073709551615UL
//# if GRUB_CPU_SIZEOF_LONG == 8		//8		x86_64
#  define PRIxGRUB_SIZE	 "lx"
#  define PRIxGRUB_ADDR	 "lx"
#  define PRIuGRUB_SIZE	 "lu"
#  define PRIdGRUB_SSIZE "ld"
//# else
//#  define PRIxGRUB_SIZE	 "llx"
//#  define PRIxGRUB_ADDR	 "llx"
//#  define PRIuGRUB_SIZE  "llu"
//#  define PRIdGRUB_SSIZE "lld"
//# endif
#else
# define GRUB_SIZE_MAX 4294967295UL
# define PRIxGRUB_SIZE	"x"
# define PRIxGRUB_ADDR	"x"
# define PRIuGRUB_SIZE	"u"
# define PRIdGRUB_SSIZE	"d"
#endif

#define GRUB_UCHAR_MAX 0xFF
#define GRUB_USHRT_MAX 65535
#define GRUB_SHRT_MAX 0x7fff
#define GRUB_UINT_MAX 4294967295U
#define GRUB_INT_MAX 0x7fffffff
#define GRUB_INT32_MIN (-2147483647 - 1)
#define GRUB_INT32_MAX 2147483647

//#if GRUB_CPU_SIZEOF_LONG == 8			//8		x86_64
#if !defined(__i386__)
# define GRUB_ULONG_MAX 18446744073709551615UL
# define GRUB_LONG_MAX 9223372036854775807L
# define GRUB_LONG_MIN (-9223372036854775807L - 1)
#else
# define GRUB_ULONG_MAX 4294967295UL
# define GRUB_LONG_MAX 2147483647L
# define GRUB_LONG_MIN (-2147483647L - 1)
#endif



#define GRUB_PROPERLY_ALIGNED_ARRAY(name, size) grub_properly_aligned_t name[((size) + sizeof (grub_properly_aligned_t) - 1) / sizeof (grub_properly_aligned_t)]



/* Byte-orders.  */
static inline grub_uint16_t grub_swap_bytes16(grub_uint16_t _x)
{
   return (grub_uint16_t) ((_x << 8) | (_x >> 8));
}

#define grub_swap_bytes16_compile_time(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define grub_swap_bytes32_compile_time(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000UL) >> 24))
#define grub_swap_bytes64_compile_time(x)	\
({ \
   grub_uint64_t _x = (x); \
   (grub_uint64_t) ((_x << 56) \
                    | ((_x & (grub_uint64_t) 0xFF00ULL) << 40) \
                    | ((_x & (grub_uint64_t) 0xFF0000ULL) << 24) \
                    | ((_x & (grub_uint64_t) 0xFF000000ULL) << 8) \
                    | ((_x & (grub_uint64_t) 0xFF00000000ULL) >> 8) \
                    | ((_x & (grub_uint64_t) 0xFF0000000000ULL) >> 24) \
                    | ((_x & (grub_uint64_t) 0xFF000000000000ULL) >> 40) \
                    | (_x >> 56)); \
})

#if (defined(__GNUC__) && (__GNUC__ > 3) && (__GNUC__ > 4 || __GNUC_MINOR__ >= 3)) || defined(__clang__)
static inline grub_uint32_t grub_swap_bytes32(grub_uint32_t x)
{
	return __builtin_bswap32(x);
}

static inline grub_uint64_t grub_swap_bytes64(grub_uint64_t x)
{
	return __builtin_bswap64(x);
}
#else					/* not gcc 4.3 or newer */
static inline grub_uint32_t grub_swap_bytes32(grub_uint32_t _x)
{
   return ((_x << 24)
	   | ((_x & (grub_uint32_t) 0xFF00UL) << 8)
	   | ((_x & (grub_uint32_t) 0xFF0000UL) >> 8)
	   | (_x >> 24));
}

static inline grub_uint64_t grub_swap_bytes64(grub_uint64_t _x)
{
   return ((_x << 56)
	   | ((_x & (grub_uint64_t) 0xFF00ULL) << 40)
	   | ((_x & (grub_uint64_t) 0xFF0000ULL) << 24)
	   | ((_x & (grub_uint64_t) 0xFF000000ULL) << 8)
	   | ((_x & (grub_uint64_t) 0xFF00000000ULL) >> 8)
	   | ((_x & (grub_uint64_t) 0xFF0000000000ULL) >> 24)
	   | ((_x & (grub_uint64_t) 0xFF000000000000ULL) >> 40)
	   | (_x >> 56));
}
#endif					/* not gcc 4.3 or newer */

//#ifdef GRUB_CPU_WORDS_BIGENDIAN				//大端
#if  GRUB_CPU_WORDS_BIGENDIAN				//大端
# define grub_cpu_to_le16(x)	grub_swap_bytes16(x)
# define grub_cpu_to_le32(x)	grub_swap_bytes32(x)
# define grub_cpu_to_le64(x)	grub_swap_bytes64(x)
# define grub_le_to_cpu16(x)	grub_swap_bytes16(x)
# define grub_le_to_cpu32(x)	grub_swap_bytes32(x)
# define grub_le_to_cpu64(x)	grub_swap_bytes64(x)
# define grub_cpu_to_be16(x)	((grub_uint16_t) (x))
# define grub_cpu_to_be32(x)	((grub_uint32_t) (x))
# define grub_cpu_to_be64(x)	((grub_uint64_t) (x))
# define grub_be_to_cpu16(x)	((grub_uint16_t) (x))
# define grub_be_to_cpu32(x)	((grub_uint32_t) (x))
# define grub_be_to_cpu64(x)	((grub_uint64_t) (x))
# define grub_cpu_to_be16_compile_time(x)	((grub_uint16_t) (x))
# define grub_cpu_to_be32_compile_time(x)	((grub_uint32_t) (x))
# define grub_cpu_to_be64_compile_time(x)	((grub_uint64_t) (x))
# define grub_be_to_cpu64_compile_time(x)	((grub_uint64_t) (x))
# define grub_cpu_to_le32_compile_time(x)	grub_swap_bytes32_compile_time(x)
# define grub_cpu_to_le64_compile_time(x)	grub_swap_bytes64_compile_time(x)
# define grub_cpu_to_le16_compile_time(x)	grub_swap_bytes16_compile_time(x)
#else /* ! WORDS_BIGENDIAN */					//小端
# define grub_cpu_to_le16(x)	((grub_uint16_t) (x))
# define grub_cpu_to_le32(x)	((grub_uint32_t) (x))
# define grub_cpu_to_le64(x)	((grub_uint64_t) (x))
# define grub_le_to_cpu16(x)	((grub_uint16_t) (x))
# define grub_le_to_cpu32(x)	((grub_uint32_t) (x))
# define grub_le_to_cpu64(x)	((grub_uint64_t) (x))
# define grub_cpu_to_be16(x)	grub_swap_bytes16(x)
# define grub_cpu_to_be32(x)	grub_swap_bytes32(x)
# define grub_cpu_to_be64(x)	grub_swap_bytes64(x)
# define grub_be_to_cpu16(x)	grub_swap_bytes16(x)
# define grub_be_to_cpu32(x)	grub_swap_bytes32(x)
# define grub_be_to_cpu64(x)	grub_swap_bytes64(x)
# define grub_cpu_to_be16_compile_time(x)	grub_swap_bytes16_compile_time(x)
# define grub_cpu_to_be32_compile_time(x)	grub_swap_bytes32_compile_time(x)
# define grub_cpu_to_be64_compile_time(x)	grub_swap_bytes64_compile_time(x)
# define grub_be_to_cpu64_compile_time(x)	grub_swap_bytes64_compile_time(x)
# define grub_cpu_to_le16_compile_time(x)	((grub_uint16_t) (x))
# define grub_cpu_to_le32_compile_time(x)	((grub_uint32_t) (x))
# define grub_cpu_to_le64_compile_time(x)	((grub_uint64_t) (x))
#endif /* ! WORDS_BIGENDIAN */

struct grub_unaligned_uint16
{
  grub_uint16_t val;
} GRUB_PACKED;
struct grub_unaligned_uint32
{
  grub_uint32_t val;
} GRUB_PACKED;
struct grub_unaligned_uint64
{
  grub_uint64_t val;
} GRUB_PACKED;

typedef struct grub_unaligned_uint16 grub_unaligned_uint16_t;
typedef struct grub_unaligned_uint32 grub_unaligned_uint32_t;
typedef struct grub_unaligned_uint64 grub_unaligned_uint64_t;

static inline grub_uint16_t grub_get_unaligned16 (const void *ptr)
{
  const struct grub_unaligned_uint16 *dd
    = (const struct grub_unaligned_uint16 *) ptr;
  return dd->val;
}

static inline void grub_set_unaligned16 (void *ptr, grub_uint16_t val)
{
  struct grub_unaligned_uint16 *dd = (struct grub_unaligned_uint16 *) ptr;
  dd->val = val;
}

static inline grub_uint32_t grub_get_unaligned32 (const void *ptr)
{
  const struct grub_unaligned_uint32 *dd
    = (const struct grub_unaligned_uint32 *) ptr;
  return dd->val;
}

static inline void grub_set_unaligned32 (void *ptr, grub_uint32_t val)
{
  struct grub_unaligned_uint32 *dd = (struct grub_unaligned_uint32 *) ptr;
  dd->val = val;
}

static inline grub_uint64_t grub_get_unaligned64 (const void *ptr)
{
  const struct grub_unaligned_uint64 *dd
    = (const struct grub_unaligned_uint64 *) ptr;
  return dd->val;
}

static inline void grub_set_unaligned64 (void *ptr, grub_uint64_t val)
{
  struct grub_unaligned_uint64_t
  {
    grub_uint64_t d;
  } GRUB_PACKED;
  struct grub_unaligned_uint64_t *dd = (struct grub_unaligned_uint64_t *) ptr;
  dd->d = val;
}

//-------------------------------------------------------------------------------------------------
//UEFI 函数调用约定    只支持 GCC4.7及以上版本
#if !defined(__i386__)
#if (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)))||(defined(__clang__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 2)))
  #define EFIAPI __attribute__((ms_abi))
#else
  #error Compiler is too old for GNU_EFI_USE_MS_ABI
#endif
#endif

#ifndef EFIAPI
  #define EFIAPI  // Substitute expresion to force C calling convention 
#endif

#define GRUB_CHAR_BIT 8
//-----------------------------------------------------------------------------------------------
//#include <stdarg.h> 可变参数定义
#ifndef _VA_LIST
typedef __builtin_va_list va_list;
#define _VA_LIST
#endif
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap)          __builtin_va_end(ap)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)
 
/* GCC always defines __va_copy, but does not define va_copy unless in c99 mode
 * or -ansi is not specified, since it was not part of C90.
 */
#define __va_copy(d,s) __builtin_va_copy(d,s)
#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST 1
typedef __builtin_va_list __gnuc_va_list;
#endif

//----------------------------------------------------------------------------------------------

/*
 *  Below this should be ONLY defines and other constructs for C code.
 */

/* function prototypes for asm functions */
unsigned char * graphics_get_font();
extern unsigned long long color_8_to_64 (unsigned char color8);
extern unsigned long long color_4_to_32 (unsigned char color4);
extern unsigned char color_64_to_8 (unsigned long long color64);
extern unsigned char color_32_to_4 (unsigned int color32);
extern unsigned int current_color;
extern unsigned long long current_color_64bit;
extern unsigned int cursor_state;
extern unsigned int OnCommandLine;
extern unsigned int graphics_mode;
extern unsigned int font_w;
extern unsigned int font_h;
extern unsigned char num_wide;
extern unsigned int font_spacing;
extern unsigned int line_spacing;
extern void rectangle(int left, int top, int length, int width, int line);
extern int hex (int v);
extern unsigned int splashimage_loaded;
extern unsigned int X_offset,Y_offset;
extern int console_color[];
extern unsigned long long console_color_64bit[];
extern unsigned int console_print_unicode (unsigned int unicode, unsigned int max_width);
extern void console_putstr_utf8 (char *str);
extern void console_putstr_utf16(unsigned short *str);
extern unsigned int is_highlight;
extern unsigned int graphics_inited;
struct box
{
	unsigned char enable;
	unsigned short start_x;
	unsigned short start_y;
	unsigned short horiz;
	unsigned short vert;
	unsigned char linewidth;
	unsigned int color;
} __attribute__ ((packed));
extern struct box DrawBox[16];
struct string
{
	unsigned char enable;
	unsigned char start_x;
	char start_y;
	unsigned long long color;
	char string[101];
} __attribute__ ((packed));

extern struct string* strings;
extern unsigned char DateTime_enable;
extern void DateTime_refresh(void);
/* The Chinese patch will begin at here */

/* multiboot stuff */

#include "mb_header.h"
#include "mb_info.h"

/* For the Linux/i386 boot protocol version 2.03.  */
struct linux_kernel_header
{
  char code1[0x0020];
  unsigned short cl_magic;		/* Magic number 0xA33F */
  unsigned short cl_offset;		/* The offset of command line */
  char code2[0x01F1 - 0x0020 - 2 - 2];
  unsigned char setup_sects;		/* The size of the setup in sectors */
  unsigned short root_flags;		/* If the root is mounted readonly */
  unsigned short syssize;		/* obsolete */
  unsigned short swap_dev;		/* obsolete */
  unsigned short ram_size;		/* obsolete */
  unsigned short vid_mode;		/* Video mode control */
  unsigned short root_dev;		/* Default root device number */
  unsigned short boot_flag;		/* 0xAA55 magic number */
  unsigned short jump;			/* Jump instruction */
  unsigned int header;			/* Magic signature "HdrS" */
  unsigned short version;		/* Boot protocol version supported */
  unsigned int realmode_swtch;		/* Boot loader hook */
  unsigned int start_sys;		/* Points to kernel version string */
  unsigned char type_of_loader;		/* Boot loader identifier */
  unsigned char loadflags;		/* Boot protocol option flags */
  unsigned short setup_move_size;	/* Move to high memory size */
  unsigned int code32_start;		/* Boot loader hook */
  unsigned int ramdisk_image;		/* initrd load address */
  unsigned int ramdisk_size;		/* initrd size */
  unsigned int bootsect_kludge;	/* obsolete */
  unsigned short heap_end_ptr;		/* Free memory after setup end */
  unsigned short pad1;			/* Unused */
  unsigned int cmd_line_ptr;			/* Points to the kernel command line */
  unsigned int initrd_addr_max;	/* The highest address of initrd */
  unsigned int kernel_alignment;
  unsigned char relocatable;
  unsigned char min_alignment;
#define LINUX_XLF_KERNEL_64                   (1<<0)
#define LINUX_XLF_CAN_BE_LOADED_ABOVE_4G      (1<<1)
#define LINUX_XLF_EFI_HANDOVER_32             (1<<2)
#define LINUX_XLF_EFI_HANDOVER_64             (1<<3)
#define LINUX_XLF_EFI_KEXEC                   (1<<4)
  unsigned short xloadflags;
  unsigned int cmdline_size;
  unsigned int hardware_subarch;
  unsigned long long hardware_subarch_data;
  unsigned int payload_offset;
  unsigned int payload_length;
  unsigned long long setup_data;
  unsigned long long pref_address;
  unsigned int init_size;
  unsigned int handover_offset;
} __attribute__ ((packed));

#define GRUB_E820_RAM        1
#define GRUB_E820_RESERVED   2
#define GRUB_E820_ACPI       3
#define GRUB_E820_NVS        4
#define GRUB_E820_BADRAM     5

struct grub_e820_mmap
{
  unsigned long long addr;
  unsigned long long size;
  unsigned int type;
} __attribute__ ((unused));

#define LINUX_IMAGE "BOOT_IMAGE="

/* Maximum number of MBR signatures to store. */
#define EDD_MBR_SIG_MAX			16

#define GRUB_LINUX_I386_MAGIC_SIGNATURE	0x53726448      /* "HdrS" */
#define GRUB_LINUX_DEFAULT_SETUP_SECTS	4
#define GRUB_LINUX_INITRD_MAX_ADDRESS	0x37FFFFFF
#define GRUB_LINUX_MAX_SETUP_SECTS	64
#define GRUB_LINUX_BOOT_LOADER_TYPE	0x72
#define GRUB_LINUX_HEAP_END_OFFSET	(0x9000 - 0x200)

/* Boot parameters for Linux based on 2.6.12. This is used by the setup
   sectors of Linux, and must be simulated by GRUB on EFI, because
   the setup sectors depend on BIOS.  */
struct linux_kernel_params
{
  unsigned char video_cursor_x;		/* 0 */
  unsigned char video_cursor_y;

  unsigned short ext_mem;		/* 2 */

  unsigned short video_page;		/* 4 */
  unsigned char video_mode;		/* 6 */
  unsigned char video_width;		/* 7 */

  unsigned char padding1[0xa - 0x8];

  unsigned short video_ega_bx;		/* a */

  unsigned char padding2[0xe - 0xc];

  unsigned char video_height;		/* e */
  unsigned char have_vga;		/* f */
  unsigned short font_size;		/* 10 */

  unsigned short lfb_width;		/* 12 */
  unsigned short lfb_height;		/* 14 */
  unsigned short lfb_depth;		/* 16 */
  unsigned int lfb_base;		/* 18 */
  unsigned int lfb_size;		/* 1c */

  unsigned short cl_magic;		/* 20 */
  unsigned short cl_offset;

  unsigned short lfb_line_len;		/* 24 */
  unsigned char red_mask_size;		/* 26 */
  unsigned char red_field_pos;
  unsigned char green_mask_size;
  unsigned char green_field_pos;
  unsigned char blue_mask_size;
  unsigned char blue_field_pos;
  unsigned char reserved_mask_size;
  unsigned char reserved_field_pos;
  unsigned short vesapm_segment;		/* 2e */
  unsigned short vesapm_offset;		/* 30 */
  unsigned short lfb_pages;		/* 32 */
  unsigned short vesa_attrib;		/* 34 */
  unsigned int capabilities;		/* 36 */
  unsigned int ext_lfb_base;		/* 3a */

  unsigned char padding3[0x40 - 0x3e];

  unsigned short apm_version;		/* 40 */
  unsigned short apm_code_segment;	/* 42 */
  unsigned int apm_entry;		/* 44 */
  unsigned short apm_16bit_code_segment;	/* 48 */
  unsigned short apm_data_segment;	/* 4a */
  unsigned short apm_flags;		/* 4c */
  unsigned int apm_code_len;		/* 4e */
  unsigned short apm_data_len;		/* 52 */

  unsigned char padding4[0x60 - 0x54];

  unsigned int ist_signature;		/* 60 */
  unsigned int ist_command;		/* 64 */
  unsigned int ist_event;		/* 68 */
  unsigned int ist_perf_level;		/* 6c */
  unsigned long long acpi_rsdp_addr;		/* 70 */

  unsigned char padding5[0x80 - 0x78];

  unsigned char hd0_drive_info[0x10];	/* 80 */
  unsigned char hd1_drive_info[0x10];	/* 90 */
  unsigned short rom_config_len;		/* a0 */

  unsigned char padding6[0xb0 - 0xa2];

  unsigned int ofw_signature;		/* b0 */
  unsigned int ofw_num_items;		/* b4 */
  unsigned int ofw_cif_handler;	/* b8 */
  unsigned int ofw_idt;		/* bc */

  unsigned char padding7[0x1b8 - 0xc0];

  union
    {
      struct
        {
          unsigned int efi_system_table;	/* 1b8 */
          unsigned int padding7_1;		/* 1bc */
          unsigned int efi_signature;		/* 1c0 */
          unsigned int efi_mem_desc_size;	/* 1c4 */
          unsigned int efi_mem_desc_version;	/* 1c8 */
          unsigned int efi_mmap_size;		/* 1cc */
          unsigned int efi_mmap;		/* 1d0 */
        } v0204;
      struct
        {
          unsigned int padding7_1;		/* 1b8 */
          unsigned int padding7_2;		/* 1bc */
          unsigned int efi_signature;		/* 1c0 */
          unsigned int efi_system_table;	/* 1c4 */
          unsigned int efi_mem_desc_size;	/* 1c8 */
          unsigned int efi_mem_desc_version;	/* 1cc */
          unsigned int efi_mmap;		/* 1d0 */
          unsigned int efi_mmap_size;		/* 1d4 */
	} v0206;
      struct
        {
          unsigned int padding7_1;		/* 1b8 */
          unsigned int padding7_2;		/* 1bc */
          unsigned int efi_signature;		/* 1c0 */
          unsigned int efi_system_table;	/* 1c4 */
          unsigned int efi_mem_desc_size;	/* 1c8 */
          unsigned int efi_mem_desc_version;	/* 1cc */
          unsigned int efi_mmap;		/* 1d0 */
          unsigned int efi_mmap_size;		/* 1d4 */
          unsigned int efi_system_table_hi;	/* 1d8 */
          unsigned int efi_mmap_hi;		/* 1dc */
        } v0208;
    };

  unsigned int alt_mem;		/* 1e0 */

  unsigned char padding8[0x1e8 - 0x1e4];

  unsigned char mmap_size;		/* 1e8 */

  unsigned char padding9[0x1f1 - 0x1e9];

  /* Linux setup header copy - BEGIN. */
  unsigned char setup_sects;		/* The size of the setup in sectors */
  unsigned short root_flags;		/* If the root is mounted readonly */
  unsigned short syssize;		/* obsolete */
  unsigned short swap_dev;		/* obsolete */
  unsigned short ram_size;		/* obsolete */
  unsigned short vid_mode;		/* Video mode control */
  unsigned short root_dev;		/* Default root device number */

  unsigned char padding10;		/* 1fe */
  unsigned char ps_mouse;		/* 1ff */

  unsigned short jump;			/* Jump instruction */
  unsigned int header;			/* Magic signature "HdrS" */
  unsigned short version;		/* Boot protocol version supported */
  unsigned int realmode_swtch;		/* Boot loader hook */
  unsigned short start_sys;		/* The load-low segment (obsolete) */
  unsigned short kernel_version;		/* Points to kernel version string */
  unsigned char type_of_loader;		/* Boot loader identifier */
  unsigned char loadflags;		/* Boot protocol option flags */
  unsigned short setup_move_size;	/* Move to high memory size */
  unsigned int code32_start;		/* Boot loader hook */
  unsigned int ramdisk_image;		/* initrd load address */
  unsigned int ramdisk_size;		/* initrd size */
  unsigned int bootsect_kludge;	/* obsolete */
  unsigned short heap_end_ptr;		/* Free memory after setup end */
  unsigned char ext_loader_ver;		/* Extended loader version */
  unsigned char ext_loader_type;		/* Extended loader type */  
  unsigned int cmd_line_ptr;		/* Points to the kernel command line */
  unsigned int initrd_addr_max;	/* Maximum initrd address */
  unsigned int kernel_alignment;	/* Alignment of the kernel */
  unsigned char relocatable_kernel;	/* Is the kernel relocatable */
  unsigned char pad1[3];
  unsigned int cmdline_size;		/* Size of the kernel command line */
  unsigned int hardware_subarch;
  unsigned long long hardware_subarch_data;
  unsigned int payload_offset;
  unsigned int payload_length;
  unsigned long long setup_data;
  unsigned long long pref_address;
  unsigned int init_size;
  unsigned int handover_offset;
  /* Linux setup header copy - END. */

  unsigned char _pad7[40];
  unsigned int edd_mbr_sig_buffer[EDD_MBR_SIG_MAX];	/* 290 */
  struct grub_e820_mmap e820_map[(0x400 - 0x2d0) / 20];	/* 2d0 */
} __attribute__ ((packed));

/* Memory map address range descriptor used by GET_MMAP_ENTRY. */
struct mmar_desc
{
  unsigned int desc_len;	/* Size of this descriptor. */
  unsigned long long addr;	/* Base address. */
  unsigned long long length;	/* Length in bytes. */
  unsigned int type;		/* Type of address range. */
} __attribute__ ((packed));

/* VBE controller information.  */
struct vbe_controller
{
  unsigned char signature[4];
  unsigned short version;
  unsigned int oem_string;
  unsigned int capabilities;
  unsigned int video_mode;
  unsigned short total_memory;
  unsigned short oem_software_rev;
  unsigned int oem_vendor_name;
  unsigned int oem_product_name;
  unsigned int oem_product_rev;
  unsigned char reserved[222];
  unsigned char oem_data[256];
} __attribute__ ((packed));

/* VBE mode information.  */
struct vbe_mode
{
  unsigned short mode_attributes;
  unsigned char win_a_attributes;
  unsigned char win_b_attributes;
  unsigned short win_granularity;
  unsigned short win_size;
  unsigned short win_a_segment;
  unsigned short win_b_segment;
  unsigned int win_func;
  unsigned short bytes_per_scanline;

  /* >=1.2 */
  unsigned short x_resolution;
  unsigned short y_resolution;
  unsigned char x_char_size;
  unsigned char y_char_size;
  unsigned char number_of_planes;
  unsigned char bits_per_pixel;
  unsigned char number_of_banks;
  unsigned char memory_model;
  unsigned char bank_size;
  unsigned char number_of_image_pages;
  unsigned char reserved0;

  /* direct color */
  unsigned char red_mask_size;
  unsigned char red_field_position;
  unsigned char green_mask_size;
  unsigned char green_field_position;
  unsigned char blue_mask_size;
  unsigned char blue_field_position;
  unsigned char reserved_mask_size;
  unsigned char reserved_field_position;
  unsigned char direct_color_mode_info;

  /* >=2.0 */
  unsigned int phys_base;
  unsigned int reserved1;
  unsigned short reversed2;

  /* >=3.0 */
  unsigned short linear_bytes_per_scanline;
  unsigned char banked_number_of_image_pages;
  unsigned char linear_number_of_image_pages;
  unsigned char linear_red_mask_size;
  unsigned char linear_red_field_position;
  unsigned char linear_green_mask_size;
  unsigned char linear_green_field_position;
  unsigned char linear_blue_mask_size;
  unsigned char linear_blue_field_position;
  unsigned char linear_reserved_mask_size;
  unsigned char linear_reserved_field_position;
  unsigned int max_pixel_clock;

  unsigned char reserved3[189];
} __attribute__ ((packed));

extern int graphicsmode_func (char *arg, int flags);
extern char splashimage[128];

#undef NULL
#define NULL         ((void *) 0)

/* Error codes (descriptions are in common.c) */
typedef enum
{
  ERR_NONE = 0,
  ERR_BAD_FILENAME,
  ERR_BAD_FILETYPE,
  ERR_BAD_GZIP_DATA,
  ERR_BAD_GZIP_HEADER,
  ERR_BAD_PART_TABLE,
  ERR_BAD_VERSION,
  ERR_BELOW_1MB,
  ERR_BOOT_COMMAND,
  ERR_BOOT_FAILURE,
  ERR_BOOT_FEATURES,
  ERR_DEV_FORMAT,
  ERR_DEV_VALUES,
  ERR_EXEC_FORMAT,
  ERR_FILELENGTH,
  ERR_FILE_NOT_FOUND,
  ERR_FSYS_CORRUPT,
  ERR_FSYS_MOUNT,
  ERR_GEOM,
  ERR_NEED_LX_KERNEL,
  ERR_NEED_MB_KERNEL,
  ERR_NO_DISK,
  ERR_NO_PART,
  ERR_NUMBER_PARSING,
  ERR_OUTSIDE_PART,
  ERR_READ,
  ERR_SYMLINK_LOOP,
  ERR_UNRECOGNIZED,
  ERR_WONT_FIT,
  ERR_WRITE,
  ERR_BAD_ARGUMENT,
  ERR_UNALIGNED,
  ERR_PRIVILEGED,
  ERR_DEV_NEED_INIT,
  ERR_NO_DISK_SPACE,
  ERR_NUMBER_OVERFLOW,

  ERR_DEFAULT_FILE,
  ERR_DEL_MEM_DRIVE,
  ERR_DISABLE_A20,
  ERR_DOS_BACKUP,
  ERR_ENABLE_A20,
  ERR_EXTENDED_PARTITION,
  ERR_FILENAME_FORMAT,
  ERR_HD_VOL_START_0,
  ERR_INT13_ON_HOOK,
  ERR_INT13_OFF_HOOK,
  ERR_INVALID_BOOT_CS,
  ERR_INVALID_BOOT_IP,
  ERR_INVALID_FLOPPIES,
  ERR_INVALID_HARDDRIVES,
  ERR_INVALID_HEADS,
  ERR_INVALID_LOAD_LENGTH,
  ERR_INVALID_LOAD_OFFSET,
  ERR_INVALID_LOAD_SEGMENT,
  ERR_INVALID_SECTORS,
  ERR_INVALID_SKIP_LENGTH,
  ERR_INVALID_RAM_DRIVE,
  ERR_IN_SITU_FLOPPY,
  ERR_IN_SITU_MEM,
  ERR_MD_BASE,
  ERR_NON_CONTIGUOUS,
	ERR_MANY_FRAGMENTS,
  ERR_NO_DRIVE_MAPPED,
  ERR_NO_HEADS,
  ERR_NO_SECTORS,
  ERR_PARTITION_TABLE_FULL,
  ERR_RD_BASE,
  ERR_SPECIFY_GEOM,
  ERR_SPECIFY_MEM,
  ERR_SPECIFY_RESTRICTION,
//  ERR_INVALID_RD_BASE,
//  ERR_INVALID_RD_SIZE,
  ERR_MD5_FORMAT,
  ERR_WRITE_GZIP_FILE,
  ERR_FUNC_CALL,
//  ERR_WRITE_TO_NON_MEM_DRIVE,
  ERR_INTERNAL_CHECK,
  ERR_KERNEL_WITH_PROGRAM,
  ERR_HALT,
  ERR_PARTITION_LOOP,
  ERR_NOT_ENOUGH_MEMORY,
  ERR_NO_VBE_BIOS,
  ERR_BAD_VBE_SIGNATURE,
  ERR_LOW_VBE_VERSION,
  ERR_NO_VBE_MODES,
  ERR_SET_VBE_MODE,
  ERR_SET_VGA_MODE,
  ERR_LOAD_SPLASHIMAGE,
  ERR_UNIFONT_FORMAT,
//  ERR_UNIFONT_RELOAD,
  ERR_DIVISION_BY_ZERO,

  MAX_ERR_NUM,

  /* these are for batch scripts and must be > MAX_ERR_NUM */
  ERR_BAT_GOTO,
  ERR_BAT_CALL,
  ERR_BAT_BRACE_END, 
} grub_error_t;	//4

extern char *get_next_arg(char *arg);
extern int brace;
struct border {
	unsigned char disp_ul;
	unsigned char disp_ur;
	unsigned char disp_ll;
	unsigned char disp_lr;
	unsigned char disp_horiz;
	unsigned char disp_vert;
	unsigned char menu_box_x; /* line start */
	unsigned char menu_box_w; /* line width */
	unsigned char menu_box_y; /* first line number */
	unsigned char menu_box_h;
	unsigned char menu_box_b;
	unsigned char border_w;
	unsigned char menu_help_x;
	unsigned char menu_help_w;
	unsigned char menu_keyhelp_y_offset;
} __attribute__ ((packed));

extern struct border menu_border;
extern unsigned int fontx;
extern unsigned int fonty;
extern unsigned int install_partition;
extern unsigned int boot_drive;
extern unsigned int boot_part_addr;
extern int saved_entryno;
extern unsigned char force_lba;
extern char version_string[];
extern char config_file[];
extern unsigned int linux_text_len;
extern char *linux_data_tmp_addr;
extern char *linux_data_real_addr;
extern char *linux_bzimage_tmp_addr;
extern int quit_print;
extern struct linux_kernel_header *linux_header;

extern unsigned char menu_tab;
extern unsigned char num_string;
extern unsigned char menu_font_spacing;
extern unsigned char menu_line_spacing;
extern int password_x;
extern unsigned char timeout_x;
extern unsigned char timeout_y;
extern unsigned long long timeout_color;
extern unsigned long long keyhelp_color;
extern unsigned char graphic_enable;
extern unsigned char graphic_type;
extern unsigned char graphic_row;
extern unsigned char graphic_list;
extern unsigned short graphic_wide;
extern unsigned short graphic_high;
extern unsigned short row_space;
extern char graphic_file[128];
extern void clear_entry (int x, int y, int w, int h);
extern void vbe_fill_color (unsigned int color);
//extern int (*hotkey_func)(char *titles,int flags,int flags1);
extern int (*hotkey_func)(char *titles,int flags,int flags1,int key); //外置热键
extern unsigned long long hotkey_color_64bit;
extern unsigned int hotkey_color;
extern int font_func (char *arg, int flags);
extern char embed_font_path[64];
extern char *embed_font;

#ifdef SUPPORT_GRAPHICS
extern unsigned int current_x_resolution;
extern unsigned int current_y_resolution;
extern unsigned int current_bits_per_pixel;
extern unsigned int current_bytes_per_scanline;
extern unsigned int current_bytes_per_pixel;
extern unsigned long long current_phys_base;
extern unsigned int fill_color;
extern unsigned char animated_enable;
extern unsigned char animated_enable_backup;
extern unsigned char animated_type;
extern unsigned short animated_delay;
extern unsigned char animated_last_num;
extern unsigned short animated_offset_x;
extern unsigned short animated_offset_y;
extern char animated_name[128];
extern int animated (void);
extern int splashimage_func(char *arg, int flags);
extern int background_transparent;
extern int use_phys_base;
#endif

struct mem_alloc_array
{
  unsigned int addr;
  unsigned int pid;
};

struct malloc_array
{
  unsigned int addr;
  struct malloc_array *next;
};

extern void *grub_malloc(grub_size_t size);
extern void *grub_zalloc(grub_size_t size);
extern void * grub_memalign (grub_size_t align, grub_size_t size);
extern void grub_free(void *ptr);
struct malloc_array *malloc_array_start;

/* If not using config file, this variable is set to zero,
   otherwise non-zero.  */
extern int use_config_file;
/* print debug message on startup if the DEBUG_KEY is pressed. */
//extern int debug_boot;
extern int console_beep (void);
//extern int beep_func(char *arg, int flags);
extern void defer(unsigned short millisecond);
//extern unsigned short count_ms;
//extern unsigned char beep_play;
//extern unsigned char beep_enable;
//extern unsigned short beep_frequency;
extern unsigned short beep_duration;
extern unsigned long long initrd_start_sector;
extern int map_func (char *arg, int flags);
//#define DEBUG_SLEEP {debug_sleep(debug_boot,__LINE__,__FILE__);}
//extern inline void debug_sleep(int debug_boot, int line, char *file);

#ifdef DEBUG_TIME
#define PRINT_DEBUG_INFO debug_time(__LINE__,__FILE__);
extern inline void debug_time(const int line,const char*file);
#endif

extern void hexdump(grub_u64_t,char*,int);
extern int builtin_cmd (char *cmd, char *arg, int flags);
extern int realmode_run(int regs_ptr);

#define MAX_USER_VARS 60
#define MAX_VARS 64
#define MAX_VAR_LEN	8
#define MAX_ENV_LEN	512
#define MAX_BUFFER	(MAX_VARS * (MAX_VAR_LEN + MAX_ENV_LEN))
typedef char VAR_NAME[MAX_VAR_LEN];
typedef char VAR_VALUE[MAX_ENV_LEN];
#define VAR ((VAR_NAME *)BASE_ADDR)
#define ENVI ((VAR_VALUE *)(BASE_ADDR + MAX_VARS * MAX_VAR_LEN))
#define _WENV_ 60

//#define VAR_EX_TMP ((char *)(BASE_ADDR+MAX_VARS * (MAX_VAR_LEN + MAX_ENV_LEN)))
/* 变量 VAR (BASE_ADDR)  需0x8200字节
 * 0x0      变量名称  8字节     64个变量
 * 0x200    变量值    512字节   64个变量值
 */
#define set_envi(var, val)			envi_cmd(var, val, 0)
#define get_env_all()				envi_cmd(NULL, NULL, 2)
#define reset_env_all()				envi_cmd(NULL, NULL, 3)
extern int envi_cmd(const char *var,char * const env,int flags);


/* GUI interface variables. */
# define MAX_FALLBACK_ENTRIES	8
extern int fallback_entries[MAX_FALLBACK_ENTRIES];
extern int fallback_entryno;
extern int default_entry;
extern int current_entryno;

/*
 * graphics menu stuff
 *
 * Note: gfx_data and all data referred to in it must lie within a 64k area.
 */
typedef struct
{
  unsigned ok;			/* set while we're in graphics mode */
  unsigned mem_start, mem_cur, mem_max;
  unsigned code_seg;		/* code segment of binary graphics code */
  unsigned jmp_table[12];	/* link to graphics functions */
  unsigned char sys_cfg[32];	/* sys_cfg[0]: identifies boot loader (grub == 2) */
  char *cmdline;		/* command line returned by gfx_input() */
  unsigned cmdline_len;		/* length of the above */
  char *menu_list;		/* list of menu entries, each of fixed length (menu_entry_len) */
  char *menu_default_entry;	/* the default entry */
  unsigned menu_entries;	/* number of entries in menu_list */
  unsigned menu_entry_len;	/* one entry */
  char *args_list;		/* same structure as menu_list, menu_entries entries */
  unsigned args_entry_len;	/* one entry */
  unsigned timeout;		/* in seconds (0: no timeout) */
  unsigned mem_file;		/* aligned gfx file start */
  unsigned mem_align;		/* aligned cpio file start */
} __attribute__ ((packed)) gfx_data_v1_t;

typedef struct
{
  unsigned ok;			/* set while we're in graphics mode */
  unsigned code_seg;		/* code segment of binary graphics code */
  unsigned jmp_table[12];	/* link to graphics functions */
  unsigned char sys_cfg[52];	/* sys_cfg[0]: identifies boot loader (grub == 2) */
  char *cmdline;		/* command line returned by gfx_input() */
  unsigned cmdline_len;		/* length of the above */
  char *menu_list;		/* list of menu entries, each of fixed length (menu_entry_len) */
  char *menu_default_entry;	/* the default entry */
  unsigned menu_entries;	/* number of entries in menu_list */
  unsigned menu_entry_len;	/* one entry */
  char *args_list;		/* same structure as menu_list, menu_entries entries */
  unsigned args_entry_len;	/* one entry */
  unsigned timeout;		/* in seconds (0: no timeout) */
} __attribute__ ((packed)) gfx_data_v2_t;

#ifdef SUPPORT_GFX
/* pointer to graphics image data */
extern char graphics_file[64];
extern unsigned int gfx_drive, gfx_partition;

int gfx_init_v1(gfx_data_v1_t *gfx_data);
int gfx_done_v1(gfx_data_v1_t *gfx_data);
int gfx_input_v1(gfx_data_v1_t *gfx_data, int *menu_entry);
int gfx_setup_menu_v1(gfx_data_v1_t *gfx_data);

int gfx_init_v2(gfx_data_v2_t *gfx_data);
int gfx_done_v2(gfx_data_v2_t *gfx_data);
int gfx_input_v2(gfx_data_v2_t *gfx_data, int *menu_entry);
int gfx_setup_menu_v2(gfx_data_v2_t *gfx_data);
#endif

/* The constants for password types.  */
typedef enum
{
  PASSWORD_PLAIN,
  PASSWORD_MD5,
  PASSWORD_UNSUPPORTED
}
password_t;

extern char *password_buf;
extern password_t password_type;
extern int auth;
extern char commands[];

/* For `more'-like feature.  */
extern int count_lines;
extern int use_pager;

#ifndef NO_DECOMPRESSION
extern int no_decompression;
extern int compressed_file;
#endif

/* instrumentation variables */
extern void (*disk_read_hook) (unsigned long long, unsigned int, unsigned long long);
extern void (*disk_read_func) (unsigned long long, unsigned int, unsigned long long);

/* The flag for debug mode.  */
extern int debug;
extern int debug_bat;
extern grub_u8_t debug_msg;

extern unsigned int current_drive;
extern unsigned int current_partition;

extern int fsys_type;
extern char vol_name[256];

extern unsigned int unicode_to_utf8 (unsigned short *filename, unsigned char *utf8, unsigned int n);

struct simp
{
  unsigned short start;
  unsigned short end;
	unsigned short offset;
};
extern struct simp unifont_simp[8];
extern unsigned char unifont_simp_on;

/* The information for a disk geometry. The CHS information is only for
   DOS/Partition table compatibility, and the real number of sectors is
   stored in TOTAL_SECTORS.  */
struct geometry
{
  /* The total number of sectors */
  unsigned long long total_sectors;
  /* Device sector size */
  unsigned int sector_size;
	/* Power of sector size 2 */
	unsigned int log2_sector_size;
	unsigned char vhd_disk;								//vhd磁盘					位0-1,仿真类型：1=不加载到内存    2=加载到内存
	unsigned char fill1;									//填充
	unsigned short fill2;									//填充	
};

extern unsigned long long part_start;
extern unsigned long long part_length;

extern unsigned int current_slice;
//extern unsigned int dos_drive_geometry;
//extern unsigned int dos_part_start;

extern unsigned int buf_drive;
extern unsigned long long buf_track;
extern struct geometry buf_geom;
extern struct geometry tmp_geom;

/* these are the current file position and maximum file position */
extern unsigned long long filepos;
extern unsigned long long filemax;
extern unsigned long long filesize;
extern unsigned long long gzip_filemax;

extern unsigned int emu_iso_sector_size_2048;

#define ISO_TYPE_9660 0
#define ISO_TYPE_udf 1
#define ISO_TYPE_Joliet 2
#define ISO_TYPE_RockRidge 3
//extern unsigned int iso_type;
extern char iso_types;
extern unsigned int udf_BytePerSector;
/*
 *  Common BIOS/boot data.
 */

//extern char *end_of_low_16bit_code;
extern struct multiboot_info mbi;
extern unsigned int saved_drive;
extern unsigned int saved_partition;
extern char saved_dir[256];
extern unsigned int e820cycles;	/* control how many e820 cycles will keep hooked */
extern unsigned int int15nolow;	/* unprotect int13_handler code with int15 */
extern unsigned int memdisk_raw;	/* raw mode as in memdisk */
extern unsigned int a20_keep_on;	/* keep a20 on after RAM drive sector access */
extern unsigned int lba_cd_boot;	/* LBA of no-emulation boot image, in 2048-byte sectors */
extern unsigned int safe_mbr_hook;	/* safe mbr hook flags used by Win9x */
extern unsigned int int13_scheme;	/* controls disk access methods in emulation */
extern unsigned char atapi_dev_count;	/* ATAPI CDROM DRIVE COUNT */
extern unsigned int reg_base_addr_append;
extern unsigned char usb_delay;
extern unsigned short One_transfer;
extern unsigned char usb_count_error;
extern unsigned char usb_drive_num[8];
extern unsigned int init_usb(void);
extern unsigned int init_atapi(void);
extern unsigned char min_cdrom_id;	/* MINIMUM ATAPI CDROM DRIVE NUMBER */
//extern unsigned int force_cdrom_as_boot_device;
extern unsigned int ram_drive;
extern unsigned long long md_part_base;
extern unsigned long long md_part_size;
extern unsigned long long rd_base;
extern unsigned long long rd_size;
extern unsigned long long saved_mem_higher;
extern unsigned int saved_mem_upper;
extern unsigned int saved_mem_lower;
extern unsigned int free_mem_lower_start;
//extern unsigned int saved_mmap_addr;
//extern unsigned int saved_mmap_length;
extern unsigned int extended_memory;
//extern unsigned int init_free_mem_start;
extern int config_len;
extern char menu_init_script_file[32];
extern unsigned int minimum_mem_hi_in_map;
extern int min_con_mem_start;
extern int min_con_mem_size;
extern int displaymem_func (char *arg, int flags);
/*
 *  Error variables.
 */

extern grub_error_t errnum;
extern char *err_list[];

/* Simplify declaration of entry_addr. */
typedef void (*entry_func) (int, int, int, int, int, int)
     __attribute__ ((noreturn));

//extern unsigned long cur_addr;    //????
extern unsigned int cur_addr;
extern entry_func entry_addr;

/* Enter the stage1.5/stage2 C code after the stack is set up. */
void cmain (void);
extern char default_file[60];

/* Halt the processor (called after an unrecoverable error). */
void stop (void) __attribute__ ((noreturn));

/* Reboot the system.  */
void grub_reboot (void) __attribute__ ((noreturn));

/* Halt the system, using APM if possible. If NO_APM is true, don't use
   APM even if it is available.  */
void grub_halt (void); //__attribute__ ((noreturn));

/* The key map.  */
struct key_map
{
	unsigned int from_code;
	unsigned int to_code;
};
extern struct key_map ascii_key_map[];
extern int remap_ascii_char (int key);

/* calls for direct boot-loader chaining */
void chain_stage1 (unsigned int segment, unsigned int offset,
		   unsigned int part_table_addr)
     __attribute__ ((noreturn));
void chain_stage2 (unsigned int segment, unsigned int offset,
		   int second_sector)
     __attribute__ ((noreturn));

/* do some funky stuff, then boot linux */
void linux_boot (void) __attribute__ ((noreturn));

/* do some funky stuff, then boot bzImage linux */
void big_linux_boot (void) __attribute__ ((noreturn));

/* booting a multiboot executable */
void multi_boot (int start, int mb_info, int, int, int, int, int) __attribute__ ((noreturn));

/* If LINEAR is nonzero, then set the Intel processor to linear mode.
   Otherwise, bit 20 of all memory accesses is always forced to zero,
   causing a wraparound effect for bugwards compatibility with the
   8086 CPU. Return 0 for failure and 1 for success. */
int gateA20 (int linear);

/* memory probe routines */
int get_memsize (int type);
int get_eisamemsize (void);

/* Fetch the next entry in the memory map and return the continuation
   value.  DESC is a pointer to the descriptor buffer, and CONT is the
   previous continuation value (0 to get the first entry in the
   map). */
extern int get_mmap_entry (struct mmar_desc *desc, int cont);

/* Get the linear address of a ROM configuration table. Return zero,
   if fails.  */
unsigned int get_rom_config_table (void);

/* Get APM BIOS information.  */
//void get_apm_info (void);

/* Get VBE controller information.  */
int get_vbe_controller_info (struct vbe_controller *controller);

/* Get VBE mode information.  */
int get_vbe_mode_info (int mode_number, struct vbe_mode *mode);

/* Set VBE mode.  */
int set_vbe_mode (int mode_number);

/* Return the data area immediately following our code. */
int get_code_end (void);

/* low-level timing info */
extern int getrtsecs (void);

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
//datetime.h
struct grub_datetime
{
  unsigned short year;
  unsigned char month;
  unsigned char day;
  unsigned char hour;
  unsigned char minute;
  unsigned char second;
	unsigned char pad1;
};

/* Get current date and time */
extern void get_datetime (struct grub_datetime *datetime);

/* Clear the screen. */
void cls (void);

/* Turn on/off cursor. */
unsigned int setcursor (unsigned int on);

/* Set the cursor position. */
extern void gotoxy (int x, int y);

/* Displays an ASCII character.  IBM displays will translate some
   characters to special graphical ones (see the DISP_* constants). */
extern unsigned int grub_putchar (unsigned int c, unsigned int max_width);
unsigned int _putchar (unsigned int c, unsigned int max_width);
unsigned char *set_putchar_hook(unsigned char *hooked);
extern unsigned char* putchar_hooked;

/* Wait for a keypress, and return its packed BIOS/ASCII key code.
   Use ASCII_CHAR(ret) to extract the ASCII code. */
extern int getkey (void);

/* Like GETKEY, but doesn't block, and returns -1 if no keystroke is
   available. */
extern int checkkey (void);

/* Low-level disk I/O */
extern int biosdisk_int13_extensions (unsigned ax, unsigned drive, void *dap, unsigned ssize);
extern int grub_map_efidisk (unsigned ax, unsigned drive, void *dap);
extern int get_diskinfo (unsigned int drive, struct geometry *geometry, unsigned int partition);
int biosdisk (unsigned int read, unsigned int drive, struct geometry *geometry,
	  unsigned long long sector, unsigned int nsec, unsigned int segment);
void stop_floppy (void);

/* Command-line interface functions. */

/* The flags for the builtins.  */
#define BUILTIN_CMDLINE		0x1	/* Run in the command-line.  */
#define BUILTIN_MENU			(1 << 1)/* Run in the menu.  */
#define BUILTIN_IFTITLE		(1 << 2)	/* Only for the command title.  */
#define BUILTIN_SCRIPT		(1 << 3)/* Run in the script.  */
#define BUILTIN_NO_ECHO		(1 << 4)	/* Don't print command on booting. */
#define BUILTIN_HELP_LIST	(1 << 5)/* Show help in listing.  */
#define BUILTIN_BOOTING		(1 << 6)	/* The command is boot-sensitive.  */
#define BUILTIN_BAT_SCRIPT	(1 << 7)
#define BUILTIN_USER_PROG	(1 << 8)
#define BUILTIN_NO_DECOMPRESSION (1 << 9)

#define BAT_SIGN 0x54414221UL		//!BAT
extern grub_size_t bat_md_start;
extern unsigned int bat_md_count;

/* The table for a psp_end*/
typedef struct {
	unsigned int len;
	unsigned int proglen;
	unsigned int arg;
	unsigned int path;
	char filename[0];
} __attribute__ ((packed)) psp_info_t;

/* The table for a builtin.  */
struct builtin
{
  /* The command name.  */
  char *name;                 //名称
  /* The callback function.  */
  int (*func) (char *, int);  //功能
  /* The combination of the flags defined above.  */
  int flags;                  //标记
  /* The short version of the documentation.  */
  char *short_doc;            //短文本
  /* The long version of the documentation.  */
  char *long_doc;             //长文本
};

/* All the builtins are registered in this.  */
extern struct builtin *builtin_table[];

/* The constants for kernel types.  */
typedef enum
{
  KERNEL_TYPE_NONE,		/* None is loaded.  */
  KERNEL_TYPE_MULTIBOOT,	/* Multiboot.  */
  KERNEL_TYPE_LINUX,		/* Linux.  */
  KERNEL_TYPE_BIG_LINUX,	/* Big Linux.  */
  KERNEL_TYPE_FREEBSD,		/* FreeBSD.  */
  KERNEL_TYPE_NETBSD,		/* NetBSD.  */
  KERNEL_TYPE_CHAINLOADER,	/* Chainloader.  */
  KERNEL_TYPE_CDROM
}
kernel_t;

extern kernel_t kernel_type;
extern int show_menu;
extern int silent_hiddenmenu;
extern char *mbr;
extern int grub_timeout;

extern char *wee_skip_to (char *cmdline, int flags);
extern char *skip_to (int flags, char *cmdline);
extern char *WENV_ENVI;
#define SKIP_LINE		0x100
#define SKIP_NONE		0
#define SKIP_WITH_TERMINATE	0x200
#define ADDR_RET_STR WENV_ENVI
//如果变量是字符串, 则:  ADDR_RET_STR = var;
//如果变量是数值, 则:  sprintf (ADDR_RET_STR,"0x%lx",var);
//#define		WENV_RANDOM	(*(unsigned long *)(WENV_ENVI+0x20))
#define		WENV_RANDOM	(*(unsigned int *)(WENV_ENVI+0x20)) //随机数 2字节
#define		PATHEXT		(WENV_ENVI + 0x40)  //路径扩展 
#define		WENV_TMP	(WENV_ENVI + 0x80)  //字符串缓存
/* 环境参数 WENV_ENVI
 * 0x0    特殊返回值 %?_UUID% %?%
 * 0x20   随机数 WENV_RANDOM 只使用2字节
 * 0x40   路径扩展 PATHEXT
 * 0x80   环境参数返回值 字符串缓存 WENV_TMP
 */

//extern char *pre_cmdline;
extern int expand_var(const char *str,char *out,const unsigned int len_max);
extern int run_line (char *heap,int flags);
struct builtin *find_command (char *command);
extern void print_cmdline_message (int forever);
extern void enter_cmdline (char *heap, int forever);

/* C library replacement functions with identical semantics. */
//void grub_printf (const char *format,...);

#define grub_printf(...) grub_sprintf(NULL, __VA_ARGS__)
#define printf_debug(...) grub_sprintf((char*)2, __VA_ARGS__)
#define printf_debug0(...) grub_sprintf((char*)1, __VA_ARGS__)
#define printf_errinfo(...) grub_sprintf((char*)3, __VA_ARGS__)
#define printf_warning(...) grub_sprintf((char*)2, __VA_ARGS__)
extern int grub_sprintf (char *buffer, const char *format, ...);
extern int grub_tolower (int c);
extern int grub_isspace (int c);
extern int grub_strncat (char *s1, const char *s2, int n);
extern void *grub_memmove (void *to, const void *from, grub_size_t len);
void *grub_memset (void *start, int c, grub_size_t len);
extern char *grub_strstr (const char *s1, const char *s2);
//char *grub_strtok (char *s, const char *delim);
extern int grub_memcmp (const char *s1, const char *s2, grub_size_t n);
extern int grub_crc32(char *data,grub_u32_t size);
extern unsigned short grub_crc16(unsigned char *data, int size);
extern int grub_strcmp (const char *s1, const char *s2);
extern int strncmpx(const char *s1,const char *s2, unsigned int n, int case_insensitive);
#define strncmp(s1,s2,n) strncmpx(s1,s2,n,0)
#define strnicmp(s1,s2,n) strncmpx(s1,s2,n,1)
#define strncmpi strnicmp
extern int grub_strlen (const char *str);
char *grub_strcpy (char *dest, const char *src);

extern unsigned long long grub_memmove64(unsigned long long dst_addr, unsigned long long src_addr, unsigned long long len);
extern unsigned long long grub_memset64(unsigned long long dst_addr, unsigned int data, unsigned long long len);
extern int grub_memcmp64(unsigned long long str1addr, unsigned long long str2addr, unsigned long long len);
int mem64 (int func, unsigned long long dest, unsigned long long src, unsigned long long len);

extern unsigned int configfile_opened;

/* misc */
void init_page (void);
void print_error (void);
#if defined(__i386__)
extern char *convert_to_ascii (char *buf, int c, ...);
#else
extern char *convert_to_ascii (char *buf, int c, unsigned long long lo);
#endif
extern char *prompt;
extern int echo_char;
//extern int readline;
struct get_cmdline_arg
{
	unsigned char *cmdline;
	unsigned char *prompt;
	unsigned int maxlen;
	unsigned int echo_char;
	unsigned int readline;
} __attribute__ ((packed));
extern struct get_cmdline_arg get_cmdline_str;
extern int get_cmdline (void);
extern int substring (const char *s1, const char *s2, int case_insensitive);
extern int nul_terminate (char *str);
int get_based_digit (int c, int base);
extern int safe_parse_maxint_with_suffix (char **str_ptr, unsigned long long *myint_ptr, int unitshift);
#define safe_parse_maxint(str_ptr, myint_ptr) safe_parse_maxint_with_suffix(str_ptr, myint_ptr, 0)
extern int parse_string (char *arg);
extern int memcheck (unsigned long long addr, unsigned long long len);
extern void grub_putstr (char *str);
extern void grub_putstr_utf16(unsigned short *str);

#ifndef NO_DECOMPRESSION
/* Compression support. */
struct decomp_entry
{
  char *name;
  int (*open_func) (void);
  void (*close_func) (void);
  unsigned long long (*read_func) (unsigned long long buf, unsigned long long len, unsigned int write);
};

#define DECOMP_TYPE_GZ   0
#define DECOMP_TYPE_LZMA 1
#define DECOMP_TYPE_LZ4  2
#define DECOMP_TYPE_VHD  3
#define NUM_DECOM 4

extern struct decomp_entry decomp_table[NUM_DECOM];
extern int decomp_type;

int gunzip_test_header (void);
void gunzip_close (void);
unsigned long long gunzip_read (unsigned long long buf, unsigned long long len, unsigned int write);
extern int dec_lzma_open (void);
void dec_lzma_close (void);
unsigned long long dec_lzma_read (unsigned long long buf, unsigned long long len, unsigned int write);
extern int dec_lz4_open (void);
void dec_lz4_close (void);
unsigned long long dec_lz4_read (unsigned long long buf, unsigned long long len, unsigned int write);
extern int dec_vhd_open(void);
void dec_vhd_close(void);
unsigned long long dec_vhd_read(unsigned long long buf, unsigned long long len, unsigned int write);
extern int vhd_read;
extern unsigned long long vhd_start_sector;
#endif /* NO_DECOMPRESSION */

extern int rawread (unsigned int drive, unsigned long long sector, unsigned int byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned int write);
extern int devread (unsigned long long sector, unsigned long long byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned int write);
extern int rawwrite (unsigned int drive, unsigned long long sector, unsigned long long buf);

/* Parse a device string and initialize the global parameters. */
char *set_device (char *device);
extern int open_device (void);
//char *setup_part (char *filename);
extern int real_open_partition (int flags);
extern int open_partition (void);
extern int next_partition (void);


/* Open a file or directory on the active device, using GRUB's
   internal filesystem support. */
extern int grub_open (char *filename);
#define GRUB_READ 0xedde0d90
#define GRUB_WRITE 0x900ddeed
#define GRUB_LISTBLK   0x4B42534C
/* Read LEN bytes into BUF from the file that was opened with
   GRUB_OPEN.  If LEN is -1, read all the remaining data in the file.  */
unsigned long long grub_read (unsigned long long buf, unsigned long long len, unsigned int write);

/* Close a file.  */
void grub_close (void);

extern int set_bootdev (int hdbias);

/* Display statistics on the current active device. */
void print_fsys_type (void);

/* Print the root device information.*/
extern void print_root_device (char *buffer,int flag);

/* Display device and filename completions. */
extern void print_a_completion (char *filename, int case_insensitive);
extern int print_completions (int is_filename, int is_completion);

/* Copies the current partition data to the desired address. */
void copy_current_part_entry (char *buf);

//extern void bsd_boot (kernel_t type, int bootdev, char *arg)
//     __attribute__ ((noreturn));
extern void bsd_boot (kernel_t type, int bootdev, char *arg);

/* Define flags for load_image here.  */
/* Don't pass a Linux's mem option automatically.  */
#define KERNEL_LOAD_NO_MEM_OPTION	(1 << 0)

kernel_t load_image (char *kernel, char *arg, kernel_t suggested_type,
		     unsigned int load_flags);

extern int load_module (char *module, char *arg);
extern int load_initrd (char *initrd);

extern int check_password(char* expected, password_t type);
extern int biosdisk_standard (unsigned ah, unsigned drive,
			      unsigned coff, unsigned hoff, unsigned soff,
			      unsigned nsec, unsigned segment);
void init_bios_info (void);

#define EFI_PARTITION   0xef
#define GRUB_PC_PARTITION_TYPE_GPT_DISK		0xee
#define ACTIVE_PARTITION   0x80

struct master_and_dos_boot_sector {
/* 00 */ char dummy1[0x0b]; /* at offset 0, normally there is a short JMP instuction(opcode is 0xEB) */
/* 0B */ unsigned short bytes_per_sector __attribute__ ((packed));/* seems always to be 512, so we just use 512 */
/* 0D */ unsigned char sectors_per_cluster;/* non-zero, the power of 2, i.e., 2^n */
/* 0E */ unsigned short reserved_sectors __attribute__ ((packed));/* FAT=non-zero, NTFS=0? */
/* 10 */ unsigned char number_of_fats;/* NTFS=0; FAT=1 or 2  */
/* 11 */ unsigned short root_dir_entries __attribute__ ((packed));/* FAT32=0, NTFS=0, FAT12/16=non-zero */
/* 13 */ unsigned short total_sectors_short __attribute__ ((packed));/* FAT32=0, NTFS=0, FAT12/16=any */
/* 15 */ unsigned char media_descriptor;/* range from 0xf0 to 0xff */
/* 16 */ unsigned short sectors_per_fat __attribute__ ((packed));/* FAT32=0, NTFS=0, FAT12/16=non-zero */
/* 18 */ unsigned short sectors_per_track __attribute__ ((packed));/* range from 1 to 63 */
/* 1A */ unsigned short total_heads __attribute__ ((packed));/* range from 1 to 256 */
/* 1C */ unsigned int hidden_sectors __attribute__ ((packed));/* any value */
/* 20 */ unsigned int total_sectors_long __attribute__ ((packed));/* FAT32=non-zero, NTFS=0, FAT12/16=any */
/* 24 */ unsigned int sectors_per_fat32 __attribute__ ((packed));/* FAT32=non-zero, NTFS=any, FAT12/16=any */
/* 28 */ unsigned long long total_sectors_long_long __attribute__ ((packed));/* NTFS=non-zero, FAT12/16/32=any */
///* 30 */ char dummy2[0x18e];
/* 30 */ char dummy2[0x188];
/* 0x1B8 */unsigned char unique_signature[4];
/* 0x1BC */unsigned char unknown[2];

    /* Partition Table, starting at offset 0x1BE */
/* 1BE */ struct {
	/* +00 */ unsigned char boot_indicator;
	/* +01 */ unsigned char start_head;
	/* +02 */ unsigned short start_sector_cylinder __attribute__ ((packed));
	/* +04 */ unsigned char system_indicator;
	/* +05 */ unsigned char end_head;
	/* +06 */ unsigned short end_sector_cylinder __attribute__ ((packed));
	/* +08 */ unsigned int start_lba __attribute__ ((packed));
	/* +0C */ unsigned int total_sectors __attribute__ ((packed));
	/* +10 */
    } P[4];
/* 1FE */ unsigned short boot_signature __attribute__ ((packed));/* 0xAA55 */
  };

extern unsigned int probed_total_sectors;
extern unsigned int probed_heads;
extern unsigned int probed_sectors_per_track;
extern unsigned int probed_cylinders;
extern unsigned int sectors_per_cylinder;

extern int filesystem_type;

extern int probe_bpb (struct master_and_dos_boot_sector *BS);
extern int probe_mbr (struct master_and_dos_boot_sector *BS, unsigned int start_sector1, unsigned int sector_count1, unsigned int part_start1);

extern int check_int13_extensions (unsigned drive, unsigned lba1sector);

struct drive_parameters
{
	unsigned short size;
	unsigned short flags;
	unsigned int cylinders;
	unsigned int heads;
	unsigned int sectors;
	unsigned long long total_sectors;
	unsigned short bytes_per_sector;
	/* ver 2.0 or higher */
	unsigned int EDD_configuration_parameters;
	/* ver 3.0 or higher */
	unsigned short signature_dpi;
	unsigned char length_dpi;
	unsigned char reserved[3];
	unsigned char name_of_host_bus[4];
	unsigned char name_of_interface_type[8];
	unsigned char interface_path[8];
	unsigned char device_path[8];
	unsigned char reserved2;
	unsigned char checksum;

	/* XXX: This is necessary, because the BIOS of Thinkpad X20
	   writes a garbage to the tail of drive parameters,
	   regardless of a size specified in a caller.  */
	unsigned char dummy[16];
} __attribute__ ((packed));

#define UUID_NODE_LEN 6
#define GUID_SIZE 16

typedef struct {
	union {
		char raw[GUID_SIZE];
		struct {
			grub_u32_t time_low;
			grub_u16_t time_mid;
			grub_u16_t time_high_and_version;
			char clock_seq_high_and_reserved;
			char clock_seq_low;
			char node[UUID_NODE_LEN];
		} UUID;
	};
} PACKED GUID;

//GPT_HDR_SIG "EFI PART"
#define	GPT_HDR_SIG		0x5452415020494645LL	//gpt磁盘签名
typedef struct {
	grub_u64_t		hdr_sig;			//签名
	grub_u32_t		hdr_revision;	//版本 
	grub_u32_t		hdr_size;			//分区表头尺寸
	grub_u32_t		hdr_crc_self;	//分区表头crc32(第0－91字节)
	grub_u32_t		__reserved;		//保留(必须是0)
	grub_u64_t		hdr_lba_self;	//当前分区表头lba(这个分区表头的位置)
	grub_u64_t		hdr_lba_alt;	//备用分区表头lba(另一个分区表头的位置)
	grub_u64_t		hdr_lba_start;//可用分区起始lba
	grub_u64_t		hdr_lba_end;	//可用分区结束lba
	GUID		hdr_uuid;						//硬盘GUID
	grub_u64_t		hdr_lba_table;//分区表起始lba
	grub_u32_t		hdr_entries;	//分区表数量(入口数)
	grub_u32_t		hdr_entsz;		//单个分区表尺寸
	grub_u32_t		hdr_crc_table;//分区crc32
	grub_u32_t		padding;			//填充(必须是0)
} PACKED GPT_HDR;
typedef GPT_HDR* P_GPT_HDR;	//gpt_标题

typedef struct {
	GUID type;                //分区类型GUID
	GUID uid;                 //分区GUID
	grub_u64_t starting_lba;	//分区起始
	grub_u64_t ending_lba;		//分区尺寸
	union{
		grub_u64_t attributes;	//属性
		struct {
			grub_u16_t unused[3];
			grub_u16_t gpt_att;
		} PACKED ms_attr;
	};
	char name[72];						//名称
} PACKED GPT_ENT;
typedef GPT_ENT* P_GPT_ENT;		//gpt_分区入口


int check_64bit (void);
#if defined(__i386__)
extern int check_64bit_and_PAE  (void);
#endif
extern int is64bit;

#define IS64BIT_PAE   1
#define IS64BIT_AMD64 2

extern int errorcheck;
extern unsigned int pxe_restart_config;
extern char *efi_pxe_buf;
extern unsigned int saved_pxe_ip;
extern unsigned char saved_pxe_mac[6];

#ifdef FSYS_PXE

#include "pxe.h"
extern grub_u8_t pxe_mac_len, pxe_mac_type;
extern MAC_ADDR pxe_mac;
extern grub_u32_t pxe_yip, pxe_sip, pxe_gip;
extern unsigned int pxe_keep;
extern BOOTPLAYER *discover_reply;
extern unsigned short pxe_basemem, pxe_freemem;
extern struct grub_efi_pxe *pxe_entry;
extern unsigned int pxe_inited;
extern unsigned int pxe_scan(void);
extern int pxe_detect(int, char *);
extern void pxe_unload(void);
extern void pxe_init (void);
int pxe_func(char* arg,int flags);
#ifdef FSYS_IPXE
extern grub_u32_t has_ipxe;
int ipxe_func(char* arg,int flags);
void ipxe_init(void);
#endif
#else /* ! FSYS_PXE */

#define pxe_detect()

#endif /* FSYS_PXE */

extern unsigned int fb_status;
/* backup of original BIOS floppy-count byte in 0x410 */
extern char floppies_orig;
/* backup of original BIOS harddrive-count byte in 0x475 */
extern char harddrives_orig;
extern char cdrom_orig;
//extern char first_boot;
typedef void *grub_efi_handle_t;
extern struct grub_disk_data *get_device_by_drive (unsigned int drive, unsigned int map);
extern struct grub_disk_data *disk_data;  //磁盘数据
extern int big_to_little (char *filename, unsigned int n);
extern void uninstall (unsigned int drive, struct grub_disk_data *d);
extern int (*ext_timer)(char *arg, int flags);

//##########################################################################################################################################

#define ARRAY_SIZE(array) (sizeof (array) / sizeof (array[0]))
#define EXPORT_FUNC(x)	x
#define EXPORT_VAR(x)	x

//----------------------------------------------------------------------------------------------------
//grub/efi/api.h
/* For consistency and safety, we name the EFI-defined types differently. 为了一致性和安全性，我们对EFI定义的类型进行了不同的命名。
   All names are transformed into lower case, _t appended, and            所有的名字都被转换成小写字母，附加_t和预置grub_efi_
   grub_efi_ prepended.  */

/* Constants. 常量 */
//事件的类型
#define GRUB_EFI_EVT_TIMER				0x80000000                    //计时器 
#define GRUB_EFI_EVT_RUNTIME				0x40000000                  //运行
#define GRUB_EFI_EVT_RUNTIME_CONTEXT			0x20000000            //运行环境
#define GRUB_EFI_EVT_NOTIFY_WAIT			0x00000100                //通知等待
#define GRUB_EFI_EVT_NOTIFY_SIGNAL			0x00000200              //通知信号
#define GRUB_EFI_EVT_SIGNAL_EXIT_BOOT_SERVICES		0x00000201    //信号出口启动服务
#define GRUB_EFI_EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE	0x60000202  //信号虚拟地址变更
//事件的优先级
#define GRUB_EFI_TPL_APPLICATION	4   //应用
#define GRUB_EFI_TPL_CALLBACK		8     //回调
#define GRUB_EFI_TPL_NOTIFY		16      //通知
#define GRUB_EFI_TPL_HIGH_LEVEL		31  //高电平

#define GRUB_EFI_MEMORY_UC	0x0000000000000001LL  //存储器UC 
#define GRUB_EFI_MEMORY_WC	0x0000000000000002LL  //存储器
#define GRUB_EFI_MEMORY_WT	0x0000000000000004LL  //存储器
#define GRUB_EFI_MEMORY_WB	0x0000000000000008LL  //存储器
#define GRUB_EFI_MEMORY_UCE	0x0000000000000010LL  //存储器
#define GRUB_EFI_MEMORY_WP	0x0000000000001000LL  //存储器
#define GRUB_EFI_MEMORY_RP	0x0000000000002000LL  //存储器
#define GRUB_EFI_MEMORY_XP	0x0000000000004000LL  //存储器
#define GRUB_EFI_MEMORY_NV	0x0000000000008000LL  //存储器
#define GRUB_EFI_MEMORY_MORE_RELIABLE	0x0000000000010000LL  //存储器更可靠
#define GRUB_EFI_MEMORY_RO	0x0000000000020000LL  //存储器
#define GRUB_EFI_MEMORY_RUNTIME	0x8000000000000000LL  //存储器运行

#define GRUB_EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL	0x00000001    //打开协议 通过句柄 
#define GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL		0x00000002        //打开协议 获得协议
#define GRUB_EFI_OPEN_PROTOCOL_TEST_PROTOCOL		0x00000004      //打开协议 测试协议
#define GRUB_EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER	0x00000008  //打开协议 通过子控制器
#define GRUB_EFI_OPEN_PROTOCOL_BY_DRIVER		0x00000010          //打开协议 通过驱动器
#define GRUB_EFI_OPEN_PROTOCOL_BY_EXCLUSIVE		0x00000020        //打开协议 通过专用的

#define GRUB_EFI_OS_INDICATIONS_BOOT_TO_FW_UI	0x0000000000000001ULL //OS指示启动到FW UI 

#define GRUB_EFI_VARIABLE_NON_VOLATILE		0x0000000000000001      //变量不易变
#define GRUB_EFI_VARIABLE_BOOTSERVICE_ACCESS	0x0000000000000002  //变量启动服务访问
#define GRUB_EFI_VARIABLE_RUNTIME_ACCESS	0x0000000000000004      //变量运行时访问

#define GRUB_EFI_TIME_ADJUST_DAYLIGHT	0x01  //时间调节日光
#define GRUB_EFI_TIME_IN_DAYLIGHT	0x02      //时间白昼

#define GRUB_EFI_UNSPECIFIED_TIMEZONE	0x07FF  //未指定时区

#define GRUB_EFI_OPTIONAL_PTR	0x00000001      //可选PTR 

//加载映像GUID 
#define GRUB_EFI_LOADED_IMAGE_GUID	\
  { 0x5b1b31a1, 0x9562, 0x11d2, \
    { 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
    }
//磁盘IO_GUID
#define GRUB_EFI_DISK_IO_GUID	\
  { 0xce345171, 0xba0b, 0x11d2, \
    { 0x8e, 0x4f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }
//块IO_GUID
#define GRUB_EFI_BLOCK_IO_GUID	\
  { 0x964e5b21, 0x6459, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }
//连续IO_GUID
#define GRUB_EFI_SERIAL_IO_GUID \
  { 0xbb25cf6f, 0xf1d4, 0x11d2, \
    { 0x9a, 0x0c, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0xfd } \
  }
//简单网络GUID
#define GRUB_EFI_SIMPLE_NETWORK_GUID	\
  { 0xa19832b9, 0xac25, 0x11d3, \
    { 0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }
//PXE GUID
#define GRUB_EFI_PXE_GUID	\
  { 0x03c4e603, 0xac28, 0x11d3, \
    { 0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }
//设备路径GUID 
#define GRUB_EFI_DEVICE_PATH_GUID	\
  { 0x09576e91, 0x6d3f, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }
//简单文本输入协议GUID 
#define GRUB_EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID \
  { 0x387477c1, 0x69c7, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }
//简单文本输入扩展协议GUID 
#define GRUB_EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID \
  { 0xdd9e7534, 0x7762, 0x4698, \
    { 0x8c, 0x14, 0xf5, 0x85, 0x17, 0xa6, 0x25, 0xaa } \
  }
//简单文本输出协议GUID 
#define GRUB_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID \
  { 0x387477c2, 0x69c7, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }
//简单指针协议GUID 
#define GRUB_EFI_SIMPLE_POINTER_PROTOCOL_GUID \
  { 0x31878c87, 0xb75, 0x11d5, \
    { 0x9a, 0x4f, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }
//绝对指针协议GUID
#define GRUB_EFI_ABSOLUTE_POINTER_PROTOCOL_GUID \
  { 0x8D59D32B, 0xC655, 0x4AE9, \
    { 0x9B, 0x15, 0xF2, 0x59, 0x04, 0x99, 0x2A, 0x43 } \
  }
//驱动程序绑定协议GUID 
#define GRUB_EFI_DRIVER_BINDING_PROTOCOL_GUID \
  { 0x18A031AB, 0xB443, 0x4D1A, \
    { 0xA5, 0xC0, 0x0C, 0x09, 0x26, 0x1E, 0x9F, 0x71 } \
  }
//加载映像协议GUID 
#define GRUB_EFI_LOADED_IMAGE_PROTOCOL_GUID \
  { 0x5B1B31A1, 0x9562, 0x11d2, \
    { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } \
  }
//加载文件协议GUID
#define GRUB_EFI_LOAD_FILE_PROTOCOL_GUID \
  { 0x56EC3091, 0x954C, 0x11d2, \
    { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } \
  }
//LoadFile2 协议 https://github.com/u-boot/u-boot/commit/ec80b4735a593961fe701cc3a5d717d4739b0fd0
#define GRUB_EFI_LOAD_FILE2_PROTOCOL_GUID \
  { 0x4006c0c1, 0xfcb3, 0x403e, \
    { 0x99, 0x6d, 0x4a, 0x6c, 0x87, 0x24, 0xe0, 0x6d } \
  }
//简单文件系统协议GUID 
#define GRUB_EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
  { 0x0964e5b22, 0x6459, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }
//磁带IO协议GUID 
#define GRUB_EFI_TAPE_IO_PROTOCOL_GUID \
  { 0x1e93e633, 0xd65a, 0x459e, \
    { 0xab, 0x84, 0x93, 0xd9, 0xec, 0x26, 0x6d, 0x18 } \
  }
//Unicode排序协议GUID
#define GRUB_EFI_UNICODE_COLLATION_PROTOCOL_GUID \
  { 0x1d85cd7f, 0xf43d, 0x11d2, \
    { 0x9a, 0x0c, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }
//SCSI IO协议GUID 
#define GRUB_EFI_SCSI_IO_PROTOCOL_GUID \
  { 0x932f47e6, 0x2362, 0x4002, \
    { 0x80, 0x3e, 0x3c, 0xd5, 0x4b, 0x13, 0x8f, 0x85 } \
  }
//USB2 HC协议GUID 
#define GRUB_EFI_USB2_HC_PROTOCOL_GUID \
  { 0x3e745226, 0x9818, 0x45b6, \
    { 0xa2, 0xac, 0xd7, 0xcd, 0x0e, 0x8b, 0xa2, 0xbc } \
  }
//调试支持协议GUID
#define GRUB_EFI_DEBUG_SUPPORT_PROTOCOL_GUID \
  { 0x2755590C, 0x6F3C, 0x42FA, \
    { 0x9E, 0xA4, 0xA3, 0xBA, 0x54, 0x3C, 0xDA, 0x25 } \
  }
//调试端口协议GUID
#define GRUB_EFI_DEBUGPORT_PROTOCOL_GUID \
  { 0xEBA4E8D2, 0x3858, 0x41EC, \
    { 0xA2, 0x81, 0x26, 0x47, 0xBA, 0x96, 0x60, 0xD0 } \
  }
//解压协议GUID
#define GRUB_EFI_DECOMPRESS_PROTOCOL_GUID \
  { 0xd8117cfe, 0x94a6, 0x11d4, \
    { 0x9a, 0x3a, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }
//设备路径文本协议 GUID
#define GRUB_EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID \
  { 0x8b843e20, 0x8132, 0x4852, \
    { 0x90, 0xcc, 0x55, 0x1a, 0x4e, 0x4a, 0x7f, 0x1c } \
  }
//设备路径实用程序协议GUID
#define GRUB_EFI_DEVICE_PATH_UTILITIES_PROTOCOL_GUID \
  { 0x379be4e, 0xd706, 0x437d, \
    { 0xb0, 0x37, 0xed, 0xb8, 0x2f, 0xb7, 0x72, 0xa4 } \
  }
//文本协议的设备路径GUID
#define GRUB_EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL_GUID \
  { 0x5c99a21, 0xc70f, 0x4ad2, \
    { 0x8a, 0x5f, 0x35, 0xdf, 0x33, 0x43, 0xf5, 0x1e } \
  }
//ACPI表协议GUID
#define GRUB_EFI_ACPI_TABLE_PROTOCOL_GUID \
  { 0xffe06bdd, 0x6107, 0x46a6, \
    { 0x7b, 0xb2, 0x5a, 0x9c, 0x7e, 0xc5, 0x27, 0x5c} \
  }
//HII配置路由协议GUID
#define GRUB_EFI_HII_CONFIG_ROUTING_PROTOCOL_GUID \
  { 0x587e72d7, 0xcc50, 0x4f79, \
    { 0x82, 0x09, 0xca, 0x29, 0x1f, 0xc1, 0xa1, 0x0f } \
  }
//HII数据库协议GUID
#define GRUB_EFI_HII_DATABASE_PROTOCOL_GUID \
  { 0xef9fc172, 0xa1b2, 0x4693, \
    { 0xb3, 0x27, 0x6d, 0x32, 0xfc, 0x41, 0x60, 0x42 } \
  }
//HII字符串协议GUID
#define GRUB_EFI_HII_STRING_PROTOCOL_GUID \
  { 0xfd96974, 0x23aa, 0x4cdc, \
    { 0xb9, 0xcb, 0x98, 0xd1, 0x77, 0x50, 0x32, 0x2a } \
  }
//HII映像协议GUID
#define GRUB_EFI_HII_IMAGE_PROTOCOL_GUID \
  { 0x31a6406a, 0x6bdf, 0x4e46, \
    { 0xb2, 0xa2, 0xeb, 0xaa, 0x89, 0xc4, 0x9, 0x20 } \
  }
//HII字体协议GUID
#define GRUB_EFI_HII_FONT_PROTOCOL_GUID \
  { 0xe9ca4775, 0x8657, 0x47fc, \
    { 0x97, 0xe7, 0x7e, 0xd6, 0x5a, 0x8, 0x43, 0x24 } \
  }
//HII配置访问协议 GUID
#define GRUB_EFI_HII_CONFIGURATION_ACCESS_PROTOCOL_GUID \
  { 0x330d4706, 0xf2a0, 0x4e4f, \
    { 0xa3, 0x69, 0xb6, 0x6f, 0xa8, 0xd5, 0x43, 0x85 } \
  }
//组件名称协议GUID
#define GRUB_EFI_COMPONENT_NAME_PROTOCOL_GUID \
  { 0x107a772c, 0xd5e1, 0x11d4, \
    { 0x9a, 0x46, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }	
//组件名称2协议GUID
#define GRUB_EFI_COMPONENT_NAME2_PROTOCOL_GUID \
  { 0x6a7a5cff, 0xe8d9, 0x4f70, \
    { 0xba, 0xda, 0x75, 0xab, 0x30, 0x25, 0xce, 0x14} \
  }
//USB IO协议GUID
#define GRUB_EFI_USB_IO_PROTOCOL_GUID \
  { 0x2B2F68D6, 0x0CD2, 0x44cf, \
    { 0x8E, 0x8B, 0xBB, 0xA2, 0x0B, 0x1B, 0x5B, 0x75 } \
  }
//天奴定制减压GUID
#define GRUB_EFI_TIANO_CUSTOM_DECOMPRESS_GUID \
  { 0xa31280ad, 0x481e, 0x41b6, \
    { 0x95, 0xe8, 0x12, 0x7f, 0x4c, 0x98, 0x47, 0x79 } \
  }
//CRC32制导剖面提取 GUID
#define GRUB_EFI_CRC32_GUIDED_SECTION_EXTRACTION_GUID \
  { 0xfc1bcdb0, 0x7d31, 0x49aa, \
    { 0x93, 0x6a, 0xa4, 0x60, 0x0d, 0x9d, 0xd0, 0x83 } \
  }
//LZMA自定义解压缩 GUID
#define GRUB_EFI_LZMA_CUSTOM_DECOMPRESS_GUID \
  { 0xee4e5898, 0x3914, 0x4259, \
    { 0x9d, 0x6e, 0xdc, 0x7b, 0xd7, 0x94, 0x03, 0xcf } \
  }
//TSC频率GUID
#define GRUB_EFI_TSC_FREQUENCY_GUID \
  { 0xdba6a7e3, 0xbb57, 0x4be7, \
    { 0x8a, 0xf8, 0xd5, 0x78, 0xdb, 0x7e, 0x56, 0x87 } \
  }
//系统资源表GUID
#define GRUB_EFI_SYSTEM_RESOURCE_TABLE_GUID \
  { 0xb122a263, 0x3661, 0x4f68, \
    { 0x99, 0x29, 0x78, 0xf8, 0xb0, 0xd6, 0x21, 0x80 } \
  }
//DXE服务表 GUID
#define GRUB_EFI_DXE_SERVICES_TABLE_GUID \
  { 0x05ad34ba, 0x6f02, 0x4214, \
    { 0x95, 0x2e, 0x4d, 0xa0, 0x39, 0x8e, 0x2b, 0xb9 } \
  }
//HOB表GUID
#define GRUB_EFI_HOB_LIST_GUID \
  { 0x7739f24c, 0x93d7, 0x11d4, \
    { 0x9a, 0x3a, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }
//内存类型信息GUID
#define GRUB_EFI_MEMORY_TYPE_INFORMATION_GUID \
  { 0x4c19049f, 0x4137, 0x4dd3, \
    { 0x9c, 0x10, 0x8b, 0x97, 0xa8, 0x3f, 0xfd, 0xfa } \
  }
//调试映像信息表GUID
#define GRUB_EFI_DEBUG_IMAGE_INFO_TABLE_GUID \
  { 0x49152e77, 0x1ada, 0x4764, \
    { 0xb7, 0xa2, 0x7a, 0xfe, 0xfe, 0xd9, 0x5e, 0x8b } \
  }
//MPS表GUID
#define GRUB_EFI_MPS_TABLE_GUID	\
  { 0xeb9d2d2f, 0x2d88, 0x11d3, \
    { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }
//ACPI表GUID
#define GRUB_EFI_ACPI_TABLE_GUID	\
  { 0xeb9d2d30, 0x2d88, 0x11d3, \
    { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }
//ACPI_20表GUID
#define GRUB_EFI_ACPI_20_TABLE_GUID	\
  { 0x8868e871, 0xe4f1, 0x11d3, \
    { 0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }
//SMBIOS表GUID
#define GRUB_EFI_SMBIOS_TABLE_GUID	\
  { 0xeb9d2d31, 0x2d88, 0x11d3, \
    { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }
//SAL表GUID
#define GRUB_EFI_SAL_TABLE_GUID \
  { 0xeb9d2d32, 0x2d88, 0x11d3, \
      { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }
//HCDP表GUID
#define GRUB_EFI_HCDP_TABLE_GUID \
  { 0xf951938d, 0x620b, 0x42ef, \
      { 0x82, 0x79, 0xa8, 0x4b, 0x79, 0x61, 0x78, 0x98 } \
  }
//设备树GUID
#define GRUB_EFI_DEVICE_TREE_GUID \
  { 0xb1b621d5, 0xf19c, 0x41a5, \
      { 0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0 } \
  }
//供应商APPLE GUID
#define GRUB_EFI_VENDOR_APPLE_GUID \
  { 0x2B0585EB, 0xD8B8, 0x49A9,	\
      { 0x8B, 0x8C, 0xE2, 0x1B, 0x01, 0xAE, 0xF2, 0xB7 } \
  }

#define GRUB_EFI_GRUB_VARIABLE_GUID \
  { 0x91376aff, 0xcba6, 0x42be, \
    { 0x94, 0x9d, 0x06, 0xfd, 0xe8, 0x11, 0x28, 0xe8 } \
  }

#define GRUB_EFI_SECURITY_PROTOCOL_GUID \
  { 0xa46423e3, 0x4617, 0x49f1, \
    { 0xb9, 0xff, 0xd1, 0xbf, 0xa9, 0x11, 0x58, 0x39 } \
  }

#define GRUB_EFI_SECURITY2_PROTOCOL_GUID \
  { 0x94ab2f58, 0x1438, 0x4ef1, \
    { 0x91, 0x52, 0x18, 0x94, 0x1a, 0x3a, 0x0e, 0x68 } \
  }

#define GRUB_EFI_SHIM_LOCK_GUID \
  { 0x605dab50, 0xe046, 0x4300, \
    { 0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23 } \
  }

#define GRUB_EFI_IP4_CONFIG2_PROTOCOL_GUID \
  { 0x5b446ed1, 0xe30b, 0x4faa, \
    { 0x87, 0x1a, 0x36, 0x54, 0xec, 0xa3, 0x60, 0x80 } \
  }

#define GRUB_EFI_IP6_CONFIG_PROTOCOL_GUID \
  { 0x937fe521, 0x95ae, 0x4d1a, \
    { 0x89, 0x29, 0x48, 0xbc, 0xd9, 0x0a, 0xd3, 0x1a } \
  }
	
#define GRUB_EFI_DHCP4_SERVICE_BINDING_PROTOCOL_GUID \
  { 0x9d9a39d8, 0xbd42, 0x4a73, \
    { 0xa4, 0xd5, 0x8e, 0xe9, 0x4b, 0xe1, 0x13, 0x80 } \
  }

#define GRUB_EFI_DHCP4_PROTOCOL_GUID \
  { 0x8a219718, 0x4ef5, 0x4761, \
    { 0x91, 0xc8, 0xc0, 0xf0, 0x4b, 0xda, 0x9e, 0x56 } \
  }

#define GRUB_EFI_DHCP6_SERVICE_BINDING_PROTOCOL_GUID \
  { 0x9fb9a8a1, 0x2f4a, 0x43a6, \
    { 0x88, 0x9c, 0xd0, 0xf7, 0xb6, 0xc4 ,0x7a, 0xd5 } \
  }

#define GRUB_EFI_DHCP6_PROTOCOL_GUID \
  { 0x87c8bad7, 0x595, 0x4053, \
    { 0x82, 0x97, 0xde, 0xde, 0x39, 0x5f, 0x5d, 0x5b } \
  }	
//HTTP服务绑定协议GUID
#define GRUB_EFI_HTTP_SERVICE_BINDING_PROTOCOL_GUID \
  { 0xbdc8e6af, 0xd9bc, 0x4379, \
      { 0xa7, 0x2a, 0xe0, 0xc4, 0xe7, 0x5d, 0xae, 0x1c } \
  }

//HTTP协议GUID
#define GRUB_EFI_HTTP_PROTOCOL_GUID \
  { 0x7A59B29B, 0x910B, 0x4171, \
      { 0x82, 0x42, 0xA8, 0x5A, 0x0D, 0xF2, 0x5B, 0x5B } \
  }	

	
	
//SAL系统表  //签名
struct grub_efi_sal_system_table
{
  unsigned int signature;        
  unsigned int total_table_len;  //总表尺寸
  unsigned short sal_rev;
  unsigned short entry_count;      //入口计数
  unsigned char checksum;          //校验和
  unsigned char reserved1[7];      //保留
  unsigned short sal_a_version;    //version
  unsigned short sal_b_version;    //version
  unsigned char oem_id[32];        //oem标识
  unsigned char product_id[32];    //产品标识
  unsigned char reserved2[8];      //保留
  unsigned char entries[0];        //条目
};

enum    //SAL系统表类型
  {
    GRUB_EFI_SAL_SYSTEM_TABLE_TYPE_ENTRYPOINT_DESCRIPTOR = 0,           //入口描述符
    GRUB_EFI_SAL_SYSTEM_TABLE_TYPE_MEMORY_DESCRIPTOR = 1,               //存储描述符
    GRUB_EFI_SAL_SYSTEM_TABLE_TYPE_PLATFORM_FEATURES = 2,               //平台特征
    GRUB_EFI_SAL_SYSTEM_TABLE_TYPE_TRANSLATION_REGISTER_DESCRIPTOR = 3, //转换寄存器描述符
    GRUB_EFI_SAL_SYSTEM_TABLE_TYPE_PURGE_TRANSLATION_COHERENCE = 4,     //清除转换连接
    GRUB_EFI_SAL_SYSTEM_TABLE_TYPE_AP_WAKEUP = 5                        //AP唤醒
  };

struct grub_efi_sal_system_table_entrypoint_descriptor  //SAL系统表入口描述符
{
  unsigned char type;
  unsigned char pad[7];
  unsigned long long pal_proc_addr;
  unsigned long long sal_proc_addr;
  unsigned long long global_data_ptr;
  unsigned long long reserved[2];
};

struct grub_efi_sal_system_table_memory_descriptor  //SAL系统表存储描述符
{
  unsigned char type;      //类型
  unsigned char sal_used;  //使用
  unsigned char attr;      //属性
  unsigned char ar;
  unsigned char attr_mask; //屏蔽掩膜
  unsigned char mem_type;  //内存类型
  unsigned char usage;     //用法
  unsigned char unknown;   //未知的
  unsigned long long addr;     //地址
  unsigned long long len;      //长度
  unsigned long long unknown2; //未知的
};

struct grub_efi_sal_system_table_platform_features  //SAL系统表平台特征
{
  unsigned char type;        //类型
  unsigned char flags;       //标记
  unsigned char reserved[14];//保留
};

struct grub_efi_sal_system_table_translation_register_descriptor    //SAL系统表转换寄存器描述符
{
  unsigned char type;            //类型
  unsigned char register_type;   //寄存器类型
  unsigned char register_number; //寄存器号
  unsigned char reserved[5];     //保留
  unsigned long long addr;           //地址
  unsigned long long page_size;      //页尺寸
  unsigned long long reserver;       //保留
};

struct grub_efi_sal_system_table_purge_translation_coherence    //SAL系统表清除转换连接
{
  unsigned char type;        //类型
  unsigned char reserved[3]; //保留
  unsigned int ndomains;   //N域
  unsigned long long coherence;  //一致性
};

struct grub_efi_sal_system_table_ap_wakeup    //SAL系统表AP唤醒
{
  unsigned char type;        //类型
  unsigned char mechanism;   //机制
  unsigned char reserved[6]; //保留
  unsigned long long vector;     //矢量
};

enum    //SAL系统表平台特征
  {
    GRUB_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_BUSLOCK = 1,     //总线锁 
    GRUB_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_IRQREDIRECT = 2, //IRQ重定向
    GRUB_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_IPIREDIRECT = 4, //IPI重定向
    GRUB_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_ITCDRIFT = 8,    //斜纹
  };

typedef enum grub_efi_parity_type //校验方式
  {
    GRUB_EFI_SERIAL_DEFAULT_PARITY, //缺省校验 
    GRUB_EFI_SERIAL_NO_PARITY,      //无校验
    GRUB_EFI_SERIAL_EVEN_PARITY,    //偶校验
    GRUB_EFI_SERIAL_ODD_PARITY      //奇校验
  }
grub_efi_parity_type_t;	//4

typedef enum grub_efi_stop_bits   //停止位
  {
    GRUB_EFI_SERIAL_DEFAULT_STOP_BITS,  //缺省停止位
    GRUB_EFI_SERIAL_1_STOP_BIT,         //1停止位
    GRUB_EFI_SERIAL_1_5_STOP_BITS,      //1.5停止位
    GRUB_EFI_SERIAL_2_STOP_BITS         //2停止位
  }
grub_efi_stop_bits_t;

/* Enumerations. 枚举 */
enum grub_efi_timer_delay   //计时器延迟
  {
    GRUB_EFI_TIMER_CANCEL,    //计时器取消
    GRUB_EFI_TIMER_PERIODIC,  //计时器周期性
    GRUB_EFI_TIMER_RELATIVE   //计时器相对
  };
typedef enum grub_efi_timer_delay grub_efi_timer_delay_t;

enum grub_efi_allocate_type //分配类型
  {
    GRUB_EFI_ALLOCATE_ANY_PAGES,    //任意页面				0	分配请求分配满足该请求的任何可用页面范围。输入时，忽略内存指向的地址
    GRUB_EFI_ALLOCATE_MAX_ADDRESS,  //最大地址				1	分配请求分配任何可用的页范围，这些页的最上面地址小于或等于输入时内存指向的地址。
    GRUB_EFI_ALLOCATE_ADDRESS,      //指定地址				2	分配请求在输入时由内存指向的地址分配页。
    GRUB_EFI_MAX_ALLOCATION_TYPE    //最大分配类型		3
  };
typedef enum grub_efi_allocate_type grub_efi_allocate_type_t;

enum grub_efi_memory_type   //存储器类型
  {
    GRUB_EFI_RESERVED_MEMORY_TYPE,          //保留内存类型        0
    GRUB_EFI_LOADER_CODE,                   //装载程序代码        1
    GRUB_EFI_LOADER_DATA,                   //装载数据            2
    GRUB_EFI_BOOT_SERVICES_CODE,            //启动服务代码        3
    GRUB_EFI_BOOT_SERVICES_DATA,            //启动服务数据        4
    GRUB_EFI_RUNTIME_SERVICES_CODE,         //运行服务代码        5
    GRUB_EFI_RUNTIME_SERVICES_DATA,         //运行服务数据        6
    GRUB_EFI_CONVENTIONAL_MEMORY,           //常规存储器          7
    GRUB_EFI_UNUSABLE_MEMORY,               //不可用内存          8
    GRUB_EFI_ACPI_RECLAIM_MEMORY,           //ACPI回收内存        9
    GRUB_EFI_ACPI_MEMORY_NVS,               //ACPI存储器NVS       a
    GRUB_EFI_MEMORY_MAPPED_IO,              //内存映射IO          b
    GRUB_EFI_MEMORY_MAPPED_IO_PORT_SPACE,   //内存映射IO端口空间  c
    GRUB_EFI_PAL_CODE,                      //PAL代码             d
    GRUB_EFI_PERSISTENT_MEMORY,             //持久记忆            e
    GRUB_EFI_MAX_MEMORY_TYPE                //最大内存类型        f
  };
typedef enum grub_efi_memory_type grub_efi_memory_type_t;	//4

enum grub_efi_interface_type    //接口类型 
  {
    GRUB_EFI_NATIVE_INTERFACE   //本机接口
  };
typedef enum grub_efi_interface_type grub_efi_interface_type_t;

enum grub_efi_locate_search_type  //定位搜索类型
  {
    GRUB_EFI_ALL_HANDLES,         //所有句柄 
    GRUB_EFI_BY_REGISTER_NOTIFY,  //通过寄存器通知
    GRUB_EFI_BY_PROTOCOL          //通过协议
  };
typedef enum grub_efi_locate_search_type grub_efi_locate_search_type_t;

enum grub_efi_reset_type    //复位类型
  {
    GRUB_EFI_RESET_COLD,    //冷复位
    GRUB_EFI_RESET_WARM,    //热复位
    GRUB_EFI_RESET_SHUTDOWN //关机
  };
typedef enum grub_efi_reset_type grub_efi_reset_type_t;



#define GRUB_EFI_ERROR_CODE(value)	\
  ((((grub_efi_status_t) 1) << (sizeof (grub_efi_status_t) * 8 - 1)) | (value))		//32位: 1<<31 | value
//uefi错误代码值  error
#define GRUB_EFI_WARNING_CODE(value)	(value)

#define GRUB_EFI_SUCCESS		0																	//正常

#define GRUB_EFI_LOAD_ERROR		GRUB_EFI_ERROR_CODE (1)					//加载错误
#define GRUB_EFI_INVALID_PARAMETER	GRUB_EFI_ERROR_CODE (2)		//无效参数
#define GRUB_EFI_UNSUPPORTED		GRUB_EFI_ERROR_CODE (3)				//不支持
#define GRUB_EFI_BAD_BUFFER_SIZE	GRUB_EFI_ERROR_CODE (4)			//参数不是设备固有块大小的倍数
#define GRUB_EFI_BUFFER_TOO_SMALL	GRUB_EFI_ERROR_CODE (5)			//缓存太小
#define GRUB_EFI_NOT_READY		GRUB_EFI_ERROR_CODE (6)					//还没准备好
#define GRUB_EFI_DEVICE_ERROR		GRUB_EFI_ERROR_CODE (7)				//设备错误
#define GRUB_EFI_WRITE_PROTECTED	GRUB_EFI_ERROR_CODE (8)			//写保护
#define GRUB_EFI_OUT_OF_RESOURCES	GRUB_EFI_ERROR_CODE (9)			//资源不足
#define GRUB_EFI_VOLUME_CORRUPTED	GRUB_EFI_ERROR_CODE (10)		//卷损坏
#define GRUB_EFI_VOLUME_FULL		GRUB_EFI_ERROR_CODE (11)			//卷已满
#define GRUB_EFI_NO_MEDIA		GRUB_EFI_ERROR_CODE (12)					//没有媒体
#define GRUB_EFI_MEDIA_CHANGED		GRUB_EFI_ERROR_CODE (13)		//媒体变更
#define GRUB_EFI_NOT_FOUND		GRUB_EFI_ERROR_CODE (14)				//没有找到
#define GRUB_EFI_ACCESS_DENIED		GRUB_EFI_ERROR_CODE (15)		//拒绝访问
#define GRUB_EFI_NO_RESPONSE		GRUB_EFI_ERROR_CODE (16)			//没有反应
#define GRUB_EFI_NO_MAPPING		GRUB_EFI_ERROR_CODE (17)				//没有映射
#define GRUB_EFI_TIMEOUT		GRUB_EFI_ERROR_CODE (18)					//超时
#define GRUB_EFI_NOT_STARTED		GRUB_EFI_ERROR_CODE (19)			//没有开始
#define GRUB_EFI_ALREADY_STARTED	GRUB_EFI_ERROR_CODE (20)		//已经开始
#define GRUB_EFI_ABORTED		GRUB_EFI_ERROR_CODE (21)					//已中止
#define GRUB_EFI_ICMP_ERROR		GRUB_EFI_ERROR_CODE (22)				//ICMP错误
#define GRUB_EFI_TFTP_ERROR		GRUB_EFI_ERROR_CODE (23)				//TFTP错误
#define GRUB_EFI_PROTOCOL_ERROR		GRUB_EFI_ERROR_CODE (24)		//协议错误
#define GRUB_EFI_INCOMPATIBLE_VERSION	GRUB_EFI_ERROR_CODE (25)//不兼容的版本
#define GRUB_EFI_SECURITY_VIOLATION	GRUB_EFI_ERROR_CODE (26)	//安全违规
#define GRUB_EFI_CRC_ERROR		GRUB_EFI_ERROR_CODE (27)				//CRC错误

#define GRUB_EFI_WARN_UNKNOWN_GLYPH	GRUB_EFI_WARNING_CODE (1)				//警告		未知字形
#define GRUB_EFI_WARN_DELETE_FAILURE	GRUB_EFI_WARNING_CODE (2)			//警告		删除失败
#define GRUB_EFI_WARN_WRITE_FAILURE	GRUB_EFI_WARNING_CODE (3)				//警告		写失败
#define GRUB_EFI_WARN_BUFFER_TOO_SMALL	GRUB_EFI_WARNING_CODE (4)		//警告		缓存太小


typedef void *grub_efi_event_t;
typedef grub_efi_uint64_t grub_efi_lba_t;
typedef grub_efi_uintn_t grub_efi_tpl_t;
typedef grub_uint8_t grub_efi_mac_address_t[32];
typedef grub_uint8_t grub_efi_ipv4_address_t[4];
typedef grub_uint16_t grub_efi_ipv6_address_t[8];
//typedef grub_uint8_t grub_efi_ip_address_t[8] __attribute__ ((aligned(4)));
typedef union
{
  grub_efi_uint32_t addr[4];
  grub_efi_ipv4_address_t v4;
  grub_efi_ipv6_address_t v6;
} grub_efi_ip_address_t __attribute__ ((aligned(4)));
typedef grub_efi_uint64_t grub_efi_physical_address_t;
typedef grub_efi_uint64_t grub_efi_virtual_address_t;

struct grub_efi_packed_guid //efi包
{
  unsigned int data1;
  unsigned short data2;
  unsigned short data3;
  unsigned char data4[8];
} __attribute__ ((packed));
typedef struct grub_efi_packed_guid grub_efi_packed_guid_t;

/* XXX although the spec does not specify the padding, this actually  虽然规格没有指定填充，这一定要有填充物！
   must have the padding!  */
struct grub_efi_memory_descriptor   //内存描述符
{
  unsigned int type;                        //类型          07
  unsigned int padding;                     //填充          00
  unsigned long long physical_start;  			//物理地址起始  0
  unsigned long long virtual_start;     		//虚拟地址起始  0
  unsigned long long num_pages;       			//页数          a0
  unsigned long long attribute;            	//属性          0f
} __attribute__ ((packed));
typedef struct grub_efi_memory_descriptor grub_efi_memory_descriptor_t;	//28(按指定)

#define NEXT_MEMORY_DESCRIPTOR(desc, size)	\
  ((grub_efi_memory_descriptor_t *) ((char *) (desc) + (size)))   //下一个内存描述符(描述符,尺寸)

/* Device Path definitions. 设备路径定义 */
struct grub_efi_device_path
{
  unsigned char type;    //类型    04				7f结束
  unsigned char subtype; //子类型  04				ff结束  01结束设备路径的此实例并启动新的设备路径
  unsigned short length; //长度    0030  \EFI\BOOT\BOOTX64.EFI	此结构的长度(字节)。长度为4字节。
} __attribute__ ((packed));
typedef struct grub_efi_device_path grub_efi_device_path_t;
/* XXX EFI does not define EFI_DEVICE_PATH_PROTOCOL but uses it.  EFI不定义EFI_DEVICE_PATH_PROTOCOL协议，而是使用它. 这似乎相同于EFI_DEVICE_PATH.
   It seems to be identical to EFI_DEVICE_PATH.  */
typedef struct grub_efi_device_path grub_efi_device_path_protocol_t;

#define GRUB_EFI_DEVICE_PATH_TYPE(dp)		((dp)->type & 0x7f)
#define GRUB_EFI_DEVICE_PATH_SUBTYPE(dp)	((dp)->subtype)
#define GRUB_EFI_DEVICE_PATH_LENGTH(dp)		((dp)->length)

/* The End of Device Path nodes. 设备路径节点的结束 */
#define GRUB_EFI_END_DEVICE_PATH_TYPE			(0xff & 0x7f)

#define GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE		0xff
#define GRUB_EFI_END_THIS_DEVICE_PATH_SUBTYPE		0x01

#define GRUB_EFI_END_ENTIRE_DEVICE_PATH(dp)	\
  (GRUB_EFI_DEVICE_PATH_TYPE (dp) == GRUB_EFI_END_DEVICE_PATH_TYPE \
   && (GRUB_EFI_DEVICE_PATH_SUBTYPE (dp) \
       == GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE))

#define GRUB_EFI_NEXT_DEVICE_PATH(dp)	\
  ((grub_efi_device_path_t *) ((char *) (dp) \
                               + GRUB_EFI_DEVICE_PATH_LENGTH (dp)))

/* Hardware Device Path. 硬件设备路径类型 */
#define GRUB_EFI_HARDWARE_DEVICE_PATH_TYPE		1
//此设备路径定义设备如何连接到系统的资源域，其中资源域只是系统的共享内存、内存映射I/O和I/O空间。可以有多个级别的硬件设备路径，例如连接到PCCARD PCI控制器的PCCARD设备。 

//PCI设备路径子类型
#define GRUB_EFI_PCI_DEVICE_PATH_SUBTYPE		1
//PCI的设备路径定义PCI设备的PCI配置空间地址的路径。每个设备和函数号都有一个PCI设备路径条目，用于定义从根PCI总线到设备的路径。由于设备的PCI总线号可能会更改，因此无法使用单个PCI设备路径项的平面编码。例如，当PCI设备位于网桥后面时，会发生以下事件之一： 
//?操作系统执行PCI总线的即插即用配置。 
//?执行PCI设备的热插拔。 
//?系统配置在重新启动之间更改。 
//PCI设备路径条目前面必须有唯一标识PCI根总线的ACPI设备路径条目。根PCI桥的编程没有由任何PCI规范定义，这就是为什么需要ACPI设备路径条目的原因。

struct grub_efi_pci_device_path   //pci设备路径
{
  grub_efi_device_path_t header;  //设备路径表头
  unsigned char function;      //功能
  unsigned char device;        //设备
} __attribute__ ((packed));
typedef struct grub_efi_pci_device_path grub_efi_pci_device_path_t;
//PC卡设备路径子类型 
#define GRUB_EFI_PCCARD_DEVICE_PATH_SUBTYPE		2

struct grub_efi_pccard_device_path  //PC卡设备路径 
{
  grub_efi_device_path_t header;  //设备路径表头
  unsigned char function;      //功能
} __attribute__ ((packed));
typedef struct grub_efi_pccard_device_path grub_efi_pccard_device_path_t;
//内存映射设备路径子类型
#define GRUB_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE	3

struct grub_efi_memory_mapped_device_path //内存映射设备路径 
{
  grub_efi_device_path_t header;
  unsigned int memory_type;       //内存类型
  unsigned long long start_address;//物理起始地址
  unsigned long long end_address;  //物理结束地址
} __attribute__ ((packed));
typedef struct grub_efi_memory_mapped_device_path grub_efi_memory_mapped_device_path_t;
//供应商设备路径子类型
#define GRUB_EFI_VENDOR_DEVICE_PATH_SUBTYPE		4

struct grub_efi_vendor_device_path  //供应商设备路径
{
  grub_efi_device_path_t header;				//句柄
  grub_efi_packed_guid_t vendor_guid;		//供应商guid
  unsigned char vendor_defined_data[0];	//供应商定义的数据
} __attribute__ ((packed));
typedef struct grub_efi_vendor_device_path grub_efi_vendor_device_path_t;
//控制器设备路径子类型
#define GRUB_EFI_CONTROLLER_DEVICE_PATH_SUBTYPE		5

struct grub_efi_controller_device_path  //控制器设备路径 
{
  grub_efi_device_path_t header;        //设备路径表头
  unsigned int controller_number;  //控制器号
} __attribute__ ((packed));
typedef struct grub_efi_controller_device_path grub_efi_controller_device_path_t;

//ACPI设备路径类型
#define GRUB_EFI_ACPI_DEVICE_PATH_TYPE			2   //类型
//此设备路径包含表示设备的即插即用硬件ID及其对应的唯一持久ID的ACPI设备ID。ACPI ID存储在与设备关联的ACPI HID、CID和UID设备标识对象中。ACPI设备路径包含的值必须与平台固件提供给操作系统的ACPI名称空间完全匹配。有关HID、CID和UID设备标识对象的完整说明，请参阅ACPI规范。
//HID和CID值是出现在ACPI名称空间中的可选设备标识对象。如果只存在HID，则必须使用HID来描述将由ACPI驱动程序枚举的任何设备。如果存在，则CID包含对操作系统附加通用驱动程序（例如，PCI总线驱动程序）重要的信息，而HID包含对操作系统附加设备特定驱动程序重要的信息。ACPI总线驱动仅在设备没有标准总线枚举器的情况下枚举设备。
//UID对象为操作系统提供一个序列号样式的ID，用于在重新启动时不会更改的设备。对象是可选的，但当系统包含两个报告相同HID的设备时，该对象是必需的。UID只需在具有相同HID值的所有设备对象中唯一。如果在一个yHID的APCI名称空间中不存在UIID，则必须在ACPI设备路径的UID字段中存储零的值。
//ACPI设备路径仅用于描述不由硬件设备路径定义的设备。由于PCI规范没有定义PCI根桥的编程模型，因此需要一个HID（如果存在，还需要一个CID）来表示PCI根桥。ACPI设备路径有两个子类型：一个简单的子类型，仅包含HID和UID字段，另一个扩展的子类型，包含HID、CID和UID字段。 
//ACPI设备路径节点仅支持用于HID和UID值的32位数值。扩展的ACPI设备路径节点支持HID、UID和CID值的数值和字符串值。结果，ACPI设备路径节点较小，如果可能的话，应该使用它来减小可能存储在非易失性存储器中的设备路径的大小。如果HID字段需要字符串值，或者UID字段需要字符串值，或者CID字段需要字符串值，则必须使用扩展的ACPI设备路径节点。如果扩展的ACPI设备路径节点的字符串字段存在，则忽略相应的数值字段。 
//ACPI设备路径节点和扩展的ACPI设备路径节点中的HID和CID字段存储为32位压缩EISA类型id。以下宏可用于从即插即用硬件ID计算这些EISA类型ID。用于计算EFI设备路径节点中的HID和CID字段的即插即用硬件ID必须与用于在ACPI表中生成匹配项的即插即用硬件ID匹配。此宏生成的压缩EISA类型ID与存储在ACPI表中的压缩EISA类型ID不同。因此，来自ACPI设备路径节点的压缩EISA类型id不能直接与来自ACPI表的压缩EISA类型id进行比较。

//ACPI设备路径子类型
#define GRUB_EFI_ACPI_DEVICE_PATH_SUBTYPE		1   //子类型

struct grub_efi_acpi_device_path  //ACPI设备路径
{
  grub_efi_device_path_t header;  //设备路径表头
  unsigned int hid;
  unsigned int uid;
} __attribute__ ((packed));
typedef struct grub_efi_acpi_device_path grub_efi_acpi_device_path_t;
//扩展的ACPI设备路径子类型
#define GRUB_EFI_EXPANDED_ACPI_DEVICE_PATH_SUBTYPE	2

struct grub_efi_expanded_acpi_device_path //扩展的ACPI设备路径
{
  grub_efi_device_path_t header;        //设备路径表头
  unsigned int hid;
  unsigned int uid;
  unsigned int cid;
  char hidstr[0];
} __attribute__ ((packed));
typedef struct grub_efi_expanded_acpi_device_path grub_efi_expanded_acpi_device_path_t;

#define GRUB_EFI_EXPANDED_ACPI_HIDSTR(dp)	\
  (((grub_efi_expanded_acpi_device_path_t *) dp)->hidstr)
#define GRUB_EFI_EXPANDED_ACPI_UIDSTR(dp)	\
  (GRUB_EFI_EXPANDED_ACPI_HIDSTR(dp) \
   + grub_strlen (GRUB_EFI_EXPANDED_ACPI_HIDSTR(dp)) + 1)
#define GRUB_EFI_EXPANDED_ACPI_CIDSTR(dp)	\
  (GRUB_EFI_EXPANDED_ACPI_UIDSTR(dp) \
   + grub_strlen (GRUB_EFI_EXPANDED_ACPI_UIDSTR(dp)) + 1)

//消息传递设备路径类型
#define GRUB_EFI_MESSAGING_DEVICE_PATH_TYPE		3
//ATAPI设备路径子类型
#define GRUB_EFI_ATAPI_DEVICE_PATH_SUBTYPE		1

struct grub_efi_atapi_device_path //ATAPI设备路径
{
  grub_efi_device_path_t header;        //设备路径表头
  unsigned char primary_secondary;			//主次 
  unsigned char slave_master;						//从主
  unsigned short lun;										//逻辑单元
} __attribute__ ((packed));
typedef struct grub_efi_atapi_device_path grub_efi_atapi_device_path_t;
//SCSI设备路径子类型
#define GRUB_EFI_SCSI_DEVICE_PATH_SUBTYPE		2

struct grub_efi_scsi_device_path  //scsi设备路径
{
  grub_efi_device_path_t header;  //设备路径表头
  unsigned short pun;
  unsigned short lun;
} __attribute__ ((packed));
typedef struct grub_efi_scsi_device_path grub_efi_scsi_device_path_t;
//光纤通道设备路径子类型 
#define GRUB_EFI_FIBRE_CHANNEL_DEVICE_PATH_SUBTYPE	3

struct grub_efi_fibre_channel_device_path //光纤通道设备路径
{
  grub_efi_device_path_t header;  //设备路径表头
  unsigned int reserved;
  unsigned long long wwn;
  unsigned long long lun;
} __attribute__ ((packed));
typedef struct grub_efi_fibre_channel_device_path grub_efi_fibre_channel_device_path_t;
//1394设备路径子类型
#define GRUB_EFI_1394_DEVICE_PATH_SUBTYPE		4

struct grub_efi_1394_device_path  //1394设备路径
{
  grub_efi_device_path_t header;  //设备路径表头
  unsigned int reserved;
  unsigned long long guid;
} __attribute__ ((packed));
typedef struct grub_efi_1394_device_path grub_efi_1394_device_path_t;
//usb设备路径子类型
#define GRUB_EFI_USB_DEVICE_PATH_SUBTYPE		5

struct grub_efi_usb_device_path   //usb设备路径
{
  grub_efi_device_path_t header;			//设备路径表头
  unsigned char parent_port_number;  	//父端口号
  unsigned char usb_interface;       	//接口
} __attribute__ ((packed));
typedef struct grub_efi_usb_device_path grub_efi_usb_device_path_t;
//USB类设备路径子类型 
#define GRUB_EFI_USB_CLASS_DEVICE_PATH_SUBTYPE		15

struct grub_efi_usb_class_device_path //USB类设备路径
{
  grub_efi_device_path_t header;		//设备路径表头
  unsigned short vendor_id;					//厂商标识 
  unsigned short product_id;       	//产品标识 
  unsigned char device_class;     	//设备类
  unsigned char device_subclass;  	//设备子类
  unsigned char device_protocol;   	//设备协议
} __attribute__ ((packed));
typedef struct grub_efi_usb_class_device_path grub_efi_usb_class_device_path_t;
//I2O设备路径子类型
#define GRUB_EFI_I2O_DEVICE_PATH_SUBTYPE		6

struct grub_efi_i2o_device_path   //i2o设备路径
{
  grub_efi_device_path_t header;
  unsigned int tid;
} __attribute__ ((packed));
typedef struct grub_efi_i2o_device_path grub_efi_i2o_device_path_t;
//MAC地址设备路径子类型
#define GRUB_EFI_MAC_ADDRESS_DEVICE_PATH_SUBTYPE	11

struct grub_efi_mac_address_device_path   //MAC地址设备路径 
{
  grub_efi_device_path_t header;
  grub_efi_mac_address_t mac_address;
  unsigned char if_type;
} __attribute__ ((packed));
typedef struct grub_efi_mac_address_device_path grub_efi_mac_address_device_path_t;
//IPV4设备路径子类型
#define GRUB_EFI_IPV4_DEVICE_PATH_SUBTYPE		12

struct grub_efi_ipv4_device_path  //ipv4设备路径
{
  grub_efi_device_path_t header;
  unsigned char local_ip_address;
  unsigned char remote_ip_address;
  unsigned short local_port;
  unsigned short remote_port;
  unsigned short protocol;
  unsigned char static_ip_address;
} __attribute__ ((packed));
typedef struct grub_efi_ipv4_device_path grub_efi_ipv4_device_path_t;
//IPV6设备路径子类型
#define GRUB_EFI_IPV6_DEVICE_PATH_SUBTYPE		13

struct grub_efi_ipv6_device_path  //ipv6设备路径
{
  grub_efi_device_path_t header;
  unsigned short local_ip_address;
  unsigned short remote_ip_address;
  unsigned short local_port;
  unsigned short remote_port;
  unsigned short protocol;
  unsigned char static_ip_address;
} __attribute__ ((packed));
typedef struct grub_efi_ipv6_device_path grub_efi_ipv6_device_path_t;
//无限宽带设备路径子类型
#define GRUB_EFI_INFINIBAND_DEVICE_PATH_SUBTYPE		9

struct grub_efi_infiniband_device_path  //无限宽带设备路径
{
  grub_efi_device_path_t header;
  unsigned int resource_flags;
  unsigned char port_gid[16];
  unsigned long long remote_id;
  unsigned long long target_port_id;
  unsigned long long device_id;
} __attribute__ ((packed));
typedef struct grub_efi_infiniband_device_path grub_efi_infiniband_device_path_t;
//UART设备路径子类型
#define GRUB_EFI_UART_DEVICE_PATH_SUBTYPE		14

struct grub_efi_uart_device_path    //uart设备路径
{
  grub_efi_device_path_t header;
  unsigned int reserved;
  unsigned long long baud_rate;
  unsigned char data_bits;
  unsigned char parity;
  unsigned char stop_bits;
} __attribute__ ((packed));
typedef struct grub_efi_uart_device_path grub_efi_uart_device_path_t;
//SATA设备路径子类型
#define GRUB_EFI_SATA_DEVICE_PATH_SUBTYPE		18

struct grub_efi_sata_device_path   //sata设备路径
{
  grub_efi_device_path_t header;
  unsigned short hba_port;
  unsigned short multiplier_port;
  unsigned short lun;
} __attribute__ ((packed));
typedef struct grub_efi_sata_device_path grub_efi_sata_device_path_t;
//供应商信息设备路径子类型
#define GRUB_EFI_VENDOR_MESSAGING_DEVICE_PATH_SUBTYPE	10

//媒体设备路径类型
#define GRUB_EFI_MEDIA_DEVICE_PATH_TYPE			4
//此设备路径用于描述由引导服务抽象的介质部分。媒体设备路径的一个例子是定义硬盘上正在使用的分区。

//硬盘驱动器设备路径子类型 
#define GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE		1
//硬盘驱动器媒体设备路径用于表示硬盘驱动器上的分区。每个分区至少有一个硬盘驱动器设备路径节点，每个节点描述分区表中的一个条目。EFI支持MBR和GPT分区格式。分区根据它们在各自分区表中的条目进行编号，从1开始。分区从lba0开始在EFI中寻址。分区号为零可用于表示原始硬盘驱动器或原始扩展分区。
//分区格式存储在设备路径中，以便将来支持新的分区格式。硬盘驱动器设备路径还包含磁盘签名和磁盘签名类型。磁盘签名由操作系统维护，并且仅由EFI用于分区设备路径节点。磁盘签名使操作系统能够查找磁盘，即使磁盘已在系统中物理移动。
//第3.1.2节定义了处理硬盘驱动器媒体设备路径的特殊规则。这些特殊规则允许磁盘的位置更改，并且系统仍然可以从磁盘启动。 

struct grub_efi_hard_drive_device_path  //硬盘驱动器路径
{
  grub_efi_device_path_t header;					//设备路径表头
  unsigned int partition_number;					//分区号				描述分区表中的条目，从条目1开始。分区号0表示整个设备。MBR分区的有效分区号是[1-4],但是并不对应主分区号。GPT分区的有效分区号为[1，NumberOfPartitionEntries]。 
  unsigned long long partition_start;			//分区起始			启动硬盘上分区的LBA 
  unsigned long long partition_size;			//分区尺寸			以逻辑块为单位的分区大小 
  unsigned char partition_signature[16];	//分区签名 			此分区唯一的签名：如果SignatureType为0，则必须用16个零初始化此字段。 
																					//							如果SignatureType为1，则MBR签名存储在此字段的前4个字节中。其他12个字节用零初始化。 签名是1b8处的4字节.
																					//							如果SignatureType为2，则此字段包含16字节签名。签名是分区GUID.
  unsigned char partmap_type;							//分区格式类型	分区格式(保留未使用的值) 0x01–PC-AT兼容的传统MBR（见第5.2.1节）。分区开始和分区大小来自分区的PartitionStartingLBA和PartitionSizeInLBA。 
																					//							0x02–GUID分区表（请参阅第5.3.2节）。 
  unsigned char signature_type;						//签名类型			磁盘签名类型：(保留未使用的值) 0x00–没有磁盘签名。0x01–来自0x01 MBR类型的地址0x1b8的32位签名. 0x02–GUID签名。
} __attribute__ ((packed));
typedef struct grub_efi_hard_drive_device_path grub_efi_hard_drive_device_path_t;
//光盘驱动器路径子类型
#define GRUB_EFI_CDROM_DEVICE_PATH_SUBTYPE		2

struct grub_efi_cdrom_device_path   //光盘驱动器路径
{
  grub_efi_device_path_t header;				//设备路径表头
  unsigned int boot_entry;							//引导入口	引导目录中的引导条目号。初始/默认项定义为零。				0
  unsigned long long boot_start;        //引导起始	引导映像文件的扇区地址。														12b
  unsigned long long boot_size;         //引导尺寸	引导时装入内存的映像文件扇区数(每扇区按0x200字节)		4
} __attribute__ ((packed));
typedef struct grub_efi_cdrom_device_path grub_efi_cdrom_device_path_t;
//多媒体设备路径子类型
#define GRUB_EFI_VENDOR_MEDIA_DEVICE_PATH_SUBTYPE	3

struct grub_efi_vendor_media_device_path  //多媒体设备路径
{
  grub_efi_device_path_t header;
  grub_efi_packed_guid_t vendor_guid;
  unsigned char vendor_defined_data[0];
} __attribute__ ((packed));
typedef struct grub_efi_vendor_media_device_path grub_efi_vendor_media_device_path_t;
//文件路径设备路径子类型
#define GRUB_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE		4

struct grub_efi_file_path_device_path   //文件路径设备路径
{
  grub_efi_device_path_t header;		//设备路径表头
  unsigned short path_name[0];			//路径名称
} __attribute__ ((packed));
typedef struct grub_efi_file_path_device_path grub_efi_file_path_device_path_t;
//协议设备路径子类型
#define GRUB_EFI_PROTOCOL_DEVICE_PATH_SUBTYPE		5

struct grub_efi_protocol_device_path  //协议设备路径 
{
  grub_efi_device_path_t header;
  grub_efi_packed_guid_t guid;
} __attribute__ ((packed));
typedef struct grub_efi_protocol_device_path grub_efi_protocol_device_path_t;
//PIWG设备路径子类型
#define GRUB_EFI_PIWG_DEVICE_PATH_SUBTYPE		6

struct grub_efi_piwg_device_path    //PIWG设备路径
{
  grub_efi_device_path_t header;
  grub_efi_packed_guid_t guid;
} __attribute__ ((packed));
typedef struct grub_efi_piwg_device_path grub_efi_piwg_device_path_t;


//BIOS设备路径类型
#define GRUB_EFI_BIOS_DEVICE_PATH_TYPE			5
//BIOS设备路径子类型
#define GRUB_EFI_BIOS_DEVICE_PATH_SUBTYPE		1

struct grub_efi_bios_device_path  //BIOS设备路径
{
  grub_efi_device_path_t header;	//设备路径表头
  unsigned short device_type;			//设备类型
  unsigned short status_flags;		//状态标志
  char description[0];
} __attribute__ ((packed));
typedef struct grub_efi_bios_device_path grub_efi_bios_device_path_t;

//设备路径转文本协议
//struct EFI_DEVICE_PATH_TO_TEXT_PROTOCOL
struct grub_efi_device_to_text_protocol
{
  //创建设备节点到文本
  unsigned short* EFIAPI 
  (*ConvertDeviceNodeToText)(                                 //PciRoot(0x0)
    grub_efi_device_path_protocol_t *DeviceNode,              //设备节点
    grub_int8_t DisplayOnly,                                  //仅显示
    grub_int8_t AllowShortcuts);                              //允许快捷方式

  //创建设备路径到文本
  unsigned short* EFIAPI
  (*ConvertDevicePathToText)(                                 //PciRoot(0x0)/Pci(0x1,0x1)/Ata(Primary,Master,0x0)/HD(1,MBR,0x12efcdab,0x3f,0x7cfc1)
    grub_efi_device_path_protocol_t *DevicePath,              //设备路径
    grub_int8_t DisplayOnly,                                  //仅显示
    grub_int8_t AllowShortcuts);                              //允许快捷方式
};
typedef struct grub_efi_device_to_text_protocol grub_efi_device_to_text_protocol_t;

//文本转设备路径协议
//将其转换为一个分配给它的二进制表示的缓冲区。 
//内存是从EFI引导服务内存分配的。调用者有责任释放分配的内存。
//struct EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL
struct grub_efi_device_pate_from_text_protocol
{
  //将文本转换为设备节点的二进制表示形式。
  grub_efi_device_path_protocol_t *EFIAPI (*ConvertTextToDeviceNode)(
        const unsigned short *TextDeviceNode);  //指向设备节点的文本表示形式。转换从第一个字符开始，一直持续到第一个非设备节点字符。

  //将文本转换为设备路径的二进制表示形式。 
  grub_efi_device_path_protocol_t *EFIAPI (*ConvertTextToDevicePath)(
        const unsigned short *TextDevicePath);  //指向设备路径的文本表示形式。转换从第一个字符开始，一直持续到第一个非设备路径字符。
};

//设备路径实用程序协议
//struct EFI_DEVICE_PATH_UTILITIES_PROTOCOL
struct grub_efi_device_pate_utilities_protocol
{
  //获得设备路径尺寸
  //此函数返回指定设备路径的尺寸(以字节为单位)，包括路径结束标记。如果DevicePath为NULL，则返回零。
  grub_efi_uintn_t *EFIAPI (* GetDevicePathSize)(
      grub_efi_device_path_protocol_t *DevicePath);   //指向EFI设备路径的起点。

  //创建指定路径的副本
  //此函数用于创建指定设备路径的副本。内存是从EFI引导服务内存分配的。调用者有责任释放分配的内存。如果DevicePath为NULL，则返回NULL，并且不分配内存。
  //此函数返回指向重复设备路径的指针，如果内存不足，则返回NULL。
  grub_efi_device_path_protocol_t *EFIAPI (* DuplicateDevicePath)(
      grub_efi_device_path_protocol_t *DevicePath);   //指向源设备路径 
      
  //附加设备路径  通过将第二个设备路径附加到第一个设备路径来创建新路径。
  //此函数通过将第二个设备路径的副本附加到新分配的缓冲区中第一个设备路径的副本来创建新的设备路径。只保留第二个设备路径中的设备路径末端设备节点。
  //如果Src1为NULL而Src2为非NULL，则返回Src2的副本。如果Src1为非NULL且Src2为NULL，则返回Src1的副本。如果Src1和Src2都为NULL，则返回设备结束路径的副本。
  //内存是从EFI引导服务内存分配的。调用者有责任释放分配的内存。 
  //此函数返回指向新创建的设备路径的指针，如果无法分配内存，则返回NULL。 
  grub_efi_device_path_protocol_t *EFIAPI (* AppendDevicePath)(
      const grub_efi_device_path_protocol_t *Src1,    //指向第一个设备路径。
      const grub_efi_device_path_protocol_t *Src2);   //指向第二个设备路径。

  //附加设备节点  通过将设备节点附加到设备路径来创建新路径。 
  //函数通过将指定设备节点的副本附加到分配的缓冲区中指定设备路径的副本来创建新的设备路径。设备路径末端设备节点移动到附加设备节点的末尾之后。
  //如果DeviceNode为空，则返回DevicePath的副本。如果DevicePath为空，则返回DeviceNode的副本，后跟设备路径的末尾device节点。
  //如果DeviceNode和DevicePath都为NULL，则返回设备路径末端设备节点的副本。
  //内存是从EFI引导服务内存分配的。调用者有责任释放分配的内存。 
  //此函数返回指向分配的设备路径的指针，如果内存不足，则返回NULL。
  grub_efi_device_path_protocol_t *EFIAPI (* AppendDeviceNode)(
      const grub_efi_device_path_protocol_t *DevicePath,  //指向设备路径。
      const grub_efi_device_path_protocol_t *DeviceNode); //指向设备节点。
    
  //附加设备路径实例  通过将指定的设备路径实例附加到指定的设备路径来创建新路径。
  //此函数通过将指定设备路径实例的副本附加到已分配缓冲区中指定设备路径的副本来创建新的设备路径。设备路径末端设备节点移动到附加设备节点的末尾之后，
  //并在两者之间插入新的设备路径结束实例节点。如果DevicePath为NULL，则返回一个如果DevicePath实例的副本.
  //内存是从EFI引导服务内存分配的。调用者有责任释放分配的内存。 
  //此函数返回一个指向新创建的设备路径的指针，如果devicepathnance为NULL或内存不足，则返回NULL 
  grub_efi_device_path_protocol_t *EFIAPI (* AppendDevicePathInstance)(
      const grub_efi_device_path_protocol_t *DevicePath,          //指向设备路径。如果为空，则忽略 
      const grub_efi_device_path_protocol_t *DevicePathInstance); //指向设备路径实例 
    
  //获得下一个设备路径实例  创建当前设备路径实例的副本，并返回指向下一个设备路径实例的指针。 
  //此函数用于创建当前设备路径实例的副本。它还将更新devicepathnistance以指向设备路径中的下一个设备路径实例（如果不再存在，则为NULL），
  //并更新DevicePathInstanceSize以保留设备路径实例副本的大小。 
  //内存是从EFI引导服务内存分配的。调用者有责任释放分配的内存。
  //此函数返回指向当前设备路径实例副本的指针，如果条目上的DevicePathInstance为NULL或内存不足，则返回NULL。
  grub_efi_device_path_protocol_t *EFIAPI (* GetNextDevicePathInstance)(
      grub_efi_device_path_protocol_t **DevicePathInstance, //设备路径实例  在输入时，它保存指向当前设备路径实例的指针。在输出时，它保留指向下一个设备路径实例的指针，如果设备路径中没有更多的设备路径实例，则为空。
      grub_efi_uintn_t *DevicePathInstanceSize);   //设备路径实例尺寸(可选)  在输出时，如果DevicePathInstance为NULL，它将保存设备路径实例的大小（以字节为单位）或零。如果为空，则不输出实例大小。
    
  //返回设备路径是否为多实例。 此函数返回指定的设备路径是否具有多个路径实例。
  //如果设备路径有多个实例，则此函数返回TRUE；如果设备路径为空或仅包含单个实例，则此函数返回FALSE。 
  grub_int8_t *(* IsDevicePathMultiInstance)(
      const grub_efi_device_path_protocol_t *DevicePath); //指向设备路径。如果为空，则忽略.

  //创建设备节点
  //此函数用于在新分配的缓冲区中创建新的设备节点。 内存是从EFI引导服务内存分配的。调用者有责任释放分配的内存。 
  //此函数返回指向创建的设备节点的指针，如果NodeLength小于头的大小或内存不足，则返回NULL。
  grub_efi_device_path_protocol_t *EFIAPI (* CreateDeviceNode)(
      unsigned char NodeType,     //节点类型
      unsigned char NodeSubType,  //节点子类型
      unsigned short NodeLength); //节点尺寸
};
typedef struct grub_efi_device_pate_utilities_protocol grub_efi_device_pate_utilities_protocol_t;
 

struct grub_efi_open_protocol_information_entry //打开协议信息入口
{
  void * agent_handle;			//代理句柄
  void * controller_handle;	//控制器句柄 
  unsigned int attributes;	//属性 
  unsigned int open_count;	//打开计数 
};
typedef struct grub_efi_open_protocol_information_entry grub_efi_open_protocol_information_entry_t;

struct grub_efi_time  //时间
{
  unsigned short year;				//年	1900-9999
  unsigned char month;				//月	1-12
  unsigned char day;					//日	1-31
  unsigned char hour;					//时	0-23
  unsigned char minute;				//分	0-59
  unsigned char second;				//秒	0-59
  unsigned char pad1;
  unsigned int nanosecond;		//毫微秒	0-999999999		1秒=10^9=1000000000毫微秒   1毫秒=10^6=1000000毫微秒   有些主板不支持,返回0!!!
  short time_zone;						//时区	-1440 到 1440		UTC - TimeZone
  unsigned char daylight;			//夏令时
  unsigned char pad2;
} __attribute__ ((packed));
typedef struct grub_efi_time grub_efi_time_t;

struct grub_efi_time_capabilities //时间能力
{
  unsigned int resolution;   //分辨率
  unsigned int accuracy;     //精度
  char sets_to_zero;					//设置为零
};
typedef struct grub_efi_time_capabilities grub_efi_time_capabilities_t;

struct grub_efi_input_key   //输入键
{
  unsigned short scan_code;    //扫描码
  unsigned short unicode_char; //unicode字符
};
typedef struct grub_efi_input_key grub_efi_input_key_t;	//4(按指定)

typedef unsigned char grub_efi_key_toggle_state_t;
struct grub_efi_key_state //键状态
{
	unsigned int key_shift_state;            		//键移位状态	反映输入设备当前按下的移位修饰符。仅当已设置高阶位时，返回值才有效。
	grub_efi_key_toggle_state_t key_toggle_state; //键切换状态	反映各种切换属性的当前内部状态。仅当已设置高阶位时，返回值才有效。
};
typedef struct grub_efi_key_state grub_efi_key_state_t;

//EFI键移位状态
#define GRUB_EFI_SHIFT_STATE_VALID     0x80000000	//键移位状态有效
#define GRUB_EFI_RIGHT_SHIFT_PRESSED   0x00000001	//右SHIFT
#define GRUB_EFI_LEFT_SHIFT_PRESSED    0x00000002	//左SHIFT
#define GRUB_EFI_RIGHT_CONTROL_PRESSED 0x00000004	//右CONTROL
#define GRUB_EFI_LEFT_CONTROL_PRESSED  0x00000008	//左CONTROL
#define GRUB_EFI_RIGHT_ALT_PRESSED     0x00000010	//右ALT
#define GRUB_EFI_LEFT_ALT_PRESSED      0x00000020	//左ALT
#define GRUB_EFI_RIGHT_LOGO_PRESSED    0x00000040	//右LOGO
#define GRUB_EFI_LEFT_LOGO_PRESSED     0x00000080	//左LOGO
#define GRUB_EFI_MENU_KEY_PRESSED      0x00000100	//菜单键
#define GRUB_EFI_SYS_REQ_PRESSED       0x00000200	//系统键
//EFI键切换状态
#define GRUB_EFI_TOGGLE_STATE_VALID 0x80	//键切换状态有效
#define GRUB_EFI_KEY_STATE_EXPOSED  0x40	//按键状态曝光
#define GRUB_EFI_SCROLL_LOCK_ACTIVE 0x01	//滚动锁定有效
#define GRUB_EFI_NUM_LOCK_ACTIVE    0x02	//数字锁定有效
#define GRUB_EFI_CAPS_LOCK_ACTIVE   0x04	//大写锁定有效

struct grub_efi_simple_text_output_mode //简单文本输出模式
{
  grub_efi_int32_t max_mode;          //最大模式
  grub_efi_int32_t mode;              //模式
  grub_efi_int32_t attribute;         //属性
  grub_efi_int32_t cursor_column;     //光标列
  grub_efi_int32_t cursor_row;        //光标行
  grub_efi_boolean_t cursor_visible;  	//光标可见属性
};
typedef struct grub_efi_simple_text_output_mode grub_efi_simple_text_output_mode_t;	//18(long按4,最后一行char按4对齐)

/* Tables.  */
struct grub_efi_table_header  //表头
{
  unsigned long long signature;    //签名      IBI SYST
  unsigned int revision;     //修订      020028
  unsigned int header_size;  //标题大小  78
  unsigned int crc32;        //crc32     7D66830F
  unsigned int reserved;     //保留      0
};
typedef struct grub_efi_table_header grub_efi_table_header_t;

struct grub_efi_block_io_media  //块输入输出介质
{
  grub_efi_uint32_t media_id;             //媒体标识    1						1f							1f		当前媒体ID。如果媒体更改，则此值将更改
  grub_efi_boolean_t removable_media;     //可移动媒体  0   				0								0			如果媒体是可移动的，则为TRUE；否则为FALSE 
  grub_efi_boolean_t media_present;       //媒体目前    1						1								1			如果设备中当前存在媒体，则为TRUE；否则为FALSE。此字段显示最近一次ReadBlocks（）或WriteBlocks（）调用时的媒体当前状态
  grub_efi_boolean_t logical_partition;   //逻辑分区    0						0								0			如果生成EFI_BLOCK_IO_协议以抽象磁盘上的分区结构，则为TRUE。如果生成块IO协议以提取硬件设备上的逻辑块，则为FALSE。
  grub_efi_boolean_t read_only;           //只读        0						0								0			如果媒体标记为只读，则为TRUE；否则为FALSE。此字段显示最近一次WriteBlocks（）调用的只读状态。
  grub_efi_boolean_t write_caching;       //写缓存      0						0								0			如果WriteBlocks（）函数缓存写入数据，则为TRUE。
  grub_efi_uint8_t pad[3];                //填充
  grub_efi_uint32_t block_size;           //块尺寸      200					200							200		设备的内部块大小。如果媒体更改，则更新此字段。返回每个逻辑块的字节数。
  grub_efi_uint32_t io_align;             //IO对齐      4						4								0			提供数据传输中使用的任何缓冲区的对齐要求。IoAlign值为0和1意味着缓冲区可以放在内存中的任何位置。否则，IoAlign必须是2的幂，并且要求缓冲区的起始地址必须可以被IoAlign均匀地整除，并且不能有余数。 
  grub_efi_uint8_t pad2[4];               //填充
  grub_efi_lba_t last_block;               //最后块      1fffff(1G)	3a38602f(465G)	1e45ff(968M)  设备上的最后一个LBA,加1等于总扇区数    这是指分区(卷)的尺寸   
};
typedef struct grub_efi_block_io_media grub_efi_block_io_media_t;	//20(按指定)

struct block_io_protocol	//块io协议
{
  grub_efi_uint64_t revision;				//修订 
  grub_efi_block_io_media_t *media;	//媒体
  grub_efi_status_t (EFIAPI *reset) (struct block_io_protocol *this,
			      grub_efi_boolean_t extended_verification);	//复位
  grub_efi_status_t (EFIAPI *read_blocks) (struct block_io_protocol *this,
				    grub_efi_uint32_t media_id,
				    grub_efi_lba_t lba,
				    grub_efi_uintn_t buffer_size,
				    void *buffer);					//读块
  grub_efi_status_t (EFIAPI *write_blocks) (struct block_io_protocol *this,
				     grub_efi_uint32_t media_id,
				     grub_efi_lba_t lba,
				     grub_efi_uintn_t buffer_size,
				     void *buffer);					//写块
  grub_efi_status_t (EFIAPI *flush_blocks) (struct block_io_protocol *this);	//填充
};
typedef struct block_io_protocol block_io_protocol_t;

struct grub_efi_boot_services   //引导服务
{
  grub_efi_table_header_t hdr;  //表头

  grub_efi_tpl_t EFIAPI
  (*raise_tpl) (grub_efi_tpl_t new_tpl);    //枚举

  void EFIAPI
  (*restore_tpl) (grub_efi_tpl_t old_tpl);  //恢复

  grub_efi_status_t EFIAPI
  (*allocate_pages) (grub_efi_allocate_type_t type,
		     grub_efi_memory_type_t memory_type,
		     grub_efi_uintn_t pages,
		     grub_efi_physical_address_t *memory);  //分配页

  grub_efi_status_t EFIAPI
  (*free_pages) (grub_efi_physical_address_t memory,
		 grub_efi_uintn_t pages); //释放页

  grub_efi_status_t EFIAPI
  (*get_memory_map) (grub_efi_uintn_t *memory_map_size,
		     grub_efi_memory_descriptor_t *memory_map,
		     grub_efi_uintn_t *map_key,
		     grub_efi_uintn_t *descriptor_size,
		     grub_efi_uint32_t *descriptor_version);  //获得内存映射

  grub_efi_status_t EFIAPI
  (*allocate_pool) (grub_efi_memory_type_t pool_type,
		    grub_efi_uintn_t size,
		    void **buffer); //分配池

  grub_efi_status_t EFIAPI
  (*free_pool) (void *buffer);  //释放池

  //创建事件
  grub_efi_status_t EFIAPI
  (*create_event) (grub_efi_uint32_t type,  //事件的类型
		   grub_efi_tpl_t notify_tpl,           //事件的优先级
		   void (*notify_function) (grub_efi_event_t event,
					    void *context),               //事件处理函数
		   void *notify_context,                //传递给事件处理函数的参数
		   grub_efi_event_t *event);            //创建的事件

  grub_efi_status_t EFIAPI
  (*set_timer) (grub_efi_event_t event,
		grub_efi_timer_delay_t type,
		grub_efi_uint64_t trigger_time);  //设置时间
    
   //等待事件
   grub_efi_status_t EFIAPI
   (*wait_for_event) (grub_efi_uintn_t num_events,  //第二个参数Event中的事件数量
		      grub_efi_event_t *event,                  //所要等待的事件数组
		      grub_efi_uintn_t *index);                 //指向满足等待条件的事件索引的指针

  grub_efi_status_t EFIAPI
  (*signal_event) (grub_efi_event_t event); //信号事件

  grub_efi_status_t EFIAPI
  (*close_event) (grub_efi_event_t event);  //关闭事件

  grub_efi_status_t EFIAPI
  (*check_event) (grub_efi_event_t event);  //检查事件

  grub_efi_status_t EFIAPI
  (*install_protocol_interface) (grub_efi_handle_t *handle,
				  grub_efi_guid_t *protocol,
				  grub_efi_interface_type_t protocol_interface_type,
				  void *protocol_interface);  //安装协议接口

  grub_efi_status_t EFIAPI
  (*reinstall_protocol_interface) (grub_efi_handle_t handle,
				   grub_efi_guid_t *protocol,
				   void *old_interface,
				   void *new_interface);  //重新安装协议接口

  grub_efi_status_t EFIAPI
  (*uninstall_protocol_interface) (grub_efi_handle_t handle,
				   grub_efi_guid_t *protocol,
				   void *protocol_interface); //卸载协议接口

  grub_efi_status_t EFIAPI
  (*handle_protocol) (grub_efi_handle_t handle,
		      grub_efi_guid_t *protocol,
		      void **protocol_interface); //句柄协议

  void *reserved;

  grub_efi_status_t EFIAPI
  (*register_protocol_notify) (grub_efi_guid_t *protocol,
			       grub_efi_event_t event,
			       void **registration);  //注册协议通知

  grub_efi_status_t EFIAPI
  (*locate_handle) (grub_efi_locate_search_type_t search_type,
		    grub_efi_guid_t *protocol,
		    void *search_key,
		    grub_efi_uintn_t *buffer_size,
		    grub_efi_handle_t *buffer); //定位句柄

  grub_efi_status_t EFIAPI
  (*locate_device_path) (grub_efi_guid_t *protocol,
			 grub_efi_device_path_t **device_path,
			 grub_efi_handle_t *device);  //定位设备路径

  grub_efi_status_t EFIAPI
  (*install_configuration_table) (grub_efi_guid_t *guid, void *table);  //安装配置表 

  grub_efi_status_t EFIAPI
  (*load_image) (grub_efi_boolean_t boot_policy,
		 grub_efi_handle_t parent_image_handle,
		 grub_efi_device_path_t *file_path,
		 void *source_buffer,
		 grub_efi_uintn_t source_size,
		 grub_efi_handle_t *image_handle);  //加载映像

  grub_efi_status_t EFIAPI
  (*start_image) (grub_efi_handle_t image_handle,
		  grub_efi_uintn_t *exit_data_size,
		  grub_efi_char16_t **exit_data);   //启动映像

  grub_efi_status_t EFIAPI
  (*exit) (grub_efi_handle_t image_handle,
	   grub_efi_status_t exit_status,
	   grub_efi_uintn_t exit_data_size,
	   grub_efi_char16_t *exit_data) __attribute__((noreturn)); //退出

  grub_efi_status_t EFIAPI
  (*unload_image) (grub_efi_handle_t image_handle); //卸载映像

  grub_efi_status_t EFIAPI
  (*exit_boot_services) (grub_efi_handle_t image_handle,
			 grub_efi_uintn_t map_key); //退出引导服务

  grub_efi_status_t EFIAPI
  (*get_next_monotonic_count) (grub_efi_uint64_t *count); //获得下一单调计数

  grub_efi_status_t EFIAPI
  (*stall) (grub_efi_uintn_t microseconds); //失速

  grub_efi_status_t EFIAPI
  (*set_watchdog_timer) (grub_efi_uintn_t timeout,
			 grub_efi_uint64_t watchdog_code,
			 grub_efi_uintn_t data_size,
			 grub_efi_char16_t *watchdog_data); //设置看门狗定时器

  //连接控制器  将一个或多个驱动程序连接到控制器
  grub_efi_status_t EFIAPI
  (*connect_controller) (grub_efi_handle_t controller_handle,	//要连接驱动程序的控制器的句柄
			 grub_efi_handle_t *driver_image_handle,	//指向支持EFI驱动程序绑定协议的有序列表句柄的指针。列表以空句柄值终止。这些句柄是驱动程序绑定协议的候选者，该协议将管理由ControllerHandle指定的控制器。 
			 grub_efi_device_path_protocol_t *remaining_device_path,	//指向设备路径的指针，指定由ControllerHandle指定的控制器的子级。
			 grub_efi_boolean_t recursive);	//如果为true，则递归调用ConnectController（），直到ControllerHandle指定的控制器下的整个控制器树都已创建。如果为false，则控制器树仅展开一个级别。

  //断开控制器	从控制器断开一个或多个驱动程序
  grub_efi_status_t EFIAPI
  (*disconnect_controller) (grub_efi_handle_t controller_handle,	//要连接驱动程序的控制器的句柄
			    grub_efi_handle_t driver_image_handle,	//从控制器手柄上断开的驱动程序。如果DriverImageHandle为空，则当前管理ControllerHandle的所有驱动程序都与ControllerHandle断开连接。 
			    grub_efi_handle_t child_handle);	//要摧毁子级的句柄。如果child_handle为空，则在驱动程序与controllerhandle断开连接之前，controllerhandle的所有子级都将被销毁。 

  grub_efi_status_t EFIAPI
  (*open_protocol) (grub_efi_handle_t handle,
		    grub_efi_guid_t *protocol,
		    void **protocol_interface,
		    grub_efi_handle_t agent_handle,
		    grub_efi_handle_t controller_handle,
		    grub_efi_uint32_t attributes);  //打开协议

  grub_efi_status_t EFIAPI
  (*close_protocol) (grub_efi_handle_t handle,
		     grub_efi_guid_t *protocol,
		     grub_efi_handle_t agent_handle,
		     grub_efi_handle_t controller_handle);  //关闭协议

  grub_efi_status_t EFIAPI
  (*open_protocol_information) (grub_efi_handle_t handle,
				grub_efi_guid_t *protocol,
				grub_efi_open_protocol_information_entry_t **entry_buffer,
				grub_efi_uintn_t *entry_count); //打开协议信息

  grub_efi_status_t EFIAPI
  (*protocols_per_handle) (grub_efi_handle_t handle,
			   grub_efi_packed_guid_t ***protocol_buffer,
			   grub_efi_uintn_t *protocol_buffer_count);  //每个句柄协议

  grub_efi_status_t EFIAPI
  (*locate_handle_buffer) (grub_efi_locate_search_type_t search_type,
			   grub_efi_guid_t *protocol,
			   void *search_key,
			   grub_efi_uintn_t *no_handles,
			   grub_efi_handle_t **buffer); //定位句柄缓冲

  grub_efi_status_t EFIAPI
  (*locate_protocol) (grub_efi_guid_t *protocol,
		      void *registration,
		      void **protocol_interface); //定位协议

  //安装多协议接口	将一个或多个协议接口安装到启动服务环境中  
  grub_efi_status_t EFIAPI
  (*install_multiple_protocol_interfaces) (grub_efi_handle_t *handle,	//指向协议接口的指针
					grub_efi_guid_t *guid,				//指向协议GUID的指针
					grub_efi_device_path_t *dp,		//指向设备路径的指针
					grub_efi_guid_t *blk_io_guid,	//指向io设备接口的指针
					block_io_protocol_t *block_io,//指向block_io设备接口的指针
					void* data);

  grub_efi_status_t EFIAPI
  (*uninstall_multiple_protocol_interfaces)(grub_efi_handle_t *handle,	//指向协议接口的指针
					grub_efi_guid_t *guid,				//指向协议GUID的指针
					grub_efi_device_path_t *dp,		//指向设备路径的指针
					grub_efi_guid_t *blk_io_guid,	//指向io设备接口的指针
					block_io_protocol_t *block_io,//指向block_io设备接口的指针
					void* data);

  grub_efi_status_t EFIAPI
  (*calculate_crc32) (void *data,
		      grub_efi_uintn_t data_size,
		      grub_efi_uint32_t *crc32);  //计算crc32 

  void EFIAPI
  (*copy_mem) (void *destination, void *source, grub_efi_uintn_t length); //复制内存(目的,源,长度)

  void EFIAPI
  (*set_mem) (void *buffer, grub_efi_uintn_t size, grub_efi_uint8_t value);//设置内存(缓存,尺寸,值)
};
typedef struct grub_efi_boot_services grub_efi_boot_services_t;	//170(表头按18,结构及void按8,其余按指定)		
																																//g4d:占c4(结构按指定,其余不论'long long'或者'void'均按4)

struct grub_efi_runtime_services  //运行时服务
{
  grub_efi_table_header_t hdr;

/*获得时间	返回硬件平台的当前时间和日期信息以及时间保持功能。
getTime（）函数返回在调用函数期间某个时间有效的时间。虽然返回的EFI U时间结构包含时区和夏令时信息，但实际时钟不维护这些值。getTime（）返回的当前时区和夏令时信息是最后通过setTime（）设置的值。 
getTime（）函数在每次调用时读取时间的时间应该大致相同。所有报告的设备功能都要进行四舍五入。
在运行时，如果平台中存在PC-AT CMOS设备，则调用方必须在调用GetTime（）之前同步对该设备的访问。
*/
  grub_efi_status_t EFIAPI
  (*get_time) (grub_efi_time_t *time,									//指向存储的指针，用于接收当前时间的快照。
	       grub_efi_time_capabilities_t *capabilities); //指向缓冲区的可选指针，用于接收实时时钟设备的功能。

  grub_efi_status_t EFIAPI
  (*set_time) (grub_efi_time_t *time);  //设置时间

  grub_efi_status_t EFIAPI
  (*get_wakeup_time) (grub_efi_boolean_t *enabled,
		      grub_efi_boolean_t *pending,
		      grub_efi_time_t *time); //获得唤醒时间

  grub_efi_status_t EFIAPI
  (*set_wakeup_time) (grub_efi_boolean_t enabled,
		      grub_efi_time_t *time); //设置唤醒时间

  grub_efi_status_t EFIAPI
  (*set_virtual_address_map) (grub_efi_uintn_t memory_map_size,
			      grub_efi_uintn_t descriptor_size,
			      grub_efi_uint32_t descriptor_version,
			      grub_efi_memory_descriptor_t *virtual_map); //设置虚拟地址MA (内存映射尺寸,描述符尺寸,描述符版本,虚拟映射)

  grub_efi_status_t EFIAPI
  (*convert_pointer) (grub_efi_uintn_t debug_disposition, void **address);  //转换指针

#define GRUB_EFI_GLOBAL_VARIABLE_GUID \
  { 0x8BE4DF61, 0x93CA, 0x11d2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B,0x8C }}	//全局变量GUID 


  grub_efi_status_t EFIAPI
  (*get_variable) (grub_efi_char16_t *variable_name,
		   const grub_efi_guid_t *vendor_guid,
		   grub_efi_uint32_t *attributes,
		   grub_efi_uintn_t *data_size,
		   void *data); //获取变量 

  grub_efi_status_t EFIAPI
  (*get_next_variable_name) (grub_efi_uintn_t *variable_name_size,
			     grub_efi_char16_t *variable_name,
			     grub_efi_guid_t *vendor_guid); //获取下一个变量名

  grub_efi_status_t EFIAPI
  (*set_variable) (grub_efi_char16_t *variable_name,
		   const grub_efi_guid_t *vendor_guid,
		   grub_efi_uint32_t attributes,
		   grub_efi_uintn_t data_size,
		   void *data); //设置变量

  grub_efi_status_t EFIAPI
  (*get_next_high_monotonic_count) (grub_efi_uint32_t *high_count);

  void EFIAPI
  (*reset_system) (grub_efi_reset_type_t reset_type,
		   grub_efi_status_t reset_status,
		   grub_efi_uintn_t data_size,
		   grub_efi_char16_t *reset_data);  //求下一个高单调数
};
typedef struct grub_efi_runtime_services grub_efi_runtime_services_t;	//g4d:占44

struct grub_efi_configuration_table //配置表
{
  grub_efi_packed_guid_t vendor_guid;		//占10		g4d
  void *vendor_table;										//占4
} GRUB_PACKED;
typedef struct grub_efi_configuration_table grub_efi_configuration_table_t;		//g4d:占14

#define GRUB_EFIEMU_SYSTEM_TABLE_SIGNATURE 0x5453595320494249LL
#define GRUB_EFIEMU_RUNTIME_SERVICES_SIGNATURE 0x56524553544e5552LL

struct grub_efi_serial_io_interface   //串行IO接口
{
  grub_efi_uint32_t revision;
  void (*reset) (void);
  grub_efi_status_t EFIAPI (*set_attributes) (struct grub_efi_serial_io_interface *this,
				       grub_efi_uint64_t speed,
				       grub_efi_uint32_t fifo_depth,
				       grub_efi_uint32_t timeout,
				       grub_efi_parity_type_t parity,
				       grub_uint8_t word_len,
				       grub_efi_stop_bits_t stop_bits);
  grub_efi_status_t EFIAPI (*set_control_bits) (struct grub_efi_serial_io_interface *this,
					 grub_efi_uint32_t flags);
  void (*get_control_bits) (void);
  grub_efi_status_t EFIAPI (*write) (struct grub_efi_serial_io_interface *this,
			      grub_efi_uintn_t *buf_size,
			      void *buffer);
  grub_efi_status_t EFIAPI (*read) (struct grub_efi_serial_io_interface *this,
			     grub_efi_uintn_t *buf_size,
			     void *buffer);
};

//函数的作用是：从输入设备中读取下一次击键。如果没有挂起的击键，则函数返回EFI_NOT_READY。如果存在挂起的击键，则扫描代码是表107中定义的EFI扫描代码。
//Unicode字符是实际的可打印字符，如果键不代表可打印字符（控制键、功能键等），则为零。
struct grub_efi_simple_input_interface    //简单输入接口
{
  //重置	重置输入设备硬件
  grub_efi_status_t EFIAPI
  (*reset) (struct grub_efi_simple_input_interface *this,
	    grub_efi_boolean_t extended_verification);   //值=1,扩展验证.  指示在重置期间，驱动程序可以对设备执行更彻底的验证操作。

  grub_efi_status_t EFIAPI
  (*read_key_stroke) (struct grub_efi_simple_input_interface *this,
		      grub_efi_input_key_t *key); //读击键		从输入设备读取下一次击键. key是指向缓冲区的指针，该缓冲区中填充了所按键的击键信息。

  grub_efi_event_t wait_for_key;  //等待键		事件与EFI_BOOT_SERVICES.WaitForEvent（）一起使用，以等待键可用。
};
typedef struct grub_efi_simple_input_interface grub_efi_simple_input_interface_t;	//grub2:18(全部按8)		g4d:c(全部按4)?

struct grub_efi_key_data {          //键数据
	grub_efi_input_key_t key;         //键信息	从输入设备返回的EFI扫描代码和Unicode值。32位
	grub_efi_key_state_t key_state;   //键状态	各种切换属性以及输入修改器值的当前状态。40位
};
typedef struct grub_efi_key_data grub_efi_key_data_t;

typedef long long (*grub_efi_key_notify_function_t) ( //状态
	grub_efi_key_data_t *key_data     //键数据
	);

struct grub_efi_simple_text_input_ex_interface  //简单文本输入扩展接口 
{
  //重置	重置输入设备硬件
	grub_efi_status_t EFIAPI
	(*reset) (struct grub_efi_simple_text_input_ex_interface *this,
		  grub_efi_boolean_t extended_verification);  //值=1,扩展验证.  指示在重置期间，驱动程序可以对设备执行更彻底的验证操作。

	grub_efi_status_t EFIAPI
	(*read_key_stroke) (struct grub_efi_simple_text_input_ex_interface *this,
			    grub_efi_key_data_t *key_data); //读击键	从输入设备读取下一次击键。 

	grub_efi_event_t wait_for_key;					//等待键	事件与WaitForEvent（）一起使用，以等待按键可用。只有KeyData.Key中包含信息时，才会触发事件。
	
//函数的作用是：允许输入设备硬件调整状态设置。通过调用SetState（）函数，使KeyToggleState参数中的EFI_KEY_STATE_EXPOSED位处于活动状态，这将使readkeystrookex函数返回不完整的击键，例如在没有键数据时按住某些键，这些键表示为KeyState的一部分。
	grub_efi_status_t EFIAPI
	(*set_state) (struct grub_efi_simple_text_input_ex_interface *this,
		      grub_efi_key_toggle_state_t *key_toggle_state); //设置状态	设置输入设备的特定状态 

//注册一个函数，当指定的击键发生时将调用该函数。指定的击键可以是KeyData.Key或KeyData.KeyState信息的任意组合。
//用来将特定按键输入事件和事件处理函数绑定的函数 
	grub_efi_status_t EFIAPI
	(*register_key_notify) (struct grub_efi_simple_text_input_ex_interface *this,
				grub_efi_key_data_t *key_data,  //要绑定的按键事件
				grub_efi_key_notify_function_t key_notification_function, //指向要绑定的函数的指针
        void **NotifyHandle);       //返回的句柄，它的值唯一，在取消绑定时要用到

//注销键通知
//解绑按键事件
	grub_efi_status_t EFIAPI
	(*unregister_key_notify) (struct grub_efi_simple_text_input_ex_interface *this,
				  void *notification_handle); 
};
typedef struct grub_efi_simple_text_input_ex_interface grub_efi_simple_text_input_ex_interface_t;

/*
绑定按键事件
展示一个使用RegisterKeyNotify() 的例子。
#include "efi.h"
#include "common.h"
unsigned char is_exit = FALSE;
unsigned long long key_notice(
struct EFI_KEY_DATA *KeyData __attribute__ ((unused)))
{
  is_exit = TRUE;
  return EFI_SUCCESS;
}
void efi_main(void *ImageHandle __attribute__ ((unused)),
struct EFI_SYSTEM_TABLE *SystemTable)
{
  unsigned long long status;
  struct EFI_KEY_DATA key_data = {{0, L'q'}, {0, 0}};
  void *notify_handle;
  efi_init(SystemTable);
  ST->ConOut->ClearScreen(ST->ConOut);
  puts(L"Waiting for the 'q' key input...\r\n");
  status = STIEP->RegisterKeyNotify(STIEP, &key_data, key_notice, &notify_handle);
  assert(status, L"RegisterKeyNotify");
  while (!is_exit);
  puts(L"exit.\r\n");
  while (TRUE);
}
上面这段代码实现了按下"q"键来退出efi_main() 中的一个循环的功能。这里要处理的事件是“按下q键”，因此我们先将key_data.Key.UnicodeChar 设置为"q"，
并将key_data 中的其它属性设为0。然后，我们使用RegisterKeyNotify() 函数，将“按下q键”这一事件与事件处理函数key_notice() 绑定。在key_notice() 函数中，
我们把全局变量is_exit 设置为TRUE ，这样就实现了按下q键跳出efi_main()中while (!is_exit); 这个循环的功能，
*/



struct grub_efi_simple_text_output_interface  //简单文本输出接口 
{
//重置	重置控制台设备
  grub_efi_status_t EFIAPI
  (*reset) (struct grub_efi_simple_text_output_interface *this,
	    grub_efi_boolean_t extended_verification);  //值=1,扩展验证.  指示在重置期间，驱动程序可以对设备执行更彻底的验证操作

//输出字符串	在当前光标位置显示设备上的字符串      
//将字符串写入输出设备。要在输出设备上显示的以空结尾的字符串。所有输出设备还必须支持“相关定义”中定义的Unicode绘图字符代码 
  grub_efi_status_t EFIAPI
  (*output_string) (struct grub_efi_simple_text_output_interface *this,
		    grub_efi_char16_t *string); 

//测试字符串	测试ConsoleOut设备是否支持此字符串。        
//函数的作用是：验证字符串中的所有字符都可以输出到目标设备。此函数提供了一种方法，可以知道输出设备上是否支持渲染所需的字符代码。
//这允许安装过程（或EFI映像）至少选择输出设备能够显示的字符代码。由于输出设备可能在引导之间发生更改，如果加载程序无法适应这些更改，
//建议加载程序调用OutputString（），并忽略任何“不受支持”的错误代码。能够显示Unicode字符代码的设备将这样做。
  grub_efi_status_t EFIAPI
  (*test_string) (struct grub_efi_simple_text_output_interface *this,
		  grub_efi_char16_t *string);

//查询模式	查询有关输出设备支持的文本模式的信息
  grub_efi_status_t EFIAPI
  (*query_mode) (struct grub_efi_simple_text_output_interface *this,
		 grub_efi_uintn_t mode_number,
		 grub_efi_uintn_t *columns,
		 grub_efi_uintn_t *rows);

//设置模式	设置输出设备的当前模式
  grub_efi_status_t EFIAPI
  (*set_mode) (struct grub_efi_simple_text_output_interface *this,
	       grub_efi_uintn_t mode_number); 

//设置属性	设置输出文本的前景色和背景色
  grub_efi_status_t EFIAPI
  (*set_attributes) (struct grub_efi_simple_text_output_interface *this,
		     grub_efi_uintn_t attribute); 

//清除屏幕	清除当前设置背景色的屏幕
//在调用清屏函数前先输出了一个空格，这是为了适配部分计算机的UEFI固件。某些UEFI固件，例如作者的联想笔记本的固件，
//在启动后不输出任何内容的情况下直接调用ClearScreen() 进行清屏似乎会被忽略。即使这里我们先调用SetAttribute() 设置背景色，
//再调用ClearScreen() 清屏，屏幕的背景色也不会被设为我们指定的颜色。
  grub_efi_status_t EFIAPI
  (*clear_screen) (struct grub_efi_simple_text_output_interface *this);  

//设置光标位置	设置当前光标位置
  grub_efi_status_t EFIAPI
  (*set_cursor_position) (struct grub_efi_simple_text_output_interface *this,
			  grub_efi_uintn_t column,
			  grub_efi_uintn_t row);  

//使能光标	打开/关闭光标的可见性
  grub_efi_status_t EFIAPI
  (*enable_cursor) (struct grub_efi_simple_text_output_interface *this,
		    grub_efi_boolean_t visible);   

//输出模式	指向简单文本输出模式数据的指针。
  grub_efi_simple_text_output_mode_t *mode; 
};
typedef struct grub_efi_simple_text_output_interface grub_efi_simple_text_output_interface_t;		//g4d:占28

//typedef unsigned char grub_efi_pxe_packet_t[1472];	//pxe包  0x5c0

typedef struct {
  grub_uint8_t addr[4];
} grub_efi_pxe_ipv4_address_t;

typedef struct {
  grub_uint8_t addr[16];
} grub_efi_pxe_ipv6_address_t;

typedef struct {
  grub_uint8_t addr[32];
} grub_efi_pxe_mac_address_t;

typedef union {
    grub_uint32_t addr[4];
    grub_efi_pxe_ipv4_address_t v4;
    grub_efi_pxe_ipv6_address_t v6;
} grub_efi_pxe_ip_address_t;

typedef struct grub_efi_pxe_dhcpv4_packet	//引导播放器		备注: 在 ipxe->bootia32.efi 时, pxe_reply与dhcp_ack相同; bootp_yi_addr = c0 a8 38 06
{																					//dhcp_discover				dhcp_ack				proxy_offer,pxe_discover,pxe_reply,pxe_bis_reply
  grub_efi_uint8_t bootp_opcode;					//01									02							全部0
  grub_efi_uint8_t bootp_hwtype;					//01
  grub_efi_uint8_t bootp_hwaddr_len;			//06
  grub_efi_uint8_t bootp_gate_hops;				//00
  grub_efi_uint32_t bootp_ident;					//59 6b 5d 13					07 6c 4a a1
  grub_efi_uint16_t bootp_seconds;				//00 00	
  grub_efi_uint16_t bootp_flags;					//80 00
  grub_efi_uint8_t bootp_ci_addr[4];			//00 00 00 00
  grub_efi_uint8_t bootp_yi_addr[4];			//00 00 00 00					c0 a8 38 02
  grub_efi_uint8_t bootp_si_addr[4];			//00 00 00 00					c0 a8 38 01
  grub_efi_uint8_t bootp_gi_addr[4];			//00 00 00 00
  grub_efi_uint8_t bootp_hw_addr[16];			//00 0c 29 8d cc d9
  grub_efi_uint8_t bootp_srv_name[64];		//0										PC-201311212111
  grub_efi_uint8_t bootp_boot_file[128];	//0										bootia32.EFI
  grub_efi_uint32_t dhcp_magik;						//63 82 53 63
  grub_efi_uint8_t dhcp_options[56];			//35 01 01 39 02 05 c0 37 - 23 01 02 03 04 05 06 0c - 0d 0f 11 12 16 17 1c 28 - 29 2a 2b 32 33 36 3a 3b
																					//3c 42 43 61 80 81 82 83 - 84 85 86 87 61 11 00 56 - 4d dc 4a a9 a8 2f 94 73 - 61 65 d5 98 8d cc d9 5e
																					//03 01 03 10 5d 02 00 06 - 3c 20 50 58 45 43 6c 69 - 65 6e 74 3a 41 72 63 68 - 3a 30 30 30 30 36 3a 55
																					//4e 44 49 3a 30 30 33 30 - 31 36 ff 00
} grub_efi_pxe_dhcpv4_packet_t;

struct grub_efi_pxe_dhcpv6_packet
{
  grub_efi_uint32_t message_type:8;
  grub_efi_uint32_t transaction_id:24;
  grub_efi_uint8_t dhcp_options[1024];
} GRUB_PACKED;
typedef struct grub_efi_pxe_dhcpv6_packet grub_efi_pxe_dhcpv6_packet_t;

typedef union
{
  grub_efi_uint8_t raw[1472];
  grub_efi_pxe_dhcpv4_packet_t dhcpv4;
  grub_efi_pxe_dhcpv6_packet_t dhcpv6;
} grub_efi_pxe_packet_t;

typedef struct grub_efi_pxe_icmp_error
{
  grub_efi_uint8_t type;
  grub_efi_uint8_t code;
  grub_efi_uint16_t checksum;
  union
    {
      grub_efi_uint32_t reserved;
      grub_efi_uint32_t mtu;
      grub_efi_uint32_t pointer;
      struct
	{
	  grub_efi_uint16_t identifier;
	  grub_efi_uint16_t sequence;
	} echo;
    } u;
  grub_efi_uint8_t data[494];
} grub_efi_pxe_icmp_error_t;

typedef struct grub_efi_pxe_tftp_error			//TFTP错误
{
  grub_efi_uint8_t error_code;							//错误代码
  grub_efi_char8_t error_string[127];				//错误信息
} grub_efi_pxe_tftp_error_t;

typedef grub_efi_uint16_t grub_efi_pxe_base_code_udp_port_t;

typedef enum {
  GRUB_EFI_PXE_BASE_CODE_TFTP_FIRST,					//TFTP首先
  GRUB_EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE,	//TFTP获得文件尺寸
  GRUB_EFI_PXE_BASE_CODE_TFTP_READ_FILE,			//TFTP读文件
  GRUB_EFI_PXE_BASE_CODE_TFTP_WRITE_FILE,			//TFTP写文件
  GRUB_EFI_PXE_BASE_CODE_TFTP_READ_DIRECTORY,	//TFTP读目录
  GRUB_EFI_PXE_BASE_CODE_MTFTP_GET_FILE_SIZE,
  GRUB_EFI_PXE_BASE_CODE_MTFTP_READ_FILE,
  GRUB_EFI_PXE_BASE_CODE_MTFTP_READ_DIRECTORY,
  GRUB_EFI_PXE_BASE_CODE_MTFTP_LAST
} grub_efi_pxe_base_code_tftp_opcode_t;				//pxe基本代码tftp操作

typedef struct {
  grub_efi_ip_address_t mcast_ip;
  grub_efi_pxe_base_code_udp_port_t c_port;
  grub_efi_pxe_base_code_udp_port_t s_port;
  grub_efi_uint16_t listen_timeout;
  grub_efi_uint16_t transmit_timeout;
} grub_efi_pxe_base_code_mtftp_info_t;

#define GRUB_EFI_PXE_BASE_CODE_MAX_IPCNT 8
typedef struct grub_efi_pxe_ip_filter
{
  grub_efi_uint8_t filters;		//过滤器 		01;			0f
  grub_efi_uint8_t ip_count;	//ip计数		00
  grub_efi_uint16_t reserved;	//保留			0000
  grub_efi_ip_address_t ip_list[GRUB_EFI_PXE_BASE_CODE_MAX_IPCNT];
} grub_efi_pxe_ip_filter_t;

typedef struct {
  grub_efi_pxe_ip_address_t ip_addr;
  grub_efi_pxe_mac_address_t mac_addr;
} grub_efi_pxe_arp_entry_t;

typedef struct {
  grub_efi_pxe_ip_address_t ip_addr;			//c0 a8 38 00
  grub_efi_pxe_ip_address_t subnet_mask;	//00 00 00 00
  grub_efi_pxe_ip_address_t gw_addr;			//ff ff ff 00
} grub_efi_pxe_route_entry_t;


#define GRUB_EFI_PXE_BASE_CODE_MAX_ARP_ENTRIES 8
#define GRUB_EFI_PXE_BASE_CODE_MAX_ROUTE_ENTRIES 8

typedef struct grub_efi_pxe_mode  //pxe模式
{																			//									bootia32.efi				ipxe->bootia32.efi			
  grub_efi_boolean_t started;					//开始了									01	-
  grub_efi_boolean_t ipv6_available;	//ipv6可用								00
  grub_efi_boolean_t ipv6_supported;	//ipv6支持								00
  grub_efi_boolean_t using_ipv6;			//使用ipv6								00
  grub_efi_boolean_t bis_supported;		//bis支持									00
  grub_efi_boolean_t bis_detected;		//bis检测到								00
  grub_efi_boolean_t auto_arp;				//自动ARP									01
  grub_efi_boolean_t send_guid;				//发送引导								00
  grub_efi_boolean_t dhcp_discover_valid;		//dhcp发现有效			01	-
  grub_efi_boolean_t dhcp_ack_received;			//收到dhcp ack			01
  grub_efi_boolean_t proxy_offer_received;	//收到代理报文			00
  grub_efi_boolean_t pxe_discover_valid;		//pxe发现有效				00
  grub_efi_boolean_t pxe_reply_received;		//pxe收到回复				00						01
  grub_efi_boolean_t pxe_bis_reply_received;//pxe bis收到答复		00
  grub_efi_boolean_t icmp_error_received;		//icmp收到错误			00
  grub_efi_boolean_t tftp_error_received;		//tftp收到错误			00
  grub_efi_boolean_t make_callbacks;				//进行回调					00	-
  grub_efi_uint8_t ttl;									//ttl										10
  grub_efi_uint8_t tos;								 	//tos										00
  grub_efi_ip_address_t station_ip;		 	//站IP									c0 a8 38 02		c0 a8 38 06
  grub_efi_ip_address_t subnet_mask;	 	//子网掩码							ff ff ff 00
  grub_efi_pxe_packet_t dhcp_discover;	//dhcp发现							
  grub_efi_pxe_packet_t dhcp_ack;				//dhcp_ack	引导播放器
  grub_efi_pxe_packet_t proxy_offer;		//代理提供
  grub_efi_pxe_packet_t pxe_discover;		//pxe发现
  grub_efi_pxe_packet_t pxe_reply;			//pxe回复
  grub_efi_pxe_packet_t pxe_bis_reply;	//pxe_bis回复
  grub_efi_pxe_ip_filter_t ip_filter;		//ip过滤器							01 00 00 00		0f 00 00 00
  grub_efi_uint32_t arp_cache_entries;	//arp缓存条目						01 00 00 00		00 00 00 00
  grub_efi_pxe_arp_entry_t arp_cache[GRUB_EFI_PXE_BASE_CODE_MAX_ARP_ENTRIES];	//ARP缓存	c0 a8 38 01, 0a 00 27 00 00 14;		0
  grub_efi_uint32_t route_table_entries;//路由表条目						01 00 00 00		00 00 00 00
  grub_efi_pxe_route_entry_t route_table[GRUB_EFI_PXE_BASE_CODE_MAX_ROUTE_ENTRIES];	//路线表	0
  grub_efi_pxe_icmp_error_t icmp_error;	//icmp错误
  grub_efi_pxe_tftp_error_t tftp_error;	//tftp错误
} grub_efi_pxe_mode_t;


typedef struct grub_efi_pxe
{
  grub_uint64_t rev;							//版本
  grub_efi_status_t (*start) (struct grub_efi_pxe *this, grub_efi_boolean_t use_ipv6);	//开始  启动PXE基本代码协议。 
																																								//在启动基本代码之前，模式结构信息无效，其他基本代码协议功能将无法运行。
  void (*stop) (void);						//停止	停止PXE基本代码协议。 该功能不会改变模式结构信息。 在重新启动基本代码之前，不会运行任何基本代码协议功能。
  grub_efi_status_t (*dhcp) (struct grub_efi_pxe *this,
			    grub_efi_boolean_t sort_offers);								//dhcp  尝试完成DHCPv4 D.O.R.A. （发现/提供/请求/确认）或DHCPv6 S.A.R.R（请求/发布/请求/回复）序列。
  void (*discover) (void);				//发现  尝试完成PXE引导服务器和/或引导映像发现序列。
	//执行TFTP和MTFTP服务。
  grub_efi_status_t (*mtftp) (struct grub_efi_pxe *this,		//指向EFI_PXE_BASE_CODE_PROTOCOL实例的指针
			    grub_efi_pxe_base_code_tftp_opcode_t operation,		//运作方式  要执行的操作类型。
			    char *buffer_ptr,																	//指向数据缓冲区的指针。 如果dont_use_buffer为TRUE，则忽略读取文件。
			    grub_efi_boolean_t overwrite,											//覆盖，仅用于写文件操作。 如果可以覆盖远程服务器上的文件，则为TRUE。
			    grub_efi_uint64_t *buffer_size,										//缓冲区尺寸  对于获得文件尺寸操作，*buffer_size返回所请求文件的尺寸。对于读文件和写文件操作，
																														//此参数设置为指定的缓冲区尺寸。 对于读取文件操作，如果返回EFI_BUFFER_TOO_SMALL，则*buffer_size返回所请求文件的尺寸。	
			    grub_efi_uintn_t *block_size,											//块尺寸  在TFTP传输期间要使用的请求块尺寸。 该字段必须至少为512。
																														//如果此字段为NULL，则将使用实现支持的最大块大小。
			    grub_u32_t *server_ip,																		//TFTP/MTFTP服务器IP地址
			    char *filename,																		//文件名  以Null结尾的ASCII字符串，用于指定目录名称或文件名。 MTFTP读取目录会忽略此内容。
			    grub_efi_pxe_base_code_mtftp_info_t *info,				//指向MTFTP信息的指针。 启动或加入多播TFTP会话需要此信息。
			    grub_efi_boolean_t dont_use_buffer);							//对于正常的TFTP和MTFTP读取文件操作，设置为FALSE。
  void (*udpwrite) (void);				//udp写 将UDP数据包写入网络接口。
  void (*udpread) (void);					//udp读 从网络接口读取UDP数据包。
  void (*setipfilter) (void);			//设置过滤器  更新网络设备的IP接收筛选器。
  void (*arp) (void);							//arp 使用ARP协议解析MAC地址。
  void (*setparams) (void);				//设定参数  更新影响PXE基本代码协议操作的参数。
  grub_efi_status_t (*set_station_ip) (struct grub_efi_pxe *this,
			    grub_u32_t *new_station_ip,
			    grub_u32_t *new_subnet_mask);	//设置站ip  更新站的IP地址和子网掩码值。
  void (*setpackets) (void);			//设置数据包  更新缓存的DHCP和发现数据包的内容。
  struct grub_efi_pxe_mode *mode;	//模式  指向此设备的EFI_PXE_BASE_CODE_MODE数据的指针。
} grub_efi_pxe_t;



#define GRUB_EFI_BLACK        0x00			//前景黑
#define GRUB_EFI_BLUE         0x01			//前景蓝
#define GRUB_EFI_GREEN        0x02			//前景绿
#define GRUB_EFI_CYAN         0x03			//前景青
#define GRUB_EFI_RED          0x04			//前景红
#define GRUB_EFI_MAGENTA      0x05		  //前景品红
#define GRUB_EFI_BROWN        0x06			//前景棕
#define GRUB_EFI_LIGHTGRAY    0x07      //前景浅灰色
#define GRUB_EFI_BRIGHT       0x08      //前景明亮
#define GRUB_EFI_DARKGRAY     0x08      //前景暗灰
#define GRUB_EFI_LIGHTBLUE    0x09      //前景浅蓝色
#define GRUB_EFI_LIGHTGREEN   0x0A      //前景浅绿色
#define GRUB_EFI_LIGHTCYAN    0x0B      //前景浅蓝色
#define GRUB_EFI_LIGHTRED     0x0C      //前景浅红
#define GRUB_EFI_LIGHTMAGENTA 0x0D      //前景浅品红
#define GRUB_EFI_YELLOW       0x0E      //前景黃色
#define GRUB_EFI_WHITE        0x0F      //前景白色

#define GRUB_EFI_BACKGROUND_BLACK	0x00			//背景黑
#define GRUB_EFI_BACKGROUND_BLUE	0x10			//背景蓝
#define GRUB_EFI_BACKGROUND_GREEN	0x20			//背景绿 
#define GRUB_EFI_BACKGROUND_CYAN	0x30			//背景青
#define GRUB_EFI_BACKGROUND_RED		0x40			//背景红
#define GRUB_EFI_BACKGROUND_MAGENTA	0x50		//背景品红
#define GRUB_EFI_BACKGROUND_BROWN	0x60			//背景棕
#define GRUB_EFI_BACKGROUND_LIGHTGRAY	0x70	//背景浅灰色

#define GRUB_EFI_TEXT_ATTR(fg, bg)	((fg) | ((bg)))

/*
typedef struct {
EFI_TABLE_HEADER Hdr;
CHAR16 *FirmwareVendor;
UINT32 FirmwareRevision;
EFI_HANDLE ConsoleInHandle;
EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
EFI_HANDLE ConsoleOutHandle;
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
EFI_HANDLE StandardErrorHandle;
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
EFI_RUNTIME_SERVICES *RuntimeServices;
EFI_BOOT_SERVICES *BootServices;
UINTN NumberOfTableEntries;
EFI_CONFIGURATION_TABLE *ConfigurationTable;
} EFI_SYSTEM_TABLE;
*/

struct grub_efi_system_table    //系统表	12b7ef90
{
  grub_efi_table_header_t hdr;                          //表头                IBI SYST(   占18字节
  grub_efi_char16_t *firmware_vendor;                   //固件供应商          12b7e990->EDK II
  grub_efi_uint32_t firmware_revision;                  //固件版本            10000
  grub_efi_handle_t console_in_handler;                 //控制台输入句柄		  12820410		活动控制台输入设备的句柄。此句柄必须支持EFI简单文本输入协议和EFI简单文本扩展输入协议。
  grub_efi_simple_input_interface_t *con_in;            //简单文本输入接口    12a9ce5c		指向与控制台输入句柄关联的EFI简单文本输入协议接口的指针
  grub_efi_handle_t console_out_handler;                //控制台输出句柄		  1281ce10		活动控制台输出设备的句柄。此句柄必须支持EFI简单文本输出协议 
  grub_efi_simple_text_output_interface_t *con_out;     //简单文本输出接口    12a9ce9c		指向与控制台输出句柄关联的EFI简单文本输出协议接口的指针。 
  grub_efi_handle_t standard_error_handle;              //标准错误句柄        1281cb90		活动标准错误控制台设备的句柄。此句柄必须支持EFI简单文本输出协议 
  grub_efi_simple_text_output_interface_t *std_err;     //标准错误输出接口    129accfc		指向与标准错误句柄关联的EFI简单文本输出协议接口的指针。
  grub_efi_runtime_services_t *runtime_services;        //运行时服务          12b7ef10		指向EFI运行时服务表的指针。见第4.5节。
  grub_efi_boot_services_t *boot_services;              //引导服务            12bc0fc8		指向EFI引导服务表的指针。见第4.4节。
  grub_efi_uintn_t num_table_entries;                 	//表格项目数          07					缓冲区配置表中的系统配置表数
  grub_efi_configuration_table_t *configuration_table;  //配置表              12b7dd90		指向系统配置表的指针。
};
typedef struct grub_efi_system_table  grub_efi_system_table_t;	//grub2:78(表头按18,其余按4或8)

struct grub_efi_loaded_image  //加载映像 11b45328
{
  grub_efi_uint32_t revision;             //修订          1000 
  void * parent_handle;        						//父句柄        0
  grub_efi_system_table_t *system_table;  //系统表        12b7ef90
  void * device_handle;        						//设备句柄      11cb5a90->hndl...
  grub_efi_device_path_t *file_path;      //文件路径      11afb890->04 04 32 00 \EFI\BOOT\BOOTIA32.EFI  不是设备句柄对应的设备路径
  void *reserved;                         //保留          00
  grub_efi_uint32_t load_options_size;    //加载选项尺寸  00
  void *load_options;                     //加载选项      00
  void *image_base;                       //映像基址      114cb000
  grub_efi_uint64_t image_size;           //映像尺寸      05be00
  grub_efi_memory_type_t image_code_type; //映像代码类型  01	装载程序代码
  grub_efi_memory_type_t image_data_type; //映像数据类型  02	装载数据
  grub_efi_status_t (*unload) (grub_efi_handle_t image_handle); //卸载 0f39a000 ??
};
typedef struct grub_efi_loaded_image grub_efi_loaded_image_t;	//60(结构及void按8,其余按指定)	38

struct grub_efi_disk_io   //磁盘输入输出
{
  grub_efi_uint64_t revision; //修订
  grub_efi_status_t EFIAPI (*read) (struct grub_efi_disk_io *this,
			     grub_efi_uint32_t media_id,
			     grub_efi_uint64_t offset,
			     grub_efi_uintn_t buffer_size,
			     void *buffer);     //读(本身,媒体标识,偏移,缓冲尺寸,缓存地址)
  grub_efi_status_t EFIAPI (*write) (struct grub_efi_disk_io *this,
			     grub_efi_uint32_t media_id,
			     grub_efi_uint64_t offset,
			     grub_efi_uintn_t buffer_size,
			     void *buffer);     //写
};
typedef struct grub_efi_disk_io grub_efi_disk_io_t;

typedef unsigned char grub_efi_mac_t[32];

struct grub_efi_simple_network_mode //简单网络模式
{																					//															bootia32.efi				ipxe->bootia32.efi
  unsigned int state;										  //状态													02
  unsigned int hwaddr_size;							  //硬件地址尺寸									06
  unsigned int media_header_size;				  //媒体头尺寸										0e
  unsigned int max_packet_size;					  //最大包尺寸										05dc(1500)
  unsigned int nvram_size;								//虚拟ram尺寸										0200								0
  unsigned int nvram_access_size;				  //虚拟ram访问尺寸								04									0
  unsigned int receive_filter_mask;			  //接收滤波器屏蔽掩模						1f									07
  unsigned int receive_filter_setting;		//接收滤波器设置								07									0
  unsigned int max_mcast_filter_count;		//最大多播地址接收筛选器数			08									0
  unsigned int mcast_filter_count;				//多播地址接收筛选器的当前数目	01									0
  grub_efi_mac_t mcast_filter[16];				//多播地址接收筛选器						01 00 5e 00 00 01		0
  grub_efi_mac_t current_address;					//当前地址											00 0c 29 8d cc d9
  grub_efi_mac_t broadcast_address;				//广播地址											ff ff ff ff ff ff
  grub_efi_mac_t permanent_address;				//永久地址											00 0c 29 8d cc d9
  unsigned char if_type;									//接口类型											01
  unsigned char mac_changeable;						//mac可更改											01
  unsigned char multitx_supported;				//支持多数据包传输							00
  unsigned char media_present_supported;	//支持媒体											01
  unsigned char media_present;						//媒体连接											01
};

enum			//简单网络状态
  {
    GRUB_EFI_NETWORK_STOPPED,							//网络停止 	0
    GRUB_EFI_NETWORK_STARTED,							//网络起动	1
    GRUB_EFI_NETWORK_INITIALIZED,					//已初始化	2
  };

enum			//接收过滤器设置的位掩码值
  {
    GRUB_EFI_SIMPLE_NETWORK_RECEIVE_UNICAST			= 0x01,						//简单网络接收  单播
    GRUB_EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST		= 0x02,						//简单网络接收  多播
    GRUB_EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST		= 0x04,						//简单网络接收 	广播 
    GRUB_EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS	= 0x08,						//简单网络接收  杂乱
    GRUB_EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST = 0x10,	//简单网络接收  混杂多播
  };

struct grub_efi_simple_network    //简单网络
{
  grub_uint64_t revision;	//修订
  grub_efi_status_t EFIAPI (*start) (struct grub_efi_simple_network *this);		//开始(指向EFI_SIMPLE_NETWORK_协议实例的指针)
  grub_efi_status_t EFIAPI (*stop) (struct grub_efi_simple_network *this);			//停止
  grub_efi_status_t EFIAPI (*initialize) (struct grub_efi_simple_network *this,
				   grub_efi_uintn_t extra_rx,
				   grub_efi_uintn_t extra_tx);																	//初始化
  grub_efi_status_t EFIAPI (*reset) (struct grub_efi_simple_network *this,
					grub_uint32_t ExtendedVerification);													//重置
  grub_efi_status_t EFIAPI (*shutdown) (struct grub_efi_simple_network *this);	//关闭
  grub_efi_status_t EFIAPI (*receive_filters) (struct grub_efi_simple_network *this,
					grub_uint32_t enable,
					grub_uint32_t disable,
					grub_efi_boolean_t reset_mcast_filter,
					grub_efi_uintn_t mcast_filter_count,
					grub_efi_mac_address_t *mcast_filter);												//接收过滤器
  void (*station_address) (void);																				//固定地址
  void (*statistics) (void);																						//统计
  void (*mcastiptomac) (void);																					//将多播IP地址映射到多播
  void (*nvdata) (void);																								//NVRAM数据
  grub_efi_status_t EFIAPI (*get_status) (struct grub_efi_simple_network *this,
				   grub_uint32_t *int_status,
				   void **txbuf);																								//获得状态
  grub_efi_status_t EFIAPI (*transmit) (struct grub_efi_simple_network *this,
				 grub_efi_uintn_t header_size,
				 grub_efi_uintn_t buffer_size,
				 void *buffer,
				 grub_efi_mac_t *src_addr,
				 grub_efi_mac_t *dest_addr,
				 grub_efi_uint16_t *protocol);																	//传输
  grub_efi_status_t EFIAPI (*receive) (struct grub_efi_simple_network *this,
				grub_efi_uintn_t *header_size,
				grub_efi_uintn_t *buffer_size,
				void *buffer,
				grub_efi_mac_t *src_addr,
				grub_efi_mac_t *dest_addr,
				grub_uint16_t *protocol);																				//接收
  void EFIAPI (*waitforpacket) (void);																					//等待包
  struct grub_efi_simple_network_mode *mode;														//简单网络模式
};
typedef struct grub_efi_simple_network grub_efi_simple_network_t;


struct grub_efi_block_io  //块输入输出
{
  grub_efi_uint64_t revision;                                       //修订  020001
  grub_efi_block_io_media_t *media;                                 //介质  fea7ed8
  grub_efi_status_t EFIAPI (*reset) (struct grub_efi_block_io *this,
			      grub_efi_boolean_t extended_verification);              //重置  ff95f10
	//读块(本身,媒体id,起始逻辑块,读写字节,缓存地址) ff9600e
  grub_efi_status_t EFIAPI (*read_blocks) (struct grub_efi_block_io *this,	//本身
				    grub_efi_uint32_t media_id,															//读取请求的媒体ID。相当于驱动器号.
				    grub_efi_lba_t lba,																			//要从设备上读取的起始逻辑块地址。以扇区计.
				    grub_efi_uintn_t buffer_size,														//缓冲区的尺寸(字节)。这必须是设备内部块大小的倍数
				    void *buffer);                                          //指向数据的目标缓冲区的指针。调用方负责隐式或显式拥有缓冲区。
	//写块(本身,媒体id,起始逻辑块,读写字节,缓存地址) ff9600e
  grub_efi_status_t EFIAPI (*write_blocks) (struct grub_efi_block_io *this,//本身
						grub_efi_uint32_t media_id,															//读取请求的媒体ID。相当于驱动器号.
						grub_efi_lba_t lba,																			//要从设备上读取的起始逻辑块地址。以扇区计.
						grub_efi_uintn_t buffer_size,														//缓冲区的尺寸(字节)。这必须是设备内部块大小的倍数。 
						void *buffer);                                         	//指向数据的目标缓冲区的指针。调用方负责隐式或显式拥有缓冲区。
						 //写块  ff9611b
  grub_efi_status_t EFIAPI (*flush_blocks) (struct grub_efi_block_io *this); //刷新块 ff96228	将所有修改过的数据刷新到物理块设备。 
};
typedef struct grub_efi_block_io grub_efi_block_io_t;

#define CR(RECORD, TYPE, FIELD) \
    ((TYPE *) ((char *) (RECORD) - (char *) &(((TYPE *) 0)->FIELD)))

enum grub_efi_ip4_config2_data_type {	//IP4配置2数据类型
  GRUB_EFI_IP4_CONFIG2_DATA_TYPE_INTERFACEINFO,		//接口信息
  GRUB_EFI_IP4_CONFIG2_DATA_TYPE_POLICY,					//策略
  GRUB_EFI_IP4_CONFIG2_DATA_TYPE_MANUAL_ADDRESS,	//手动地址
  GRUB_EFI_IP4_CONFIG2_DATA_TYPE_GATEWAY,					//网关
  GRUB_EFI_IP4_CONFIG2_DATA_TYPE_DNSSERVER,				//dns服务器
  GRUB_EFI_IP4_CONFIG2_DATA_TYPE_MAXIMUM					//最大值
};
typedef enum grub_efi_ip4_config2_data_type grub_efi_ip4_config2_data_type_t;

struct grub_efi_ip4_config2_protocol	//IP4配置2协议
{
//设置数据
  grub_efi_status_t EFIAPI (*set_data) (struct grub_efi_ip4_config2_protocol *this,
				 grub_efi_ip4_config2_data_type_t data_type,
				 grub_efi_uintn_t data_size,
				 void *data);
//获得数据
  grub_efi_status_t EFIAPI (*get_data) (struct grub_efi_ip4_config2_protocol *this,
				 grub_efi_ip4_config2_data_type_t data_type,
				 grub_efi_uintn_t *data_size,
				 void *data);
//注册数据通知
  grub_efi_status_t EFIAPI (*register_data_notify) (struct grub_efi_ip4_config2_protocol *this,
					     grub_efi_ip4_config2_data_type_t data_type,
					     grub_efi_event_t event);
//注销数据通知
  grub_efi_status_t EFIAPI (*unregister_datanotify) (struct grub_efi_ip4_config2_protocol *this,
					     grub_efi_ip4_config2_data_type_t data_type,
					     grub_efi_event_t event);
};
typedef struct grub_efi_ip4_config2_protocol grub_efi_ip4_config2_protocol_t;

struct grub_efi_ip4_route_table {						//IP4路线表
  grub_efi_ipv4_address_t subnet_address;		//子网地址
  grub_efi_ipv4_address_t subnet_mask;			//子网掩码
  grub_efi_ipv4_address_t gateway_address;	//网关地址
};

typedef struct grub_efi_ip4_route_table grub_efi_ip4_route_table_t;

#define GRUB_EFI_IP4_CONFIG2_INTERFACE_INFO_NAME_SIZE 32		//P4配置2接口信息名称尺寸

struct grub_efi_ip4_config2_interface_info {		//P4配置2接口信息
  grub_efi_char16_t name[GRUB_EFI_IP4_CONFIG2_INTERFACE_INFO_NAME_SIZE];	//名称
  grub_efi_uint8_t if_type;											//接口类型
  grub_efi_uint32_t hw_address_size;						//硬件地址尺寸
  grub_efi_mac_address_t hw_address;						//硬件地址
  grub_efi_ipv4_address_t station_address;			//站地址
  grub_efi_ipv4_address_t subnet_mask;					//子网掩码
  grub_efi_uint32_t route_table_size;						//路由表尺寸
  grub_efi_ip4_route_table_t *route_table;			//路由表
};

typedef struct grub_efi_ip4_config2_interface_info grub_efi_ip4_config2_interface_info_t;

enum grub_efi_ip4_config2_policy {		//P4配置2策略
  GRUB_EFI_IP4_CONFIG2_POLICY_STATIC,	//静态
  GRUB_EFI_IP4_CONFIG2_POLICY_DHCP,		//DHCP
  GRUB_EFI_IP4_CONFIG2_POLICY_MAX			//最大值
};

typedef enum grub_efi_ip4_config2_policy grub_efi_ip4_config2_policy_t;

struct grub_efi_ip4_config2_manual_address {//P4配置2策略手动地址
  grub_efi_ipv4_address_t address;					//地址
  grub_efi_ipv4_address_t subnet_mask;			//子网掩码
};

typedef struct grub_efi_ip4_config2_manual_address grub_efi_ip4_config2_manual_address_t;

enum grub_efi_ip6_config_data_type {							//ip6配置数据类型
  GRUB_EFI_IP6_CONFIG_DATA_TYPE_INTERFACEINFO,		//接口信息
  GRUB_EFI_IP6_CONFIG_DATA_TYPE_ALT_INTERFACEID,	//ALT接口
  GRUB_EFI_IP6_CONFIG_DATA_TYPE_POLICY,						//策略
  GRUB_EFI_IP6_CONFIG_DATA_TYPE_DUP_ADDR_DETECT_TRANSMITS,	//DUP地址检测传输
  GRUB_EFI_IP6_CONFIG_DATA_TYPE_MANUAL_ADDRESS,		//手动地址
  GRUB_EFI_IP6_CONFIG_DATA_TYPE_GATEWAY,					//网关
  GRUB_EFI_IP6_CONFIG_DATA_TYPE_DNSSERVER,				//dns服务器
  GRUB_EFI_IP6_CONFIG_DATA_TYPE_MAXIMUM						//最大值
};
typedef enum grub_efi_ip6_config_data_type grub_efi_ip6_config_data_type_t;

struct grub_efi_ip6_config_protocol		//ip6配置协议
{
//设置数据
  grub_efi_status_t EFIAPI (*set_data) (struct grub_efi_ip6_config_protocol *this,
				 grub_efi_ip6_config_data_type_t data_type,
				 grub_efi_uintn_t data_size,
				 void *data);
//获得数据
  grub_efi_status_t EFIAPI (*get_data) (struct grub_efi_ip6_config_protocol *this,
				 grub_efi_ip6_config_data_type_t data_type,
				 grub_efi_uintn_t *data_size,
				 void *data);
//注册数据通知
  grub_efi_status_t EFIAPI (*register_data_notify) (struct grub_efi_ip6_config_protocol *this,
					     grub_efi_ip6_config_data_type_t data_type,
					     grub_efi_event_t event);
//注销数据通知
  grub_efi_status_t EFIAPI (*unregister_datanotify) (struct grub_efi_ip6_config_protocol *this,
					     grub_efi_ip6_config_data_type_t data_type,
					     grub_efi_event_t event);
};
typedef struct grub_efi_ip6_config_protocol grub_efi_ip6_config_protocol_t;

enum grub_efi_ip6_config_policy {				//ip6配置策略
  GRUB_EFI_IP6_CONFIG_POLICY_MANUAL,		//手动
  GRUB_EFI_IP6_CONFIG_POLICY_AUTOMATIC	//自动
};
typedef enum grub_efi_ip6_config_policy grub_efi_ip6_config_policy_t;

struct grub_efi_ip6_address_info {			//ip6地址信息
  grub_efi_ipv6_address_t address;			//地址
  grub_efi_uint8_t prefix_length;				//前缀长度
};
typedef struct grub_efi_ip6_address_info grub_efi_ip6_address_info_t;

struct grub_efi_ip6_route_table {						//ip6路线表
  grub_efi_pxe_ipv6_address_t gateway;			//网关
  grub_efi_pxe_ipv6_address_t destination;	//目的地
  grub_efi_uint8_t prefix_length;						//前缀长度
};
typedef struct grub_efi_ip6_route_table grub_efi_ip6_route_table_t;

struct grub_efi_ip6_config_interface_info {		//ip6配置接口信息
  grub_efi_char16_t name[32];									//名称
  grub_efi_uint8_t if_type;										//接口类型
  grub_efi_uint32_t hw_address_size;					//硬件地址尺寸
  grub_efi_mac_address_t hw_address;					//硬件地址
  grub_efi_uint32_t address_info_count;				//地址信息计数
  grub_efi_ip6_address_info_t *address_info;	//地址信息
  grub_efi_uint32_t route_count;							//路线数
  grub_efi_ip6_route_table_t *route_table;		//路线表
};
typedef struct grub_efi_ip6_config_interface_info grub_efi_ip6_config_interface_info_t;

struct grub_efi_ip6_config_dup_addr_detect_transmits {	//ip6配置dup地址检测传送
  grub_efi_uint32_t dup_addr_detect_transmits;	//dup地址检测传送
};
typedef struct grub_efi_ip6_config_dup_addr_detect_transmits grub_efi_ip6_config_dup_addr_detect_transmits_t;

struct grub_efi_ip6_config_manual_address {	//ip6配置手动地址
  grub_efi_ipv6_address_t address;		//地址
  grub_efi_boolean_t is_anycast;			//是任播
  grub_efi_uint8_t prefix_length;			//前缀长度
};
typedef struct grub_efi_ip6_config_manual_address grub_efi_ip6_config_manual_address_t;

struct grub_efi_load_file2
{
  grub_efi_status_t (EFIAPI *load_file)(struct grub_efi_load_file2 this,
                                        grub_efi_device_path_t file_path,
                                        grub_efi_boolean_t boot_policy,
                                        grub_efi_uintn_t *buffer_size,
                                        void *buffer);
};
typedef struct grub_efi_load_file2 grub_efi_load_file2_t;

#define LINUX_EFI_INITRD_MEDIA_GUID  \
  { 0x5568e427, 0x68fc, 0x4f3d, \
    { 0xac, 0x74, 0xca, 0x55, 0x52, 0x31, 0xcc, 0x68 } \
  }

struct initrd_media_device_path
{
  grub_efi_vendor_media_device_path_t  vendor;
  grub_efi_device_path_t               end;
} GRUB_PACKED;
typedef struct initrd_media_device_path initrd_media_device_path_t;

/*
*******************************************************
 UNICODE DRAWING CHARACTERS			UNICODE绘图字符 
*******************************************************
#define BOXDRAW_HORIZONTAL 0x2500									//水平   ─		
#define BOXDRAW_VERTICAL 0x2502										//垂直   │
#define BOXDRAW_DOWN_RIGHT 0x250c									//左上角 ┌		uefi命名含义：绘画动作  从一点开始，向下画；再从这点向右画
#define BOXDRAW_DOWN_LEFT 0x2510									//右上角 ┐   UEFI固件的右上角(0x2510)是宽字符
#define BOXDRAW_UP_RIGHT 0x2514										//左下角 └
#define BOXDRAW_UP_LEFT 0x2518										//右下角 ┘
#define BOXDRAW_VERTICAL_RIGHT 0x251c							//垂直右 ├
#define BOXDRAW_VERTICAL_LEFT 0x2524							//垂直左 ┤
#define BOXDRAW_DOWN_HORIZONTAL 0x252c						//下水平 ┬
#define BOXDRAW_UP_HORIZONTAL 0x2534							//上水平 ┴
#define BOXDRAW_VERTICAL_HORIZONTAL 0x253c				//垂直水平 ┼
#define BOXDRAW_DOUBLE_HORIZONTAL 0x2550					//双水平 ═
#define BOXDRAW_DOUBLE_VERTICAL 0x2551						//双垂直 ║
#define BOXDRAW_DOWN_RIGHT_DOUBLE 0x2552					//下右双 ╒
#define BOXDRAW_DOWN_DOUBLE_RIGHT 0x2553					//下双右 ╓
#define BOXDRAW_DOUBLE_DOWN_RIGHT 0x2554					//双下右 ╔
#define BOXDRAW_DOWN_LEFT_DOUBLE 0x2555						//下左双 ╕
#define BOXDRAW_DOWN_DOUBLE_LEFT 0x2556						//下双左 ╖
#define BOXDRAW_DOUBLE_DOWN_LEFT 0x2557						//双下左 ╗
#define BOXDRAW_UP_RIGHT_DOUBLE 0x2558						//上右双 ╘
#define BOXDRAW_UP_DOUBLE_RIGHT 0x2559						//上双右 ╙
#define BOXDRAW_DOUBLE_UP_RIGHT 0x255a						//双上右 ╚
#define BOXDRAW_UP_LEFT_DOUBLE 0x255b							//上左双 ╛
#define BOXDRAW_UP_DOUBLE_LEFT 0x255c							//上双左 ╜
#define BOXDRAW_DOUBLE_UP_LEFT 0x255d							//双上左 ╝
#define BOXDRAW_VERTICAL_RIGHT_DOUBLE 0x255e			//垂直右双 ╞
#define BOXDRAW_VERTICAL_DOUBLE_RIGHT 0x255f			//垂直双右 ╟
#define BOXDRAW_DOUBLE_VERTICAL_RIGHT 0x2560			//双垂直右 ╠
#define BOXDRAW_VERTICAL_LEFT_DOUBLE 0x2561				//垂直左双 ╡
#define BOXDRAW_VERTICAL_DOUBLE_LEFT 0x2562				//垂直双左 ╢
#define BOXDRAW_DOUBLE_VERTICAL_LEFT 0x2563				//双垂直左 ╣
#define BOXDRAW_DOWN_HORIZONTAL_DOUBLE 0x2564			//下水平双 ╤
#define BOXDRAW_DOWN_DOUBLE_HORIZONTAL 0x2565			//下双水平 ╥
#define BOXDRAW_DOUBLE_DOWN_HORIZONTAL 0x2566			//双下水平 ╦
#define BOXDRAW_UP_HORIZONTAL_DOUBLE 0x2567				//上水平双 ╧
#define BOXDRAW_UP_DOUBLE_HORIZONTAL 0x2568				//上双水平 ╨
#define BOXDRAW_DOUBLE_UP_HORIZONTAL 0x2569				//双上水平 ╩
#define BOXDRAW_VERTICAL_HORIZONTAL_DOUBLE 0x256a	//垂直水平双 ╪
#define BOXDRAW_VERTICAL_DOUBLE_HORIZONTAL 0x256b	//垂直双水平 ╫
#define BOXDRAW_DOUBLE_VERTICAL_HORIZONTAL 0x256c	//双垂直水平 ╬
*******************************************************
 EFI Required Block Elements Code Chart		EFI所需块元素代码表
*******************************************************
#define BLOCKELEMENT_FULL_BLOCK 0x2588						//元素全块 █
#define BLOCKELEMENT_LIGHT_SHADE 0x2591						//元素浅色 ░
*******************************************************
 EFI Required Geometric Shapes Code Chart		EFI要求的几何形状代码表
*******************************************************
#define GEOMETRICSHAPE_UP_TRIANGLE 0x25b2					//向上三角形 ▲
#define GEOMETRICSHAPE_RIGHT_TRIANGLE 0x25ba			//向右三角形 ►
#define GEOMETRICSHAPE_DOWN_TRIANGLE 0x25bc				//向下三角形 ▼
#define GEOMETRICSHAPE_LEFT_TRIANGLE 0x25c4				//向左三角形 ◄
*******************************************************
 EFI Required Arrow shapes		EFI所需箭头形状
*******************************************************
#define ARROW_LEFT 0x2190													//向左箭头 ←
#define ARROW_UP 0x2191														//向上箭头 ↑
#define ARROW_RIGHT 0x2192												//向右箭头 →
#define ARROW_DOWN 0x2193													//向下箭头 ↓


EFI简单文本输入协议定义了一个包含Unicode字符和所需EFI扫描代码的输入流。
只有表106中定义的控制字符在Unicode输入或输出流中具有含义。控制字符定义为从U+0000到U+001F的字符。输入流不支持任何软件流控制。 
表106。支持的Unicode控制字符 
助记符	Unicode代码		描述
Null		U+0000				空字符在接收时被忽略
BS			U+0008				退格。将光标左移一列。如果光标位于左边距，则不执行任何操作。 
TAB			U+0x0009			TAB
LF			U+000A				换行。将光标移到下一行。				注意：只换行，不回车。
CR			U+000D				回车。将光标移到当前行的左边距。注意：只回车。

除了Unicode字符外，输入流还支持扫描代码。如果扫描代码设置为0x00，则Unicode字符有效，应使用。如果扫描代码设置为非0x00值，则表示表107定义的特殊键。 
表107。EFI简单文本输入协议的EFI扫描码 
EFI扫描码		描述
0x00				空扫描代码。 
0x01				将光标上移一行。 
0x02				将光标下移一行。 
0x03				将光标右移1列。 
0x04				将光标左移1列。 
0x05				回家。 Home
0x06				结束。 end
0x07				插入。 Insert
0x08				删除。 Delete
0x09				向上翻页。 Page Up
0x0a 				向下翻页。 Page Down
0x0b 				功能1。 
0x0c 				功能2。 
0x0d				功能3。 
0x0e				功能4。 
0x0f				功能5。 
0x10				功能6。 
0x11				功能7。 
0x12				功能8。 
0x13				功能9。 
0x14				功能10。 
0x17				逃离。 Escape

表108。EFI简单文本输入扩展协议的EFI扫描码 
EFI扫描码		描述
0x15				功能11
0x16				功能12
0x68				功能13 
0x69				功能14 
0x6A				功能15 
0x6B				功能16 
0x6C				功能17 
0x6D				功能18 
0x6E				功能19 
0x6F				功能20 
0x70				功能21 
0x71				功能22 
0x72				功能23 
0x73				功能24 
0x7F				静音 
0x80				音量增大 
0x81				音量降低 
0x100				亮度增大 
0x101				亮度降低 
0x102				暂停 
0x103				休眠 
0x104				切换显示 
0x105				恢复 
0x106				弹出 
0x8000-0xFFFF 	原始设备制造商保留 

字符串显示在输出设备上的当前光标位置，光标根据表109中列出的规则前进。
Table 109. EFI 光标位置/前进规则 
助记符		Unicode代码	描述 
Null			U+0000			忽略字符，不要移动光标。
BS				U+0008			如果光标不在显示器的左边缘，则将光标向左移动一列。
LF				U+000A			如果光标位于显示器底部，则将显示器滚动一行，不要更新光标位置。否则，将光标下移一行。	注意：只换行，不回车。
CR				U+000D			将光标移到当前行的开头。																															注意：只回车。
Other			U+XXXX			在当前光标位置打印字符并将光标向右移动一列。如果这将光标移过显示器的右边缘，则该行应换行到下一行的开头。
											这相当于插入一个CR和一个LF。注意，如果光标位于显示器的底部，并且行换行，则显示器将滚动一行。
*/

/*
#if (GRUB_TARGET_SIZEOF_VOID_P == 4) || defined (__ia64__) \
  || defined (__aarch64__) || defined (__MINGW64__) || defined (__CYGWIN__)
*/
#if defined(__i386__)
#define efi_call_0(func)		func()
#define efi_call_1(func, a)		func(a)
#define efi_call_2(func, a, b)		func(a, b)
#define efi_call_3(func, a, b, c)		func(a, b, c)
#define efi_call_4(func, a, b, c, d)	func(a, b, c, d)
#define efi_call_5(func, a, b, c, d, e)		func(a, b, c, d, e)
#define efi_call_6(func, a, b, c, d, e, f)	func(a, b, c, d, e, f)
#define efi_call_7(func, a, b, c, d, e, f, g)		func(a, b, c, d, e, f, g)
#define efi_call_10(func, a, b, c, d, e, f, g, h, i, j)		func(a, b, c, d, e, f, g, h, i, j)

#else

#define efi_call_0(func) \
  efi_wrap_0(func)
#define efi_call_1(func, a) \
  efi_wrap_1(func, (unsigned long long) (a))
#define efi_call_2(func, a, b) \
  efi_wrap_2(func, (unsigned long long) (a), (unsigned long long) (b))
#define efi_call_3(func, a, b, c) \
  efi_wrap_3(func, (unsigned long long) (a), (unsigned long long) (b), \
	     (unsigned long long) (c))
#define efi_call_4(func, a, b, c, d) \
  efi_wrap_4(func, (unsigned long long) (a), (unsigned long long) (b), \
	     (unsigned long long) (c), (unsigned long long) (d))
#define efi_call_5(func, a, b, c, d, e)	\
  efi_wrap_5(func, (unsigned long long) (a), (unsigned long long) (b), \
	     (unsigned long long) (c), (unsigned long long) (d), (unsigned long long) (e))
#define efi_call_6(func, a, b, c, d, e, f) \
  efi_wrap_6(func, (unsigned long long) (a), (unsigned long long) (b), \
	     (unsigned long long) (c), (unsigned long long) (d), (unsigned long long) (e), \
	     (unsigned long long) (f))
#define efi_call_7(func, a, b, c, d, e, f, g) \
  efi_wrap_7(func, (unsigned long long) (a), (unsigned long long) (b), \
	     (unsigned long long) (c), (unsigned long long) (d), (unsigned long long) (e), \
	     (unsigned long long) (f), (unsigned long long) (g))
#define efi_call_10(func, a, b, c, d, e, f, g, h, i, j) \
  efi_wrap_10(func, (unsigned long long) (a), (unsigned long long) (b), \
	      (unsigned long long) (c), (unsigned long long) (d), (unsigned long long) (e), \
	      (unsigned long long) (f), (unsigned long long) (g),	(unsigned long long) (h), \
	      (unsigned long long) (i), (unsigned long long) (j))

unsigned long long EXPORT_FUNC(efi_wrap_0) (void *func);
unsigned long long EXPORT_FUNC(efi_wrap_1) (void *func, unsigned long long arg1);
unsigned long long EXPORT_FUNC(efi_wrap_2) (void *func, unsigned long long arg1,
                                       unsigned long long arg2);
unsigned long long EXPORT_FUNC(efi_wrap_3) (void *func, unsigned long long arg1,
                                       unsigned long long arg2, unsigned long long arg3);
unsigned long long EXPORT_FUNC(efi_wrap_4) (void *func, unsigned long long arg1,
                                       unsigned long long arg2, unsigned long long arg3,
                                       unsigned long long arg4);
unsigned long long EXPORT_FUNC(efi_wrap_5) (void *func, unsigned long long arg1,
                                       unsigned long long arg2, unsigned long long arg3,
                                       unsigned long long arg4, unsigned long long arg5);
unsigned long long EXPORT_FUNC(efi_wrap_6) (void *func, unsigned long long arg1,
                                       unsigned long long arg2, unsigned long long arg3,
                                       unsigned long long arg4, unsigned long long arg5,
                                       unsigned long long arg6);
unsigned long long EXPORT_FUNC(efi_wrap_7) (void *func, unsigned long long arg1,
                                       unsigned long long arg2, unsigned long long arg3,
                                       unsigned long long arg4, unsigned long long arg5,
                                       unsigned long long arg6, unsigned long long arg7);
unsigned long long EXPORT_FUNC(efi_wrap_10) (void *func, unsigned long long arg1,
                                        unsigned long long arg2, unsigned long long arg3,
                                        unsigned long long arg4, unsigned long long arg5,
                                        unsigned long long arg6, unsigned long long arg7,
                                        unsigned long long arg8, unsigned long long arg9,
                                        unsigned long long arg10);
#endif
//_____________________________________________________________________________________
//grub/err.h
#define GRUB_MAX_ERRMSG		256
typedef enum
  {
    GRUB_ERR_NONE = 0,
    GRUB_ERR_TEST_FAILURE,
    GRUB_ERR_BAD_MODULE,
    GRUB_ERR_OUT_OF_MEMORY,
    GRUB_ERR_BAD_FILE_TYPE,
    GRUB_ERR_FILE_NOT_FOUND,
    GRUB_ERR_FILE_READ_ERROR,
    GRUB_ERR_BAD_FILENAME,
    GRUB_ERR_UNKNOWN_FS,
    GRUB_ERR_BAD_FS,
    GRUB_ERR_BAD_NUMBER,
    GRUB_ERR_OUT_OF_RANGE,
    GRUB_ERR_UNKNOWN_DEVICE,
    GRUB_ERR_BAD_DEVICE,
    GRUB_ERR_READ_ERROR,
    GRUB_ERR_WRITE_ERROR,
    GRUB_ERR_UNKNOWN_COMMAND,
    GRUB_ERR_INVALID_COMMAND,
    GRUB_ERR_BAD_ARGUMENT,
    GRUB_ERR_BAD_PART_TABLE,
    GRUB_ERR_UNKNOWN_OS,
    GRUB_ERR_BAD_OS,
    GRUB_ERR_NO_KERNEL,
    GRUB_ERR_BAD_FONT,
    GRUB_ERR_NOT_IMPLEMENTED_YET,
    GRUB_ERR_SYMLINK_LOOP,
    GRUB_ERR_BAD_COMPRESSED_DATA,
    GRUB_ERR_MENU,
    GRUB_ERR_TIMEOUT,
    GRUB_ERR_IO,
    GRUB_ERR_ACCESS_DENIED,
    GRUB_ERR_EXTRACTOR,
    GRUB_ERR_NET_BAD_ADDRESS,
    GRUB_ERR_NET_ROUTE_LOOP,
    GRUB_ERR_NET_NO_ROUTE,
    GRUB_ERR_NET_NO_ANSWER,
    GRUB_ERR_NET_NO_CARD,
    GRUB_ERR_WAIT,
    GRUB_ERR_BUG,
    GRUB_ERR_NET_PORT_CLOSED,
    GRUB_ERR_NET_INVALID_RESPONSE,
    GRUB_ERR_NET_UNKNOWN_ERROR,
    GRUB_ERR_NET_PACKET_TOO_BIG,
    GRUB_ERR_NET_NO_DOMAIN,
    GRUB_ERR_EOF,
    GRUB_ERR_BAD_SIGNATURE
  }
grub_err_t;

extern grub_err_t grub_errno;

struct grub_error_saved
{
  grub_err_t grub_errno;
  char errmsg[GRUB_MAX_ERRMSG];
};

extern grub_err_t EXPORT_VAR(grub_errno);
//-----------------------------------------------------------------------------------------------
//grub/types.h

//grub/efi/efi.h
/* Functions.  */
extern void *EXPORT_FUNC(grub_efi_locate_protocol) (grub_efi_guid_t *protocol,
					     void *registration);
extern grub_efi_handle_t *
EXPORT_FUNC(grub_efi_locate_handle) (grub_efi_locate_search_type_t search_type,
				     grub_efi_guid_t *protocol,
				     void *search_key,
				     grub_efi_uintn_t *num_handles);
extern void *EXPORT_FUNC(grub_efi_open_protocol) (grub_efi_handle_t handle,
					   grub_efi_guid_t *protocol,
					   grub_efi_uint32_t attributes);
extern int EXPORT_FUNC(grub_efi_set_text_mode) (int on);
extern void EXPORT_FUNC(grub_efi_stall) (grub_efi_uintn_t microseconds);
extern void *
EXPORT_FUNC(grub_efi_allocate_pages_real) (grub_efi_physical_address_t address,
				           grub_efi_uintn_t pages,
					   grub_efi_allocate_type_t alloctype,
					   grub_efi_memory_type_t memtype);
extern void *
EXPORT_FUNC(grub_efi_allocate_fixed) (grub_efi_physical_address_t address,
				      grub_efi_uintn_t pages);
extern void *
EXPORT_FUNC(grub_efi_allocate_any_pages) (grub_efi_uintn_t pages);
extern void EXPORT_FUNC(grub_efi_free_pages) (grub_efi_physical_address_t address,
				       grub_efi_uintn_t pages);
extern grub_efi_uintn_t EXPORT_FUNC(grub_efi_find_mmap_size) (void);
extern int
EXPORT_FUNC(grub_efi_get_memory_map) (grub_efi_uintn_t *memory_map_size,
				      grub_efi_memory_descriptor_t *memory_map,
				      grub_efi_uintn_t *map_key,
				      grub_efi_uintn_t *descriptor_size,
				      grub_efi_uint32_t *descriptor_version);
extern void grub_efi_memory_fini (void);
extern grub_efi_loaded_image_t *grub_efi_get_loaded_image (grub_efi_handle_t image_handle);
extern void EXPORT_FUNC(grub_efi_print_device_path) (grub_efi_device_path_t *dp);
extern char *EXPORT_FUNC(grub_efi_get_filename) (grub_efi_device_path_t *dp);
extern grub_efi_device_path_t *
EXPORT_FUNC(grub_efi_get_device_path) (grub_efi_handle_t handle);
extern grub_efi_device_path_t *
EXPORT_FUNC(grub_efi_find_last_device_path) (const grub_efi_device_path_t *dp);
extern grub_efi_device_path_t *
EXPORT_FUNC(grub_efi_duplicate_device_path) (const grub_efi_device_path_t *dp);
extern grub_err_t EXPORT_FUNC (grub_efi_finish_boot_services) (grub_efi_uintn_t *outbuf_size, void *outbuf,
							grub_efi_uintn_t *map_key,
							grub_efi_uintn_t *efi_desc_size,
							grub_efi_uint32_t *efi_desc_version);
extern grub_err_t EXPORT_FUNC (grub_efi_set_virtual_address_map) (grub_efi_uintn_t memory_map_size,
							   grub_efi_uintn_t descriptor_size,
							   grub_efi_uint32_t descriptor_version,
							   grub_efi_memory_descriptor_t *virtual_map);
extern void *EXPORT_FUNC (grub_efi_get_variable) (const char *variable,
					   const grub_efi_guid_t *guid,
					   grub_size_t *datasize_out);
extern grub_err_t
EXPORT_FUNC (grub_efi_set_variable) (const char *var,
				     const grub_efi_guid_t *guid,
				     void *data,
				     grub_size_t datasize);
extern int
EXPORT_FUNC (grub_efi_compare_device_paths) (const grub_efi_device_path_t *dp1,
					     const grub_efi_device_path_t *dp2);

extern void (*EXPORT_VAR(grub_efi_net_config)) (grub_efi_handle_t hnd, 
						char **device,
						char **path);

#if defined(__arm__) || defined(__aarch64__)
extern void *EXPORT_FUNC(grub_efi_get_firmware_fdt)(void);
extern grub_err_t EXPORT_FUNC(grub_efi_get_ram_base)(grub_addr_t *);
#include <grub/cpu/linux.h>
extern grub_err_t grub_armxx_efi_linux_check_image(struct linux_armxx_kernel_header *lh);
extern grub_err_t grub_armxx_efi_linux_boot_image(grub_addr_t addr, grub_size_t size,
                                           char *args);
#endif

extern grub_addr_t grub_efi_modules_addr (void);

extern void grub_efi_mm_init (void);
extern void grub_efi_mm_fini (void);
extern void grub_efi_init (void);
extern void grub_efi_fini (void);
extern void grub_efi_set_prefix (void);

/* Variables.  */
extern grub_efi_system_table_t *EXPORT_VAR(grub_efi_system_table);
extern grub_efi_handle_t EXPORT_VAR(grub_efi_image_handle);

extern int EXPORT_VAR(grub_efi_is_finished);

struct grub_net_card;	//网卡

extern grub_efi_handle_t
grub_efinet_get_device_handle (struct grub_net_card *card);
//------------------------------------------------------------------------------------
//grub/efi/console_control.h
#define GRUB_EFI_CONSOLE_CONTROL_GUID	\
  { 0xf42f7782, 0x12e, 0x4c12, \
    { 0x99, 0x56, 0x49, 0xf9, 0x43, 0x4, 0xf7, 0x21 } \
  }	//控制台控件GUID 
	
enum grub_efi_screen_mode
  {
    GRUB_EFI_SCREEN_TEXT,
    GRUB_EFI_SCREEN_GRAPHICS,
    GRUB_EFI_SCREEN_TEXT_MAX_VALUE
  };
typedef enum grub_efi_screen_mode grub_efi_screen_mode_t;

struct grub_efi_console_control_protocol
{
  grub_efi_status_t
  (*get_mode) (struct grub_efi_console_control_protocol *this,
	       grub_efi_screen_mode_t *mode,
	       grub_efi_boolean_t *uga_exists,
	       grub_efi_boolean_t *std_in_locked);

  grub_efi_status_t
  (*set_mode) (struct grub_efi_console_control_protocol *this,
	       grub_efi_screen_mode_t mode);

  grub_efi_status_t
  (*lock_std_in) (struct grub_efi_console_control_protocol *this,
		  grub_efi_char16_t *password);
};
typedef struct grub_efi_console_control_protocol grub_efi_console_control_protocol_t;
//--------------------------------------------------------------------------------------------
//grub/charset.h
#define GRUB_UINT8_1_LEADINGBIT 0x80
#define GRUB_UINT8_2_LEADINGBITS 0xc0
#define GRUB_UINT8_3_LEADINGBITS 0xe0
#define GRUB_UINT8_4_LEADINGBITS 0xf0
#define GRUB_UINT8_5_LEADINGBITS 0xf8
#define GRUB_UINT8_6_LEADINGBITS 0xfc
#define GRUB_UINT8_7_LEADINGBITS 0xfe

#define GRUB_UINT8_1_TRAILINGBIT 0x01
#define GRUB_UINT8_2_TRAILINGBITS 0x03
#define GRUB_UINT8_3_TRAILINGBITS 0x07
#define GRUB_UINT8_4_TRAILINGBITS 0x0f
#define GRUB_UINT8_5_TRAILINGBITS 0x1f
#define GRUB_UINT8_6_TRAILINGBITS 0x3f

#define GRUB_MAX_UTF8_PER_UTF16 4
/* You need at least one UTF-8 byte to have one UTF-16 word.
   You need at least three UTF-8 bytes to have 2 UTF-16 words (surrogate pairs).
 */
#define GRUB_MAX_UTF16_PER_UTF8 1
#define GRUB_MAX_UTF8_PER_CODEPOINT 4

#define GRUB_UCS2_LIMIT 0x10000
#define GRUB_UTF16_UPPER_SURROGATE(code) \
  (0xD800 | ((((code) - GRUB_UCS2_LIMIT) >> 10) & 0x3ff))
#define GRUB_UTF16_LOWER_SURROGATE(code) \
  (0xDC00 | (((code) - GRUB_UCS2_LIMIT) & 0x3ff))

#define GRUB_UCS2_LIMIT 0x10000
#define GRUB_UTF16_UPPER_SURROGATE(code) \
  (0xD800 | ((((code) - GRUB_UCS2_LIMIT) >> 10) & 0x3ff))
#define GRUB_UTF16_LOWER_SURROGATE(code) \
  (0xDC00 | (((code) - GRUB_UCS2_LIMIT) & 0x3ff))
	
/* Process one character from UTF8 sequence. 
   At beginning set *code = 0, *count = 0. Returns 0 on failure and
   1 on success. *count holds the number of trailing bytes.  */
static inline int
grub_utf8_process (grub_uint8_t c, grub_uint32_t *code, int *count)
{
  if (*count)
    {
      if ((c & GRUB_UINT8_2_LEADINGBITS) != GRUB_UINT8_1_LEADINGBIT)
	{
	  *count = 0;
	  /* invalid */
	  return 0;
	}
      else
	{
	  *code <<= 6;
	  *code |= (c & GRUB_UINT8_6_TRAILINGBITS);
	  (*count)--;
	  /* Overlong.  */
	  if ((*count == 1 && *code <= 0x1f)
	      || (*count == 2 && *code <= 0xf))
	    {
	      *code = 0;
	      *count = 0;
	      return 0;
	    }
	  return 1;
	}
    }

  if ((c & GRUB_UINT8_1_LEADINGBIT) == 0)
    {
      *code = c;
      return 1;
    }
  if ((c & GRUB_UINT8_3_LEADINGBITS) == GRUB_UINT8_2_LEADINGBITS)
    {
      *count = 1;
      *code = c & GRUB_UINT8_5_TRAILINGBITS;
      /* Overlong */
      if (*code <= 1)
	{
	  *count = 0;
	  *code = 0;
	  return 0;
	}
      return 1;
    }
  if ((c & GRUB_UINT8_4_LEADINGBITS) == GRUB_UINT8_3_LEADINGBITS)
    {
      *count = 2;
      *code = c & GRUB_UINT8_4_TRAILINGBITS;
      return 1;
    }
  if ((c & GRUB_UINT8_5_LEADINGBITS) == GRUB_UINT8_4_LEADINGBITS)
    {
      *count = 3;
      *code = c & GRUB_UINT8_3_TRAILINGBITS;
      return 1;
    }
  return 0;
}
	
/* Convert a (possibly null-terminated) UTF-8 string of at most SRCSIZE
   bytes (if SRCSIZE is -1, it is ignored) in length to a UTF-16 string.
   Return the number of characters converted. DEST must be able to hold
   at least DESTSIZE characters. If an invalid sequence is found, return -1.
   If SRCEND is not NULL, then *SRCEND is set to the next byte after the
   last byte used in SRC.  */
static inline grub_size_t
grub_utf8_to_utf16 (grub_uint16_t *dest, grub_size_t destsize,
		    const grub_uint8_t *src, grub_size_t srcsize,
		    const grub_uint8_t **srcend)
{
  grub_uint16_t *p = dest;
  int count = 0;
  grub_uint32_t code = 0;

  if (srcend)
    *srcend = src;

  while (srcsize && destsize)
    {
      int was_count = count;
      if (srcsize != (grub_size_t)-1)
	srcsize--;
      if (!grub_utf8_process (*src++, &code, &count))
	{
	  code = '?';
	  count = 0;
	  /* Character c may be valid, don't eat it.  */
	  if (was_count)
	    src--;
	}
      if (count != 0)
	continue;
      if (code == 0)
	break;
      if (destsize < 2 && code >= GRUB_UCS2_LIMIT)
	break;
      if (code >= GRUB_UCS2_LIMIT)
	{
	  *p++ = GRUB_UTF16_UPPER_SURROGATE (code);
	  *p++ = GRUB_UTF16_LOWER_SURROGATE (code);
	  destsize -= 2;
	}
      else
	{
	  *p++ = code;
	  destsize--;
	}
    }

  if (srcend)
    *srcend = src;
  return p - dest;
}
//--------------------------------------------------------------------------------------------
//
#define GRUB_MAX_UTF16_PER_UTF8 1
#define GRUB_MAX_UTF8_PER_UTF16 4
#define GRUB_LOADER_FLAG_NORETURN 1
extern grub_err_t EXPORT_VAR(grub_errno);

#define GRUB_PE32_SIGNATURE_SIZE 4
#define GRUB_PE32_MSDOS_STUB_SIZE	0x80

//grub/efi/pe32.h
struct grub_pe32_data_directory
{
  unsigned int rva;  //RVA
  unsigned int size; //尺寸
};

struct grub_pe32_section_table        //段表
{
  char name[8];                       //名称          msds
  unsigned int virtual_size;          //虚拟尺寸      047c00
  unsigned int virtual_address;       //虚拟地址      019400
  unsigned int raw_data_size;         //原始数据尺寸  047c00
  unsigned int raw_data_offset;       //原始数据偏移  019400
  unsigned int relocations_offset;    //重定位偏移    0
  unsigned int line_numbers_offset;   //行数偏移      0
  unsigned short num_relocations;     //重新定位数    0
  unsigned short num_line_numbers;    //行数          0
  unsigned int characteristics;       //特点          c0000040
};

struct grub_pe64_optional_header
{
  unsigned short magic;                //魔术          020b
  unsigned char major_linker_version;  //主连接器版本  0
  unsigned char minor_linker_version;  //次连接器版本  0
  unsigned int code_size;              //代码尺寸      a000
  unsigned int data_size;              //数据尺寸      56c00
  unsigned int bss_size;               //BSS尺寸       0
  unsigned int entry_addr;             //入口地址      400
  unsigned int code_base;              //代码基址      400

  unsigned long long image_base;           //映像基址      0

  unsigned int section_alignment;      //段对齐        200
  unsigned int file_alignment;         //文件对齐      200
  unsigned short major_os_version;     //主操作版本    0
  unsigned short minor_os_version;     //次操作版本    0
  unsigned short major_image_version;  //主映像版本    0
  unsigned short minor_image_version;  //次映像版本    0
  unsigned short major_subsystem_version;//主子系统版本  0
  unsigned short minor_subsystem_version;//次子系统版本  0
  unsigned int reserved;              //保留          0
  unsigned int image_size;            //映像尺寸      061e00
  unsigned int header_size;           //头部尺寸      400
  unsigned int checksum;              //校验和        0
  unsigned short subsystem;            //子系统        0a
  unsigned short dll_characteristics;  //DLL特征       0

  unsigned long long stack_reserve_size;   //栈保留尺寸    10000
  unsigned long long stack_commit_size;    //栈提交尺寸    10000
  unsigned long long heap_reserve_size;    //堆保留大小    10000
  unsigned long long heap_commit_size;     //堆提交大小    10000

  unsigned int loader_flags;         //加载器标志    0
  unsigned int num_data_directories; //数据目录数    10

  /* Data directories.  */
  struct grub_pe32_data_directory export_table;           //出口表       0,0   占8字节
  struct grub_pe32_data_directory import_table;           //入口表       0,0
  struct grub_pe32_data_directory resource_table;         //资源表       0,0
  struct grub_pe32_data_directory exception_table;        //例外表       0,0
  struct grub_pe32_data_directory certificate_table;      //证书表       0,0
  struct grub_pe32_data_directory base_relocation_table;  //重定位表基址  61000,0e00
  struct grub_pe32_data_directory debug;                  //调试          0,0
  struct grub_pe32_data_directory architecture;           //结构          0,0
  struct grub_pe32_data_directory global_ptr;             //全局指针      0,0
  struct grub_pe32_data_directory tls_table;              //TLS表         0,0
  struct grub_pe32_data_directory load_config_table;      //加载配置表    0,0
  struct grub_pe32_data_directory bound_import;           //绑定输入      0,0
  struct grub_pe32_data_directory iat;                    //iat           0,0
  struct grub_pe32_data_directory delay_import_descriptor;//延迟导入描述符0,0
  struct grub_pe32_data_directory com_runtime_header;     //COM运行时报头 0,0
  struct grub_pe32_data_directory reserved_entry;         //保留条目      0,0
};

struct grub_pe32_coff_header            //COFF文件头
{
  unsigned short machine;               //机器          8664
  unsigned short num_sections;          //段数          0004
  unsigned int time;                    //时间          54a48e00
  unsigned int symtab_offset;           //符号偏移      0
  unsigned int num_symbols;             //符号数        0
  unsigned short optional_header_size;  //可选标题大小  00f0
  unsigned short characteristics;       //特点          020e
};

struct grub_pe32_header   //PE32 头
{
  /* This should be filled in with GRUB_PE32_MSDOS_STUB. 这应该用GRUB_PE32_MSDOS_STUB */
  unsigned char msdos_stub[GRUB_PE32_MSDOS_STUB_SIZE]; //[80] 根尺寸

  /* This is always PE\0\0. 这总是PE\0\0 */
  char signature[GRUB_PE32_SIGNATURE_SIZE]; //[4] 签名长度

  /* The COFF file header. COFF文件头 */
  struct grub_pe32_coff_header coff_header; //COFF文件头 占0x14字节

  /* The Optional header.  */
  struct grub_pe64_optional_header optional_header; //可选头部  a0000000020b
};



struct grub_pe32_optional_header          //可选头部
{
  unsigned short magic;                    //魔术          020b
  unsigned char major_linker_version;      //主连接器版本  0
  unsigned char minor_linker_version;      //次连接器版本  0
  unsigned int code_size;                //代码尺寸      a000
  unsigned int data_size;                //数据尺寸      56c00
  unsigned int bss_size;                 //BSS尺寸       0
  unsigned int entry_addr;               //入口地址      400
  unsigned int code_base;                //代码基址      400

  unsigned int data_base;                //数据基址      0
  unsigned int image_base;               //映像基址      0

  unsigned int section_alignment;        //段对齐        200
  unsigned int file_alignment;           //文件对齐      200
  unsigned short major_os_version;         //主操作版本    0
  unsigned short minor_os_version;         //次操作版本    0
  unsigned short major_image_version;      //主映像版本    0
  unsigned short minor_image_version;      //次映像版本    0
  unsigned short major_subsystem_version;  //主子系统版本  0
  unsigned short minor_subsystem_version;  //次子系统版本  0
  unsigned int reserved;                 //保留          0
  unsigned int image_size;               //映像尺寸      061e00
  unsigned int header_size;              //头部尺寸      400
  unsigned int checksum;                 //校验和        0
  unsigned short subsystem;                //子系统        0a
  unsigned short dll_characteristics;      //DLL特征       0

  unsigned int stack_reserve_size;       //栈保留尺寸    10000
  unsigned int stack_commit_size;        //栈提交尺寸    0        ??
  unsigned int heap_reserve_size;        //堆保留大小    10000
  unsigned int heap_commit_size;         //堆提交尺寸    0        ??

  unsigned int loader_flags;             //加载器标志    10000
  unsigned int num_data_directories;     //数据目录数    0        ??

  /* Data directories. 数据目录  */
  struct grub_pe32_data_directory export_table;           //出口表
  struct grub_pe32_data_directory import_table;           //入口表
  struct grub_pe32_data_directory resource_table;         //资源表
  struct grub_pe32_data_directory exception_table;        //例外表
  struct grub_pe32_data_directory certificate_table;      //证书表
  struct grub_pe32_data_directory base_relocation_table;  //重定位表基址
  struct grub_pe32_data_directory debug;                  //调试
  struct grub_pe32_data_directory architecture;           //结构 
  struct grub_pe32_data_directory global_ptr;             //全局指针
  struct grub_pe32_data_directory tls_table;              //TLS表 
  struct grub_pe32_data_directory load_config_table;      //加载配置表
  struct grub_pe32_data_directory bound_import;           //绑定输入
  struct grub_pe32_data_directory iat;                    //iat
  struct grub_pe32_data_directory delay_import_descriptor;//延迟导入描述符
  struct grub_pe32_data_directory com_runtime_header;     //COM运行时报头
  struct grub_pe32_data_directory reserved_entry;         //保留条目
};

/* The start point of the C code.  */
void grub_main (void) __attribute__ ((noreturn));
extern unsigned int next_partition_drive;
extern unsigned int next_partition_dest;
extern unsigned int *next_partition_partition;
extern unsigned int *next_partition_type;
extern unsigned long long *next_partition_start;
extern unsigned long long *next_partition_len;
extern unsigned long long *next_partition_offset;
extern unsigned int *next_partition_entry;
extern unsigned int *next_partition_ext_offset;
extern char *next_partition_buf;
extern unsigned char partition_signature[16]; //分区签名
extern unsigned char partition_activity_flag; //分区活动标志
extern unsigned char *UNIFONT_START;
extern unsigned char *narrow_mem;
extern char *PAGING_TABLES_BUF;
extern unsigned char *PRINTF_BUFFER;
extern char *MENU_TITLE;
extern char *cmd_buffer;
extern char *FSYS_BUF;
extern char *CMDLINE_BUF;
extern char *COMPLETION_BUF;
extern char *UNIQUE_BUF;
extern char *HISTORY_BUF;
extern char *BUFFERADDR;
extern char *BASE_ADDR;
extern char *CMD_RUN_ON_EXIT;
extern char *SCRATCHADDR;
extern int return_value;
extern long long return_value64;
//extern int QUOTE_CHAR;
//extern char *GRUB_MOD_ADDR;
//extern char* mod_end;
extern char *CONFIG_ENTRIES;
extern unsigned char *IMAGE_BUFFER;
extern unsigned char *JPG_FILE;
extern char *menu_mem;

extern int grub_efidisk_readwrite (int drive, grub_disk_addr_t sector,
			grub_size_t size, char *buf, int read_write);
			
extern char *grub_image;
extern char *g4e_data;
extern grub_efi_loaded_image_t *image;
extern grub_efi_device_path_t *efi_file_path;
extern grub_efi_handle_t efi_handle;
extern void grub_machine_fini (void);

#define OBJ_TYPE_ELF     0x00  // 外部命令
#define OBJ_TYPE_MEMDISK 0x01  // MOD文件
#define OBJ_TYPE_CONFIG  0x02  // 配置菜单
#define OBJ_TYPE_PREFIX  0x03  // 前缀路径
#define OBJ_TYPE_FONT    0x04  // 字体

/* The module header.  */
struct grub_module_header
{
  /* The type of object.  */
  grub_uint16_t type;
  /* real_size = size - sizeof (struct grub_module_header) - pad_size */
  grub_uint16_t pad_size;
  /* The size of object (including this header).  */
  grub_uint32_t size;
} GRUB_PACKED;

/* "gmim" (GRUB Module Info Magic).  GRUB模块信息魔法 */
#define GRUB_MODULE_MAGIC 0x676d696d

struct grub_module_info32
{
  /* Magic number so we know we have modules present. 神奇的数字，所以我们知道我们有模块存在  */
  grub_uint32_t magic;
  /* The offset of the modules.  模的偏移量*/
  grub_uint32_t offset;
  /* The size of all modules plus this header.  所有模块的大小加上这个标题 */
  grub_uint32_t size;
};

struct grub_module_info64
{
  /* Magic number so we know we have modules present. 神奇的数字，所以我们知道我们有模块存在 */
  grub_uint32_t magic;
  grub_uint32_t padding;
  /* The offset of the modules. 模的偏移量*/
  grub_uint64_t offset;
  /* The size of all modules plus this header.  所有模块的大小加上这个标题 */
  grub_uint64_t size;
};

//#if GRUB_TARGET_SIZEOF_VOID_P == 8
#if !defined(__i386__)
#define grub_module_info grub_module_info64
#else
#define grub_module_info grub_module_info32
#endif

extern grub_addr_t EXPORT_VAR (grub_modbase);

#define FOR_MODULES(var)  for (\
  var = (grub_modbase && ((((struct grub_module_info *) grub_modbase)->magic) == GRUB_MODULE_MAGIC)) ? (struct grub_module_header *) \
    (grub_modbase + (((struct grub_module_info *) grub_modbase)->offset)) : 0;\
  var && (grub_addr_t) var \
    < (grub_modbase + (((struct grub_module_info *) grub_modbase)->size));    \
  var = (struct grub_module_header *)					\
    (((grub_uint32_t *) var) + ((((struct grub_module_header *) var)->size + sizeof (grub_addr_t) - 1) / sizeof (grub_addr_t)) * (sizeof (grub_addr_t) / sizeof (grub_uint32_t))))

//----------------------------------------------------------------------------------------------------------
//macho.h
/* Multi-architecture header. Always in big-endian. */

#define GRUB_MACHO_CPUTYPE_IS_HOST32(x) ((x) == GRUB_MACHO_CPUTYPE_IA32)
#define GRUB_MACHO_CPUTYPE_IS_HOST64(x) ((x) == GRUB_MACHO_CPUTYPE_AMD64)
//#ifdef __x86_64__
//#define GRUB_MACHO_CPUTYPE_IS_HOST_CURRENT(x) ((x) == GRUB_MACHO_CPUTYPE_AMD64)
//#else
#define GRUB_MACHO_CPUTYPE_IS_HOST_CURRENT(x) ((x) == GRUB_MACHO_CPUTYPE_IA32)
//#endif

struct grub_macho_fat_header
{
  grub_uint32_t magic;
  grub_uint32_t nfat_arch;
} GRUB_PACKED;

enum
  {
    GRUB_MACHO_CPUTYPE_IA32 = 0x00000007,
    GRUB_MACHO_CPUTYPE_AMD64 = 0x01000007
  };

#define GRUB_MACHO_FAT_MAGIC 0xcafebabe				//大男子FAT魔法
#define GRUB_MACHO_FAT_EFI_MAGIC 0x0ef1fab9U	//大男子FAT_EFI魔法

typedef grub_uint32_t grub_macho_cpu_type_t;
typedef grub_uint32_t grub_macho_cpu_subtype_t;

/* Architecture descriptor. Always in big-endian. */
struct grub_macho_fat_arch
{
  grub_macho_cpu_type_t cputype;
  grub_macho_cpu_subtype_t cpusubtype;
  grub_uint32_t offset;
  grub_uint32_t size;
  grub_uint32_t align;
} GRUB_PACKED;

/* File header for 32-bit. Always in native-endian. */
struct grub_macho_header32
{
#define GRUB_MACHO_MAGIC32 0xfeedface
  grub_uint32_t magic;
  grub_macho_cpu_type_t cputype;
  grub_macho_cpu_subtype_t cpusubtype;
  grub_uint32_t filetype;
  grub_uint32_t ncmds;
  grub_uint32_t sizeofcmds;
  grub_uint32_t flags;
} GRUB_PACKED;

/* File header for 64-bit. Always in native-endian. */
struct grub_macho_header64
{
#define GRUB_MACHO_MAGIC64 0xfeedfacf
  grub_uint32_t magic;
  grub_macho_cpu_type_t cputype;
  grub_macho_cpu_subtype_t cpusubtype;
  grub_uint32_t filetype;
  grub_uint32_t ncmds;
  grub_uint32_t sizeofcmds;
  grub_uint32_t flags;
  grub_uint32_t reserved;
} GRUB_PACKED;

/* Common header of Mach-O commands. */
struct grub_macho_cmd
{
  grub_uint32_t cmd;
  grub_uint32_t cmdsize;
} GRUB_PACKED;

typedef grub_uint32_t grub_macho_vmprot_t;

/* 32-bit segment command. */
struct grub_macho_segment32
{
#define GRUB_MACHO_CMD_SEGMENT32  1
  grub_uint32_t cmd;
  grub_uint32_t cmdsize;
  grub_uint8_t segname[16];
  grub_uint32_t vmaddr;
  grub_uint32_t vmsize;
  grub_uint32_t fileoff;
  grub_uint32_t filesize;
  grub_macho_vmprot_t maxprot;
  grub_macho_vmprot_t initprot;
  grub_uint32_t nsects;
  grub_uint32_t flags;
} GRUB_PACKED;

/* 64-bit segment command. */
struct grub_macho_segment64
{
#define GRUB_MACHO_CMD_SEGMENT64  0x19
  grub_uint32_t cmd;
  grub_uint32_t cmdsize;
  grub_uint8_t segname[16];
  grub_uint64_t vmaddr;
  grub_uint64_t vmsize;
  grub_uint64_t fileoff;
  grub_uint64_t filesize;
  grub_macho_vmprot_t maxprot;
  grub_macho_vmprot_t initprot;
  grub_uint32_t nsects;
  grub_uint32_t flags;
} GRUB_PACKED;

#define GRUB_MACHO_CMD_THREAD     5

struct grub_macho_lzss_header
{
  char magic[8];
#define GRUB_MACHO_LZSS_MAGIC "complzss"
  grub_uint32_t unused;
  grub_uint32_t uncompressed_size;
  grub_uint32_t compressed_size;
};

/* Convenience union. What do we need to load to identify the file type. */
union grub_macho_filestart
{
  struct grub_macho_fat_header fat;
  struct grub_macho_header32 thin32;
  struct grub_macho_header64 thin64;
  struct grub_macho_lzss_header lzss;
} GRUB_PACKED;

struct grub_macho_thread32
{
  grub_uint32_t cmd;
  grub_uint32_t cmdsize;
  grub_uint8_t unknown1[48];
  grub_uint32_t entry_point;
  grub_uint8_t unknown2[20];
} GRUB_PACKED;

struct grub_macho_thread64
{
  grub_uint32_t cmd;
  grub_uint32_t cmdsize;
  grub_uint8_t unknown1[0x88];
  grub_uint64_t entry_point;
  grub_uint8_t unknown2[0x20];
} GRUB_PACKED;

#define GRUB_MACHO_LZSS_OFFSET 0x180


grub_size_t
grub_decompress_lzss (grub_uint8_t *dst, grub_uint8_t *dstend,
		      grub_uint8_t *src, grub_uint8_t *srcend);

#define VDISK_MEDIA_ID 0x1
#define DISK_TYPE_CD 0
#define DISK_TYPE_HD 1
#define DISK_TYPE_FD 2


//struct grub_efi_block_io_media  //块输入输出介质
//{																																														My_Boot_ISO_xg.iso
//  unsigned int media_id;           //媒体标识    1						1f							1f						1		
//  char removable_media;   					//可移动媒体  0   				0								0							1					
//  char media_present;     					//媒体目前    1						1								1							1	
//  char logical_partition; 					//逻辑分区    0						0								0							0
//  char read_only;         					//只读        0						0								0							1
//  char write_caching;     					//写缓存      0						0								0							0
//  unsigned char pad[3];             //填充
//  unsigned int block_size;         //块尺寸      200					200							200						800
//  unsigned int io_align;           //IO对齐      4						4								0							0	
//  unsigned char pad2[4];            //填充
//  unsigned long long last_block;    //最后块      1fffff(1G)	3a38602f(465G)	1e45ff(968M)	11ad
//};
//typedef struct grub_efi_block_io_media grub_efi_block_io_media_t;	//0x20(按指定)

typedef struct
{
	grub_efi_handle_t from_handle;
	grub_efi_device_path_t *dp;
	block_io_protocol_t block_io;
	grub_efi_block_io_media_t media;
} grub_efivdisk_t;

struct efidisk_data
{
  grub_efi_handle_t device_handle;
  grub_efi_device_path_t *device_path;
  grub_efi_device_path_t *last_device_path;
  grub_efi_block_io_t *block_io;
//  unsigned char partition_number;
  unsigned long long partition_start;
  unsigned long long partition_size;
//  unsigned char partition_signature[16];
  struct efidisk_data *next;
};
					
struct grub_disk_data  //efi磁盘数据	(软盘,硬盘,光盘)    注意外部命令兼容性
{
  grub_efi_handle_t device_handle;          //句柄          11cba410		hndl
//  grub_efi_device_path_t *device_path;      //设备路径      11cba890		类型,子类型,长度
//  grub_efi_device_path_t *last_device_path; //最后设备路径  11cba8a2		类型,子类型,长度
  grub_efi_block_io_t *block_io;          	//块输入输出    1280d318		修订,媒体,重置,读块,写块,清除块
  struct grub_disk_data *next;           		//下一个
  unsigned char drive;                      //from驱动器					f0
  unsigned char to_drive;                   //to驱动器                  原生磁盘为0
  unsigned char from_log2_sector;           //from每扇区字节2的幂	0b
  unsigned char to_log2_sector;             //to每扇区字节2的幂         原生磁盘为0
  unsigned long long start_sector;          //起始扇区                  原生磁盘为0  from在to的起始扇区  每扇区字节=(1 << to_log2_sector)
  unsigned long long sector_count;          //扇区计数                  原生磁盘为0  from在to的扇区数    每扇区字节=(1 << to_log2_sector)
  unsigned long long total_sectors;         //总扇区数                  from驱动器的总扇区数  每扇区字节=(1 << from_log2_sector)
  unsigned char disk_signature[16];         //磁盘签名                  软盘/光盘或略  启动wim/vhd需要  mbr类型同分区签名,gpt类型则异样  原生磁盘为0
  unsigned short to_block_size;             //to块尺寸                  原生磁盘为0
  unsigned char partmap_type;               //硬盘分区类型    1/2=MBR/GPT
  unsigned char fragment;                   //碎片
  unsigned char read_only;                  //只读
  unsigned char disk_type;                  //磁盘类型        0/1/2=光盘/硬盘/软盘
  unsigned char vhd_disk;										//vhd磁盘					位0-1,仿真类型：1=不加载到内存    2=加载到内存
  unsigned char fill;                      	//填充
  grub_efivdisk_t *vdisk;                   //虚拟磁盘指针
}  __attribute__ ((packed));

struct grub_part_data  //efi分区数据	(硬盘)    注意外部命令兼容性
{
	struct grub_part_data *next;  				  //下一个
	unsigned char	drive;									  //驱动器
	unsigned char	partition_type;					  //MBR分区ID         EE是gpt分区类型     光盘:
	unsigned char	partition_activity_flag;  //MBR分区活动标志   80活动              光盘:
	unsigned char partition_entry;				  //分区入口                              光盘: 启动目录确认入口   
	unsigned int partition_ext_offset;		  //扩展分区偏移                          光盘: 启动目录扇区地址
	unsigned int partition;							    //当前分区                              光盘: ffff
	unsigned long long partition_offset;	  //分区偏移
	unsigned long long partition_start;		  //分区起始扇区                          光盘: 引导镜像是硬盘时，分区起始扇区
	unsigned long long partition_size;			//分区扇区尺寸                          光盘: 引导镜像是硬盘时，分区扇区尺寸
	unsigned char partition_signature[16];  //分区签名                              光盘: 
	unsigned int boot_start;                //                                      光盘: 引导镜像在光盘的起始扇区(1扇区=2048字节)
	unsigned int boot_size;                 //                                      光盘: 引导镜像的扇区数(1扇区=512字节)
	grub_efi_handle_t part_handle;          //句柄
  unsigned char partition_number;         //入口号           未使用
	unsigned char partition_boot;           //启动分区         /efi/boot/bootx64.efi文件所在分区
} __attribute__ ((packed));

extern struct grub_part_data *get_partition_info (int drive, int partition);
extern struct grub_part_data *partition_info;
extern struct grub_disk_data *previous_struct;
extern struct grub_part_data *get_boot_partition (int drive);
extern void renew_part_data (void);

struct drive_map_slot
{
	/* Remember to update DRIVE_MAP_SLOT_SIZE once this is modified.
	 * The struct size must be a multiple of 4.
	 */
	unsigned char from_drive;
	unsigned char to_drive;						/* 0xFF indicates a memdrive */
	unsigned char max_head;
  
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
	unsigned char to_sector;
	unsigned long long start_sector;
	unsigned long long sector_count;
} __attribute__ ((packed));

struct fragment_map_slot
{
	unsigned short slot_len;
	unsigned char from;
	unsigned char to;
	unsigned long long fragment_data[0];
} __attribute__ ((packed));

struct fragment
{
	unsigned long long start_sector;
	unsigned long long sector_count;
};

//extern struct drive_map_slot	vpart_drive_map[DRIVE_MAP_SIZE + 1];
//extern struct drive_map_slot	disk_drive_map[DRIVE_MAP_SIZE + 1];
extern struct fragment_map_slot	disk_fragment_map;
//extern char disk_buffer[0x1000];
extern char *disk_buffer;
//extern int drive_map_slot_empty (struct drive_map_slot item);
extern struct fragment_map_slot *fragment_map_slot_find(struct fragment_map_slot *q, unsigned int from);
extern int grub_SectorSequence_readwrite (int drive, struct fragment *data, unsigned char from_log2_sector, unsigned char to_log2_sector,
			grub_disk_addr_t sector, grub_size_t size, char *buf, int read_write);


#define CDVOL_TYPE_STANDARD 0x0
#define CDVOL_TYPE_CODED    0x1
#define CDVOL_TYPE_END      0xFF

#define CDVOL_ID  "CD001"
#define CDVOL_ELTORITO_ID "EL TORITO SPECIFICATION"

// ELTORITO_CATALOG.Boot.MediaTypes
#define ELTORITO_NO_EMULATION 0x00
#define ELTORITO_12_DISKETTE  0x01
#define ELTORITO_14_DISKETTE  0x02
#define ELTORITO_28_DISKETTE  0x03
#define ELTORITO_HARD_DISK    0x04

//Indicator types
#define ELTORITO_ID_CATALOG               0x01
#define ELTORITO_ID_SECTION_BOOTABLE      0x88
#define ELTORITO_ID_SECTION_NOT_BOOTABLE  0x00
#define ELTORITO_ID_SECTION_HEADER        0x90
#define ELTORITO_ID_SECTION_HEADER_FINAL  0x91

typedef union
{
  struct
  {
    grub_uint8_t type;
    grub_uint8_t id[5]; ///< "CD001"
    grub_uint8_t reserved[82];
  } unknown;
  struct
  {
    grub_uint8_t type;          ///< Must be 0
    grub_uint8_t id[5];         ///< "CD001"
    grub_uint8_t version;       ///< Must be 1
    grub_uint8_t system_id[32]; ///< "EL TORITO SPECIFICATION"
    grub_uint8_t unused[32];    ///< Must be 0
    grub_uint8_t elt_catalog[4];///< Absolute pointer to first sector of Boot Catalog
    grub_uint8_t unused2[13];   ///< Must be 0
  } boot_record_volume;
  struct
  {
    grub_uint8_t  type;
    grub_uint8_t  id[5];             ///< "CD001"
    grub_uint8_t  version;
    grub_uint8_t  unused;            ///< Must be 0
    grub_uint8_t  system_id[32];
    grub_uint8_t  volume_id[32];
    grub_uint8_t  unused2[8];        ///< Must be 0
    grub_uint32_t vol_space_size[2]; ///< the number of Logical Blocks
  } primary_volume;
} cdrom_volume_descriptor_t;

typedef union
{
  struct
  {
    grub_uint8_t reserved[0x30];
  } unknown;
  /// Catalog validation entry (Catalog header)
  struct
  {
    grub_uint8_t  indicator;     ///< Must be 01
    grub_uint8_t  platform_id;
    grub_uint16_t reserved;
    grub_uint8_t  manufac_id[24];
    grub_uint16_t checksum;
    grub_uint16_t id55AA;
    grub_uint8_t reserved1[0x10];
  } catalog;
  /// Initial/Default Entry or Section Entry
  struct
  {
    grub_uint8_t reserved[0x20];
    grub_uint8_t  indicator;     ///< 88 = Bootable, 00 = Not Bootable
    grub_uint8_t  media_type : 4; //取4位,即半字节
    grub_uint8_t  reserved1 : 4; ///< Must be 0
    grub_uint16_t load_segment;
    grub_uint8_t  system_type;
    grub_uint8_t  reserved2;     ///< Must be 0
    grub_uint16_t sector_count;
    grub_uint32_t lba;
  } boot;
  /// Section Header Entry
  struct
  {
    grub_uint8_t  indicator; ///< 90 - Header, more header follw, 91 - Final Header
    grub_uint8_t  platform_id;
    grub_uint16_t section_entries;///< Number of section entries following this header
    grub_uint8_t  id[28];
    grub_uint8_t reserved[0x10];
  } section;
} eltorito_catalog1_t;	//0x20


struct boot
{
  grub_uint8_t  indicator1;       //入口号: 必须从1开始，0x91结束 
  grub_uint8_t  platform_id;      //平台类型: 0=Intel平台; 1=Power PC; 2=Mac; 0xEF=UEFI。
  grub_uint16_t section_entries;
  grub_uint8_t  manufac_id[24];
  grub_uint16_t checksum;
  grub_uint16_t id55AA;
  grub_uint8_t  indicator88;      //标志信息: 88=可引导，00=不可引导
  grub_uint8_t  media_type : 4;   //取4位,即半字节
  grub_uint8_t  reserved1 : 4;    //必须为0
  grub_uint16_t load_segment;
  grub_uint8_t  system_type;
  grub_uint8_t  reserved2;        //必须为0
  grub_uint16_t sector_count;
  grub_uint32_t lba;
  grub_uint8_t reserved[0x14];
} GRUB_PACKED;	//0x40
typedef struct boot eltorito_catalog0_t;


struct grub_packed_guid
{
  grub_uint32_t data1;
  grub_uint16_t data2;
  grub_uint16_t data3;
  grub_uint8_t data4[8];
} GRUB_PACKED;
typedef struct grub_packed_guid grub_packed_guid_t;

extern grub_packed_guid_t VDISK_GUID;
extern grub_efi_uint32_t cd_boot_entry;
extern grub_efi_uint16_t cd_boot_start;
extern grub_efi_uint32_t cd_boot_size;
extern grub_efi_uint32_t cd_Image_part_start;
extern grub_efi_uint32_t cd_Image_disk_size;
extern grub_efi_uint64_t	part_addr;
extern grub_efi_uint64_t	part_size;
extern struct grub_part_data *part_data;
void file_read (grub_efi_boolean_t disk, void *file,
                void *buf, grub_efi_uintn_t len, grub_efi_uint64_t offset);
grub_efi_uint64_t get_size (grub_efi_boolean_t disk, void *file);

/* vboot */
extern grub_efi_handle_t grub_load_image (unsigned int drive, const char *filename, void *boot_image, unsigned long long file_len, grub_efi_handle_t *devhandle);
/* vdisk */
extern grub_efi_status_t vdisk_install  (int drive, int partition);
/* vpart */
extern grub_efi_status_t vpart_install (int drive, struct grub_part_data *part);

struct grub_efi_component_name2_protocol
{
  grub_efi_status_t (*get_driver_name)
          (struct grub_efi_component_name2_protocol *this,
           grub_efi_char8_t *language,
           grub_efi_char16_t **driver_name);
  grub_efi_status_t (*get_controller_name)
          (struct grub_efi_component_name2_protocol *this,
           grub_efi_handle_t controller_handle,
           grub_efi_handle_t child_handle,
           grub_efi_char8_t *language,
           grub_efi_char16_t **controller_name);
  grub_efi_char8_t *supported_languages;
};
typedef struct grub_efi_component_name2_protocol grub_efi_component_name2_protocol_t;

#define GRUB_EFI_FILE_PROTOCOL_REVISION  0x00010000
#define GRUB_EFI_FILE_PROTOCOL_REVISION2 0x00020000
#define GRUB_EFI_FILE_PROTOCOL_LATEST_REVISION GRUB_EFI_FILE_PROTOCOL_REVISION2

#define GRUB_EFI_FILE_REVISION GRUB_EFI_FILE_PROTOCOL_REVISION

typedef struct
{
  grub_efi_event_t event;
  grub_efi_status_t status;
  grub_efi_uintn_t buffer_size;
  void *buffer;
} grub_efi_file_io_token_t;

// Open modes
#define GRUB_EFI_FILE_MODE_READ   0x0000000000000001ULL
#define GRUB_EFI_FILE_MODE_WRITE  0x0000000000000002ULL
#define GRUB_EFI_FILE_MODE_CREATE 0x8000000000000000ULL

// File attributes
#define GRUB_EFI_FILE_READ_ONLY  0x0000000000000001ULL
#define GRUB_EFI_FILE_HIDDEN     0x0000000000000002ULL
#define GRUB_EFI_FILE_SYSTEM     0x0000000000000004ULL
#define GRUB_EFI_FILE_RESERVED   0x0000000000000008ULL
#define GRUB_EFI_FILE_DIRECTORY  0x0000000000000010ULL
#define GRUB_EFI_FILE_ARCHIVE    0x0000000000000020ULL
#define GRUB_EFI_FILE_VALID_ATTR 0x0000000000000037ULL

struct grub_efi_file_protocol
{
  grub_efi_uint64_t revision;
  grub_efi_status_t (*file_open) (struct grub_efi_file_protocol *this,
                                  struct grub_efi_file_protocol **new_handle,
                                  grub_efi_char16_t *file_name,
                                  grub_efi_uint64_t open_mode,
                                  grub_efi_uint64_t attributes);
  grub_efi_status_t (*file_close) (struct grub_efi_file_protocol *this);
  grub_efi_status_t (*file_delete) (struct grub_efi_file_protocol *this);
  grub_efi_status_t (*file_read) (struct grub_efi_file_protocol *this,
                                  grub_efi_uintn_t *buffer_size,
                                  void *buffer);
  grub_efi_status_t (*file_write) (struct grub_efi_file_protocol *this,
                                   grub_efi_uintn_t *buffer_size,
                                   void *buffer);
  grub_efi_status_t (*get_pos) (struct grub_efi_file_protocol *this,
                                grub_efi_uint64_t *pos);
  grub_efi_status_t (*set_pos) (struct grub_efi_file_protocol *this,
                                grub_efi_uint64_t pos);
  grub_efi_status_t (*get_info) (struct grub_efi_file_protocol *this,
                                 grub_efi_guid_t *information_type,
                                 grub_efi_uintn_t *buffer_size,
                                 void *buffer);
  grub_efi_status_t (*set_info) (struct grub_efi_file_protocol *this,
                                 grub_efi_guid_t *information_type,
                                 grub_efi_uintn_t buffer_size,
                                 void *buffer);
  grub_efi_status_t (*flush) (struct grub_efi_file_protocol *this);
  grub_efi_status_t (*open_ex) (struct grub_efi_file_protocol *this,
                                struct grub_efi_file_protocol **new_handle,
                                grub_efi_char16_t *file_name,
                                grub_efi_uint64_t open_mode,
                                grub_efi_uint64_t attributes,
                                grub_efi_file_io_token_t *token);
  grub_efi_status_t (*read_ex) (struct grub_efi_file_protocol *this,
                                grub_efi_file_io_token_t *token);
  grub_efi_status_t (*write_ex) (struct grub_efi_file_protocol *this,
                                 grub_efi_file_io_token_t *token);
  grub_efi_status_t (*flush_ex) (struct grub_efi_file_protocol *this,
                                 grub_efi_file_io_token_t *token);
};
typedef struct grub_efi_file_protocol grub_efi_file_protocol_t;
typedef grub_efi_file_protocol_t *grub_efi_file_handle_t;
typedef grub_efi_file_protocol_t  grub_efi_file_t;

#define GRUB_EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION 0x00010000
#define GRUB_EFI_FILE_IO_REVISION  GRUB_EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION
#define EFI_REMOVABLE_MEDIA_FILE_NAME_IA32    "/EFI/BOOT/BOOTIA32.EFI"
#define EFI_REMOVABLE_MEDIA_FILE_NAME_X64     "/EFI/BOOT/BOOTX64.EFI"
#define EFI_REMOVABLE_MEDIA_FILE_NAME_ARM     "/EFI/BOOT/BOOTARM.EFI"
#define EFI_REMOVABLE_MEDIA_FILE_NAME_AARCH64 "/EFI/BOOT/BOOTAA64.EFI"

#if defined(__i386__)
  #define EFI_REMOVABLE_MEDIA_FILE_NAME   EFI_REMOVABLE_MEDIA_FILE_NAME_IA32
#else
  #define EFI_REMOVABLE_MEDIA_FILE_NAME   EFI_REMOVABLE_MEDIA_FILE_NAME_X64
//#elif defined (__arm__)
//  #define EFI_REMOVABLE_MEDIA_FILE_NAME   EFI_REMOVABLE_MEDIA_FILE_NAME_ARM
//#elif defined (__aarch64__)
//  #define EFI_REMOVABLE_MEDIA_FILE_NAME   EFI_REMOVABLE_MEDIA_FILE_NAME_AARCH64
//#else
//  #error Unknown Processor Type
#endif

struct grub_efi_simple_fs_protocol
{
  grub_efi_uint64_t revision;
  grub_efi_status_t (*open_volume) (struct grub_efi_simple_fs_protocol *this,
                                    grub_efi_file_protocol_t **root);
};
typedef struct grub_efi_simple_fs_protocol grub_efi_simple_fs_protocol_t;

#define GRUB_EFI_FILE_INFO_GUID \
  { 0x09576e92, 0x6d3f, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define GRUB_EFI_FILE_SYSTEM_INFO_GUID \
  { 0x09576e93, 0x6d3f, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define GRUB_EFI_FILE_SYSTEM_VOLUME_LABEL_GUID \
  { 0xdb47d7d3, 0xfe81, 0x11d3, \
    { 0x9a, 0x35, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define GRUB_GPT_HEADER_MAGIC \
  { 0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54 }

#define GRUB_GPT_HEADER_VERSION	\
  grub_cpu_to_le32_compile_time (0x00010000U)
	
#define GRUB_GPT_PARTITION_TYPE_EFI_SYSTEM \
  { 0xc12a7328, 0xf81f, 0x11d2, { 0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b } }

typedef struct grub_efi_file_info
{
  /* size of file_info structure */
  grub_efi_uint64_t size;
  grub_efi_uint64_t file_size;
  grub_efi_uint64_t physical_size;
  grub_efi_time_t create_time;
  grub_efi_time_t last_access_time;
  grub_efi_time_t modification_time;
  grub_efi_uint64_t attribute;
  grub_efi_char16_t file_name[1];
} grub_efi_file_info_t;

typedef struct grub_efi_file_system_info
{
  grub_efi_uint64_t size;
  grub_efi_boolean_t read_only;
  grub_efi_uint64_t volume_size;
  grub_efi_uint64_t free_space;
  grub_efi_uint32_t block_size;
  grub_efi_char16_t volume_label[1];
} grub_efi_fs_info_t;

typedef struct grub_efi_file_system_volume_label
{
  grub_efi_char16_t volume_label[1];
} grub_efi_fs_label_t;

typedef __WCHAR_TYPE__ wchar_t;
/* 宽字符，gcc在linux下，使用4个字节存储一个字符。
 * 比如 wchar_t p[] = L"1122"，它存储为：31 00 00 00 31 00 00 00 32 00 00 00 32 00 00 00
 * 而普通字符，使用1个字节存储一个字符。
 * 比如 char p2[] = "1122"，它存储为：31 31 32 32
 */

extern void grub_efidisk_fini (void);
extern void grub_efidisk_init (void);
extern void enumerate_disks (void);
extern grub_efi_device_path_protocol_t*
										grub_efi_create_device_node (grub_efi_uint8_t node_type, grub_efi_uintn_t node_subtype,grub_efi_uint16_t node_length);
extern grub_efi_device_path_protocol_t*
										grub_efi_append_device_node (const grub_efi_device_path_protocol_t *device_path,
                    const grub_efi_device_path_protocol_t *device_node);
extern grub_efi_boolean_t guidcmp (const grub_packed_guid_t *g1, const grub_packed_guid_t *g2);
extern grub_packed_guid_t * guidcpy (grub_packed_guid_t *dst, const grub_packed_guid_t *src);

extern char * grub_strchr (const char *s, int c);
extern char * grub_strrchr (const char *s, int c);
extern int gopprobe (char *arg, int flags);

//////////////////////////////////////////////////////////////////////////////////
/* Based on UEFI specification.  基于UEFI规范 */
/*
GOP （图形输出协议）
GOP (Graphic Output Protocol)，是用来将图形驱动程序延伸至UEFI固件的接口，借以取代传统VBIOS（视讯BIOS）在开机资源要求等初始化行为。
GOP： 无 64 KB 的限制。32 位元保护模式。不需要 CSM。速度最佳化 (快速开机支持)。
*/

#define GRUB_EFI_GOP_GUID \
  { 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a }}

typedef enum
  {
//像素是32位，字节0代表红色，字节1代表绿色，字节2代表蓝色，字节3保留。这是物理帧缓冲区的定义。红色、绿色和蓝色组件的字节值表示颜色强度。此颜色强度值的范围从最小强度0到最大强度255。
    GRUB_EFI_GOT_RGBA8,			//红绿兰A 8位
//像素是32位，字节0代表蓝色，字节1代表绿色，字节2代表红色，字节3保留。这是物理帧缓冲区的定义。红色、绿色和蓝色组件的字节值表示颜色强度。此颜色强度值的范围从最小强度0到最大强度255。 
    GRUB_EFI_GOT_BGRA8,			//兰绿红A 8位		通常使用  最低位兰,最高位A
//物理帧缓冲区的像素定义由EFI像素位掩码定义。
    GRUB_EFI_GOT_BITMASK,		//位掩模
//此模式不支持物理帧缓冲区。
		PixelBltOnly,
//有效的EFI图形像素格式枚举值小于此值。
		PixelFormatMax
  }
  grub_efi_gop_pixel_format_t;	//gop像素格式

//EFI_GRAPHICS_OUTPUT_BLT_PIXEL
struct grub_efi_gop_blt_pixel	//gop_blt像素
{
  grub_uint8_t blue;			//兰
  grub_uint8_t green;			//绿
  grub_uint8_t red;				//红
  grub_uint8_t reserved;	//保留
};

//如果在RedMask、GreenMask或BlueMask中设置了一个位，那么这些像素位表示相应的颜色。
//RedMask、GreenMask、BlueMask和ReserverdMask中的位不能超过圈位位置。位掩码中红色、绿色和蓝色组件的值表示颜色强度。
//颜色强度必须随着每个颜色遮罩的颜色值的增加而增加，颜色遮罩中所有位的最小强度清除为颜色遮罩集中所有位的最大强度。
//EFI_PIXEL_BITMASK
struct grub_efi_gop_pixel_bitmask	//gop像素位掩模
{
  grub_uint32_t r;	//红
  grub_uint32_t g;	//绿
  grub_uint32_t b;	//兰
  grub_uint32_t a;	//保留
};

//EFI_GRAPHICS_OUTPUT_MODE_INFORMATION
struct grub_efi_gop_mode_info	//gop模式信息
{
  grub_efi_uint32_t version;												//版本	值为零表示本规范中定义的EFI图形输出模式信息结构。本规范的未来版本可能会以向后兼容的方式扩展此数据结构，并增加版本的值。 
  grub_efi_uint32_t width;													//宽		视频屏幕的大小，以X维像素为单位。
  grub_efi_uint32_t height;													//高		视频屏幕的大小（以Y维像素为单位）。 
  grub_efi_gop_pixel_format_t pixel_format;					//像素格式 		定义像素物理格式的枚举。PixelBltOnly意味着线性帧缓冲区不可用于此模式。
  struct grub_efi_gop_pixel_bitmask pixel_bitmask;	//像素位掩模 	此位掩码仅在PixelFormat设置为PixelPixelBitMask时有效。正在设置的位定义了用于什么目的的位，如红色、绿色、蓝色或保留位。
  grub_efi_uint32_t pixels_per_scanline;						//每扫描行像素数 	定义每个视频内存行的像素元素数。出于性能原因，或者由于硬件限制，扫描线可能会被填充到一定的内存对齐量。
				//这些填充像素元素位于水平分辨率覆盖的区域之外，不可见。对于直接帧缓冲区访问，此数字用作视频内存中像素线开始之间的间隔.基于单个像素元素和像素扫描线的大小，
				//视频内存中从像素元素（x，y）到像素元素（x，y+1）的偏移量必须计算为“sizeof（pixellement）*PixelsPerScanLine”，
				//不是“sizeof（pixellement）*HorizontalResolution”，尽管在许多情况下这些值可以重合。 
				//此值取决于视频硬件和模式分辨率。GOP实现负责为该字段提供准确的值。
};

//是只读的，只有使用适当的接口函数才能更改值：
struct grub_efi_gop_mode	//gop模式
{
  grub_efi_uint32_t max_mode;						//最大模式	QueryMode（）和SetMode（）支持的模式数。
  grub_efi_uint32_t mode;								//模式			图形设备的当前模式。有效的模式编号是0到MaxMode-1。 
  struct grub_efi_gop_mode_info *info;	//模式信息	指向只读EFI_GRAPHICS_OUTPUT_MODE_INFORMATION数据的指针。
  grub_efi_uintn_t info_size;						//信息尺寸	信息结构的大小（字节）。本规范的未来版本可能会增加EFI_GRAPHICS_OUTPUT_MODE_INFORMATION 数据的大小。
  grub_efi_physical_address_t fb_base;	//物理地址	图形线性帧缓冲区的基址。信息包含允许软件不使用Blt()直接绘制到帧缓冲区所需的信息。FrameBufferBase中的偏移量0表示显示的左上角像素。
  grub_efi_uintn_t fb_size;							//缓存尺寸	支持由PixelsPerScanLine x VerticalResolution x PixelElementSize定义的活动模式所需的帧缓冲区量。
};

/* Forward declaration.  前向声明*/
struct grub_efi_gop;

typedef grub_efi_status_t
(*grub_efi_gop_query_mode_t) (struct grub_efi_gop *this,
			      grub_efi_uint32_t mode_number,
			      grub_efi_uintn_t *size_of_info,
			      struct grub_efi_gop_mode_info **info);	//gop查询模式(本身,模式号,返回信息尺寸,返回信息)

typedef grub_efi_status_t
(*grub_efi_gop_set_mode_t) (struct grub_efi_gop *this,
			    grub_efi_uint32_t mode_number);						//gop设置模式(本身,模式号)

//说明: 函数的作用是：将BltBuffer矩形绘制到视频屏幕上。
//BltBuffer表示将使用BltOperation指定的操作在图形屏幕上绘制的高宽像素矩形。Delta值可用于在BltBuffer的子矩形上执行BltOperation。
typedef grub_efi_status_t
(*grub_efi_gop_blt_t) (struct grub_efi_gop *this,	//本身
		       void *buffer,								//缓存	要传输到图形屏幕的数据。尺寸至少为Width*Height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL).
		       grub_efi_uintn_t operation,	//操作	将BltBuffer复制到图形屏幕时要执行的操作。
		       grub_efi_uintn_t sx,					//源x		用于BltOperation的源的X坐标。屏幕的原点是0，0，这是屏幕的左上角。
		       grub_efi_uintn_t sy,					//源y		用于BltOperation的源的Y坐标。屏幕的原点是0，0，这是屏幕的左上角。
		       grub_efi_uintn_t dx,					//目的x	BltOperation目标的X坐标。屏幕的原点是0，0，这是屏幕的左上角。
		       grub_efi_uintn_t dy,					//目的y	BltOperation目标的Y坐标。屏幕的原点是0，0，这是屏幕的左上角。
		       grub_efi_uintn_t width,			//宽		blt矩形中矩形的宽度(像素)。每个像素由EFI_GRAPHICS_OUTPUT_BLT_PIXEL元素表示。
		       grub_efi_uintn_t height,			//高		blt矩形中矩形的高度(像素)。 
		       grub_efi_uintn_t delta);			//增量	不用于EfiBltVideoFill或EfiBltVideoToVideo操作。如果使用0的增量，则整个BltBuffer正在运行。
																				//如果正在使用BltBuffer的子矩形，则Delta表示BltBuffer行中的字节数。

//表113描述了矩形上支持的bltoperation。矩形有坐标（左，上）（右，下）： 
typedef enum
  {
//将数据从BltBuffer像素(0,0)直接写入视频显示矩形(DestinationX，DestinationY)(DestinationX+宽度，DestinationY+高度)。只有一个像素将使用从BltBuffer。不使用增量。
    GRUB_EFI_BLT_VIDEO_FILL,					//BLT视频填充 
//从视频显示矩形(SourceX，SourceY)(SourceX+Width，SourceY+Height)读取数据，并将其放置在BltBuffer矩形(DestinationX，DestinationY)(DestinationX+Width，DestinationY+Height)中。
//如果DestinationX或DestinationY不为零，则Delta必须设置为BltBuffer中一行的长度(字节)。 
    GRUB_EFI_BLT_VIDEO_TO_BLT_BUFFER,	//BLT视频到BLT缓冲区
//将数据从BltBuffer矩形(SourceX，SourceY)(SourceX+Width，SourceY+Height)直接写入视频显示矩形(DestinationX，DestinationY)(DestinationX+Width，DestinationY+Height)。 
//如果SourceX或SourceY不为零，则Delta必须设置为BltBuffer中一行的长度(以字节为单位)。 
    GRUB_EFI_BLT_BUFFER_TO_VIDEO,			//BLT缓冲区到视频
//从视频显示矩形(SourceX，SourceY)(SourceX+Width，SourceY+Height)复制到视频显示矩形(DestinationX，DestinationY)(DestinationX+Width，DestinationY+Height)。
//此模式中不使用BltBuffer和Delta。源矩形和目标矩形的重叠没有限制。
    GRUB_EFI_BLT_VIDEO_TO_VIDEO,			//BLT视频到视频
    GRUB_EFI_BLT_OPERATION_MAX				//BLT最大操作
  }
  grub_efi_gop_blt_operation_t;	//gop blt操作

//EFI_GRAPHICS_OUTPUT_PROTOCOL
struct grub_efi_gop
{
  grub_efi_gop_query_mode_t query_mode;		//gop查询模式		返回图形设备和一组活动视频输出设备支持的可用图形模式的信息 
  grub_efi_gop_set_mode_t set_mode;				//gop设置模式		将视频设备设置为指定模式，并将输出显示的可见部分清除为黑色。 
//UEFI标准中的Blt 函数不仅可以将内容传送至帧缓冲区(EfiBltBufferToVideo)，还可以保存帧缓冲区中的内容(EfiBltVideoToBltBuffer)，
//或是将帧缓冲区中一个位置的内容移动至另一位置(EfiBltVideoToVideo)。
  grub_efi_gop_blt_t blt;									//gop_blt				基于视频设备帧缓冲区的软件抽象 
  struct grub_efi_gop_mode *mode;					//gop模式				指向EFI图形输出协议模式数据的指针。
};

//说明 
//EFI_GRAPHICS_OUTPUT_协议提供了一个软件抽象，允许将像素直接绘制到帧缓冲区。EFI_-GRAPHICS_-OUTPUT_协议设计为轻量级，支持在操作系统引导之前进行图形输出的基本需求。 



/*
注意: 以下代码示例是预期字段用法的示例：
INTN
GetPixelElementSize (IN EFI_PIXEL_BITMASK *PixelBits)	//获得像素元素尺寸(像素位掩模)
{
	INTN HighestPixel = -1;	//最高像素
	INTN BluePixel;					//兰像素
	INTN RedPixel;					//红像素
	INTN GreenPixel;				//绿像素
	INTN RsvdPixel;					//保留像素
	BluePixel = FindHighestSetBit (PixelBits->BlueMask);		//查找最高点设置位(像素位掩模->兰掩模)  ff
	RedPixel = FindHighestSetBit (PixelBits->RedMask);			//ff0000
	GreenPixel = FindHighestSetBit (PixelBits->GreenMask);	//ff00
	RsvdPixel = FindHighestSetBit (PixelBits->ReservedMask);//0
	HighestPixel = max (BluePixel, RedPixel);								//ff0000
	HighestPixel = max (HighestPixel, GreenPixel);					//ff0000
	HighestPixel = max (HighestPixel, RsvdPixel);						//ff0000
	return HighestPixel;
}
EFI_PHYSICAL_ADDRESS NewPixelAddress;							物理地址   				新像素地址
EFI_PHYSICAL_ADDRESS CurrentPixelAddress;					物理地址 					当前像素地址
EFI_GRAPHICS_OUTPUT_MODE_INFORMATION OutputInfo;	图形输出模式信息	输出信息
INTN PixelElementSize;
switch (OutputInfo.PixelFormat)	//输出信息.像素格式
{
	case PixelBitMask:	//位掩模
		PixelElementSize = GetPixelElementSize (&OutputInfo.PixelInformation);	//输出信息.像素格式
		break;
	case PixelBlueGreenRedReserved8BitPerColor:	//兰绿红A
	case PixelRedGreenBlueReserved8BitPerColor:	//红绿兰A
		PixelElementSize = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
		break;
}
//
// NewPixelAddress after execution points to the pixel		执行后的新像素地址指向当前像素地址所指像素下一行的像素 
// positioned one line below the one pointed by
// CurrentPixelAddress
//
NewPixelAddress = CurrentPixelAddress + (PixelElementSize * OutputInfo.PixelsPerScanLine);
注释结束代码示例。
*/
////////////////////////////////////////////////////////////////////////////////
typedef enum grub_net_card_flags
  {
    GRUB_NET_CARD_HWADDRESS_IMMUTABLE = 1,
    GRUB_NET_CARD_NO_MANUAL_INTERFACES = 2
  } grub_net_card_flags_t;

typedef enum grub_link_level_protocol_id 
{
  GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET	//NET链接级别协议以太网
} grub_link_level_protocol_id_t;

typedef struct grub_net_link_level_address
{
  grub_link_level_protocol_id_t type;	//类型
  union
  {
    unsigned char mac[6];							//mac
  };
} grub_net_link_level_address_t;			//网络链路级地址


struct grub_net_slaac_mac_list
{
  struct grub_net_slaac_mac_list *next;
  struct grub_net_slaac_mac_list **prev;
  grub_net_link_level_address_t address;
  int slaac_counter;
  char *name;
};

struct grub_net_buff
{
  /* Pointer to the start of the buffer.  */
  grub_uint8_t *head;
  /* Pointer to the data.  */
  grub_uint8_t *data;
  /* Pointer to the tail.  */
  grub_uint8_t *tail;
  /* Pointer to the end of the buffer.  */
  grub_uint8_t *end;
};

struct grub_net_card_driver
{
  struct grub_net_card_driver *next;
  struct grub_net_card_driver **prev;
  const char *name;
  grub_err_t (*open) (struct grub_net_card *dev);
  void (*close) (struct grub_net_card *dev);
  grub_err_t (*send) (struct grub_net_card *dev,
		      struct grub_net_buff *buf);
  struct grub_net_buff * (*recv) (struct grub_net_card *dev);
};

struct grub_net_card	//网卡
{
  struct grub_net_card *next;		//下一个
  struct grub_net_card **prev;	//上一个
  const char *name;							//名称
  struct grub_net_card_driver *driver;						//驱动器
  grub_net_link_level_address_t default_address;	//网络链路级地址 
  grub_net_card_flags_t flags;	//标记
  int num_ifaces;								//网络数
  int opened;										//已打开
  unsigned idle_poll_delay_ms;	//空闲轮询延迟ms 
  unsigned long long last_poll;	//最后获得
  grub_size_t mtu;							//最大包尺寸
  struct grub_net_slaac_mac_list *slaac_list;	//网从mac列表 
  grub_ssize_t new_ll_entry;		//新入口
  struct grub_net_link_layer_entry *link_layer_table;	//网络链接层入口
  void *txbuf;									//tx缓存地址
  void *rcvbuf;									//rcv缓存地址
  grub_size_t rcvbufsize;				//rcv缓存尺寸
  grub_size_t txbufsize;				//tx缓存尺寸
  int txbusy;										//tx忙碌
  union
  {
    struct
    {
      struct grub_efi_simple_network *efi_net;	//简单网络 
      grub_efi_handle_t efi_handle;							//句柄
      grub_size_t last_pkt_size;								//最后pkt尺寸 
    };
    void *data;		//数据指针
    int data_num;	//数据号
  };
};

#define GRUB_NET_BOOTP_MAC_ADDR_LEN	16
typedef grub_uint8_t grub_net_bootp_mac_addr_t[GRUB_NET_BOOTP_MAC_ADDR_LEN];

typedef struct grub_net_network_level_address
{
  grub_network_level_protocol_id_t type;
  union
  {
    grub_uint32_t ipv4;
    grub_uint64_t ipv6[2];
  };
  grub_dns_option_t option;
} grub_net_network_level_address_t;

struct grub_net_bootp_packet		//引导播放器
{
  grub_uint8_t opcode;		//操作码												01
  grub_uint8_t hw_type;		//硬件类型											01
  grub_uint8_t hw_len;		//件地址长度										06
  grub_uint8_t gate_hops;	//归零													00
  grub_uint32_t ident;		//客户选择的随机数							59 6b 5d 13 
  grub_uint16_t seconds;	//自初始引导以来的秒数					00 00
  grub_uint16_t flags;		//标记													80 00
  grub_uint32_t	client_ip;	//客户IP											00 00 00 00
  grub_uint32_t your_ip;		//你的IP											00 00 00 00
  grub_uint32_t	server_ip;	//服务器IP										00 00 00 00
  grub_uint32_t	gateway_ip;	//网关IP											00 00 00 00
  grub_net_bootp_mac_addr_t mac_addr;	//客户端硬件地址		00 0c 29 8d cc d9 00 00 - 00 00 00 00 00 00 00 00
  char server_name[64];			//服务器的主机名							0
  char boot_file[128];			//引导文件名									0
  grub_uint8_t vendor[0];		//供应商
} GRUB_PACKED;

struct grub_net_network_level_interface
{
  struct grub_net_network_level_interface *next;
  struct grub_net_network_level_interface **prev;
  char *name;
  struct grub_net_card *card;
  grub_net_network_level_address_t address;
  grub_net_link_level_address_t hwaddress;
  grub_net_interface_flags_t flags;
  struct grub_net_bootp_packet *dhcp_ack;
  grub_size_t dhcp_acklen;
  grub_uint16_t vlantag;
  void *data;
};

//======================================================================================================================

struct grub_efi_service_binding;

typedef grub_efi_status_t
(*grub_efi_service_binding_create_child) (struct grub_efi_service_binding *this,
                                          grub_efi_handle_t *child_handle);
																					
typedef grub_efi_status_t
(*grub_efi_service_binding_destroy_child) (struct grub_efi_service_binding *this,
                                           grub_efi_handle_t *child_handle);

typedef struct grub_efi_service_binding
{
  grub_efi_service_binding_create_child create_child;
  grub_efi_service_binding_destroy_child destroy_child;
} grub_efi_service_binding_t;

typedef struct grub_efi_dhcp4_protocol grub_efi_dhcp4_protocol_t;

enum grub_efi_dhcp4_state {
  GRUB_EFI_DHCP4_STOPPED,
  GRUB_EFI_DHCP4_INIT,
  GRUB_EFI_DHCP4_SELECTING,
  GRUB_EFI_DHCP4_REQUESTING,
  GRUB_EFI_DHCP4_BOUND,
  GRUB_EFI_DHCP4_RENEWING,
  GRUB_EFI_DHCP4_REBINDING,
  GRUB_EFI_DHCP4_INIT_REBOOT,
  GRUB_EFI_DHCP4_REBOOTING
};

typedef enum grub_efi_dhcp4_state grub_efi_dhcp4_state_t;

struct grub_efi_dhcp4_header {
  grub_efi_uint8_t op_code;
  grub_efi_uint8_t hw_type;
  grub_efi_uint8_t hw_addr_len;
  grub_efi_uint8_t hops;
  grub_efi_uint32_t xid;
  grub_efi_uint16_t seconds;
  grub_efi_uint16_t reserved;
  grub_efi_ipv4_address_t client_addr;
  grub_efi_ipv4_address_t your_addr;
  grub_efi_ipv4_address_t server_addr;
  grub_efi_ipv4_address_t gateway_addr;
  grub_efi_uint8_t client_hw_addr[16];
  grub_efi_char8_t server_name[64];
  grub_efi_char8_t boot_file_name[128];
} GRUB_PACKED;

typedef struct grub_efi_dhcp4_header grub_efi_dhcp4_header_t;

struct grub_efi_dhcp4_packet {
  grub_efi_uint32_t size;
  grub_efi_uint32_t length;
  struct {
    grub_efi_dhcp4_header_t header;
    grub_efi_uint32_t magik;
    grub_efi_uint8_t option[1];
  } dhcp4;
} GRUB_PACKED;

typedef struct grub_efi_dhcp4_packet grub_efi_dhcp4_packet_t;

struct grub_efi_dhcp4_listen_point {
  grub_efi_ipv4_address_t listen_address;
  grub_efi_ipv4_address_t subnet_mask;
  grub_efi_uint16_t listen_port;
};

typedef struct grub_efi_dhcp4_listen_point grub_efi_dhcp4_listen_point_t;

struct grub_efi_dhcp4_transmit_receive_token {
  grub_efi_status_t status;
  grub_efi_event_t completion_event;
  grub_efi_ipv4_address_t remote_address;
  grub_efi_uint16_t remote_port;
  grub_efi_ipv4_address_t gateway_address;
  grub_efi_uint32_t listen_point_count;
  grub_efi_dhcp4_listen_point_t *listen_points;
  grub_efi_uint32_t timeout_value;
  grub_efi_dhcp4_packet_t *packet;
  grub_efi_uint32_t response_count;
  grub_efi_dhcp4_packet_t *response_list;
};

typedef struct grub_efi_dhcp4_transmit_receive_token grub_efi_dhcp4_transmit_receive_token_t;

enum grub_efi_dhcp4_event {
  GRUB_EFI_DHCP4_SEND_DISCOVER = 0X01,
  GRUB_EFI_DHCP4_RCVD_OFFER,
  GRUB_EFI_DHCP4_SELECT_OFFER,
  GRUB_EFI_DHCP4_SEND_REQUEST,
  GRUB_EFI_DHCP4_RCVD_ACK,
  GRUB_EFI_DHCP4_RCVD_NAK,
  GRUB_EFI_DHCP4_SEND_DECLINE,
  GRUB_EFI_DHCP4_BOUND_COMPLETED,
  GRUB_EFI_DHCP4_ENTER_RENEWING,
  GRUB_EFI_DHCP4_ENTER_REBINDING,
  GRUB_EFI_DHCP4_ADDRESS_LOST,
  GRUB_EFI_DHCP4_FAIL
};

typedef enum grub_efi_dhcp4_event grub_efi_dhcp4_event_t;

struct grub_efi_dhcp4_packet_option {
  grub_efi_uint8_t op_code;
  grub_efi_uint8_t length;
  grub_efi_uint8_t data[1];
} GRUB_PACKED;

typedef struct grub_efi_dhcp4_packet_option grub_efi_dhcp4_packet_option_t;

struct grub_efi_dhcp4_config_data {
  grub_efi_uint32_t discover_try_count;
  grub_efi_uint32_t *discover_timeout;
  grub_efi_uint32_t request_try_count;
  grub_efi_uint32_t *request_timeout;
  grub_efi_ipv4_address_t client_address;
  grub_efi_status_t (*dhcp4_callback) (
    grub_efi_dhcp4_protocol_t *this,
    void *context,
    grub_efi_dhcp4_state_t current_state,
    grub_efi_dhcp4_event_t dhcp4_event,
    grub_efi_dhcp4_packet_t *packet,
    grub_efi_dhcp4_packet_t **new_packet
  );
  void *callback_context;
  grub_efi_uint32_t option_count;
  grub_efi_dhcp4_packet_option_t **option_list;
};

typedef struct grub_efi_dhcp4_config_data grub_efi_dhcp4_config_data_t;

struct grub_efi_dhcp4_mode_data {
  grub_efi_dhcp4_state_t state;
  grub_efi_dhcp4_config_data_t config_data;
  grub_efi_ipv4_address_t client_address;
  grub_efi_mac_address_t client_mac_address;
  grub_efi_ipv4_address_t server_address;
  grub_efi_ipv4_address_t router_address;
  grub_efi_ipv4_address_t subnet_mask;
  grub_efi_uint32_t lease_time;
  grub_efi_dhcp4_packet_t *reply_packet;
};

typedef struct grub_efi_dhcp4_mode_data grub_efi_dhcp4_mode_data_t;

struct grub_efi_dhcp4_protocol {
  grub_efi_status_t (*get_mode_data) (grub_efi_dhcp4_protocol_t *this,
	      grub_efi_dhcp4_mode_data_t *dhcp4_mode_data);
  grub_efi_status_t (*configure) (grub_efi_dhcp4_protocol_t *this,
	      grub_efi_dhcp4_config_data_t *dhcp4_cfg_data);
  grub_efi_status_t (*start) (grub_efi_dhcp4_protocol_t *this,
	      grub_efi_event_t completion_event);
  grub_efi_status_t (*renew_rebind) (grub_efi_dhcp4_protocol_t *this,
	      grub_efi_boolean_t rebind_request,
	      grub_efi_event_t completion_event);
  grub_efi_status_t (*release) (grub_efi_dhcp4_protocol_t *this);
  grub_efi_status_t (*stop) (grub_efi_dhcp4_protocol_t *this);
  grub_efi_status_t (*build) (grub_efi_dhcp4_protocol_t *this,
	      grub_efi_dhcp4_packet_t *seed_packet,
	      grub_efi_uint32_t delete_count,
	      grub_efi_uint8_t *delete_list,
	      grub_efi_uint32_t append_count,
	      grub_efi_dhcp4_packet_option_t *append_list[],
	      grub_efi_dhcp4_packet_t **new_packet);
  grub_efi_status_t (*transmit_receive) (grub_efi_dhcp4_protocol_t *this,
	      grub_efi_dhcp4_transmit_receive_token_t *token);
  grub_efi_status_t (*parse) (grub_efi_dhcp4_protocol_t *this,
	      grub_efi_dhcp4_packet_t *packet,
	      grub_efi_uint32_t *option_count,
	      grub_efi_dhcp4_packet_option_t *packet_option_list[]);
};

typedef struct grub_efi_dhcp6_protocol grub_efi_dhcp6_protocol_t;

struct grub_efi_dhcp6_retransmission {
  grub_efi_uint32_t irt;
  grub_efi_uint32_t mrc;
  grub_efi_uint32_t mrt;
  grub_efi_uint32_t mrd;
};

typedef struct grub_efi_dhcp6_retransmission grub_efi_dhcp6_retransmission_t;

enum grub_efi_dhcp6_event {
  GRUB_EFI_DHCP6_SEND_SOLICIT,
  GRUB_EFI_DHCP6_RCVD_ADVERTISE,
  GRUB_EFI_DHCP6_SELECT_ADVERTISE,
  GRUB_EFI_DHCP6_SEND_REQUEST,
  GRUB_EFI_DHCP6_RCVD_REPLY,
  GRUB_EFI_DHCP6_RCVD_RECONFIGURE,
  GRUB_EFI_DHCP6_SEND_DECLINE,
  GRUB_EFI_DHCP6_SEND_CONFIRM,
  GRUB_EFI_DHCP6_SEND_RELEASE,
  GRUB_EFI_DHCP6_SEND_RENEW,
  GRUB_EFI_DHCP6_SEND_REBIND
};

typedef enum grub_efi_dhcp6_event grub_efi_dhcp6_event_t;

struct grub_efi_dhcp6_packet_option {
  grub_efi_uint16_t op_code;
  grub_efi_uint16_t op_len;
  grub_efi_uint8_t data[1];
} GRUB_PACKED;

typedef struct grub_efi_dhcp6_packet_option grub_efi_dhcp6_packet_option_t;

struct grub_efi_dhcp6_header {
  grub_efi_uint32_t transaction_id:24;
  grub_efi_uint32_t message_type:8;
} GRUB_PACKED;

typedef struct grub_efi_dhcp6_header grub_efi_dhcp6_header_t;

struct grub_efi_dhcp6_packet {
  grub_efi_uint32_t size;
  grub_efi_uint32_t length;
  struct {
    grub_efi_dhcp6_header_t header;
    grub_efi_uint8_t option[1];
  } dhcp6;
} GRUB_PACKED;

typedef struct grub_efi_dhcp6_packet grub_efi_dhcp6_packet_t;

struct grub_efi_dhcp6_ia_address {
  grub_efi_ipv6_address_t ip_address;
  grub_efi_uint32_t preferred_lifetime;
  grub_efi_uint32_t valid_lifetime;
};

typedef struct grub_efi_dhcp6_ia_address grub_efi_dhcp6_ia_address_t;

enum grub_efi_dhcp6_state {
  GRUB_EFI_DHCP6_INIT,
  GRUB_EFI_DHCP6_SELECTING,
  GRUB_EFI_DHCP6_REQUESTING,
  GRUB_EFI_DHCP6_DECLINING,
  GRUB_EFI_DHCP6_CONFIRMING,
  GRUB_EFI_DHCP6_RELEASING,
  GRUB_EFI_DHCP6_BOUND,
  GRUB_EFI_DHCP6_RENEWING,
  GRUB_EFI_DHCP6_REBINDING
};

typedef enum grub_efi_dhcp6_state grub_efi_dhcp6_state_t;

#define GRUB_EFI_DHCP6_IA_TYPE_NA 3
#define GRUB_EFI_DHCP6_IA_TYPE_TA 4

struct grub_efi_dhcp6_ia_descriptor {
  grub_efi_uint16_t type;
  grub_efi_uint32_t ia_id;
};

typedef struct grub_efi_dhcp6_ia_descriptor grub_efi_dhcp6_ia_descriptor_t;

struct grub_efi_dhcp6_ia {
  grub_efi_dhcp6_ia_descriptor_t descriptor;
  grub_efi_dhcp6_state_t state;
  grub_efi_dhcp6_packet_t *reply_packet;
  grub_efi_uint32_t ia_address_count;
  grub_efi_dhcp6_ia_address_t ia_address[1];
};

typedef struct grub_efi_dhcp6_ia grub_efi_dhcp6_ia_t;

struct grub_efi_dhcp6_duid {
  grub_efi_uint16_t length;
  grub_efi_uint8_t duid[1];
};

typedef struct grub_efi_dhcp6_duid grub_efi_dhcp6_duid_t;

struct grub_efi_dhcp6_mode_data {
  grub_efi_dhcp6_duid_t *client_id;
  grub_efi_dhcp6_ia_t *ia;
};

typedef struct grub_efi_dhcp6_mode_data grub_efi_dhcp6_mode_data_t;

struct grub_efi_dhcp6_config_data {
  grub_efi_status_t (*dhcp6_callback) (grub_efi_dhcp6_protocol_t this,
		void *context,
		grub_efi_dhcp6_state_t current_state,
		grub_efi_dhcp6_event_t dhcp6_event,
		grub_efi_dhcp6_packet_t *packet,
		grub_efi_dhcp6_packet_t **new_packet);
  void *callback_context;
  grub_efi_uint32_t option_count;
  grub_efi_dhcp6_packet_option_t **option_list;
  grub_efi_dhcp6_ia_descriptor_t ia_descriptor;
  grub_efi_event_t ia_info_event;
  grub_efi_boolean_t reconfigure_accept;
  grub_efi_boolean_t rapid_commit;
  grub_efi_dhcp6_retransmission_t *solicit_retransmission;
};

typedef struct grub_efi_dhcp6_config_data grub_efi_dhcp6_config_data_t;

struct grub_efi_dhcp6_protocol {
  grub_efi_status_t (*get_mode_data) (grub_efi_dhcp6_protocol_t *this,
	    grub_efi_dhcp6_mode_data_t *dhcp6_mode_data,
	    grub_efi_dhcp6_config_data_t *dhcp6_config_data);
  grub_efi_status_t (*configure) (grub_efi_dhcp6_protocol_t *this,
	    grub_efi_dhcp6_config_data_t *dhcp6_cfg_data);
  grub_efi_status_t (*start) (grub_efi_dhcp6_protocol_t *this);
  grub_efi_status_t (*info_request) (grub_efi_dhcp6_protocol_t *this,
	    grub_efi_boolean_t send_client_id,
	    grub_efi_dhcp6_packet_option_t *option_request,
	    grub_efi_uint32_t option_count,
	    grub_efi_dhcp6_packet_option_t *option_list[],
	    grub_efi_dhcp6_retransmission_t *retransmission,
	    grub_efi_event_t timeout_event,
	    grub_efi_status_t (*reply_callback) (grub_efi_dhcp6_protocol_t *this,
		    void *context,
		    grub_efi_dhcp6_packet_t *packet),
	    void *callback_context);
  grub_efi_status_t (*renew_rebind) (grub_efi_dhcp6_protocol_t *this,
	    grub_efi_boolean_t rebind_request);
  grub_efi_status_t (*decline) (grub_efi_dhcp6_protocol_t *this,
	    grub_efi_uint32_t address_count,
	    grub_efi_ipv6_address_t *addresses);
  grub_efi_status_t (*release) (grub_efi_dhcp6_protocol_t *this,
	    grub_efi_uint32_t address_count,
	    grub_efi_ipv6_address_t *addresses);
  grub_efi_status_t (*stop) (grub_efi_dhcp6_protocol_t *this);
  grub_efi_status_t (*parse) (grub_efi_dhcp6_protocol_t *this,
	    grub_efi_dhcp6_packet_t *packet,
	    grub_efi_uint32_t *option_count,
	    grub_efi_dhcp6_packet_option_t *packet_option_list[]);
};
//======================================================================================================================
#define EFIHTTP_WAIT_TIME 10000 // 10000ms = 10s
#define EFIHTTP_RX_BUF_LEN 10240

//******************************************
// Protocol Interface Structure		协议接口结构
//******************************************
struct grub_efi_http;

//******************************************
// EFI_HTTP_VERSION			HTTP版本
//******************************************
typedef enum {
  GRUB_EFI_HTTPVERSION10,						//HTTP版本10
  GRUB_EFI_HTTPVERSION11,						//HTTP版本11
  GRUB_EFI_HTTPVERSIONUNSUPPORTED		//不支持HTTP版本
} grub_efi_http_version_t;

//******************************************
// EFI_HTTPv4_ACCESS_POINT		HTTPv4访问点
//******************************************
typedef struct {
  grub_efi_boolean_t use_default_address;	//使用默认地址
  grub_efi_ipv4_address_t local_address;	//本地地址
  grub_efi_ipv4_address_t local_subnet;		//本地子网
  grub_efi_uint16_t local_port;						//本地端口
} grub_efi_httpv4_access_point_t;

//******************************************
// EFI_HTTPv6_ACCESS_POINT		HTTPv6访问点
//******************************************
typedef struct {
  grub_efi_ipv6_address_t local_address;	//本地地址
  grub_efi_uint16_t local_port;						//本地端口
} grub_efi_httpv6_access_point_t;

//******************************************
// EFI_HTTP_CONFIG_DATA		HTTP配置数据
//******************************************
typedef struct {
  grub_efi_http_version_t http_version;		//HTTP版本
  grub_efi_uint32_t timeout_millisec;			//超时毫秒
  grub_efi_boolean_t local_address_is_ipv6;	//本地地址是ipv6
  union {
    grub_efi_httpv4_access_point_t *ipv4_node;	//ipv4节点
    grub_efi_httpv6_access_point_t *ipv6_node;	//ipv6节点
  } access_point;													//切入点
} grub_efi_http_config_data_t;

//******************************************
// EFI_HTTP_METHOD		HTTP方法
//******************************************
typedef enum {
  GRUB_EFI_HTTPMETHODGET,				//HTTP方法  获得
  GRUB_EFI_HTTPMETHODPOST,			//HTTP方法  开机自检
  GRUB_EFI_HTTPMETHODPATCH,			//HTTP方法  补丁
  GRUB_EFI_HTTPMETHODOPTIONS,		//HTTP方法  选项
  GRUB_EFI_HTTPMETHODCONNECT,		//HTTP方法  连接
  GRUB_EFI_HTTPMETHODHEAD,			//HTTP方法  头
  GRUB_EFI_HTTPMETHODPUT,				//HTTP方法  放置
  GRUB_EFI_HTTPMETHODDELETE,		//HTTP方法  删除
  GRUB_EFI_HTTPMETHODTRACE,			//HTTP方法  跟踪
} grub_efi_http_method_t;

//******************************************
// EFI_HTTP_REQUEST_DATA		HTTP请求数据
//******************************************
typedef struct {
  grub_efi_http_method_t method;	//方法
  grub_efi_char16_t *url;					//网址
} grub_efi_http_request_data_t;

typedef enum {
  GRUB_EFI_HTTP_STATUS_UNSUPPORTED_STATUS = 0,						//不支持
  GRUB_EFI_HTTP_STATUS_100_CONTINUE,											//继续
  GRUB_EFI_HTTP_STATUS_101_SWITCHING_PROTOCOLS,						//交换协议
  GRUB_EFI_HTTP_STATUS_200_OK,														//ok
  GRUB_EFI_HTTP_STATUS_201_CREATED,												//已创建
  GRUB_EFI_HTTP_STATUS_202_ACCEPTED,											//公认
  GRUB_EFI_HTTP_STATUS_203_NON_AUTHORITATIVE_INFORMATION,	//非权威信息
  GRUB_EFI_HTTP_STATUS_204_NO_CONTENT,										//无内容
  GRUB_EFI_HTTP_STATUS_205_RESET_CONTENT,									//重置内容
  GRUB_EFI_HTTP_STATUS_206_PARTIAL_CONTENT,								//部分内容
  GRUB_EFI_HTTP_STATUS_300_MULTIPLE_CHIOCES,							//多种选择
  GRUB_EFI_HTTP_STATUS_301_MOVED_PERMANENTLY,							//永久移动
  GRUB_EFI_HTTP_STATUS_302_FOUND,													//发现
  GRUB_EFI_HTTP_STATUS_303_SEE_OTHER,											//另见
  GRUB_EFI_HTTP_STATUS_304_NOT_MODIFIED,									//未修改
  GRUB_EFI_HTTP_STATUS_305_USE_PROXY,											//使用代理服务器
  GRUB_EFI_HTTP_STATUS_307_TEMPORARY_REDIRECT,						//暂时重定向
  GRUB_EFI_HTTP_STATUS_400_BAD_REQUEST,										//错误的请求
  GRUB_EFI_HTTP_STATUS_401_UNAUTHORIZED,									//未经授权
  GRUB_EFI_HTTP_STATUS_402_PAYMENT_REQUIRED,							//需要付款
  GRUB_EFI_HTTP_STATUS_403_FORBIDDEN,											//禁止的
  GRUB_EFI_HTTP_STATUS_404_NOT_FOUND,											//未找到
  GRUB_EFI_HTTP_STATUS_405_METHOD_NOT_ALLOWED,						//不允许的方法
  GRUB_EFI_HTTP_STATUS_406_NOT_ACCEPTABLE,								//不能接受的
  GRUB_EFI_HTTP_STATUS_407_PROXY_AUTHENTICATION_REQUIRED,	//要求代理授权
  GRUB_EFI_HTTP_STATUS_408_REQUEST_TIME_OUT,							//请求超时
  GRUB_EFI_HTTP_STATUS_409_CONFLICT,											//冲突
  GRUB_EFI_HTTP_STATUS_410_GONE,													//离去
  GRUB_EFI_HTTP_STATUS_411_LENGTH_REQUIRED,								//所需长度
  GRUB_EFI_HTTP_STATUS_412_PRECONDITION_FAILED,						//失败的前提
  GRUB_EFI_HTTP_STATUS_413_REQUEST_ENTITY_TOO_LARGE,			//求的实体太大
  GRUB_EFI_HTTP_STATUS_414_REQUEST_URI_TOO_LARGE,					//请求URI太大
  GRUB_EFI_HTTP_STATUS_415_UNSUPPORTED_MEDIA_TYPE,				//不受支持的媒体类型
  GRUB_EFI_HTTP_STATUS_416_REQUESTED_RANGE_NOT_SATISFIED,	//未满足要求的范围
  GRUB_EFI_HTTP_STATUS_417_EXPECTATION_FAILED,						//预期失败
  GRUB_EFI_HTTP_STATUS_500_INTERNAL_SERVER_ERROR,					//内部服务器错误
  GRUB_EFI_HTTP_STATUS_501_NOT_IMPLEMENTED,								//未实现
  GRUB_EFI_HTTP_STATUS_502_BAD_GATEWAY,										//错误的网关
  GRUB_EFI_HTTP_STATUS_503_SERVICE_UNAVAILABLE,						//暂停服务
  GRUB_EFI_HTTP_STATUS_504_GATEWAY_TIME_OUT,							//网关超时
  GRUB_EFI_HTTP_STATUS_505_HTTP_VERSION_NOT_SUPPORTED			//不支持HTTP版本
} grub_efi_http_status_code_t;		//状态

//******************************************
// EFI_HTTP_RESPONSE_DATA		HTTP响应数据
//******************************************
typedef struct {
  grub_efi_http_status_code_t status_code;	//状态码
} grub_efi_http_response_data_t;

//******************************************
// EFI_HTTP_HEADER		HTTP头
//******************************************
typedef struct {
  grub_efi_char8_t *field_name;		//领域名称
  grub_efi_char8_t *field_value;	//领域值
} grub_efi_http_header_t;

//******************************************
// EFI_HTTP_MESSAGE		HTTP消息
//******************************************
typedef struct {
  union {
    grub_efi_http_request_data_t *request;		//请求
    grub_efi_http_response_data_t *response;	//响应
  } data;																			//数据
  grub_efi_uint32_t header_count;							//标题计数
  grub_efi_http_header_t *headers;						//标头
  grub_efi_uint32_t body_length;							//体长
  void *body;																	//体
} grub_efi_http_message_t;

//******************************************
// EFI_HTTP_TOKEN		HTTP令牌
//******************************************
typedef struct {
  grub_efi_event_t event;						//事件
  grub_efi_status_t status;					//状态
  grub_efi_http_message_t *message;	//信息
} grub_efi_http_token_t;

struct grub_efi_http {
//获得模式数据
  grub_efi_status_t
  (*get_mode_data) (struct grub_efi_http *this,
                    grub_efi_http_config_data_t *http_config_data);
//配置
  grub_efi_status_t
  (*configure) (struct grub_efi_http *this,
                grub_efi_http_config_data_t *http_config_data);	//配置数据
//请求
  grub_efi_status_t
  (*request) (struct grub_efi_http *this,
              grub_efi_http_token_t *token);	//令牌
//取消
  grub_efi_status_t
  (*cancel) (struct grub_efi_http *this,
             grub_efi_http_token_t *token);	//令牌
//响应
  grub_efi_status_t
  (*response) (struct grub_efi_http *this,
               grub_efi_http_token_t *token);	//令牌
//获得
  grub_efi_status_t
  (*poll) (struct grub_efi_http *this);
};
typedef struct grub_efi_http grub_efi_http_t;	//协议接口结构

typedef struct grub_efi_net_interface grub_efi_net_interface_t;
typedef struct grub_efi_net_ip_config grub_efi_net_ip_config_t;
typedef union grub_efi_net_ip_address grub_efi_net_ip_address_t;
typedef struct grub_efi_net_ip_manual_address grub_efi_net_ip_manual_address_t;

struct grub_efi_net_interface		//网络接口
{
  char *name;														//名称
  int prefer_ip6;												//是ip6
  struct grub_efi_net_device *dev;			//网络设备
  struct grub_efi_net_io *io;						//网络io
  grub_efi_net_ip_config_t *ip_config;	//ip配置
  int io_type;													//io类型
  struct grub_efi_net_interface *next;	//下一个
};

#define efi_net_interface_get_hw_address(inf) inf->ip_config->get_hw_address (inf->dev)
#define efi_net_interface_get_address(inf) inf->ip_config->get_address (inf->dev)
#define efi_net_interface_get_route_table(inf) inf->ip_config->get_route_table (inf->dev)
#define efi_net_interface_set_address(inf, addr, with_subnet) inf->ip_config->set_address (inf->dev, addr, with_subnet)
#define efi_net_interface_set_gateway(inf, addr) inf->ip_config->set_gateway (inf->dev, addr)
#define efi_net_interface_set_dns(inf, addr) inf->ip_config->set_dns (inf->dev, addr)

struct grub_efi_net_ip_config
{
  char * (*get_hw_address) (struct grub_efi_net_device *dev);
  char * (*get_address) (struct grub_efi_net_device *dev);
  char ** (*get_route_table) (struct grub_efi_net_device *dev);
  grub_efi_net_interface_t * (*best_interface) (struct grub_efi_net_device *dev, grub_efi_net_ip_address_t *address);
  int (*set_address) (struct grub_efi_net_device *dev, grub_efi_net_ip_manual_address_t *net_ip, int with_subnet);
  int (*set_gateway) (struct grub_efi_net_device *dev, grub_efi_net_ip_address_t *address);
  int (*set_dns) (struct grub_efi_net_device *dev, grub_efi_net_ip_address_t *dns);
};

union grub_efi_net_ip_address
{
  grub_efi_ipv4_address_t ip4;
  grub_efi_ipv6_address_t ip6;
};

struct grub_efi_net_ip_manual_address
{
  int is_ip6;
  union
  {
    grub_efi_ip4_config2_manual_address_t ip4;
    grub_efi_ip6_config_manual_address_t ip6;
  };
};

struct grub_efi_net_device
{
  grub_efi_handle_t handle;
  grub_efi_ip4_config2_protocol_t *ip4_config;
  grub_efi_ip6_config_protocol_t *ip6_config;
  grub_efi_handle_t http_handle;
  grub_efi_http_t *http;
  grub_efi_handle_t ip4_pxe_handle;
  grub_efi_pxe_t *ip4_pxe;
  grub_efi_handle_t ip6_pxe_handle;
  grub_efi_pxe_t *ip6_pxe;
  grub_efi_handle_t dhcp4_handle;
  grub_efi_dhcp4_protocol_t *dhcp4;
  grub_efi_handle_t dhcp6_handle;
  grub_efi_dhcp6_protocol_t *dhcp6;
  char *card_name;
  grub_efi_net_interface_t *net_interfaces;
  struct grub_efi_net_device *next;
};

struct grub_efi_net_io
{
  void (*configure) (struct grub_efi_net_device *dev, int prefer_ip6);
  grub_err_t (*open) (struct grub_efi_net_device *dev,
		    int prefer_ip6,
		    //grub_file_t file,
				char *file,
		    const char *filename,
		    int type);
  grub_ssize_t (*read) (struct grub_efi_net_device *dev,
			int prefer_ip6,
			//grub_file_t file,
			char *file,
			char *buf,
			grub_size_t len);
  grub_err_t (*close) (struct grub_efi_net_device *dev,
		      int prefer_ip6,
		      //grub_file_t file);
					char *file);
};

extern struct grub_efi_net_device *net_devices;

extern struct grub_efi_net_io io_http;
extern struct grub_efi_net_io io_pxe;

extern grub_efi_net_ip_config_t *efi_net_ip4_config;
extern grub_efi_net_ip_config_t *efi_net_ip6_config;

char *
grub_efi_ip4_address_to_string (grub_efi_ipv4_address_t *address);

char *
grub_efi_ip6_address_to_string (grub_efi_pxe_ipv6_address_t *address);

char *
grub_efi_hw_address_to_string (grub_efi_uint32_t hw_address_size, grub_efi_mac_address_t hw_address);

int
grub_efi_string_to_ip4_address (const char *val, grub_efi_ipv4_address_t *address, const char **rest);

int
grub_efi_string_to_ip6_address (const char *val, grub_efi_ipv6_address_t *address, const char **rest);

char *
grub_efi_ip6_interface_name (struct grub_efi_net_device *dev);

char *
grub_efi_ip4_interface_name (struct grub_efi_net_device *dev);

grub_efi_net_interface_t *
grub_efi_net_create_interface (struct grub_efi_net_device *dev,
		const char *interface_name,
		grub_efi_net_ip_manual_address_t *net_ip,
		int has_subnet);

int grub_efi_net_fs_init (void);
void grub_efi_net_fs_fini (void);
int grub_efi_net_boot_from_https (void);
int grub_efi_net_boot_from_opa (void);

extern grub_efi_status_t EFIAPI blockio_read_write (block_io_protocol_t *this, grub_efi_uint32_t media_id,
              grub_efi_lba_t lba, grub_efi_uintn_t len, void *buf, int read_write);
extern grub_size_t block_io_protocol_this;
extern int get_efi_device_boot_path (int drive, int flags);
extern grub_efi_device_path_t * grub_efi_file_device_path (grub_efi_device_path_t *dp, const char *filename);
extern int no_install_vdisk;
extern grub_efi_physical_address_t grub4dos_self_address;
extern grub_uint64_t grub_divmod64 (grub_uint64_t n, grub_uint64_t d, grub_uint64_t *r);
extern grub_uint64_t EXPORT_FUNC (__umoddi3) (grub_uint64_t a, grub_uint64_t b);
extern grub_uint64_t EXPORT_FUNC (__udivdi3) (grub_uint64_t a, grub_uint64_t b);
extern void start_event (void);
extern void close_event (void);
extern int find_specified_file (int drive, int partition, char* file);
extern char *vhd_file_name;
extern char vhd_file_path [128];
extern int GetParentUtf8Name (char *dest, grub_uint16_t *src);
extern int GetSectorSequence (char* Utf8Name, struct fragment_map_slot** SectorSeq, int exist);
extern char *preset_menu;
extern int use_preset_menu;
//======================================================================================================================

#define GRUB_RSDP_SIGNATURE "RSD PTR "
#define GRUB_RSDP_SIGNATURE_SIZE 8

struct grub_acpi_rsdp_v10
{
  grub_uint8_t signature[GRUB_RSDP_SIGNATURE_SIZE];
  grub_uint8_t checksum;
  grub_uint8_t oemid[6];
  grub_uint8_t revision;
  grub_uint32_t rsdt_addr;
} __attribute__ ((packed));

struct grub_acpi_rsdp_v20
{
  struct grub_acpi_rsdp_v10 rsdpv1;
  grub_uint32_t length;
  grub_uint64_t xsdt_addr;
  grub_uint8_t checksum;
  grub_uint8_t reserved[3];
} __attribute__ ((packed));

struct grub_acpi_table_header
{
  grub_uint8_t signature[4];
  grub_uint32_t length;
  grub_uint8_t revision;
  grub_uint8_t checksum;
  grub_uint8_t oemid[6];
  grub_uint8_t oemtable[8];
  grub_uint32_t oemrev;
  grub_uint8_t creator_id[4];
  grub_uint32_t creator_rev;
} __attribute__ ((packed));

#define GRUB_ACPI_FADT_SIGNATURE "FACP"

struct grub_acpi_fadt
{
  struct grub_acpi_table_header hdr;
  grub_uint32_t facs_addr;
  grub_uint32_t dsdt_addr;
  grub_uint8_t somefields1[20];
  grub_uint32_t pm1a;
  grub_uint8_t somefields2[8];
  grub_uint32_t pmtimer;
  grub_uint8_t somefields3[32];
  grub_uint32_t flags;
  grub_uint8_t somefields4[16];
  grub_uint64_t facs_xaddr;
  grub_uint64_t dsdt_xaddr;
  grub_uint8_t somefields5[96];
} __attribute__ ((packed));

#define GRUB_ACPI_SLP_EN (1 << 13)
#define GRUB_ACPI_SLP_TYP_OFFSET 10

enum
{
  GRUB_ACPI_OPCODE_ZERO = 0, GRUB_ACPI_OPCODE_ONE = 1,
  GRUB_ACPI_OPCODE_NAME = 8, GRUB_ACPI_OPCODE_ALIAS = 0x06,
  GRUB_ACPI_OPCODE_BYTE_CONST = 0x0a,
  GRUB_ACPI_OPCODE_WORD_CONST = 0x0b,
  GRUB_ACPI_OPCODE_DWORD_CONST = 0x0c,
  GRUB_ACPI_OPCODE_STRING_CONST = 0x0d,
  GRUB_ACPI_OPCODE_SCOPE = 0x10,
  GRUB_ACPI_OPCODE_BUFFER = 0x11,
  GRUB_ACPI_OPCODE_PACKAGE = 0x12,
  GRUB_ACPI_OPCODE_METHOD = 0x14, GRUB_ACPI_OPCODE_EXTOP = 0x5b,
  GRUB_ACPI_OPCODE_ADD = 0x72,
  GRUB_ACPI_OPCODE_CONCAT = 0x73,
  GRUB_ACPI_OPCODE_SUBTRACT = 0x74,
  GRUB_ACPI_OPCODE_MULTIPLY = 0x77,
  GRUB_ACPI_OPCODE_DIVIDE = 0x78,
  GRUB_ACPI_OPCODE_LSHIFT = 0x79,
  GRUB_ACPI_OPCODE_RSHIFT = 0x7a,
  GRUB_ACPI_OPCODE_AND = 0x7b,
  GRUB_ACPI_OPCODE_NAND = 0x7c,
  GRUB_ACPI_OPCODE_OR = 0x7d,
  GRUB_ACPI_OPCODE_NOR = 0x7e,
  GRUB_ACPI_OPCODE_XOR = 0x7f,
  GRUB_ACPI_OPCODE_CONCATRES = 0x84,
  GRUB_ACPI_OPCODE_MOD = 0x85,
  GRUB_ACPI_OPCODE_INDEX = 0x88,
  GRUB_ACPI_OPCODE_CREATE_DWORD_FIELD = 0x8a,
  GRUB_ACPI_OPCODE_CREATE_WORD_FIELD = 0x8b,
  GRUB_ACPI_OPCODE_CREATE_BYTE_FIELD = 0x8c,
  GRUB_ACPI_OPCODE_TOSTRING = 0x9c,
  GRUB_ACPI_OPCODE_IF = 0xa0, GRUB_ACPI_OPCODE_ONES = 0xff
};

enum
{
  GRUB_ACPI_EXTOPCODE_MUTEX = 0x01,
  GRUB_ACPI_EXTOPCODE_EVENT_OP = 0x02,
  GRUB_ACPI_EXTOPCODE_OPERATION_REGION = 0x80,
  GRUB_ACPI_EXTOPCODE_FIELD_OP = 0x81,
  GRUB_ACPI_EXTOPCODE_DEVICE_OP = 0x82,
  GRUB_ACPI_EXTOPCODE_PROCESSOR_OP = 0x83,
  GRUB_ACPI_EXTOPCODE_POWER_RES_OP = 0x84,
  GRUB_ACPI_EXTOPCODE_THERMAL_ZONE_OP = 0x85,
  GRUB_ACPI_EXTOPCODE_INDEX_FIELD_OP = 0x86,
  GRUB_ACPI_EXTOPCODE_BANK_FIELD_OP = 0x87,
};

#endif /* ! ASM_FILE */
#endif /* ! GRUB_SHARED_HEADER */
