/* disk_io.c - implement abstract BIOS disk input and output */
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


#include <shared.h>
#include <filesys.h>
#include <iso9660.h>
#include "iamath.h"

/* function declaration */
unsigned long long
gunzip_read_func (unsigned long long buf, unsigned long long len, unsigned long write);
unsigned long long
block_read_func (unsigned long long buf, unsigned long long len, unsigned long write);

/* instrumentation variables */
//void (*disk_read_hook) (unsigned long long, unsigned long, unsigned long long) = NULL;
void (*disk_read_func) (unsigned long long, unsigned long, unsigned long long) = NULL;

/* Forward declarations.  */
static int next_bsd_partition (void);
static int next_pc_slice (void);
static int next_gpt_slice(void);
static char open_filename[512];
static unsigned long relative_path;

int print_possibilities;

/* patch about cur_part_start and cur_part_entry was suggested by Icecube:
 *
 * http://www.boot-land.net/forums/index.php?showtopic=10262
 *
 * with details:
 *
 * Grub4dos can't chainload my second logical partition (EXTLINUX installed)
 * correctly:
 *
 *	title Boot Extlinux
 *	root (hd0,5)
 *	chainloader (hd0,5)+1
 *
 * EXTLINUX (and SYSLINUX) trust the info from the loader that loads the boot
 * sector of the partition, not the partition offset stored on the partition
 * itself.
 *
 * A patch for Grub Legacy can be found:
 *
 * Logical-partition-residing bootloader chainload patch for GRUB Legacy
 * http://bugs.gentoo.org/230905
 * http://forums.gentoo.org/viewtopic.php?p=5142693#5142693
 *
 */

static int unique;
static char *unique_string;
static unsigned long long cur_part_offset;
static unsigned long cur_part_addr;
static unsigned long long cur_part_start;
static unsigned long cur_part_entry;

static int do_completion;
static int set_filename(char *filename);
int dir (char *dirname);
static int sane_partition (void);
unsigned long long md_part_size;
unsigned long long md_part_base;

/* XX used for device completion in 'set_device' and 'print_completions' */
static int incomplete, disk_choice;
static enum
{
  PART_UNSPECIFIED = 0,
  PART_DISK,
  PART_CHOSEN,
}
part_choice;


//unsigned long i;

/* The first sector of stage2 can be reused as a tmp buffer.
 * Do NOT write more than 512 bytes to this buffer!
 * The stage2-body, i.e., the pre_stage2, starts at 0x8200!
 * Do NOT overwrite the pre_stage2 code at 0x8200!
 */
char *mbr = (char *)0x8000; /* 512-byte buffer for any use. */

static unsigned long dest_partition;
static unsigned long long part_offset;
static unsigned long entry;
static unsigned long ext_offset;

static unsigned long bsd_part_no;
static unsigned long pc_slice_no;

unsigned long long fsmax;
struct fsys_entry fsys_table[NUM_FSYS + 1] =
{
  /* TFTP should come first because others don't handle net device.  */
# ifdef FSYS_PXE
  {"pxe", pxe_mount, pxe_read, pxe_dir, pxe_close, 0},
# endif
# ifdef FSYS_TFTP
  {"tftp", tftp_mount, tftp_read, tftp_dir, tftp_close, 0},
# endif
# ifdef FSYS_FB
  {"fb", fb_mount, fb_read, fb_dir, 0, 0},
#endif
# ifdef FSYS_EXT2FS
  {"ext2fs", ext2fs_mount, ext2fs_read, ext2fs_dir, 0, 0},
# endif
# ifdef FSYS_FAT
  {"fat", fat_mount, fat_read, fat_dir, 0, 0},
# endif
# ifdef FSYS_NTFS
  {"ntfs", ntfs_mount, ntfs_read, ntfs_dir, 0, 0},
# endif
//# ifdef FSYS_MINIX
//{"minix", minix_mount, minix_read, minix_dir, 0, 0},
//# endif
//# ifdef FSYS_REISERFS
//  {"reiserfs", reiserfs_mount, reiserfs_read, reiserfs_dir, 0, reiserfs_embed},
//# endif
//# ifdef FSYS_VSTAFS
//  {"vstafs", vstafs_mount, vstafs_read, vstafs_dir, 0, 0},
// endif
//# ifdef FSYS_JFS
//  {"jfs", jfs_mount, jfs_read, jfs_dir, 0, jfs_embed},
//# endif
//# ifdef FSYS_XFS
//  {"xfs", xfs_mount, xfs_read, xfs_dir, 0, 0},
//# endif
//# ifdef FSYS_UFS2
//  {"ufs2", ufs2_mount, ufs2_read, ufs2_dir, 0, ufs2_embed},
//# endif
# ifdef FSYS_ISO9660
  {"iso9660", iso9660_mount, iso9660_read, iso9660_dir, 0, 0},
# endif
  /* XX FFS should come last as it's superblock is commonly crossing tracks
     on floppies from track 1 to 2, while others only use 1.  */
//# ifdef FSYS_FFS
//  {"ffs", ffs_mount, ffs_read, ffs_dir, 0, ffs_embed},
//# endif
# ifdef FSYS_INITRD
  {"initrdfs", initrdfs_mount, initrdfs_read, initrdfs_dir, initrdfs_close, 0},
# endif
  {0, 0, 0, 0, 0, 0}
};

/* The register ESI should contain the address of the partition to be
   used for loading a chain-loader when chain-loading the loader.  */
unsigned long boot_part_addr = 0;

/*
 *  Global variables describing details of the filesystem
 */

/* FIXME: BSD evil hack */
#include "freebsd.h"
int bsd_evil_hack;

/* filesystem type */
int fsys_type = NUM_FSYS;

struct geometry buf_geom;
struct geometry tmp_geom;	/* tmp variable used in many functions. */
struct geometry fd_geom[4];
struct geometry hd_geom[8];

int rawread_ignore_memmove_overflow = 0;/* blocklist_func() set this to 1 */

unsigned long emu_iso_sector_size_2048 = 0;

/* Convert unicode filename to UTF-8 filename. N is the max UTF-16 characters
 * to be converted. The caller should asure there is enough room in the UTF8
 * buffer. Return the length of the converted UTF8 string.
 */
unsigned long
unicode_to_utf8 (unsigned short *filename, unsigned char *utf8, unsigned long n)
{
	unsigned short uni;
	unsigned long j, k;

	for (j = 0, k = 0; j < n && (uni = filename[j]); j++)
	{
		if (uni <= 0x007F)
		{
#if 0
			if (uni != ' ')
#endif
				utf8[k++] = uni;
#if 0
			else
			{
				/* quote the SPACE with a backslash */
				utf8[k++] = '\\';
				utf8[k++] = uni;
			}
#endif
		}
		else if (uni <= 0x07FF)
		{
			utf8[k++] = 0xC0 | (uni >> 6);
			utf8[k++] = 0x80 | (uni & 0x003F);
		}
		else
		{
			utf8[k++] = 0xE0 | (uni >> 12);
			utf8[k++] = 0x80 | ((uni >> 6) & 0x003F);
			utf8[k++] = 0x80 | (uni & 0x003F);
		}
	}
	utf8[k] = 0;
	return k;
}

#define FOUR_CHAR(x0,x1,x2,x3) (((unsigned long)(char)(x0))|((unsigned long)(char)(x1)<<8)|((unsigned long)(char)(x2)<<16)|((unsigned long)(char)(x3)<<24))

static int
rawdisk_read (unsigned long drive, unsigned long long sector, unsigned long nsec, unsigned long segment)
{
    const unsigned long BADDATA1 = FOUR_CHAR('B','A','D','?');
    unsigned long *plast; /* point to buffer of last sector to be read */
    int r;
    /* Write "BAD?" data to last sector buffer */
    /* No need to fill the whole sector.  Just 16 bytes should be enought to avoid most false-positive case. */
    plast = (unsigned long *)((segment<<4)+(nsec-1)*buf_geom.sector_size);
    plast[3] = plast[2] = plast[1] = plast[0] = BADDATA1;
    r = biosdisk(BIOSDISK_READ, drive, &buf_geom, sector, nsec, segment);
    if (r) // error
	return r;
    /* Check for bad data in last read sector */
    if (plast[0]!=BADDATA1 || plast[1]!=BADDATA1 || plast[2]!=BADDATA1 || plast[3]!=BADDATA1)
	return 0; // not "BAD?", success

    printf_warning("\nFatal! Inconsistent data read from (0x%X)%ld+%d\n",drive,sector,nsec);
	return -1; // error
}

/* Read bytes from DRIVE to BUF. The bytes start at BYTE_OFFSET in absolute
 * sector number SECTOR and with BYTE_LEN bytes long.
 */
int
rawread (unsigned long drive, unsigned long long sector, unsigned long byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned long write)
{
  unsigned long slen, sectors_per_vtrack;
  unsigned long sector_size_bits = log2_tmp (buf_geom.sector_size);

  if (write != 0x900ddeed && write != 0xedde0d90 && write != GRUB_LISTBLK)
	return !(errnum = ERR_FUNC_CALL);

  errnum = 0;

  if (write == 0x900ddeed && ! buf)
    return 1;

//  /* right now safely disable writing 64-bit sector number */
//  if (write == 0x900ddeed && (sector >> 32))
//	return !(errnum = ERR_WRITE);

  /* Reset geometry and invalidate track buffer if the disk is wrong. */
  if (buf_drive != drive)
  {
	if (get_diskinfo (drive, &buf_geom, 0))
	    return !(errnum = ERR_NO_DISK);
	buf_drive = drive;
	buf_track = -1;
	sector_size_bits = log2_tmp (buf_geom.sector_size);
  }

  if (write == GRUB_LISTBLK)
  {
      if (disk_read_func) (*disk_read_func)(sector, byte_offset, byte_len);
      return 1;
  }

  if (!buf)
  {	/* Don't waste time reading from disk, just call disk_read_func. */

	if (disk_read_func)
	{
	    unsigned long sectorsize = buf_geom.sector_size;
	    if (byte_offset)
	    {
		unsigned long len = sectorsize - byte_offset;
		if (len > byte_len) len = byte_len; 
		(*disk_read_func) (sector++, byte_offset, len);
		byte_len -= len;
	    }
	    if (byte_len)
	    {
		while (byte_len > sectorsize)
		{
		    (*disk_read_func) (sector++, 0, sectorsize);
		    byte_len -= sectorsize;
		}
		(*disk_read_func) (sector, 0, byte_len);
	    }
	}
	return 1;
  }

  while (byte_len > 0)
  {
      unsigned long soff, num_sect, size;
      unsigned long long track;
      char *bufaddr;
      unsigned long bufseg;

      size = (byte_len > BUFFERLEN)? BUFFERLEN: (unsigned long)byte_len;

      /* Sectors that need to read. */
      slen = ((byte_offset + size + buf_geom.sector_size - 1) >> sector_size_bits);

      if ((buf_geom.flags & BIOSDISK_FLAG_LBA_EXTENSION) && (! (buf_geom.flags & BIOSDISK_FLAG_BIFURCATE) || (drive & 0xFFFFFF00) == 0x100))
      {
	  /* LBA */
	  sectors_per_vtrack = (BUFFERLEN >> sector_size_bits);

	  /* Get the first sector number in the track.  */
	  soff = ((unsigned long)sector) & (sectors_per_vtrack - 1);

	  /* buffer can be 64K for CDROM, but 63.5K(127 sectors) for HDD. */
	  if (sector_size_bits == 9)	/* 512-byte sector size */
	  {
		/* BUFFERLEN must be 64K */
		sectors_per_vtrack = 127;
		//soff = sector % sectors_per_vtrack;
		if (sector >> 32)
		{
		    // 16 == (0x100000000 % 127)
		    soff = (((unsigned long)(sector >> 32) % 127) * 16 +
			    ((unsigned long)sector % 127)) % 127;
		}
		else
		{
		    soff = ((unsigned long)sector) % sectors_per_vtrack;
		}
	  }
      }
      else
      {
	  /* CHS */
	  if (sector >> 32)	/* sector exceeding 32 bit, too big */
		return !(errnum = ERR_READ);

	  sectors_per_vtrack = buf_geom.sectors;

	  /* Get the first sector number in the track.  */
	  soff = ((unsigned long)sector) % sectors_per_vtrack;
      }

      /* Get the starting sector number of the track. */
      track = sector - soff;

      /* max number of sectors to read in the track. */
      num_sect = sectors_per_vtrack - soff;

      /* Read data into the track buffer; Not all sectors in the track would be filled in. */
      bufaddr = ((char *) BUFFERADDR + (soff << sector_size_bits) + byte_offset);
      bufseg = BUFFERSEG;

      if (track != buf_track)
      {
	  unsigned long long read_start = track;	/* = sector - soff <= sector */
	  unsigned long read_len = sectors_per_vtrack;	/* >= num_sect */

	  buf_track = track;

	  /* If more than one track need to read, only read the portion needed
	   * rather than the whole track with data that won't be used.  */
	  if (slen > num_sect)
	  {
	      buf_track = -1;		/* invalidate the buffer */
	      read_start = sector;	/* read the portion from this sector */
	      read_len = num_sect;	/* to the end of the track */
	      //bufaddr = (char *) BUFFERADDR + byte_offset;
	      bufseg = BUFFERSEG + (soff << (sector_size_bits - 4));
	  }

	  if (rawdisk_read (drive, read_start, read_len, bufseg))
	  {
	      buf_track = -1;		/* invalidate the buffer */
	      /* On error try again to load only the required sectors. */
	      if (slen > num_sect || slen == read_len)
		    return !(errnum = ERR_READ);
	      bufseg = BUFFERSEG + (soff << (sector_size_bits - 4));
	      if (rawdisk_read (drive, sector, slen, bufseg))
		    return !(errnum = ERR_READ);
	      //bufaddr = (char *) BUFFERADDR + byte_offset;
	      /* slen <= num_sect && slen < sectors_per_vtrack */
	      num_sect = slen;
	  }
      } /* if (track != buf_track) */

      /* num_sect is sectors that has been read at BUFADDR and will be used. */
      if (size > (num_sect << sector_size_bits) - byte_offset)
	  size = (num_sect << sector_size_bits) - byte_offset;

      if (write == 0x900ddeed)
      {
	  if (grub_memcmp64 (buf, (unsigned long long)(unsigned int)bufaddr, size) == 0)
		goto next;		/* no need to write */
	  buf_track = -1;		/* invalidate the buffer */
	  grub_memmove64 ((unsigned long long)(unsigned int)bufaddr, buf, size);	/* update data at bufaddr */
	  /* write it! */
	  bufseg = BUFFERSEG + (soff << (sector_size_bits - 4));
	  if (biosdisk (BIOSDISK_WRITE, drive, &buf_geom, sector, num_sect, bufseg))
		return !(errnum = ERR_WRITE);
	  goto next;
      }
      /* Use this interface to tell which sectors were read and used. */
      if (disk_read_func)
      {
	  unsigned long long sector_num = sector;
	  unsigned long length = buf_geom.sector_size - byte_offset;
	  if (length > size)
	      length = size;
	  (*disk_read_func) (sector_num++, byte_offset, length);
	  length = size - length;
	  if (length > 0)
	  {
	      while (length > buf_geom.sector_size)
	      {
		  (*disk_read_func) (sector_num++, 0, buf_geom.sector_size);
		  length -= buf_geom.sector_size;
	      }
	      (*disk_read_func) (sector_num, 0, length);
	  }
      }

      grub_memmove64 (buf, (unsigned long long)(unsigned int)bufaddr, size);
      if (errnum == ERR_WONT_FIT)
      {
	  if (! rawread_ignore_memmove_overflow && buf)
		return 0;
	  errnum = 0;
	  buf = 0/*NULL*/; /* so that further memcheck() always fail */
      }
      else
next:
	  buf += size;
      byte_len -= size;		/* byte_len always >= size */
      sector += num_sect;
      byte_offset = 0;
  } /* while (byte_len > 0) */

  return 1;//(!errnum);
}


int
devread (unsigned long long sector, unsigned long long byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned long write)
{
  unsigned long sector_size_bits = log2_tmp(buf_geom.sector_size);
  unsigned long rw_flag = write;
#if 0   //太旧版本不再支持了  2023-05-24
  if (rw_flag != 0x900ddeed && rw_flag != 0xedde0d90 && rw_flag != GRUB_LISTBLK)
  {//for old devread with 32-bit byte_offset compatibility.
    rw_flag = *(unsigned long*)(&write - 1);
    if (rw_flag != 0x900ddeed && rw_flag != 0xedde0d90)
      return !(errnum = ERR_FUNC_CALL);
    buf = *(unsigned long long*)(&write - 3);
    byte_len = *(unsigned long long*)(&write - 5);
    byte_offset = (unsigned long)byte_offset;
  }
#endif
  if (emu_iso_sector_size_2048)
    {
      emu_iso_sector_size_2048 = 0;
      sector <<= (ISO_SECTOR_BITS - sector_size_bits);
    }

  /* Check partition boundaries */
  if (((unsigned long long)(sector + ((byte_offset + byte_len - 1) >> sector_size_bits)) >= (unsigned long long)part_length) && part_start)
      return !(errnum = ERR_OUTSIDE_PART);

  /* Get the read to the beginning of a partition. */
  sector += byte_offset >> sector_size_bits;
  byte_offset &= buf_geom.sector_size - 1;

  if (disk_read_hook && (((unsigned long)debug) >= 0x7FFFFFFF))
    printf ("<%ld, %ld, %ld>", (unsigned long long)sector, (unsigned long long)byte_offset, (unsigned long long)byte_len);

  /*  Call RAWREAD, which is very similar, but:
   *  --  It takes an extra parameter, the drive number.
   *  --  It requires that "sector" is relative to the beginning of the disk.
   *  --  It doesn't handle offsets across the sector boundary.
   */
  return rawread (current_drive, (sector += part_start), byte_offset, byte_len, buf, rw_flag);
}


/* Write 1 sector at BUF onto sector number SECTOR on drive DRIVE.
 * Only a 512-byte sector should be written with this function.
 * Return:
 *		1	success
 *		0	failure
 */
int
rawwrite (unsigned long drive, unsigned long long sector, unsigned long long buf)
{
  /* Reset geometry and invalidate track buffer if the disk is wrong. */
  if (buf_drive != drive)
  {
	if (get_diskinfo (drive, &buf_geom, 0))
	    return !(errnum = ERR_NO_DISK);
	buf_drive = drive;
	buf_track = -1;
  }

  /* skip the write if possible. */
  if (rawdisk_read(drive, sector, 1, SCRATCHSEG)) /* use buf_geom */
    {
      errnum = ERR_READ;
      return 0;
    }

  if (! grub_memcmp64 ((unsigned long long) SCRATCHADDR, buf, SECTOR_SIZE))
	return 1;

  grub_memmove64 ((unsigned long long) SCRATCHADDR, buf, SECTOR_SIZE);
  if (biosdisk (BIOSDISK_WRITE, drive, &buf_geom, sector, 1, SCRATCHSEG))
    {
      errnum = ERR_WRITE;
      return 0;
    }

#if 1
  //if (buf_drive == drive && sector - sector % buf_geom.sectors == buf_track)
  if (buf_drive == drive && sector >= buf_track && sector - buf_track < buf_geom.sectors)
    {
	/* Update the cache. */
	grub_memmove64 (BUFFERADDR + ((sector - buf_track) << SECTOR_BITS), buf, SECTOR_SIZE);
    }
#else
  if (sector - sector % buf_geom.sectors == buf_track)
    /* Clear the cache.  */
    buf_track = -1;
#endif

  return 1;
}

int
devwrite (unsigned long long sector, unsigned long long sector_count, unsigned long long buf)
{
      unsigned long i;

      for (i = 0; i < sector_count; i++)
	{
	  if (! rawwrite (current_drive, (part_start + sector + i), buf + (i << SECTOR_BITS)))
	      return 0;
	}
      return 1;
}


#if 1
int
set_bootdev (int hdbias)
{
  int i, j;

//  if (kernel_type != KERNEL_TYPE_FREEBSD && kernel_type != KERNEL_TYPE_NETBSD)
//	return 0;
  /* Copy the boot partition information to 0x7be-0x7fd for chain-loading.  */
  if ((current_drive & 0x80) && cur_part_addr)
    {
      if (rawread (current_drive, cur_part_offset, 0, SECTOR_SIZE, (unsigned long long)(unsigned long) SCRATCHADDR, 0xedde0d90))
	{
	  char *dst, *src;

	  /* Need only the partition table.
	     XXX: We cannot use grub_memmove because BOOT_PART_TABLE
	     (0x07be) is less than 0x1000.  */
	  dst = (char *) BOOT_PART_TABLE;
	  src = (char *) SCRATCHADDR + BOOTSEC_PART_OFFSET;
	  while (dst < (char *) BOOT_PART_TABLE + BOOTSEC_PART_LENGTH)
	    *dst++ = *src++;
	  PC_SLICE_START (BOOT_PART_TABLE - PC_SLICE_OFFSET, cur_part_entry) = cur_part_start;

	  /* Clear the active flag of all partitions.  */
	  for (i = 0; i < 4; i++)
	    PC_SLICE_FLAG (BOOT_PART_TABLE - BOOTSEC_PART_OFFSET, i) = 0;

	  /* Set the active flag of the booted partition.  */
	  *((unsigned char *) cur_part_addr) = PC_SLICE_FLAG_BOOTABLE;
	  boot_part_addr = cur_part_addr;
	}
      else
      {
	return 0;
      }
    }

  /*
   *  Set BSD boot device.
   */
  i = (current_partition >> 16) + 2;
  if (current_partition == 0xFFFFFF)
    i = 1;
  else if ((current_partition >> 16) == 0xFF)
    i = 0;

  /* FIXME: extremely evil hack!!! */
  j = 2;
  if (current_drive & 0x80)
    j = bsd_evil_hack;

  return MAKEBOOTDEV (j, (i >> 4), (i & 0xF), ((current_drive - hdbias) & 0x7F), ((current_partition >> 8) & 0xFF));
}
#endif


/*
 *  This prints the filesystem type or gives relevant information.
 */
void
print_fsys_type (void)
{
	if (do_completion) return;

	printf (" Filesystem type ");

	if (fsys_type != NUM_FSYS)
	{
	#ifdef FSYS_PXE
		#ifdef FSYS_IPXE
		if (fsys_table[fsys_type].mount_func == pxe_mount)
		{
			printf ("is %cPXE\n",(current_partition == IPXE_PART)?'i':' ');
			return;
		}
		#endif
	#endif
		if (fsys_table[fsys_type].mount_func == fat_mount)
		{
			switch (fats_type)
			{
				case 12:
					printf ("is fat12, ");
					break;
				case 16:
					printf ("is fat16, ");
					break;
				case 32:
					printf ("is fat32, ");
					break;
				case 64:
					printf ("is exfat, ");
					break;
				default:
					printf ("is %s, ", fsys_table[fsys_type].name);
					break;
			}
		}
		else if (fsys_table[fsys_type].mount_func == iso9660_mount)
		{
			switch (iso_type)
			{
				case 1:
					printf ("is udf, ");
					break;
				case 2:
					printf ("is iso9660_Joliet, ");
					break;
				case 3:
					printf ("is iso9660_RockRidge, ");
					break;
				default:
					printf ("is %s, ", fsys_table[fsys_type].name);
					break;
			}
		}
		else
			printf ("is %s, ", fsys_table[fsys_type].name);
	}
	else
		printf ("unknown, ");

	if (current_partition == 0xFFFFFF)
		printf ("using whole disk\n");
	else
		printf ("partition type 0x%02X\n", (unsigned long)(unsigned char)current_slice);
}


  /* Get next BSD partition in current PC slice.  */
static int
next_bsd_partition (/*unsigned long drive, unsigned long *partition, int *type, unsigned long *start, unsigned long *len, char *buf*/void)
{
      int i;
      bsd_part_no = (*next_partition_partition & 0xFF00) >> 8;

      /* If this is the first time...  */
      if (bsd_part_no == 0xFF)
	{
	  /* Check if the BSD label is within current PC slice.  */
	  if (*next_partition_len < BSD_LABEL_SECTOR + 1)
	    {
	      errnum = ERR_BAD_PART_TABLE;
	      return 0;
	    }

	  /* Read the BSD label.  */
	  if (! rawread (next_partition_drive, *next_partition_start + BSD_LABEL_SECTOR,
			 0, SECTOR_SIZE, (unsigned long long)(unsigned int)next_partition_buf, 0xedde0d90))
	    return 0;

	  /* Check if it is valid.  */
	  if (! BSD_LABEL_CHECK_MAG (next_partition_buf))
	    {
	      errnum = ERR_BAD_PART_TABLE;
	      return 0;
	    }

	  bsd_part_no = -1;
	}

      /* Search next valid BSD partition.  */
      if (BSD_LABEL_NPARTS (next_partition_buf) <= BSD_LABEL_NPARTS_MAX)
      for (i = bsd_part_no + 1; i < BSD_LABEL_NPARTS (next_partition_buf); i++)
	{
	  if (BSD_PART_TYPE (next_partition_buf, i))
	    {
	      /* Note that *TYPE and *PARTITION were set
		 for current PC slice.  */
	      *next_partition_type = (BSD_PART_TYPE (next_partition_buf, i) << 8) | (*next_partition_type & 0xFF);
	      *next_partition_start = BSD_PART_START (next_partition_buf, i);
	      *next_partition_len = BSD_PART_LENGTH (next_partition_buf, i);
	      *next_partition_partition = (*next_partition_partition & 0xFF00FF) | (i << 8);

	      /* XXX */
	      if ((next_partition_drive & 0x80) && BSD_LABEL_DTYPE (next_partition_buf) == DTYPE_SCSI)
		bsd_evil_hack = 4;

	      return 1;
	    }
	}

      errnum = ERR_NO_PART;
      return 0;
}

  /* Get next PC slice. Be careful of that this function may return
     an empty PC slice (i.e. a partition whose type is zero) as well.  */

#define GPT_ENTRY_SIZE 0x80
static char primary_partition_table[64];
static int partition_table_type = 0;
static unsigned int gpt_part_max;
static int next_gpt_slice(void)
{
redo:
	if (++pc_slice_no >= gpt_part_max)
	{
		errnum = ERR_PARTITION_LOOP;
		return 0;
	}
	grub_u64_t sector = *next_partition_entry  + (pc_slice_no >> 2);
	if (! rawread (next_partition_drive, sector,(pc_slice_no & 3) * sizeof(GPT_ENT) , sizeof(GPT_ENT), (unsigned long long)(unsigned int)next_partition_buf, 0xedde0d90))
		return 0;
	P_GPT_ENT PI = (P_GPT_ENT)(unsigned int)next_partition_buf;
	if (PI->starting_lba == 0LL /*|| PI->starting_lba > 0xFFFFFFFFL*/)
	{
		errnum = ERR_NO_PART;
		return 0;
	}
	//skip MS_Reserved Partition
	if (memcmp(PI->type.raw,"\x16\xE3\xC9\xE3\x5C\x0B\xB8\x4D\x81\x7D\xF9\x2D\xF0\x02\x15\xAE",16) == 0 && next_partition_dest == 0xffffff)
		goto redo;
	*next_partition_start = PI->starting_lba;
	*next_partition_len = (unsigned long long)(PI->ending_lba - PI->starting_lba + 1);
	*next_partition_partition = (pc_slice_no << 16) | 0xFFFF;
	*next_partition_type = PC_SLICE_TYPE_GPT;
	return 1;
}

static int is_gpt_part(void)
{
	GPT_HDR hdr;
	if (! rawread (next_partition_drive, 1, 0, sizeof(hdr), (unsigned long long)(unsigned int)&hdr, 0xedde0d90))
		return 0;
	if (hdr.hdr_sig != GPT_HDR_SIG) /* Signature ("EFI PART") */
		return 0;
	if (hdr.hdr_size != 0x5C)/*Header size (in bytes, usually 5C 00 00 00 meaning 92 bytes)*/
		return 0;
	if (hdr.hdr_lba_self != 1LL) /*Current LBA (location of this header copy),must be 1*/
		return 0;
	if (hdr.hdr_entsz != GPT_ENTRY_SIZE) /* Size of a partition entry (usually 128) */
	{
		return 0;
	}
	*next_partition_entry = hdr.hdr_lba_table;/* Partition entries starting LBA */
	gpt_part_max = hdr.hdr_entries;/* Number of partition entries */
	partition_table_type = PC_SLICE_TYPE_GPT;
	return 1;
}

static int
next_pc_slice (void)
{
redo:
      pc_slice_no = (*next_partition_partition & 0xFF0000) >> 16;

      if (pc_slice_no == 0xFE)
	{
	  errnum = ERR_PARTITION_LOOP;
	  return 0;
	}

      /* If this is the first time...  */
      if (pc_slice_no == 0xFF)
	{
	  partition_table_type = 0;
	  *next_partition_offset = 0;
	  *next_partition_ext_offset = 0;
	  *next_partition_entry = -1;
	  pc_slice_no = -1;
	}
	else if (partition_table_type == PC_SLICE_TYPE_GPT)
	{
		return next_gpt_slice();
	}

      /* Read the MBR or the boot sector of the extended partition.  */
      if (! rawread (next_partition_drive, *next_partition_offset, 0, SECTOR_SIZE, (unsigned long long)(unsigned int)next_partition_buf, 0xedde0d90))
	return 0;
      if (pc_slice_no == -1 && next_partition_buf[0x1C2] == '\xEE' && is_gpt_part())
	{
		if (next_partition_dest != 0xffffff)
			pc_slice_no = (next_partition_dest>>16) - 1;
		return next_gpt_slice();
	}


      /* Check if it is valid.  */
      if (! PC_MBR_CHECK_SIG (next_partition_buf))
	{
bad_part_table:
	  errnum = ERR_BAD_PART_TABLE;
	  return 0;
	}

      /* backup partition table in the MBR */
      if (*next_partition_offset == 0)
      {
	grub_memmove (primary_partition_table, next_partition_buf + 0x1BE, 64);
      }
      else
      {
	int i;

	/* Check if it is the same as primary_partition_table.  */
	if (! grub_memcmp (primary_partition_table, next_partition_buf + 0x1BE, 64))
		goto bad_part_table;

	/* Check if it contains extended partition entry. if yes, check if it is valid.  */
	for (i = 0; i < PC_SLICE_MAX; i++)
	{
		if (IS_PC_SLICE_TYPE_EXTENDED (PC_SLICE_TYPE (next_partition_buf, i)))
		{
			/* the start should not equal to the last one */
			if ((*next_partition_ext_offset + PC_SLICE_START (next_partition_buf, i)) == *next_partition_offset)
				goto bad_part_table;
		}
	}
      }

next_entry:
      /* Increase the entry number.  */
      (*next_partition_entry)++;

      /* If this is out of current partition table...  */
      if (*next_partition_entry == PC_SLICE_MAX)
	{
	  int i;

	  /* Search the first extended partition in current table.  */
	  for (i = 0; i < PC_SLICE_MAX; i++)
	    {
	      if (IS_PC_SLICE_TYPE_EXTENDED (PC_SLICE_TYPE (next_partition_buf, i)))
		{
		  /* Found. Set the new offset and the entry number,
		     and restart this function.  */
#if 1
		  unsigned long long tmp_start = (unsigned long long)(unsigned long)(PC_SLICE_START (next_partition_buf, i));
		  unsigned long long tmp_ext_offset = (unsigned long long)(unsigned long)(*next_partition_ext_offset);
		  unsigned long long tmp_offset = tmp_ext_offset + tmp_start;
		  /* if overflow ... */
#if 0
		  if (((unsigned long *)(&tmp_offset))[1])  //if (tmp_offset >= 0x100000000ULL)
			continue;
#else
		  /* use this to keep away from the gcc bug.
		   * (tmp_offset >= 0x100000000ULL) is also OK, but
		   * (((unsigned long *)(&tmp_offset))[1]) is not OK with the buggy gcc.
		   */
		  if (tmp_offset >> 32) //if (tmp_offset >= 0x100000000ULL)
			continue;
#endif
		  *next_partition_offset = tmp_offset;
		  if (! *next_partition_ext_offset)
		    *next_partition_ext_offset = tmp_start;
#else
		  if (! *next_partition_ext_offset)
		    *next_partition_offset = (*next_partition_ext_offset = tmp_start);
		  else
		    *next_partition_offset = (*next_partition_ext_offset + tmp_start);
#endif
		  *next_partition_entry = -1;

#if 0
		  return next_pc_slice ();	/* FIXME: Recursive!!!! */
#else
		  goto redo;
#endif
		}
	    }

	  errnum = ERR_NO_PART;
	  return 0;
	}

      {
	unsigned long long tmp_start = (unsigned long long)(unsigned long)(PC_SLICE_START (next_partition_buf, *next_partition_entry));
	unsigned long long tmp_offset = *next_partition_offset;
	tmp_start += tmp_offset;
	*next_partition_start = tmp_start;
	*next_partition_type = PC_SLICE_TYPE (next_partition_buf, *next_partition_entry);
	*next_partition_len = PC_SLICE_LENGTH (next_partition_buf, *next_partition_entry);
	/* if overflow ... */
#if 0
	if (((unsigned long *)(&tmp_start))[1])  //if (tmp_start >= 0x100000000ULL)
#else
	/* use this to keep away from the gcc bug.
	 * (tmp_start >= 0x100000000ULL) is also OK, but
	 * (((unsigned long *)(&tmp_start))[1]) is not OK with the buggy gcc.
	 */
	if (tmp_start >> 32) //if (tmp_offset >= 0x100000000ULL)
#endif
	  //if (((int)pc_slice_no) >= PC_SLICE_MAX - 1)	/* yes, on overflow it is always a logical partition. */
		goto next_entry;

	/* The calculation of a PC slice number is complicated, because of
	   the rather odd definition of extended partitions. Even worse,
	   there is no guarantee that this is consistent with every
	   operating systems. Uggh.  */
	if (((int)pc_slice_no) >= PC_SLICE_MAX - 1)	/* if it is a logical partition */
	{
	    if (PC_SLICE_ENTRY_IS_EMPTY (next_partition_buf, *next_partition_entry)) /* ignore the garbage entry(typically all bytes are 0xF6). */
		goto next_entry;
	}
	else	/* primary partition */
	{
	    if ((PC_SLICE_FLAG (next_partition_buf, *next_partition_entry)) & 0x7F) /* ignore the garbage entry with wrong boot indicator. */
		goto null_entry;
	    if (!((PC_SLICE_SEC (next_partition_buf, *next_partition_entry)) & 0x3F)) /* ignore the garbage entry with wrong starting sector. */
		goto null_entry;
	    if (!((PC_SLICE_ESEC (next_partition_buf, *next_partition_entry)) & 0x3F)) /* ignore the garbage entry with wrong ending sector. */
		goto null_entry;
	    if ((PC_SLICE_HEAD (next_partition_buf, *next_partition_entry)) == 0xFF) /* ignore the garbage entry with wrong starting head. */
		goto null_entry;
	    if ((PC_SLICE_EHEAD (next_partition_buf, *next_partition_entry)) == 0xFF) /* ignore the garbage entry with wrong ending head. */
	    {
null_entry:
		*next_partition_start = 0;
		*next_partition_type = 0;
		*next_partition_len = 0;
	    }
	}

#if 0
	/* disable partition id 00. */
	if (((int)pc_slice_no) >= PC_SLICE_MAX - 1		/* if it is a logical partition */
	    && *next_partition_type == PC_SLICE_TYPE_NONE)	/* ignore the partition with id=00. */
		goto next_entry;
#else
	/* enable partition id 00. */
#endif
      }
#if 0
      if (((int)pc_slice_no) < PC_SLICE_MAX - 1
	  || ! IS_PC_SLICE_TYPE_EXTENDED (*next_partition_type))
	pc_slice_no++;
#else
      if (((int)pc_slice_no) >= PC_SLICE_MAX - 1
	  && IS_PC_SLICE_TYPE_EXTENDED (*next_partition_type))
		goto next_entry;

#if 1
	/* disable partition length of 0. */
	if (((int)pc_slice_no) >= PC_SLICE_MAX - 1	/* if it is a logical partition */
	    && *next_partition_len == 0)		/* ignore the partition with length=0. */
		goto next_entry;
#else
	/* enable partition length of 0. */
#endif

	pc_slice_no++;
#endif
      *next_partition_partition = (pc_slice_no << 16) | 0xFFFF;
      return 1;
}


/* Get the information on next partition on the drive DRIVE.
   The caller must not modify the contents of the arguments when
   iterating this function. The partition representation in GRUB will
   be stored in *PARTITION. Likewise, the partition type in *TYPE, the
   start sector in *START, the length in *LEN, the offset of the
   partition table in *OFFSET, the entry number in the table in *ENTRY,
   the offset of the extended partition in *EXT_OFFSET.
   BUF is used to store a MBR, the boot sector of a partition, or
   a BSD label sector, and it must be at least 512 bytes length.
   When calling this function first, *PARTITION must be initialized to
   0xFFFFFF. The return value is zero if fails, otherwise non-zero.  */
int
next_partition (/*unsigned long drive, unsigned long dest,
		unsigned long *partition, int *type,
		unsigned long *start, unsigned long *len,
		unsigned long *offset, int *entry,
		unsigned long *ext_offset, char *buf*/void)
{
  /* Start the body of this function.  */

  if ((current_drive == NETWORK_DRIVE) || (current_drive == PXE_DRIVE) || (current_drive == FB_DRIVE))
    return 0;

  /* If previous partition is a BSD partition or a PC slice which
     contains BSD partitions...  */
  if ((*next_partition_partition != 0xFFFFFF && IS_PC_SLICE_TYPE_BSD (*next_partition_type & 0xff))
      || ! (next_partition_drive & 0x80))
    {
      if (*next_partition_type == PC_SLICE_TYPE_NONE)
	*next_partition_type = PC_SLICE_TYPE_FREEBSD;

      /* Get next BSD partition, if any.  */
      if (next_bsd_partition (/*next_partition_drive, next_partition_partition, next_partition_type, next_partition_start, next_partition_len, next_partition_buf*/))
	return 1;

      /* If the destination partition is a BSD partition and current
	 BSD partition has any error, abort the operation.  */
      if ((next_partition_dest & 0xFF00) != 0xFF00
	  && ((next_partition_dest & 0xFF0000) == 0xFF0000
	      || (next_partition_dest & 0xFF0000) == (*next_partition_partition & 0xFF0000)))
	return 0;

      /* Ignore the error.  */
      errnum = ERR_NONE;
    }
  return next_pc_slice ();
}

static void
attempt_mount (void)
{
  int cdrom = (current_drive != NETWORK_DRIVE && current_drive != PXE_DRIVE && current_drive != FB_DRIVE && buf_geom.sector_size == 2048);

  for (fsys_type = 0; fsys_type < NUM_FSYS; fsys_type++)
  {
//    if (fsys_type >= 4) continue;
    if (cdrom && fsys_table[fsys_type].mount_func != iso9660_mount)
	continue;
    if (errnum = 0, ((fsys_table[fsys_type].mount_func) ()))
      break;
  }

  if (fsys_type == NUM_FSYS && errnum == ERR_NONE)
    errnum = ERR_FSYS_MOUNT;
}


/*
 *  This performs a "mount" on the current device, both drive and partition
 *  number.
 */

int
open_device (void)
{
  if (open_partition ())
    attempt_mount (); /* device could be pd, nd or ud */

  if (errnum != ERR_NONE)
    return 0;

  return 1;
}


  /* For simplicity.  */
static unsigned long next_part (void);
static unsigned long
next_part (void)
{
	unsigned long i;
	next_partition_drive		= current_drive;
	next_partition_dest		= dest_partition;
	next_partition_partition	= &current_partition;
	next_partition_type		= &current_slice;
	next_partition_start		= &part_start;
	next_partition_len		= &part_length;
	next_partition_offset		= &part_offset;
	next_partition_entry		= &entry;
	next_partition_ext_offset	= &ext_offset;
	next_partition_buf		= mbr;
	i = next_partition ();
	bsd_part_no = (current_partition >> 8) & 0xFF;
	pc_slice_no = current_partition >> 16;
	return i;
}


static void
check_and_print_mount (void)
{
  /* at this point, device has normal partition table. */
  attempt_mount ();
  if (errnum == ERR_FSYS_MOUNT)
    errnum = ERR_NONE;
  if (!errnum)
    print_fsys_type ();
  print_error ();
}


/* Open a partition.  */
int
real_open_partition (int flags)
{
  dest_partition = current_partition;
  grub_memset(vol_name, 0, 256);
  cur_part_offset = 0;
  /* network drive */
  if ((current_drive == NETWORK_DRIVE) || (current_drive==PXE_DRIVE))
  {
		part_length = 0;
    return 1;
	}

  if (current_drive == FB_DRIVE)
    {
      bsd_evil_hack = 0;
      current_slice = 0;
      part_start = 0;
      part_length = 0;
      return 1;
    }

  if (! sane_partition ())
    return 0;

  bsd_evil_hack = 0;
  current_slice = 0;
  part_start = 0;

  /* Make sure that buf_geom is valid. */
  if (buf_drive != current_drive)
    {
      if (get_diskinfo (current_drive, &buf_geom, 0))
	{
	  errnum = ERR_NO_DISK;
	  return 0;
	}
      buf_drive = current_drive;
      buf_track = -1;
    }
  part_length = buf_geom.total_sectors;

  if (buf_geom.sector_size == 2048)
    return 1;

  /* If this is the whole disk, return here.  */
  if (! flags && current_partition == 0xFFFFFF)
    return 1;

  if (flags)
    dest_partition = 0xFFFFFF;

  /* Initialize CURRENT_PARTITION for next_partition.  */
  current_partition = 0xFFFFFF;

  while (next_part ())
    {
    loop_start:

      cur_part_offset = part_offset;
      cur_part_addr = BOOT_PART_TABLE + (entry << 4);
      cur_part_start = part_start;
      cur_part_entry = entry;

      /* If this is a valid partition...  */
      if (current_slice)
	{
	  /* Display partition information.  */
	  if (flags && ! IS_PC_SLICE_TYPE_EXTENDED (current_slice))
	    {
	      if (! do_completion)
		{
		  //if (current_drive & 0x80)
		    {
			int active = (PC_SLICE_FLAG (mbr, entry) == PC_SLICE_FLAG_BOOTABLE);
			grub_printf ("   Partition num: %d%s, ",
				 (unsigned long)(unsigned char)(current_partition >> 16), (active ? ", active": ""));
		    }

		  if (! IS_PC_SLICE_TYPE_BSD (current_slice))
		    check_and_print_mount ();
		  else
		    {
		      int got_part = 0;
		      int saved_slice = current_slice;

		      while (next_part ())
			{
			  if (bsd_part_no == 0xFF)
			    break;

			  if (! got_part)
			    {
			      grub_printf ("[BSD sub-partitions immediately follow]\n");
			      got_part = 1;
			    }

			  grub_printf ("     BSD Partition num: \'%c\', ",
				       (bsd_part_no + 'a'));
			  check_and_print_mount ();
			}

		      if (! got_part)
			grub_printf (" No BSD sub-partition found, partition type 0x%x\n",
				     saved_slice);

		      if (errnum)
			{
			  errnum = ERR_NONE;
			  break;
			}

		      goto loop_start;
		    }
		}
	      else
		{
		  if (bsd_part_no != 0xFF)
		    {
		      char str[16];

		      if (! (current_drive & 0x80)
			  || (dest_partition >> 16) == pc_slice_no)
			grub_sprintf (str, "%c)", (bsd_part_no + 'a'));
		      else
			grub_sprintf (str, "%d,%c)",
				      pc_slice_no, (bsd_part_no + 'a'));
		      print_a_completion (str, 0);
		    }
		  else if (! IS_PC_SLICE_TYPE_BSD (current_slice))
		    {
		      char str[8];

		      grub_sprintf (str, "%d)", pc_slice_no);
		      print_a_completion (str, 0);
		    }
		}
	    }

	  errnum = ERR_NONE;

	  /* Check if this is the destination partition.  */
	  if (! flags
	      && (dest_partition == current_partition
		  || ((dest_partition >> 16) == 0xFF
		      && ((dest_partition >> 8) & 0xFF) == bsd_part_no)))
	    return 1;
	}
    }

  if (flags)
    {
      if (! (current_drive & 0x80))
	{
	  current_partition = 0xFFFFFF;
	  check_and_print_mount ();
	}

      errnum = ERR_NONE;
      return 1;
    }

  return 0;
}


int
open_partition (void)
{
  return real_open_partition (0);
}


static int
sane_partition (void)
{
  /* network drive */
  if (current_drive == PXE_DRIVE)
    return 1;
//
//  /* ram drive */
//  if (current_drive == ram_drive)
//    return 1;

  if (!(current_partition & 0xFF000000uL)	/* the drive field must be 0 */
//      && ((current_drive & 0xFFFFFF7F) < 8	/* and the drive must be < 8 ... */
//	  || current_drive == cdrom_drive)	/* ... or it is cdrom */
      && (current_partition & 0xFF) == 0xFF	/* the low byte is not used and must be 0xFF */
      && ((current_partition & 0xFF00) == 0xFF00 /* the higher byte must be 0xFF for normal ... */
	  || (current_partition & 0xFF00) < 0x800) /* ... or < 8 for BSD partitions */
      /*&& ((current_partition >> 16) == 0xFF*/	/* the partition field must be whole-drive for floppy */
      /*  || (current_drive & 0x80)) */)	/* ... or it is hard drive */
    return 1;

  errnum = ERR_DEV_VALUES;
  return 0;
}


/* Parse a device string and initialize the global parameters. */
char *
set_device (char *device)
{
  int result = 0;

  errnum = 0;
  incomplete = 0;
  disk_choice = 1;
  part_choice = PART_UNSPECIFIED;
  current_drive = saved_drive;
  current_partition = 0xFFFFFF;

  if (*device == '(' && !*(device + 1))
    /* user has given '(' only, let disk_choice handle what disks we have */
    return device + 1;

  if (*device == '(' && *(++device))
    {
      if (*device == ')')	/* the device "()" is for the current root */
	{
	  current_partition = saved_partition;
	  return device + 1;
	}
      if (*device != ',' /* && *device != ')' */ )
	{
	  char ch = *device;
	  if (*device == 'f' || *device == 'h' || *device == 'm' || *device == 'r' || *device == 'b' 
#ifdef FSYS_PXE
	      || (*device == 'p' && pxe_entry)
#endif /* FSYS_PXE */
	      || (*device == 'c' && (cdrom_drive != GRUB_INVALID_DRIVE || atapi_dev_count)))
	    {
	      /* user has given '([fhn]', check for resp. add 'd' and
		 let disk_choice handle what disks we have */
	      if (!*(device + 1))
		{
		  device++;
		  *device++ = 'd';
		  *device = '\0';
		  return device;
		}
	      else if (*(device + 1) == 'd' && !*(device + 2))
		return device + 2;
	    }

	  if ((*device == 'f'
	      || *device == 'h'
	      || *device == 'm'
	      || *device == 'r'
	      || *device == 'b'
#ifdef FSYS_PXE
	      || (*device == 'p' && pxe_entry)
	#ifdef FSYS_IPXE
	      || (*device == 'w' && has_ipxe)
	#endif
#endif
#ifdef FSYS_FB
              || (*device == 'u' && fb_status)
#endif
	      || (*device == 'c' && (cdrom_drive != GRUB_INVALID_DRIVE || atapi_dev_count)))
	      && *(++device, device++) != 'd')
	    errnum = ERR_NUMBER_PARSING;

#ifdef FSYS_PXE
	  if (ch == 'p' && pxe_entry)
	    current_drive = PXE_DRIVE;
	  else
	#ifdef FSYS_IPXE
	  if (ch == 'w' && has_ipxe)
	  {
	    current_drive = PXE_DRIVE;
	    current_partition = IPXE_PART;
	  }
	  else
	#endif
#endif /* FSYS_PXE */
#ifdef FSYS_FB
	  if (ch == 'u' && fb_status)
	    current_drive = FB_DRIVE;
	  else
#endif /* FSYS_FB */
	    {
	      if (ch == 'c' && cdrom_drive != GRUB_INVALID_DRIVE && *device == ')')
		current_drive = cdrom_drive;
	      else if (ch == 'm')
	      {
		current_drive = 0xffff;
		md_part_base = md_part_size = 0LL;
		if (*device == ',')
		{
			++device;
			if (!safe_parse_maxint (&device, &md_part_base) || *device++ != ',' || !safe_parse_maxint (&device, &md_part_size))
			{
				errnum = ERR_DEV_FORMAT;
				return 0;
			}
		}
		}
	      else if (ch == 'r')
		current_drive = ram_drive;
          else if (ch == 'b')
          {
         current_partition = install_partition;
         current_drive = boot_drive;
         return device + 1;
         }
	      else if (ch == 'h' && (*device == ',' || *device == ')'))
		{
			/* it is (hd) for the next new drive that can be added. */
			current_drive = (unsigned char)(0x80 + (*(unsigned char *)0x475));
		}
	      else
		{
		  unsigned long long ull;

		  safe_parse_maxint (&device, &ull);
		  current_drive = ull;
		  disk_choice = 0;
		  if (ch == 'h')
		  {
			if ((long long)ull < 0)
			{
				if ((-ull) <= (unsigned long long)(*(unsigned char *)0x475))
					current_drive = (unsigned char)(0x80 + (*(unsigned char *)0x475) + current_drive);
				else
					return (char *)!(errnum = ERR_DEV_FORMAT);
			} else
				current_drive |= 0x80;
		  }
		  //else if (ch == 'c' && cdrom_drive != GRUB_INVALID_DRIVE && current_drive < 8)
		  //{
		  //  if (cdrom_drives[current_drive] != GRUB_INVALID_DRIVE)
		  //	    current_drive = cdrom_drives[current_drive];
		  //}
		  else if (ch == 'c' && atapi_dev_count && current_drive < (unsigned long)atapi_dev_count)
		  {
		    current_drive += min_cdrom_id;
		  }
		}
	    }
	}

      if (errnum)
	return 0;

      if (*device == ')')
	{
	  part_choice = PART_CHOSEN;
	  result = 1;
	}
      else if (*device == ',')
	{
	  /* Either an absolute PC or BSD partition. */
	  disk_choice = 0;
	  part_choice ++;
	  device++;

	  if (current_drive == FB_DRIVE && fb_status)
	    current_drive = (unsigned char)(fb_status >> 8);
	  if (*device >= '0' && *device <= '9')
	    {
	      unsigned long long ull;
	      part_choice ++;

	      if (!safe_parse_maxint (&device, &ull))
		{
		  errnum = ERR_DEV_FORMAT;
		  return 0;
		}

	      current_partition = (ull << 16) + 0xFFFF;

	      if (*device == ',')
		device++;

	      if (*device >= 'a' && *device <= 'h')
		{
		  current_partition = (((*(device++) - 'a') << 8)
				       | (current_partition & 0xFF00FF));
		}
	    }
	  else if (*device >= 'a' && *device <= 'h')
	    {
	      part_choice ++;
	      current_partition = ((*(device++) - 'a') << 8) | 0xFF00FF;
	    }

	  if (*device == ')')
	    {
	      if (part_choice == PART_DISK)
		{
		  current_partition = saved_partition;
		  part_choice ++;
		}

	      result = 1;
	    }
	}
    }

  if (! sane_partition ())
    return 0;

  if (result)
    return device + 1;
  else
    {
      if (!*device)
	incomplete = 1;
      errnum = ERR_DEV_FORMAT;
    }

  return 0;
}

static char *
setup_part (char *filename)
{
  relative_path = 1;

  if (*filename == '(')
    {
      relative_path = 0;
      if ((filename = set_device (filename)) == 0)
	{
	  current_drive = GRUB_INVALID_DRIVE;
	  return 0;
	}
# ifndef NO_BLOCK_FILES
      if (*filename != '/')
	open_partition ();
      else
# endif /* ! NO_BLOCK_FILES */
	open_device ();
    }
  else if (saved_drive != current_drive
	   || saved_partition != current_partition
	   || (*filename == '/' && fsys_type == NUM_FSYS)
	   || buf_drive == -1)
    {
      current_drive = saved_drive;
      current_partition = saved_partition;
      /* allow for the error case of "no filesystem" after the partition
         is found.  This makes block files work fine on no filesystem */
# ifndef NO_BLOCK_FILES
      if (*filename != '/')
	open_partition ();
      else
# endif /* ! NO_BLOCK_FILES */
	open_device ();
    }

  if (errnum && (*filename == '/' || errnum != ERR_FSYS_MOUNT))
    return 0;
  else
    errnum = 0;

  if (!sane_partition ())
    return 0;

  return filename;
}


///* Reposition a file offset.  */
//unsigned long
//grub_seek (unsigned long offset)
//{
//  if (offset > filemax /*|| offset < 0*/)
//    return -1;
//
//  filepos = offset;
//  return offset;
//}
static int set_filename(char *filename)
{
	char ch = nul_terminate(filename);
	int i = grub_strlen(filename);
	int j = grub_strlen(saved_dir);
	int k;

	if (i >= sizeof(open_filename) || (relative_path && grub_strlen(saved_dir)+i >= sizeof(open_filename)))
		return !(errnum = ERR_WONT_FIT);

	if (relative_path)
		grub_sprintf (open_filename, "%s", saved_dir);
	else
		j = 0;

	for (k = 0; filename[k]; k++)
	{
		if (filename[k] == '"' || filename[k] == '\\' )
			continue;
		else
			open_filename[j++] = filename[k];
	}	
	open_filename[j] = 0;
	filename[i] = ch;

	return 1;
}

int
dir (char *dirname)
{
  int ret;
#ifndef NO_DECOMPRESSION
  compressed_file = 0;
#endif /* NO_DECOMPRESSION */

  if (!(dirname = setup_part (dirname)))
    return 0;

  if (*dirname != '/')
    return !(errnum = ERR_BAD_FILENAME);

  if (fsys_type == NUM_FSYS)
    return !(errnum = ERR_FSYS_MOUNT);

	if (set_filename(dirname) == 0)
		return 0;

  /* set "dir" function to list completions */
  print_possibilities = 1;

  ret = (*(fsys_table[fsys_type].dir_func)) (open_filename);
  if (!ret && !errnum) errnum = ERR_FILE_NOT_FOUND;
  return ret;
}


/* If DO_COMPLETION is true, just print NAME. Otherwise save the unique
   part into UNIQUE_STRING.  */
void
print_a_completion (char *name, int case_insensitive)
{
	char tem[256];
	char *p = tem;
  /* If NAME is "." or "..", do not count it.  */
  if (grub_strcmp (name, ".") == 0 || grub_strcmp (name, "..") == 0)
    return;

	while (*name)
	{
		if (*name == ' ')
		{
			*p++ = '\\';
			*p++ = *name++;
		}
		else
			*p++ = *name++;
	}
	*p = 0;
	name = tem;

  if (do_completion)
    {
      char *buf = unique_string;

      if (! unique)
	while ((*buf++ = (case_insensitive ? tolower(*name++): (*name++))))
	  ;
      else
	{
	  while (*buf && (*buf == (case_insensitive ? tolower(*name) : *name)))
	    {
	      buf++;
	      name++;
	    }
	  /* mismatch, strip it.  */
	  *buf = '\0';
	}
    }
  else
    grub_printf (" %s", name);

  unique++;
}

/*
 *  This lists the possible completions of a device string, filename, or
 *  any sane combination of the two.
 */

int
print_completions (int is_filename, int is_completion)
{
  char *buf = (char *) COMPLETION_BUF;
  char *ptr = buf;

  unique_string = (char *) UNIQUE_BUF;
  *unique_string = 0;
  unique = 0;
  do_completion = is_completion;

  if (! is_filename)
    {
      /* Print the completions of builtin commands.  */
      struct builtin **builtin;

      if (! is_completion)
	grub_printf (" Possible commands are:");

      for (builtin = builtin_table; (*builtin); builtin++)
	{
	  /* If *BUILTIN cannot be run in the command-line, skip it.  */
	  if (! ((*builtin)->flags & BUILTIN_CMDLINE))
	    continue;

	  if (substring (buf, (*builtin)->name, 0) <= 0)
	    print_a_completion ((*builtin)->name, 0);
	}

      if (is_completion && *unique_string)
	{
	  if (unique == 1)
	    {
	      char *u = unique_string + grub_strlen (unique_string);

	      *u++ = ' ';
	      *u = 0;
	    }

	  grub_strcpy (buf, unique_string);
	}

      if (! is_completion)
	grub_putchar ('\n', 255);

      print_error ();
      do_completion = 0;
      if (errnum)
	return -1;
      else
	return unique - 1;
    }

  if (*buf == '/' || (ptr = set_device (buf)) || incomplete)
    {
      errnum = 0;

      if (*buf == '(' && (incomplete || ! *ptr))
	{
	  if (! part_choice)
	    {
	      /* disk completions */
	      int j;
//	      struct geometry tmp_geom;

	      if (! is_completion)
		grub_printf (" Possible disks are: ");

	      if (!ptr
		  || *(ptr-1) != 'd' || (
		  *(ptr-2) != 'c' &&
		  *(ptr-2) != 'm' &&
		  *(ptr-2) != 'r'))
		{
		  int k;
		  for (k = (ptr && (*(ptr-1) == 'd' && *(ptr-2) == 'h') ? 1:0);
		       k < (ptr && (*(ptr-1) == 'd' && *(ptr-2) == 'f') ? 1:2);
		       k++)
		    {
#define HARD_DRIVES (*((char *)0x475))
#define FLOPPY_DRIVES ((*(char*)0x410 & 1)?(*(char*)0x410 >> 6) + 1 : 0)
		      for (j = 0; j < (k ? HARD_DRIVES : FLOPPY_DRIVES); j++)
#undef HARD_DRIVES
#undef FLOPPY_DRIVES
			{
			  unsigned long i;
			  i = (k * 0x80) + j;
			  if ((disk_choice || i == current_drive)
			      && ! get_diskinfo (i, &tmp_geom, 0))
			    {
			      char dev_name[8];

			      grub_sprintf (dev_name, "%cd%d", (k ? 'h':'f'), (unsigned long)j);
			      print_a_completion (dev_name, 0);
			    }
			}
		    }
		}

	      if (rd_base != -1ULL
		  && (disk_choice || ram_drive == current_drive)
		  && (!ptr
		      || *(ptr-1) == '('
		      || (*(ptr-1) == 'd' && *(ptr-2) == 'r')))
		print_a_completion ("rd", 0);

	      if (cdrom_drive != GRUB_INVALID_DRIVE
		  && (disk_choice || cdrom_drive == current_drive)
		  && (!ptr
		      || *(ptr-1) == '('
		      || (*(ptr-1) == 'd' && *(ptr-2) == 'c')))
		print_a_completion ("cd", 0);

	      if (atapi_dev_count  && (!ptr || *(ptr-1) == '(' || (*(ptr-1) == 'd' && *(ptr-2) == 'c')))
	      {
		for (j = 0; j < (unsigned long)atapi_dev_count; j++)
		  if (disk_choice || min_cdrom_id + j == current_drive)
		    {
			char dev_name[8];

			grub_sprintf (dev_name, "cd%d", (unsigned long)j);
			print_a_completion (dev_name, 0);
		    }
	      }

# ifdef FSYS_PXE
	      if (pxe_entry
		  && (disk_choice || PXE_DRIVE == current_drive)
		  && (!ptr
		      || *(ptr-1) == '('
		      || (*(ptr-1) == 'd' && *(ptr-2) == 'p')))
		print_a_completion ("pd", 0);
# endif /* FSYS_PXE */

# ifdef FSYS_FB
	      if (fb_status
		  && (disk_choice || FB_DRIVE == current_drive)
		  && (!ptr
		      || *(ptr-1) == '('
		      || (*(ptr-1) == 'd' && *(ptr-2) == 'u')))
		print_a_completion ("ud", 0);
#endif /* FSYS_FB */

	      if (is_completion && *unique_string)
		{
		  ptr = buf;
		  while (*ptr != '(')
		    ptr--;
		  ptr++;
		  grub_strcpy (ptr, unique_string);
		  if (unique == 1)
		    {
		      ptr += grub_strlen (ptr);
		      if (*unique_string == 'h')
			{
			  *ptr++ = ',';
			  *ptr = 0;
			}
		      else
			{
			  *ptr++ = ')';
			  *ptr = 0;
			}
		    }
		}

	      if (! is_completion)
		grub_putchar ('\n', 255);
	    }
	  else
	    {
	      /* partition completions */
	      if (part_choice == PART_CHOSEN
		  && open_partition ()
		  && ! IS_PC_SLICE_TYPE_BSD (current_slice))
		{
		  unique = 1;
		  ptr = buf + grub_strlen (buf);
		  if (*(ptr - 1) != ')')
		    {
		      *ptr++ = ')';
		      *ptr = 0;
		    }
		}
	      else
		{
		  if (! is_completion)
		    grub_printf (" Possible partitions are:\n");
		  real_open_partition (1);

		  if (is_completion && *unique_string)
		    {
		      ptr = buf;
		      while (*ptr++ != ',')
			;
		      grub_strcpy (ptr, unique_string);
		    }
		}
	    }
	}
      else if (ptr && *ptr == '/')
	{
	  /* filename completions */
	  if (! is_completion)
	    grub_printf (" Possible files are:");

	  dir (buf);

	  if (is_completion && *unique_string)
	    {
	      ptr += grub_strlen (ptr);
	      while (*ptr != '/')
		ptr--;
	      ptr++;

	      grub_strcpy (ptr, unique_string);

	      if (unique == 1)
		{
		  ptr += grub_strlen (unique_string);

		  /* Check if the file UNIQUE_STRING is a directory.  */
		  *ptr = '/';
		  *(ptr + 1) = 0;

		  dir (buf);

		  /* Restore the original unique value.  */
		  unique = 1;

		  if (errnum)
		    {
		      /* Regular file */
		      errnum = 0;
		      *ptr = ' ';
		      *(ptr + 1) = 0;
		    }
		}
	    }

	  if (! is_completion)
	    grub_putchar ('\n', 255);
	}
      else
	errnum = ERR_BAD_FILENAME;
    }

  print_error ();
  do_completion = 0;
  if (errnum)
    return -1;
  else
    return unique - 1;
}


#ifndef NO_BLOCK_FILES
static int block_file = 0;
static unsigned char blk_sector_bit = 0;

struct BLK_LIST_ENTRY {
    unsigned long long start, length;
};
struct BLK_BUF {
  unsigned long long cur_filepos;
  struct BLK_LIST_ENTRY *cur_blklist;
  unsigned long long cur_blknum;
  struct BLK_LIST_ENTRY blklist[1];
};
#define blk_buf (*(struct BLK_BUF *)FSYS_BUF)
#define blk_max_ptr ((struct BLK_LIST_ENTRY *)(FSYS_BUF+0x7800-sizeof(struct BLK_LIST_ENTRY)))

#endif /* NO_BLOCK_FILES */

/*
 *  This is the generic file open function.
 */

int
grub_open (char *filename)
{
#ifndef NO_DECOMPRESSION
  compressed_file = 0;
#endif /* NO_DECOMPRESSION */

  errnum = 0;

  /* if any "dir" function uses/sets filepos, it must
     set it to zero before returning if opening a file! */
  filepos = 0;

  if (!(filename = setup_part (filename)))
    return 0;

#ifndef NO_BLOCK_FILES
  block_file = 0;
#endif /* NO_BLOCK_FILES */

  /* This accounts for partial filesystem implementations. */
  fsmax = 0xFFFFFFFFFFFFFFFFLL;//MAXINT;

  if (*filename != '/')
    {
#ifdef FSYS_IPXE
      if (has_ipxe)
      {
        char *ch = grub_strstr(filename,":");
	if (ch && (grub_u32_t)(ch - filename) < 10)
	{
	   setup_part("(wd)/");
	   goto not_block_file;
	}
      }
#endif
#ifdef NO_BLOCK_FILES
      return !(errnum = ERR_BAD_FILENAME);
#else
      char *ptr = filename;
      //unsigned long list_addr = FSYS_BUF + 12;  /* BLK_BLKLIST_START */
      struct BLK_LIST_ENTRY *p_blklist = blk_buf.blklist;
      filemax = 0;

      //while (list_addr < FSYS_BUF + 0x77F9)	/* BLK_MAX_ADDR */
      while (p_blklist < blk_max_ptr)
	{
	  unsigned long long tmp;
	  tmp = 0;
	  safe_parse_maxint_with_suffix (&ptr, &tmp, 9);
	  errnum = 0;

	  if (*ptr != '+')
	    {
	      /* Set FILEMAX in bytes, Undocumented!! */

	      /* The FILEMAX must not exceed the one calculated from
	       * all the blocks in the list.
	       */

	      if ((*ptr && *ptr != '/' && !isspace (*ptr))
		  || tmp == 0 || tmp > filemax)
		break;		/* failure */

	      filemax = tmp;
	      goto block_file;		/* success */
	    }

	  /* since we use the same filesystem buffer, mark it to
	     be remounted */
	  fsys_type = NUM_FSYS;

	  //*((unsigned long*)list_addr) = tmp;	/* BLK_BLKSTART */
	  p_blklist->start = tmp;
	  ptr++;		/* skip the plus sign */

	  safe_parse_maxint_with_suffix (&ptr, &tmp, 9);

	  if (errnum)
		return 0;

	  if (!tmp || (*ptr && *ptr != ',' && *ptr != '/' && !isspace (*ptr)))
		break;		/* failure */

	  //*((unsigned long*)(list_addr+4)) = tmp;	/* BLK_BLKLENGTH */
	  p_blklist->length = tmp;

	  //tmp *= buf_geom.sector_size;
	  filemax += tmp * buf_geom.sector_size;
	  //list_addr += 8;			/* BLK_BLKLIST_INC_VAL */
	  ++p_blklist;

	  if (*ptr != ',')
		goto block_file;		/* success */

	  ptr++;		/* skip the comma sign */
	} /* while (list_addr < FSYS_BUF + 0x77F9) */

      return !(errnum = ERR_BAD_FILENAME);

//      if (list_addr < FSYS_BUF + 0x77F9 && ptr != filename)
//	{
block_file:
	  block_file = 1;
	  //(*((unsigned long*)FSYS_BUF))= 0;	/* BLK_CUR_FILEPOS */
	  //(*((unsigned long*)(FSYS_BUF+4))) = FSYS_BUF + 12;	/* let BLK_CUR_BLKLIST = BLK_BLKLIST_START */
	  //(*((unsigned long*)(FSYS_BUF+8))) = 0;	/* BLK_CUR_BLKNUM */
	  blk_buf.cur_filepos = 0;
	  blk_buf.cur_blklist = blk_buf.blklist;
	  blk_buf.cur_blknum = 0;

	  blk_sector_bit = 9;
	  while((buf_geom.sector_size >> blk_sector_bit) > 1) ++blk_sector_bit;
	//  if ((1<<blk_sector_bit) != buf_geom.sector_size) blk_sector_bit = 0;

	  unsigned long long mem_drive_size = 0;
	  if (current_drive == ram_drive)
	  {
		mem_drive_size = rd_size;
		goto check_ram_drive_size;
	  }
	  else if (current_drive == 0xffff)
	  {
		if (!md_part_base && !(blk_buf.blklist[0].start))
			return 1;
		mem_drive_size = md_part_size;
		if (md_part_base)
		{
	check_ram_drive_size:
			if (filemax == 512 && (blk_buf.blklist[0].start) == 0)
			{
				filemax = mem_drive_size;
				blk_buf.blklist[0].length = (filemax + 0x1FF) >> 9;
			} else if (filemax > mem_drive_size)
				filemax = mem_drive_size;
		}
	  }
#ifdef NO_DECOMPRESSION
	  return 1;
#else
	  //return (current_drive == 0xffff && ! *((unsigned long*)(FSYS_BUF + 12)) ) ? 1 : gunzip_test_header ();
	  return gunzip_test_header ();
#endif
//	}
#endif /* block files */
    } /* if (*filename != '/') */
#ifdef FSYS_IPXE
not_block_file:
#endif
  if (!errnum && fsys_type == NUM_FSYS)
    errnum = ERR_FSYS_MOUNT;

  /* set "dir" function to open a file */
  print_possibilities = 0;
  if (!set_filename(filename))
	return 0;

  if (!errnum && (*(fsys_table[fsys_type].dir_func)) (open_filename))
    {
#ifdef NO_DECOMPRESSION
      return 1;
#else
      if (no_decompression)
	return 1;
#if 0
      int i;
      i = strlen(open_filename);
      if (i>=5 && substring(open_filename+i-5,".lzma",1)==0)
        dec_lzma_open ();
      else
#endif
        gunzip_test_header ();
      errnum = 0;
      return 1;
#endif
    }

  return 0;
}

#ifndef NO_BLOCK_FILES
unsigned long long
block_read_func (unsigned long long buf, unsigned long long len, unsigned long write)
{
	unsigned long long ret = 0;
	unsigned long size;
	unsigned long off;

	while (len && !errnum)
	{
	  /* we may need to look for the right block in the list(s) */
	  //if (filepos < (*((unsigned long*)FSYS_BUF)) /* BLK_CUR_FILEPOS */)
	  if (filepos < blk_buf.cur_filepos)
	    {
	      //(*((unsigned long*)FSYS_BUF)) = 0;
	      //(*((unsigned long*)(FSYS_BUF+4))) = FSYS_BUF + 12;	/* let BLK_CUR_BLKLIST = BLK_BLKLIST_START */
	      //(*((unsigned long*)(FSYS_BUF+8))) = 0;	/* BLK_CUR_BLKNUM */
	      blk_buf.cur_filepos = 0;
	      blk_buf.cur_blklist = blk_buf.blklist;
	      blk_buf.cur_blknum = 0;
	    }

	     if (filepos > blk_buf.cur_filepos)
	    {
		unsigned long long i;
		unsigned long long tmp;

		tmp = filepos - (blk_buf.cur_filepos & -(unsigned long long)buf_geom.sector_size);
		tmp >>= blk_sector_bit;

		while(tmp)
		{
			i = blk_buf.cur_blklist->length - blk_buf.cur_blknum;
			if (i > tmp)
			{
				blk_buf.cur_blknum += tmp;
				break;
			}
			else
			{
				tmp -= i;
				blk_buf.cur_blklist++;
				blk_buf.cur_blknum = 0;
			}
		}

		blk_buf.cur_filepos = filepos;
	    }

	  off = filepos & (buf_geom.sector_size - 1);

	  {
	    unsigned long long tmp;

	    //tmp = ((unsigned long long)(*((unsigned long*)((*((unsigned long*)(FSYS_BUF+4))) + 4)) - (*((unsigned long*)(FSYS_BUF+8))))
	    //  * (unsigned long long)(buf_geom.sector_size)) - (unsigned long long)off;
	    tmp = (unsigned long long)(blk_buf.cur_blklist->length - blk_buf.cur_blknum) * (buf_geom.sector_size)
		  - off;
	    if (tmp > 0x40000000) tmp = 0x40000000;
	    size = (tmp > len) ? (unsigned long)len : (unsigned long)tmp;
	  }

	  disk_read_func = disk_read_hook;

	  /* read current block and put it in the right place in memory */
	  //devread ((*((unsigned long*)(*((unsigned long*)(FSYS_BUF+4))))) + (*((unsigned long*)(FSYS_BUF+8))),
	  //	   off, size, buf, write);
	  devread (blk_buf.cur_blklist->start + blk_buf.cur_blknum, off, size, buf, write);

	  disk_read_func = NULL;

	  len -= size;
	  filepos += size;
	  ret += size;
	  if (buf)
		buf += size;
	}

      if (errnum)
	ret = 0;

      return ret;
}
#endif /* NO_BLOCK_FILES */

unsigned long long grub_read_loop_threshold = 0x800000ULL; // 8MB
unsigned long long grub_read_step = 0x800000ULL; // 8MB

unsigned long long
grub_read (unsigned long long buf, unsigned long long len, unsigned long write)
{
  if (filepos >= filemax)
      return 0;//!(errnum = ERR_FILELENGTH);

  if (len > filemax - filepos)
      len = filemax - filepos;

  /* if target file position is past the end of
     the supported/configured filesize, then
     there is an error */
  if (filepos + len > fsmax)
      return !(errnum = ERR_FILELENGTH);

  errnum = 0;

  unsigned long long (*read_func) (unsigned long long _buf, unsigned long long _len, unsigned long _write);

#ifndef NO_DECOMPRESSION
  if (compressed_file)
  {
    if (write == 0x900ddeed)
	return !(errnum = ERR_WRITE_GZIP_FILE);
    else 
	read_func = decomp_table[decomp_type].read_func;
  }
  else 
#endif /* NO_DECOMPRESSION */

#ifndef NO_BLOCK_FILES
  if (block_file)
  {
    read_func = block_read_func;
  }
  else
#endif /* NO_BLOCK_FILES */

  if (fsys_type == NUM_FSYS)
    return !(errnum = ERR_FSYS_MOUNT);
  else
    read_func = fsys_table[fsys_type].read_func;

  /* Now, read_func is ready. */
  if ((!buf) || (len < grub_read_loop_threshold)
#ifdef FSYS_IPXE
     || fsys_table[fsys_type].read_func == pxe_read
#endif
#ifndef NO_DECOMPRESSION
      || (compressed_file && decomp_type == DECOMP_TYPE_LZMA)
#endif /* NO_DECOMPRESSION */
  )
  {
    /* Do whole request at once. */
      return read_func(buf, len, write);
  }
  else 
  {
    /* Transfer small amount of data at a time and print progress. */
    unsigned long long byteread = 0;
    unsigned long long remaining = len;
    while (remaining)
    {
	unsigned long long len1;
	unsigned long long ret1;
	grub_printf("\r [%ldM/%ldM]",byteread>>20,len>>20);
	len1 = (remaining > grub_read_step)? grub_read_step : remaining;
	ret1 = read_func(buf, len1, write);
	if (!ret1 || ret1 > len1) break;/*pxe_read returns 0xffffffff when error.*/
	byteread += ret1;
	buf += ret1;		/* Don't do this if buf is 0 */
	remaining -= ret1;
    }
#if 0
    if (remaining)
		grub_printf("\r[%ldM/%ldM]\n",byteread>>20,len>>20);
    else
#endif
		grub_printf("\r                        \r");
    return byteread;
  }
}

void
grub_close (void)
{
#ifndef NO_DECOMPRESSION
  if (compressed_file)
      decomp_table[decomp_type].close_func ();
  compressed_file = 0;
#endif /* NO_DECOMPRESSION */

#ifndef NO_BLOCK_FILES
  if (block_file)
    return;
#endif /* NO_BLOCK_FILES */

  if (fsys_table[fsys_type].close_func != 0)
    (*(fsys_table[fsys_type].close_func)) ();
}
