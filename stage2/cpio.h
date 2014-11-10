#ifndef _CPIO_H
#define _CPIO_H

/** A CPIO archive header
 *
 * All field are hexadecimal ASCII numbers padded with '0' on the
 * left to the full width of the field.
 */
struct cpio_header {
	char c_magic[6]; 	/** The string "070701" or "070702" */
	char c_ino[8];		/** File inode number */
	char c_mode[8];		/** File mode and permissions */
	char c_uid[8];		/** File uid */
	char c_gid[8];		/** File gid */
	char c_nlink[8];	/** Number of links */
	char c_mtime[8];	/** Modification time */
	char c_filesize[8];	/** Size of data field */
	char c_maj[8];		/** Major part of file device number */
	char c_min[8];		/** Minor part of file device number */
	char c_rmaj[8];		/** Major part of device node reference */
	char c_rmin[8];		/** Minor part of device node reference */
	char c_namesize[8];	/** Length of filename, including final NUL */
	char c_chksum[8];	/** Checksum of data field if c_magic is 070702, othersize zero */
} __attribute__ (( packed ));

/** CPIO magic */
#define CPIO_MAGIC "070701"
#define CPIO_MODE_DIR	0040000
#define CPIO_ALIGN	4
static inline grub_u32_t cpio_image_align( grub_u32_t len )
{
	return (len + (CPIO_ALIGN - 1)) & ~(CPIO_ALIGN - 1);
}

#endif /* _CPIO_H */
