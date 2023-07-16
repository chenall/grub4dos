#ifndef _INITRDFS_H_
#define _INITRDFS_H_

#include "cpio.h"
struct initrdfs_file
{
//	grub_u32_t base;
//	grub_u32_t size;
//	grub_u32_t name;
	grub_size_t base;     //基址      =hdr+对齐的(hdr尺寸+hdr->c_namesize)
	grub_size_t size;     //尺寸      =(hdr->c_filesize)
	grub_size_t name;     //名称      =hdr+hdr尺寸
	grub_u16_t isdir;     //属性      =hdr->c_mode & 0040000   0/1=文件/目录
	grub_u16_t name_size; //名称尺寸  =hdr->c_namesize
};
#endif /* _INITRDFS_H_ */
