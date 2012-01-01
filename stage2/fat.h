/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2001  Free Software Foundation, Inc.
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
 *  Defines for the FAT BIOS Parameter Block (embedded in the first block
 *  of the partition.
 */

typedef __signed__ char __s8;
typedef unsigned char __u8;
typedef __signed__ short __s16;
typedef unsigned short __u16;
typedef __signed__ int __s32;
typedef unsigned int __u32;
typedef __signed__ long long __s64;
typedef unsigned long long __u64;

/* Note that some shorts are not aligned, and must therefore
 * be declared as array of two bytes.
 */
struct fat_bpb {
	__s8	ignored[3];	/* Boot strap short or near jump */
	__s8	system_id[8];	/* Name - can be used to special case
				   partition manager volumes */
	__u8	bytes_per_sect[2];	/* bytes per logical sector */
	__u8	sects_per_clust;/* sectors/cluster */
	__u8	reserved_sects[2];	/* reserved sectors */
	__u8	num_fats;	/* number of FATs */
	__u8	dir_entries[2];	/* root directory entries */
	__u8	short_sectors[2];	/* number of sectors */
	__u8	media;		/* media code (unused) */
	__u16	fat_length;	/* sectors/FAT */
	__u16	secs_track;	/* sectors per track */
	__u16	heads;		/* number of heads */
	__u32	hidden;		/* hidden sectors (unused) */
	__u32	long_sectors;	/* number of sectors (if short_sectors == 0) */

	/* The following fields are only used by FAT32 */
	__u32	fat32_length;	/* sectors/FAT */
	__u16	flags;		/* bit 8: fat mirroring, low 4: active fat */
	__u8	version[2];	/* major, minor filesystem version */
	__u32	root_cluster;	/* first cluster in root directory */
	__u16	info_sector;	/* filesystem info sector */
	__u16	backup_boot;	/* backup boot sector */
	__u16	reserved2[6];	/* Unused */

	/* The following fields are only used by exFAT */
	__u64	sector_start;		/* 0x40 partition first sector */
	__u64	sector_count;		/* 0x48 partition sectors count */
	__u32	fat_sector_start;	/* 0x50 FAT first sector */
	__u32	fat_sector_count;	/* 0x54 FAT sectors count */
	__u32	cluster_sector_start;	/* 0x58 first cluster sector */
	__u32	cluster_count;		/* 0x5C total clusters count */
	__u32	rootdir_cluster;	/* 0x60 first cluster of root dir */
	__u32	volume_serial;		/* 0x64 volume serial number */
	__u8	fs_version[2];		/* 0x68 FS version */
	__u16	volume_state;		/* 0x6A volume state flags */
	__u8	sector_bits;		/* 0x6C sector size as (1 << n) */
	__u8	spc_bits;	/* 0x6D sectors per cluster as (1 << n) */
	__u8 	fat_count;	/* 0x6E nearly always 1, 2 for TexFAT */

//	__u8 drive_no;		/* 0x6F always 0x80 */
//	__u8 allocated_percent;	/* 0x70 percentage of allocated space */
//	__u8 __unused2[397];	/* 0x71 always 0 */
//	__u16 boot_signature;	/* 0x1FE the value of 0xAA55 */ 
} __attribute__ ((packed));

#define FAT_CVT_U16(bytarr) (* (__u16*)(bytarr))

/*
 *  Defines how to differentiate a 12-bit and 16-bit FAT.
 */

#define FAT_MAX_12BIT_CLUST       4087	/* 4085 + 2 */

/*
 *  Defines for the file "attribute" byte
 */

#define FAT_ATTRIB_OK_MASK        0x37
#define FAT_ATTRIB_NOT_OK_MASK    0xC8
#define FAT_ATTRIB_DIR            0x10
#define FAT_ATTRIB_LONGNAME       0x0F

/*
 *  Defines for FAT directory entries
 */

#define FAT_DIRENTRY_LENGTH       32

#define FAT_DIRENTRY_ATTRIB(entry)	(*(unsigned char *)(entry+11))
#define FAT_DIRENTRY_VALID(entry) \
  ( ((*((unsigned char *) entry)) != 0) \
    && ((*((unsigned char *) entry)) != 0xE5) \
    && !(FAT_DIRENTRY_ATTRIB(entry) & FAT_ATTRIB_NOT_OK_MASK) )
#define FAT_DIRENTRY_FIRST_CLUSTER(entry) \
  ((*(unsigned short *)(entry+26))+((*(unsigned short *)(entry+20)) << 16))
#define FAT_DIRENTRY_FILELENGTH(entry)	(*(unsigned long *)(entry+28))

#define FAT_LONGDIR_ID(entry)	(*(unsigned char *)(entry))
#define FAT_LONGDIR_ALIASCHECKSUM(entry)	(*(unsigned char *)(entry+13))

//#define EXFAT_FIRST_DATA_CLUSTER 2
//#define EXFAT_CLUSTER_FREE         0 /* free cluster */
//#define EXFAT_CLUSTER_BAD 0xfffffff7 /* cluster contains bad sector */
#define EXFAT_CLUSTER_END 0xffffffff /* final cluster of file or directory */


#define EXFAT_DIRENTRY_ATTRIB(entry)	(*(unsigned char *)(entry))

#define EXFAT_ENTRY_VALID     0x80
#define EXFAT_ENTRY_CONTINUED 0x40

#define EXFAT_ENTRY_EOD       (0)
#define EXFAT_ENTRY_BITMAP    (1 | EXFAT_ENTRY_VALID)
#define EXFAT_ENTRY_UPCASE    (2 | EXFAT_ENTRY_VALID)
#define EXFAT_ENTRY_LABEL     (3 | EXFAT_ENTRY_VALID)
#define EXFAT_ENTRY_FILE      (5 | EXFAT_ENTRY_VALID)
#define EXFAT_ENTRY_FILE_INFO (0 | EXFAT_ENTRY_VALID | EXFAT_ENTRY_CONTINUED)
#define EXFAT_ENTRY_FILE_NAME (1 | EXFAT_ENTRY_VALID | EXFAT_ENTRY_CONTINUED)

#define EXFAT_FLAG_ALWAYS1	(1u << 0)
#define EXFAT_FLAG_CONTIGUOUS	(1u << 1)

