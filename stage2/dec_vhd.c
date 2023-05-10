/*
 *  GRUB4DOS  --  GRand Unified Bootloader
 *  Copyright (C) 1999  Free Software Foundation, Inc.
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
 *  Based on
 *  - LZ4 Block Format Description 2015-03-26 Yann Collet
 *  - LZ4 Frame Format Description version 1.5.1 2015-03-31 Yann Collet
 */

#include "shared.h"

#ifndef NO_DECOMPRESSION

typedef struct VHDFooter {							//VHD页脚表  尺寸512
	unsigned char cookie[8];              //标记 字符串conectix
	grub_u32_t features;                  //特征
	grub_u32_t fileFormatVersion;         //文件格式版本
	grub_u64_t dataOffset;                //数据偏移  结构的起始绝对字节位置，如果是动态磁盘，这表明了dd_hdr 的物理字节位置。如果是固定磁盘，似乎总是0xFFFFFFFF
	grub_u32_t timeStamp;                 //时间戳 
	unsigned char creatorApplication[4];  //创建者应用程序
	grub_u32_t creatorVersion;            //创建者版本
	grub_u32_t creatorHostOS;             //创建者主机操作系统 
	grub_u64_t originalSize;              //原始尺寸  虚拟出来的磁盘的可用寻址空间
	grub_u64_t currentSize;               //当前尺寸
	struct {
		unsigned short cylinder;            //柱面
		unsigned char heads;                //磁头
		unsigned char sectorsPerTrack;      //扇区
	} diskGeometry;
	grub_u32_t diskType;                  //磁盘类型  2固定, 3动态, 4差分
	grub_u32_t checksum;                  //校验和   整个扇区所有字节(当然一开始不包括 checksum 本身)相加得到32位数，再按位取反. 计算范围为从开始的512字节
	unsigned char uniqueId[16];           //唯一Id 
	unsigned char savedState;             //状态数据
	unsigned char reserved[427];          //保留
} __attribute__ ((packed)) VHDFooter;

#define VHD_FOOTER_COOKIE      0x78697463656E6F63ULL  //VHD页脚表标记      字符串conectix
#define VHD_DYNAMIC_COOKIE     0x6573726170737863ULL  //VHD动态磁盘头标记  字符串cxsparse

#define VHD_DISKTYPE_FIXED      2   //固定类型
#define VHD_DISKTYPE_DYNAMIC    3   //动态类型
#define VHD_DISKTYPE_DIFFERENCE 4   //差分类型

typedef struct VHDDynamicDiskHeader {   //VHD动态磁盘头  尺寸1024
	unsigned char cookie[8];              //标记 字符串cxsparse
	grub_u64_t dataOffset;                //数据偏移  总设置为0xFFFFFFFF
	grub_u64_t tableOffset;               //BAT结构偏移  在$VHD文件中的绝对字节位置
	grub_u32_t headerVersion;             //版本
	grub_u32_t maxTableEntries;           //BAT条目的最大值，实际上每个bat条目，就相当于一个块
	grub_u32_t blockSize;                 //块大小，几乎总是2MB
	grub_u32_t checksum;                  //校验和  与VHD页脚表中的计算方式相同。计算范围为从开始的1024字节
	unsigned char parentUniqueID[16];     //父vhd的uuid  差异磁盘中非常重要
	grub_u32_t parentTimeStamp;           //父磁盘的修改时间
	unsigned char reserved[4];            //保留
	unsigned char parentUnicodeName[512]; //父磁盘的unicode名称。可以更快地找到父磁盘，但找到后，还需通过uuid校验。
	unsigned char parentLocaterEntry[8][24];  //用来记录在不同平台上的父磁盘的名称
	unsigned char reserved2[256];         //保留
} __attribute__ ((packed)) VHDDynamicDiskHeader;

typedef struct VHDFileControl { //VHD控制文件
	struct VHDFileControl *next;					//下一个
	char *blockAllocationTable;						//BAT指针
	char *blockBitmapAndData;							//块缓存指针(也是块位图指针)
	char *blockData;											//块数据指针
	struct fragment_map_slot *SectorSequence;	//扇区序列
	unsigned long long volumeSize;        //解压后文件尺寸
	unsigned int currentBlockOffset;      //当前块缓存
	unsigned int blockLBA;								//逻辑块
	unsigned int blockSize;               //块尺寸
	unsigned char blockSizeLog2;					//块尺寸2的幂
	unsigned short blockBitmapSize;       //块位图尺寸
	unsigned char diskType;               //磁盘类型
	unsigned char	from_drive;							//from驱动器
	unsigned char	to_drive;								//to驱动器
	unsigned char	index;									//索引
	unsigned char	start;									//起始
	unsigned char	end;										//结束
	unsigned short fill0;
	unsigned char fill1;
} __attribute__ ((packed)) VHDFileControl;

struct fragment_map_slot *SectorSequence;
VHDFileControl *vhdfc_data;
int start;
int vhd_vhdfc_index = -1;
unsigned char	from_log2_sector;
unsigned char	to_log2_sector;
unsigned short to_block_size;

unsigned int log2pot32(unsigned int x);
grub_u32_t bswap_32(grub_u32_t *x);
void bswap_64(grub_u64_t *x);
void vhd_footer_in(VHDFooter *footer);
void vhd_header_in(VHDDynamicDiskHeader *header);

unsigned int log2pot32(unsigned int x);
unsigned int log2pot32(unsigned int x) {
	// x must be power of two
	return ((x & 0xFFFF0000) ? 16 : 0) | ((x & 0xFF00FF00) ? 8 : 0) | ((x & 0xF0F0F0F0) ? 4 : 0) | ((x & 0xCCCCCCCC) ? 2 : 0) | ((x & 0xAAAAAAAA) ? 1 : 0);
}

grub_u32_t bswap_32(grub_u32_t *x);
grub_u32_t bswap_32(grub_u32_t *x)
{
  grub_u32_t i = *x;
  *x = ((i & 0xFF000000) >> 24) |
       ((i & 0x00FF0000) >> 8)  |
       ((i & 0x0000FF00) << 8)  |
       ((i & 0x000000FF) << 24);
   return *x;
}

void bswap_64(grub_u64_t *x);
void bswap_64(grub_u64_t *x)  //字节交换  大尾转小尾
{
  grub_u32_t hi = (grub_u32_t)*x;
  grub_u32_t lo = (grub_u32_t)(*x >> 32);
  *x = ((grub_u64_t)bswap_32(&hi)<<32)|bswap_32(&lo);
}

void vhd_footer_in(VHDFooter *footer);
void vhd_footer_in(VHDFooter *footer)   //VHD页脚表in
{
	bswap_64(&footer->dataOffset);        //数据偏移
	bswap_64(&footer->currentSize);       //当前尺寸
	bswap_32(&footer->diskType);          //磁盘类型
}

void vhd_header_in(VHDDynamicDiskHeader *header);
void vhd_header_in(VHDDynamicDiskHeader *header)  //VHD动态磁盘头in
{
	bswap_64(&header->tableOffset);       //BAT结构偏移
	bswap_32(&header->maxTableEntries);   //BAT条目的最大值
	bswap_32(&header->blockSize);         //块尺寸
}

void dec_vhd_close(void);
void
dec_vhd_close(void)
{
#if 0
	if (vhdfc) {
		if (vhdfc->blockAllocationTable) {
			grub_free(vhdfc->blockAllocationTable);
      vhdfc->blockAllocationTable = 0;
		}
		if (vhdfc->blockBitmapAndData) {
			grub_free(vhdfc->blockBitmapAndData);
      vhdfc->blockBitmapAndData = 0;
		}
		grub_free(vhdfc);
    vhdfc = 0;
	}
  
  if (parentVHDFC) {
		if (parentVHDFC->blockAllocationTable) {
			grub_free(parentVHDFC->blockAllocationTable);
      parentVHDFC->blockAllocationTable = 0;
		}
		if (parentVHDFC->blockBitmapAndData) {
			grub_free(parentVHDFC->blockBitmapAndData);
      parentVHDFC->blockBitmapAndData = 0;
		}
		grub_free(parentVHDFC);
    parentVHDFC = 0;
	}
#endif
}

VHDFileControl *get_vhdfc_by_index(int index);
VHDFileControl *
get_vhdfc_by_index(int index)
{
	VHDFileControl *v;
	for (v = vhdfc_data; v; v = v->next)
	{
		if (v->index == index)
			return v;
	}
	return 0;
}

char *parentName;
char parentUniqueID[16];
int fill_vhdfc(int index, int vhd_start, VHDFooter *footer, VHDDynamicDiskHeader *dynaheader, VHDFileControl *vhdfc);
int
fill_vhdfc(int index, int vhd_start, VHDFooter *footer, VHDDynamicDiskHeader *dynaheader, VHDFileControl *vhdfc)	//填充VHD控制文件
{
	int i = 0;
	struct fragment_map_slot* q = SectorSequence;
	struct fragment *data = (struct fragment *)&q->fragment_data;

	//读VHD页脚表。  动态及差分VHD，起始1扇区是VHD页脚表。
	grub_SectorSequence_readwrite (current_drive, data, from_log2_sector, to_log2_sector, 0, sizeof(VHDFooter), (char *)footer, 0, GRUB_READ);
  grub_u64_t* a = (grub_u64_t*)footer->cookie;
  if (*a!=VHD_FOOTER_COOKIE)	//比较VHD页脚表标记
	{
		if (vhd_start)		//如果索引为零并且没有页脚表，则可能不是VHD，也可能是固定VHD
			goto quit;
		//查固定类型的父VHD页脚表
		while (data[i].start_sector)  //查找最后块   固定VHD的结构表在最后一个扇区。
      i++;

		vhd_read = 1;
		grub_efidisk_readwrite (current_drive, data[i-1].start_sector + data[i-1].sector_count - 1,
						sizeof(VHDFooter), (char *)footer, GRUB_READ);
		vhd_read = 0;

		if (*(grub_u64_t*)footer->cookie != VHD_FOOTER_COOKIE)
			goto quit;
	}

  vhd_footer_in(footer);
	if (!vhd_start && grub_memcmp ((const char *)parentUniqueID, (const char *)footer->uniqueId, 16))			//比较父VHD的guid 	
	{
		printf_debug ("GUID of parent VHD does not match.\n");
		goto quit;
	}

	if (footer->diskType == VHD_DISKTYPE_FIXED)  //磁盘类型是固定
		goto bbb;
	if (footer->dataOffset + sizeof(VHDDynamicDiskHeader) > filemax)		//数据偏移+VHD页脚表 > filemax, 退出
		goto quit;

	//读VHD动态磁盘头。
	filepos = footer->dataOffset;
	grub_SectorSequence_readwrite (current_drive, data, from_log2_sector, to_log2_sector, filepos >> 9, 
					sizeof(VHDDynamicDiskHeader), (char *)dynaheader, 0, GRUB_READ);

bbb:	
/*
动态、差分缓存基址	|------0x200------|----------------------------------0x200000----------------------------------|
								位图起始					数据起始

固定(差分之父)缓存基址								|----------------------------------0x200000----------------------------------|
																	数据起始
*/

  //设置VHD控制文件
	vhd_header_in(dynaheader);
	vhdfc->diskType = footer->diskType;
	vhdfc->volumeSize = footer->currentSize;

	if (footer->diskType == VHD_DISKTYPE_FIXED)		//如果父磁盘类型是固定
	{
		vhdfc->blockSize = 0x200000; 
		vhdfc->blockBitmapSize = 0;
	}
	else
	{
		vhdfc->blockSize = 0x200000; 
		vhdfc->blockBitmapSize = 0x200;
	}

	vhdfc->blockSizeLog2 = 0x15;
	vhdfc->currentBlockOffset = -1LL;
	vhdfc->from_drive = 0x80 + harddrives_orig;
	vhdfc->blockBitmapAndData = grub_malloc(vhdfc->blockBitmapSize + vhdfc->blockSize);
	vhdfc->blockData = vhdfc->blockBitmapAndData + vhdfc->blockBitmapSize;
	vhdfc->to_drive = current_drive;
	vhdfc->SectorSequence = SectorSequence;

	if (vhd_start)
		start = index;

	vhdfc->start = start;
	vhdfc->index = index;

	if (footer->diskType != VHD_DISKTYPE_DIFFERENCE)	//如果父磁盘类型不是差分
		vhdfc->end = index;
	if (footer->diskType == VHD_DISKTYPE_FIXED)	//如果父磁盘类型是固定
		goto ccc;

	unsigned int batSize = (dynaheader->maxTableEntries * 4 + 511)&(-512LL);
	vhdfc->blockAllocationTable = grub_malloc(batSize);

	//读BAT结构
	filepos = dynaheader->tableOffset;
	grub_SectorSequence_readwrite (vhdfc->to_drive, data, from_log2_sector, to_log2_sector, filepos >> 9, batSize, 
					vhdfc->blockAllocationTable, 0, GRUB_READ);

	if (footer->diskType == VHD_DISKTYPE_DIFFERENCE)	//差分
	{
		grub_memmove(&parentUniqueID, &dynaheader->parentUniqueID, 16);
		//获取父VHD文件名
		GetParentUtf8Name (parentName, (grub_uint16_t *)&dynaheader->parentUnicodeName);
	}

ccc:
	return vhdfc->diskType;
	
quit:
	return 0;
}

int dec_vhd_open(void);
int
dec_vhd_open(void)
/* return 1=success or 0=failure */
{
	int diskType;
	struct grub_disk_data *d;	//磁盘数据
	VHDFileControl *v, *vhdfc;   //VHD控制文件
	VHDFooter *footer = 0;
	VHDDynamicDiskHeader *dynaheader = 0;
	footer = grub_zalloc (sizeof(VHDFooter));		//分配页脚表
  if (!footer)
    return 0;
  dynaheader = grub_zalloc (sizeof(VHDDynamicDiskHeader));	//分配动态磁盘头
  if (!dynaheader)
    goto quit; 
	parentName = grub_zalloc(256);

  if (filemax < 0x10000) return 0;//file is to small
	/* Now it does not support openning more than 1 file at a time. 现在它不支持一次打开多个文件。
	   Make sure previously allocated memory blocks is freed.       确保先前分配的内存块已释放。
	   Don't need this line if grub_close is called for every openned file before grub_open is called for next file. */
     //如果在为下一个文件调用grub_open之前为每个打开的文件调用grub_close，则不需要此行。
		 
	if (vhd_file_name == 0)
		goto quit;

//  if (vhdfc[0]) //如果子VHD打开过 
//		goto normalP;

	char *filename = vhd_file_name;
	skip_to(0x200,filename);
  char *suffix = &filename[strlen (filename) - 3]; //取尾缀
	if ((suffix[0] | 0x20) != 'v' || (suffix[1] | 0x20) != 'h' || (suffix[2] | 0x20) != 'd')
		goto quit;

	dec_vhd_close();
	d = get_device_by_drive (current_drive,0);
	if (!d)
		return 0;
	from_log2_sector = 9;	//未考虑4k磁盘
	to_log2_sector = d->from_log2_sector;
	to_block_size = 1 << to_log2_sector;
		
	vhd_vhdfc_index++;
	GetSectorSequence (vhd_file_name, &SectorSequence, 1);
	if (!SectorSequence)
	{
		printf_debug ("Failed to obtain file sector sequence.\n");
		goto quit; 
	}	

	vhdfc = (VHDFileControl*) grub_zalloc(sizeof(VHDFileControl));	//分配控制文件
	if (!vhdfc)
		goto quit;
	if (!vhd_vhdfc_index)
		vhdfc_data = vhdfc;
	else
	{
		for (v = vhdfc_data; v; v = v->next)
		{
			if (!v->next)
			{
				v->next = vhdfc;
				break;
			}
		}
	}

	diskType = fill_vhdfc (vhd_vhdfc_index,1,footer,dynaheader,vhdfc);
	if (diskType == 0 || diskType == VHD_DISKTYPE_FIXED)	//失败或固定
		goto quit;

  //如果是差分
  if (diskType == VHD_DISKTYPE_DIFFERENCE)
  {
aaa:
		vhd_vhdfc_index++;
		GetSectorSequence (parentName, &SectorSequence, 0);     //获得父VHD文件扇区序列
		if (!SectorSequence)
    {
      printf_debug ("Failed to obtain file sector sequence.\n");
      goto quit; 
    }

		vhdfc = (VHDFileControl*) grub_zalloc(sizeof(VHDFileControl));	//分配控制文件
		if (!vhdfc)
			goto quit;
		for (v = vhdfc_data; v; v = v->next)
		{
			if (!v->next)
			{
				v->next = vhdfc;
				break;
			}
		}

		diskType = fill_vhdfc (vhd_vhdfc_index,0,footer,dynaheader,vhdfc);
		if (diskType == VHD_DISKTYPE_DIFFERENCE)	//如果父磁盘类型是差分
		{
			goto aaa;
		}
	}

//normalP:  
	compressed_file = 1;            //压缩文件
	decomp_type = DECOMP_TYPE_VHD;  //解压缩类型VHD 
	filemax = vhdfc->volumeSize;	//修改filemax

quit:
	filepos = 0;
	grub_free(parentName);
	grub_free(dynaheader);
	grub_free(footer);
	errnum = ERR_NONE;
	return compressed_file;
}

static void read_differ_itself (int index1, int index2);
static void
read_differ_itself (int index1, int index2)
{
	unsigned int i, j, k, test;
	VHDFileControl *vhdfc1, *vhdfc2;
	vhdfc1 = get_vhdfc_by_index(index1);
	vhdfc2 = get_vhdfc_by_index(index2);
	unsigned int *bitmap = (unsigned int *)vhdfc1->blockBitmapAndData;

	for (i = 0; i < 0x80; i++)	//1扇区0x80块  i是块号
	{
		test = 0x80000000;	//测试位
		j = 0;							//块内位移

		if (bitmap[i] != 0)
		{
			bswap_32(&bitmap[i]);
			while (test)
			{
				if (bitmap[i] & test) //bitmap位为1, 直接读取子VHD
				{
					k = 0;				//读扇区数
					while (bitmap[i] & test)	//把bitmap位为1的扇区一起读
					{
						test >>= 1;	//下一测试位
						k++;				//连续的读扇区数
					} 
					grub_memmove64((unsigned long long)(grub_size_t)(vhdfc2->blockData + (((i << 5) + j) << 9)), 
									(unsigned long long)(grub_size_t)(vhdfc1->blockData + (((i << 5) + j) << 9)), 
									512 * k);
					j += k;				//块内位移数
				}
				else //bitmap位为0, 跳过
				{
					test >>= 1;
					j++;
				}
			}
		}
	}
}

int current_index = -1;
unsigned long long dec_vhd_read(unsigned long long buf, unsigned long long len, unsigned int write);
unsigned long long
dec_vhd_read(unsigned long long buf, unsigned long long len, unsigned int write)
{
	unsigned long long ret = 0;
	int i, index = 0, parent = 0;
	struct grub_disk_data *d;
	struct fragment_map_slot *q;
	struct fragment *data;
	VHDFileControl *v;   //VHD控制文件
//	compressed_file = 0;

	d = get_device_by_drive (current_drive,0);
	if (!d)
		return 0;

	if (vhd_file_name)
	{
		from_log2_sector = 9;	//未考虑4k磁盘
		to_log2_sector = d->from_log2_sector;
		to_block_size = 1 << to_log2_sector;
		for (v = vhdfc_data; v; v = v->next)
		{
			if (v->index == vhd_vhdfc_index)
				break;
		}
		index = v->start;
		parent = vhd_vhdfc_index;
	}
	else
	{
		for (v = vhdfc_data; v; v = v->next)
		{
			if (v->from_drive == current_drive && v->end)
			{
				index = v->start;
				parent = v->end;
				break;
			}
		}
		from_log2_sector = d->from_log2_sector;
		to_log2_sector = d->to_log2_sector;
		to_block_size = d->to_block_size;
	}
	
	VHDFileControl *vhdfc_index, *vhdfc_parent;
	vhdfc_index = get_vhdfc_by_index(index);
	vhdfc_parent = get_vhdfc_by_index(parent);

	if (filepos + len > vhdfc_index->volumeSize)
		len = (filepos <= vhdfc_index->volumeSize) ? vhdfc_index->volumeSize - filepos : 0;
		// VHD_DISKTYPE_DYNAMIC
	if (write == GRUB_WRITE)
	{
		errnum = ERR_WRITE_GZIP_FILE;
		return 0;
	}
	unsigned long long uFilePos = filepos;
	if (len > vhdfc_index->volumeSize - uFilePos)
		len = vhdfc_index->volumeSize - uFilePos;
	errnum = ERR_NONE;
	unsigned long long rem = len;
  unsigned int blockNumber;
  unsigned long long blockOffset;
  unsigned int offsetInBlock;
  unsigned int txLen;

  while (rem)
  {
    blockNumber = (unsigned int)(uFilePos >> vhdfc_index->blockSizeLog2);                                  //块号
    blockOffset = (unsigned long long)blockNumber << vhdfc_index->blockSizeLog2;                           //块起始
    offsetInBlock = (unsigned int)(uFilePos - blockOffset);                                                //在块内偏移
    txLen = (rem < vhdfc_index->blockSize - offsetInBlock) ? rem : vhdfc_index->blockSize - offsetInBlock; //块内读尺寸
    vhdfc_index->blockLBA = *(grub_u32_t*)(vhdfc_index->blockAllocationTable + blockNumber * 4);           //块逻辑扇区
    bswap_32(&vhdfc_index->blockLBA);                                                                      //大尾转小尾
#if 0
    if (vhdfc_index->diskType == VHD_DISKTYPE_DYNAMIC)   //如果是动态
    {
			if (current_index != index || blockOffset != vhdfc_index->currentBlockOffset)
			{
				if (vhdfc_index->blockLBA == 0xFFFFFFFF) 
					grub_memset64((unsigned long long)(grub_size_t)vhdfc_index->blockData, 0, vhdfc_index->blockBitmapSize);
				else
				{
					q = vhdfc_index->SectorSequence;
					data = (struct fragment *)&q->fragment_data;
					grub_SectorSequence_readwrite (vhdfc_index->to_drive, data, from_log2_sector, to_log2_sector, vhdfc_index->blockLBA, 
									vhdfc_index->blockBitmapSize + vhdfc_index->blockSize, vhdfc_index->blockBitmapAndData, 0, GRUB_READ);
				}
				
				vhdfc_index->currentBlockOffset = blockOffset;
				current_index = index;
			}
			grub_memmove64(buf, (grub_size_t)(vhdfc_index->blockData + offsetInBlock), txLen);
    }
    else   //如果是差分
#endif
    {
/*
vhdfc_index：是用户最后生成的差分磁盘。由用户使用map映射，挂钩，分配驱动器号。
vhdfc_parent：是终极父磁盘，可能是固定磁盘或者动态磁盘。由本程序使用GetSectorSequence函数探测生成。
vhdfc_i：中间的磁盘都是差分磁盘。由本程序使用GetSectorSequence函数探测生成。
*/
			//读磁盘，更新缓存区
      if (current_index != index || blockOffset != vhdfc_index->currentBlockOffset)                   //如果块偏移不等于当前块偏移
      {
				//确定磁盘的逻辑块，然后读父磁盘到各自的父缓存
				for (i = index; i <= parent; i++)
				{
					VHDFileControl *vhdfc_i;
					vhdfc_i = get_vhdfc_by_index(i);
					if (vhdfc_i->diskType == VHD_DISKTYPE_FIXED)			//如果父磁盘类型是固定
						vhdfc_i->blockLBA = blockOffset >> 9;  //父块逻辑扇区
					else
					{
						vhdfc_i->blockLBA = *(grub_u32_t*)(vhdfc_i->blockAllocationTable + blockNumber * 4);  //父块逻辑扇区
						bswap_32(&vhdfc_i->blockLBA);
					}

					if (vhdfc_i->blockLBA != 0xFFFFFFFF)							//如果逻辑块不是‘-1’		
					{
						q = vhdfc_i->SectorSequence;
						data = (struct fragment *)&q->fragment_data;
						grub_SectorSequence_readwrite (vhdfc_i->to_drive, data, from_log2_sector, to_log2_sector, vhdfc_i->blockLBA, 
										vhdfc_i->blockBitmapSize + vhdfc_i->blockSize, vhdfc_i->blockBitmapAndData, 0, GRUB_READ);
					}
					else if (vhdfc_i->diskType == VHD_DISKTYPE_DYNAMIC)	//如果父类型是动态
					{
						grub_memset64((unsigned long long)(grub_size_t)vhdfc_i->blockData, 0, vhdfc_i->blockSize);	//填充0
					}
				}
			
				//各差分磁盘穿透进父磁盘进行补差
				for (i = parent - 1; i >= index; i--)
				{
					VHDFileControl *vhdfc_i;
					vhdfc_i = get_vhdfc_by_index(i);
					if (vhdfc_i->blockLBA != 0xFFFFFFFF)
						read_differ_itself (i, parent);
				}
				vhdfc_index->currentBlockOffset = blockOffset;
				current_index = index;
			}

			grub_memmove64(buf, (unsigned long long)(grub_size_t)(vhdfc_parent->blockData + offsetInBlock), txLen);
		}

		buf += txLen;
		uFilePos += txLen;
		rem -= txLen;
		ret += txLen;
	}

		filepos = uFilePos;
//	compressed_file = 1;
//	filemax = vhdfc->volumeSize;
	return ret;
}

#endif /* ! NO_DECOMPRESSION */
