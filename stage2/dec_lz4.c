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

#define LZ4_MAGIC_NUMBER 0x184D2204

/* initial dic pos 64K */
#define LZ4_DICPOSSTART   0x10000UL
/* dictionary size 8 MB */
#define LZ4_DICBUFSIZE   0x800000UL
/* input buffer size 8 MB */
#define LZ4_INPBUFSIZE   0x800000UL

struct {
	unsigned char flg, bd, hc;
	unsigned char flg_version, flg_bindep, flg_bchecksum, flg_csize, flg_cchecksum, flg_reserved;
	unsigned char bd_blockmaxsize, bd_reserved;
	unsigned int blockMaxSize;    /* max uncompressed size for one block can be 64K,256K,1M,4M */
	unsigned int headerSize;
	unsigned int nextBlockSize;   /* compressed size */
	unsigned long long content_size;
	unsigned long long cfilemax, cfilepos, ufilemax, ufilepos;
	unsigned long long dicFilePos; /* uncompress file pos for data at (dic + LZ4_DICPOSSTART) */
	unsigned long long inpFilePos;
	unsigned int dicPos, dicSize, inpPos, inpSize;
	unsigned char *inp;
	unsigned char *dic;
} lz4dec;

void dec_lz4_close(void);
void
dec_lz4_close(void)
{
	if (lz4dec.inp) { grub_free(lz4dec.inp); lz4dec.inp = 0; }
	if (lz4dec.dic) { grub_free(lz4dec.dic); lz4dec.dic = 0; }
}

int dec_lz4_open(void);
int
dec_lz4_open(void)
/* return 1=success or 0=failure */
{
	unsigned char header[20];

	if (no_decompression) return 0;

	/* Now it does not support openning more than 1 file at a time. 
	   Make sure previously allocated memory blocks is freed. 
	   Don't need this line if grub_close is called for every openned file before grub_open is called for next file. */
	dec_lz4_close();
	filepos = 0;
	int bytestoread = (filemax<20) ? (int)filemax : 20;
	int bytesread = (int)grub_read((unsigned long long)(grub_size_t)(char *)header, bytestoread, GRUB_READ);
	/* check header */
  grub_u32_t* a = (grub_u32_t*)&header;
//	if (bytesread < (4+2+1+4) || *(grub_u32_t*)(grub_size_t)&header != LZ4_MAGIC_NUMBER) {
  if (bytesread < (4+2+1+4) || *a != LZ4_MAGIC_NUMBER) {
		/* file is not LZ4 frame */
		filepos = 0;
		return 0;
	}
	/* file has LZ4 magic number */
	/* check header */
	lz4dec.flg = header[4];
	lz4dec.flg_version = (lz4dec.flg >> 6) & 3;
	lz4dec.flg_bindep = (lz4dec.flg >> 5) & 1;
	lz4dec.flg_bchecksum = (lz4dec.flg >> 4) & 1;
	lz4dec.flg_csize = (lz4dec.flg >> 3) & 1;
	lz4dec.flg_cchecksum = (lz4dec.flg >> 2) & 1;
	lz4dec.flg_reserved = (lz4dec.flg) & 3;
	lz4dec.bd = header[5];
	lz4dec.bd_blockmaxsize = (lz4dec.bd >> 4) & 7;
	lz4dec.bd_reserved = (lz4dec.bd) & 0x8f;
	int pos = 6;

	if (lz4dec.flg_version != 1) {
		/* grub_printf("version != 1\n"); */
		goto fail;
	}
	if (lz4dec.flg_reserved != 0 || lz4dec.bd_reserved != 0) {
		/* grub_printf("reserved != 0\n"); */
		goto fail;
	}
	if (lz4dec.flg_csize != 0) {
		if (bytesread < (4 + 2 + 8 + 1 + 4)) {
			/* grub_printf("bytesread %d < 4+2+8+1+4\n", bytesread); */
			goto fail;
		}
		lz4dec.content_size = *(grub_u64_t*)(grub_size_t)&header[pos];
		pos += 8;
	}
	else {
		/* grub_printf("no content size\n"); */
		lz4dec.content_size = 0;
		goto fail;
	}
	lz4dec.hc = header[pos++];
	if (lz4dec.bd_blockmaxsize < 4) {
		/* grub_printf("unsupported blockmaxsize field %d\n", lz4dec.bd_blockmaxsize); */
		goto fail;
	}
	else {
		lz4dec.blockMaxSize = 256UL << (lz4dec.bd_blockmaxsize * 2);
		/* grub_printf("blockMaxSize %d\n", lz4dec.blockMaxSize); */
	}
	/* valid header */
	lz4dec.headerSize = pos;
  grub_u32_t* b = (grub_u32_t*)&header[pos];
//	lz4dec.nextBlockSize = *(grub_u32_t*)(grub_size_t)&header[pos];
  lz4dec.nextBlockSize = *b;
	pos += 4;
	lz4dec.cfilemax = filemax;
	lz4dec.cfilepos = pos;
	lz4dec.ufilemax = lz4dec.content_size;
	lz4dec.ufilepos = 0;
	lz4dec.inp = (unsigned char *)grub_malloc(LZ4_INPBUFSIZE);
	lz4dec.dic = (unsigned char *)grub_malloc(LZ4_DICBUFSIZE);
	if (lz4dec.inp == 0 || lz4dec.dic == 0) {
		if (lz4dec.inp) { grub_free(lz4dec.inp); lz4dec.inp = 0; }
		if (lz4dec.dic) { grub_free(lz4dec.dic); lz4dec.dic = 0; }
		errnum = ERR_NOT_ENOUGH_MEMORY;;
		filepos = 0;
		return 0;
	}
	decomp_type = DECOMP_TYPE_LZ4;
	compressed_file = 1;
	filemax = lz4dec.ufilemax;
	filepos = lz4dec.ufilepos;
	gzip_filemax = lz4dec.cfilemax;
	lz4dec.inpSize = 0;
	lz4dec.inpPos = 0;
	lz4dec.dicPos = LZ4_DICPOSSTART;
	lz4dec.dicFilePos = 0;
	memset(lz4dec.dic, 0, LZ4_DICPOSSTART);
	/* success */
	errnum = ERR_NONE;
	return 1;
fail:
	errnum = ERR_BAD_GZIP_HEADER;
	filepos = 0;
	return 0;
}

unsigned long long dec_lz4_read(unsigned long long buf, unsigned long long len, unsigned int write);
unsigned long long
dec_lz4_read(unsigned long long buf, unsigned long long len, unsigned int write)
{
	unsigned long long outTx, outSkip;
	/* grub_printf("LZ4 read buf=%lX len=%lX dic=%X inp=%X\n",buf,len,lz4dec.dic,lz4dec.inp);
	getkey();
	*/

	compressed_file = 0;

	lz4dec.ufilemax = filemax;
	lz4dec.ufilepos = filepos;
	filemax = lz4dec.cfilemax;
	filepos = lz4dec.cfilepos;

	/* Now filemax, filepos are of compressed file
	* ufilepos, ufilepos are of uncompressed data
	* cfilemax, cfilepos are not used
	*/

	/* if reading before dic, reset decompression to beginning */
	if (lz4dec.ufilepos + LZ4_DICPOSSTART < lz4dec.dicFilePos) {
		filepos = lz4dec.headerSize;
		grub_read((grub_u64_t)(grub_size_t)&lz4dec.nextBlockSize, 4, GRUB_READ);
		lz4dec.inpSize = 0;
		lz4dec.inpPos = 0;
		lz4dec.dicPos = LZ4_DICPOSSTART;
		lz4dec.dicFilePos = 0;
		memset(lz4dec.dic, 0, LZ4_DICPOSSTART);
	}

	outTx = 0;
	outSkip = lz4dec.ufilepos + LZ4_DICPOSSTART - lz4dec.dicFilePos;
	errnum = ERR_NONE;

	while (len && !errnum)
	{
		/* Copy uncompressed data from dic in range dic[0]...dic[dicPos-1] */
		if (outSkip < lz4dec.dicPos)
		{
			unsigned int outTxCur = lz4dec.dicPos - (unsigned int)outSkip;
			if (outTxCur > len)
				outTxCur = (unsigned int)len;
			if (buf) {
				grub_memmove64(buf, (grub_size_t)(lz4dec.dic + outSkip), outTxCur);
				buf += outTxCur;
			}
			outSkip = lz4dec.dicPos;
			outTx += outTxCur;
			lz4dec.ufilepos += outTxCur;
			len -= outTxCur;
			if (len == 0) {
				break;
			}
		}
		/*while (lz4dec.nextBlockSize && lz4dec.dicPos + lz4dec.blockMaxSize <= LZ4_DICBUFSIZE) */{
			/* All existing wanted data from dic have been copied. We will have to decompress more data. */
			/* Read next compressed block (with optional checksum) with next block size */
			unsigned int blockSize = lz4dec.nextBlockSize;
			//grub_printf("blockSize %X\n",blockSize);
			if (blockSize == 0) break;
			int bUncompressedBlock = ((blockSize & 0x80000000) != 0);
			blockSize &= 0x7FFFFFFF;
			unsigned int inSizeCur = blockSize + lz4dec.flg_bchecksum * 4 + 4;
			//grub_printf("read filepos %lX size %X inp %X\n", filepos, inSizeCur, lz4dec.inp);
			inSizeCur = grub_read((grub_u64_t)(grub_size_t)lz4dec.inp, inSizeCur, GRUB_READ);
			lz4dec.inpSize = inSizeCur;
			//grub_printf("     inpSize %X filepos %lX\n", lz4dec.inpSize, filepos);
			lz4dec.inpPos = 0;
//			unsigned char *pNextBlockSize = lz4dec.inp + blockSize + lz4dec.flg_bchecksum * 4;
			lz4dec.nextBlockSize = *(grub_u32_t*)(grub_size_t)(lz4dec.inp + blockSize + lz4dec.flg_bchecksum * 4);

			/* If dic is full, move 64K to beginning. */
			if (lz4dec.dicPos + lz4dec.blockMaxSize > LZ4_DICBUFSIZE)
			{
				unsigned int dicPosSrc = lz4dec.dicPos - 65536;
				memmove(lz4dec.dic, lz4dec.dic + dicPosSrc, 65536);
				lz4dec.dicPos = 65536;
				lz4dec.dicFilePos += dicPosSrc;
				outSkip -= dicPosSrc;
			}

			/* Decode 1 block */
			if (bUncompressedBlock) {
				memmove(lz4dec.dic + lz4dec.dicPos, lz4dec.inp, blockSize);
				lz4dec.dicPos += blockSize;
			}
			else {
				/* Decompress LZ4 Block format*/
				unsigned char *q = lz4dec.dic + lz4dec.dicPos;
				unsigned char *p = lz4dec.inp;
				unsigned int outRem = LZ4_DICBUFSIZE - lz4dec.dicPos;
				unsigned int inpRem = blockSize;
				while (1) {
					/* grub_printf("decompressing %X %X %X\n", q, p, inpRem); */
					if (!inpRem) {
						errnum = ERR_BAD_GZIP_DATA;
						break;
					}
					/* read token */
					unsigned char token = *(p++); --inpRem;
					/* read more literal length */
					unsigned int litlen = token >> 4;
					if (litlen == 15 && inpRem) {
						unsigned char c;
						do {
							c = *(p++);
							litlen += c;
							--inpRem;
						} while (inpRem && c == 255);
					}
					/* grub_printf("litlen %X\n", litlen); */
					/* copy literal */
					if (inpRem < litlen || outRem < litlen) {
						/* error */
						/* grub_printf("inpRem %X < litlen %X\n", inpRem, litlen); */
						errnum = ERR_BAD_GZIP_DATA;
						break;
					}
					else {
						inpRem -= litlen;
						outRem -= litlen;
						unsigned int counter;
						for (counter = litlen; counter; --counter)
							*(q++) = *(p++);
						if (inpRem == 0) {
							break; /* end of compressed block */
						}
					}
					/* read match offset */
					unsigned int matoff = (((unsigned int)p[0]) + ((unsigned int)p[1] << 8)); p += 2; inpRem -= 2;
					/* read more match size */
					unsigned int matlen = (token & 15);
					if (matlen == 15) {
						unsigned char c;
						do {
							c = *(p++);
							matlen += c;
							--inpRem;
						} while (inpRem && c == 255);
					}
					matlen += 4;
					/* grub_printf("matlen %X\n", matlen); */
					/* copy match */
					if (outRem < matlen) {
						/* error */
						errnum = ERR_BAD_GZIP_DATA;
						break;
					}
					else {
						outRem -= matlen;
						unsigned char *src = q - matoff;
						unsigned int counter;
						for (counter = matlen; counter; --counter)
							*(q++) = *(src++);
					}
				}
				/* grub_printf("dic %X dic+dicPos %X dic+dicPos+blockSize %X q %X q-dic %X blockSize %X\n",
				lz4dec.dic, lz4dec.dic + lz4dec.dicPos, lz4dec.dic + lz4dec.dicPos + blockSize,
				q, q-lz4dec.dic, blockSize ); */
				lz4dec.dicPos = q - lz4dec.dic;
			}
		}
	}
	compressed_file = 1;

	lz4dec.cfilemax = filemax;
	lz4dec.cfilepos = filepos;
	filemax = lz4dec.ufilemax;
	filepos = lz4dec.ufilepos;

	/* Now filemax, filepos are of uncompressed data
	* cfilemax, cfilepos are of compressed file
	* ufilemax, ufilepos are not used
	*/

	return outTx;
}

#endif /* ! NO_DECOMPRESSION */
