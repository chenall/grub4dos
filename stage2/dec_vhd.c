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

struct VHDFooter{						            //VHD页脚表  尺寸512
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
	unsigned char uniqueId[16];           //本磁盘uuid 
	unsigned char savedState;             //状态数据
	unsigned char reserved[427];          //保留
};

struct VHDFooter *footer=0;

#define VHD_FOOTER_COOKIE      0x78697463656E6F63ULL  //VHD页脚表标记      字符串conectix
#define VHD_DYNAMIC_COOKIE     0x6573726170737863ULL  //VHD动态磁盘头标记  字符串cxsparse

#define VHD_DISKTYPE_FIXED      2   //固定类型
#define VHD_DISKTYPE_DYNAMIC    3   //动态类型
#define VHD_DISKTYPE_DIFFERENCE 4   //差分类型

struct VHDDynamicDiskHeader{            //VHD动态磁盘头  尺寸1024
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
};

struct VHDDynamicDiskHeader *dynaheader=0;

struct VHDFileControl{                  //VHD控制文件
	char *blockAllocationTable;						//BAT指针
	char *blockBitmapAndData;							//块缓存指针(也是块位图指针)
	char *blockData;											//块数据指针
  union
  {
    struct fragment_map_slot *SectorSequence;	//扇区序列
    char *efi_pxe_buf;                  //网起缓存
  };
  char *vhd_file_name;                  //文件名
	unsigned long long volumeSize;        //解压后文件尺寸
  unsigned long long filemax;           //未解压文件尺寸
	unsigned int currentBlockOffset;      //当前块缓存
	unsigned int blockLBA;								//逻辑块
	unsigned int blockSize;               //块尺寸
	unsigned char blockSizeLog2;					//块尺寸2的幂
	unsigned short blockBitmapSize;       //块位图尺寸
	unsigned char diskType;               //磁盘类型
	unsigned char	from_drive;							//from驱动器
	unsigned char	to_drive;								//to驱动器
	unsigned char	index;									//索引
	unsigned char fill1;
};

struct VHDFileControl *vhdfc[4];

struct fragment_map_slot *SectorSequence;
int index = -1;
unsigned char	from_log2_sector;
unsigned char	to_log2_sector;
unsigned short to_block_size;

grub_u32_t bswap_32(grub_u32_t x);
grub_u32_t bswap_32(grub_u32_t x)
{
  grub_u32_t i = x;
  x = ((i & 0xFF000000) >> 24) |
       ((i & 0x00FF0000) >> 8)  |
       ((i & 0x0000FF00) << 8)  |
       ((i & 0x000000FF) << 24);
   return x;
}

grub_u64_t bswap_64(grub_u64_t x);
grub_u64_t bswap_64(grub_u64_t x)  //字节交换  大尾转小尾
{
  grub_u32_t hi = (grub_u32_t)x;
  grub_u32_t lo = (grub_u32_t)(x >> 32);
  x = ((grub_u64_t)bswap_32(hi)<<32)|bswap_32(lo);
  return x;
}

void vhd_footer_in(struct VHDFooter *f);
void vhd_footer_in(struct VHDFooter *f)   //VHD页脚表in
{
	f->dataOffset = bswap_64(f->dataOffset);    //数据偏移
	f->currentSize = bswap_64(f->currentSize);  //当前尺寸
	f->diskType = bswap_32(f->diskType);        //磁盘类型
}

void vhd_header_in(struct VHDDynamicDiskHeader *header);
void vhd_header_in(struct VHDDynamicDiskHeader *header)  //VHD动态磁盘头in
{
	header->tableOffset = bswap_64(header->tableOffset);          //BAT结构偏移
	header->maxTableEntries = bswap_32(header->maxTableEntries);  //BAT条目的最大值
	header->blockSize = bswap_32(header->blockSize);              //块尺寸
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

char *parentName;
char *childName;
int end;
char parentUniqueID[16];
int fill_vhdfc(struct VHDFileControl *v);
int
fill_vhdfc(struct VHDFileControl *v)	//填充VHD控制文件
{
	int i = 0;
	struct fragment_map_slot* q = SectorSequence;
  int offse = offsetof(struct fragment_map_slot, fragment_data);
  struct fragment *data = (struct fragment *)((char *)q + offse);

	//读VHD页脚表。  动态及差分VHD，起始1扇区是VHD页脚表。
  filepos = 0;
  if (current_drive != 0x21)
  {
		grub_efidisk_readwrite (current_drive, data[0].start_sector, sizeof(struct VHDFooter), (char *)footer, GRUB_READ);
  }
  else
    pxe_read ((unsigned long long)(grub_size_t)footer, sizeof(struct VHDFooter), GRUB_READ);

  grub_u64_t* a = (grub_u64_t*)footer->cookie; 
  if (*a!=VHD_FOOTER_COOKIE)	//比较VHD页脚表标记
	{
		if (!index)		//如果索引为零并且没有页脚表，则可能不是VHD，也可能是固定VHD
			goto fill_quit;
		//查固定类型的父VHD页脚表  此处没有考虑0x21 !!!
    if (current_drive != 0x21)
    {
      while (data[i].start_sector)  //查找最后块   固定VHD的结构表在最后一个扇区。
        i++;
		grub_efidisk_readwrite (current_drive, data[i-1].start_sector + data[i-1].sector_count - 1,
						sizeof(struct VHDFooter), (char *)footer, GRUB_READ);

		if (*(grub_u64_t*)footer->cookie != VHD_FOOTER_COOKIE)
			goto fill_quit;
    }
    else if (cur_pxe_type && http_feature)  //http_206
    {
      filepos = filemax - 512;
      pxe_read ((unsigned long long)(grub_size_t)footer, 512, GRUB_READ);
    }
    else
      grub_memmove64((unsigned long long)(grub_size_t)footer, 
        (unsigned long long)(grub_size_t)efi_pxe_buf + filemax - 512, 512);
  }

  vhd_footer_in(footer);
  //比较父VHD的guid 	
	if (index && grub_memcmp ((const char *)parentUniqueID, (const char *)footer->uniqueId, 16))
	{
		printf_debug ("GUID of parent VHD does not match.\n");
		goto fill_quit;
	}

	if (footer->diskType == VHD_DISKTYPE_FIXED)  //磁盘类型是固定
		goto bbb;
	if (footer->dataOffset + sizeof(struct VHDDynamicDiskHeader) > filemax)		//数据偏移+VHD页脚表 > filemax, 退出
		goto fill_quit;
	//读VHD动态磁盘头。
	filepos = footer->dataOffset;
  if (current_drive != 0x21)
    grub_SectorSequence_readwrite (current_drive, data, from_log2_sector, to_log2_sector, filepos >> 9, 
					sizeof(struct VHDDynamicDiskHeader), (char *)dynaheader, 0, GRUB_READ);
  else if (cur_pxe_type && http_feature)  //http_206
    pxe_read ((unsigned long long)(grub_size_t)dynaheader, sizeof(struct VHDDynamicDiskHeader), GRUB_READ);
  else
    grub_memmove64((unsigned long long)(grub_size_t)dynaheader, 
        (unsigned long long)(grub_size_t)efi_pxe_buf + filepos, sizeof(struct VHDDynamicDiskHeader));
bbb:	
/*
动态、差分缓存基址	|------0x200------|----------------------------------0x200000----------------------------------|
								位图起始					数据起始

固定(差分之父)缓存基址								|----------------------------------0x200000----------------------------------|
																	数据起始
*/
  //设置VHD控制文件
	vhd_header_in(dynaheader);
	v->diskType = footer->diskType;
	v->volumeSize = footer->currentSize;
  v->filemax = filemax;

	if (footer->diskType == VHD_DISKTYPE_FIXED)		//如果父磁盘类型是固定
	{
		v->blockSize = 0x200000; 
		v->blockBitmapSize = 0;
	}
	else
	{
		v->blockSize = 0x200000; 
		v->blockBitmapSize = 0x200;
	}

	v->blockSizeLog2 = 0x15;
	v->currentBlockOffset = -1LL;
	v->from_drive = 0x80 + harddrives_orig;
	v->blockBitmapAndData = grub_malloc(v->blockBitmapSize + v->blockSize);
	v->blockData = v->blockBitmapAndData + v->blockBitmapSize;
	v->to_drive = current_drive;
  if (current_drive != 0x21)
    v->SectorSequence = SectorSequence;
  else
    v->efi_pxe_buf = efi_pxe_buf;

	v->index = index;
	if (footer->diskType == VHD_DISKTYPE_FIXED)	//如果父磁盘类型是固定
		goto fill_0;
	unsigned int batSize = (dynaheader->maxTableEntries * 4 + 511)&(-512LL);
	v->blockAllocationTable = grub_malloc(batSize);
	//读BAT结构
	filepos = dynaheader->tableOffset;
  if (current_drive != 0x21)
    grub_SectorSequence_readwrite (v->to_drive, data, from_log2_sector, to_log2_sector, filepos >> 9, batSize, 
					v->blockAllocationTable, 0, GRUB_READ);
  else if (cur_pxe_type && http_feature)  //http_206
    pxe_read ((unsigned long long)(grub_size_t)v->blockAllocationTable, batSize, GRUB_READ);
  else
    grub_memmove64((unsigned long long)(grub_size_t)v->blockAllocationTable, 
        (unsigned long long)(grub_size_t)efi_pxe_buf + filepos, batSize);
  
  if (!index)
  {
    childName = grub_zalloc (256);
    grub_memmove(childName, vhd_file_name, 256);
    v->vhd_file_name = childName;
  }

	if (footer->diskType == VHD_DISKTYPE_DIFFERENCE)	//差分
	{
		grub_memmove(&parentUniqueID, &dynaheader->parentUniqueID, 16);
		//获取父VHD文件名
    parentName = grub_zalloc(256);
		GetParentUtf8Name (parentName, (grub_uint16_t *)&dynaheader->parentUnicodeName);
	}

fill_0:
	return v->diskType;
	
fill_quit:
	return 0;
}

int dec_vhd_open(void);
int
dec_vhd_open(void)
/* return 1=success or 0=failure */
{
	int diskType;
	struct grub_disk_data *d;	//磁盘数据

	footer = grub_zalloc (sizeof(struct VHDFooter));		//分配页脚表
  if (!footer)
    goto quit;
  dynaheader = grub_zalloc (sizeof(struct VHDDynamicDiskHeader));	//分配动态磁盘头
  if (!dynaheader)
    goto quit; 

  if (filemax < 0x10000) return 0;//file is to small
	if (vhd_file_name == 0)
		goto quit;
//	dec_vhd_close();
	if (current_drive == 0x21) //为了网起   2024-12-12
	{
    from_log2_sector = 9;	//未考虑4k磁盘
    to_log2_sector = 9;
    to_block_size = 512;
    goto bbb;
	}
	d = get_device_by_drive (current_drive,0);
	if (!d)
		return 0;
	from_log2_sector = 9;	//未考虑4k磁盘
	to_log2_sector = d->from_log2_sector;
	to_block_size = 1 << to_log2_sector;
bbb:		
  index++;
  if (current_drive == 0x21)
    goto ccc;
  GetSectorSequence (vhd_file_name, &SectorSequence, 1);  //获得VHD文件的扇区序列
  if (!SectorSequence)
  {
    printf_debug ("Failed to obtain file sector sequence.\n");
    goto quit; 
  }	
ccc:
	vhdfc[index] = (struct VHDFileControl *)grub_zalloc(sizeof(struct VHDFileControl));	//分配控制文件
	if (!vhdfc[index])
		goto quit;

	diskType = fill_vhdfc (vhdfc[index]);  //填充VHD控制文件
	if (diskType == 0 || diskType == VHD_DISKTYPE_FIXED)	//失败或固定
		goto quit;

  //如果是差分
  if (diskType == VHD_DISKTYPE_DIFFERENCE)
  {
aaa:
		index++;
    if (index > 3)
    {
      printf_warning ("Differential VHD cannot exceed level 4.\n");
      goto quit;
    }
    if (current_drive != 0x21)
    {
      GetSectorSequence (parentName, &SectorSequence, 0);     //获得父VHD文件扇区序列
      if (!SectorSequence)
      {
        printf_debug ("Failed to obtain file sector sequence.\n");
        goto quit; 
      }
    }
    else
      pxe_open (parentName);

		vhdfc[index] = (struct VHDFileControl *) grub_zalloc(sizeof(struct VHDFileControl));	//分配控制文件
		if (!vhdfc[index])
			goto quit;
		vhdfc[index]->vhd_file_name = parentName;
		diskType = fill_vhdfc (vhdfc[index]);
		if (diskType == VHD_DISKTYPE_DIFFERENCE)	//如果父磁盘类型是差分
			goto aaa;
    else
      end = index;
	}

	compressed_file = 1;            //压缩文件
	decomp_type = DECOMP_TYPE_VHD;  //解压缩类型: VHD 
	filemax = vhdfc[index]->volumeSize;	  //压缩文件尺寸
quit:
	grub_free(dynaheader);
	grub_free(footer);
  footer = 0;
	filepos = 0;
	errnum = ERR_NONE;
	return compressed_file;
}


static void read_differ_itself (int start, int blook);
static void
read_differ_itself (int start, int blook) //读取差异本身
{
	unsigned int i, j, test;
	unsigned int *bitmap;
  test = 0x80000000;	//测试位
  j = 0;							//块内位移

  bitmap = (unsigned int *)vhdfc[0]->blockBitmapAndData;
      bitmap[blook] = bswap_32(bitmap[blook]);
  while (test)
  {
    for (i = start; i < end; i++)	//i是差分序号
    {
      if (bitmap[blook] & test) //bitmap位为1, 直接读取子VHD
      {
        grub_memmove64((unsigned long long)(grub_size_t)(vhdfc[end]->blockData + (((blook << 5) + j) << 9)), 
                (unsigned long long)(grub_size_t)(vhdfc[i]->blockData + (((blook << 5) + j) << 9)), 512);
        goto kkk;
      }
    }
kkk:
    test >>= 1;
    j++;
  }
}

int read_disk (struct VHDFileControl *v, int type, unsigned long long pointer);
int
read_disk (struct VHDFileControl *v, int type, unsigned long long pointer)
{
  struct fragment_map_slot *q;
	struct fragment *data;
  char *dest;
  unsigned long long len;
  char *back_name;
  unsigned long long back_filepos;
  unsigned long long back_filemax;
  int a = 0xff;

  if (type == 2)
  {
    dest = v->blockData;
    len = v->blockSize;
  }
  else if (type == 3)
  {
    unsigned int blockNumber;
    blockNumber = (unsigned int)(pointer >> v->blockSizeLog2);                                  //块号
    v->blockLBA = *(grub_u32_t*)(v->blockAllocationTable + blockNumber * 4);              //块逻辑扇区
    v->blockLBA = bswap_32(v->blockLBA);                                                  //大尾转小尾
    dest = v->blockBitmapAndData;
    len = v->blockBitmapSize + v->blockSize;

    if (v->blockLBA == 0xFFFFFFFF)
    {
      grub_memset64((unsigned long long)(grub_size_t)v->blockData, 0, v->blockSize);
      return 1;
    }
  }
  else
  {
    dest = v->blockBitmapAndData;
    len = v->blockBitmapSize + v->blockSize;
  }
  
  if (current_drive != 0x21)
  {
    q = v->SectorSequence;
    int offse = offsetof(struct fragment_map_slot, fragment_data);
    data = (struct fragment *)((char *)q + offse);
    grub_SectorSequence_readwrite (v->to_drive, data, from_log2_sector, to_log2_sector, v->blockLBA, len, dest, 0, GRUB_READ);
  }
  else if (cur_pxe_type && http_feature)  //http_206
  {
    back_name = pxe_name;
    back_filepos = filepos;
    back_filemax = filemax;
    pxe_name = v->vhd_file_name;
    filemax = v->filemax;
    filepos = v->blockLBA * 512;
    while (a)
      a--;
    pxe_read ((unsigned long long)(grub_size_t)dest, len, GRUB_READ);
    pxe_name = back_name;
    filepos = back_filepos;
    filemax = back_filemax;
  }
  else
  {
    back_filepos = filepos;
    filepos = v->blockLBA * 512;
    grub_memmove64((unsigned long long)(grub_size_t)dest, (unsigned long long)(grub_size_t)v->efi_pxe_buf + v->blockLBA * 512, len);
    filepos = back_filepos;
  }

  return 1;
}

int current_index = -1;
unsigned long long dec_vhd_read(unsigned long long buf, unsigned long long len, unsigned int write);
unsigned long long
dec_vhd_read(unsigned long long buf, unsigned long long len, unsigned int write)
{
	unsigned long long ret = 0;
	unsigned int i, j;

//	from_log2_sector = 9;	//未考虑4k磁盘
//	to_log2_sector = 9;
//	to_block_size = 1 << to_log2_sector;

	if (filepos + len > vhdfc[0]->volumeSize)
		len = (filepos <= vhdfc[0]->volumeSize) ? vhdfc[0]->volumeSize - filepos : 0;
	if (write == GRUB_WRITE)
	{
		errnum = ERR_WRITE_GZIP_FILE;
		return 0;
	}
	unsigned long long uFilePos = filepos;
	if (len > vhdfc[0]->volumeSize - uFilePos)
		len = vhdfc[0]->volumeSize - uFilePos;
	errnum = ERR_NONE;
	unsigned long long rem = len;
  unsigned int blockNumber;
  unsigned long long blockOffset;
  unsigned int offsetInBlock;
  unsigned int txLen;

  while (rem)
  {
    blockNumber = (unsigned int)(uFilePos >> vhdfc[0]->blockSizeLog2);                                //块号  1块=0x200000字节
    blockOffset = (unsigned long long)blockNumber << vhdfc[0]->blockSizeLog2;                         //块起始字节
    offsetInBlock = (unsigned int)(uFilePos - blockOffset);                                           //在块内字节偏移
    txLen = (rem < vhdfc[0]->blockSize - offsetInBlock) ? rem : vhdfc[0]->blockSize - offsetInBlock;  //块内读尺寸
    vhdfc[0]->blockLBA = *(grub_u32_t*)(vhdfc[0]->blockAllocationTable + blockNumber * 4);            //块逻辑扇区
    vhdfc[0]->blockLBA = bswap_32(vhdfc[0]->blockLBA);                                                //大尾转小尾

    if (vhdfc[0]->diskType == VHD_DISKTYPE_DYNAMIC)   //如果是动态类型
    {
			if (current_index != 0 || blockOffset != vhdfc[0]->currentBlockOffset)
			{
        read_disk (vhdfc[0], 3, uFilePos);
				vhdfc[0]->currentBlockOffset = blockOffset;
				current_index = 0;
			}
			grub_memmove64(buf, (grub_size_t)(vhdfc[0]->blockData + offsetInBlock), txLen);
    }
    else   //如果是差分类型
    {
			//读磁盘，更新缓存区
      if (current_index != 0 || blockOffset != vhdfc[0]->currentBlockOffset)  //如果块偏移不等于当前块偏移
      {
				//确定磁盘的逻辑块，然后读磁盘到各自的缓存
				for (i = 0; ; i++)
				{
          //确定块逻辑扇区
					if (vhdfc[i]->diskType == VHD_DISKTYPE_FIXED)			//如果磁盘类型是固定
						vhdfc[i]->blockLBA = blockOffset >> 9;          //块逻辑扇区
					else
					{
						vhdfc[i]->blockLBA = *(grub_u32_t*)(vhdfc[i]->blockAllocationTable + blockNumber * 4);  //块逻辑扇区
						vhdfc[i]->blockLBA = bswap_32(vhdfc[i]->blockLBA);
					}
          
          //按类型读取
          if (vhdfc[i]->diskType == VHD_DISKTYPE_DYNAMIC)	//如果是动态类型
          {
            read_disk (vhdfc[i], 3, uFilePos);
            break;
          }
          else if (vhdfc[i]->diskType != VHD_DISKTYPE_DIFFERENCE)	//如果是固定类型
          {
            read_disk (vhdfc[i], 2, 0);
            break;
          }
          else if (vhdfc[0]->blockLBA != 0xFFFFFFFF)//如果是差分类型,非稀疏
          {
            read_disk (vhdfc[i], 4, 0);
          }
          else  //差分类型,未分配，位图填充0
          {
            grub_memset64((unsigned long long)(grub_size_t)vhdfc[1]->blockBitmapAndData, 0, vhdfc[0]->blockBitmapSize);
          }
        }
        //各差分磁盘穿透进父磁盘进行补差
        unsigned int *bitmap;
        bitmap = (unsigned int *)vhdfc[0]->blockBitmapAndData;
        
				for (i = 0; i < 0x80; i++)	//i是块号  1扇区0x80块
				{
          bitmap[i] = bswap_32(bitmap[i]);
          if (bitmap[i] == 0xFFFFFFFF)
          {
            grub_memmove64((unsigned long long)(grub_size_t)(vhdfc[end]->blockData + i*0x4000), (unsigned long long)(grub_size_t)(vhdfc[0]->blockData + i*0x4000), 0x4000);
          }
          else
          {
            for (j = 0; j < end; j++)
            {
              unsigned int *b;
              b = (unsigned int *)vhdfc[j]->blockBitmapAndData;
              b[i] = bswap_32(b[i]);
              if (b[i] != 0)
              {
                read_differ_itself (j,i); //读取差异本身  vhdfc[end] <- vhdfc[j]
                goto zzz;
              }
            }
zzz:
          }
        }
				vhdfc[0]->currentBlockOffset = blockOffset;
				current_index = 0;
			}

			grub_memmove64(buf, (unsigned long long)(grub_size_t)(vhdfc[end]->blockData + offsetInBlock), txLen);
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
