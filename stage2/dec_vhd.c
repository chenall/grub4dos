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

#define makeUInt32(b0,b1,b2,b3) (((unsigned long)(unsigned char)b0) + ((unsigned long)(unsigned char)b1<<8) + ((unsigned long)(unsigned char)b2<<16) + ((unsigned long)(unsigned char)b3<<24))
#define makeUInt64(b0,b1,b2,b3,b4,b5,b6,b7) (((unsigned long long)makeUInt32(b0,b1,b2,b3)) + ((unsigned long long)makeUInt32(b4,b5,b6,b7)<<32))

typedef struct VHDFooter {
	unsigned char cookie[8];
	unsigned char features[4];
	unsigned char fileFormatVersion[4];
	unsigned char dataOffset[8];
	unsigned char timeStamp[4];
	unsigned char creatorApplication[4];
	unsigned char creatorVersion[4];
	unsigned char creatorHostOS[4];
	unsigned char originalSize[8];
	unsigned char currentSize[8];
	struct {
		unsigned char cylinder[2];
		unsigned char heads;
		unsigned char sectorsPerTrack;
	} diskGeometry;
	unsigned char diskType[4];
	unsigned char checksum[4];
	unsigned char uniqueId[16];
	unsigned char savedState;
	unsigned char reserved[427];
} VHDFooter;

#define VHD_FOOTER_COOKIE      "conectix"
#define VHD_FOOTER_COOKIE_REV  0x636F6E6563746978ULL
#define VHD_DYNAMIC_COOKIE     "cxsparse"
#define VHD_DYNAMIC_COOKIE_REV 0x6378737061727365ULL

#define VHD_DISKTYPE_FIXED      2
#define VHD_DISKTYPE_DYNAMIC    3
#define VHD_DISKTYPE_DIFFERENCE 4

typedef struct VHDDynamicDiskHeader {
	unsigned char cookie[8];
	unsigned char dataOffset[8];
	unsigned char tableOffset[8];
	unsigned char headerVersion[4];
	unsigned char maxTableEntries[4];
	unsigned char blockSize[4];
	unsigned char checksum[4];
	unsigned char parentUniqueID[16];
	unsigned char parentTimeStamp[4];
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
	unsigned long blockSize;
	unsigned int  blockSizeLog2;
	unsigned long batEntries;
	unsigned long blockBitmapSize;
	unsigned char *blockAllocationTable;
	unsigned char *blockBitmapAndData;
	unsigned char *blockData;
	unsigned long currentBlockOffset;
	struct VHDFileControl *parentVHDFC;
};

extern unsigned long map_image_HPC;
extern unsigned long map_image_SPT;

VHDFileControl *vhdfc;

unsigned int log2pot32(unsigned long x);
unsigned short readBE16(unsigned char *p);
unsigned long readBE32(unsigned char *p);
unsigned long long readBE64(unsigned char *p);

unsigned int log2pot32(unsigned long x) {
	// x must be power of two
	return ((x & 0xFFFF0000) ? 16 : 0) | ((x & 0xFF00FF00) ? 8 : 0) | ((x & 0xF0F0F0F0) ? 4 : 0) | ((x & 0xCCCCCCCC) ? 2 : 0) | ((x & 0xAAAAAAAA) ? 1 : 0);
}
unsigned short readBE16(unsigned char *p) {
	unsigned char *pb = (unsigned char *)p;
	return (((unsigned short)pb[0] << 8) + pb[1]);
}
unsigned long readBE32(unsigned char *p) {
	unsigned char *pb = (unsigned char *)p;
	return (((((((unsigned long)pb[0] << 8) + pb[1]) << 8) + pb[2]) << 8) + pb[3]);
}
unsigned long long readBE64(unsigned char *p) {
	unsigned char *pb = (unsigned char *)p;
	return (((((((((((((((unsigned long long)pb[0] << 8) + pb[1]) << 8) + pb[2]) << 8) + pb[3]) << 8) + pb[4]) << 8) + pb[5]) << 8) + pb[6]) << 8) + pb[7]);
}

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

int
dec_vhd_open(void)
/* return 1=success or 0=failure */
{
	VHDFooter footer;
	VHDDynamicDiskHeader dynaheader;

	if (no_decompression) return 0;

	/* Now it does not support openning more than 1 file at a time. 
	   Make sure previously allocated memory blocks is freed. 
	   Don't need this line if grub_close is called for every openned file before grub_open is called for next file. */
	dec_vhd_close();

	memset(&footer, 0, 512);
	memset(&dynaheader, 0, 1024);

	filepos = (filemax - 1) & (unsigned long long)(-512LL);
	int bytestoread = filemax - filepos;
	int bytesread = (int)grub_read((unsigned long)&footer, bytestoread, 0xedde0d90);

	if (bytesread < 511) {
		// grub_printf("bytesread %d < 511\n",bytesread);
		return 0;
	}
	if (readBE64(footer.cookie)!=VHD_FOOTER_COOKIE_REV) {
		// grub_printf("cookie %lX != %lX\n", readBE64(footer.cookie), VHD_FOOTER_COOKIE_REV);
		return 0;
	}

	unsigned long diskType = readBE32(footer.diskType);
	if (/*diskType != VHD_DISKTYPE_FIXED && */diskType != VHD_DISKTYPE_DYNAMIC) {
		/* Differencing disk and unknown diskType are not supported */
		// grub_printf("diskType %d not supported\n", diskType);
		return 0;
	}

	if (diskType == VHD_DISKTYPE_DYNAMIC) {
		unsigned long long dataOffset = readBE64(footer.dataOffset);
		if (dataOffset + 1024 > filemax) {
			// grub_printf("footer dataOffset %lX\n", dataOffset);
			return 0;
		}
		filepos = dataOffset;
		bytesread = (int)grub_read((unsigned long)&dynaheader, 1024, 0xedde0d90);
	}

	vhdfc = (VHDFileControl*) grub_malloc(sizeof(VHDFileControl));
	if (!vhdfc) {
		return 0;
	}

	memset(vhdfc, 0, sizeof(VHDFileControl));
	vhdfc->cFileMax = filemax;
	vhdfc->volumeSize = readBE64(footer.currentSize);
	vhdfc->diskType = readBE32(footer.diskType);
	if (vhdfc->diskType == VHD_DISKTYPE_FIXED) {
	} else if (vhdfc->diskType == VHD_DISKTYPE_DYNAMIC) {
		vhdfc->tableOffset = readBE64(dynaheader.tableOffset);
		vhdfc->blockSize = readBE32(dynaheader.blockSize);
		vhdfc->blockSizeLog2 = log2pot32(vhdfc->blockSize);
		vhdfc->batEntries = readBE32(dynaheader.maxTableEntries);
		unsigned long batSize = (vhdfc->batEntries * 4 + 511)&(-512LL);
		vhdfc->blockAllocationTable = grub_malloc(batSize);
		vhdfc->blockBitmapSize = vhdfc->blockSize / (512 * 8);
		vhdfc->blockBitmapAndData = grub_malloc(vhdfc->blockBitmapSize + vhdfc->blockSize);
		vhdfc->blockData = vhdfc->blockBitmapAndData + vhdfc->blockBitmapSize;
		filepos = vhdfc->tableOffset;
		grub_read((unsigned long)vhdfc->blockAllocationTable, batSize, GRUB_READ);
		vhdfc->currentBlockOffset = -1LL;
	}
	map_image_HPC = footer.diskGeometry.heads;
	map_image_SPT = footer.diskGeometry.sectorsPerTrack;
	compressed_file = 1;
	decomp_type = DECOMP_TYPE_VHD;
	filemax = vhdfc->volumeSize;
	filepos = 0;

	errnum = ERR_NONE;
	return 1;
}

unsigned long long
dec_vhd_read(unsigned long long buf, unsigned long long len, unsigned long write)
{
	unsigned long long ret = 0;
	compressed_file = 0;
	filemax = vhdfc->cFileMax;
	if (filepos + len > vhdfc->volumeSize)
		len = (filepos <= vhdfc->volumeSize) ? vhdfc->volumeSize - filepos : 0;
	if (vhdfc->diskType == VHD_DISKTYPE_FIXED) {
		ret = grub_read(buf, len, write);
	} else {
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
			unsigned long blockNumber = (unsigned long)(uFilePos >> vhdfc->blockSizeLog2);
			unsigned long long blockOffset = (unsigned long long)blockNumber << vhdfc->blockSizeLog2;
			unsigned long offsetInBlock = (unsigned long)(uFilePos - blockOffset);
			unsigned long txLen = (rem < vhdfc->blockSize - offsetInBlock) ? rem : vhdfc->blockSize - offsetInBlock;
			unsigned long blockLBA = readBE32(vhdfc->blockAllocationTable + blockNumber * 4);
			// grub_printf("read bn %x of %x txlen %x lba %x\n", blockNumber, offsetInBlock, txLen, blockLBA);
			if (blockLBA == 0xFFFFFFFF) {
				// unused block on dynamic VHD. read zero
				grub_memset64(buf, 0, txLen);
			}
			else {
				if (blockOffset != vhdfc->currentBlockOffset) {
					filepos = blockLBA * 512;
					// grub_printf("read vhd lba %x filepos %lx\n", blockLBA, filepos);
					unsigned long long nread = grub_read((unsigned long)vhdfc->blockBitmapAndData, vhdfc->blockBitmapSize + vhdfc->blockSize, GRUB_READ);
					if (nread < vhdfc->blockBitmapSize + vhdfc->blockSize)
						break;
					vhdfc->currentBlockOffset = blockOffset;
				}
				grub_memmove64(buf, (unsigned long)(vhdfc->blockData + offsetInBlock), txLen);
			}
			buf += txLen;
			uFilePos += txLen;
			rem -= txLen;
			ret += txLen;
		}
		filepos = uFilePos;
	}
	compressed_file = 1;
	filemax = vhdfc->volumeSize;
	return ret;
}

#endif /* ! NO_DECOMPRESSION */
