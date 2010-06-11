/*
 * The C code for a grub4dos executable may have defines as follows:
 * 用于编写外部命令的函数定义。
*/
#ifndef GRUB4DOS_2010_03_01
#define GRUB4DOS_2010_03_01
int grub_main (char *arg,int flags);
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

  MAX_ERR_NUM
} grub_error_t;

#define install_partition (*(unsigned long *)0x8208)
#define boot_drive (*(unsigned long *)0x8280)
#define pxe_yip (*(unsigned long *)0x8284)
#define pxe_sip (*(unsigned long *)0x8288)
#define pxe_gip (*(unsigned long *)0x828C)
#define filesize (*(unsigned long long *)0x8290)
#define saved_mem_upper (*(unsigned long *)0x8298)
#define saved_partition (*(unsigned long *)0x829C)
#define saved_drive (*(unsigned long *)0x82A0)
#define no_decompression (*(unsigned long *)0x82A4)
#define part_start (*(unsigned long long *)0x82A8)
#define part_length (*(unsigned long long *)0x82B0)
#define fb_status (*(unsigned long *)0x82B8)
#define is64bit (*(unsigned long *)0x82BC)
#define saved_mem_higher (*(unsigned long long *)0x82C0)
#define cdrom_drive (*(unsigned long *)0x82C8)
#define ram_drive (*(unsigned long *)0x82CC)
#define rd_base (*(unsigned long long *)0x82D0)
#define rd_size (*(unsigned long long *)0x82D8)
#define addr_system_functions (*(unsigned long *)0x8300)
#define next_partition_drive		((*(unsigned long **)0x8304)[0])
#define next_partition_dest		((*(unsigned long **)0x8304)[1])
#define next_partition_partition	((*(unsigned long ***)0x8304)[2])
#define next_partition_type		((*(unsigned long ***)0x8304)[3])
#define next_partition_start		((*(unsigned long ***)0x8304)[4])
#define next_partition_len		((*(unsigned long ***)0x8304)[5])
#define next_partition_offset		((*(unsigned long ***)0x8304)[6])
#define next_partition_entry		((*(unsigned long ***)0x8304)[7])
#define next_partition_ext_offset	((*(unsigned long ***)0x8304)[8])
#define next_partition_buf		((*(char ***)0x8304)[9])
#define quit_print		((*(int **)0x8304)[10])
//#define buf_drive	((*(int **)0x8304)[11])
//#define buf_track	((*(int **)0x8304)[12])
#define filesystem_type ((*(int **)0x8304)[13])
//#define query_block_entries ((*(long **)0x8304)[14])
//#define map_start_sector ((*(unsigned long **)0x8304)[15])
#define buf_geom ((*(struct geometry ***)0x8304)[16])
#define tmp_geom ((*(struct geometry ***)0x8304)[17])
#define term_table ((*(struct term_entry ***)0x8304)[18])
#define current_term ((*(struct term_entry ***)0x8304)[19])
#define fsys_table ((*(struct fsys_entry ***)0x8304)[20])
//#define fsys_type ((*(int **)0x8304)[21])
//#define NUM_FSYS ((*(const int **)0x8304)[22])

#define graphics_inited ((*(const int **)0x8304)[23])
#define VARIABLE_GRAPHICS ((char *)(*(int ***)0x8304)[24])
#define font8x16 ((unsigned char *)(*(int ***)0x8304)[25])
#define fontx ((*(int **)0x8304)[26])
#define fonty ((*(int **)0x8304)[27])
#define graphics_CURSOR ((*(int **)0x8304)[28])
#define menu_broder ((*(struct broder ***)0x8304)[29])

#define cursorX (*(short *)(VARIABLE_GRAPHICS))
#define cursorY (*(short *)(VARIABLE_GRAPHICS + 2))
#define cursorBuf ((char *)(VARIABLE_GRAPHICS + 6))

#define free_mem_start (*(unsigned long *)0x82F0)
#define free_mem_end (*(unsigned long *)0x82F4)
#define saved_mmap_addr (*(unsigned long *)0x82F8)
#define saved_mmap_length (*(unsigned long *)0x82FB)

#define DRIVE_MAP_SIZE	8
#define PXE_DRIVE		0x21
#define INITRD_DRIVE	0x22
#define FB_DRIVE		0x23
#define SECTOR_SIZE		0x200
#define SECTOR_BITS		9
#define BIOSDISK_FLAG_BIFURCATE		0x4
#define MB_ARD_MEMORY	1
#define MB_INFO_MEM_MAP	0x00000040

#define errnum (*(grub_error_t *)0x8314)
#define current_drive (*(unsigned long *)0x8318)
#define current_partition (*(unsigned long *)0x831C)
#define filemax (*(unsigned long long *)0x8320)
#define filepos (*(unsigned long long *)0x8328)
#define debug (*(int *)0x8330)
#define current_slice (*(unsigned long *)0x8334)

#define GRUB_READ 0xedde0d90
#define GRUB_WRITE 0x900ddeed

#define sprintf ((int (*)(char *, const char *, ...))((*(int **)0x8300)[0]))
#define printf(...) sprintf(NULL, __VA_ARGS__)
#define putstr ((void (*)(const char *))((*(int **)0x8300)[1]))
#define putchar ((void (*)(int))((*(int **)0x8300)[2]))
#define get_cmdline ((int (*)(struct get_cmdline_arg))((*(int **)0x8300)[3]))
#define getxy ((int (*)(void))((*(int **)0x8300)[4]))
#define gotoxy ((void (*)(int, int))((*(int **)0x8300)[5]))
#define cls ((void (*)(void))((*(int **)0x8300)[6]))
#define setcursor ((int (*)(int))((*(int **)0x8300)[7]))
#define nul_terminate ((int (*)(char *))((*(int **)0x8300)[8]))
#define safe_parse_maxint_with_suffix ((int (*)(char **str_ptr, unsigned long long *myint_ptr, int unitshift))((*(int **)0x8300)[9]))
#define safe_parse_maxint(str_ptr, myint_ptr) safe_parse_maxint_with_suffix(str_ptr, myint_ptr, 0)
#define substring ((int (*)(const char *s1, const char *s2, int case_insensitive))((*(int **)0x8300)[10]))
#define strstr ((char *(*)(const char *s1, const char *s2))((*(int **)0x8300)[11]))
#define strlen ((int (*)(const char *str))((*(int **)0x8300)[12]))
#define strtok ((char *(*)(char *s, const char *delim))((*(int **)0x8300)[13]))
#define strncat ((int (*)(char *s1, const char *s2, int n))((*(int **)0x8300)[14]))
#define strcmp ((int (*)(const char *s1, const char *s2))((*(int **)0x8300)[15]))
#define strcpy ((char *(*)(char *dest, const char *src))((*(int **)0x8300)[16]))
#define tolower ((int (*)(int))((*(int **)0x8300)[17]))
#define isspace ((int (*)(int))((*(int **)0x8300)[18]))
#define getkey ((int (*)(void))((*(int **)0x8300)[19]))
#define checkkey ((int (*)(void))((*(int **)0x8300)[20]))
#define sleep ((unsigned int (*)(unsigned int))((*(int **)0x8300)[21]))
#define memcmp ((int (*)(const char *s1, const char *s2, int n))((*(int **)0x8300)[22]))
#define memmove ((void *(*)(void *to, const void *from, int len))((*(int **)0x8300)[23]))
#define memset ((void *(*)(void *start, int c, int len))((*(int **)0x8300)[24]))
#define mem64 ((int (*)(int, unsigned long long, unsigned long long, unsigned long long))((*(int **)0x8300)[25]))
#define open ((int (*)(char *))((*(int **)0x8300)[26]))
#define read ((unsigned long long (*)(unsigned long long, unsigned long long, unsigned long))((*(int **)0x8300)[27]))
#define close ((void (*)(void))((*(int **)0x8300)[28]))
#define unicode_to_utf8 ((void (*)(unsigned short *, unsigned char *, unsigned long))((*(int **)0x8300)[29]))
/*
int
rawread (unsigned long drive, unsigned long sector, unsigned long byte_offset, unsigned long byte_len, unsigned long long buf, unsigned long write)
*/
#define rawread ((int (*)(unsigned long, unsigned long, unsigned long, unsigned long long, unsigned long long, unsigned long))((*(int **)0x8300)[30]))
/*
int
rawwrite (unsigned long drive, unsigned long sector, char *buf)
*/
#define rawwrite ((int (*)(unsigned long, unsigned long, char *))((*(int **)0x8300)[31]))
/*
int
devread (unsigned long drive, unsigned long sector, unsigned long byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned long write)
*/
#define devread ((int (*)(unsigned long sector, unsigned long byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned long write))((*(int **)0x8300)[32]))
/*
 * int
 * devwrite (unsigned long sector, unsigned long sector_count, char *buf)
 */
#define devwrite ((int (*)(unsigned long, unsigned long, char *))((*(int **)0x8300)[33]))
#define next_partition ((int (*)(void))((*(int **)0x8300)[34]))
#define open_device ((int (*)(void))((*(int **)0x8300)[35]))
#define real_open_partition ((int (*)(int))((*(int **)0x8300)[36]))
#define set_device ((char *(*)(char *))((*(int **)0x8300)[37]))
#define dir ((int (*)(char *))((*(int **)0x8300)[38]))
#define print_a_completion ((void (*)(char *))((*(int **)0x8300)[39]))
#define print_completions ((int (*)(int, int))((*(int **)0x8300)[40]))
#define parse_string ((int (*)(char *))((*(int **)0x8300)[41]))
#define hexdump ((void (*)(unsigned long, char *, int))((*(int **)0x8300)[42]))
#define skip_to ((char *(*)(int after_equal, char *cmdline))((*(int **)0x8300)[43]))
#define builtin_cmd ((int (*)(char *cmd, char *arg, int flags))((*(int **)0x8300)[44]))
#define get_datetime ((void (*)(unsigned long *date, unsigned long *time))((*(int **)0x8300)[45]))
#define lba_to_chs ((void (*)(unsigned long lba, unsigned long *cl, unsigned long *ch, unsigned long *dh))((*(int **)0x8300)[46]))
#define probe_bpb ((int (*)(struct master_and_dos_boot_sector *BS))((*(int **)0x8300)[47]))
#define probe_mbr ((int (*)(struct master_and_dos_boot_sector *BS, unsigned long start_sector1, unsigned long sector_count1, unsigned long part_start1))((*(int **)0x8300)[48]))
#define graphics_get_font ((unsigned char *(*)(void))((*(int **)0x8300)[51]))


#define RAW_ADDR(x) (x)
#define SCRATCHADDR  RAW_ADDR (0x37e00)
#define MBR ((char *)0x8000)
//#define grub_memcmp memcmp
struct get_cmdline_arg
{
	char *cmdline;
	char *prompt;
	int maxlen;
	int echo_char;
	int readline;
} __attribute__ ((packed));

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

/* fsys.h */
struct fsys_entry
{
  char *name;
  int (*mount_func) (void);
  unsigned long (*read_func) (unsigned long long buf, unsigned long long len, unsigned long write);
  int (*dir_func) (char *dirname);
  void (*close_func) (void);
  unsigned long (*embed_func) (unsigned long *start_sector, unsigned long needed_sectors);
};
/* fsys.h */

/* shared.h */
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

	unsigned char from_drive;
	unsigned char to_drive;		/* 0xFF indicates a memdrive */
	unsigned char max_head;
	unsigned char max_sector;	/* bit 7: read only */
					/* bit 6: disable lba */

	unsigned short to_cylinder;	/* max cylinder of the TO drive */
					/* bit 15:  TO  drive support LBA */
					/* bit 14:  TO  drive is CDROM(with big 2048-byte sector) */
					/* bit 13: FROM drive is CDROM(with big 2048-byte sector) */

	unsigned char to_head;		/* max head of the TO drive */
	unsigned char to_sector;	/* max sector of the TO drive */
					/* bit 7: in-situ */
					/* bit 6: fake-write or safe-boot */

	unsigned long long start_sector;
	//unsigned long start_sector_hi;	/* hi dword of the 64-bit value */
	unsigned long long sector_count;
	//unsigned long sector_count_hi;	/* hi dword of the 64-bit value */
};


 /* shared.h */

typedef enum
{
	COLOR_STATE_STANDARD,
	/* represents the user defined colors for normal text */
	COLOR_STATE_NORMAL,
	/* represents the user defined colors for highlighted text */
	COLOR_STATE_HIGHLIGHT,
	/* represents the user defined colors for help text */
	COLOR_STATE_HELPTEXT,
	/* represents the user defined colors for heading line */
	COLOR_STATE_HEADING
} color_state;

struct term_entry
{
  /* The name of a terminal.  */
  const char *name;
  /* The feature flags defined above.  */
  unsigned long flags;
  /* Default for maximum number of lines if not specified */
  unsigned short max_lines;
  /* Put a character.  */
  void (*PUTCHAR) (int c);
  /* Check if any input character is available.  */
  int (*CHECKKEY) (void);
  /* Get a character.  */
  int (*GETKEY) (void);
  /* Get the cursor position. The return value is ((X << 8) | Y).  */
  int (*GETXY) (void);
  /* Go to the position (X, Y).  */
  void (*GOTOXY) (int x, int y);
  /* Clear the screen.  */
  void (*CLS) (void);
  /* Set the current color to be used */
  void (*SETCOLORSTATE) (color_state state);
  /* Set the normal color and the highlight color. The format of each
     color is VGA's.  */
  void (*SETCOLOR) (int normal_color, int highlight_color, int helptext_color, int heading_color);
  /* Turn on/off the cursor.  */
  int (*SETCURSOR) (int on);

  /* function to start a terminal */
  int (*STARTUP) (void);
  /* function to use to shutdown a terminal */
  void (*SHUTDOWN) (void);
};
#endif