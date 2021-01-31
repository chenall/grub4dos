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

typedef struct VHDFooter {              //VHD页脚表  尺寸512
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

typedef struct VHDFileControl VHDFileControl;

struct VHDFileControl { //VHD控制文件
	unsigned long long cFileMax;          //文件最大尺寸
	unsigned long long volumeSize;        //卷尺寸  VHD当前尺寸
	unsigned long long tableOffset;       //BAT结构偏移
	unsigned int  diskType;               //磁盘类型
	unsigned int blockSize;               //块尺寸
	unsigned int  blockSizeLog2;          //块尺寸2的幂
	unsigned int batEntries;              //BAT条目的最大值
	unsigned int blockBitmapSize;         //块位图尺寸
	unsigned char *blockAllocationTable;  //块分配表指针
	unsigned char *blockBitmapAndData;    //块位图和数据指针
	unsigned char *blockData;             //块数据指针
  unsigned char uniqueId[16];           //唯一Id
	unsigned int currentBlockOffset;      //当前块缓存
} __attribute__ ((packed));

VHDFileControl *vhdfc = 0;
VHDFileControl *parentVHDFC = 0;
struct fragment *ParentDisk = 0;

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
	bswap_64(&footer->dataOffset);        //数据偏移  200
	bswap_64(&footer->currentSize);       //当前尺寸  4004000
	bswap_32(&footer->diskType);          //磁盘类型  3
}

void vhd_header_in(VHDDynamicDiskHeader *header);
void vhd_header_in(VHDDynamicDiskHeader *header)  //VHD动态磁盘头in
{
	bswap_64(&header->tableOffset);       //BAT结构偏移     600
	bswap_32(&header->maxTableEntries);   //BAT条目的最大值 21
	bswap_32(&header->blockSize);         //块尺寸          200000
}

void dec_vhd_close(void);
void
dec_vhd_close(void)
{
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
}

int dec_vhd_open(void);
int
dec_vhd_open(void)
/* return 1=success or 0=failure */
{
	VHDFooter *footer = 0;
	VHDDynamicDiskHeader *dynaheader = 0;
  int i = 0;

  if (filemax < 0x10000) return 0;//file is to small
	/* Now it does not support openning more than 1 file at a time. 现在它不支持一次打开多个文件。
	   Make sure previously allocated memory blocks is freed.       确保先前分配的内存块已释放。
	   Don't need this line if grub_close is called for every openned file before grub_open is called for next file. */
     //如果在为下一个文件调用grub_open之前为每个打开的文件调用grub_close，则不需要此行。
  if (vhdfc) //如果子VHD打开过 
    goto quit;
 
	dec_vhd_close();
  footer = grub_zalloc (sizeof(VHDFooter));
  if (!footer)
    goto quit;

	grub_read((unsigned long long)(grub_size_t)(char *)footer, sizeof(VHDFooter), 0xedde0d90); //动态及差分VHD，起始1扇区是VHD页脚表。固定VHD的结构表在最后一个扇区。
  grub_u64_t* a = (grub_u64_t*)footer->cookie;
  if (*a!=VHD_FOOTER_COOKIE) {    //不是VHD页脚表标记,退出   (包含固定VHD)
    
		goto quit_1;
	}

  vhd_footer_in(footer);
	if (footer->diskType != VHD_DISKTYPE_DYNAMIC && footer->diskType != VHD_DISKTYPE_DIFFERENCE)  //磁盘类型不是动态, 不是差分, 退出
		goto quit_1; 

		if (footer->dataOffset + sizeof(VHDDynamicDiskHeader) > filemax) { //数据偏移+VHD页脚表 > filemax, 退出
			goto quit_1;
		}
  dynaheader = grub_zalloc (sizeof(VHDDynamicDiskHeader));
  if (!dynaheader)
    goto quit_1;  
		filepos = footer->dataOffset;
		grub_read((unsigned long long)(grub_size_t)(char *)dynaheader, sizeof(VHDDynamicDiskHeader), 0xedde0d90); //读VHD动态磁盘头 
    
  vhdfc = (VHDFileControl*) grub_zalloc(sizeof(VHDFileControl));
  if (!vhdfc)
    goto quit_2;

  //设置VHD控制文件
	vhd_header_in(dynaheader);
	vhdfc->cFileMax = filemax;                          
	vhdfc->volumeSize = footer->currentSize;
	vhdfc->diskType = footer->diskType;
		vhdfc->tableOffset = dynaheader->tableOffset;
		vhdfc->blockSize = dynaheader->blockSize;
		vhdfc->blockSizeLog2 = log2pot32(vhdfc->blockSize);
		vhdfc->batEntries = dynaheader->maxTableEntries;
		unsigned int batSize = (vhdfc->batEntries * 4 + 511)&(-512LL);
		vhdfc->blockAllocationTable = grub_malloc(batSize);
		vhdfc->blockBitmapSize = vhdfc->blockSize / (512 * 8);
		vhdfc->blockBitmapAndData = grub_malloc(vhdfc->blockBitmapSize + vhdfc->blockSize);
		vhdfc->blockData = vhdfc->blockBitmapAndData + vhdfc->blockBitmapSize;
		filepos = vhdfc->tableOffset;
		grub_read((grub_u64_t)(grub_size_t)vhdfc->blockAllocationTable, batSize, GRUB_READ);//读BAT结构
		vhdfc->currentBlockOffset = -1LL;
  grub_memmove(&vhdfc->uniqueId, &footer->uniqueId, 16);
   
  //如果是差分
  if (footer->diskType == VHD_DISKTYPE_DIFFERENCE)
  {
    VHDFooter *footerParent = 0;
    VHDDynamicDiskHeader *dynaheaderParent = 0;
    char *parentName = 0;
    
    footerParent = grub_zalloc (sizeof(VHDFooter));                     //父VHD页脚表
    dynaheaderParent = grub_zalloc (sizeof(VHDDynamicDiskHeader));      //父VHD动态磁盘头
    parentVHDFC = (VHDFileControl*) grub_zalloc(sizeof(VHDFileControl));//父VHD控制文件
    parentName = grub_zalloc (256);
    if (!footerParent || !dynaheaderParent || !parentVHDFC || !parentName)
      goto quitP;
    //获取父VHD文件名
    GetParentUtf8Name (parentName, (grub_uint16_t *)dynaheader->parentUnicodeName);
    grub_close ();  //关闭子VHD  
    get_ParentDisk (parentName, &ParentDisk);     //获得父VHD文件扇区序列    
    if (!ParentDisk)
    {
      printf_debug ("Parent VHD not found.\n");
      goto quitP; 
    }
    //查父VHD页脚表
    while (ParentDisk[i].start_sector)  //查找最后块
      i++;
    grub_efidisk_readwrite (current_drive, ParentDisk[i-1].start_sector + ParentDisk[i-1].sector_count - 1,
        sizeof(VHDFooter), (char *)footerParent, GRUB_READ);            //读父VHD页脚表
    a = (grub_u64_t*)footerParent->cookie;
    if (*a!=VHD_FOOTER_COOKIE)                                          //不是VHD页脚表标记,退出   (包含固定VHD)
      goto quitP_1;
    vhd_footer_in(footerParent);
    if (footerParent->diskType == VHD_DISKTYPE_DIFFERENCE)                 //如果父磁盘类型是差分, 退出
      goto quitP_1;
		if (footerParent->dataOffset + sizeof(VHDDynamicDiskHeader) > filemax) //数据偏移+VHD页脚表 > filemax, 退出
			goto quitP_1;
    if (grub_memcmp ((const char *)dynaheader->parentUniqueID, (const char *)footerParent->uniqueId, 16))			//比较父VHD的guid  
    {
      printf_debug ("GUID of parent VHD does not match.\n");
      goto quitP_1;
    }
    
    parentVHDFC->volumeSize = footerParent->currentSize;
    parentVHDFC->diskType = footerParent->diskType;
    parentVHDFC->blockSize = vhdfc->blockSize;
    parentVHDFC->blockSizeLog2 = vhdfc->blockSizeLog2;
    parentVHDFC->batEntries = vhdfc->batEntries;
    unsigned int parentbatSize = (parentVHDFC->batEntries * 4 + 511)&(-512LL);
    parentVHDFC->blockAllocationTable = grub_malloc(parentbatSize);

    if (footerParent->diskType == VHD_DISKTYPE_FIXED)                   //如果父磁盘类型是固定, 成功结束
    {
      parentVHDFC->blockBitmapSize = 0;
      parentVHDFC->blockBitmapAndData = grub_malloc(parentVHDFC->blockBitmapSize + parentVHDFC->blockSize);
      parentVHDFC->blockData = parentVHDFC->blockBitmapAndData + parentVHDFC->blockBitmapSize;
    }
    else
    {
      grub_efidisk_readwrite (current_drive, ParentDisk[0].start_sector + (footerParent->dataOffset >> buf_geom.log2_sector_size),
          sizeof(VHDDynamicDiskHeader), (char *)dynaheaderParent, GRUB_READ); //读父VHD动态磁盘头
    
      //设置父VHD控制文件
      vhd_header_in(dynaheaderParent);                        
      parentVHDFC->tableOffset = dynaheaderParent->tableOffset;
      parentVHDFC->blockBitmapSize = parentVHDFC->blockSize / (512 * 8);
      parentVHDFC->blockBitmapAndData = grub_malloc(parentVHDFC->blockBitmapSize + parentVHDFC->blockSize);
      parentVHDFC->blockData = parentVHDFC->blockBitmapAndData + parentVHDFC->blockBitmapSize;
      grub_efidisk_readwrite (current_drive, ParentDisk[0].start_sector + (parentVHDFC->tableOffset >> buf_geom.log2_sector_size),
          (unsigned long long)parentbatSize, (char *)parentVHDFC->blockAllocationTable, GRUB_READ); //读父BAT结构
    }

    grub_close ();  //关闭父VHD
    grub_open (map_file_name);	//打开子VHD
    grub_free (parentName);
    parentName = 0;
    grub_free (dynaheaderParent);
    dynaheaderParent = 0;
    grub_free (footerParent);
    footerParent = 0;
    goto normalP;

quitP_1:
    grub_free (ParentDisk);
    ParentDisk = 0;
quitP:
    grub_free (parentName);
    grub_free (footerParent);
    grub_free (dynaheaderParent);
    grub_free (parentVHDFC);
    parentName = 0;
    footerParent = 0;
    dynaheaderParent = 0;
    parentVHDFC = 0;
    goto quit_2;
	}
normalP:  
	compressed_file = 1;            //压缩文件
	decomp_type = DECOMP_TYPE_VHD;  //解压缩类型VHD 
	filemax = vhdfc->volumeSize;    //修改filemax

quit_2:
  grub_free (dynaheader);
  dynaheader = 0;
quit_1:
  grub_free (footer);
  footer = 0;
quit:
	filepos = 0;

	errnum = ERR_NONE;
	return compressed_file;
}

unsigned long long dec_vhd_read(unsigned long long buf, unsigned long long len, unsigned int write);
unsigned long long
dec_vhd_read(unsigned long long buf, unsigned long long len, unsigned int write)
{
	unsigned long long ret = 0;
	compressed_file = 0;
	filemax = vhdfc->cFileMax;
	if (filepos + len > vhdfc->volumeSize)
		len = (filepos <= vhdfc->volumeSize) ? vhdfc->volumeSize - filepos : 0;
		// VHD_DISKTYPE_DYNAMIC
		if (write == GRUB_WRITE) {
			errnum = ERR_WRITE_GZIP_FILE;
			return 0;
		}
		unsigned long long uFilePos = filepos;
		if (len > vhdfc->volumeSize - uFilePos)
			len = vhdfc->volumeSize - uFilePos;
		errnum = ERR_NONE;
		unsigned long long rem = len;
  unsigned int blockNumber;
  unsigned long long blockOffset;
  unsigned int offsetInBlock;
  unsigned int txLen;
  grub_u32_t blockLBA;
  grub_u32_t parent_blockLBA;
  unsigned long long total, fragment_len; 
  unsigned long long nread;
  unsigned int i, j, k;

  while (rem) {
    blockNumber = (unsigned int)(uFilePos >> vhdfc->blockSizeLog2);                             //块号
    blockOffset = (unsigned long long)blockNumber << vhdfc->blockSizeLog2;                      //块偏移
    offsetInBlock = (unsigned int)(uFilePos - blockOffset);                                     //在块内偏移
    txLen = (rem < vhdfc->blockSize - offsetInBlock) ? rem : vhdfc->blockSize - offsetInBlock;  //读尺寸
    blockLBA = *(grub_u32_t*)(vhdfc->blockAllocationTable + blockNumber * 4);                   //块逻辑扇区
    bswap_32(&blockLBA);                                                                        //大尾转小尾

    if (vhdfc->diskType == VHD_DISKTYPE_DYNAMIC)   //如果是动态
    {
			// grub_printf("read bn %x of %x txlen %x lba %x\n", blockNumber, offsetInBlock, txLen, blockLBA);
			if (blockLBA == 0xFFFFFFFF) {
				// unused block on dynamic VHD. read zero  动态VHD上未使用的块。读取0 
				grub_memset64(buf, 0, txLen);
			}
			else {
				if (blockOffset != vhdfc->currentBlockOffset) {
					filepos = blockLBA * 512;
					// grub_printf("read vhd lba %x filepos %lx\n", blockLBA, filepos);
					nread = grub_read((grub_size_t)vhdfc->blockBitmapAndData, vhdfc->blockBitmapSize + vhdfc->blockSize, GRUB_READ);
					if (nread < vhdfc->blockBitmapSize + vhdfc->blockSize)
						break;
					vhdfc->currentBlockOffset = blockOffset;
				}
				grub_memmove64(buf, (grub_size_t)(vhdfc->blockData + offsetInBlock), txLen);
			}
    }
    else   //如果是差分
    {
      if (parentVHDFC->diskType == VHD_DISKTYPE_DYNAMIC)  //如果父磁盘类型是动态
      {
        parent_blockLBA = *(grub_u32_t*)(parentVHDFC->blockAllocationTable + blockNumber * 4);  //父块逻辑扇区
        bswap_32(&parent_blockLBA);
        
        if (blockLBA == 0xFFFFFFFF && parent_blockLBA == 0xFFFFFFFF)   //如果BAT与父BAT都是0xFFFFFFFF
        {
          grub_memset64(buf, 0, txLen);
          goto quit;
        }
      }
      else    //如果父磁盘类型是固定
        parent_blockLBA = blockOffset >> buf_geom.log2_sector_size;

      if (blockOffset != vhdfc->currentBlockOffset)                   //如果块偏移不等于当前块偏移
      {
        //确定Form扇区起始在哪个碎片
        total = 0;
        fragment_len = 0;
        i = 0;
        unsigned char *blockBitmapAndData_back = parentVHDFC->blockBitmapAndData;
        while (1)
        {
          total += ParentDisk[i++].sector_count;
          if (parent_blockLBA < total)		//确定起始位置的条件
            break;
        }
        //确定本碎片最大访问字节
        fragment_len = (total - parent_blockLBA) << buf_geom.log2_sector_size;
        //确定扇区起始的确切位置
        parent_blockLBA += ParentDisk[i-1].start_sector + ParentDisk[i-1].sector_count - total;
        nread = parentVHDFC->blockBitmapSize + parentVHDFC->blockSize;  //读位图与数据,200200字节
        //确定实际读字节数
        if (fragment_len >= nread)
          total = nread;        
        else
          total = fragment_len;
        nread -= total; //剩余读字节数
        grub_efidisk_readwrite (current_drive, parent_blockLBA, total, (char *)blockBitmapAndData_back, GRUB_READ);
        blockBitmapAndData_back += total; //调整缓存

        while (nread)   //如果需要读
        {
          fragment_len = ParentDisk[i++].sector_count << buf_geom.log2_sector_size; //下一碎片尺寸
          parent_blockLBA = ParentDisk[i++].start_sector;                           //下一碎片起始
          //确定实际读字节数
          if (fragment_len >= nread)
            total = nread;
          else
            total = fragment_len;
          
          grub_efidisk_readwrite (current_drive, parent_blockLBA, total, (char *)blockBitmapAndData_back, GRUB_READ);
          nread -= total; //剩余读字节数
          blockBitmapAndData_back += total; //调整缓存
        }
        if (blockLBA != 0xFFFFFFFF)
        {
          filepos = blockLBA * 512; 
          grub_read((grub_size_t)vhdfc->blockBitmapAndData, vhdfc->blockBitmapSize + vhdfc->blockSize, GRUB_READ);
          unsigned int *bitmap = (unsigned int *)vhdfc->blockBitmapAndData;
          unsigned int test;

          for (i = 0; i < 0x80; i++)
          {
            test = 0x80000000;
            j = 0;

            if (bitmap[i] != 0)
            {
              bswap_32(&bitmap[i]);
              while (test)
              {
                if (bitmap[i] & test) //bitmap位为1, 直接读取子VHD
                {
                  k = 0;
                  while (bitmap[i] & test)
                  {
                    test >>= 1;
                    k++;
                  } 
                  grub_memmove64((unsigned long long)(grub_size_t)(parentVHDFC->blockData + (((i << 5) + j) << buf_geom.log2_sector_size)), 
                        (unsigned long long)(grub_size_t)(vhdfc->blockData + (((i << 5) + j) << buf_geom.log2_sector_size)), 
                        buf_geom.sector_size * k);
                  j += k;
                }
                else
                {
                  test >>= 1;
                  j++;
                }
              }
            }
          }
        }
        vhdfc->currentBlockOffset = blockOffset;
      }
      grub_memmove64(buf, (unsigned long long)(grub_size_t)(parentVHDFC->blockData + offsetInBlock), txLen);
    }
quit: 
			buf += txLen;
			uFilePos += txLen;
			rem -= txLen;
			ret += txLen;
		}
		filepos = uFilePos;
	compressed_file = 1;
	filemax = vhdfc->volumeSize;
	return ret;
}

#endif /* ! NO_DECOMPRESSION */
