/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000, 2001  Free Software Foundation, Inc.
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
 * Elements of this file were originally from the FreeBSD "biosboot"
 * bootloader file "disk.c" dated 4/12/95.
 *
 * The license and header comments from that file are included here.
 */

/*
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 *
 *	from: Mach, Revision 2.2  92/04/04  11:35:49  rpd
 *	$Id: fsys_ffs.c,v 1.10 2001/11/12 06:57:29 okuji Exp $
 */

#ifdef FSYS_FFS

#include "shared.h"

#include "filesys.h"

/*
 * The I node is the focus of all file activity in the BSD Fast File System.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 * An inode is 'named' by its dev/inumber pair. (iget/iget.c)
 * Data in icommon is read in from permanent inode on volume.
 */

#define	FFS_NDADDR	12	/* direct addresses in inode */
#define	FFS_NIADDR	3	/* indirect addresses in inode */

#define	FFS_MAX_FASTLINK_SIZE	((FFS_NDADDR + FFS_NIADDR) \
				 * sizeof (unsigned int))

struct icommon
  {
    unsigned short ic_mode;	/*  0: mode and type of file */
    short ic_nlink;		/*  2: number of links to file */
    unsigned short ic_uid;	/*  4: owner's user id */
    unsigned short ic_gid;	/*  6: owner's group id */
    unsigned long long ic_size;	/*  8: number of bytes in file */
    unsigned int ic_atime;	/* 16: time last accessed */
    int ic_atspare;
    unsigned int ic_mtime;	/* 24: time last modified */
    int ic_mtspare;
    unsigned int ic_ctime;	/* 32: last time inode changed */
    int ic_ctspare;
    union
      {
	struct
	  {
	    unsigned int Mb_db[FFS_NDADDR];	/* 40: disk block addresses */
	    unsigned int Mb_ib[FFS_NIADDR];	/* 88: indirect blocks */
	  }
	ic_Mb;
	char ic_Msymlink[FFS_MAX_FASTLINK_SIZE];
	/* 40: symbolic link name */
      }
    ic_Mun;
#define	ic_db		ic_Mun.ic_Mb.Mb_db
#define	ic_ib		ic_Mun.ic_Mb.Mb_ib
#define	ic_symlink	ic_Mun.ic_Msymlink
    int ic_flags;		/* 100: status, currently unused */
    int ic_blocks;		/* 104: blocks actually held */
    int ic_gen;			/* 108: generation number */
    int ic_spare[4];		/* 112: reserved, currently unused */
  };

/*
 *	Same structure, but on disk.
 */
struct dinode
  {
    union
      {
	struct icommon di_com;
	char di_char[128];
      }
    di_un;
  };
#define	di_ic	di_un.di_com

#define	NDADDR	FFS_NDADDR
#define	NIADDR	FFS_NIADDR

#define	MAX_FASTLINK_SIZE	FFS_MAX_FASTLINK_SIZE

#define	IC_FASTLINK	0x0001	/* Symbolic link in inode */

#define	i_mode		ic_mode
#define	i_nlink		ic_nlink
#define	i_uid		ic_uid
#define	i_gid		ic_gid
//#if	defined(BYTE_MSF) && BYTE_MSF
//#define	i_size		ic_size.val[1]
//#else /* BYTE_LSF */
//#define	i_size		ic_size.val[0]
//#endif
#define	i_db		ic_db
#define	i_ib		ic_ib
#define	i_atime		ic_atime
#define	i_mtime		ic_mtime
#define	i_ctime		ic_ctime
#define i_blocks	ic_blocks
#define	i_rdev		ic_db[0]
#define	i_symlink	ic_symlink
#define i_flags		ic_flags
#define i_gen		ic_gen

/* modes */
#define	IFMT	0xf000		/* type of file */
#define	IFCHR	0x2000		/* character special */
#define	IFDIR	0x4000		/* directory */
#define	IFBLK	0x6000		/* block special */
#define	IFREG	0x8000		/* regular */
#define	IFLNK	0xa000		/* symbolic link */
#define	IFSOCK	0xc000		/* socket */


#define	ISUID		0x0800	/* set user id on execution */
#define	ISGID		0x0400	/* set group id on execution */
#define	ISVTX		0x0200	/* save swapped text even after use */
#define	IREAD		0x0100	/* read, write, execute permissions */
#define	IWRITE		0x0080
#define	IEXEC		0x0040

#define	MAXNAMLEN	255

struct direct
  {
    unsigned int d_ino;		/* inode number of entry */
    unsigned short d_reclen;	/* length of this record */
    unsigned short d_namlen;	/* length of string in d_name */
    char d_name[MAXNAMLEN + 1];	/* name with length <= MAXNAMLEN */
  };

#define	DEV_BSIZE	512
#define BBSIZE		8192
#define SBSIZE		8192
#define	BBOFF		((unsigned int)(0))
#define	SBOFF		((unsigned int)(BBOFF + BBSIZE))
#define	BBLOCK		((unsigned int)(0))
#define	SBLOCK		((unsigned int)(BBLOCK + BBSIZE / DEV_BSIZE))

/*
 * MINBSIZE is the smallest allowable block size.
 * In order to insure that it is possible to create files of size
 * 2^32 with only two levels of indirection, MINBSIZE is set to 4096.
 * MINBSIZE must be big enough to hold a cylinder group block,
 * thus changes to (struct cg) must keep its size within MINBSIZE.
 * Note that super blocks are always of size SBSIZE,
 * and that both SBSIZE and MAXBSIZE must be >= MINBSIZE.
 */
#define MINBSIZE	4096

/*
 * The path name on which the file system is mounted is maintained
 * in fs_fsmnt. MAXMNTLEN defines the amount of space allocated in 
 * the super block for this name.
 * The limit on the amount of summary information per file system
 * is defined by MAXCSBUFS. It is currently parameterized for a
 * maximum of two million cylinders.
 */
#define MAXMNTLEN 512
#define MAXCSBUFS 32

/*
 * Per cylinder group information; summarized in blocks allocated
 * from first cylinder group data blocks.  These blocks have to be
 * read in from fs_csaddr (size fs_cssize) in addition to the
 * super block.
 *
 * N.B. sizeof(struct csum) must be a power of two in order for
 * the ``fs_cs'' macro to work (see below).
 */
struct csum
  {
    int cs_ndir;		/* number of directories */
    int cs_nbfree;		/* number of free blocks */
    int cs_nifree;		/* number of free inodes */
    int cs_nffree;		/* number of free frags */
  };

/*
 * Super block for a file system.
 */
#define	FS_MAGIC	0x011954
struct fs
  {
    int xxx1;			/* struct       fs *fs_link; */
    int xxx2;			/* struct       fs *fs_rlink; */
    unsigned int fs_sblkno;	/* addr of super-block in filesys */
    unsigned int fs_cblkno;	/* offset of cyl-block in filesys */
    unsigned int fs_iblkno;	/* offset of inode-blocks in filesys */
    unsigned int fs_dblkno;	/* offset of first data after cg */
    int fs_cgoffset;		/* cylinder group offset in cylinder */
    int fs_cgmask;		/* used to calc mod fs_ntrak */
    unsigned int fs_time;	/* last time written */
    int fs_size;		/* number of blocks in fs */
    int fs_dsize;		/* number of data blocks in fs */
    int fs_ncg;			/* number of cylinder groups */
    int fs_bsize;		/* size of basic blocks in fs */
    int fs_fsize;		/* size of frag blocks in fs */
    int fs_frag;		/* number of frags in a block in fs */
/* these are configuration parameters */
    int fs_minfree;		/* minimum percentage of free blocks */
    int fs_rotdelay;		/* num of ms for optimal next block */
    int fs_rps;			/* disk revolutions per second */
/* these fields can be computed from the others */
    int fs_bmask;		/* ``blkoff'' calc of blk offsets */
    int fs_fmask;		/* ``fragoff'' calc of frag offsets */
    int fs_bshift;		/* ``lblkno'' calc of logical blkno */
    int fs_fshift;		/* ``numfrags'' calc number of frags */
/* these are configuration parameters */
    int fs_maxcontig;		/* max number of contiguous blks */
    int fs_maxbpg;		/* max number of blks per cyl group */
/* these fields can be computed from the others */
    int fs_fragshift;		/* block to frag shift */
    int fs_fsbtodb;		/* fsbtodb and dbtofsb shift constant */
    int fs_sbsize;		/* actual size of super block */
    int fs_csmask;		/* csum block offset */
    int fs_csshift;		/* csum block number */
    int fs_nindir;		/* value of NINDIR */
    int fs_inopb;		/* value of INOPB */
    int fs_nspf;		/* value of NSPF */
/* yet another configuration parameter */
    int fs_optim;		/* optimization preference, see below */
/* these fields are derived from the hardware */
    int fs_npsect;		/* # sectors/track including spares */
    int fs_interleave;		/* hardware sector interleave */
    int fs_trackskew;		/* sector 0 skew, per track */
    int fs_headswitch;		/* head switch time, usec */
    int fs_trkseek;		/* track-to-track seek, usec */
/* sizes determined by number of cylinder groups and their sizes */
    unsigned int fs_csaddr;	/* blk addr of cyl grp summary area */
    int fs_cssize;		/* size of cyl grp summary area */
    int fs_cgsize;		/* cylinder group size */
/* these fields are derived from the hardware */
    int fs_ntrak;		/* tracks per cylinder */
    int fs_nsect;		/* sectors per track */
    int fs_spc;			/* sectors per cylinder */
/* this comes from the disk driver partitioning */
    int fs_ncyl;		/* cylinders in file system */
/* these fields can be computed from the others */
    int fs_cpg;			/* cylinders per group */
    int fs_ipg;			/* inodes per group */
    int fs_fpg;			/* blocks per group * fs_frag */
/* this data must be re-computed after crashes */
    struct csum fs_cstotal;	/* cylinder summary information */
/* these fields are cleared at mount time */
    char fs_fmod;		/* super block modified flag */
    char fs_clean;		/* file system is clean flag */
    char fs_ronly;		/* mounted read-only flag */
    char fs_flags;		/* currently unused flag */
    char fs_fsmnt[MAXMNTLEN];	/* name mounted on */
/* these fields retain the current block allocation info */
    int fs_cgrotor;		/* last cg searched */
#if 1
    int was_fs_csp[MAXCSBUFS];
#else
    struct csum *fs_csp[MAXCSBUFS];	/* list of fs_cs info buffers */
#endif
    int fs_cpc;			/* cyl per cycle in postbl */
    short fs_opostbl[16][8];	/* old rotation block list head */
    long fs_sparecon[50];	/* reserved for future constants */
    long fs_contigsumsize;	/* size of cluster summary array */
    long fs_maxsymlinklen;	/* max length of an internal symlink */
    long fs_inodefmt;		/* format of on-disk inodes */
    unsigned long long fs_maxfilesize;	/* maximum representable file size */
    unsigned long long fs_qbmask; /* ~fs_bmask - for use with quad size */
    unsigned long long fs_qfmask; /* ~fs_fmask - for use with quad size */
    long fs_state;		/* validate fs_clean field */
    int fs_postblformat;	/* format of positional layout tables */
    int fs_nrpos;		/* number of rotaional positions */
    int fs_postbloff;		/* (short) rotation block list head */
    int fs_rotbloff;		/* (u_char) blocks for each rotation */
    int fs_magic;		/* magic number */
    unsigned char fs_space[1];	/* list of blocks for each rotation */
/* actually longer */
  };
/*
 * Preference for optimization.
 */
#define FS_OPTTIME	0	/* minimize allocation time */
#define FS_OPTSPACE	1	/* minimize disk fragmentation */

/*
 * Rotational layout table format types
 */
#define FS_42POSTBLFMT		-1	/* 4.2BSD rotational table format */
#define FS_DYNAMICPOSTBLFMT	1	/* dynamic rotational table format */
/*
 * Macros for access to superblock array structures
 */
#define fs_postbl(fs, cylno) \
    (((fs)->fs_postblformat == FS_42POSTBLFMT) \
    ? ((fs)->fs_opostbl[cylno]) \
    : ((short *)((char *)(fs) + (fs)->fs_postbloff) + (cylno) * (fs)->fs_nrpos))
#define fs_rotbl(fs) \
    (((fs)->fs_postblformat == FS_42POSTBLFMT) \
    ? ((fs)->fs_space) \
    : ((unsigned char *)((char *)(fs) + (fs)->fs_rotbloff)))

/*
 * Convert cylinder group to base address of its global summary info.
 *
 * N.B. This macro assumes that sizeof(struct csum) is a power of two.
 */
#define fs_cs(fs, indx) \
	fs_csp[(indx) >> (fs)->fs_csshift][(indx) & ~(fs)->fs_csmask]

/*
 * Cylinder group block for a file system.
 */
#define	CG_MAGIC	0x090255
#define	MAXFRAG		8
struct cg
  {
    int xxx1;			/* struct       cg *cg_link; */
    int cg_magic;		/* magic number */
    unsigned int cg_time;	/* time last written */
    int cg_cgx;			/* we are the cgx'th cylinder group */
    short cg_ncyl;		/* number of cyl's this cg */
    short cg_niblk;		/* number of inode blocks this cg */
    int cg_ndblk;		/* number of data blocks this cg */
    struct csum cg_cs;		/* cylinder summary information */
    int cg_rotor;		/* position of last used block */
    int cg_frotor;		/* position of last used frag */
    int cg_irotor;		/* position of last used inode */
    int cg_frsum[MAXFRAG];	/* counts of available frags */
    int cg_btotoff;		/* (long) block totals per cylinder */
    int cg_boff;		/* (short) free block positions */
    int cg_iusedoff;		/* (char) used inode map */
    int cg_freeoff;		/* (u_char) free block map */
    int cg_nextfreeoff;		/* (u_char) next available space */
    int cg_sparecon[16];	/* reserved for future use */
    unsigned char cg_space[1];	/* space for cylinder group maps */
/* actually longer */
  };
/*
 * Macros for access to cylinder group array structures
 */
#define cg_blktot(cgp) \
    (((cgp)->cg_magic != CG_MAGIC) \
    ? (((struct ocg *)(cgp))->cg_btot) \
    : ((int *)((char *)(cgp) + (cgp)->cg_btotoff)))
#define cg_blks(fs, cgp, cylno) \
    (((cgp)->cg_magic != CG_MAGIC) \
    ? (((struct ocg *)(cgp))->cg_b[cylno]) \
    : ((short *)((char *)(cgp) + (cgp)->cg_boff) + (cylno) * (fs)->fs_nrpos))
#define cg_inosused(cgp) \
    (((cgp)->cg_magic != CG_MAGIC) \
    ? (((struct ocg *)(cgp))->cg_iused) \
    : ((char *)((char *)(cgp) + (cgp)->cg_iusedoff)))
#define cg_blksfree(cgp) \
    (((cgp)->cg_magic != CG_MAGIC) \
    ? (((struct ocg *)(cgp))->cg_free) \
    : ((unsigned char *)((char *)(cgp) + (cgp)->cg_freeoff)))
#define cg_chkmagic(cgp) \
    ((cgp)->cg_magic == CG_MAGIC || ((struct ocg *)(cgp))->cg_magic == CG_MAGIC)

/*
 * The following structure is defined
 * for compatibility with old file systems.
 */
struct ocg
  {
    int xxx1;			/* struct       ocg *cg_link; */
    int xxx2;			/* struct       ocg *cg_rlink; */
    unsigned int cg_time;	/* time last written */
    int cg_cgx;			/* we are the cgx'th cylinder group */
    short cg_ncyl;		/* number of cyl's this cg */
    short cg_niblk;		/* number of inode blocks this cg */
    int cg_ndblk;		/* number of data blocks this cg */
    struct csum cg_cs;		/* cylinder summary information */
    int cg_rotor;		/* position of last used block */
    int cg_frotor;		/* position of last used frag */
    int cg_irotor;		/* position of last used inode */
    int cg_frsum[8];		/* counts of available frags */
    int cg_btot[32];		/* block totals per cylinder */
    short cg_b[32][8];		/* positions of free blocks */
    char cg_iused[256];		/* used inode map */
    int cg_magic;		/* magic number */
    unsigned char cg_free[1];	/* free block map */
/* actually longer */
  };

/*
 * Turn file system block numbers into disk block addresses.
 * This maps file system blocks to device size blocks.
 */
#define fsbtodb(fs, b)	((b) << (fs)->fs_fsbtodb)
#define	dbtofsb(fs, b)	((b) >> (fs)->fs_fsbtodb)

/*
 * Cylinder group macros to locate things in cylinder groups.
 * They calc file system addresses of cylinder group data structures.
 */
#define	cgbase(fs, c)	((unsigned int)((fs)->fs_fpg * (c)))
#define cgstart(fs, c) \
	(cgbase(fs, c) + (fs)->fs_cgoffset * ((c) & ~((fs)->fs_cgmask)))
#define	cgsblock(fs, c)	(cgstart(fs, c) + (fs)->fs_sblkno)	/* super blk */
#define	cgtod(fs, c)	(cgstart(fs, c) + (fs)->fs_cblkno)	/* cg block */
#define	cgimin(fs, c)	(cgstart(fs, c) + (fs)->fs_iblkno)	/* inode blk */
#define	cgdmin(fs, c)	(cgstart(fs, c) + (fs)->fs_dblkno)	/* 1st data */

/*
 * Macros for handling inode numbers:
 *     inode number to file system block offset.
 *     inode number to cylinder group number.
 *     inode number to file system block address.
 */
#define	itoo(fs, x)	((x) % INOPB(fs))
#define	itog(fs, x)	((x) / (fs)->fs_ipg)
#define	itod(fs, x) \
	((unsigned int)(cgimin(fs, itog(fs, x)) + \
	(blkstofrags((fs), (((x) % (fs)->fs_ipg) / INOPB(fs))))))

/*
 * Give cylinder group number for a file system block.
 * Give cylinder group block number for a file system block.
 */
#define	dtog(fs, d)	((d) / (fs)->fs_fpg)
#define	dtogd(fs, d)	((d) % (fs)->fs_fpg)

/*
 * The following macros optimize certain frequently calculated
 * quantities by using shifts and masks in place of divisions
 * modulos and multiplications.
 */
#define blkoff(fs, loc)		/* calculates (loc % fs->fs_bsize) */ \
	((loc) & ~(fs)->fs_bmask)
#define fragoff(fs, loc)	/* calculates (loc % fs->fs_fsize) */ \
	((loc) & ~(fs)->fs_fmask)
#define lblkno(fs, loc)		/* calculates (loc / fs->fs_bsize) */ \
	((loc) >> (fs)->fs_bshift)
#define numfrags(fs, loc)	/* calculates (loc / fs->fs_fsize) */ \
	((loc) >> (fs)->fs_fshift)
#define blkroundup(fs, size)	/* calculates roundup(size, fs->fs_bsize) */ \
	(((size) + (fs)->fs_bsize - 1) & (fs)->fs_bmask)
#define fragroundup(fs, size)	/* calculates roundup(size, fs->fs_fsize) */ \
	(((size) + (fs)->fs_fsize - 1) & (fs)->fs_fmask)
#define fragstoblks(fs, frags)	/* calculates (frags / fs->fs_frag) */ \
	((frags) >> (fs)->fs_fragshift)
#define blkstofrags(fs, blks)	/* calculates (blks * fs->fs_frag) */ \
	((blks) << (fs)->fs_fragshift)
#define fragnum(fs, fsb)	/* calculates (fsb % fs->fs_frag) */ \
	((fsb) & ((fs)->fs_frag - 1))
#define blknum(fs, fsb)		/* calculates rounddown(fsb, fs->fs_frag) */ \
	((fsb) &~ ((fs)->fs_frag - 1))

/*
 * Determine the number of available frags given a
 * percentage to hold in reserve
 */
#define freespace(fs, percentreserved) \
	(blkstofrags((fs), (fs)->fs_cstotal.cs_nbfree) + \
	(fs)->fs_cstotal.cs_nffree - ((fs)->fs_dsize * (percentreserved) / 100))

/*
 * Determining the size of a file block in the file system.
 */
#define blksize(fs, ip, lbn) \
	(((lbn) >= NDADDR || (unsigned long)((ip)->ic_size) >= ((lbn) + 1) << (fs)->fs_bshift) \
	    ? (fs)->fs_bsize \
	    : (fragroundup(fs, blkoff(fs, (unsigned long)((ip)->ic_size)))))
#define dblksize(fs, dip, lbn) \
	(((lbn) >= NDADDR || (dip)->di_size >= ((lbn) + 1) << (fs)->fs_bshift) \
	    ? (fs)->fs_bsize \
	    : (fragroundup(fs, blkoff(fs, (dip)->di_size))))

/*
 * Number of disk sectors per block; assumes DEV_BSIZE byte sector size.
 */
#define	NSPB(fs)	((fs)->fs_nspf << (fs)->fs_fragshift)
#define	NSPF(fs)	((fs)->fs_nspf)

/*
 * INOPB is the number of inodes in a secondary storage block.
 */
#define	INOPB(fs)	((fs)->fs_inopb)
#define	INOPF(fs)	((fs)->fs_inopb >> (fs)->fs_fragshift)

/*
 * NINDIR is the number of indirects in a file system block.
 */
#define	NINDIR(fs)	((fs)->fs_nindir)

/* used for filesystem map blocks */
static int mapblock;
static int mapblock_offset;
static int mapblock_bsize;

/* pointer to superblock */
#define SUPERBLOCK ((struct fs *) ( FSYS_BUF + 8192 ))
#define INODE ((struct icommon *) ( FSYS_BUF + 16384 ))
#define MAPBUF ( FSYS_BUF + 24576 )
#define MAPBUF_LEN 8192
#define NAME_BUF (FSYS_BUF - 512)


int
ffs_mount (void)
{
  int retval = 1;

  if (/*(((current_drive & 0x80) || (current_slice != 0))
       && ! IS_PC_SLICE_TYPE_BSD_WITH_FS (current_slice, FS_BSDFFS))
      ||*/ (unsigned long)part_length < (SBLOCK + (SBSIZE / DEV_BSIZE))
      || ! devread (SBLOCK, 0, SBSIZE, (unsigned long long)(unsigned int)(char *) SUPERBLOCK, 0xedde0d90)
      || SUPERBLOCK->fs_magic != FS_MAGIC)
    retval = 0;

  mapblock = -1;
  mapblock_offset = -1;
  
  return retval;
}

static int
block_map (int file_block)
{
  int bnum, offset, bsize;
  
  if (file_block < NDADDR)
    return (INODE->i_db[file_block]);
  
  /* If the blockmap loaded does not include FILE_BLOCK,
     load a new blockmap.  */
  if ((bnum = fsbtodb (SUPERBLOCK, INODE->i_ib[0])) != mapblock
      || (mapblock_offset <= bnum && bnum <= mapblock_offset + mapblock_bsize))
    {
      if (MAPBUF_LEN < SUPERBLOCK->fs_bsize)
	{
	  offset = ((file_block - NDADDR) % NINDIR (SUPERBLOCK));
	  bsize = MAPBUF_LEN;
	  
	  if (offset + MAPBUF_LEN > SUPERBLOCK->fs_bsize)
	    offset = (SUPERBLOCK->fs_bsize - MAPBUF_LEN) / sizeof (int);
	}
      else
	{
	  bsize = SUPERBLOCK->fs_bsize;
	  offset = 0;
	}
      
      if (! devread (bnum, offset * sizeof (int), bsize, (unsigned long long)(unsigned int)(char *) MAPBUF, 0xedde0d90))
	{
	  mapblock = -1;
	  mapblock_bsize = -1;
	  mapblock_offset = -1;
	  errnum = ERR_FSYS_CORRUPT;
	  return -1;
	}
      
      mapblock = bnum;
      mapblock_bsize = bsize;
      mapblock_offset = offset;
    }
  
  return (((int *) MAPBUF)[((file_block - NDADDR) % NINDIR (SUPERBLOCK))
			  - mapblock_offset]);
}


unsigned long long
ffs_read (unsigned long long buf, unsigned long long len, unsigned long write)
{
  unsigned long logno, off, size, map, ret = 0;
  
  while (len && ! errnum)
    {
      off = blkoff (SUPERBLOCK, filepos);
      logno = lblkno (SUPERBLOCK, filepos);
      size = blksize (SUPERBLOCK, INODE, logno);

      if ((map = block_map (logno)) < 0)
	  break;

      size -= off;

      if (size > len)
	  size = len;

      disk_read_func = disk_read_hook;

      devread (fsbtodb (SUPERBLOCK, map), off, size, buf, write);

      disk_read_func = NULL;

      if (buf)
	buf += size;
      len -= size;	/* len always >= 0 */
      filepos += size;
      ret += size;
    }

  if (errnum)
      ret = 0;

  return ret;
}


int
ffs_dir (char *dirname)
{
  char *rest, ch;
  unsigned long block, off, loc, map, ino = 2;
  struct direct *dp;
  int j, k;
  char ch1;
  char *tmp_name = (char *)(NAME_BUF);	/* MAXNAMLEN is 255, so 512 byte buffer is needed. */

/* main loop to find destination inode */
loop:

  /* load current inode (defaults to the root inode) */

  if (! devread (fsbtodb (SUPERBLOCK, itod (SUPERBLOCK, ino)),
	ino % (SUPERBLOCK->fs_inopb) * sizeof (struct dinode),
	sizeof (struct dinode), (unsigned long long)(unsigned int)(char *) INODE, 0xedde0d90))
    return 0;			/* XXX what return value? */

  /* if we have a real file (and we're not just printing possibilities),
     then this is where we want to exit */

  if (! *dirname || isspace (*dirname))
    {
      if ((INODE->i_mode & IFMT) != IFREG)
	{
	  errnum = ERR_BAD_FILETYPE;
	  return 0;
	}

      filemax = (unsigned long)(INODE->ic_size);

      /* incomplete implementation requires this! */
      fsmax = (NDADDR + NINDIR (SUPERBLOCK)) * SUPERBLOCK->fs_bsize;
      return 1;
    }

  /* continue with file/directory name interpretation */

  while (*dirname == '/')
    dirname++;

  if (! ((unsigned long)(INODE->ic_size)) || ((INODE->i_mode & IFMT) != IFDIR))
    {
      errnum = ERR_BAD_FILETYPE;
      return 0;
    }

  //for (rest = dirname; (ch = *rest) && !isspace (ch) && ch != '/'; rest++);
  for (rest = dirname; (ch = *rest) /*&& !isspace (ch)*/ && ch != '/'; rest++)
  {
#if 0
	if (ch == '\\')
	{
		rest++;
		if (! (ch = *rest))
			break;
	}
#endif
  }

  *rest = 0;
  loc = 0;

  /* loop for reading a the entries in a directory */

  do
    {
      if (loc >= (unsigned long)(INODE->ic_size))
	{
#if 0
	  putchar ('\n');
#endif

	  if (print_possibilities < 0)
	    return 1;

	  errnum = ERR_FILE_NOT_FOUND;
	  *rest = ch;
	  return 0;
	}

      if (! (off = blkoff (SUPERBLOCK, loc)))
	{
	  block = lblkno (SUPERBLOCK, loc);

	  if ((map = block_map (block)) < 0
	      || ! devread (fsbtodb (SUPERBLOCK, map), 0,
			   blksize (SUPERBLOCK, INODE, block),
			   (unsigned long long)(unsigned int)(char *) FSYS_BUF, 0xedde0d90))
	    {
	      errnum = ERR_FSYS_CORRUPT;
	      *rest = ch;
	      return 0;
	    }
	}

      dp = (struct direct *) (FSYS_BUF + off);
      loc += dp->d_reclen;

	/* copy dp->name to tmp_name, and quote the spaces with a '\\' */
	for (j = 0, k = 0; j < dp->d_namlen; j++)
	{
		if (! (ch1 = dp->d_name[j]))
			break;
#if 0
		if (ch1 == ' ')
			tmp_name[k++] = '\\';
#endif
		tmp_name[k++] = ch1;
	}
	tmp_name[k] = 0;

      if (dp->d_ino && print_possibilities && ch != '/'
	  && (!*dirname || substring (dirname, tmp_name, 0) <= 0))
	{
	  if (print_possibilities > 0)
	    print_possibilities = -print_possibilities;

	  print_a_completion (tmp_name, 0);
	}
    }
  while (!dp->d_ino || (substring (dirname, dp->d_name, 0) != 0
			|| (print_possibilities && ch != '/')));

  /* only get here if we have a matching directory entry */

  ino = dp->d_ino;
  *(dirname = rest) = ch;

  /* go back to main loop at top of function */
  goto loop;
}

unsigned long
ffs_embed (unsigned long *start_sector, unsigned long needed_sectors)
{
  /* XXX: I don't know if this is really correct. Someone who is
     familiar with BSD should check for this.  */
  if (needed_sectors > 14)
    return 0;
  
  *start_sector = 1;
#if 1
  /* FIXME: Disable the embedding in FFS until someone checks if
     the code above is correct.  */
  return 0;
#else
  return 1;
#endif
}

#endif /* FSYS_FFS */
