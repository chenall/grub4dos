/* bios.c - implement C part of low-level BIOS disk input and output */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2003,2004  Free Software Foundation, Inc.
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

#include "shared.h"
#include "iso9660.h"

/* These are defined in asm.S, and never be used elsewhere, so declare the
   prototypes here.  */
extern int biosdisk_standard (unsigned ah, unsigned drive,
			      unsigned coff, unsigned hoff, unsigned soff,
			      unsigned nsec, unsigned segment);
extern int get_diskinfo_standard (unsigned long drive,
				  unsigned long *cylinders,
				  unsigned long *heads,
				  unsigned long *sectors);

/* Read/write NSEC sectors starting from SECTOR in DRIVE disk with GEOMETRY
   from/into SEGMENT segment. If READ is BIOSDISK_READ, then read it,
   else if READ is BIOSDISK_WRITE, then write it. If an geometry error
   occurs, return BIOSDISK_ERROR_GEOMETRY, and if other error occurs, then
   return the error number. Otherwise, return 0.  */
int
biosdisk (unsigned long read, unsigned long drive, struct geometry *geometry,
	  unsigned long long sector, unsigned long nsec, unsigned long segment)
{
  int err;
  unsigned long max_sec, count, seg;
  unsigned long long start;

  if ((fb_status) && (drive == (unsigned char)(fb_status >> 8)))
    max_sec = (unsigned char)fb_status;
  else
    max_sec = nsec;
 
  /* first, use EBIOS if possible */
  if ((geometry->flags & BIOSDISK_FLAG_LBA_EXTENSION) && (! (geometry->flags & BIOSDISK_FLAG_BIFURCATE) || (drive & 0xFFFFFF00) == 0x100))
    {      
      struct disk_address_packet
      {
	unsigned char length;
	unsigned char reserved;
	unsigned short blocks;
	unsigned long buffer;
	unsigned long long block;
	
	/* This structure is passed in the stack. A buggy BIOS could write
	 * garbage data to the tail of the struct and hang the machine. So
	 * we need this protection. - Tinybit
	 */
	unsigned char dummy[16];
      } __attribute__ ((packed)) *dap;

      /* Even the above protection is not enough to avoid stupid actions by
       * buggy BIOSes. So we do it in the 0040:0000 segment. - Tinybit
       */
      dap = (struct disk_address_packet *)0x580;

      if (drive == 0xffff || (drive == ram_drive && rd_base != -1ULL))
      {
	unsigned long long disk_sector;
	unsigned long long buf_address;
	unsigned long long tmp;
	
	if (nsec <=0 || nsec >= 0x80)
		return 1;	/* failure */
	if (sector + nsec > geometry->total_sectors)
		return 1;	/* failure */
	//if ((unsigned long)sector + (unsigned long)nsec >= 0x800000)
	//	return 1;	/* failure */
	
	tmp = ((sector + nsec) << 9);
	if (drive == ram_drive)
	    tmp += rd_base;
	else
	    tmp += md_part_base;
	if (tmp > 0x100000000ULL && ! is64bit)
		return 1;	/* failure */
	disk_sector = ((sector<<9) + ((drive==0xffff) ? md_part_base : rd_base));
	buf_address = (segment<<4);

	if (read)	/* read == 1 really means write to DISK */
		grub_memmove64 (disk_sector, buf_address, nsec << 9);
	else		/* read == 0 really means read from DISK */
		grub_memmove64 (buf_address, disk_sector, nsec << 9);
		
	return 0;	/* success */
      }

      start = sector;
      count = nsec;
      seg = segment;
      do
	{
	  unsigned long n;

	  n = (count > max_sec) ? max_sec : count;
	  if (n > 127)
	      n = 127;

	  dap->length = 0x10;
	  dap->block = start;
	  dap->blocks = n;
	  dap->reserved = 0;
	  dap->buffer = seg << 16;

	  err = biosdisk_int13_extensions ((read + 0x42) << 8, (unsigned char)drive, dap, geometry->sector_size | (!!(geometry->flags & BIOSDISK_FLAG_LBA_1_SECTOR)));
	  start += n;
	  count -= n;
	  seg += n << 5;
	} while ((count) && (! err));

      if (!err)
	return 0;	/* success */

      /* bootable CD-ROM specification has no standard CHS-mode call */
      if (geometry->flags & BIOSDISK_FLAG_CDROM)
      {
	if (debug > 1)
	  grub_printf ("biosdisk_int13_extensions read=%d, drive=0x%x, dap=%x, err=0x%x\n", read, drive, dap, err);
	return err;
      }

      if (geometry->flags & BIOSDISK_FLAG_BIFURCATE)
	return err;

    } /* if (geometry->flags & BIOSDISK_FLAG_LBA_EXTENSION) */

   /* try the standard CHS mode */
  
    if (sector >> 32)	/* sector exceeding 32 bit, too big */
	return 1;	/* failure */
    {
      unsigned long cylinder_offset, head_offset, sector_offset;
      unsigned long head;

      /* SECTOR_OFFSET is counted from one, while HEAD_OFFSET and
	 CYLINDER_OFFSET are counted from zero.  */
      sector_offset = ((unsigned long)sector) % geometry->sectors + 1;
      head = ((unsigned long)sector) / geometry->sectors;
      head_offset = head % geometry->heads;
      cylinder_offset = head / geometry->heads;

      if (cylinder_offset > 1023)	/* cylinder too big */
		return 1;		/* failure */
      do
	{
	  unsigned long n;

	  n = (nsec > max_sec) ? max_sec : nsec;

	  /* we should avoid accessing sectors across track boundary. */
	  if (n > geometry->sectors - sector_offset + 1)
	      n = geometry->sectors - sector_offset + 1;

	  err = biosdisk_standard (read + 0x02, drive,
				   cylinder_offset, head_offset, sector_offset,
				   n, segment);
	  sector_offset += n;
	  nsec -= n;
	  segment += n << 5;
	} while ((nsec) && (! err));
    }

  return err;
}

/* Check bootable CD-ROM emulation status. Return 0 on failure. */
int
get_cdinfo (unsigned long drive, struct geometry *geometry)
{
  int err;
  struct iso_spec_packet
  {
    unsigned char size;
    unsigned char media_type;
    unsigned char drive_no;
    unsigned char controller_no;
    unsigned long image_lba;
    unsigned short device_spec;
    unsigned short cache_seg;
    unsigned short load_seg;
    unsigned short length_sec512;
    unsigned char cylinders;
    unsigned char sectors;
    unsigned char heads;
    
    unsigned char dummy[16];
  } __attribute__ ((packed));
  
  struct iso_spec_packet *cdrp;
  
  cdrp = (struct iso_spec_packet *)0x580;
  grub_memset (cdrp, 0, sizeof (struct iso_spec_packet));
  cdrp->size = sizeof (struct iso_spec_packet) - 16;

  if (debug > 1)
	grub_printf ("\rget_cdinfo int13/4B01(%X), ", drive);
  err = biosdisk_int13_extensions (0x4B01, drive, cdrp, 2048);
  if (debug > 1)
	grub_printf ("err=%X, ", err);

  if (drive == 0x7F && drive < (unsigned long)(cdrp->drive_no))
	drive = cdrp->drive_no;

  if (! err && cdrp->drive_no == drive && !(cdrp->media_type & 0x0F))
    {
	/* No-emulation mode bootable CD-ROM */
	geometry->flags = BIOSDISK_FLAG_LBA_EXTENSION | BIOSDISK_FLAG_CDROM;
	geometry->cylinders = 65536;
	geometry->heads = 255;
	geometry->sectors = 15;
	geometry->sector_size = 2048;
	geometry->total_sectors = 65536 * 255 * 15;
	if (debug > 1)
	  grub_printf ("drive=%d\n", drive);
	DEBUG_SLEEP
	return drive;
    }
  if (debug > 1) {
    if (err)
      grub_printf ("drive=0\n");
    else
      grub_printf ("\r%40s\r", " "); /* erase line if no err and drive are both 0 */
  }
  return 0;	/* failure */
}

static unsigned long flags;
static unsigned long cylinders;
static unsigned long heads;
static unsigned long sectors;
static unsigned long heads_ok;
static unsigned long sectors_ok;
unsigned long force_geometry_tune = 0;

/* Return the geometry of DRIVE in GEOMETRY. If an error occurs, return
   non-zero, otherwise zero.  */
int
get_diskinfo (unsigned long drive, struct geometry *geometry, unsigned long lba1sector)
{
  int err;
  int version;
  unsigned long long total_sectors = 0, tmp = 0;

  if (drive == 0xffff)	/* memory disk */
    {
      unsigned long long total_mem_sectors;

      total_mem_sectors = 0;

      if (mbi.flags & MB_INFO_MEM_MAP)
        {
          struct AddrRangeDesc *map = (struct AddrRangeDesc *) saved_mmap_addr;
          unsigned long end_addr = saved_mmap_addr + saved_mmap_length;

          for (; end_addr > (unsigned long) map; map = (struct AddrRangeDesc *) (((int) map) + 4 + map->size))
	    {
	      unsigned long long top_end;

	      if (map->Type != MB_ARD_MEMORY)
		  continue;
	      top_end =  map->BaseAddr + map->Length;
	      /* check overflow... */
	      if (top_end < map->BaseAddr && top_end < map->Length)
		  total_mem_sectors = 0x80000000000000ULL;
	      else
		{
		  if (top_end > 0x100000000ULL && ! is64bit)
		      top_end = 0x100000000ULL;
		  if (total_mem_sectors < (top_end >> SECTOR_BITS))
		      total_mem_sectors = (top_end >> SECTOR_BITS);
		}

	    }
        }
      else
	  grub_printf ("Address Map BIOS Interface is not activated.\n");

//      if (total_mem_sectors)
//      {
	geometry->flags = BIOSDISK_FLAG_LBA_EXTENSION;
	geometry->sector_size = SECTOR_SIZE;
	/* if for some reason(e.g., a bios bug) the memory is reported less than 1M(too few), then we suppose the memory is unlimited. */
	if (md_part_size)
		geometry->total_sectors = (md_part_size >> SECTOR_BITS) + !!(md_part_size & (SECTOR_SIZE - 1));
	else
		geometry->total_sectors = (total_mem_sectors < 0x800ULL ? 0x80000000000000ULL : total_mem_sectors);
	geometry->heads = 255;
	geometry->sectors = 63;
	geometry->cylinders = (geometry->total_sectors > (0xFFFFFFFFULL - 255 * 63 + 1) ? 0xFFFFFFFFUL / (255UL * 63UL):((unsigned long)geometry->total_sectors + 255 * 63 -1) / (255 * 63));
	return 0;
//      }
      
    } else if (drive == ram_drive)	/* ram disk device */
    {
      if (rd_base != -1ULL)
      {
	geometry->flags = BIOSDISK_FLAG_LBA_EXTENSION;
	geometry->sector_size = SECTOR_SIZE;
	geometry->total_sectors = (rd_size >> SECTOR_BITS) + !!(rd_size & (SECTOR_SIZE - 1));
	geometry->heads = 255;
	geometry->sectors = 63;
	geometry->cylinders = (geometry->total_sectors > (0xFFFFFFFFULL - 255 * 63 + 1) ? 0xFFFFFFFFUL / (255UL * 63UL):((unsigned long)geometry->total_sectors + 255 * 63 -1) / (255 * 63));
	return 0;
      }
    }

  if (drive == cdrom_drive || (drive >= (unsigned char)min_cdrom_id && drive < (unsigned char)(min_cdrom_id + atapi_dev_count)))
  {
	/* No-emulation mode bootable CD-ROM */
	geometry->flags = BIOSDISK_FLAG_LBA_EXTENSION | BIOSDISK_FLAG_CDROM;
	geometry->cylinders = 65536;
	geometry->heads = 255;
	geometry->sectors = 15;
	geometry->sector_size = 2048;
	geometry->total_sectors = 65536 * 255 * 15;
	return 0;
  }

  /* Clear the flags.  */
  flags = 0;
  heads_ok = 0;
  sectors_ok = 0;

#define FIND_DRIVES (*((char *)0x475))
      if (((unsigned char)drive) >= 0x80 + FIND_DRIVES /* || (version && (drive & 0x80)) */ )
	{
	  /* Possible CD-ROM - check the status.  */
	  if (get_cdinfo ((unsigned char)drive, geometry))
	    return 0;
	}
      
    //if (! force_geometry_tune)
    {
	unsigned long j;
	unsigned long d;

	/* check if the drive is virtual. */
	d = (unsigned char)drive;
	j = DRIVE_MAP_SIZE;		/* real drive */
	if (! unset_int13_handler (1))
	    for (j = 0; j < DRIVE_MAP_SIZE; j++)
	    {
		if (drive_map_slot_empty (hooked_drive_map[j]))
		{
			j = DRIVE_MAP_SIZE;	/* real drive */
			break;
		}

		if (((unsigned char)drive) != hooked_drive_map[j].from_drive)
			continue;

		/* this is a mapped drive */
		/* check cdrom emulation first */
		if (hooked_drive_map[j].to_cylinder & 0x2000) /* drive is cdrom */
		{
			geometry->flags = BIOSDISK_FLAG_CDROM | BIOSDISK_FLAG_LBA_EXTENSION;
			geometry->heads = 255;
			geometry->sectors = 15;
			geometry->sector_size = 2048;
			geometry->total_sectors = hooked_drive_map[j].sector_count >> 2;
			geometry->cylinders = (geometry->total_sectors > (0xFFFFFFFFULL - 255 * 15 + 1) ? 0xFFFFFFFFUL / (255UL * 15UL):((unsigned long)geometry->total_sectors + 255 * 15 -1) / (255 * 15));
			if (! geometry->cylinders)
			{
				geometry->cylinders = 65536;
				geometry->total_sectors = 65536 * 255 * 15;
			}
			return 0;
		}
		if ((hooked_drive_map[j].max_sector & 0x3E) == 0 && hooked_drive_map[j].start_sector == 0 && hooked_drive_map[j].sector_count <= 1)
		{
			/* this is a map for the whole drive. */
			d = hooked_drive_map[j].to_drive;
			j = DRIVE_MAP_SIZE;	/* real drive */
			break;
		}
		//break;
		/* this is a drive emulation, get the geometry from the drive map table */
		geometry->flags = BIOSDISK_FLAG_LBA_EXTENSION;
		geometry->heads = hooked_drive_map[j].max_head + 1;
		geometry->sectors = hooked_drive_map[j].max_sector & 0x3F;
		geometry->sector_size = 512;
		geometry->total_sectors = hooked_drive_map[j].sector_count;
		geometry->cylinders = (geometry->heads * geometry->sectors);
		geometry->cylinders = (geometry->total_sectors > (0xFFFFFFFFULL - geometry->cylinders + 1) ? 0xFFFFFFFFUL / geometry->cylinders:((unsigned long)geometry->total_sectors + geometry->cylinders - 1) / geometry->cylinders);
		return 0;
	    }

	if (j == DRIVE_MAP_SIZE)	/* real drive */
	{
	    if (d >= 0x80 && d < 0x88)
	    {
		d -= 0x80;
		if (lba1sector & 0x80)
			hd_geom[d].flags &= ~BIOSDISK_FLAG_LBA_1_SECTOR;
		else if (lba1sector & 1)
			hd_geom[d].flags |= BIOSDISK_FLAG_LBA_1_SECTOR;
		flags = (hd_geom[d].flags & (BIOSDISK_FLAG_GEOMETRY_OK | BIOSDISK_FLAG_LBA_1_SECTOR));
		heads_ok = hd_geom[d].heads;
		sectors_ok = hd_geom[d].sectors;
		if (hd_geom[d].sector_size == 512 && hd_geom[d].sectors > 0 && hd_geom[d].sectors <= 63 && hd_geom[d].heads <= 256)
		{
		    if ((! force_geometry_tune) || (flags & BIOSDISK_FLAG_GEOMETRY_OK))
		    {
			geometry->flags = hd_geom[d].flags;
			if ((geometry->flags & BIOSDISK_FLAG_BIFURCATE) && (drive & 0xFFFFFF00) == 0x100)
			{
				if (geometry->flags & BIOSDISK_FLAG_CDROM)
				{
					geometry->cylinders = 65536;
					geometry->heads = 255;
					geometry->sectors = 15;
					geometry->sector_size = 2048;
					geometry->total_sectors = 65536 * 255 * 15;
					return 0;
				}
			}
			geometry->sector_size = hd_geom[d].sector_size;
			geometry->total_sectors = hd_geom[d].total_sectors;
			geometry->heads = hd_geom[d].heads;
			geometry->sectors = hd_geom[d].sectors;
			geometry->cylinders = hd_geom[d].cylinders;
			return 0;
		    }
		}
	    } else if (d < 4) {
		if (lba1sector & 0x80)
			fd_geom[d].flags &= ~BIOSDISK_FLAG_LBA_1_SECTOR;
		else if (lba1sector & 1)
			fd_geom[d].flags |= BIOSDISK_FLAG_LBA_1_SECTOR;
		flags = (fd_geom[d].flags & (BIOSDISK_FLAG_GEOMETRY_OK | BIOSDISK_FLAG_LBA_1_SECTOR));
		heads_ok = fd_geom[d].heads;
		sectors_ok = fd_geom[d].sectors;
		if (fd_geom[d].sector_size == 512 && fd_geom[d].sectors > 0 && fd_geom[d].sectors <= 63 && fd_geom[d].heads <= 256)
		{
		    if ((! force_geometry_tune) || (flags & BIOSDISK_FLAG_GEOMETRY_OK))
		    {
			geometry->flags = fd_geom[d].flags;
			if ((geometry->flags & BIOSDISK_FLAG_BIFURCATE) && (drive & 0xFFFFFF00) == 0x100)
			{
				if (geometry->flags & BIOSDISK_FLAG_CDROM)
				{
					geometry->cylinders = 65536;
					geometry->heads = 255;
					geometry->sectors = 15;
					geometry->sector_size = 2048;
					geometry->total_sectors = 65536 * 255 * 15;
					return 0;
				}
			}
			geometry->sector_size = fd_geom[d].sector_size;
			geometry->total_sectors = fd_geom[d].total_sectors;
			geometry->heads = fd_geom[d].heads;
			geometry->sectors = fd_geom[d].sectors;
			geometry->cylinders = fd_geom[d].cylinders;
			return 0;
		    }
		}
	    }
	}
    }

	if (debug > 1)      
		grub_printf ("\rget_diskinfo int13/41(%X), ", drive);
	version = check_int13_extensions ((unsigned char)drive, (lba1sector | (!!(flags & BIOSDISK_FLAG_LBA_1_SECTOR))));
	if (debug > 1)      
		grub_printf ("version=%X, ", version);

	/* Set the LBA flag.  */
	if (version & 1) /* support functions 42h-44h, 47h-48h */
	{
		flags |= BIOSDISK_FLAG_LBA_EXTENSION;
		if (version & 0x100) /* cannot read 127 sectors at a time. */
			flags |= BIOSDISK_FLAG_LBA_1_SECTOR;
	}
	total_sectors = 0;

	if (debug > 1)
		grub_printf ("int13/08(%X), ", drive);

	version = get_diskinfo_standard ((unsigned char)drive, &cylinders, &heads, &sectors);

	if (debug > 1)
		grub_printf ("version=%X, C/H/S=%d/%d/%d, ", version, cylinders, heads, sectors);

	if (debug > 1)
		grub_printf ("int13/02(%X), ", drive);

	/* read the boot sector: int 13, AX=0x201, CX=1, DH=0. Use buffer 0x20000 - 0x2FFFF */
	err = biosdisk_standard (0x02, (unsigned char)drive, 0, 0, 1, 1, 0x2F00/*SCRATCHSEG*/);

	if (debug > 1)
		grub_printf ("err=%X\n", err);
  DEBUG_SLEEP

	//version = 0;

	/* try again using LBA */
	if (flags & BIOSDISK_FLAG_LBA_EXTENSION || ((unsigned char)drive) >= 0x80 + FIND_DRIVES)
	{
		struct disk_address_packet
		{
			unsigned char length;
			unsigned char reserved;
			unsigned short blocks;
			unsigned long buffer;
			unsigned long long block;

			unsigned char dummy[16];
		} __attribute__ ((packed)) *dap;

		dap = (struct disk_address_packet *)0x580;

		dap->length = 0x10;
		dap->reserved = 0;
		dap->blocks = 1;
		dap->buffer = 0x2F80/*SCRATCHSEG*/ << 16;
		dap->block = 0;

		/* set a known value */
		grub_memset ((char *)0x2F800, 0xEC, 0x800);
		version = biosdisk_int13_extensions (0x4200, (unsigned char)drive, dap, 0);
		/* see if it is a big sector */
		{
			char *p;
			for (p = (char *)0x2FA00; p < (char *)0x30000; p++)
			{
				if ((*p) != (char)0xEC)
				{
					flags |= BIOSDISK_FLAG_CDROM | BIOSDISK_FLAG_LBA_EXTENSION;
					if (! err)
						flags |= BIOSDISK_FLAG_BIFURCATE;
					break;
				}
			}
		}
		if ((! version) && (! err))
		{
			if (grub_memcmp ((char *)0x2F060, (char *)0x2F860, 0x150)) /* ignore BPB and partition table */
			{
				flags |= BIOSDISK_FLAG_BIFURCATE;
			}
		}
		if (err && ! (flags & BIOSDISK_FLAG_BIFURCATE) && !(flags & BIOSDISK_FLAG_CDROM))
		{
			grub_memmove ((char *)0x2F000, (char *)0x2F800, 0x200);
		}

	} /* if (geometry->flags & BIOSDISK_FLAG_LBA_EXTENSION) */

	if (err && version)
		return err; /* When we return with ERROR, we should not change the geometry!! */

	geometry->flags = flags;

	if (err && (flags & BIOSDISK_FLAG_CDROM))
	{
		geometry->cylinders = 65536;
		geometry->heads = 255;
		geometry->sectors = 15;
		geometry->sector_size = 2048;
		geometry->total_sectors = 65536 * 255 * 15;
		return 0;
	}

	geometry->cylinders = cylinders;
	geometry->heads = heads;
	geometry->sectors = sectors;
	geometry->sector_size = SECTOR_SIZE;
	if (geometry->heads > 256)
	    geometry->heads = 256;
	if (geometry->sectors * geometry->sector_size > 63 * 512)
	    geometry->sectors = 63 * 512 / geometry->sector_size;
	tmp = (unsigned long long)(geometry->cylinders) *
	      (unsigned long long)(geometry->heads) *
	      (unsigned long long)(geometry->sectors);
	if (total_sectors < tmp)
	    total_sectors = tmp;
	geometry->total_sectors = total_sectors;

	/* successfully read boot sector */

	if (force_geometry_tune != 2) /* if not force BIOS geometry */
	{
	    if (drive & 0x80)
	    {
		    /* hard disk */
		    if ((err = probe_mbr((struct master_and_dos_boot_sector *)0x2F000/*SCRATCHADDR*/, 0, total_sectors, 0)))
		    {
			    if (debug > 1)
				    printf_warning ("\nWarning: Unrecognized partition table for drive %X. Please rebuild it using\na Microsoft-compatible FDISK tool(err=%d). Current C/H/S=%d/%d/%d\n", drive, err, geometry->cylinders, geometry->heads, geometry->sectors);
			    goto failure_probe_boot_sector;
		    }
		    err = (int)"MBR";
	    }else{
		    /* floppy */
		    if (probe_bpb((struct master_and_dos_boot_sector *)0x2F000/*SCRATCHADDR*/))
		    {
			    goto failure_probe_boot_sector;
		    }

		    err = (int)"BPB";
	    }

	    if (drive & 0x80)
	    if (probed_cylinders != geometry->cylinders)
		if (debug > 1)
		    printf_warning ("\nWarning: %s cylinders(%d) is not equal to the BIOS one(%d).\n", err, probed_cylinders, geometry->cylinders);

	    geometry->cylinders = probed_cylinders;

	    if (probed_heads != geometry->heads)
		if (debug > 1)
		    printf_warning ("\nWarning: %s heads(%d) is not equal to the BIOS one(%d).\n", err, probed_heads, geometry->heads);

	    geometry->heads	= probed_heads;

	    if (probed_sectors_per_track != geometry->sectors)
		if (debug > 1)
		    printf_warning ("\nWarning: %s sectors per track(%d) is not equal to the BIOS one(%d).\n", err, probed_sectors_per_track, geometry->sectors);

	    geometry->sectors = probed_sectors_per_track;

	    if (probed_total_sectors > total_sectors)
	    {
		if (drive & 0x80)
		if (debug > 1)
		    printf_warning ("\nWarning: %s total sectors(%d) is greater than the BIOS one(%d).\nSome buggy BIOSes could hang when you access sectors exceeding the BIOS limit.\n", err, probed_total_sectors, total_sectors);
		geometry->total_sectors	= probed_total_sectors;
	    }

	    if (drive & 0x80)
	    if (probed_total_sectors < total_sectors)
		if (debug > 1)
		    printf_warning ("\nWarning: %s total sectors(%d) is less than the BIOS one(%d).\n", err, probed_total_sectors, total_sectors);

	}
failure_probe_boot_sector:
	
#if 1
	if (flags & BIOSDISK_FLAG_GEOMETRY_OK)
	{
		err = geometry->heads;
		version = geometry->sectors;

		geometry->heads = heads_ok;
		geometry->sectors = sectors_ok;

		if (debug > 0)
		{
		    if (err != geometry->heads)
			grub_printf ("\n!! number of heads for drive %X restored from %d to %d.\n", drive, err, geometry->heads);
		    if (version != geometry->sectors)
			grub_printf ("\n!! sectors-per-track for drive %X restored from %d to %d.\n", drive, version, geometry->sectors);
		}
	}
	else if (force_geometry_tune==1 || (!(flags & BIOSDISK_FLAG_LBA_EXTENSION) && ! ((*(char *)0x8205) & 0x08)))
	{
		err = geometry->heads;
		version = geometry->sectors;

		/* DH non-zero for geometry_tune */
		get_diskinfo_standard (drive | 0x0100, &geometry->cylinders, &geometry->heads, &geometry->sectors);

		if (debug > 0)
		{
		    if (err != geometry->heads)
			grub_printf ("\nNotice: number of heads for drive %X tuned from %d to %d.\n", drive, err, geometry->heads);
		    if (version != geometry->sectors)
			grub_printf ("\nNotice: sectors-per-track for drive %X tuned from %d to %d.\n", drive, version, geometry->sectors);
		}
	}
#endif

	/* if C/H/S=0/0/0, use a safe default one. */
	if (geometry->sectors == 0)
	{
		if (drive & 0x80)
		{
			/* hard disk */
			geometry->sectors = 63;
		}else{
			/* floppy */
			if (geometry->total_sectors > 5760)
				geometry->sectors = 63;
			else if (geometry->total_sectors > 2880)
				geometry->sectors = 36;
			else
				geometry->sectors = 18;
		}
	}
	if (geometry->heads == 0)
	{
		if (drive & 0x80)
		{
			/* hard disk */
			geometry->heads = 255;
		}else{
			/* floppy */
			if (geometry->total_sectors > 5760)
				geometry->heads = 255;
			else
				geometry->heads = 2;
		}
	}
	if (geometry->cylinders == 0)
	{
		unsigned long long tmp_sectors = geometry->total_sectors;

		if (tmp_sectors > 0xFFFFFFFFULL)
		    tmp_sectors = 0xFFFFFFFFULL;
		geometry->cylinders = (((unsigned long)tmp_sectors) / geometry->heads / geometry->sectors);
	}

	if (geometry->cylinders == 0)
		geometry->cylinders = 1;
	total_sectors = geometry->cylinders * geometry->heads * geometry->sectors;
	if (geometry->total_sectors < total_sectors)
	    geometry->total_sectors = total_sectors;

  /* backup the geometry into array hd_geom or fd_geom. */

    {
	unsigned long j;
	unsigned long d;

	/* check if the drive is virtual. */
	d = (unsigned char)drive;
	j = DRIVE_MAP_SIZE;		/* real drive */
	if (! unset_int13_handler (1))
	    for (j = 0; j < DRIVE_MAP_SIZE; j++)
	    {
		if (drive_map_slot_empty (hooked_drive_map[j]))
		{
			j = DRIVE_MAP_SIZE;	/* real drive */
			break;
		}

		if (((unsigned char)drive) != hooked_drive_map[j].from_drive)
			continue;
		if (hooked_drive_map[j].to_cylinder & 0x2000) /* drive is cdrom */
			break;
		if ((hooked_drive_map[j].max_sector & 0x3E) == 0 && hooked_drive_map[j].start_sector == 0 && hooked_drive_map[j].sector_count <= 1)
		{
			/* this is a map for the whole drive. */
			d = hooked_drive_map[j].to_drive;
			j = DRIVE_MAP_SIZE;	/* real drive */
		}
		break;
	    }

	if (j == DRIVE_MAP_SIZE)	/* real drive */
	{
	    if (d >= 0x80 && d < 0x88)
	    {
		d -= 0x80;
		if (force_geometry_tune || hd_geom[d].sector_size != 512 || hd_geom[d].sectors <= 0 || hd_geom[d].sectors > 63 || hd_geom[d].heads > 256)
		{
			hd_geom[d].flags		&= BIOSDISK_FLAG_GEOMETRY_OK;
			hd_geom[d].flags		|= geometry->flags;
			hd_geom[d].sector_size		= geometry->sector_size;
			hd_geom[d].total_sectors	= geometry->total_sectors;
			hd_geom[d].heads		= geometry->heads;
			hd_geom[d].sectors		= geometry->sectors;
			hd_geom[d].cylinders		= geometry->cylinders;
		}
	    } else if (d < 4) {
		if (force_geometry_tune || fd_geom[d].sector_size != 512 || fd_geom[d].sectors <= 0 || fd_geom[d].sectors > 63 || fd_geom[d].heads > 256)
		{
			fd_geom[d].flags		&= BIOSDISK_FLAG_GEOMETRY_OK;
			fd_geom[d].flags		|= geometry->flags;
			fd_geom[d].sector_size		= geometry->sector_size;
			fd_geom[d].total_sectors	= geometry->total_sectors;
			fd_geom[d].heads		= geometry->heads;
			fd_geom[d].sectors		= geometry->sectors;
			fd_geom[d].cylinders		= geometry->cylinders;
		}
	    }
	}
    }
	if ((geometry->flags & BIOSDISK_FLAG_BIFURCATE) && (drive & 0xFFFFFF00) == 0x100)
	{
		if (geometry->flags & BIOSDISK_FLAG_CDROM)
		{
			geometry->cylinders = 65536;
			geometry->heads = 255;
			geometry->sectors = 15;
			geometry->sector_size = 2048;
			geometry->total_sectors = 65536 * 255 * 15;
		}
	}

  return 0;
}

#undef FIND_DRIVES

