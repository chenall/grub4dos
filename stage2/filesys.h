/* filesys.h - abstract filesystem interface */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2004  Free Software Foundation, Inc.
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

#ifndef ASM_FILE
#include "pc_slice.h"
#endif

#ifdef FSYS_FFS
#define FSYS_FFS_NUM 1
#ifndef ASM_FILE
int ffs_mount (void);
unsigned long long ffs_read (unsigned long long buf, unsigned long long len, unsigned long write);
int ffs_dir (char *dirname);
unsigned long ffs_embed (unsigned long *start_sector, unsigned long needed_sectors);
#endif
#else
#define FSYS_FFS_NUM 0
#endif

#ifdef FSYS_UFS2
#define FSYS_UFS2_NUM 1
#ifndef ASM_FILE
int ufs2_mount (void);
unsigned long long ufs2_read (unsigned long long buf, unsigned long long len, unsigned long write);
int ufs2_dir (char *dirname);
unsigned long ufs2_embed (unsigned long *start_sector, unsigned long needed_sectors);
#endif
#else
#define FSYS_UFS2_NUM 0
#endif

#ifdef FSYS_FAT
#define FSYS_FAT_NUM 1
#ifndef ASM_FILE
int fat_mount (void);
unsigned long long fat_read (unsigned long long buf, unsigned long long len, unsigned long write);
int fat_dir (char *dirname);
#endif
#else
#define FSYS_FAT_NUM 0
#endif

#ifdef FSYS_NTFS
#define FSYS_NTFS_NUM 1
#ifndef ASM_FILE
int ntfs_mount (void);
unsigned long long ntfs_read (unsigned long long buf, unsigned long long len, unsigned long write);
int ntfs_dir (char *dirname);
#endif
#else
#define FSYS_NTFS_NUM 0
#endif

#ifdef FSYS_EXT2FS
#define FSYS_EXT2FS_NUM 1
#ifndef ASM_FILE
int ext2fs_mount (void);
unsigned long long ext2fs_read (unsigned long long buf, unsigned long long len, unsigned long write);
int ext2fs_dir (char *dirname);
#endif
#else
#define FSYS_EXT2FS_NUM 0
#endif

#ifdef FSYS_MINIX
#define FSYS_MINIX_NUM 1
#ifndef ASM_FILE
int minix_mount (void);
unsigned long long minix_read (unsigned long long buf, unsigned long long len, unsigned long write);
int minix_dir (char *dirname);
#endif
#else
#define FSYS_MINIX_NUM 0
#endif

#ifdef FSYS_REISERFS
#define FSYS_REISERFS_NUM 1
#ifndef ASM_FILE
int reiserfs_mount (void);
unsigned long long reiserfs_read (unsigned long long buf, unsigned long long len, unsigned long write);
int reiserfs_dir (char *dirname);
unsigned long reiserfs_embed (unsigned long *start_sector, unsigned long needed_sectors);
#endif
#else
#define FSYS_REISERFS_NUM 0
#endif

#ifdef FSYS_VSTAFS
#define FSYS_VSTAFS_NUM 1
#ifndef ASM_FILE
int vstafs_mount (void);
unsigned long long vstafs_read (unsigned long long buf, unsigned long long len, unsigned long write);
int vstafs_dir (char *dirname);
#endif
#else
#define FSYS_VSTAFS_NUM 0
#endif

#ifdef FSYS_JFS
#define FSYS_JFS_NUM 1
#ifndef ASM_FILE
int jfs_mount (void);
unsigned long long jfs_read (unsigned long long buf, unsigned long long len, unsigned long write);
int jfs_dir (char *dirname);
unsigned long jfs_embed (unsigned long *start_sector, unsigned long needed_sectors);
#endif
#else
#define FSYS_JFS_NUM 0
#endif

#ifdef FSYS_XFS
#define FSYS_XFS_NUM 1
#ifndef ASM_FILE
int xfs_mount (void);
unsigned long long xfs_read (unsigned long long buf, unsigned long long len, unsigned long write);
int xfs_dir (char *dirname);
#endif
#else
#define FSYS_XFS_NUM 0
#endif

#ifdef FSYS_TFTP
#define FSYS_TFTP_NUM 1
#ifndef ASM_FILE
int tftp_mount (void);
unsigned long long tftp_read (unsigned long long buf, unsigned long long len, unsigned long write);
int tftp_dir (char *dirname);
void tftp_close (void);
#endif
#else
#define FSYS_TFTP_NUM 0
#endif

#ifdef FSYS_ISO9660
#define FSYS_ISO9660_NUM 1
#ifndef ASM_FILE
int iso9660_mount (void);
unsigned long long iso9660_read (unsigned long long buf, unsigned long long len, unsigned long write);
int iso9660_dir (char *dirname);
#endif
#else
#define FSYS_ISO9660_NUM 0
#endif

#ifdef FSYS_PXE
#define FSYS_PXE_NUM 1
#ifndef ASM_FILE
int pxe_mount (void);
unsigned long long pxe_read (unsigned long long buf, unsigned long long len, unsigned long write);
int pxe_dir (char *dirname);
void pxe_close (void);
#endif
#else
#define FSYS_PXE_NUM 0
#endif

#ifdef FSYS_INITRD
#define FSYS_INITRD_NUM 1
#ifndef ASM_FILE
int initrdfs_mount (void);
unsigned long long initrdfs_read (unsigned long long buf, unsigned long long len, unsigned long write);
int initrdfs_dir (char *dirname);
void initrdfs_close (void);
#endif
#else
#define FSYS_INITRD_NUM 0
#endif

#ifdef FSYS_FB
#define FSYS_FB_NUM 1
#ifndef ASM_FILE
int fb_mount (void);
unsigned long long fb_read (unsigned long long buf, unsigned long long len, unsigned long write);
int fb_dir (char *dirname);
#endif
#else
#define FSYS_FB_NUM 0
#endif

#ifndef NUM_FSYS
#define NUM_FSYS	\
  (FSYS_FFS_NUM + FSYS_FAT_NUM + FSYS_NTFS_NUM + FSYS_EXT2FS_NUM + FSYS_MINIX_NUM	\
   + FSYS_REISERFS_NUM + FSYS_VSTAFS_NUM + FSYS_JFS_NUM + FSYS_XFS_NUM	\
   + FSYS_TFTP_NUM + FSYS_ISO9660_NUM + FSYS_UFS2_NUM + FSYS_PXE_NUM + FSYS_FB_NUM + FSYS_INITRD_NUM)
#endif

#ifndef ASM_FILE
/* defines for the block filesystem info area */
#ifndef NO_BLOCK_FILES
#define BLK_CUR_FILEPOS      (*((unsigned long*)FSYS_BUF))
#define BLK_CUR_BLKLIST      (*((unsigned long*)(FSYS_BUF+4)))
#define BLK_CUR_BLKNUM       (*((unsigned long*)(FSYS_BUF+8)))
#define BLK_MAX_ADDR         (FSYS_BUF+0x77F9)
#define BLK_BLKSTART(l)      (*((unsigned long*)l))
#define BLK_BLKLENGTH(l)     (*((unsigned long*)(l+4)))
#define BLK_BLKLIST_START    (FSYS_BUF+12)
#define BLK_BLKLIST_INC_VAL  8
#endif /* NO_BLOCK_FILES */

/* this next part is pretty ugly, but it keeps it in one place! */

struct fsys_entry
{
  char *name;
  int (*mount_func) (void);
  unsigned long long (*read_func) (unsigned long long buf, unsigned long long len, unsigned long write);
  int (*dir_func) (char *dirname);
  void (*close_func) (void);
  unsigned long (*embed_func) (unsigned long *start_sector, unsigned long needed_sectors);
};

extern int print_possibilities;

extern unsigned long long fsmax;
extern struct fsys_entry fsys_table[NUM_FSYS + 1];
#endif
