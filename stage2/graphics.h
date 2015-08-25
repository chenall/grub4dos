/* graphics.h - graphics console interface */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002  Free Software Foundation, Inc.
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

#ifndef GRAPHICS_H
#define GRAPHICS_H
#define SPLASH_BASE_ADDR (splashimage_loaded & ~0xfL)
#define SPLASH_W (*(unsigned long *)SPLASH_BASE_ADDR)
#define SPLASH_H (*(unsigned long *)(SPLASH_BASE_ADDR+4))
#define SPLASH_IMAGE ((unsigned long*)(SPLASH_BASE_ADDR+0x10))
#endif /* GRAPHICS_H */

#define JPG_FILE						0x3A0000
#define JPG_FILE_LENGTH			0x8000
#define	IMAGE_BUFFER				0x1000000
#define	IMAGE_BUFFER_LENGTH	0x753000		// 1600*1200*4
#define MAKEWORD(a, b) (((unsigned long)((unsigned char)(a) & 0xff)) | ((unsigned long)(((unsigned char)(b) & 0xff) << 8)))
#define HEX    0
#define BIN    1
#define HORIZ  0
#define VERTI  1
#define H_TO_L 0
#define L_TO_H 1

#define M_SOF0  (unsigned char)0xc0
#define M_SOF2  (unsigned char)0xc2
#define M_DHT   (unsigned char)0xc4
#define M_EOI   (unsigned char)0xd9
#define M_SOS   (unsigned char)0xda
#define M_DQT   (unsigned char)0xdb
#define M_DRI   (unsigned char)0xdd
#define M_APP0  (unsigned char)0xe0

#define W1 2841 /* 2048*sqrt(2)*cos(1*pi/16) */
#define W2 2676 /* 2048*sqrt(2)*cos(2*pi/16) */
#define W3 2408 /* 2048*sqrt(2)*cos(3*pi/16) */
#define W5 1609 /* 2048*sqrt(2)*cos(5*pi/16) */
#define W6 1108 /* 2048*sqrt(2)*cos(6*pi/16) */
#define W7 565  /* 2048*sqrt(2)*cos(7*pi/16) */

static short Zig_Zag[8][8]={{0,1,5,6,14,15,27,28},
						  {2,4,7,13,16,26,29,42},
						  {3,8,12,17,25,30,41,43},
						  {9,11,18,24,37,40,44,53},
						  {10,19,23,32,39,45,52,54},
						  {20,22,33,38,46,51,55,60},
						  {21,34,37,47,50,56,59,61},
						  {35,36,48,49,57,58,62,63}
						 };
