#ifndef _INITRDFS_H_
#define _INITRDFS_H_

#include "cpio.h"

struct initrdfs_file
{
	grub_u64_t base;
	grub_u64_t name;
	grub_u32_t size;
	grub_u32_t isdir;
};

grub_u64_t char2u64(char *node);
int cpio_dir(void);
grub_u32_t cpio_file(grub_u64_t base,struct initrdfs_file *p_fn);
grub_u32_t raw_file(grub_u64_t base,struct initrdfs_file *p_fn,grub_u32_t max_size);

#endif /* _INITRDFS_H_ */
