#ifndef _INITRDFS_H_
#define _INITRDFS_H_

#include "cpio.h"

struct initrdfs_file
{
	grub_u32_t base;
	grub_u32_t size;
	grub_u32_t name;
	grub_u16_t isdir;
	grub_u16_t name_size;
};

#endif /* _INITRDFS_H_ */
