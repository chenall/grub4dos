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
gunzip_read_func (unsigned long long buf, unsigned long long len, unsigned int write);

block_io_protocol_t blockio_template;


unsigned int fats_type;
unsigned int iso_type;

/* instrumentation variables */
void (*disk_read_func) (unsigned long long, unsigned int, unsigned long long) = NULL;

/* Forward declarations.  */
static int next_bsd_partition (void);
static int next_pc_slice (void);
static int next_gpt_slice(void);
static char open_filename[512];
static unsigned int relative_path;

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
static unsigned int cur_part_addr;
static unsigned long long cur_part_start;
static unsigned int cur_part_entry;

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


/* The first sector of stage2 can be reused as a tmp buffer.
 * Do NOT write more than 512 bytes to this buffer!
 * The stage2-body, i.e., the pre_stage2, starts at 0x8200!
 * Do NOT overwrite the pre_stage2 code at 0x8200!
 */
//char *mbr = (char *)0x8000; /* 512-byte buffer for any use. */

static unsigned int dest_partition;
static unsigned int entry;

static unsigned int bsd_part_no;
static unsigned int pc_slice_no;

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
//  {"minix", minix_mount, minix_read, minix_dir, 0, 0},
//# endif
//# ifdef FSYS_REISERFS
//  {"reiserfs", reiserfs_mount, reiserfs_read, reiserfs_dir, 0, reiserfs_embed},
//# endif
//# ifdef FSYS_VSTAFS
//  {"vstafs", vstafs_mount, vstafs_read, vstafs_dir, 0, 0},
//# endif
//# ifdef FSYS_JFS
//  {"jfs", jfs_mount, jfs_read, jfs_dir, 0, jfs_embed},
//# endif
//# ifdef FSYS_XFS
//  {"xfs", xfs_mount, xfs_read, xfs_dir, 0, 0},
//# endif
//# ifdef FSYS_UFS2
// {"ufs2", ufs2_mount, ufs2_read, ufs2_dir, 0, ufs2_embed},
//# endif
# ifdef FSYS_ISO9660
  {"iso9660", iso9660_mount, iso9660_read, iso9660_dir, 0, 0},
# endif
  /* XX FFS should come last as it's superblock is commonly crossing tracks
     on floppies from track 1 to 2, while others only use 1.  */
//# ifdef FSYS_FFS
//  {"ffs", ffs_mount, ffs_read, ffs_dir, 0, ffs_embed},
//# endif
//# ifdef FSYS_INITRD
//  {"initrdfs", initrdfs_mount, initrdfs_read, initrdfs_dir, initrdfs_close, 0},
//# endif
  {0, 0, 0, 0, 0, 0}
};

/* The register ESI should contain the address of the partition to be
   used for loading a chain-loader when chain-loading the loader.  */
unsigned int boot_part_addr = 0;

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

int rawread_ignore_memmove_overflow = 0;/* blocklist_func() set this to 1 */

unsigned int emu_iso_sector_size_2048 = 0;

/* Convert unicode filename to UTF-8 filename. N is the max UTF-16 characters
 * to be converted. The caller should asure there is enough room in the UTF8
 * buffer. Return the length of the converted UTF8 string.
 */
unsigned int unicode_to_utf8 (unsigned short *filename, unsigned char *utf8, unsigned int n);
unsigned int
unicode_to_utf8 (unsigned short *filename, unsigned char *utf8, unsigned int n)
{
	unsigned short uni;
	unsigned int j, k;

	for (j = 0, k = 0; j < n && (uni = filename[j]); j++)
	{
		if (uni <= 0x007F)
		{
				utf8[k++] = uni;
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

/* Read bytes from DRIVE to BUF. The bytes start at BYTE_OFFSET in absolute   从驱动器DRIVE中读取字节到缓存BUF。
 * sector number SECTOR and with BYTE_LEN bytes long.                         字节开始于绝对扇区号SECTOR的偏移BYTE_OFFSET，字节长BYTE_LEN。
 */
//原读(驱动器号,扇区号,字节偏移,字节长度,缓存,读/写)		返回: 0/1=失败/成功
//1. 如果缓存驱动器号≠驱动器号, 获取磁盘信息
//2. 处理读列表块, 处理磁盘读挂钩
//3. 处理写磁盘
//4. 首先从磁盘读数据到临时缓存BUFFERADDR, 然后再复制到目标缓存buf. 有何必要?
int rawread (unsigned int drive, unsigned long long sector, unsigned int byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned int write);
int
rawread (unsigned int drive, unsigned long long sector, unsigned int byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned int write)
{
  unsigned int sector_size_bits;

  if (write != 0x900ddeed && write != 0xedde0d90 && write != GRUB_LISTBLK)	//如果“write”不是写/读/块列表.	则错误
		return !(errnum = ERR_FUNC_CALL);

  errnum = 0;

  if (write == 0x900ddeed && ! buf)	//如果是写, 而没有缓存, 假写
    return 1;

  /* Reset geometry and invalidate track buffer if the disk is wrong. */
	//如果磁盘错误，请重置几何形状并使轨道缓冲区无效。
	//如果缓存驱动器≠驱动器, 获得磁盘信息
  if (buf_drive != drive)
  {
		if (get_diskinfo (drive, &buf_geom, 0))	//如果'获得磁盘信息'返回非0, 错误
			return !(errnum = ERR_NO_DISK);
		buf_drive = drive;
		buf_track = -1;
  }
  sector_size_bits = log2_tmp (buf_geom.sector_size);	//9/11/12=磁盘/光盘/4k扇区磁盘

	//如果是列表块, 完成它
  if (write == GRUB_LISTBLK)
  {
		if (disk_read_func) (*disk_read_func)(sector, byte_offset, byte_len);
      return 1;
  }
	//如果缓存为零, 不是正常读写, 完成它
  if (!buf)
  {	/* Don't waste time reading from disk, just call disk_read_func. */
		//不要浪费时间从磁盘读取，只需调用disk_read_func“。
		if (disk_read_func)																	//如果disk_read_func挂钩	
		{
	    unsigned int sectorsize = buf_geom.sector_size;	//扇区尺寸
	    if (byte_offset)																	//如果有偏移
	    {
				unsigned int len = sectorsize - byte_offset;		//长度=扇区尺寸-偏移
				if (len > byte_len) len = byte_len; 						//如果长度>请求长度,则长度=请求长度
				(*disk_read_func) (sector++, byte_offset, len);	//向(*disk_read_func)所代表的函数赋值并执行
				byte_len -= len;																//请求长度-长度
	    }
	    if (byte_len)																			//如果请求长度非零
	    {
				while (byte_len > sectorsize)										//条件:请求长度>扇区大小
				{
					(*disk_read_func) (sector++, 0, sectorsize);	//向(*disk_read_func)所代表的函数赋值并执行
					byte_len -= sectorsize;												//请求长度-扇区尺寸
				}
				(*disk_read_func) (sector, 0, byte_len);				//向(*disk_read_func)所代表的函数赋值并执行
			}
		}
		return 1;
  }

  //正常读写
  while (byte_len > 0)		//如果请求字节长度>0
  {
		unsigned int num_sect, size;
		char *bufaddr;

    num_sect = (BUFFERLEN >> sector_size_bits);     //缓存扇区数
    //如果缓存无效, 或者扇区号不在缓存范围
    if (buf_track == (unsigned long long)-1 || sector < buf_track || sector >= (buf_track + num_sect))
    {
      if (grub_efidisk_readwrite (drive, sector, BUFFERLEN, BUFFERADDR, 0xedde0d90))
      {
        buf_track = -1;		/* invalidate the buffer */     
        return !(errnum = ERR_READ);
      }
      buf_track = sector;
    }

    //缓存地址
    bufaddr = BUFFERADDR + ((sector - buf_track) << sector_size_bits) + byte_offset;
    //缓存尺寸
    if (byte_len > (unsigned long long)(BUFFERLEN - (bufaddr - BUFFERADDR)))
      size = BUFFERLEN - (bufaddr - BUFFERADDR);
    else
      size = byte_len;

		if (write == 0x900ddeed)		//如果写
		{
			if (grub_memcmp64 (buf, (unsigned long long)(grub_size_t)bufaddr, size) == 0)
				goto next;		/* no need to write */
			buf_track = -1;		/* invalidate the buffer */
			grub_memmove64 ((unsigned long long)(grub_size_t)bufaddr, buf, size);	/* update data at bufaddr */
			/* write it! */
      if (grub_efidisk_readwrite (drive, sector, size, bufaddr, 0x900ddeed))
				return !(errnum = ERR_WRITE);
			goto next;
		}

		/* Use this interface to tell which sectors were read and used. */
    //使用此接口可以判断哪些扇区被读取和使用。正常读写或略
		if (disk_read_func)
		{
			unsigned long long sector_num = sector;
			unsigned int length = buf_geom.sector_size - byte_offset;
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

		grub_memmove64 (buf, (unsigned long long)(grub_size_t)bufaddr, size);
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
    sector += size >> sector_size_bits;
		byte_offset = 0;
	} /* while (byte_len > 0) */

  return 1;//(!errnum);
}

//设备读(扇区号,字节偏移,字节长度,缓冲区,读/写)
//1. 如果是光盘, 调整扇区号, 按每扇区200字节计
//2. 检查分区边界
//3. 调整字节偏移,使其在1扇区内
int devread (unsigned long long sector, unsigned long long byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned int write);
int
devread (unsigned long long sector, unsigned long long byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned int write)
{
  unsigned int sector_size_bits = log2_tmp(buf_geom.sector_size);
  unsigned int rw_flag = write;

  if (rw_flag != 0x900ddeed && rw_flag != 0xedde0d90 && rw_flag != GRUB_LISTBLK)
  {//for old devread with 32-bit byte_offset compatibility.  为了兼容旧驱动器的32位byte_offset
    rw_flag = *(unsigned int*)(&write - 1);
    if (rw_flag != 0x900ddeed && rw_flag != 0xedde0d90)
      return !(errnum = ERR_FUNC_CALL);
    buf = *(unsigned long long*)(&write - 3);
    byte_len = *(unsigned long long*)(&write - 5);
    byte_offset = (unsigned int)byte_offset;
  }

  if (emu_iso_sector_size_2048)			//如果是读光盘
    {
      emu_iso_sector_size_2048 = 0;	//修改为每扇区0x200字节
      sector <<= (ISO_SECTOR_BITS - sector_size_bits);	//0b-09
    }

  /* Check partition boundaries */
	//检查分区边界
	//如果(扇区号+(字节偏移+字节长度-1)*扇区尺寸)>=分区长度,并且分区起始不为零
  if (((unsigned long long)(sector + ((byte_offset + byte_len - 1) >> sector_size_bits)) >= (unsigned long long)part_length) && part_start)
      return !(errnum = ERR_OUTSIDE_PART);

  /* Get the read to the beginning of a partition. */
	//获取分区的开头。调整字节偏移,使其在1扇区内
  sector += byte_offset >> sector_size_bits;	//扇区号+(字节偏移/扇区尺寸)
  byte_offset &= buf_geom.sector_size - 1;		//字节偏移&(扇区尺寸-1)
	//如果磁盘读挂钩,并且debug) >= 0x7FFFFFFF,打印"扇区号,字节偏移,字节长度"
  if (disk_read_hook && (((unsigned int)debug) >= 0x7FFFFFFF))
    printf ("<%ld, %ld, %ld>", (unsigned long long)sector, (unsigned long long)byte_offset, (unsigned long long)byte_len);

  /*  Call RAWREAD, which is very similar, but:																//调用RAWREAD，这是非常相似，但是
   *  --  It takes an extra parameter, the drive number.											//- 它需要一个额外的参数，驱动器号。
   *  --  It requires that "sector" is relative to the beginning of the disk. //- 它要求，“扇区”是相对于在磁盘开始，即绝对逻辑扇区。
   *  --  It doesn't handle offsets across the sector boundary.								//- 它不处理跨扇区边界的偏移量。
   */
  return rawread (current_drive, (sector += part_start), byte_offset, byte_len, buf, rw_flag);
}


/* Write 1 sector at BUF onto sector number SECTOR on drive DRIVE.		从缓存区写1扇区到扇区号sector
 * Only a 512-byte sector should be written with this function.				只有512字节的扇区可以使用这个功能。
 * Return:																														返回0/1=失败/成功
 *		1	success
 *		0	failure
 */
int rawwrite (unsigned int drive, unsigned long long sector, unsigned long long buf);
int
rawwrite (unsigned int drive, unsigned long long sector, unsigned long long buf)
{
  /* Reset geometry and invalidate track buffer if the disk is wrong. */
	//如果磁盘错误，请重置几何形状并使轨道缓冲区无效。
  if (buf_drive != drive)
  {
		if (get_diskinfo (drive, &buf_geom, 0))
	    return !(errnum = ERR_NO_DISK);
		buf_drive = drive;
		buf_track = -1;
  }

  /* skip the write if possible. 如果可能的话，跳过写*/
  if (grub_efidisk_readwrite (drive, sector, buf_geom.sector_size, SCRATCHADDR, 0xedde0d90)) 
  { 
      errnum = ERR_READ;
      return 0;
    }

  if (! grub_memcmp64 ((unsigned long long)(grub_size_t)SCRATCHADDR, buf, SECTOR_SIZE))
	return 1;

  grub_memmove64 ((unsigned long long)(grub_size_t)SCRATCHADDR, buf, SECTOR_SIZE);
  if (grub_efidisk_readwrite (drive, sector, buf_geom.sector_size, SCRATCHADDR, 0x900ddeed))
    {
      errnum = ERR_WRITE;
      return 0;
    }

  return 1;
}

int set_bootdev (int hdbias);
int
set_bootdev (int hdbias)
{
  int i, j;

  /* Copy the boot partition information to 0x7be-0x7fd for chain-loading.  */
  if ((current_drive & 0x80) && cur_part_addr)
    {
#if 0
      if (rawread (current_drive, cur_part_offset, 0, SECTOR_SIZE, (unsigned long long)(unsigned long)SCRATCHADDR, 0xedde0d90))
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
#endif
    return 0;
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


/*
 *  This prints the filesystem type or gives relevant information.
 */
void print_fsys_type (void);
void
print_fsys_type (void)
{
	if (do_completion) return;

	printf (" Filesystem type ");

	if (fsys_type != NUM_FSYS)
	{
	#ifdef FSYS_PXE
    #if 0
		#ifdef FSYS_IPXE
		if (fsys_table[fsys_type].mount_func == pxe_mount)
		{
			printf ("is %cPXE\n",(current_partition == IPXE_PART)?'i':' ');
			return;
		}
		#endif
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
		printf ("partition type 0x%02X\n", (unsigned int)(unsigned char)current_slice);
}


  /* Get next BSD partition in current PC slice.  */
static int next_bsd_partition (void);
static int
next_bsd_partition (void)
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
			 0, SECTOR_SIZE, (unsigned long long)(grub_size_t)next_partition_buf, 0xedde0d90))
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
static int next_gpt_slice(void);
static int next_gpt_slice(void)
{
redo:
	if (++pc_slice_no >= gpt_part_max)
	{
		errnum = ERR_PARTITION_LOOP;
		return 0;
	}
	grub_u64_t sector = *next_partition_entry  + (pc_slice_no >> 2);
	if (! rawread (next_partition_drive, sector,(pc_slice_no & 3) * sizeof(GPT_ENT) , sizeof(GPT_ENT), (unsigned long long)(grub_size_t)next_partition_buf, 0xedde0d90))
		return 0;
	P_GPT_ENT PI = (P_GPT_ENT)(grub_size_t)next_partition_buf;

	if (PI->starting_lba == 0LL /*|| PI->starting_lba > 0xFFFFFFFFL*/)
	{
//		errnum = ERR_NO_PART;
		return 0;
	}

	//skip MS_Reserved Partition
	if (memcmp(PI->type.raw,"\x16\xE3\xC9\xE3\x5C\x0B\xB8\x4D\x81\x7D\xF9\x2D\xF0\x02\x15\xAE",16) == 0/* && next_partition_dest == 0xffffff*/)
		goto redo;

	*next_partition_start = PI->starting_lba;
	*next_partition_len = (unsigned long long)(PI->ending_lba - PI->starting_lba + 1);
	*next_partition_partition = (pc_slice_no << 16) | 0xFFFF;
	*next_partition_type = PC_SLICE_TYPE_GPT;
	return 1;
}

static int is_gpt_part(void);
static int is_gpt_part(void)
{
	GPT_HDR hdr;
	if (! rawread (next_partition_drive, 1, 0, sizeof(hdr), (unsigned long long)(grub_size_t)&hdr, 0xedde0d90))
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

static int next_pc_slice (void);
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
      if (! rawread (next_partition_drive, *next_partition_offset, 0, SECTOR_SIZE, (unsigned long long)(grub_size_t)next_partition_buf, 0xedde0d90))
	return 0;

	if (pc_slice_no == (unsigned int)-1 && next_partition_buf[0x1C2] == '\xEE' && is_gpt_part())
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

		  unsigned long long tmp_start = (unsigned long long)(grub_size_t)(PC_SLICE_START (next_partition_buf, i));
		  unsigned long long tmp_ext_offset = (unsigned long long)(grub_size_t)(*next_partition_ext_offset);
		  unsigned long long tmp_offset = tmp_ext_offset + tmp_start;
		  /* if overflow ... */

		  /* use this to keep away from the gcc bug.
		   * (tmp_offset >= 0x100000000ULL) is also OK, but
		   * (((unsigned long *)(&tmp_offset))[1]) is not OK with the buggy gcc.
		   */
		  if (tmp_offset >> 32) //if (tmp_offset >= 0x100000000ULL)
			continue;

		  *next_partition_offset = tmp_offset;
		  if (! *next_partition_ext_offset)
		    *next_partition_ext_offset = tmp_start;

		  *next_partition_entry = -1;

		  goto redo;
		}
	    }

	if (next_partition_dest != 0xffffff)
	  errnum = ERR_NO_PART;

	  return 0;
	}

      {
	unsigned long long tmp_start = (unsigned long long)(grub_size_t)(PC_SLICE_START (next_partition_buf, *next_partition_entry));
	unsigned long long tmp_offset = *next_partition_offset;
	tmp_start += tmp_offset;
	*next_partition_start = tmp_start;
	*next_partition_type = PC_SLICE_TYPE (next_partition_buf, *next_partition_entry);
	*next_partition_len = PC_SLICE_LENGTH (next_partition_buf, *next_partition_entry);
	/* if overflow ... */

	/* use this to keep away from the gcc bug.
	 * (tmp_start >= 0x100000000ULL) is also OK, but
	 * (((unsigned long *)(&tmp_start))[1]) is not OK with the buggy gcc.
	 */
	if (tmp_start >> 32) //if (tmp_offset >= 0x100000000ULL)

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
      }

      if (((int)pc_slice_no) >= PC_SLICE_MAX - 1
	  && IS_PC_SLICE_TYPE_EXTENDED (*next_partition_type))
		goto next_entry;

	/* disable partition length of 0. */
	if (((int)pc_slice_no) >= PC_SLICE_MAX - 1	/* if it is a logical partition */
	    && *next_partition_len == 0)		/* ignore the partition with length=0. */
		goto next_entry;

	pc_slice_no++;

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
int next_partition (void);
int
next_partition (void)
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
      if (next_bsd_partition ())
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

static void attempt_mount (void);
static void
attempt_mount (void)
{
	int cdrom = (current_drive >= 0xa0);

  for (fsys_type = 0; fsys_type < NUM_FSYS; fsys_type++)
  {
    if ((cdrom && fsys_table[fsys_type].mount_func != iso9660_mount) || !(fsys_table[fsys_type].mount_func))
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
int open_device (void);
int
open_device (void)
{
  if (open_partition ())
    attempt_mount (); /* device could be pd, nd or ud */

  if (errnum != ERR_NONE)
    return 0;

  return 1;
}

static void check_and_print_mount (void);
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
int real_open_partition (int flags);
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

	//next_partition_buf = mbr;
	struct grub_part_data *q;
	int i;
	for (i=0; (flags ? (i < 16) : (i < 1)); i++)
	{
		if (flags)
			q = get_partition_info (current_drive, (i<<16 | 0xffff));
		else
			q = get_partition_info (current_drive, dest_partition);
		if (!q)
			continue;	

    loop_start:
		
		current_partition	= q->partition;
		bsd_part_no = (current_partition >> 8) & 0xFF;
		pc_slice_no = current_partition >> 16;
		current_slice = q->partition_type;
		
		part_start = q->partition_start;
		cur_part_start = part_start;
		cur_part_offset = q->partition_offset;
		cur_part_addr = BOOT_PART_TABLE + (q->partition_entry << 4);
		cur_part_entry = q->partition_entry;

      /* If this is a valid partition...  */
		if (current_slice)
		{
			/* Display partition information.  */
			if (flags && ! IS_PC_SLICE_TYPE_EXTENDED (current_slice))
	    {				
	      if (! do_completion)
				{
					int active = (PC_SLICE_FLAG (mbr, entry) == PC_SLICE_FLAG_BOOTABLE);
					grub_printf ("   Partition num: %d%s, ",
						(unsigned int)(unsigned char)(current_partition >> 16), (active ? ", active": ""));

					if (! IS_PC_SLICE_TYPE_BSD (current_slice))
						check_and_print_mount ();		
					else
					{
						int got_part = 0;
								

						int saved_slice = current_slice;

						int j;
						for (j=0; (flags ? (j < 16) : (j < 1)); j++)
						{
							if (flags)
								q = get_partition_info (current_drive, (j<<16 | 0xffff));
							else
								q = get_partition_info (current_drive, dest_partition);
							if (!q)
								continue;
							current_partition	= q->partition;
							bsd_part_no = (current_partition >> 8) & 0xFF;
							pc_slice_no = current_partition >> 16;

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
				}//if (! do_completion)
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
			}//if (flags && ! IS_PC_SLICE_TYPE_EXTENDED (current_slice))

			errnum = ERR_NONE;

			/* Check if this is the destination partition.  */
			if (! flags
					&& (dest_partition == current_partition
					|| ((dest_partition >> 16) == 0xFF
		      && ((dest_partition >> 8) & 0xFF) == bsd_part_no)))
				return 1;
		}//if (current_slice)
	}// while (next_part ())

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

int open_partition (void);
int
open_partition (void)
{
  return real_open_partition (0);
}

static int sane_partition (void);
static int
sane_partition (void)
{
  /* network drive */
  if (current_drive == PXE_DRIVE)	//0x21
    return 1;

  /* ram drive */
  if (current_drive == ram_drive)
    return 1;

  if (!(current_partition & 0xFF000000uL)	/* the drive field must be 0 */
      && (current_partition & 0xFF) == 0xFF	/* the low byte is not used and must be 0xFF */
      && ((current_partition & 0xFF00) == 0xFF00 /* the higher byte must be 0xFF for normal ... */
      || (current_partition & 0xFF00) < 0x800) /* ... or < 8 for BSD partitions */
      )	/* ... or it is hard drive */
    return 1;

  errnum = ERR_DEV_VALUES;
  return 0;
}


/* Parse a device string and initialize the global parameters. */
char * set_device (char *device);
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
				|| (*device == 'c' /*&& (cdrom_drive != GRUB_INVALID_DRIVE || atapi_dev_count)*/))
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
  #if 0
	#ifdef FSYS_IPXE
	      || (*device == 'w' && has_ipxe)
	#endif
  #endif
#endif
#ifdef FSYS_FB
              || (*device == 'u' && fb_status)
#endif
	      || (*device == 'c' /*&& (cdrom_drive != GRUB_INVALID_DRIVE || atapi_dev_count)*/))
	      && *(++device, device++) != 'd')
	    errnum = ERR_NUMBER_PARSING;
#ifdef FSYS_PXE
	  if (ch == 'p' && pxe_entry)
	    current_drive = PXE_DRIVE;	//0x21
	  else
  #if 0
	#ifdef FSYS_IPXE
	  if (ch == 'w' && has_ipxe)
	  {
	    current_drive = PXE_DRIVE;	//0x21
	    current_partition = IPXE_PART;	//0x45585069
	  }
	  else
	#endif
  #endif
#endif /* FSYS_PXE */

#ifdef FSYS_FB
	  if (ch == 'u' && fb_status)
	    current_drive = FB_DRIVE;
	  else
#endif /* FSYS_FB */
	    {
	      if (ch == 'c' /*&& cdrom_drive != GRUB_INVALID_DRIVE*/ && *device == ')')	//(cd)
				{
			current_drive = (unsigned char)(0xa0 + cdrom_orig);
				}
				else	if (ch == 'f' && *device == ')')	//(fd)
				{
			current_drive = (unsigned char)(floppies_orig);
				}
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
	      else if (ch == 'h' && (*device == ',' || *device == ')'))	//(hd)
		{
			/* it is (hd) for the next new drive that can be added. */
			current_drive = (unsigned char)(0x80 + harddrives_orig);
		}
	      else
		{
		  unsigned long long ull;

		  safe_parse_maxint (&device, &ull);
		  current_drive = ull;
		  disk_choice = 0;

		  if (ch == 'h')	//(hd-1)	取最后一块硬盘
		  {
			if ((long long)ull < 0)
			{
				if ((-ull) <= (unsigned long long)harddrives_orig)
					current_drive = (unsigned char)(0x80 + harddrives_orig + current_drive);
				else
					return (char *)(grub_size_t)!(errnum = ERR_DEV_FORMAT);
			} else
				current_drive |= 0x80;
		  }
		  else if (ch == 'c')
		  {
				if ((long long)ull < 0)
				{
					if ((-ull) <= (unsigned long long)cdrom_orig)
						current_drive = (unsigned char)(0xa0 + cdrom_orig + current_drive);
					else
						return (char *)(grub_size_t)!(errnum = ERR_DEV_FORMAT);
				}
				else
					current_drive |= 0xa0;
		  }
			else if (ch == 'f')
		  {
				if ((long long)ull < 0)
				{
					if ((-ull) <= (unsigned long long)floppies_orig)
						current_drive = (unsigned char)(0 + floppies_orig + current_drive);
					else
						return (char *)(grub_size_t)!(errnum = ERR_DEV_FORMAT);
				}
				else
					current_drive |= 0;
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

static char *setup_part (char *filename);
static char *
setup_part (char *filename)
{
  relative_path = 1;
  if (*filename == '(')
    {
      relative_path = 0;
      if ((filename = (char *)set_device (filename)) == 0)
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
	   || buf_drive == (unsigned int)-1)
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

static int set_filename(char *filename);
static int set_filename(char *filename)
{
	char ch = nul_terminate(filename);
	int i = grub_strlen(filename);
	int j = grub_strlen(saved_dir);
	int k;

	if (i >= (int)sizeof(open_filename) || (relative_path && grub_strlen(saved_dir)+i >= (int)sizeof(open_filename)))
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

int dir (char *dirname);
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

	if (set_filename((char *)dirname) == 0)
		return 0;

  /* set "dir" function to list completions */
  print_possibilities = 1;

  ret = (*(fsys_table[fsys_type].dir_func)) (open_filename);
  if (!ret && !errnum) errnum = ERR_FILE_NOT_FOUND;
  return ret;
}


/* If DO_COMPLETION is true, just print NAME. Otherwise save the unique
   part into UNIQUE_STRING.  */
void print_a_completion (char *filename, int case_insensitive);
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
int print_completions (int is_filename, int is_completion);
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
	    print_a_completion ((char *)(*builtin)->name, 0);
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

  if (*buf == '/' || (ptr = (char *)set_device (buf)) || incomplete)
    {
      errnum = 0;

      if (*buf == '(' && (incomplete || ! *ptr))
	{
	  if (! part_choice)
	    {
	      /* disk completions */
	      int j;

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

#define HARD_DRIVES harddrives_orig
#define FLOPPY_DRIVES floppies_orig
		      for (j = 0; j < (k ? HARD_DRIVES : FLOPPY_DRIVES); j++)
#undef HARD_DRIVES
#undef FLOPPY_DRIVES
			{
			  unsigned int i;
			  i = (k * 0x80) + j;
			  if ((disk_choice || i == current_drive)
			      && ! get_diskinfo (i, &tmp_geom, 0))
			    {
			      char dev_name[8];

			      grub_sprintf (dev_name, "%cd%d", (k ? 'h':'f'), (unsigned int)j);
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
		print_a_completion ((char *)"rd", 0);
# ifdef FSYS_PXE
	      if (pxe_entry
		  && (disk_choice || PXE_DRIVE == current_drive)
		  && (!ptr
		      || *(ptr-1) == '('
		      || (*(ptr-1) == 'd' && *(ptr-2) == 'p')))
		print_a_completion ((char *)"pd", 0);
# endif /* FSYS_PXE */
# ifdef FSYS_FB
	      if (fb_status
		  && (disk_choice || FB_DRIVE == current_drive)
		  && (!ptr
		      || *(ptr-1) == '('
		      || (*(ptr-1) == 'd' && *(ptr-2) == 'u')))
		print_a_completion ((char *)"ud", 0);
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
int grub_open (char *filename);
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
#if 0
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
#endif
#ifdef NO_BLOCK_FILES
      return !(errnum = ERR_BAD_FILENAME);
#else
      char *ptr = filename;
      struct BLK_LIST_ENTRY *p_blklist = blk_buf.blklist;
      filemax = 0;
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

	  p_blklist->start = tmp;
	  ptr++;		/* skip the plus sign */

	  safe_parse_maxint_with_suffix (&ptr, &tmp, 9);

	  if (errnum)
		return 0;

	  if (!tmp || (*ptr && *ptr != ',' && *ptr != '/' && !isspace (*ptr)))
		break;		/* failure */

	  p_blklist->length = tmp;

	  filemax += tmp * buf_geom.sector_size;
	  ++p_blklist;

	  if (*ptr != ',')
		goto block_file;		/* success */

	  ptr++;		/* skip the comma sign */
	} /* while (list_addr < FSYS_BUF + 0x77F9) */
      return !(errnum = ERR_BAD_FILENAME);

block_file:
	  block_file = 1;
	  blk_buf.cur_filepos = 0;
	  blk_buf.cur_blklist = blk_buf.blklist;
	  blk_buf.cur_blknum = 0;

	  blk_sector_bit = 9;
	  while((buf_geom.sector_size >> blk_sector_bit) > 1) ++blk_sector_bit;
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
	  return gunzip_test_header ();
#endif

#endif /* block files */
    } /* if (*filename != '/') */
#if 0
#ifdef FSYS_IPXE
not_block_file:
#endif
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
unsigned long long block_read_func (unsigned long long buf, unsigned long long len, unsigned int write);
unsigned long long
block_read_func (unsigned long long buf, unsigned long long len, unsigned int write)
{
	unsigned long long ret = 0;
	unsigned int size;
	unsigned int off;

	while (len && !errnum)
	{
	  /* we may need to look for the right block in the list(s) */
	  if (filepos < blk_buf.cur_filepos)
	    {
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

	    tmp = (unsigned long long)(blk_buf.cur_blklist->length - blk_buf.cur_blknum) * (buf_geom.sector_size)
		  - off;
	    if (tmp > 0x40000000) tmp = 0x40000000;
	    size = (tmp > len) ? (unsigned int)len : (unsigned int)tmp;
	  }

	  disk_read_func = disk_read_hook;

	  /* read current block and put it in the right place in memory */
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

unsigned long long grub_read (unsigned long long buf, unsigned long long len, unsigned int write);
unsigned long long
grub_read (unsigned long long buf, unsigned long long len, unsigned int write)
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
  unsigned long long (*read_func) (unsigned long long _buf, unsigned long long _len, unsigned int _write);
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
#if 0
#ifdef FSYS_IPXE
     || fsys_table[fsys_type].read_func == pxe_read
#endif
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

void grub_close (void);
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

int get_diskinfo (unsigned int drive, struct geometry *geometry, unsigned int partition);
int
get_diskinfo (unsigned int drive, struct geometry *geometry, unsigned int partition)
{
	struct grub_disk_data *p;	//磁盘数据
	
	if (drive == 0xffff || drive == ram_drive)
	{
		geometry->sector_size = 512;
		geometry->total_sectors = -1;
		geometry->log2_sector_size = 9;
		return 0;
	}
	
	p = get_device_by_drive (drive);
	if (p)
	{
		geometry->sector_size = p->sector_size;
		geometry->total_sectors = p->total_sectors;
		geometry->log2_sector_size = log2_tmp(p->sector_size);
		return 0; //成功
	}

	return 1;
}

//==============================================================================================================================


//disk.h
/* The sector size.  */
#define GRUB_DISK_SECTOR_SIZE	0x200
#define GRUB_DISK_SECTOR_BITS	9

//efidisk

/* GUID.  */
static grub_efi_guid_t block_io_guid = GRUB_EFI_BLOCK_IO_GUID;

//由枚举磁盘调用
static struct grub_disk_data *make_devices (void);
static struct grub_disk_data *
make_devices (void) //制作设备		
{
  grub_efi_uintn_t num_handles;	//句柄数	api返回
  grub_efi_handle_t *handles;		//句柄集	api返回
  grub_efi_handle_t *handle;		//句柄
  struct grub_disk_data *devices = 0;

  /* Find handles which support the disk io interface. 查找支持磁盘IO接口的句柄 */
  handles = grub_efi_locate_handle (GRUB_EFI_BY_PROTOCOL, &block_io_guid,
				    0, &num_handles); //定位句柄  返回句柄集及句柄数
  if (! handles)							//如果没有句柄集
    return 0;

  /* Make a linked list of devices. 制作设备的链表 */
  for (handle = handles; num_handles--; handle++)
	{
		grub_efi_device_path_t *dp;		//设备路径
		grub_efi_device_path_t *ldp;	//最后一个设备路径
		struct grub_disk_data *d;	//磁盘数据
		grub_efi_block_io_t *bio;

		dp = grub_efi_get_device_path (*handle);  //获得设备路径  通过协议
    if (debug > 1)
      grub_efi_print_device_path(dp);

		if (! dp) //如果失败,继续
			continue;

		ldp = grub_efi_find_last_device_path (dp);  //查找最后设备路径
		if (! ldp) //如果失败,继续
			/* This is empty. Why? 这是空的。为什么 */
			continue;

		bio = grub_efi_open_protocol (*handle, &block_io_guid,
				    GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL); //打开协议 获得协议 块输入输出
		if (! bio) //如果失败,继续
			/* This should not happen... Why? 这不应该发生…为什么 */
			continue; 
      /* iPXE adds stub Block IO protocol to loaded image device handle. It is    IPXE将存根块IO协议添加到加载的图像设备句柄。
         completely non-functional and simply returns an error for every method.  它完全是非功能性的，只为每个方法返回一个错误。
        So attempt to detect and skip it. Magic number is literal "iPXE" and      因此，尝试检测并跳过它。幻数是字面“IPXE”和检查块大小.
        check block size as well */
      /* FIXME: shoud we close it? We do not do it elsewhere 我们关闭它吗？我们不在别处做 */
		if (bio->media && bio->media->media_id == 0x69505845U &&
         bio->media->block_size == 1)
			continue;

		d = grub_malloc (sizeof (*d));  //分配内存
		if (! d)  //如果分配内存失败
		{
			/* Uggh.  */
			grub_free (handles);  //释放内存
			while (devices) //设备存在
	    {
	      d = devices->next;
	      grub_free (devices);  //释放设备
	      devices = d;
	    }
			return 0;
		}

		//填充设备结构
		d->handle = *handle;
		d->device_path = dp;
		d->last_device_path = ldp;
		d->block_io = bio;
		d->sector_size = bio->media->block_size;
		d->log2_sector = log2_tmp(bio->media->block_size);
		d->total_sectors = bio->media->last_block + 1;
		d->next = devices;
		devices = d;
	}

  grub_free (handles);
  return devices;
}


/* Find the parent device.*/
//由命名设备调用
static struct grub_disk_data *find_parent_device (struct grub_disk_data *devices, struct grub_disk_data *d);
static struct grub_disk_data *
find_parent_device (struct grub_disk_data *devices,
		    struct grub_disk_data *d)		//查找根设备		
{
  grub_efi_device_path_t *dp, *ldp;	//设备路径
  struct grub_disk_data *parent;	//磁盘数据

  dp = grub_efi_duplicate_device_path (d->device_path);	//复制设备路径
  if (! dp)	//如果为零
    return 0;
	//填充数据
  ldp = grub_efi_find_last_device_path (dp);	//查找最后设备路径 
  ldp->type = GRUB_EFI_END_DEVICE_PATH_TYPE;	//0x7f
  ldp->subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;	//0xff
  ldp->length = sizeof (*ldp);

  for (parent = devices; parent; parent = parent->next)
	{
		/* Ignore itself.  忽略自己*/
		if (parent == d)	//如果根=自己, 继续
			continue;

		if (grub_efi_compare_device_paths (parent->device_path, dp) == 0)	//比较根与复制的设备路径, 如果相同, 结束循环
			break;
	}

  grub_free (dp);	//释放复制设备路径产生的内存
  return parent;	//返回根
}

/* Add a device into a list of devices in an ascending order. 以升序将设备添加到设备列表中 */
//由命名设备调用
static void add_device (struct grub_disk_data **devices, struct grub_disk_data *d);
static void
add_device (struct grub_disk_data **devices, struct grub_disk_data *d)  //增加设备	添加d到devices
{
  struct grub_disk_data **p;	//磁盘数据
  struct grub_disk_data *n;	//磁盘数据

  for (p = devices; *p; p = &((*p)->next))
	{
		int ret;

		ret = grub_efi_compare_device_paths (grub_efi_find_last_device_path ((*p)->device_path),
					   grub_efi_find_last_device_path (d->device_path));  //比较devices与d的设备路径	返回: 0/非0=成功/失败
		if (ret == 0)	//如果相同
			ret = grub_efi_compare_device_paths ((*p)->device_path,
					     d->device_path);                                 //再比较一次设备路径		有必要???
		if (ret == 0)	//如果相同, 返回(不添加)
			return;
		else if (ret > 0)	//如果不同, 退出循环
			break;
	}

  n = grub_malloc (sizeof (*n));  //分配内存cdrom_orig
  if (! n)
    return;

  grub_memcpy (n, d, sizeof (*n));	//复制d到n
  n->next = (*p);	//下一个					//0
  (*p) = n;													//保存已分配内存
	if (devices == &hd_devices)
	{
		n->drive = 0x80 + harddrives_orig;
		harddrives_orig++;
	}
	else if (devices == &fd_devices)
	{
		n->drive = floppies_orig;
		floppies_orig++;
	}
	else if (devices == &cd_devices)
	{
		n->drive = 0xa0 + cdrom_orig;
		cdrom_orig++;
	}
}

//由枚举磁盘调用
/* Name the devices. 命名设备 */
static void name_devices (struct grub_disk_data *devices);
static void
name_devices (struct grub_disk_data *devices) //命名设备
{
  struct grub_disk_data *d;	//磁盘数据
	//grub_efi_block_io_media_t *m;	//块io媒体

  /* First, identify devices by media device paths. 首先，识别媒体设备路径类型的设备 */
  for (d = devices; d; d = d->next)
	{
		grub_efi_device_path_t *dp;	//设备路径

		dp = d->last_device_path;		//最后设备路径

		if (! dp)	//如果为零,继续
			continue;

		if (GRUB_EFI_DEVICE_PATH_TYPE (dp) == GRUB_EFI_MEDIA_DEVICE_PATH_TYPE)  //如果设备路径类型=4, 是媒体类型
		{
			int is_hard_drive = 0;	//初始化'是硬盘'

			switch (GRUB_EFI_DEVICE_PATH_SUBTYPE (dp)) //设备子类型
	    {
				case GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE: //1	硬盘子类型
					is_hard_drive = 1;
	      /* Intentionally fall through. 故意跌倒 */
				case GRUB_EFI_CDROM_DEVICE_PATH_SUBTYPE:      //2	光盘子类型
	      {	//注意: 包含硬盘,光盘
					struct grub_disk_data *parent, *parent2;	//磁盘数据

					parent = find_parent_device (devices, d); //查找根设备

					if (!parent)	//如果根设备不存在, 退出
					{
            if (debug > 1)
            {
              grub_printf ("\nskipping orphaned partition: ");  //"跳过孤立分区："
              grub_efi_print_device_path (d->device_path);	//efi打印设备路径
            }
						break;
					}
					parent2 = find_parent_device (devices, parent); //查找根设备的根设备

					if (parent2)	//如果存在根设备的根设备, 标记自己使用, 退出
					{
            if (debug > 1)
            {
              grub_printf ("\nskipping subpartition: ");  //"跳过子分区"
              grub_efi_print_device_path (d->device_path);	//efi打印设备路径
            }
						/* Mark itself as used. 标记自己使用 */
						d->last_device_path = 0;
						break;
					}
					if (!parent->last_device_path)	//如果根设备->最后设备路径不存在,  标记自己使用, 退出.  	//此处的作用是: 5个gpt分区,只保留了最后一个分区,其他个分区过滤了.
					{
						d->last_device_path = 0;
						break;
					}
					if (is_hard_drive)  //如果是硬盘
					{
            if (debug > 1)
            {
              grub_printf ("\nadding a hard drive by a partition: ");	//"通过分区添加硬盘驱动器："
              grub_efi_print_device_path (parent->device_path);	//efi打印设备路径
            }
						add_device (&hd_devices, parent);  //增加设备->硬盘
					}
					else                //如果不是硬盘
					{
            if (debug > 1)
            {
              grub_printf ("\nadding a cdrom by a partition: ");	//"添加光盘由一个分区："
              grub_efi_print_device_path (parent->device_path);	//efi打印设备路径
            }
						add_device (&cd_devices, parent);  //增加设备->光盘
					}

					/* Mark the parent as used. 将根标记为已使用 */
					parent->last_device_path = 0;
	      }
	      /* Mark itself as used. 标记自己使用 */
	      d->last_device_path = 0;	//此处的作用是: 5个gpt分区,只保留了最后一个分区,其他个分区过滤了.
	      break;

				default:
          if (debug > 1)
          {
            grub_printf ("skipping other type: ");	//"跳过其他类型："
            grub_efi_print_device_path (d->device_path);	//efi打印设备路径
          }
	      /* For now, ignore the others. 现在，忽略其他 */
	      break;
	    }
		}
		else
		{
      if (debug > 1)
      {
        grub_printf ("\nskipping non-media: ");	//"跳过非媒体："
        grub_efi_print_device_path (d->device_path);	//efi打印设备路径
      }
		}
	}

	//其次,识别其他类型的设备 光驱,软驱等等
  /* Let's see what can be added more. 让我们看看可以增加什么 */
  for (d = devices; d; d = d->next)
	{
		grub_efi_device_path_t *dp;		//磁盘数据
		grub_efi_block_io_media_t *m;	//块io媒体
		int is_floppy = 0;						//初始化'是软盘'

		dp = d->last_device_path;			//最后设备路径
		if (! dp)	//如果为零
			continue;
		/* Ghosts proudly presented by Apple. 苹果骄傲地展示鬼魂 */	//绕过苹果设备
		if (GRUB_EFI_DEVICE_PATH_TYPE (dp) == GRUB_EFI_MEDIA_DEVICE_PATH_TYPE   //设备路径类型=4
				&& GRUB_EFI_DEVICE_PATH_SUBTYPE (dp)
				== GRUB_EFI_VENDOR_MEDIA_DEVICE_PATH_SUBTYPE)                       //并且设备子类型=3
		{
			grub_efi_vendor_device_path_t *vendor = (grub_efi_vendor_device_path_t *) dp;	//供应商设备路径
			const struct grub_efi_guid apple = GRUB_EFI_VENDOR_APPLE_GUID;      	//供应商苹果 GUID

			if (vendor->header.length == sizeof (*vendor)						//如果句柄尺寸=供应商设备路径尺寸
					&& grub_memcmp ((const char *)&vendor->vendor_guid, (const char *)&apple,	sizeof (vendor->vendor_guid)) == 0			//并且比较苹果guid一致     
					&& find_parent_device (devices, d))									//并且查找根设备成功
				continue;																							//则继续
		}

		m = d->block_io->media;
		if (GRUB_EFI_DEVICE_PATH_TYPE (dp) == GRUB_EFI_ACPI_DEVICE_PATH_TYPE    //设备路径类型=2
				&& GRUB_EFI_DEVICE_PATH_SUBTYPE (dp)
				== GRUB_EFI_ACPI_DEVICE_PATH_SUBTYPE)                             	//设备子类型=1
		{
			grub_efi_acpi_device_path_t *acpi = (grub_efi_acpi_device_path_t *) dp;//acpi设备路径
			/* Floppy EISA ID. 软盘EISA ID  */ 
			if (acpi->hid == 0x60441d0 || acpi->hid == 0x70041d0
					|| acpi->hid == 0x70141d1)
				is_floppy = 1;  //是软盘
		}
		if (is_floppy)  //如果是软盘, 增加软盘
		{
//      if (debug > 1)
//      {
//        grub_printf ("adding a floppy: ");	//"增加软盘"
//        grub_efi_print_device_path (d->device_path);	//efi打印设备路径
//      }
//			add_device (&fd_devices, d);  //增加设备->软盘
		}
		else if (m->read_only && m->block_size > GRUB_DISK_SECTOR_SIZE)	//如果是只读,并且扇区尺寸>512, 增加光盘
		{
			/* This check is too heuristic, but assume that this is a 
	     CDROM drive. 这个检查太有启发性了，但是假设这是CDROM驱动器 */
//      if (debug > 1)
//      {
//        grub_printf ("adding a cdrom by guessing: ");	//“通过猜测添加CDROM：” 
//        grub_efi_print_device_path (d->device_path);	//efi打印设备路径
//      }
//			add_device (&cd_devices, d);  //增加设备->光盘
		}
		else	//否则增加硬盘
		{
	  /* The default is a hard drive. 默认是硬盘驱动器 */
      if (debug > 1)
      {
			grub_printf ("\nadding a hard drive by guessing: ");	//“通过猜测添加硬盘驱动器：” 
			grub_efi_print_device_path (d->device_path);	//efi打印设备路径
      }
//			add_device (&hd_devices, d);  //增加设备->硬盘
			add_device (&fd_devices, d);  //增加设备->软盘
		}
	}
}

//由枚举磁盘,efidisk结束,获取设备句柄调用
static void free_disk_data (struct grub_disk_data *devices);
static void
free_disk_data (struct grub_disk_data *devices)  //释放磁盘数据		
{
  struct grub_disk_data *p, *q;

  for (p = devices; p; p = q)
	{
		q = p->next;
		grub_free (p);
	}
}

static void free_part_data (struct grub_part_data *partitions);
static void
free_part_data (struct grub_part_data *partitions)  //释放分区数据		
{
  struct grub_part_data *p, *q;
	p = partitions;

  while (p)
	{
		q = p->next;
		grub_free (p);
		p = q;
	}
}

/* Enumerate all disks to name devices. 枚举所有磁盘到名称设备 */
//由efidisk初始化调用
void enumerate_disks (void);
void
enumerate_disks (void) //枚举磁盘		
{
  struct grub_disk_data *devices;

  devices = make_devices ();  //制作设备
  if (! devices)  //如果失败
    return;

  name_devices (devices); //命名设备
  free_disk_data (devices); //释放
}				

grub_efi_device_path_t *efi_file_path;
grub_efi_handle_t efi_handle;
void * grub_memalign (grub_size_t align, grub_size_t size);


struct grub_disk_data *get_device_by_drive (unsigned int drive);
struct grub_disk_data *
get_device_by_drive (unsigned int drive)	//由驱动器号获得设备(驱动器号)
{
  struct grub_disk_data *d;	//磁盘数据
	
	if (harddrives_orig && drive >= 0x80 && drive <= 0x8f)
		d = hd_devices;
	else if (cdrom_orig && drive >= 0xa0 && drive <= 0xff)
		d = cd_devices;
	else if (floppies_orig && drive <= 0x7f)
		d = fd_devices;
	else
		return 0;

	for ( ; d; d = d->next)	//从设备结构起始查; 只要设备存在,并且驱动器号不为零;
	{
		if (d->drive == drive)
			break;
	}
	if (! d)	//如果设备=0, 错误
		return 0;

  return d;
}	


//由磁盘读,磁盘写调用
//efi磁盘读写(驱动器号,扇区号,读字节数,缓存区,读/写)		返回: 0/1=成功/失败
//1. 磁盘数据对齐
int grub_efidisk_readwrite (int drive, grub_disk_addr_t sector, grub_size_t size, char *buf, int read_write);
int 
grub_efidisk_readwrite (int drive, grub_disk_addr_t sector,
			grub_size_t size, char *buf, int read_write)
{
  struct grub_disk_data *d;	//磁盘数据
  grub_efi_block_io_t *bio;			//块io
  grub_efi_status_t status;			//状态
  grub_size_t io_align;					//对齐
  char *aligned_buf;						//对齐缓存

  if (read_write != 0xedde0d90 && read_write != 0x900ddeed) //如果不是读/写, 错误
    return 1;

	if (drive == 0xffff || (drive == (int)ram_drive && rd_base != -1ULL))
	{
		unsigned long long disk_sector;
		unsigned long long buf_address;
		unsigned long long tmp;
	
		tmp = ((sector << 9) + size);
		if (drive == (int)ram_drive)
	    tmp += rd_base;
		else
	    tmp += md_part_base;
		if (tmp > 0x100000000ULL && ! is64bit)
			return 1;	/* failure */
		disk_sector = ((sector<<9) + ((drive==0xffff) ? md_part_base : rd_base));
		buf_address = (unsigned long long)(grub_size_t)buf;

		if (read_write == 0x900ddeed) //写
			grub_memmove64 (disk_sector, buf_address, size);
		else
			grub_memmove64 (buf_address, disk_sector, size);
		
		return 0;	/* success */
	}	

	d = get_device_by_drive (drive);
//	efi_handle = d->handle;
//	efi_file_path = d->device_path;
  bio = d->block_io;	//块io

  /* Set alignment to 1 if 0 specified 如果0指定，则将对齐设置为1*/
  io_align = bio->media->io_align ? bio->media->io_align : 1;	//对齐, 如果没有指定则为1
  if ((grub_addr_t) buf & (io_align - 1))	//如果缓存未对齐
	{
		aligned_buf = (char *)grub_memalign (io_align, size);	//使用对齐方式分配内存
		if (! aligned_buf)
			return (errnum = ERR_UNALIGNED);
		if (read_write == 0x900ddeed) //写
			grub_memcpy (aligned_buf, buf, size);
	}
  else
	{
    aligned_buf = buf;
	}

  status = efi_call_5 (((read_write == 0x900ddeed) ? bio->write_blocks : bio->read_blocks), bio,
      bio->media->media_id, (grub_efi_uint64_t) sector,
      (grub_efi_uintn_t) size, aligned_buf);	//读写(读/写,本身块io,media_id,扇区,字节尺寸,对齐缓存)

  if ((grub_addr_t) buf & (io_align - 1))	  	//如果缓存未对齐
	{
		if (read_write == 0xedde0d90)  //读
			grub_memcpy (buf, aligned_buf, size);
		grub_free (aligned_buf);
	}
	
	if (status == GRUB_EFI_NO_MEDIA)
    return (errnum = ERR_DEV_VALUES);
  else if (status != GRUB_EFI_SUCCESS)
    return ((read_write == 0x900ddeed) ? (errnum = ERR_WRITE) : (errnum = ERR_READ));

  return 0;
}


void partition_info_init (void);
void
partition_info_init (void)
{
	struct grub_part_data *d;	//efi分区数据
	int drive;
	unsigned int back_saved_drive = saved_drive;
	unsigned int back_current_drive	=	current_drive;
	unsigned int back_saved_partition	=	saved_partition;
	unsigned int back_current_partition	=	current_partition;
	
	if (partition_info)
		free_part_data (partition_info);
	partition_info = 0;

	for (drive = 0x80; (drive&0xf) < harddrives_orig; drive++)
	{
		unsigned int part = 0xFFFFFF;
		unsigned long long start, len, offset;
		unsigned int type, entry1, ext_offset1;
		saved_drive = current_drive = drive;
		saved_partition = current_partition = part;
		while ((	next_partition_drive = drive,
				next_partition_dest = 0xFFFFFF,
				next_partition_partition = &part,
				next_partition_type = &type,
				next_partition_start = &start,
				next_partition_len = &len,
				next_partition_offset = &offset,
				next_partition_entry = &entry1,
				next_partition_ext_offset	= &ext_offset1,
				next_partition_buf = mbr,
				next_partition ()))
		{
			d = grub_malloc (sizeof (*d));  //分配内存	
			if(! d)  //如果分配内存失败
				return;
			if (*next_partition_type == 0 || *next_partition_type == 5 || *next_partition_type == 0xf)
				continue;

			//填充设备结构																				//(hd0,0)	(hd0,2)	(hd0,3)	(hd0,4)	(hd1,0)	(hd1,1)		(hd1,2)	(hd1,4)	 	(hd1,5)		(hd1,6)
			d->drive = drive;																			//80			80			80			80			81			81				81			81			 	81				81
			d->partition = *next_partition_partition;							//ffff		2ffff		3ffff		4ffff		ffff		1ffff			2ffff		4ffff		 	5ffff			6ffff
			d->partition_type = *next_partition_type;							//ee			ee			ee			ee			07			0f				0				07			 	07				07
			d->partition_start = *next_partition_start;						//800			50800		b4800		118800	3f			a0029cc		0				a002a0b	 	1a207094	2a40b71d
			d->partition_len = *next_partition_len;								//40000		64000		64000		cbddf		a00298d	30382275	0				1020464a 	1020464a	ff79524
			d->partition_offset = *next_partition_offset;					//0				0				0				0				0				0					0      	a0029cc 	1a207055	2a40b6de
			d->partition_entry = *next_partition_entry;						//2				2				2				2				0				1					2				0					0					0
			d->partition_ext_offset = *next_partition_ext_offset;	//0				0				0				0				0				0					0				a0029cc		a0029cc		a0029cc
			d->next = partition_info;															//0				dfb0110	dfb00e0	dfb00b0	dfb0080										dfb0050		dfaff90		dfaff60
			partition_info = d;																		//dfb0110	dfb00e0 dfb00b0	dfb0080	dfb0050										dfaff90		dfaff60		dfaff30
		}
	}
	saved_drive = back_saved_drive;
	current_drive = back_current_drive;
	saved_partition = back_saved_partition;
	current_partition = back_current_partition;
}

struct grub_part_data *get_partition_info (int drive, int partition);
struct grub_part_data *
get_partition_info (int drive, int partition)
{
	struct grub_part_data *dp;
	
	if (drive < 0x80 || drive > 0x8f)
		return 0;
	for (dp = partition_info; dp; dp = dp->next)
	{
		if (dp->drive == drive && (int)dp->partition == partition)
			return dp;
	}
	return 0;
}

void grub_efidisk_fini (void);
void
grub_efidisk_fini (void)		//efidisk结束		
{
  free_disk_data (fd_devices);  //释放设备 软盘
  free_disk_data (hd_devices);  //释放设备 硬盘
  free_disk_data (cd_devices);  //释放设备 光盘
  fd_devices = 0;
  hd_devices = 0;
  cd_devices = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//kern.misc.c
#if 0
/* Divide N by D, return the quotient, and store the remainder in *R.  将N除以D，返回商，并将余数存储在*R中*/
grub_uint64_t grub_divmod64 (grub_uint64_t n, grub_uint64_t d, grub_uint64_t *r);
grub_uint64_t
grub_divmod64 (grub_uint64_t n, grub_uint64_t d, grub_uint64_t *r)    //没有使用
{
  /* This algorithm is typically implemented by hardware. The idea	该算法通常由硬件实现。
     is to get the highest bit in N, 64 times, by keeping						我们的想法是得到N的最高位64次，
     upper(N * 2^i) = (Q * D + M), where upper											通过保持上限(N*2^i)=(Q*D+M)，其中，上限表示128位空间中的高64位。
     represents the high 64 bits in 128-bits space.  */
  unsigned bits = 64;
  grub_uint64_t q = 0;
  grub_uint64_t m = 0;

  /* ARM and IA64 don't have a fast 32-bit division.								ARM和IA64没有快速的32位除法。 
     Using that code would just make us use software division routines, calling  使用这些代码只会让我们使用软件部门的程序，
     ourselves indirectly and hence getting infinite recursion.			间接地呼唤自己，从而得到无限的执念。 
  */
#if 0
  /* Skip the slow computation if 32-bit arithmetic is possible.  如果可以使用32位算法，则跳过慢速计算*/
  if (n < 0xffffffff && d < 0xffffffff)
    {
      if (r)
	*r = ((grub_uint32_t) n) % (grub_uint32_t) d;

      return ((grub_uint32_t) n) / (grub_uint32_t) d;
    }
#endif

  while (bits--)
    {
      m <<= 1;

      if (n & (1ULL << 63))
	m |= 1;

      q <<= 1;
      n <<= 1;

      if (m >= d)
	{
	  q |= 1;
	  m -= d;
	}
    }

  if (r)
    *r = m;

  return q;
}

static grub_efi_uintn_t device_path_node_length (const void *node);
static grub_efi_uintn_t
device_path_node_length (const void *node)  //设备路径节点长度   pppp
{
  return grub_get_unaligned16 ((grub_efi_uint16_t *)
                              &((grub_efi_device_path_protocol_t *)(node))->length);
}

grub_efi_uintn_t grub_efi_get_dp_size (const grub_efi_device_path_protocol_t *dp);
grub_efi_uintn_t
grub_efi_get_dp_size (const grub_efi_device_path_protocol_t *dp)  //获得路径尺寸   pppp
{
  grub_efi_device_path_t *p;
  grub_efi_uintn_t total_size = 0;
  for (p = (grub_efi_device_path_t *) dp; ; p = GRUB_EFI_NEXT_DEVICE_PATH (p))
  {
    total_size += GRUB_EFI_DEVICE_PATH_LENGTH (p);
    if (GRUB_EFI_END_ENTIRE_DEVICE_PATH (p))
      break;
  }
  return total_size;
}

static void set_device_path_node_length (void *node, grub_efi_uintn_t len);
static void
set_device_path_node_length (void *node, grub_efi_uintn_t len)	//设置设备路径节点长度  pppp
{
  grub_set_unaligned16 ((grub_efi_uint16_t *)
                        &((grub_efi_device_path_protocol_t *)(node))->length,
                        (grub_efi_uint16_t)(len));
}

grub_efi_device_path_protocol_t*grub_efi_create_device_node (grub_efi_uint8_t node_type, grub_efi_uintn_t node_subtype,
                    grub_efi_uint16_t node_length);
grub_efi_device_path_protocol_t*
grub_efi_create_device_node (grub_efi_uint8_t node_type, grub_efi_uintn_t node_subtype,
                    grub_efi_uint16_t node_length)	//创建设备节点(类型,子类型,尺寸)  pppp
{
  grub_efi_device_path_protocol_t *dp;
  if (node_length < sizeof (grub_efi_device_path_protocol_t))
    return NULL;
  dp = grub_zalloc (node_length);	//分配缓存, 并清零

  if (dp != NULL)
  {
    dp->type = node_type;					//类型
    dp->subtype = node_subtype;		//子类型
    set_device_path_node_length (dp, node_length);	//设置设备路径节点长度
  }
  return dp;
}

grub_efi_device_path_protocol_t* grub_efi_append_device_path (const grub_efi_device_path_protocol_t *dp1,
                    const grub_efi_device_path_protocol_t *dp2);
grub_efi_device_path_protocol_t*
grub_efi_append_device_path (const grub_efi_device_path_protocol_t *dp1,
                    const grub_efi_device_path_protocol_t *dp2) //附加设备路径   pppp
{
  grub_efi_uintn_t size;
  grub_efi_uintn_t size1;
  grub_efi_uintn_t size2;
  grub_efi_device_path_protocol_t *new_dp;
  grub_efi_device_path_protocol_t *tmp_dp;
  // If there's only 1 path, just duplicate it.
  if (dp1 == NULL)
  {
    if (dp2 == NULL)
      return grub_efi_create_device_node (GRUB_EFI_END_DEVICE_PATH_TYPE,	//0x7f
                                 GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE,	//0xff
                                 sizeof (grub_efi_device_path_protocol_t));//创建设备节点(类型,子类型,尺寸)
    else
      return grub_efi_duplicate_device_path (dp2);
  }
  if (dp2 == NULL)
    grub_efi_duplicate_device_path (dp1);
  // Allocate space for the combined device path. It only has one end node of
  // length EFI_DEVICE_PATH_PROTOCOL.
  size1 = grub_efi_get_dp_size (dp1);
  size2 = grub_efi_get_dp_size (dp2);
  size = size1 + size2 - sizeof (grub_efi_device_path_protocol_t);
  new_dp = grub_malloc (size);

  if (new_dp != NULL)
  {
    new_dp = grub_memcpy (new_dp, dp1, size1);
    // Over write FirstDevicePath EndNode and do the copy
    tmp_dp = (grub_efi_device_path_protocol_t *)
           ((char *) new_dp + (size1 - sizeof (grub_efi_device_path_protocol_t)));
    grub_memcpy (tmp_dp, dp2, size2);
  }
  return new_dp;
}

grub_efi_device_path_protocol_t*grub_efi_append_device_node (const grub_efi_device_path_protocol_t *device_path,
                    const grub_efi_device_path_protocol_t *device_node);
grub_efi_device_path_protocol_t*
grub_efi_append_device_node (const grub_efi_device_path_protocol_t *device_path,
                    const grub_efi_device_path_protocol_t *device_node)	//附加设备节点  pppp
{
  grub_efi_device_path_protocol_t *tmp_dp;
  grub_efi_device_path_protocol_t *next_node;
  grub_efi_device_path_protocol_t *new_dp;
  grub_efi_uintn_t node_length;
  if (device_node == NULL)
  {
    if (device_path == NULL)
      return grub_efi_create_device_node (GRUB_EFI_END_DEVICE_PATH_TYPE,	//0x7f
                                 GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE,	//0xff
                                 sizeof (grub_efi_device_path_protocol_t));//创建设备节点(类型,子类型,尺寸)
    else
      return grub_efi_duplicate_device_path (device_path);
  }
  // Build a Node that has a terminator on it
  node_length = device_path_node_length (device_node);

  tmp_dp = grub_malloc (node_length + sizeof (grub_efi_device_path_protocol_t));
  if (tmp_dp == NULL)
    return NULL;
  tmp_dp = grub_memcpy (tmp_dp, device_node, node_length);
  // Add and end device path node to convert Node to device path
  next_node = GRUB_EFI_NEXT_DEVICE_PATH (tmp_dp);
  next_node->type = GRUB_EFI_END_DEVICE_PATH_TYPE;	//0x7f
  next_node->subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;	//0xff
  next_node->length = sizeof (grub_efi_device_path_protocol_t);
  // Append device paths
  new_dp = grub_efi_append_device_path (device_path, tmp_dp);
  grub_free (tmp_dp);
  return new_dp;
}
#endif

//========================================================================================================================================

static grub_efi_boolean_t get_mbr_info (int slot_number, grub_efi_device_path_t *dp, struct grub_part_data *p);
static grub_efi_boolean_t
get_mbr_info (int slot_number, grub_efi_device_path_t *dp, struct grub_part_data *p)	//获得mbr磁盘信息
{
  grub_efi_boot_services_t *b;		//引导服务
  b = grub_efi_system_table->boot_services;	//系统表->引导服务
	grub_efi_device_path_t *tmp_dp;
  grub_efi_device_pate_utilities_protocol_t *DPUP;  //设备路径实用程序协议
  grub_efi_guid_t dpup_guid = GRUB_EFI_DEVICE_PATH_UTILITIES_PROTOCOL_GUID;
  DPUP = grub_efi_locate_protocol (&dpup_guid, 0);  //EFI定位协议
  //创建设备节点
  tmp_dp = (grub_efi_device_path_t *)efi_call_3 (DPUP->CreateDeviceNode,
                      GRUB_EFI_MEDIA_DEVICE_PATH_TYPE,                  //0x04    媒体类型
                      GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE,	        //0x01    硬件子类型
                      sizeof(grub_efi_hard_drive_device_path_t));		    //节点尺寸
  ((grub_efi_hard_drive_device_path_t*)tmp_dp)->partition_number = (p->partition >> 16) + 1;	//分区号
  ((grub_efi_hard_drive_device_path_t*)tmp_dp)->partition_start  = p->partition_start;	      //分区起始 扇区
  ((grub_efi_hard_drive_device_path_t*)tmp_dp)->partition_size   = p->partition_len;	        //分区尺寸 扇区
  *(grub_efi_uint32_t*)((grub_efi_hard_drive_device_path_t*)tmp_dp)
				->partition_signature = signature;																		                //分区签名
  ((grub_efi_hard_drive_device_path_t*)tmp_dp)->partmap_type = 1;							                //分区格式类型
  ((grub_efi_hard_drive_device_path_t*)tmp_dp)->signature_type = 1;						                //签名类型
  //附加设备节点
  disk_drive_map[slot_number].dp = (grub_efi_device_path_t *)efi_call_2 (DPUP->AppendDeviceNode,
                      dp,	                          //设备路径
                      tmp_dp);                      //设备节点
  efi_call_1 (b->free_pool, tmp_dp);	              //释放数据  使用DPUP->CreateDeviceNode创建的tmp_dp只能使用b->free_pool释放
  
  return TRUE;
}

static grub_efi_boolean_t get_gpt_info (int slot_number, grub_efi_device_path_t *dp, struct grub_part_data *p);
static grub_efi_boolean_t
get_gpt_info (int slot_number, grub_efi_device_path_t *dp, struct grub_part_data *p)	//获得gpt磁盘信息
{
  grub_efi_boot_services_t *b;		//引导服务
  b = grub_efi_system_table->boot_services;	//系统表->引导服务
	grub_efi_device_path_t *tmp_dp;
  grub_efi_device_pate_utilities_protocol_t *DPUP;  //设备路径实用程序协议
  grub_efi_guid_t dpup_guid = GRUB_EFI_DEVICE_PATH_UTILITIES_PROTOCOL_GUID;
  DPUP = grub_efi_locate_protocol (&dpup_guid, 0);  //EFI定位协议
  //创建设备节点
  tmp_dp = (grub_efi_device_path_t *)efi_call_3 (DPUP->CreateDeviceNode,
                      GRUB_EFI_MEDIA_DEVICE_PATH_TYPE,                  //0x04    媒体类型
                      GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE,	        //0x01    硬件子类型
                      sizeof(grub_efi_hard_drive_device_path_t));		    //节点尺寸
  ((grub_efi_hard_drive_device_path_t*)tmp_dp)->partition_number = (p->partition >> 16) + 1;	//分区号
  ((grub_efi_hard_drive_device_path_t*)tmp_dp)->partition_start  = p->partition_start;	      //分区起始 扇区
  ((grub_efi_hard_drive_device_path_t*)tmp_dp)->partition_size   = p->partition_len;	        //分区尺寸 扇区
  *(grub_efi_uint32_t*)((grub_efi_hard_drive_device_path_t*)tmp_dp)
				->partition_signature = signature;																		                //分区签名
  ((grub_efi_hard_drive_device_path_t*)tmp_dp)->partmap_type = 2;							                //分区格式类型
  ((grub_efi_hard_drive_device_path_t*)tmp_dp)->signature_type = 2;						                //签名类型
  //附加设备节点
  disk_drive_map[slot_number].dp = (grub_efi_device_path_t *)efi_call_2 (DPUP->AppendDeviceNode,
                      dp,	                          //设备路径
                      tmp_dp);                      //设备节点
  efi_call_1 (b->free_pool, tmp_dp);	              //释放数据  使用DPUP->CreateDeviceNode创建的tmp_dp只能使用b->free_pool释放  
  
  return TRUE;
}

static grub_efi_boolean_t get_iso_info (int slot_number , grub_efi_device_path_t *dp, struct grub_part_data *p);
static grub_efi_boolean_t
get_iso_info (int slot_number, grub_efi_device_path_t *dp, struct grub_part_data *p)
{
  grub_efi_boot_services_t *b;		//引导服务
  b = grub_efi_system_table->boot_services;	//系统表->引导服务
	grub_efi_device_path_t *tmp_dp;
  grub_efi_device_pate_utilities_protocol_t *DPUP;  //设备路径实用程序协议
  grub_efi_guid_t dpup_guid = GRUB_EFI_DEVICE_PATH_UTILITIES_PROTOCOL_GUID;
  DPUP = grub_efi_locate_protocol (&dpup_guid, 0);  //EFI定位协议
  //创建设备节点
  tmp_dp = (grub_efi_device_path_t *)efi_call_3 (DPUP->CreateDeviceNode,
                      GRUB_EFI_MEDIA_DEVICE_PATH_TYPE,                  //0x04    媒体类型
                      GRUB_EFI_CDROM_DEVICE_PATH_SUBTYPE,	              //0x02    光盘子类型
                      sizeof(grub_efi_hard_drive_device_path_t));		    //节点尺寸

  ((grub_efi_cdrom_device_path_t *)tmp_dp)->boot_entry = (unsigned int)boot_entry;	  //引导入口
  ((grub_efi_cdrom_device_path_t *)tmp_dp)->partition_start =	part_addr;	//引导起始 扇区
  ((grub_efi_cdrom_device_path_t *)tmp_dp)->partition_size = part_size;		//引导尺寸 扇区
  //附加设备节点
  disk_drive_map[slot_number].dp = (grub_efi_device_path_t *)efi_call_2 (DPUP->AppendDeviceNode,
                      dp,	                          //设备路径
                      tmp_dp);                      //设备节点
  efi_call_1 (b->free_pool, tmp_dp);	              //释放数据  使用DPUP->CreateDeviceNode创建的tmp_dp只能使用b->free_pool释放  
  
  return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//vboot.c
void copy_file_path (grub_efi_file_path_device_path_t *fp,
		const char *str, grub_efi_uint16_t len);
void
copy_file_path (grub_efi_file_path_device_path_t *fp,
		const char *str, grub_efi_uint16_t len)
{
  grub_efi_char16_t *p, *path_name;
  grub_efi_uint16_t size;

  fp->header.type = GRUB_EFI_MEDIA_DEVICE_PATH_TYPE;
  fp->header.subtype = GRUB_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE;

  path_name = grub_malloc (len * GRUB_MAX_UTF16_PER_UTF8 * sizeof (*path_name));
  if (!path_name)
    return;

  size = grub_utf8_to_utf16 (path_name, len * GRUB_MAX_UTF16_PER_UTF8,
			     (const grub_uint8_t *) str, len, 0);
  for (p = path_name; p < path_name + size; p++)
    if (*p == '/')
      *p = '\\';

  grub_memcpy (fp->path_name, path_name, size * sizeof (*fp->path_name));
  /* File Path is NULL terminated */
  fp->path_name[size++] = '\0';
  fp->header.length = size * sizeof (grub_efi_char16_t) + sizeof (*fp);
  grub_free (path_name);
}

grub_efi_device_path_t * grub_efi_file_device_path (grub_efi_device_path_t *dp, const char *filename);
grub_efi_device_path_t *
grub_efi_file_device_path (grub_efi_device_path_t *dp, const char *filename)//文件设备路径(设备路径, 文件名)
{
  char *dir_start;
  grub_size_t size;
  grub_efi_device_path_t *d;
  grub_efi_device_path_t *file_path;

  dir_start = grub_strchr (filename, ')');	//在字符串中查找字符   查到,返回第一个匹配的字符串位置;否则返回0
  if (! dir_start)
    dir_start = (char *) filename;  //"/EFI/BOOT/bootia32.EFI"	
  else
    dir_start++;

  size = 0;
  d = dp;
  while (1)
	{
		size += GRUB_EFI_DEVICE_PATH_LENGTH (d);		//尺寸+设备路径尺寸
		if ((GRUB_EFI_END_ENTIRE_DEVICE_PATH (d)))	//如果是结束符,退出
			break;
		d = GRUB_EFI_NEXT_DEVICE_PATH (d);	//下一设备路径
	}
  /* File Path is NULL terminated. Allocate space for 2 extra characters 文件路径以空结尾。分配2个额外字符空间 */
  /* FIXME why we split path in two components?  修正为什么我们把路径分成两部分*/
  file_path = grub_malloc (size															//路径尺寸
			   + ((grub_strlen (dir_start) + 2)										//+(文件名尺寸+2)*1*2
			      * GRUB_MAX_UTF16_PER_UTF8									//*1
			      * sizeof (grub_efi_char16_t))							//*2
			   + sizeof (grub_efi_file_path_device_path_t) * 2);	//+(6)*2
  if (! file_path)
    return 0;

  grub_memcpy (file_path, dp, size);	//复制路径

  /* Fill the file path for the directory.  填写目录的文件路径。*/
  d = (grub_efi_device_path_t *) ((char *) file_path
				  + ((char *) d - (char *) dp));		//跳过路径 移动到 7f ff 04 00 之前

  copy_file_path ((grub_efi_file_path_device_path_t *) d,
		  dir_start, grub_strlen (dir_start));	


  /* Fill the file path for the file.  填充文件的文件路径。*/

  /* Fill the end of device path nodes.  */
  d = GRUB_EFI_NEXT_DEVICE_PATH (d);
  d->type = GRUB_EFI_END_DEVICE_PATH_TYPE;;	//0x7f
  d->subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;	//0xff
  d->length = sizeof (*d);

  return file_path;
}

//map.lib.vdisc.c
grub_efi_boolean_t guidcmp (const grub_packed_guid_t *g1, const grub_packed_guid_t *g2);
grub_efi_boolean_t
guidcmp (const grub_packed_guid_t *g1, const grub_packed_guid_t *g2)	//比较guid
{
  grub_efi_uint64_t g1_low, g2_low;
  grub_efi_uint64_t g1_high, g2_high;
  g1_low = grub_get_unaligned64 ((const grub_efi_uint64_t *)g1);
  g2_low = grub_get_unaligned64 ((const grub_efi_uint64_t *)g2);
  g1_high = grub_get_unaligned64 ((const grub_efi_uint64_t *)g1 + 1);
  g2_high = grub_get_unaligned64 ((const grub_efi_uint64_t *)g2 + 1);
  return (grub_efi_boolean_t) (g1_low == g2_low && g1_high == g2_high);
}

grub_packed_guid_t *guidcpy (grub_packed_guid_t *dst, const grub_packed_guid_t *src);
grub_packed_guid_t *
guidcpy (grub_packed_guid_t *dst, const grub_packed_guid_t *src)
{
  grub_set_unaligned64 ((grub_efi_uint64_t *)dst,
                        grub_get_unaligned64 ((const grub_efi_uint64_t *)src));
  grub_set_unaligned64 ((grub_efi_uint64_t *)dst + 1,
                        grub_get_unaligned64 ((const grub_efi_uint64_t*)src + 1));
  return dst;
}

grub_packed_guid_t VDISK_GUID =		//打包guid  虚拟磁盘GUID
{ 0xebe35ad8, 0x6c1e, 0x40f5,
  { 0xaa, 0xed, 0x0b, 0x91, 0x9a, 0x46, 0xbf, 0x4b }
};

static void gen_uuid (void);
static void
gen_uuid (void)	//获得uuid
{
	int i;
	unsigned long long tem;
	struct grub_datetime datetime;
	get_datetime(&datetime);

	tem = (unsigned long long)(datetime.second + datetime.minute) << 56 | (unsigned long long)datetime.second << 48 |
			(unsigned long long)datetime.minute << 40 | (unsigned long long)datetime.hour << 32 | (unsigned long long)datetime.day << 24
				| datetime.month << 16 | datetime.year;
  for (i = 0; i < 2; i++)
  {
    grub_memcpy (((char *)(&VDISK_GUID) + (i << 3)), &tem, 8);
  }
}


//vpart.c
//创建设备节点; 附加设备节点; 安装多协议接口; 连接控制器;
//不安装虚拟分区，一般情况可以正常启动。但是对于4k磁盘，则必须安装。（可能使UEFI对4k磁盘正常不充分）
grub_efi_status_t vpart_install (int slot_number, grub_efi_device_path_t *dp, struct grub_part_data *p); //安装虚拟分区
grub_efi_status_t
vpart_install (int slot_number, grub_efi_device_path_t *dp, struct grub_part_data *p) //安装虚拟分区
{
  grub_efi_status_t status;
  grub_efi_boot_services_t *b;
  b = grub_efi_system_table->boot_services;
  struct grub_disk_data *d;	//磁盘数据
  int present;

  /* guid */
  grub_efi_guid_t dp_guid = GRUB_EFI_DEVICE_PATH_GUID;			//设备路径GUID 
  grub_efi_guid_t blk_io_guid = GRUB_EFI_BLOCK_IO_GUID;			//块IO_GUID

  if (current_drive >= 0xa0)
    present = get_iso_info (slot_number, dp, p);
	else if (p->partition_type == 0xee)
    present = get_gpt_info (slot_number, dp, p);
	else
    present = get_mbr_info (slot_number, dp, p);

  if (!present)
  {
    grub_printf ("NOT FOUND\n");
    return GRUB_EFI_NOT_FOUND;
  }
  printf_debug ("OK\n");

  grub_memcpy (&disk_drive_map[slot_number].block_io, &blockio_template, sizeof (block_io_protocol_t));
  
  disk_drive_map[slot_number].from_handle = NULL;
  disk_drive_map[slot_number].block_io.media = &disk_drive_map[slot_number].media;
	disk_drive_map[slot_number].media.media_id = disk_drive_map[slot_number].from_drive;
  disk_drive_map[slot_number].media.removable_media = FALSE;
  disk_drive_map[slot_number].media.media_present = TRUE;
  disk_drive_map[slot_number].media.logical_partition = TRUE;
  disk_drive_map[slot_number].media.write_caching = FALSE;
  disk_drive_map[slot_number].media.io_align = 0x10;
  disk_drive_map[slot_number].media.last_block =  disk_drive_map[slot_number].sector_count - 1;

  /* info */
  printf_debug ("disk_drive_map[%x] addr=%x size=%lx\n", slot_number, (unsigned long)disk_drive_map[slot_number].start_sector,
          (unsigned long long)disk_drive_map[slot_number].sector_count);						//4cdc,4000000;	31512,2d0000;	2244,2d0000;				27b6c,168000;	4f9a3,1fffe00;
  printf_debug ("disk_drive_map[%x] blksize=%x lastblk=%lx\n", slot_number, disk_drive_map[slot_number].media.block_size,
          (unsigned long long)disk_drive_map[slot_number].media.last_block);//200,1ffff;		800,59f;			200,167f(800,59f);	200,b3f;			200,fffe;
  
  status = efi_call_6 (b->install_multiple_protocol_interfaces,	//安装多协议接口
                       &disk_drive_map[slot_number].from_handle,										//指向协议接口的指针(如果要分配新句柄，则指向NULL的指针)
                       &dp_guid, disk_drive_map[slot_number].dp,										//指向协议GUID的指针,指向设备路径的指针
                       &blk_io_guid, &disk_drive_map[slot_number].block_io, NULL);	//指向io设备接口的指针,指向block_io设备接口的指针,NULL 

  if(status != GRUB_EFI_SUCCESS)
  {
    printf_errinfo ("failed to install virtual partition\n");	//无法安装虚拟分区
    return GRUB_EFI_NOT_FOUND;
  }
	//此函数要读磁盘
  status = efi_call_4 (b->connect_controller, disk_drive_map[slot_number].from_handle, NULL, NULL, TRUE);	//引导服务->连接控制器,要连接驱动程序的控制器的句柄,驱动程序绑定协议的有序列表句柄的指针,指向设备路径的指针,如果为true则递归调用ConnectController（），

//填充磁盘数据
  floppies_orig++;
	d = grub_zalloc (sizeof (*d));  //分配内存, 并清零
	struct grub_disk_data	*d1;

	d->handle = disk_drive_map[slot_number].from_handle;
	d->device_path = disk_drive_map[slot_number].dp;
	d->last_device_path = 0;
	d->block_io = (grub_efi_block_io_t *)&disk_drive_map[slot_number].block_io;
	d->sector_size = disk_drive_map[slot_number].media.block_size;
	d->log2_sector = disk_drive_map[slot_number].from_log2_sector;
	d->total_sectors = disk_drive_map[slot_number].sector_count;
	d->drive = disk_drive_map[slot_number].from_drive;
	d->next = 0;

  if (!fd_devices)
    fd_devices = d;
  d1 = fd_devices;

	if (d1 != d)
	{
		for (; d1->next; d1 = d1->next);
		d1->next = d;
	}
  return GRUB_EFI_SUCCESS;
}

//获得文件设备路径,加载映像
grub_efi_handle_t vpart_load_image (grub_efi_handle_t *part_handle);
grub_efi_handle_t
vpart_load_image (grub_efi_handle_t *part_handle)	//虚拟分区引导
{
  grub_efi_status_t status;
  grub_efi_handle_t boot_image_handle = NULL;
  grub_efi_device_path_t *boot_file = NULL;
  grub_efi_boot_services_t *b;
  b = grub_efi_system_table->boot_services;

  if (!part_handle)
		return NULL;

  boot_file = grub_efi_file_device_path (grub_efi_get_device_path (part_handle),
                                         EFI_REMOVABLE_MEDIA_FILE_NAME);	//文件设备路径(设备路径, 可移动媒体文件名)  
  if (debug > 1)
    grub_efi_print_device_path(boot_file);
	//加载映像	将EFI映像加载到内存中  要读磁盘
  status = efi_call_6 (b->load_image, TRUE, grub_efi_image_handle,
                       boot_file, NULL, 0, &boot_image_handle);	//(void **)
  if (boot_file)
    grub_free (boot_file);
  if (status != GRUB_EFI_SUCCESS)
  {
    grub_printf ("Failed to load image\n");
			return NULL;
  }
  return boot_image_handle;
}

//获得uuid; 创建设备节点; 附加设备节点; 安装多协议接口; 连接控制器; 
grub_efi_status_t vdisk_install (int slot_number);
grub_efi_status_t
vdisk_install (int slot_number)	//安装虚拟磁盘(映射插槽号)
{
  grub_efi_status_t status;				//状态
  grub_efi_device_path_t *tmp_dp = 0;	//路径
  grub_efi_boot_services_t *b;		//引导服务
  b = grub_efi_system_table->boot_services;	//系统表->引导服务
  struct grub_disk_data	*d = grub_zalloc (sizeof (*d));  //分配内存, 并清零
  struct grub_disk_data	*p;
  char buf[256];
  int k;
  grub_efi_guid_t dp_guid = GRUB_EFI_DEVICE_PATH_GUID;	//设备路径GUID 
  grub_efi_guid_t blk_io_guid = GRUB_EFI_BLOCK_IO_GUID;	//块IO_GUID
  
  gen_uuid ();		//获得uuid
 
#if 1  
  grub_efi_device_pate_utilities_protocol_t *DPUP;  //设备路径实用程序协议
  grub_efi_guid_t dpup_guid = GRUB_EFI_DEVICE_PATH_UTILITIES_PROTOCOL_GUID;
  DPUP = grub_efi_locate_protocol (&dpup_guid, 0);  //EFI定位协议
  //创建设备节点
  tmp_dp = (grub_efi_device_path_t *)efi_call_3 (DPUP->CreateDeviceNode,
                      GRUB_EFI_HARDWARE_DEVICE_PATH_TYPE,               //0x01  硬件设备路径类型
                      GRUB_EFI_VENDOR_DEVICE_PATH_SUBTYPE,	            //0x04	供应商设备路径子类型
                      sizeof(grub_efi_vendor_device_path_t));		        //节点尺寸

  guidcpy ((grub_packed_guid_t *)&((grub_efi_vendor_device_path_t *)tmp_dp)->vendor_guid, (grub_packed_guid_t *)&VDISK_GUID);	//复制guid
  //附加设备节点
  disk_drive_map[slot_number].dp = (grub_efi_device_path_t *)efi_call_2 (DPUP->AppendDeviceNode,
                      NULL,	                                //设备路径
                      tmp_dp);                              //设备节点

  status = efi_call_1 (b->free_pool, tmp_dp);	//释放数据  使用DPUP->CreateDeviceNode创建的tmp_dp只能使用b->free_pool释放
#else
  tmp_dp = grub_efi_create_device_node (GRUB_EFI_HARDWARE_DEVICE_PATH_TYPE, GRUB_EFI_VENDOR_DEVICE_PATH_SUBTYPE,	//0x01,0x04	硬件设备路径类型,供应商设备路径子类型
                                        sizeof(grub_efi_vendor_device_path_t));		//创建设备节点
                                     
  guidcpy ((grub_packed_guid_t *)&((grub_efi_vendor_device_path_t *)tmp_dp)->vendor_guid, (grub_packed_guid_t *)&VDISK_GUID);	//复制guid
  disk_drive_map[slot_number].dp = grub_efi_append_device_node (NULL, tmp_dp);	//附加设备节点
 
  if (tmp_dp)	            //如果存在    使用grub_efi_create_device_node创建的tmp_dp可以释放; 使用DPUP->CreateDeviceNode创建的tmp_dp,可能被DPUP->AppendDeviceNode释放,
    grub_free (tmp_dp);   //因此再释放报错!!!
#endif

  /* vdisk 虚拟磁盘*/
  disk_drive_map[slot_number].from_handle = NULL;	//句柄
  /* block_io 块io*/
  grub_memcpy (&disk_drive_map[slot_number].block_io, &blockio_template, sizeof (block_io_protocol_t));
  block_io_protocol_this = (grub_size_t)&disk_drive_map[slot_number].block_io;
  /* media 媒体*/
  disk_drive_map[slot_number].block_io.media = &disk_drive_map[slot_number].media;		//媒体地址
	disk_drive_map[slot_number].media.media_id = disk_drive_map[slot_number].from_drive;
  disk_drive_map[slot_number].media.removable_media = FALSE;		//可移动媒体
  disk_drive_map[slot_number].media.media_present = TRUE;				//媒体展示 
  disk_drive_map[slot_number].media.logical_partition = FALSE;	//逻辑分区
  disk_drive_map[slot_number].media.write_caching = FALSE;			//写缓存 
  disk_drive_map[slot_number].media.io_align = 0x10;						//对齐
  disk_drive_map[slot_number].media.last_block = disk_drive_map[slot_number].sector_count - 1;//最后块

  /* info 打印信息*/
  printf_debug ("disk_drive_map addr=%x size=%lx\n", (unsigned int)disk_drive_map[slot_number].start_sector,
          (unsigned long long)disk_drive_map[slot_number].sector_count);
  printf_debug ("disk_drive_map blksize=%x lastblk=%lx\n", disk_drive_map[slot_number].media.block_size,
          (unsigned long long)disk_drive_map[slot_number].media.last_block);
          
//填充磁盘数据
	d->device_path = disk_drive_map[slot_number].dp;
	d->last_device_path = 0;
	d->block_io = (grub_efi_block_io_t *)&disk_drive_map[slot_number].block_io;
	d->sector_size = disk_drive_map[slot_number].media.block_size;
	d->log2_sector = disk_drive_map[slot_number].from_log2_sector;
	d->total_sectors = disk_drive_map[slot_number].sector_count;
	d->drive = disk_drive_map[slot_number].from_drive;
	d->next = 0;
	if (disk_drive_map[slot_number].from_drive >= 0xa0)
	{
		cdrom_orig++;
		if (!cd_devices)
			cd_devices = d;
		p = cd_devices;
	}
	else if (disk_drive_map[slot_number].from_drive >= 0x80)
	{
		harddrives_orig++;
		if (!hd_devices)
			hd_devices = d;
		p = hd_devices;
	}
	else
	{
		floppies_orig++;
		if (!fd_devices)
			fd_devices = d;
		p = fd_devices;
	}
	
	if (p != d)
	{
		for (; p->next; p = p->next);
		p->next = d;
	}
	if (disk_drive_map[slot_number].from_drive >= 0x80 && disk_drive_map[slot_number].from_drive <= 0x8f)
	{
		free_part_data (partition_info);
		partition_info = 0;
		partition_info_init ();
	}

  if (disk_drive_map[slot_number].from_drive >= 0xa0)
  {
    get_efi_cdrom_device_boot_path(disk_drive_map[slot_number].from_drive);
    grub_sprintf (buf, "(0x%X)0x%lX+0x%lX %s", disk_drive_map[slot_number].from_drive, part_addr, part_size,
                      (filesystem_type == 0x100 ? "(hd)" : "(fd)"));
    no_install_vdisk = 1;  //0/1=安装虚拟磁盘/不安装虚拟磁盘
    k = map_func (buf, 1);
    no_install_vdisk = 0;
    struct grub_disk_data	*d1 = get_device_by_drive (disk_drive_map[slot_number].from_drive);
    vpart_install (k >> 8, d1->device_path, 0);				    //安装虚拟分区
  }

  status = efi_call_6 (b->install_multiple_protocol_interfaces,	//安装多协议接口
                          &disk_drive_map[slot_number].from_handle,										//指向协议接口的指针(如果要分配新句柄，则指向NULL的指针)
                          &dp_guid, disk_drive_map[slot_number].dp,										//指向协议GUID的指针,指向设备路径的指针
                          &blk_io_guid, &disk_drive_map[slot_number].block_io, NULL);	//指向io设备接口的指针,指向block_io设备接口的指针,NULL                         
  //刚刚获得from_handle
	d->handle = disk_drive_map[slot_number].from_handle;
  if (status != GRUB_EFI_SUCCESS)	//安装失败
  {
    printf_errinfo ("failed to install virtual disk\n");	//无法安装虚拟磁盘 
    return status;
  }

	//此函数要读磁盘
  status = efi_call_4 (b->connect_controller, disk_drive_map[slot_number].from_handle, NULL, NULL, TRUE);	//引导服务->连接控制器,要连接驱动程序的控制器的句柄,驱动程序绑定协议的有序列表句柄的指针,指向设备路径的指针,如果为true则递归调用ConnectController（），

	return GRUB_EFI_SUCCESS;
}

static grub_efi_status_t (EFIAPI *orig_locate_handle)
                      (grub_efi_locate_search_type_t search_type,
                       grub_efi_guid_t *protocol,
                       void *search_key,
                       grub_efi_uintn_t *buffer_size,
                       grub_efi_handle_t *buffer) = NULL;

static int
compare_guid (grub_efi_guid_t *a, grub_efi_guid_t *b)
{
  int i;
  if (a->data1 != b->data1 || a->data2 != b->data2 || a->data3 != b->data3)
    return 0;
  for (i = 0; i < 8; i++)
  {
    if (a->data4[i] != b->data4[i])
      return 0;
  }
  return 1;
}

static grub_efi_handle_t saved_handle;

static grub_efi_status_t EFIAPI
locate_handle_wrapper (grub_efi_locate_search_type_t search_type,
                       grub_efi_guid_t *protocol,
                       void *search_key,
                       grub_efi_uintn_t *buffer_size,
                       grub_efi_handle_t *buffer)
{
  grub_efi_uintn_t i;
  grub_efi_handle_t handle = NULL;
  grub_efi_status_t status = GRUB_EFI_SUCCESS;
  grub_efi_guid_t guid = GRUB_EFI_BLOCK_IO_GUID;

  status = efi_call_5 (orig_locate_handle, search_type,
                       protocol, search_key, buffer_size, buffer);

  if (status != GRUB_EFI_SUCCESS || !protocol)
    return status;

  if (!compare_guid (&guid, protocol))
    return status;

  for (i = 0; i < (*buffer_size) / sizeof(grub_efi_handle_t); i++)
  {
    if (buffer[i] == saved_handle)  //11328c18
    {
      handle = buffer[0];     //handle=11b3d398
      buffer[0] = buffer[i];  //buffer[0]=11328c18
      buffer[i] = handle;     //buffer[i]=11b3d398
      break;
    }
  }

  return status;
}



//定位句柄缓冲,获得文件设备路径,加载映像
grub_efi_handle_t vdisk_load_image (unsigned int drive);	//虚拟磁盘引导
grub_efi_handle_t
vdisk_load_image (unsigned int drive)	//虚拟磁盘引导
{
  grub_efi_status_t status;
  grub_efi_uintn_t count = 0;	//计数
  grub_efi_device_path_t *tmp_dp;
  grub_efi_device_path_t *boot_file = NULL;
  grub_efi_handle_t boot_image_handle = NULL;
  struct grub_disk_data	*d;
  unsigned char type;
  unsigned char subtype = 0;
  unsigned char device_type = 0;
  grub_efi_boot_services_t *b;
  b = grub_efi_system_table->boot_services;	//系统表->引导服务

  if (drive >= 0xa0)
  {
    //windows启动cdrom时，只启动第一个cdrom，因此如果有多个cdrom，必须把要启动的cdrom移动到第一位。
    d = get_device_by_drive (drive);
    saved_handle = d->handle;

    if (!orig_locate_handle)
    {
      orig_locate_handle = (void *) b->locate_handle;
      b->locate_handle = (void *) locate_handle_wrapper;
    }
    type = GRUB_EFI_MEDIA_DEVICE_PATH_TYPE;         //4
    subtype = GRUB_EFI_CDROM_DEVICE_PATH_SUBTYPE;   //2
    get_efi_cdrom_device_boot_path (drive);
    device_type = 1;
  }
  else if (drive >= 0x80 && drive <= 0x8f)
  {
    type = GRUB_EFI_MEDIA_DEVICE_PATH_TYPE;             //4
    subtype = GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE;  //1
    get_efi_hd_device_boot_path (drive);
    device_type = 2;
  }

#if 1
  int i;
  grub_efi_handle_t *buf = NULL;	//缓存
  grub_efi_guid_t sfs_protocol_guid = GRUB_EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;	//简单文件系统协议GUID
  
  status = efi_call_5 (b->locate_handle_buffer, GRUB_EFI_BY_PROTOCOL,		//引导服务->定位句柄缓冲.通过协议
                              &sfs_protocol_guid, NULL, &count, &buf);

  if(status != GRUB_EFI_SUCCESS)	//如果失败
  {
    grub_printf ("SimpleFileSystemProtocol not found.\n");	//未找到简单文件系统协议
    return NULL;
  }
  for (i = 0; i < count; i++)
	{
    tmp_dp = grub_efi_get_device_path (buf[i]);		//获得设备路径

#else
  grub_efi_handle_t *handles;		//句柄集	api返回
  grub_efi_handle_t *handle;		//句柄

  /* Find handles which support the disk io interface. 查找支持磁盘IO接口的句柄 */
  handles = grub_efi_locate_handle (GRUB_EFI_BY_PROTOCOL, &block_io_guid,
				    0, &count); //定位句柄  返回句柄集及句柄数
  if (! handles)							//如果没有句柄集
    return 0;
  for (handle = handles; count--; handle++)
	{
    tmp_dp = grub_efi_get_device_path (*handle);		    //获得设备路径 

#endif
    tmp_dp = grub_efi_find_last_device_path (tmp_dp);   //查找最后设备路径
    if (debug > 1)
      grub_efi_print_device_path(tmp_dp);
             
    if (((grub_efi_vendor_device_path_t*)tmp_dp)->header.type			 	//供应商设备路径->句柄.类型, 不是硬件设备路径类型 1
            != type ||
        ((grub_efi_vendor_device_path_t*)tmp_dp)->header.subtype			//或者供应商设备路径->句柄.子类型, 不是供应商设备路径子类型 4
            != subtype ||
        (device_type == 1 ?
        ((((grub_efi_cdrom_device_path_t*)tmp_dp)->partition_start != part_addr || ((grub_efi_cdrom_device_path_t*)tmp_dp)->partition_size != part_size))
        :
        (((grub_efi_hard_drive_device_path_t*)tmp_dp)->partition_start != part_addr || ((grub_efi_hard_drive_device_path_t*)tmp_dp)->partition_size != part_size)))
    {
      continue;	//继续
    }

#if 1
//buf[i]包含分区路径，而d->device_path或者grub_efi_get_device_path (d->handle)不包含分区路径
    boot_file = grub_efi_file_device_path (grub_efi_get_device_path (buf[i]),
                                           EFI_REMOVABLE_MEDIA_FILE_NAME);	//文件设备路径(获得设备路径,可移动媒体文件名)  "/EFI/BOOT/bootx64.EFI"

#else
    boot_file = grub_efi_file_device_path (grub_efi_get_device_path (*handle),
                                           EFI_REMOVABLE_MEDIA_FILE_NAME);	//文件设备路径(获得设备路径,可移动媒体文件名)  "/EFI/BOOT/boot

#endif
    if (debug > 1)
      grub_efi_print_device_path(boot_file);

		//加载映像	将EFI映像加载到内存中  要读磁盘
    status = efi_call_6 (b->load_image, TRUE, 			//启动策略. 如果为true，则表示请求来自引导管理器，并且引导管理器正尝试将设备路径作为引导选择加载 
													grub_efi_image_handle,		//调用方的映像句柄. 此字段用于为正在加载的映像初始化EFI加载的映像协议的父句柄字段。
													boot_file, 								//从中加载映像的设备句柄特定文件路径
													NULL, 										//缓存指针. 如果不为空，则是指向包含要加载的映像副本的内存位置的指针。
													0,												//缓存字节尺寸。如果缓存为空，则忽略
													&boot_image_handle);			//指向成功加载映像时创建的返回映像句柄的指针。(void **)

    if (status != GRUB_EFI_SUCCESS)	//失败
    {
      if (boot_file)
        grub_free (boot_file);
      continue;
    }
    break;
  }

  if (!boot_image_handle)
  {
    grub_printf ("boot_image_handle not found\n");
    return NULL;
  }

  return boot_image_handle;	//返回映像句柄的指针
}




grub_efi_status_t blockio_read_write (block_io_protocol_t *this, grub_efi_uint32_t media_id,
              grub_efi_lba_t lba, grub_efi_uintn_t len, void *buf, int read_write);
grub_efi_status_t
blockio_read_write (block_io_protocol_t *this, grub_efi_uint32_t media_id,
              grub_efi_lba_t lba, grub_efi_uintn_t len, void *buf, int read_write)	//读(自身,媒体id,起始逻辑扇区,读字节尺寸,缓存)
{
  grub_efi_status_t status;			//状态
  grub_efi_uintn_t block_num;
  unsigned int offset, read_len;
  unsigned long long read_start, lba_byte;
	unsigned long long fragment_len = 0, total = 0; 
  struct fragment_map_slot *q;
	struct fragment *data=0;
	unsigned int i, j=0;
	unsigned char	from_drive, to_drive;			//驱动器
  
  if (read_write != 0xedde0d90 && read_write != 0x900ddeed) //如果不是读/写, 错误
    return GRUB_EFI_DEVICE_ERROR;

  if (!buf)	//如果缓存为零
    return GRUB_EFI_INVALID_PARAMETER;

  if (!len)	//如果尺寸为零
    return GRUB_EFI_SUCCESS;

  from_drive = media_id;

  //如果尺寸未对齐扇区块
  if ((len % this->media->block_size) != 0)
    return GRUB_EFI_BAD_BUFFER_SIZE;//BufferSize参数不是设备内部块大小的倍数。 

  block_num = len / this->media->block_size;
  //如果读写数据越界  可以包含"如果扇区号越界"
  if ((lba + block_num - 1) > this->media->last_block)
    return GRUB_EFI_INVALID_PARAMETER;//读取请求包含无效的lba，或者缓冲区未正确对齐。 

#if 0
  //由form驱动器查找to驱动器
  //drive_map_slot_empty(disk_drive_map[i] 在这里引起数据竞争!!!
  //现象1: 总是插槽为空.
  //现象2: 如果增加简单的打印信息, 则插槽有效非空, disk_drive_map[i].from_drive=0x81(正确), disk_drive_map[i].to_drive应当等于0x80, 然而却是等于0!!!
  //如果增加比较复杂的打印信息, 如: grub_printf("\nlockio_read_write-0,%x,%x,%x,%x,%x,",this,media_id,lba,len,buf); ,则全部正确.
  //在此处增加延迟函数, 不起作用. 
  to_drive = 0xaa;
	for (i = 0; i < (unsigned int)DRIVE_MAP_SIZE/* && !(drive_map_slot_empty(disk_drive_map[i]))*/; i++)		//如果i<8,并且插槽有效非空
	{
		if (disk_drive_map[i].from_drive != from_drive)  //如果插槽from驱动器号不等于输入驱动器,继续
			continue;
		to_drive = disk_drive_map[i].to_drive;
		break;
	}

	if (to_drive == 0xaa)
		return GRUB_EFI_MEDIA_CHANGED;
#endif

  i = 0;
  while (i < (unsigned int)DRIVE_MAP_SIZE && disk_drive_map[i].from_drive != from_drive)
    i++;
  
  if (i == (unsigned int)DRIVE_MAP_SIZE)
    return GRUB_EFI_MEDIA_CHANGED;
  else
    to_drive = disk_drive_map[i].to_drive;

//假设:
//参数           from_log2_sector=b,	from_block_size=800, to_log2_sector=9, to_block_size=200
//from映射到to   13ae10+95940, 1dc8d0+180
//读参数         lba=25640	len=10000

	lba_byte = lba << disk_drive_map[i].from_log2_sector;	//from驱动器起始逻辑扇区lba转起始字节	12aac800

  //内存驱动器	
	if (to_drive == 0xff && disk_drive_map[i].to_log2_sector == 9)			//如果是内存驱动器, 映射盘加载到内存, 
	{
		lba_byte += (disk_drive_map[i].start_sector << 9);		//加映射起始(字节)
    if (read_write == 0xedde0d90) //读
      grub_memmove64 ((unsigned long long)(grub_size_t)buf, lba_byte, len);
    else
      grub_memmove64 (lba_byte, (unsigned long long)(grub_size_t)buf, len);
		return 0;
	}

	//确定to驱动器起始相对逻辑扇区号
	lba = lba_byte >> disk_drive_map[i].to_log2_sector;		      //95564	
	//确定to驱动器起始偏移字节
	offset = lba_byte & (disk_drive_map[i].to_block_size - 1);	//12aac800&fff=0

  //判断有无碎片
	if (!disk_drive_map[i].fragment)		//没有碎片    3f9+8, 641+1b30
	{
		lba += disk_drive_map[i].start_sector;			//加映射起始(扇区或字节)		4cc0		
    fragment_len = len;
	}
  else
//有碎片
//			 To_count(0)			To_count(1)					To_count(2)					To_count(3)
//		├──────────────┼───────────────────┼───────────────────┼───────────────────┤		逻辑扇区  从To_start(0)起始,扇区不连续
//To_start(0)		To_start(1)					To_start(2)					To_start(3)
//																									Form_len
//	  ├---------------------------------------├----------------------┤								虚拟扇区  从0起始(相对于逻辑扇区To_start(0)),扇区连续  
//	 0																		Form_statr

//		|-----------8-----------|------4-----|----------7---------|
//    0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
//		|-------------9------------|---------6--------|
//	  0											    lba
  {
    //从碎片插槽查找Form驱动器
    q = &disk_fragment_map;
    q = fragment_map_slot_find (q, from_drive);

    //确定Form扇区起始在哪个碎片										//4cc0+6, 6673+3b0
    data = (struct fragment *)&q->fragment_data;

    while (1)     //3f9+8,641+1b30
    {
      total += data[j++].sector_count;            //6
      if (lba < total)		//确定起始位置的条件    //4<6 
        break;
    }

    //确定本碎片最大访问字节
    fragment_len = (total - lba) << disk_drive_map[i].to_log2_sector;
    //确定Form扇区起始的确切位置(To_start(j)+偏移)
    lba += data[j-1].start_sector + data[j-1].sector_count - total;
  }

  while (len)
  {
    //判断本碎片可否一次访问完毕
    if (len > fragment_len)
      read_len = fragment_len;  
    else
      read_len = len;

    //首先处理偏移, 实际读写字节(disk_drive_map[i].to_block_size - offset)
    if (offset || len < disk_drive_map[i].to_block_size)  //如果有偏移字节
    {
      status = grub_efidisk_readwrite (to_drive, lba, disk_drive_map[i].to_block_size, disk_buffer, 0xedde0d90);
      read_start = (unsigned long long)(grub_size_t)disk_buffer + offset;         //读写起始
      read_len = disk_drive_map[i].to_block_size - offset;                         //读写尺寸   
      if (read_len > len)
        read_len = len;
      
      if (read_write == 0xedde0d90) //读
        grub_memmove64 ((unsigned long long)(grub_size_t)buf, read_start, read_len);
      else              //写
      {
        grub_memmove64 (read_start, (unsigned long long)(grub_size_t)buf, read_len);
        status = grub_efidisk_readwrite (to_drive, lba, disk_drive_map[i].to_block_size, disk_buffer, 0x900ddeed);
      }
      
      buf = (void *)((grub_size_t)buf + read_len);
      fragment_len -= read_len;
      len -= read_len;
      offset = 0;
    }
    else
    {
      read_len &= ~(disk_drive_map[i].to_block_size -1);
      len -= read_len;
      status = grub_efidisk_readwrite (to_drive, lba, read_len, buf, read_write);
      buf = (void *)((grub_size_t)buf + read_len);
      fragment_len -= read_len;
    }

    //判断是否需要再读
    if (!len)
      return status;
    //确定本碎片还可以访问扇区数
    if (disk_drive_map[i].fragment && !fragment_len)  //肯定有碎片, 否则fragment_len=len, 既然len不为零, 则fragment_len也不会为零.
    {
      lba = data[j].start_sector;
      fragment_len = data[j].sector_count << disk_drive_map[i].to_log2_sector;
      j++;
    }
    else  //不能确定有无碎片
    {
      lba += (read_len + disk_drive_map[i].to_block_size - 1) >> disk_drive_map[i].to_log2_sector;
    }      
  }
  return GRUB_EFI_DEVICE_ERROR;
}

/*
读
GRUB_EFI_SUCCESS							数据已从设备中正确读取。
GRUB_EFI_DEVICE_ERROR					设备在尝试执行读取操作时报告了一个错误。
GRUB_EFI_NO_MEDIA							设备中没有媒体。 
GRUB_EFI_MEDIA_CHANGED				MediaId不适用于当前媒体。
GRUB_EFI_BAD_BUFFER_SIZE			BufferSize参数不是设备内部块大小的倍数。
GRUB_EFI_INVALID_PARAMETER		读取请求包含无效的lba，或者缓冲区未正确对齐。

写
GRUB_EFI_SUCCESS							数据已正确写入设备。 
GRUB_EFI_DEVICE_ERROR					设备在尝试执行写入操作时报告了一个错误。
GRUB_EFI_NO_MEDIA							设备中没有媒体。
GRUB_EFI_MEDIA_CHANGED				MediaId不适用于当前媒体。 
GRUB_EFI_BAD_BUFFER_SIZE			BufferSize参数不是设备内部块大小的倍数。 
GRUB_EFI_INVALID_PARAMETER		写入请求包含无效的lba，或者缓冲区未正确对齐。
GRUB_EFI_WRITE_PROTECTED			无法写入设备。
*/

static grub_efi_status_t EFIAPI blockio_reset (block_io_protocol_t *this,grub_efi_boolean_t extended);
static grub_efi_status_t EFIAPI
blockio_reset (block_io_protocol_t *this,
               grub_efi_boolean_t extended)
{
  return GRUB_EFI_SUCCESS;
}

static grub_efi_status_t EFIAPI blockio_read (block_io_protocol_t *this, grub_efi_uint32_t media_id,
              grub_efi_lba_t lba, grub_efi_uintn_t len, void *buf);
static grub_efi_status_t EFIAPI
blockio_read (block_io_protocol_t *this, grub_efi_uint32_t media_id,
              grub_efi_lba_t lba, grub_efi_uintn_t len, void *buf)
{
  return blockio_read_write (this, media_id, lba, len, buf, 0xedde0d90);
}

static grub_efi_status_t EFIAPI blockio_write (block_io_protocol_t *this, grub_efi_uint32_t media_id,
              grub_efi_lba_t lba, grub_efi_uintn_t len, void *buf);
static grub_efi_status_t EFIAPI
blockio_write (block_io_protocol_t *this, grub_efi_uint32_t media_id,
              grub_efi_lba_t lba, grub_efi_uintn_t len, void *buf)
{
  return blockio_read_write (this, media_id, lba, len, buf, 0x900ddeed);
}

static grub_efi_status_t EFIAPI blockio_flush (block_io_protocol_t *this);
static grub_efi_status_t EFIAPI
blockio_flush (block_io_protocol_t *this)
{
  return GRUB_EFI_SUCCESS;
}

#define EFI_BLOCK_IO_PROTOCOL_REVISION 0x00010000

block_io_protocol_t blockio_template =
{
  EFI_BLOCK_IO_PROTOCOL_REVISION,
  (grub_efi_block_io_media_t *) 0,
  blockio_reset,
  blockio_read,
  blockio_write,
  blockio_flush
};

#if 0
static EFI_LOCATE_HANDLE g_org_locate_handle = NULL;

/* Fixup the 1st cdrom influnce for Windows boot 修复Windows引导的第一个cdrom影响 */
static grub_efi_status_t EFIAPI grub_wrapper_locate_handle //包装器定位句柄
      (grub_efi_locate_search_type_t search_type,
        grub_efi_guid_t *protocol,
		    void *search_key,
		    grub_efi_uintn_t *buffer_size,
		    grub_efi_handle_t *buffer); 
{
  UINTN i;
  EFI_HANDLE Handle = NULL;
  EFI_STATUS Status = EFI_SUCCESS;

  Status = g_org_locate_handle(SearchType, Protocol, SearchKey, BufferSize, Buffer);  //定位句柄 

  if (EFI_SUCCESS == Status && Protocol &&              //没有错误,协议存在
      CompareGuid(&gEfiBlockIoProtocolGuid, Protocol))  //比较Guid
  {
    for (i = 0; i < (*BufferSize) / sizeof(EFI_HANDLE); i++)  //
    {
      if (Buffer[i] == gBlockData.Handle) //如果等于块数据.句柄, 则调换句柄到第一位置
      {
        Handle = Buffer[0];
        Buffer[0] = Buffer[i];
        Buffer[i] = Handle;
        break;
      }
    }
  }
  return Status;
}

EFI_STATUS grub_hook_1st_cdrom_start(VOID)  //钩第一光盘开始
{
  g_org_locate_handle = gBS->LocateHandle;
  gBS->LocateHandle = ventoy_wrapper_locate_handle; //包装器定位句柄 
  return EFI_SUCCESS;
}

EFI_STATUS grub_hook_1st_cdrom_stop(VOID)  //钩第一光盘结束 
{
  gBS->LocateHandle = g_org_locate_handle;
  g_org_locate_handle = NULL;
  return EFI_SUCCESS;
}
#endif



void GRUB_MOD_INIT_efinet(void);
int force_pxe_as_boot_device = 0;
void grub_efidisk_init (void);
void
grub_efidisk_init (void)  //efidisk初始化		
{
	grub_efidisk_fini();
  enumerate_disks (); //枚举磁盘
	partition_info_init ();

#if i386
	is64bit = check_64bit_and_PAE ();
#endif

  if (!image)
    return;
  
	struct grub_disk_data *d = NULL;
  
/* image->device_handle是装载映像句柄，通过它可以获得起始驱动器。但是它不一定等于grub_disk_data->handle。
  qemu启动一个设备，image->device_handle = grub_disk_data->handle.
  qemu启动一个光盘，再附加一个硬盘，image->device_handle即不等于光盘的grub_disk_data->handle，也不等于硬盘的grub_disk_data->handle。
  但是由image->device_handle获得设备路径，与光盘的设备路径相同。
*/
  /* Set root drive and partition.  设置根驱动器和分区。*/
  grub_efi_device_path_t *dp, *ldp;	//设备路径
  dp = grub_efi_get_device_path (image->device_handle);	//获得设备路径

  if (! dp)	//如果为零, 错误
    return;
  if (debug > 1)
    grub_efi_print_device_path(dp);

//03 0b 25 00 00 0c 29 8d - cc d9 00 00 00 00 00 00	网络
//00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00
//00 00 00 00 00 7f ff 04 - 00 
//

	grub_efi_device_path_t *dup_ldp = 0;
  ldp = grub_efi_find_last_device_path (dp);	//查找最后设备路径

//03 0b 25 00 00 0c 29 8d - cc d9 00 00 00 00 00 00	网络
//00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00
//00 00 00 00 00 7f ff 04 - 00 
//硬盘
//					 |hid         |uid						ACPI设备路径类型,ACPI设备路径子类型    注: 这是电脑真实的设备路径!!
//02 01 0c 00 d0 41 03 0a - 00 00 00 00 	
//           |功能 |设备									硬件设备路径类型,PCI设备路径子类型         
//01 01 06 00 01    01
//  				 |主次 |从主 |逻辑单元			  消息传递设备路径类型,ATAPI设备路径子类型    注: 这是电脑真实的设备路径!!
//03 01 08 00 00    00    00 00
//           |分区号      |分区起始								  |分区尺寸                 |分区签名																					  |分区格式类型/签名类型		媒体设备路径类型,硬盘驱动器设备路径子类型  注: 这是UEFI提供的设备路径
//04 01 2a 00 01 00 00 00 - 3f 00 00 00 00 00 00 00 - c1 cf 01 00 00 00 00 00 - ab cd ef 12 00 00 00 00 - 00 00 00 00 00 00 00 00 - 01 01 7f ff 04 00
//04 01 2a 00 01 00 00 00 - 00 08 00 00 00 00 00 00 - 00 00 04 00 00 00 00 00 - b4 02 50 fc 4d 3c 8c 43 - bc 04 aa ef 87 c0 ae 24 - 02 02 7f ff 04 00

//#ifdef FSYS_PXE
	if (GRUB_EFI_DEVICE_PATH_TYPE (ldp) == GRUB_EFI_MESSAGING_DEVICE_PATH_TYPE								//如果最后设备路径类型是通讯设备路径 3
				&& GRUB_EFI_DEVICE_PATH_SUBTYPE (ldp) == GRUB_EFI_MAC_ADDRESS_DEVICE_PATH_SUBTYPE)	//并且最后设备路径子类型是MAC地址设备子路径 11
	{
		if (! ((*(char *)IMG(0x8205)) & 0x01))	/* if it is not disable pxe 如果没有禁用pxe */
		{
			pxe_init ();
#if 0
#ifdef FSYS_IPXE
		ipxe_init();
#endif
#endif
		/* on pxe boot, we only use preset_menu 在pxe启动时，我们只使用预设菜单*/
			boot_drive = PXE_DRIVE;	//0x21
#if 0
#ifdef FSYS_IPXE
		char *ch = grub_strstr((char*)discover_reply->bootfile,":");
		if (ch && ((grub_u32_t)ch - (grub_u32_t)discover_reply->bootfile) < 10)
				install_partition = IPXE_PART;	//0x45585069
		else
#endif
#endif
			saved_drive = boot_drive;
			current_drive = boot_drive;
			install_partition = 0xFFFFFF;
			saved_partition = install_partition;
			current_partition = install_partition;
			run_line((char *)"set ?_BOOT=%@root%",1);
			QUOTE_CHAR = '\"';	
			cmain ();
		}
	}
//#endif /* FSYS_PXE */

  if (GRUB_EFI_DEVICE_PATH_TYPE (ldp) == GRUB_EFI_MEDIA_DEVICE_PATH_TYPE								//如果最后设备路径类型=4
      && (GRUB_EFI_DEVICE_PATH_SUBTYPE (ldp) == GRUB_EFI_CDROM_DEVICE_PATH_SUBTYPE			//并且最后设备路径子类型=2或者1		光盘/硬盘
			|| GRUB_EFI_DEVICE_PATH_SUBTYPE (ldp) == GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE))
	{
		if (GRUB_EFI_DEVICE_PATH_SUBTYPE (ldp) == GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE)	//如果是硬盘, 获取安装分区
			part_start = ((grub_efi_hard_drive_device_path_t *)ldp)->partition_start;	//分区起始

		/* It is necessary to duplicate the device path so that GRUB  必须复制设备路径，以便GRUB可以重写它。
			can overwrite it.  */
		grub_efi_device_path_t *dup_dp;
		dup_dp = grub_efi_duplicate_device_path (dp);	//复制设备路径
		if (! dup_dp)	//如果为零, 错误
			return;

//03 0b 25 00 00 0c 29 8d - cc d9 00 00 00 00 00 00	网络
//00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00
//00 00 00 00 00 7f ff 04 - 00 

		while (1)
		{
			dup_ldp = grub_efi_find_last_device_path (dup_dp);		//查找最后设备路径
			if (!(GRUB_EFI_DEVICE_PATH_TYPE (dup_ldp) == GRUB_EFI_MEDIA_DEVICE_PATH_TYPE								//如果最后设备路径不是(类型=4
					&& (GRUB_EFI_DEVICE_PATH_SUBTYPE (dup_ldp) == GRUB_EFI_CDROM_DEVICE_PATH_SUBTYPE				//并且最后设备路径子类型=2或者1)		非光盘/非硬盘
					|| GRUB_EFI_DEVICE_PATH_SUBTYPE (dup_ldp) == GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE)))
				break;		//退出

			dup_ldp->type = GRUB_EFI_END_DEVICE_PATH_TYPE;	//设备路径节点的结束	7f
			dup_ldp->subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;	//0xff
			dup_ldp->length = sizeof (*dup_ldp);
		}
		grub_free (dup_dp);
	}

//03 0b 25 00 00 0c 29 8d - cc d9 00 00 00 00 00 00	网络
//00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00
//00 00 00 00 00 7f ff 04 - 00 

	int i;
  int ret;
	for (i=0; i<0xff; i++)
	{
		d = get_device_by_drive (i);
		if (!d)
			continue;
		ret = grub_efi_compare_device_paths (grub_efi_find_last_device_path (d->device_path),
					grub_efi_find_last_device_path (dup_ldp));  //比较devices与d的设备路径	返回: 0/非0=成功/失败
		if (ret == 0)	//如果相同
			break;
	}
	boot_drive = i;
	struct grub_part_data *fq;
	for (fq = partition_info; fq; fq = fq->next)
	{
		if (fq->drive == boot_drive && fq->partition_start == part_start)
			install_partition = fq->partition;
	}			

//初始值: boot_drive=current_drive=0xFFFFFFFF  saved_drive=0
//初始值: install_partition=0x00FFFFFF  saved_partition=current_partition=0
	saved_drive = boot_drive;
	current_drive = boot_drive;
	saved_partition = install_partition;
	current_partition = install_partition;
  
  debug = 1;	
//初始化变量空间	
  run_line((char *)"set ?_BOOT=%@root%",1);
  QUOTE_CHAR = '\"';	

	run_line((char *)"errorcheck off",1);
//	run_line((char *)"configfile /menu.lst",1);
//	run_line((char *)"configfile /boot/grub/menu.lst",1);
//	run_line((char *)"configfile /grub/menu.lst",1);
  run_line((char *)"configfile /efi/grub/menu.lst",1);
	run_line((char *)"errorcheck on",1);

	cmain ();
}
