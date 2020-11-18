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

typedef struct VHDFooter {
	unsigned char cookie[8];//string conectix
	grub_u32_t features;
	grub_u32_t fileFormatVersion;
	grub_u64_t dataOffset;
	grub_u32_t timeStamp;
	unsigned char creatorApplication[4];
	grub_u32_t creatorVersion;
	grub_u32_t creatorHostOS;
	grub_u64_t originalSize;
	grub_u64_t currentSize;
	struct {
		unsigned short cylinder;
		unsigned char heads;
		unsigned char sectorsPerTrack;
	} diskGeometry;
	grub_u32_t diskType;
	grub_u32_t checksum;
	unsigned char uniqueId[16];
	unsigned char savedState;
	unsigned char reserved[427];
} VHDFooter;

#define VHD_FOOTER_COOKIE      0x78697463656E6F63ULL
#define VHD_DYNAMIC_COOKIE     0x6573726170737863ULL

#define VHD_DISKTYPE_FIXED      2
#define VHD_DISKTYPE_DYNAMIC    3
#define VHD_DISKTYPE_DIFFERENCE 4

typedef struct VHDDynamicDiskHeader {
	unsigned char cookie[8];//string cxsparse
	grub_u64_t dataOffset;
	grub_u64_t tableOffset;
	grub_u32_t headerVersion;
	grub_u32_t maxTableEntries;
	grub_u32_t blockSize;
	grub_u32_t checksum;
	unsigned char parentUniqueID[16];
	grub_u32_t parentTimeStamp;
	unsigned char reserved[4];
	unsigned char parentUnicodeName[512];
	unsigned char parentLocaterEntry[8][24];
	unsigned char reserved2[256];
} VHDDynamicDiskHeader;

typedef struct VHDFileControl VHDFileControl;

struct VHDFileControl {
	unsigned long long cFileMax;
	unsigned long long volumeSize;
	unsigned long long tableOffset;
	unsigned int  diskType;
	unsigned int blockSize;
	unsigned int  blockSizeLog2;
	unsigned int batEntries;
	unsigned int blockBitmapSize;
	unsigned char *blockAllocationTable;
	unsigned char *blockBitmapAndData;
	unsigned char *blockData;
	unsigned int currentBlockOffset;
	struct VHDFileControl *parentVHDFC;
};

extern unsigned int map_image_HPC;
extern unsigned int map_image_SPT;

VHDFileControl *vhdfc;

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
void bswap_64(grub_u64_t *x)
{
  grub_u32_t hi = (grub_u32_t)*x;
  grub_u32_t lo = (grub_u32_t)(*x >> 32);
  *x = ((grub_u64_t)bswap_32(&hi)<<32)|bswap_32(&lo);
}

void vhd_footer_in(VHDFooter *footer);
void vhd_footer_in(VHDFooter *footer)
{
//	bswap_32(&footer->features);
//	bswap_32(&footer->fileFormatVersion);
	bswap_64(&footer->dataOffset);
//	bswap_32(&footer->timeStamp);
//	bswap_32(&footer->creatorVersion);
//	bswap_32(&footer->creatorHostOS);
//	bswap_64(&footer->originalSize);
	bswap_64(&footer->currentSize);
	bswap_32(&footer->diskType);
//	bswap_32(&footer->checksum);
}

void vhd_header_in(VHDDynamicDiskHeader *header);
void vhd_header_in(VHDDynamicDiskHeader *header)
{
//	bswap_64(&header->dataOffset);
	bswap_64(&header->tableOffset);
//	bswap_32(&header->headerVersion);
	bswap_32(&header->maxTableEntries);
	bswap_32(&header->blockSize);
//	bswap_32(&header->checksum);
//	bswap_32(&header->parentTimeStamp);
}

void dec_vhd_close(void);
void
dec_vhd_close(void)
{
	if (vhdfc) {
		if (vhdfc->blockAllocationTable) {
			grub_free(vhdfc->blockAllocationTable);
		}
		if (vhdfc->blockBitmapAndData) {
			grub_free(vhdfc->blockBitmapAndData);
		}
		grub_free(vhdfc);
		map_image_HPC = 0;
		map_image_SPT = 0;
	}
}

int dec_vhd_open(void);
int
dec_vhd_open(void)
/* return 1=success or 0=failure */
{
	VHDFooter footer;
	VHDDynamicDiskHeader dynaheader;

  if (filemax < 0x10000) return 0;//file is to small
	/* Now it does not support openning more than 1 file at a time. 
	   Make sure previously allocated memory blocks is freed. 
	   Don't need this line if grub_close is called for every openned file before grub_open is called for next file. */
	dec_vhd_close();

	memset(&footer, 0, sizeof(footer));
	memset(&dynaheader, 0, sizeof(dynaheader));

	int bytesread = (int)grub_read((grub_size_t)&footer, 0x200, 0xedde0d90);
	bytesread = bytesread;
	//if (bytesread < 511) {
		// grub_printf("bytesread %d < 511\n",bytesread);
	//	goto quit;
	//}
  grub_u64_t* a = (grub_u64_t*)&footer.cookie;
//	if (*(grub_u64_t*)&footer.cookie!=VHD_FOOTER_COOKIE) {
  if (*a!=VHD_FOOTER_COOKIE) {
		// grub_printf("cookie %lX != %lX\n", footer.cookie, VHD_FOOTER_COOKIE);
		goto quit;
	}

  vhd_footer_in(&footer);

	if (footer.diskType != VHD_DISKTYPE_DYNAMIC) {
		/* Differencing disk and unknown diskType are not supported */
		goto quit;
	}

//	if (footer.diskType == VHD_DISKTYPE_DYNAMIC) {
		if (footer.dataOffset + sizeof(dynaheader) > filemax) {
			// grub_printf("footer dataOffset %lX\n", dataOffset);
			goto quit;
		}
		filepos = footer.dataOffset;
		bytesread = (int)grub_read((grub_size_t)&dynaheader, sizeof(dynaheader), 0xedde0d90);
//	}

	vhdfc = (VHDFileControl*) grub_malloc(sizeof(VHDFileControl));
	if (!vhdfc) {
		goto quit;
	}

	memset(vhdfc, 0, sizeof(VHDFileControl));
	vhd_header_in(&dynaheader);
	vhdfc->cFileMax = filemax;
	vhdfc->volumeSize = footer.currentSize;
	vhdfc->diskType = footer.diskType;
	//if (vhdfc->diskType == VHD_DISKTYPE_FIXED) {
	//} else if (vhdfc->diskType == VHD_DISKTYPE_DYNAMIC) {
		vhdfc->tableOffset = dynaheader.tableOffset;
		vhdfc->blockSize = dynaheader.blockSize;
		vhdfc->blockSizeLog2 = log2pot32(vhdfc->blockSize);
		vhdfc->batEntries = dynaheader.maxTableEntries;
		unsigned int batSize = (vhdfc->batEntries * 4 + 511)&(-512LL);
		vhdfc->blockAllocationTable = grub_malloc(batSize);
		vhdfc->blockBitmapSize = vhdfc->blockSize / (512 * 8);
		vhdfc->blockBitmapAndData = grub_malloc(vhdfc->blockBitmapSize + vhdfc->blockSize);
		vhdfc->blockData = vhdfc->blockBitmapAndData + vhdfc->blockBitmapSize;
		filepos = vhdfc->tableOffset;
		grub_read((grub_u64_t)(grub_size_t)vhdfc->blockAllocationTable, batSize, GRUB_READ);
		vhdfc->currentBlockOffset = -1LL;
	//}
	map_image_HPC = footer.diskGeometry.heads;
	map_image_SPT = footer.diskGeometry.sectorsPerTrack;
	compressed_file = 1;
	decomp_type = DECOMP_TYPE_VHD;
	filemax = vhdfc->volumeSize;
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
	//if (vhdfc->diskType == VHD_DISKTYPE_FIXED) {
	//	ret = grub_read(buf, len, write);
	//} else {
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
		while (rem) {
			unsigned int blockNumber = (unsigned int)(uFilePos >> vhdfc->blockSizeLog2);
			unsigned long long blockOffset = (unsigned long long)blockNumber << vhdfc->blockSizeLog2;
			unsigned int offsetInBlock = (unsigned int)(uFilePos - blockOffset);
			unsigned int txLen = (rem < vhdfc->blockSize - offsetInBlock) ? rem : vhdfc->blockSize - offsetInBlock;
			grub_u32_t blockLBA = *(grub_u32_t*)(vhdfc->blockAllocationTable + blockNumber * 4);
			bswap_32(&blockLBA);
			// grub_printf("read bn %x of %x txlen %x lba %x\n", blockNumber, offsetInBlock, txLen, blockLBA);
			if (blockLBA == 0xFFFFFFFF) {
				// unused block on dynamic VHD. read zero
				grub_memset64(buf, 0, txLen);
			}
			else {
				if (blockOffset != vhdfc->currentBlockOffset) {
					filepos = blockLBA * 512;
					// grub_printf("read vhd lba %x filepos %lx\n", blockLBA, filepos);
					unsigned long long nread = grub_read((grub_size_t)vhdfc->blockBitmapAndData, vhdfc->blockBitmapSize + vhdfc->blockSize, GRUB_READ);
					if (nread < vhdfc->blockBitmapSize + vhdfc->blockSize)
						break;
					vhdfc->currentBlockOffset = blockOffset;
				}
				grub_memmove64(buf, (grub_size_t)(vhdfc->blockData + offsetInBlock), txLen);
			}
			buf += txLen;
			uFilePos += txLen;
			rem -= txLen;
			ret += txLen;
		}
		filepos = uFilePos;
//	}
	compressed_file = 1;
	filemax = vhdfc->volumeSize;
	return ret;
}

#endif /* ! NO_DECOMPRESSION */
