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
#if 1 //调试时设置为0,显示警告信息
/* Disable all gcc warnings */

#if defined (__GNUC__) && defined (__GNUC_MINOR__) && (((__GNUC__ == 4) && (__GNUC_MINOR__  > 8)) || (__GNUC__ >= 5))
#pragma GCC diagnostic ignored "-Wunused-value"
#endif

#if defined (__GNUC__) && (__GNUC__ >= 6) 
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif

#if defined (__GNUC__) && (__GNUC__ >= 10) 
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#endif
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

#define SYSTEM_RESERVED_MEMORY	0x2000000
#define LINUX_TMP_MEMORY	0x2600000

/* unifont start at 24M */
//#define UNIFONT_START		0x1800000
//#define UNIFONT_START_SIZE		0x800000		//32*32

//#define narrow_char_indicator	(*(unsigned long *)(UNIFONT_START + 'A'*num_wide*font_h))

/* graphics video memory */
#define VIDEOMEM 0xA0000

/* Maximum command line size. Before you blindly increase this value,
   see the comment in char_io.c (get_cmdline).  */
#define MAX_CMDLINE 1600
#define NEW_HEAPSIZE 1500

/* 512-byte scratch area */
/* more than 1 sector used! See chainloader code. */
#define SCRATCHADDR  RAW_ADDR (0x1F000)
#define SCRATCHSEG   RAW_SEG (0x1F00)

/*
 *  This is the location of the raw device buffer.  It is 31.5K
 *  in size.
 */

/* BUFFERLEN must be a power of two, i.e., 2^n, or 2**n */
/* BUFFERLEN must be 64K for now! */
#define BUFFERLEN   0x10000
#define BUFFERADDR  RAW_ADDR (0x30000)
#define BUFFERSEG   RAW_SEG (0x3000)

#define BOOT_PART_TABLE	RAW_ADDR (0x07be)

/*
 *  BIOS disk defines
 */
#define BIOSDISK_READ			0x0
#define BIOSDISK_WRITE			0x1
#define BIOSDISK_ERROR_GEOMETRY		0x100
#define BIOSDISK_FLAG_LBA_EXTENSION	0x1
#define BIOSDISK_FLAG_CDROM		0x2
#define BIOSDISK_FLAG_BIFURCATE		0x4	/* accessibility acts differently between chs and lba */
#define BIOSDISK_FLAG_GEOMETRY_OK	0x8
#define BIOSDISK_FLAG_LBA_1_SECTOR	0x10

/*
 *  This is the filesystem (not raw device) buffer.
 *  It is 32K in size, do not overrun!
 */

#define FSYS_BUFLEN  0x8000
#define FSYS_BUF RAW_ADDR (0x3E0000)

/* Paging structure : PML4, PDPT, PD  4096-bytes each */
/* Memory area from 0x50000 to the end of low memory is used by gfxmenu. So we
 * should not use 0x60000 for page tables. And all other free room in the low
 * memory is reserved. So we should use extended memory if possible.
 * Currently we use the ending 16K of the first 16M. -- tinybit
 *
 * Big problem! Some chipset use 1M at 15M. -- tinybit 2012-11-01
 */

//#define PAGING_TABLES_BUF	0x60000
//#define PAGING_TABLES_BUF	0xFFC000
#define PAGING_TABLES_BUF	0xEFC000
#define PAGING_TABLES_BUFLEN	0x4000

/* Command-line buffer for Multiboot kernels and modules. This area
   includes the area into which Stage 1.5 and Stage 1 are loaded, but
   that's no problem.  */
#define MB_CMDLINE_BUF		RAW_ADDR (0x7000)
#define MB_CMDLINE_BUFLEN	0x1000

//#define FSYS_BUF		0x3E0000
//#define FSYS_BUFLEN		0x008000

//#define PART_TABLE_BUF	0x3E8000
#define PART_TABLE_BUF		(FSYS_BUF + FSYS_BUFLEN)
#define PART_TABLE_BUFLEN	0x001000

//#define PART_TABLE_TMPBUF	0x3E9000
#define PART_TABLE_TMPBUF	(PART_TABLE_BUF + PART_TABLE_BUFLEN)
#define PART_TABLE_TMPBUFLEN	0x000200

//#define CMDLINE_BUF		0x3E9200
#define CMDLINE_BUF		(PART_TABLE_TMPBUF + PART_TABLE_TMPBUFLEN)
#define CMDLINE_BUFLEN		0x000640

/* The buffer for the completion.  */
//#define COMPLETION_BUF	0x3E9840
#define COMPLETION_BUF		(CMDLINE_BUF + CMDLINE_BUFLEN)
#define COMPLETION_BUFLEN	0x000640

/* The buffer for the unique string.  */
//#define UNIQUE_BUF		0x3E9E80
#define UNIQUE_BUF		(COMPLETION_BUF + COMPLETION_BUFLEN)
#define UNIQUE_BUFLEN		0x000640

/* The history buffer for the command-line.  */
//#define HISTORY_BUF		0x3EA4C0
#define HISTORY_BUF		(UNIQUE_BUF + UNIQUE_BUFLEN)
#define HISTORY_SIZE		5
//#define HISTORY_BUFLEN	0x001F40
#define HISTORY_BUFLEN		(MAX_CMDLINE * HISTORY_SIZE)

///* THe buffer for the filename of "/boot/grub/default".  */
//#define DEFAULT_FILE_BUF	(PASSWORD_BUF + PASSWORD_BUFLEN)
//#define DEFAULT_FILE_BUFLEN	0x60

/* graphics.c uses 0x3A0000 - 0x3DA980 and 0x3FC000 - 0x3FF9D0 */

/* The size of the drive map.  */
#define	MAP_NUM_16	0
#define	CDROM_INIT  0

#if	MAP_NUM_16
#define DRIVE_MAP_SIZE		16
#else
#define DRIVE_MAP_SIZE		8
#endif

/* The size of the drive_map_slot struct.  */
#define DRIVE_MAP_SLOT_SIZE	24

/* The fragment of the drive map.  */
//#define DRIVE_MAP_FRAGMENT		32
#define DRIVE_MAP_FRAGMENT		0x7E

//#define FRAGMENT_MAP_SLOT_SIZE		0x280
#define FRAGMENT_MAP_SLOT_SIZE		0x800

/* The size of the key map.  */
#define KEY_MAP_SIZE		128

/*
 *  extended chainloader code address for switching to real mode
 */

#define HMA_ADDR		0x2B0000

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
#define DISP_UL		(menu_border.disp_ul)
#define DISP_UR		(menu_border.disp_ur)
#define DISP_LL		(menu_border.disp_ll)
#define DISP_LR		(menu_border.disp_lr)
#define DISP_HORIZ	(menu_border.disp_horiz)
#define DISP_VERT	(menu_border.disp_vert)
#define DISP_LEFT	0x1b
#define DISP_RIGHT	0x1a
#define DISP_UP		0x18
#define DISP_DOWN	0x19

/* Remap some libc-API-compatible function names so that we prevent
   circularararity. */
#ifndef WITHOUT_LIBC_STUBS
#define memmove grub_memmove
#define memcpy grub_memmove	/* we don't need a separate memcpy */
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

#ifndef ASM_FILE

typedef unsigned char 	grub_u8_t;
typedef unsigned short 	grub_u16_t;
typedef unsigned int		grub_u32_t;
typedef unsigned long long	grub_u64_t;
typedef signed char		grub_s8_t;
typedef short			grub_s16_t;
typedef int			grub_s32_t;
typedef long long		grub_s64_t;
#define PACKED			__attribute__ ((packed))

/*
 *  Below this should be ONLY defines and other constructs for C code.
 */

/* function prototypes for asm functions */
unsigned char * graphics_get_font();
void graphics_set_palette(int idx, int color);
extern unsigned long long color_8_to_64 (unsigned char color8);
extern unsigned long long color_4_to_32 (unsigned char color4);
extern unsigned char color_64_to_8 (unsigned long long color64);
extern unsigned char color_32_to_4 (unsigned long color32);
extern int console_color[6];
extern unsigned long long console_color_64bit[6];
extern unsigned long current_color;
extern unsigned long long current_color_64bit;
extern unsigned long cursor_state;
extern unsigned long graphics_mode;
extern unsigned long font_w;
extern unsigned long font_h;
extern unsigned char num_wide;
extern unsigned long font_spacing;
extern unsigned long line_spacing;
extern void rectangle(int left, int top, int length, int width, int line);
extern int hex (int v);
extern unsigned long splashimage_loaded;
extern unsigned long X_offset,Y_offset;
struct box
{
	unsigned char enable;
	unsigned short start_x;
	unsigned short start_y;
	unsigned short horiz;
	unsigned short vert;
	unsigned char linewidth;
	unsigned long color;
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
#define MENU_TITLE					0x3A8000
#define MENU_TITLE_LENGTH		0x800
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
  unsigned long header;			/* Magic signature "HdrS" */
  unsigned short version;		/* Boot protocol version supported */
  unsigned long realmode_swtch;		/* Boot loader hook */
  unsigned long start_sys;		/* Points to kernel version string */
  unsigned char type_of_loader;		/* Boot loader identifier */
  unsigned char loadflags;		/* Boot protocol option flags */
  unsigned short setup_move_size;	/* Move to high memory size */
  unsigned long code32_start;		/* Boot loader hook */
  unsigned long ramdisk_image;		/* initrd load address */
  unsigned long ramdisk_size;		/* initrd size */
  unsigned long bootsect_kludge;	/* obsolete */
  unsigned short heap_end_ptr;		/* Free memory after setup end */
  unsigned short pad1;			/* Unused */
  char *cmd_line_ptr;			/* Points to the kernel command line */
  unsigned long initrd_addr_max;	/* The highest address of initrd */
} __attribute__ ((packed));

/* Memory map address range descriptor used by GET_MMAP_ENTRY. */
struct mmar_desc
{
  unsigned long desc_len;	/* Size of this descriptor. */
  unsigned long long addr;	/* Base address. */
  unsigned long long length;	/* Length in bytes. */
  unsigned long type;		/* Type of address range. */
} __attribute__ ((packed));

/* VBE controller information.  */
struct vbe_controller
{
  unsigned char signature[4];
  unsigned short version;
  unsigned long oem_string;
  unsigned long capabilities;
  unsigned long video_mode;
  unsigned short total_memory;
  unsigned short oem_software_rev;
  unsigned long oem_vendor_name;
  unsigned long oem_product_name;
  unsigned long oem_product_rev;
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
  unsigned long win_func;
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
  unsigned long phys_base;
  unsigned long reserved1;
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
  unsigned long max_pixel_clock;

  unsigned char reserved3[189];
} __attribute__ ((packed));


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
} grub_error_t;

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
extern unsigned long fontx;
extern unsigned long fonty;
extern unsigned long install_partition;
extern unsigned long boot_drive;
//extern unsigned long install_second_sector;
//extern struct apm_info apm_bios_info;
extern unsigned long boot_part_addr;
extern int saved_entryno;
extern unsigned char force_lba;
extern char version_string[];
extern char config_file[];
extern unsigned long linux_text_len;
extern char *linux_data_tmp_addr;
extern char *linux_data_real_addr;
extern char *linux_bzimage_tmp_addr;
extern int quit_print;
extern struct linux_kernel_header *linux_header;

extern unsigned long free_mem_start;
extern unsigned long free_mem_end;

extern unsigned char menu_tab;
extern unsigned char menu_tab_ext;
extern unsigned char num_string;
extern unsigned char menu_font_spacing;
extern unsigned char menu_line_spacing;
extern int password_x;
extern unsigned char timeout_x;
extern unsigned char timeout_y;
extern unsigned long long timeout_color;
extern unsigned long long keyhelp_color;
//extern unsigned char font_type;
//extern unsigned char scan_mode;
//extern unsigned char store_mode;
extern unsigned char graphic_enable;
extern unsigned char graphic_type;
extern unsigned char graphic_row;
extern unsigned char graphic_list;
extern unsigned short graphic_wide;
extern unsigned short graphic_high;
extern unsigned short row_space;
extern char graphic_file[128];
extern void clear_entry (int x, int y, int w, int h);
extern void vbe_fill_color (unsigned long color);
extern unsigned long long hotkey_color_64bit;
extern unsigned int hotkey_color;
extern int (*ext_timer)(char *arg, int flags);

#ifdef SUPPORT_GRAPHICS
extern unsigned long current_x_resolution;
extern unsigned long current_y_resolution;
extern unsigned long current_bits_per_pixel;
extern unsigned long current_bytes_per_scanline;
extern unsigned long current_bytes_per_pixel;
extern unsigned long current_phys_base;
extern unsigned long fill_color;
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
  unsigned long addr;
  unsigned long pid;
};

struct mem_alloc_array *mem_alloc_array_start;
struct mem_alloc_array *mem_alloc_array_end;
extern unsigned int prog_pid;
struct malloc_array
{
  unsigned long addr;
  struct malloc_array *next;
};

extern void *grub_malloc(unsigned long size);
extern void *grub_zalloc(unsigned long size);
extern void grub_free(void *ptr);
struct malloc_array *malloc_array_start;

/* If not using config file, this variable is set to zero,
   otherwise non-zero.  */
extern int use_config_file;
#define	use_preset_menu *(unsigned long *)0x307FF8
/* print debug message on startup if the DEBUG_KEY is pressed. */
extern int debug_boot;
extern int console_checkkey (void);
extern int console_getkey (void);
extern int console_beep (void);
extern int beep_func(char *arg, int flags);
extern int defer(unsigned short millisecond);
extern unsigned short count_ms;
extern unsigned char beep_play;
extern unsigned char beep_enable;
extern unsigned short beep_frequency;
extern unsigned short beep_duration;
extern unsigned long long initrd_start_sector;
extern int disable_map_info;
extern int map_func (char *arg, int flags);
//#define SLEEP {unsigned long i;for (i=0;i<0xFFFFFFFF;i++);}
#define DEBUG_SLEEP {debug_sleep(debug_boot,__LINE__,__FILE__);}
extern inline void debug_sleep(int debug_boot, int line, char *file);

#ifdef DEBUG_TIME
#define PRINT_DEBUG_INFO debug_time(__LINE__,__FILE__);
extern inline void debug_time(const int line,const char*file);
#endif

extern void hexdump(grub_u64_t,char*,int);
extern int builtin_cmd (char *cmd, char *arg, int flags);
extern long realmode_run(long regs_ptr);

#define MAX_USER_VARS 60
#define MAX_VARS 64
#define MAX_VAR_LEN	8
#define MAX_ENV_LEN	512
#define MAX_BUFFER	(MAX_VARS * (MAX_VAR_LEN + MAX_ENV_LEN))
//#define BASE_ADDR 0x45000
#define VARIABLE_BASE_ADDR (*(unsigned long*)0x307FF4)
#define		QUOTE_CHAR	(*(char*)0x307FF0)
#define BASE_ADDR	VARIABLE_BASE_ADDR
typedef char VAR_NAME[MAX_VAR_LEN];
typedef char VAR_VALUE[MAX_ENV_LEN];
#define VAR ((VAR_NAME *)BASE_ADDR)
#define ENVI ((VAR_VALUE *)(BASE_ADDR + MAX_VARS * MAX_VAR_LEN))
#define _WENV_ 60

#define		WENV_ENVI	((char*)0x4CA00)
#define		WENV_RANDOM	(*(unsigned long *)(WENV_ENVI+0x20))
//#define		QUOTE_CHAR	(*(char*)(WENV_ENVI + 0x30))
#define		PATHEXT		(WENV_ENVI + 0x40)
#define		WENV_TMP	(WENV_ENVI + 0x80)

#define VAR_EX_TMP ((char *)(BASE_ADDR+MAX_VARS * (MAX_VAR_LEN + MAX_ENV_LEN)))
#define set_envi(var, val)			envi_cmd(var, val, 0)
//#define get_env(var, val)			envi_cmd(var, val, 1)
#define get_env_all()				envi_cmd(NULL, NULL, 2)
#define reset_env_all()				envi_cmd(NULL, NULL, 3)
extern int envi_cmd(const char *var,char * const env,int flags);
extern long long retval64;

/* GUI interface variables. */
# define MAX_FALLBACK_ENTRIES	8
extern int fallback_entries[MAX_FALLBACK_ENTRIES];
extern int fallback_entryno;
extern int default_entry;
extern int current_entryno;
extern const char *preset_menu;

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
extern unsigned long gfx_drive, gfx_partition;

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
extern void (*disk_read_hook) (unsigned long long, unsigned long, unsigned long long);
extern void (*disk_read_func) (unsigned long long, unsigned long, unsigned long long);

/* The flag for debug mode.  */
extern int debug;
extern int debug_bat;
extern grub_u8_t debug_msg;

extern unsigned long current_drive;
extern unsigned long current_partition;

extern int fsys_type;
extern unsigned int fats_type;
extern char vol_name[256];

//extern inline unsigned long log2_tmp (unsigned long word);
extern unsigned long unicode_to_utf8 (unsigned short *filename, unsigned char *utf8, unsigned long n);

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
  /* The number of cylinders */
  unsigned long cylinders;
  /* The number of heads */
  unsigned long heads;
  /* The number of sectors */
  unsigned long sectors;
  /* The total number of sectors */
  unsigned long long total_sectors;
  /* Device sector size */
  unsigned long sector_size;
  /* Flags */
  unsigned long flags;
};

extern unsigned long long part_start;
extern unsigned long long part_length;

extern unsigned long current_slice;
extern unsigned long dos_drive_geometry;
extern unsigned long dos_part_start;

extern unsigned long force_geometry_tune;

extern int buf_drive;
extern int buf_track;
extern struct geometry buf_geom;
extern struct geometry tmp_geom;
extern struct geometry fd_geom[4];
extern struct geometry hd_geom[8];

/* these are the current file position and maximum file position */
extern unsigned long long filepos;
extern unsigned long long filemax;
extern unsigned long long filesize;
extern unsigned long long gzip_filemax;

extern unsigned long emu_iso_sector_size_2048;

#define ISO_TYPE_9660 0
#define ISO_TYPE_udf 1
#define ISO_TYPE_Joliet 2
#define ISO_TYPE_RockRidge 3
extern unsigned int iso_type;
extern char iso_types;
extern unsigned long udf_BytePerSector;
/*
 *  Common BIOS/boot data.
 */

extern char *end_of_low_16bit_code;
extern struct multiboot_info mbi;
extern unsigned long saved_drive;
extern unsigned long saved_partition;
extern char saved_dir[256];
extern unsigned long e820cycles;	/* control how many e820 cycles will keep hooked */
extern unsigned long int15nolow;	/* unprotect int13_handler code with int15 */
extern unsigned long memdisk_raw;	/* raw mode as in memdisk */
extern unsigned long a20_keep_on;	/* keep a20 on after RAM drive sector access */
extern unsigned long lba_cd_boot;	/* LBA of no-emulation boot image, in 2048-byte sectors */
extern unsigned long safe_mbr_hook;	/* safe mbr hook flags used by Win9x */
extern unsigned long int13_scheme;	/* controls disk access methods in emulation */
extern unsigned char atapi_dev_count;	/* ATAPI CDROM DRIVE COUNT */
extern unsigned long reg_base_addr_append;
extern unsigned char usb_delay;
extern unsigned short One_transfer;
extern unsigned char usb_count_error;
extern unsigned char usb_drive_num[8];
extern unsigned long init_usb(void);
extern unsigned long init_atapi(void);
extern unsigned char min_cdrom_id;	/* MINIMUM ATAPI CDROM DRIVE NUMBER */
extern unsigned long cdrom_drive;
extern unsigned long force_cdrom_as_boot_device;
extern unsigned long ram_drive;
extern unsigned long long md_part_base;
extern unsigned long long md_part_size;
extern unsigned long long rd_base;
extern unsigned long long rd_size;
extern unsigned long long saved_mem_higher;
extern unsigned long saved_mem_upper;
extern unsigned long saved_mem_lower;
extern unsigned long saved_mmap_addr;
extern unsigned long saved_mmap_length;
extern unsigned long extended_memory;
extern unsigned long init_free_mem_start;
extern int config_len;
extern char menu_init_script_file[32];
extern unsigned long minimum_mem_hi_in_map;
/*
 *  Error variables.
 */

extern grub_error_t errnum;
extern char *err_list[];

/* Simplify declaration of entry_addr. */
typedef void (*entry_func) (int, int, int, int, int, int)
     __attribute__ ((noreturn));

extern unsigned long cur_addr;
extern entry_func entry_addr;

/* Enter the stage1.5/stage2 C code after the stack is set up. */
void cmain (void);
extern char default_file[60];

/* Halt the processor (called after an unrecoverable error). */
void stop (void) __attribute__ ((noreturn));

/* Reboot the system.  */
void grub_reboot (void) __attribute__ ((noreturn));

void boot_int18 (void) __attribute__ ((noreturn));

/* Halt the system, using APM if possible. If NO_APM is true, don't use
   APM even if it is available.  */
void grub_halt (int skip_flags); //__attribute__ ((noreturn));

struct drive_map_slot
{
	/* Remember to update DRIVE_MAP_SLOT_SIZE once this is modified.
	 * The struct size must be a multiple of 4.
	 */

	  /* X=max_sector bit 7: read only or fake write */
	  /* Y=to_sector  bit 6: safe boot or fake write */
	  /* ------------------------------------------- */
	  /* X Y: meaning of restrictions imposed on map */
	  /* ------------------------------------------- */
	  /* 1 1: read only=0, fake write=1, safe boot=0 */
	  /* 1 0: read only=1, fake write=0, safe boot=0 */
	  /* 0 1: read only=0, fake write=0, safe boot=1 */
	  /* 0 0: read only=0, fake write=0, safe boot=0 */

	unsigned char from_drive;																										//00
	unsigned char to_drive;		/* 0xFF indicates a memdrive */										//01
	unsigned char max_head;																											//02
	unsigned char max_sector;	/* bit 7: read only */														//03
					/* bit 6: disable lba */

	unsigned short to_cylinder;	/* max cylinder of the TO drive */							//04
					/* bit 15:  TO  drive support LBA */
					/* bit 14:  TO  drive is CDROM(with big 2048-byte sector) */
					/* bit 13: FROM drive is CDROM(with big 2048-byte sector) */

	unsigned char to_head;		/* max head of the TO drive */										//06
	unsigned char to_sector;	/* max sector of the TO drive */									//07
					/* bit 7: in-situ */
					/* bit 6: fake-write or safe-boot */

	unsigned long long start_sector;																						//08
	//unsigned long start_sector_hi;	/* hi dword of the 64-bit value */
	unsigned long long sector_count;																						//16
	//unsigned long sector_count_hi;	/* hi dword of the 64-bit value */
};

struct fragment_map_slot
{
	unsigned short slot_len;
	unsigned char from;
	unsigned char to;
	unsigned long long fragment_data[0];
};

#if	MAP_NUM_16
struct drive_map_slot hooked_drive_map[DRIVE_MAP_SIZE + 1];
extern struct drive_map_slot hooked_drive_map_1[DRIVE_MAP_SIZE / 2 + 1];
extern struct drive_map_slot hooked_drive_map_2[DRIVE_MAP_SIZE / 2 + 1];
#else
extern struct drive_map_slot hooked_drive_map[DRIVE_MAP_SIZE + 1];
#endif
extern struct drive_map_slot   bios_drive_map[DRIVE_MAP_SIZE + 1];
extern struct fragment_map_slot hooked_fragment_map;
extern int drive_map_slot_empty (struct drive_map_slot item);

/* Copy MAP to the drive map and set up int13_handler.  */
void set_int13_handler (struct drive_map_slot *map);

/* Restore the original int13 handler.  */
int unset_int13_handler (int check_status_only);

/* Set up int15_handler.  */
void set_int15_handler (void);

/* Restore the original int15 handler.  */
void unset_int15_handler (void);

/* Track the int13 handler to probe I/O address space.  */
void track_int13 (int drive);

/* The key map.  */
//extern unsigned short bios_key_map[];
extern unsigned long ascii_key_map[];

/* calls for direct boot-loader chaining */
void chain_stage1 (unsigned long segment, unsigned long offset,
		   unsigned long part_table_addr)
     __attribute__ ((noreturn));
void chain_stage2 (unsigned long segment, unsigned long offset,
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
int get_mmap_entry (struct mmar_desc *desc, int cont);

/* Get the linear address of a ROM configuration table. Return zero,
   if fails.  */
unsigned long get_rom_config_table (void);

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
int getrtsecs (void);

/* Get current date and time */
void get_datetime(unsigned long *date, unsigned long *time);

#define currticks()	(*(unsigned long *)0x46C)

/* Clear the screen. */
void cls (void);

/* Turn on/off cursor. */
unsigned long setcursor (unsigned long on);

/* Get the current cursor position (where 0,0 is the top left hand
   corner of the screen).  Returns packed values, (RET >> 8) is x,
   (RET & 0xff) is y. */
int getxy (void);

/* Set the cursor position. */
void gotoxy (int x, int y);

/* Displays an ASCII character.  IBM displays will translate some
   characters to special graphical ones (see the DISP_* constants). */
unsigned int (*grub_putchar) (unsigned int c, unsigned int max_width);
unsigned int _putchar (unsigned int c, unsigned int max_width);
unsigned char *set_putchar_hook(unsigned char *hooked);
extern unsigned char* putchar_hooked;

/* Wait for a keypress, and return its packed BIOS/ASCII key code.
   Use ASCII_CHAR(ret) to extract the ASCII code. */
int getkey (void);

/* Like GETKEY, but doesn't block, and returns -1 if no keystroke is
   available. */
int checkkey (void);

/* Low-level disk I/O */
extern int biosdisk_int13_extensions (unsigned ax, unsigned drive, void *dap, unsigned ssize);
int get_cdinfo (unsigned long drive, struct geometry *geometry);
int get_diskinfo (unsigned long drive, struct geometry *geometry, unsigned long lba1sector);
int biosdisk (unsigned long subfunc, unsigned long drive, struct geometry *geometry,
	      unsigned long long sector, unsigned long nsec, unsigned long segment);
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

#define BAT_SIGN 0x54414221UL

/* The table for a psp_end*/
typedef struct {
	unsigned long len;
	unsigned long proglen;
	unsigned long arg;
	unsigned long path;
	char filename[0];
} __attribute__ ((packed)) psp_info_t;

/* The table for a builtin.  */
struct builtin
{
  /* The command name.  */
  char *name;
  /* The callback function.  */
  int (*func) (char *, int);
  /* The combination of the flags defined above.  */
  int flags;
  /* The short version of the documentation.  */
  char *short_doc;
  /* The long version of the documentation.  */
  char *long_doc;
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
extern unsigned char timeout_enable;
extern void timeout_refresh(void);

char *wee_skip_to (char *cmdline, int flags);
char *skip_to (int flags, char *cmdline);
#define SKIP_LINE		0x100
#define SKIP_NONE		0
#define SKIP_WITH_TERMINATE	0x200
#define ADDR_RET_STR WENV_ENVI

//extern char *pre_cmdline;
#define CMD_RUN_ON_EXIT ((char *)0x4CB08)
extern int expand_var(const char *str,char *out,const unsigned int len_max);
int run_line (char *heap,int flags);
struct builtin *find_command (char *command);
void print_cmdline_message (int forever);
void enter_cmdline (char *heap, int forever);

/* C library replacement functions with identical semantics. */
//void grub_printf (const char *format,...);

#define grub_printf(...) grub_sprintf(NULL, __VA_ARGS__)
#define printf_debug(...) grub_sprintf((char*)2, __VA_ARGS__)
#define printf_debug0(...) grub_sprintf((char*)1, __VA_ARGS__)
#define printf_errinfo(...) grub_sprintf((char*)3, __VA_ARGS__)
#define printf_warning(...) grub_sprintf((char*)2, __VA_ARGS__)
int grub_sprintf (char *buffer, const char *format, ...);
int grub_tolower (int c);
int grub_isspace (int c);
void *grub_memcpy (void *to, const void *from, unsigned int n);
void *grub_memmove (void *to, const void *from, int len);
void *grub_memset (void *start, int c, int len);
int grub_strncat (char *s1, const char *s2, int n);
char *grub_strstr (const char *s1, const char *s2);
char *grub_strtok (char *s, const char *delim);
int grub_memcmp (const char *s1, const char *s2, int n);
int grub_crc32(char *data,grub_u32_t size);
unsigned short grub_crc16(unsigned char *data, int size);
int grub_strcmp (const char *s1, const char *s2);
int strncmpx(const char *s1,const char *s2, unsigned long n, int case_insensitive);
#define strncmp(s1,s2,n) strncmpx(s1,s2,n,0)
#define strnicmp(s1,s2,n) strncmpx(s1,s2,n,1)
#define strncmpi strnicmp
int grub_strlen (const char *str);
char *grub_strcpy (char *dest, const char *src);

unsigned long long grub_memmove64(unsigned long long dst_addr, unsigned long long src_addr, unsigned long long len);
unsigned long long grub_memset64(unsigned long long dst_addr, unsigned int data, unsigned long long len);
int grub_memcmp64(unsigned long long str1addr, unsigned long long str2addr, unsigned long long len);
//void grub_memset64 (unsigned long long start, unsigned long long c, unsigned long long len);
//int grub_memcmp64 (const unsigned long long s1, const unsigned long long s2, unsigned long long n);
//void grub_memmove64 (unsigned long long to, const unsigned long long from, unsigned long long len);
int mem64 (int func, unsigned long long dest, unsigned long long src, unsigned long long len);

extern unsigned long configfile_opened;

/* misc */
void init_page (void);
void print_error (void);
char *convert_to_ascii (char *buf, int c, int lo, int hi);
extern char *prompt;
extern int echo_char;
extern int readline;
struct get_cmdline_arg
{
	unsigned char *cmdline;
	unsigned char *prompt;
	unsigned int maxlen;
	unsigned int echo_char;
	unsigned int readline;
} __attribute__ ((packed));
extern struct get_cmdline_arg get_cmdline_str;
int get_cmdline (void);
int substring (const char *s1, const char *s2, int case_insensitive);
int nul_terminate (char *str);
int get_based_digit (int c, int base);
int safe_parse_maxint_with_suffix (char **str_ptr, unsigned long long *myint_ptr, int unitshift);
#define safe_parse_maxint(str_ptr, myint_ptr) safe_parse_maxint_with_suffix(str_ptr, myint_ptr, 0)
//int safe_parse_maxint (char **str_ptr, unsigned long long *myint_ptr);
int parse_string (char *arg);
int memcheck (unsigned long long addr, unsigned long long len);
void grub_putstr (const char *str);
unsigned int grub_sleep (unsigned int seconds);

#ifndef NO_DECOMPRESSION
/* Compression support. */
struct decomp_entry
{
  char *name;
  int (*open_func) (void);
  void (*close_func) (void);
  unsigned long long (*read_func) (unsigned long long buf, unsigned long long len, unsigned long write);
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
unsigned long long gunzip_read (unsigned long long buf, unsigned long long len, unsigned long write);
int dec_lzma_open (void);
void dec_lzma_close (void);
unsigned long long dec_lzma_read (unsigned long long buf, unsigned long long len, unsigned long write);
int dec_lz4_open (void);
void dec_lz4_close (void);
unsigned long long dec_lz4_read (unsigned long long buf, unsigned long long len, unsigned long write);
int dec_vhd_open(void);
void dec_vhd_close(void);
unsigned long long dec_vhd_read(unsigned long long buf, unsigned long long len, unsigned long write);
#endif /* NO_DECOMPRESSION */

int rawread (unsigned long drive, unsigned long long sector, unsigned long byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned long write);
int devread (unsigned long long sector, unsigned long long byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned long write);
int rawwrite (unsigned long drive, unsigned long long sector, unsigned long long buf);
int devwrite (unsigned long long sector, unsigned long long sector_len, unsigned long long buf);

/* Parse a device string and initialize the global parameters. */
char *set_device (char *device);
int open_device (void);
//char *setup_part (char *filename);
int real_open_partition (int flags);
int open_partition (void);
int next_partition (void);
//int next_partition (unsigned long drive, unsigned long dest,
//		    unsigned long *partition, int *type,
//		    unsigned long *start, unsigned long *len,
//		    unsigned long *offset, int *entry,
//		    unsigned long *ext_offset, char *buf);

/* Sets device to the one represented by the SAVED_* parameters. */
//int make_saved_active (int status_only);

/* Set or clear the current root partition's hidden flag.  */
//int set_partition_hidden_flag (int hidden);

/* Open a file or directory on the active device, using GRUB's
   internal filesystem support. */
int grub_open (char *filename);
#define GRUB_READ 0xedde0d90
#define GRUB_WRITE 0x900ddeed
#define GRUB_LISTBLK   0x4B42534C
/* Read LEN bytes into BUF from the file that was opened with
   GRUB_OPEN.  If LEN is -1, read all the remaining data in the file.  */
unsigned long long grub_read (unsigned long long buf, unsigned long long len, unsigned long write);

/* Reposition a file offset.  */
//unsigned long grub_seek (unsigned long offset);

/* Close a file.  */
void grub_close (void);

/* List the contents of the directory that was opened with GRUB_OPEN,
   printing all completions. */
//int dir (char *dirname);

int set_bootdev (int hdbias);

/* Display statistics on the current active device. */
void print_fsys_type (void);

/* Print the root device information.*/
void print_root_device (char *buffer,int flag);

/* Display device and filename completions. */
void print_a_completion (char *filename, int case_insensitive);
int print_completions (int is_filename, int is_completion);

/* Copies the current partition data to the desired address. */
void copy_current_part_entry (char *buf);

void bsd_boot (kernel_t type, int bootdev, char *arg)
     __attribute__ ((noreturn));

/* Define flags for load_image here.  */
/* Don't pass a Linux's mem option automatically.  */
#define KERNEL_LOAD_NO_MEM_OPTION	(1 << 0)

kernel_t load_image (char *kernel, char *arg, kernel_t suggested_type,
		     unsigned long load_flags);

int load_module (char *module, char *arg);
int load_initrd (char *initrd);

int check_password(char* expected, password_t type);
extern int biosdisk_standard (unsigned ah, unsigned drive,
			      unsigned coff, unsigned hoff, unsigned soff,
			      unsigned nsec, unsigned segment);
void init_bios_info (void);

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
/* 1C */ unsigned long hidden_sectors __attribute__ ((packed));/* any value */
/* 20 */ unsigned long total_sectors_long __attribute__ ((packed));/* FAT32=non-zero, NTFS=0, FAT12/16=any */
/* 24 */ unsigned long sectors_per_fat32 __attribute__ ((packed));/* FAT32=non-zero, NTFS=any, FAT12/16=any */
/* 28 */ unsigned long long total_sectors_long_long __attribute__ ((packed));/* NTFS=non-zero, FAT12/16/32=any */
/* 30 */ char dummy2[0x18e];

    /* Partition Table, starting at offset 0x1BE */
/* 1BE */ struct {
	/* +00 */ unsigned char boot_indicator;
	/* +01 */ unsigned char start_head;
	/* +02 */ unsigned short start_sector_cylinder __attribute__ ((packed));
	/* +04 */ unsigned char system_indicator;
	/* +05 */ unsigned char end_head;
	/* +06 */ unsigned short end_sector_cylinder __attribute__ ((packed));
	/* +08 */ unsigned long start_lba __attribute__ ((packed));
	/* +0C */ unsigned long total_sectors __attribute__ ((packed));
	/* +10 */
    } P[4];
/* 1FE */ unsigned short boot_signature __attribute__ ((packed));/* 0xAA55 */
#if 0
	 /* This starts at offset 0x200 */
/* 200 */ unsigned long probed_total_sectors __attribute__ ((packed));
/* 204 */ unsigned long probed_heads __attribute__ ((packed));
/* 208 */ unsigned long probed_sectors_per_track __attribute__ ((packed));
/* 20C */ unsigned long probed_cylinders __attribute__ ((packed));
/* 210 */ unsigned long sectors_per_cylinder __attribute__ ((packed));
/* 214 */ char dummy3[0x0c] __attribute__ ((packed));

    /* matrix of coefficients of linear equations
     *
     *   C[n] * (H_count * S_count) + H[n] * S_count = LBA[n] - S[n] + 1
     *
     * where n = 1, 2, 3, 4, 5, 6, 7, 8
     */
	 /* This starts at offset 0x130 */
/* 220 */ long long L[9] __attribute__ ((packed)); /* L[n] == LBA[n] - S[n] + 1 */
/* 268 */ long H[9] __attribute__ ((packed));
/* 28C */ short C[9] __attribute__ ((packed));
/* 29E */ short X __attribute__ ((packed));
/* 2A0 */ short Y __attribute__ ((packed));
/* 2A2 */ short Cmax __attribute__ ((packed));
/* 2A4 */ long Hmax __attribute__ ((packed));
/* 2A8 */ unsigned long Z __attribute__ ((packed));
/* 2AC */ short Smax __attribute__ ((packed));
/* 2AE */
#endif
  };

extern unsigned long probed_total_sectors;
extern unsigned long probed_heads;
extern unsigned long probed_sectors_per_track;
extern unsigned long probed_cylinders;
extern unsigned long sectors_per_cylinder;

extern int filesystem_type;
extern unsigned long bios_id;	/* 1 for bochs, 0 for unknown. */

int probe_bpb (struct master_and_dos_boot_sector *BS);
int probe_mbr (struct master_and_dos_boot_sector *BS, unsigned long start_sector1, unsigned long sector_count1, unsigned long part_start1);

extern int check_int13_extensions (unsigned drive, unsigned lba1sector);

struct drive_parameters
{
	unsigned short size;
	unsigned short flags;
	unsigned long cylinders;
	unsigned long heads;
	unsigned long sectors;
	unsigned long long total_sectors;
	unsigned short bytes_per_sector;
	/* ver 2.0 or higher */
	unsigned long EDD_configuration_parameters;
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
#define	GPT_HDR_SIG		0x5452415020494645LL
typedef struct {
	grub_u64_t		hdr_sig;
	grub_u32_t		hdr_revision;
	grub_u32_t		hdr_size;
	grub_u32_t		hdr_crc_self;
	grub_u32_t		__reserved;
	grub_u64_t		hdr_lba_self;
	grub_u64_t		hdr_lba_alt;
	grub_u64_t		hdr_lba_start;
	grub_u64_t		hdr_lba_end;
	GUID		hdr_uuid;
	grub_u64_t		hdr_lba_table;
	grub_u32_t		hdr_entries;
	grub_u32_t		hdr_entsz;
	grub_u32_t		hdr_crc_table;
	grub_u32_t		padding;
} PACKED GPT_HDR;
typedef GPT_HDR* P_GPT_HDR;

typedef struct {
	GUID type;
	GUID uid;
	grub_u64_t starting_lba;
	grub_u64_t ending_lba;
	union{
		grub_u64_t attributes;
		struct {
			grub_u16_t unused[3];
			grub_u16_t gpt_att;
		} PACKED ms_attr;
	};
	char name[72];
} PACKED GPT_ENT;
typedef GPT_ENT* P_GPT_ENT;

int check_64bit (void);
int check_64bit_and_PAE  (void);
extern int is64bit;

// mask for is64bit bits
#define IS64BIT_PAE   1
#define IS64BIT_AMD64 2

extern int errorcheck;
extern unsigned long pxe_restart_config;

extern unsigned long saved_pxe_ip;
extern unsigned char saved_pxe_mac[6];

#ifdef FSYS_PXE

#include "pxe.h"
extern grub_u8_t pxe_mac_len, pxe_mac_type;
extern MAC_ADDR pxe_mac;
extern IP4 pxe_yip, pxe_sip, pxe_gip;
extern unsigned long pxe_keep;
extern BOOTPLAYER *discover_reply;
extern unsigned short pxe_basemem, pxe_freemem;
extern unsigned long pxe_entry;
extern unsigned long pxe_inited;
extern unsigned long pxe_scan(void);
extern int pxe_detect(int, char *);
extern void pxe_unload(void);
extern int pxe_call(int func,void* data);
#if PXE_FAST_READ
extern int pxe_fast_read(void* data,int num);
#endif
int pxe_func(char* arg,int flags);
#ifdef FSYS_IPXE
extern grub_u32_t has_ipxe;
int ipxe_func(char* arg,int flags);
void ipxe_init(void);
#endif
#else /* ! FSYS_PXE */

#define pxe_detect()

#endif /* FSYS_PXE */

extern unsigned long fb_status;
extern unsigned long next_partition_drive;
extern unsigned long next_partition_dest;
extern unsigned long *next_partition_partition;
extern unsigned long *next_partition_type;
extern unsigned long long *next_partition_start;
extern unsigned long long *next_partition_len;
extern unsigned long long *next_partition_offset;
extern unsigned long *next_partition_entry;
extern unsigned long *next_partition_ext_offset;
extern char *next_partition_buf;
extern unsigned char *IMAGE_BUFFER;
extern unsigned char *JPG_FILE;
extern unsigned char *UNIFONT_START;
extern unsigned char *narrow_mem;
#endif /* ! ASM_FILE */

#endif /* ! GRUB_SHARED_HEADER */
