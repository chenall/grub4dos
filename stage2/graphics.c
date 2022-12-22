/* graphics.c - graphics mode support for GRUB */
/* Implemented as a terminal type by Jeremy Katz <katzj@redhat.com> based
 * on a patch by Paulo C閟ar Pereira de Andrade <pcpa@conectiva.com.br>
 */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2001,2002  Red Hat, Inc.
 *  Portions copyright (C) 2000  Conectiva, Inc.
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



#ifdef SUPPORT_GRAPHICS

#include <term.h>
#include <shared.h>
#include <graphics.h>

//static int saved_videomode = 0;
extern unsigned char *font8x16;

int outline = 0;
extern unsigned long is_highlight;
extern unsigned long graphics_inited;
char splashimage[128];

#define VSHADOW VSHADOW1
#if 0
/* 8x16 dot array, total chars = 80*30. plano size = 80*30*16 = 38400 bytes */
/* 8x16 dot array, total chars = 100*37. plano size = 800*600/8 = 60000 bytes */
static unsigned char *VSHADOW1 = (unsigned char *)0x3A0000;	//unsigned char VSHADOW1[60000];
static unsigned char *VSHADOW2 = (unsigned char *)0x3AEA60;	//unsigned char VSHADOW2[60000];
static unsigned char *VSHADOW4 = (unsigned char *)0x3BD4C0;	//unsigned char VSHADOW4[60000];
static unsigned char *VSHADOW8 = (unsigned char *)0x3CBF20;	//unsigned char VSHADOW8[60000]; end at 0x3DA980
/* text buffer has to be kept around so that we can write things as we
 * scroll and the like */
//static unsigned short text[80 * 30];
static unsigned long *text = (unsigned long *)0x3FC000; // length in bytes = 100*37*4 = 0x39D0
#endif
//extern unsigned long splashimage_loaded;

/* constants to define the viewable area */
unsigned long x1 = 80;
unsigned long y1 = 30;
unsigned long font_w = 8;
unsigned long font_h = 16;
unsigned char num_wide = (16+7)/8;
unsigned long font_spacing = 0;
unsigned long line_spacing = 0;
unsigned long xpixels = 640;
unsigned long ypixels = 480;
unsigned long plano_size = 38400;
unsigned long graphics_mode = 3;
//unsigned long current_x_resolution;
unsigned long current_y_resolution;
unsigned long current_bits_per_pixel;
unsigned long current_bytes_per_scanline;
unsigned long current_bytes_per_pixel;
unsigned long current_phys_base;
unsigned long image_pal[16];

/* why do these have to be kept here? */
unsigned long foreground = 0xFFFFFF; //(63 << 16) | (63 << 8) | (63)
unsigned long background = 0;

/* global state so that we don't try to recursively scroll or cursor */
//static int no_scroll = 0;

/* graphics local functions */
static void graphics_scroll (void);
void SetPixel (unsigned long x, unsigned long y, unsigned long color);
void XorPixel (unsigned long x, unsigned long y, unsigned long color);
static int read_image (void);
//static void graphics_cursor (int set);
static void vbe_cursor (int set);
void rectangle(int left, int top, int length, int width, int line);
extern void (*graphics_CURSOR) (int set);
unsigned long pixel_shift(unsigned long color);
/* FIXME: where do these really belong? */
static inline void outb(unsigned short port, unsigned char val)
{
    __asm __volatile ("outb %0,%1"::"a" (val), "d" (port));
}
#if 0
static void MapMask(int value) {
    outb(0x3c4, 2);
    outb(0x3c5, value);
}

/* bit mask register */
static void BitMask(int value) {
    outb(0x3ce, 8);
    outb(0x3cf, value);
}
#endif
extern void memmove_forward_SSE (void *dst, const void *src, unsigned int len);

#if 0
#if 1
/* memmove using SSE */
static void _memcpy_forward (void *dst, const void *src, unsigned int len)
{
#if 0
  asm ("  movl	%cr0, %eax");
  asm ("  orb	$2, %al");			// set CR0.MP
  asm ("  movl	%eax, %cr0");
  asm ("  movl	%cr4, %eax");
  asm ("  orb	$0x6, %ah");// set CR4.OSFXSR (bit 9) OSXMMEXCPT (bit 10)
  asm ("  movl	%eax, %cr4");
#endif

#if 1
  /* this piece of code must exist! or else gcc will fail to run the asm
   * code that immediately follows. So do not comment out this C code! */
  if (((int)src | (int)dst) & 0xF)
  {
	fontx = fonty = 0;
	printf ("Unaligned!\n");
	return;
  }
#endif
  asm ("  pushl %esi");
  asm ("  pushl %edi");
  asm ("  cld");
  asm ("  movl	%0, %%edi" : :"m"(dst));
  asm ("  movl	%0, %%esi" : :"m"(src));
  asm ("  movl	%0, %%ecx" : :"m"(len));
  asm ("  shrl	$7, %ecx");	// ECX = len / (16 * 8)
  asm ("1:");
#if 1
  asm ("  movdqa	(%esi), %xmm0");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntps	%xmm0, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	(%esi), %xmm1");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntps	%xmm1, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	(%esi), %xmm2");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntps	%xmm2, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	(%esi), %xmm3");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntps	%xmm3, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	(%esi), %xmm4");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntps	%xmm4, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	(%esi), %xmm5");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntps	%xmm5, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	(%esi), %xmm6");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntps	%xmm6, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	(%esi), %xmm7");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntps	%xmm7, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
#else
#if 1
  asm ("  movdqa	(%esi), %xmm0");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movdqa	(%esi), %xmm1");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movdqa	(%esi), %xmm2");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movdqa	(%esi), %xmm3");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movdqa	(%esi), %xmm4");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movdqa	(%esi), %xmm5");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movdqa	(%esi), %xmm6");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movdqa	(%esi), %xmm7");	// works on PIII and up
  asm ("  addl	$16, %esi");
#else
  asm ("  movntdqa	(%esi), %xmm0");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntdqa	(%esi), %xmm1");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntdqa	(%esi), %xmm2");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntdqa	(%esi), %xmm3");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntdqa	(%esi), %xmm4");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntdqa	(%esi), %xmm5");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntdqa	(%esi), %xmm6");	// works on PIII and up
  asm ("  addl	$16, %esi");
  asm ("  movntdqa	(%esi), %xmm7");	// works on PIII and up
  asm ("  addl	$16, %esi");
#endif
#if 0
  asm ("  movdqa	%xmm0, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	%xmm1, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	%xmm2, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	%xmm3, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	%xmm4, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	%xmm5, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	%xmm6, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movdqa	%xmm7, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
#else
  asm ("  movntps	%xmm0, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movntps	%xmm1, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movntps	%xmm2, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movntps	%xmm3, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movntps	%xmm4, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movntps	%xmm5, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movntps	%xmm6, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
  asm ("  movntps	%xmm7, (%edi)");	// works on PIII and up
  asm ("  addl	$16, %edi");
#endif
#endif
  asm ("  loop	1b");
  asm ("  popl %edi");
  asm ("  popl %esi");
}

#else

static inline void * _memcpy_forward(void *dst, const void *src, unsigned int len)
{
    int r0, r1, r2, r3;
    __asm__ __volatile__(
	"movl %%ecx, %0; shrl $2, %%ecx; "	// ECX=(len / 4)
	"rep; movsl; "
	"movl %0, %%ecx; andl $3, %%ecx; "	// ECX=(len % 4)
	"rep; movsb; "
	: "=&r"(r0), "=&c"(r1), "=&D"(r2), "=&S"(r3)
	: "1"(len), "2"((long)dst), "3"((long)src)
	: "memory");
    return dst;
}
#endif
#endif
#if 0
static inline void _memset(void *dst, unsigned char data, unsigned int len)
{
    int r0,r1,r2,r3;
    __asm__ __volatile__ (
	"movb %b2, %h2; movzwl %w2, %3; shll $16, %2; orl %3, %2; "	// duplicate data into all 4-bytes of EAX
	"movl %0, %3; shrl $2, %0; "	// ECX=(len / 4)
	"rep; stosl;  "
	"movl %3, %0; andl $3, %0; "	// ECX=(len % 4)
	"rep; stosb;  "
	:"=&c"(r1),"=&D"(r2),"=&a"(r0),"=&r"(r3)
	:"0"(len),"1"(dst),"2"(data)
	:"memory");
}
#endif
void SetPixel (unsigned long x, unsigned long y, unsigned long color)
{
	unsigned char *lfb;

	if (x < 0 || y < 0 || x >= current_x_resolution || y >= current_y_resolution)
		return;

	lfb = (unsigned char *)(current_phys_base + (y * current_bytes_per_scanline) + (x * current_bytes_per_pixel));
	switch (current_bits_per_pixel)
	{
	case 24:
		*(unsigned short *)lfb = (unsigned short)color;
		lfb[2] = (unsigned char)(color >> 16);
		break;
	case 32:
		*(unsigned long *)lfb = (unsigned long)color;
		break;
	default:
		*(unsigned short *)lfb = (unsigned short)color;
		break;
	}
}

void XorPixel (unsigned long x, unsigned long y, unsigned long color)
{
	unsigned char *lfb;

	if (x < 0 || y < 0 || x >= current_x_resolution || y >= current_y_resolution)
		return;

	lfb = (unsigned char *)(current_phys_base + (y * current_bytes_per_scanline) + (x * current_bytes_per_pixel));
	switch (current_bits_per_pixel)
	{
	case 24:
		*(unsigned short *)lfb ^= (unsigned short)color;
		lfb[2] ^= (unsigned char)(color >> 16);
		break;
	case 32:
		*(unsigned long *)lfb ^= (unsigned long)color;
		break;
	default:
		*(unsigned short *)lfb ^= (unsigned short)color;
		break;
	}
}

/* Initialize a vga16 graphics display with the palette based off of
 * the image in splashimage.  If the image doesn't exist, leave graphics
 * mode.  */
int
graphics_init (void)
{
//    if (! graphics_CURSOR)
//	  graphics_CURSOR = (void *)&graphics_cursor;
	
    if (! graphics_inited)
    {
	/* get font info before seting mode! some buggy BIOSes destroyed the
	 * red planar of the VGA on geting font info call. So we should set
	 * mode only after the get font info call.
	 */
	//if (! font8x16)
	//    font8x16 = graphics_get_font ();

	if (graphics_mode > 0xFF) /* VBE */
	{
	    if (graphics_mode == 0x102)
	    {
//		if (set_vbe_mode (graphics_mode) != 0x004F)
//		{
			graphics_end ();
			return !(errnum = ERR_SET_VBE_MODE);
//		}
//		graphics_mode = 0x6A;
//		goto success;
	    }
	    if (set_vbe_mode (graphics_mode | (1 << 14)) != 0x004F)
	    {
		graphics_end ();
		return !(errnum = ERR_SET_VBE_MODE);
	    }

	    current_term->chars_per_line = x1 = current_x_resolution / (font_w + font_spacing);
	    current_term->max_lines = y1 = current_y_resolution / (font_h + line_spacing);

	    /* here should read splashimage. */
	    graphics_CURSOR = (void *)&vbe_cursor;
	    //graphics_inited = 1;
	    //return 1;
	}
#if 0
	else
	{
	    unsigned long tmp_mode;
	    //saved_videomode = set_videomode (graphics_mode);
	    /* the mode set could fail !! */
	    if (graphics_mode == 0x12)
	    {
		if (set_videomode (graphics_mode) != graphics_mode)
		{
			graphics_end ();
			return !(errnum = ERR_SET_VGA_MODE);
		}
		current_term->chars_per_line = x1 = 80;
		current_term->max_lines = y1 = 30;
		xpixels = 640;
		ypixels = 480;
		plano_size = (640 * 480) / 8;
	    }
	    else /* 800x600x4 */
	    {
		/* first, try VBE mode 0x102 anyway */
		if (set_vbe_mode (0x102) == 0x004F)
			goto success;
		if (set_videomode (graphics_mode) != graphics_mode
			|| (*(unsigned char *)(0x8000 + 64) != 0x1B)
			|| (*(unsigned char *)(0x8000 + 4) != graphics_mode)
			|| (*(unsigned short *)(0x8000 + 5) != 100)
			/* qemu bug: byte at 0x22 should be 37, but only 36 */
			|| (((*(unsigned char *)(0x8000 + 0x22)) & 0xFE) != 36)
			|| (*(unsigned short *)(0x8000 + 0x23) != 16)
			|| (*(unsigned short *)(0x8000 + 0x27) != 16)
			/* qemu bug: byte at 0x29 should be 1, but was 8 */
			/*|| (*(unsigned char *)(0x8000 + 0x29) != 1)*/
			)
		{
		  /* probe 800x600x4 modes */
		  for (tmp_mode = 0x15; tmp_mode < 0x78; tmp_mode++)
		  {
		    if (set_videomode (tmp_mode) == tmp_mode
			&& (*(unsigned char *)(0x8000 + 64) == 0x1B)
			&& (*(unsigned char *)(0x8000 + 4) == tmp_mode)
			&& (*(unsigned short *)(0x8000 + 5) == 100)
			/* qemu bug: byte at 0x22 should be 37, but only 36 */
			&& (((*(unsigned char *)(0x8000 + 0x22)) & 0xFE) == 36)
			&& (*(unsigned short *)(0x8000 + 0x23) == 16)
			&& (*(unsigned short *)(0x8000 + 0x27) == 16)
			/* qemu bug: byte at 0x29 should be 1, but was 8 */
			/*&& (*(unsigned char *)(0x8000 + 0x29) == 1)*/
			)
		    {
			/* got it! */
			graphics_mode = tmp_mode;
			goto success;
		    }
		  }
		  graphics_end ();	/* failure, go back to console. */
		  return !(errnum = ERR_SET_VGA_MODE);
		}
success:
		current_term->chars_per_line = x1 = 100;
		current_term->max_lines = y1 = 37;
		xpixels = 800;
		ypixels = 600;
		plano_size = (800 * 600) / 8;
	    }
	}
#endif
	else
		return !(errnum = ERR_SET_VBE_MODE);
	menu_border.disp_ul = 0x14;
	menu_border.disp_ur = 0x15;
	menu_border.disp_ll = 0x16;
	menu_border.disp_lr = 0x13;
	menu_border.disp_horiz = 0x0F;
	menu_border.disp_vert = 0x0E;
    }

  if (! fill_color)
  {
    if (! read_image ())
    {
	//set_videomode (3/*saved_videomode*/);
//	graphics_end ();
	return !(errnum = ERR_LOAD_SPLASHIMAGE);
    }
  }
  else
  {
    *splashimage = 1;
    splashimage_loaded = IMAGE_BUFFER;
    splashimage_loaded |= 2;
  }

	current_term = term_table + 1;	/* terminal graphics */
    fontx = fonty = 0;
    graphics_inited = graphics_mode;

    return 1;
}

/* Leave graphics mode */
void
graphics_end (void)
{
	current_term = term_table; /* set terminal to console */
	set_videomode (3); /* set normal 80x25 text mode. */
	set_videomode (3); /* set it once more for buggy BIOSes. */

	menu_border.disp_ul = 218;
	menu_border.disp_ur = 191;
	menu_border.disp_ll = 192;
	menu_border.disp_lr = 217;
	menu_border.disp_horiz = 196;
	menu_border.disp_vert = 179;

	graphics_CURSOR = 0;
	fontx = fonty = 0;
	graphics_inited = 0;
}

static unsigned long pending = 0;
static unsigned long byte_SN;
static unsigned long unicode;
static unsigned long invalid;

static void
check_scroll (void)
{
    ++fonty;
    if (fonty >= current_term->max_lines)
    {
	--fonty;
	graphics_scroll();
    }
    if (count_lines >= 0)
		++count_lines;
}

unsigned long ged_unifont_simp (unsigned long unicode);
static unsigned long
print_unicode (unsigned long max_width)
{
    unsigned long i, j/*, k*/;
//    unsigned long pat;
    unsigned long char_width;
    unsigned long bgcolor;
    unsigned long CursorX,CursorY;
	unsigned char *lfb, *pat, *p;
	unsigned char column;
//	unsigned char tem;
	unsigned long long dot_matrix;
//	unsigned char buf[64*64/8];
	
	CursorX = fontx * (font_w + font_spacing);
	CursorY = fonty * (font_h + line_spacing);

	//print triangle
	if (unicode==0x10 || unicode==0x11)
	{
		lfb = (unsigned char *)(current_phys_base + CursorY*current_bytes_per_scanline + CursorX*current_bytes_per_pixel);
		for (i=0;i<font_w;++i)
		{
			if (unicode==0x10)
				p = lfb + i*current_bytes_per_pixel + ((font_w>>1)+(i>>1))*current_bytes_per_scanline;
			else
				p = lfb + (font_w-i-1)*current_bytes_per_pixel + ((font_w>>1)+(i>>1))*current_bytes_per_scanline;

			for (j=0;j<font_w-i;++j)
			{
				if (current_bytes_per_pixel == 3)
				{
					*(unsigned short *)p = (unsigned short)current_color_64bit;
					*(p+2) = (unsigned char)(current_color_64bit>>16);
				}
				else if(current_bytes_per_pixel == 4)
					*(unsigned long *)p = (unsigned long)current_color_64bit;
				else
					*(unsigned short *)p = (unsigned short)pixel_shift((unsigned long)current_color_64bit);
				
				p += current_bytes_per_scanline;
			}
		}
		char_width = 1;
		goto triangle;
	}

	if (unifont_simp_on)
		unicode = ged_unifont_simp (unicode);
    char_width = 2;				/* wide char */
//    pat = UNIFONT_START + (unicode << 5);
		pat = (unsigned char *)UNIFONT_START + unicode*num_wide*font_h;

//    if (*(unsigned long *)pat == narrow_char_indicator || unicode < 0x80)
//	{ --char_width; pat += 16; }		/* narrow char */
#if 0
	if (font_type==BIN && scan_mode==HORIZ)
	{
		p = pat;
		//HORIZ to VERTI
		for (i = 0; i < font_h; i++)
		{
			dot_matrix = 0;
			for (j=0; j<font_h; j++)
			{
				unsigned long long t = 0;
				t = p[(i>>3)+(j*num_wide)];
				if (store_mode==H_TO_L)
					dot_matrix |= ((t >> ((8*num_wide-1-i) & 7)) & 1) << j;
				else	//store_mode==L_TO_H
					dot_matrix |= ((t >> (i & 7)) & 1) << j;	
			}
			for (j=0; j<num_wide; j++)
				buf[i*num_wide +j] = (dot_matrix >> j*8)&0xff;
		}
		pat = buf;
	}

	p = pat;
	if (font_type)	//BIN,etc
	{
		p += num_wide*font_w;
	i=0;
	while (i<num_wide*font_w && *p==0)
	{
		i++;
		p++;
	}
	if (i==num_wide*font_w || unicode < 0x80)
		--char_width;
	}
	else if (*(unsigned long *)pat == narrow_char_indicator || unicode < 0x80)
#endif
		if (*(unsigned long *)pat == narrow_char_indicator || unicode < 0x80)
		{
			--char_width;
			pat += num_wide*font_w;
		}		/* narrow char */

    if (max_width < char_width)
	return (1 << 31) | invalid | (byte_SN << 8); // printed width = 0

    if (cursor_state & 1)
	graphics_CURSOR(0);

    /* print CRLF and scroll if needed */
    if (fontx + char_width > current_term->chars_per_line)
	{ fontx = 0; check_scroll (); }

	if (!(splashimage_loaded & 2) || !(cursor_state & 2) || (is_highlight && current_color_64bit >> 32) || (current_color_64bit & 0x1000000000000000))
		bgcolor = current_color_64bit >> 32 | 0x1000000;
	else
		bgcolor = 0;

//	CursorX = fontx * (font_w + font_spacing);
//	CursorY = fonty * (font_h + line_spacing);
	for (i = 0; i<char_width * (font_w+font_spacing);++i)
	{
		unsigned long tmp_x = CursorX + i;
		for (j = 0;j<font_h+line_spacing;++j)
		{
//			SetPixel (tmp_x, CursorY + j,bgcolor?bgcolor : SPLASH_IMAGE[tmp_x+(CursorY+j)*SPLASH_W]);
			lfb = (unsigned char *)SPLASH_IMAGE + tmp_x*current_bytes_per_pixel + (CursorY+j)*current_bytes_per_scanline;
			SetPixel (tmp_x, CursorY + j,bgcolor?bgcolor : *(unsigned long *)lfb);
		}
	}

	CursorX += font_spacing>>1;
	CursorY += line_spacing>>1;

#if 0
    /* print dot matrix of the unicode char */
    for (i = 0; i < char_width * font_w; ++i)
    {
	unsigned long tmp_x = CursorX + i;
	unsigned long column = ((unsigned short *)pat)[i];
	for (j = 0; j < font_h; ++j)
	{
	    /* print char using foreground and background colors. */
	    if ((column >> j) & 1)
			{
				if(current_bits_per_pixel == 24 || current_bits_per_pixel == 32)
			SetPixel (tmp_x, CursorY + j,current_color_64bit);
				else
					SetPixel (tmp_x, CursorY + j,pixel_shift(current_color_64bit));
			}
	}
    }
#endif

	/* print dot matrix of the unicode char */
	for (i = 0; i < char_width * font_w; ++i)
	{
		unsigned long tmp_x = CursorX + i;
		dot_matrix = 0;
		for(j = 0; j < num_wide; j++)
		{
			column = *(unsigned char *)pat;
			pat++;
#if 0
			if (font_type==BIN && scan_mode==VERTI && store_mode==H_TO_L)
			{
				tem = 0;
				for(k = 0; k < 8; k++)		//l to h
					tem |= ((column & (0x80 >> k))?(1<< k):0);
				column = tem;
			}
#endif
			dot_matrix |= (((unsigned long long)column) << j*8);
		}
		for (j = 0; j < font_h; ++j)
		{
	    /* print char using foreground and background colors. */
	    if ((dot_matrix >> j) & 1)
			{
				if(current_bits_per_pixel == 24 || current_bits_per_pixel == 32)
					SetPixel (tmp_x, CursorY + j,current_color_64bit);
				else
					SetPixel (tmp_x, CursorY + j,pixel_shift(current_color_64bit));
			}
		}
	}
//#endif
triangle:
    fontx += char_width;
    if (cursor_state & 1)
    {
	if (fontx >= current_term->chars_per_line)
	    { fontx = 0; check_scroll (); }
	graphics_CURSOR(1);
    }
    return invalid | (byte_SN << 8) | char_width;
}

static unsigned long
print_invalid_pending_bytes (unsigned long max_width)
{
    unsigned long tmpcode = unicode;
    unsigned long ret = 0;

    /* now pending is on, so byte_SN can tell the number of bytes, and
     * unicode can tell the value of each byte.
     */
    invalid = (1 << 15);	/* set the error bit for return */
    if (byte_SN == 2)	/* print 2 bytes */
    {
	unicode = (unicode >> 6) | 0xDCE0;	/* the 1st byte */
	ret = print_unicode (max_width);
	if ((long)ret < 0)
	    return ret;
	max_width -= (unsigned char)ret;
	unicode = (tmpcode & 0x3F) | 0xDC80;	/* the 2nd byte */
    }
    else // byte_SN == 1
	unicode |= 0xDCC0 | ((pending - 1) << 5);
    return print_unicode (max_width) + (unsigned char)ret;
}

/* Print ch on the screen.  Handle any needed scrolling or the like */
unsigned int
graphics_putchar (unsigned int ch, unsigned int max_width)
{
    unsigned long ret;

    if (fontx >= current_term->chars_per_line)
        { fontx = 0; check_scroll (); }

    if (graphics_mode <= 0xFF)
//	goto vga;
			return 0;

    /* VBE */

    invalid = 0;

    if ((unsigned char)ch > 0x7F)	/* multi-byte */
	goto multibyte;

    if (pending)	/* print (byte_SN) invalid bytes */
    {
	ret = print_invalid_pending_bytes (max_width);
	pending = 0;	/* end the utf8 byte sequence */
	if ((long)ret < 0)
	    return ret;
	max_width -= (unsigned char)ret;
    }

    if (! max_width)
	return (1 << 31);	/* printed width = 0 */

    if (cursor_state & 1)
	graphics_CURSOR(0);

    if ((char)ch == '\n')
    {
	check_scroll ();
	if (cursor_state & 1)
	    graphics_CURSOR(1);
	return 1;
    }
    if ((char)ch == '\r')
    {
	fontx = 0;
	if (cursor_state & 1)
	    graphics_CURSOR(1);
	return 1;
    }
    /* we know that the ASCII chars are all narrow */
    unicode = ch;
    return print_unicode (1);
multibyte:

    if (! (((unsigned char)ch) & 0x40))	/* continuation byte 10xxxxxx */
    {
	if (! pending)
	{
	    /* print this single invalid byte */
	    unicode = 0xDC00 | (unsigned char)ch; /* the invalid code point */
	    invalid = (1 << 15);	/* set the error bit for return */
	    byte_SN = 0;
	}
	else
	{
	    pending--; byte_SN++;
	    unicode = ((unicode << 6) | (ch & 0x3F));

	    if (pending) /* pending = 1, awaiting the last continuation byte. */
		return (byte_SN << 8); // succeed with printed width = 0
	}

	/* pending = 0, end the sequence, print the unicode char */
	return print_unicode (max_width);
    }
    if (! (((unsigned char)ch) & 0x20)) /* leading byte 110xxxxx */
    {
	if (pending)	/* print (byte_SN) invalid bytes */
	{
	    ret = print_invalid_pending_bytes (max_width);
	    if ((long)ret < 0)
		return ret;
	    max_width -= (unsigned char)ret;
	}
	pending = 1;	/* awaiting 1 continuation byte */
	byte_SN = 1;
	unicode = (ch & 0x1F);
	return (1 << 8); // succeed with printed width = 0
    }
    if (! (((unsigned char)ch) & 0x10)) /* leading byte 1110xxxx */
    {
	if (pending)	/* print (byte_SN) invalid bytes */
	{
	    ret = print_invalid_pending_bytes (max_width);
	    if ((long)ret < 0)
		return ret;
	    max_width -= (unsigned char)ret;
	}
	pending = 2;	/* awaiting 2 continuation bytes */
	byte_SN = 1;
	unicode = (ch & 0x0F);
	return (1 << 8); // succeed with printed width = 0
    }

    /* now the current byte is invalid */

    if (pending)	/* print (byte_SN) invalid bytes */
    {
	ret = print_invalid_pending_bytes (max_width);
	if ((long)ret < 0)
	    return ret;
	max_width -= (unsigned char)ret;
    }

    unicode = 0xDC00 | (unsigned char)ch; /* the invalid code point */
    invalid = (1 << 15);	/* set the error bit for return */
    pending = 0;	/* end the utf8 byte sequence */
    /* print the current invalid byte */
    return print_unicode (max_width);

//////////////////////////////////////////////////////////////////////////////
#if 0
vga:
    if ((char)ch == '\n')
    {
	if (cursor_state & 1)
	    graphics_CURSOR(0);
	check_scroll ();
	if (cursor_state & 1)
	    graphics_CURSOR(1);
	return 1;
    }
    if ((char)ch == '\r')
    {
	if (cursor_state & 1)
	    graphics_CURSOR(0);
	fontx = 0;
	if (cursor_state & 1)
	    graphics_CURSOR(1);
	return 1;
    }

    text[fonty * x1 + fontx] = (unsigned char)ch;

    graphics_CURSOR(0);

    fontx++;
    if (cursor_state & 1)
    {
	if (fontx >= x1)
	    { fontx = 0; check_scroll (); }
	graphics_CURSOR(1);
    }
    return 1;
#endif
}

/* get the current location of the cursor */
int
graphics_getxy(void)
{
    return (fonty << 8) | fontx;
}

void
graphics_gotoxy (int x, int y)
{
    if (cursor_state & 1)
	graphics_CURSOR(0);

    fontx = x;
    fonty = y;

    if (cursor_state & 1)
	graphics_CURSOR(1);
}

void
graphics_cls (void)
{
//    int i;
//    unsigned char *mem, *s1, *s2, *s4, *s8;
	unsigned char *mem,*s1;

    fontx = 0;
    fonty = 0;

    if (graphics_mode <= 0xFF)
//	goto vga;
			return;

    /* VBE */
    #if 0
    _memset ((char *)current_phys_base, 0, current_y_resolution * current_bytes_per_scanline);
    #else
	s1 = (unsigned char *)current_phys_base;
	unsigned long color = current_color_64bit >> 32;
	unsigned long y,x,z;
	unsigned char *lfb;
	z = current_bytes_per_pixel;
	
	for(y=0;y<current_y_resolution;++y)
	{
		mem = s1;
		for(x=0;x<current_x_resolution;++x)
		{
			if (graphics_mode > 0xff && (splashimage_loaded & 2) && (cursor_state & 2))
			{
//				*(unsigned long *)mem = SPLASH_IMAGE[x+y*SPLASH_W];
				lfb = (unsigned char *)SPLASH_IMAGE + x*current_bytes_per_pixel + y*current_bytes_per_scanline;
				if(current_bits_per_pixel == 32)
					*(unsigned long *)mem = *(unsigned long *)lfb;
				else if(current_bits_per_pixel == 24)
				{
					*(unsigned short *)mem = *(unsigned short *)lfb;
					*(mem+2) = *(lfb+2);
				}
				else
					*(unsigned short *)mem = *(unsigned short *)lfb;
			}
			else
			{
				if(current_bits_per_pixel == 32)
					*(unsigned long *)mem = color;
				else if(current_bits_per_pixel == 24)
				{
					*(unsigned short *)mem = (unsigned short)color;
					*(mem+2) = (unsigned char)(color>>16);
				}
				else
					*(unsigned short *)mem = (unsigned short)pixel_shift(color);
			}
			mem += z;
		}
		s1 += current_bytes_per_scanline;
	}
    #endif
    if (cursor_state & 1)
	    graphics_CURSOR(1);
    return;
#if 0
vga:
    mem = (unsigned char*)VIDEOMEM;

    for (i = 0; i < x1 * y1; i++)
        text[i] = ' ';

    if (!(cursor_state & 2))
    {
	if (splashimage_loaded & 1)
	{
		MapMask(15);
		_memset (mem, 0, plano_size);
	}
	else
	{
		for(i=1;i<16;i<<=1)
		{
			MapMask(i);
			_memset (mem,(current_color & (i<<4))?0xff:0, plano_size);
		}
	}
	return;
    }

    s1 = (unsigned char*)VSHADOW1;
    s2 = (unsigned char*)VSHADOW2;
    s4 = (unsigned char*)VSHADOW4;
    s8 = (unsigned char*)VSHADOW8;

    BitMask(0xff);

    /* plano 1 */
    MapMask(1);
    memmove_forward_SSE (mem, s1, plano_size);

    /* plano 2 */
    MapMask(2);
    memmove_forward_SSE (mem, s2, plano_size);

    /* plano 3 */
    MapMask(4);
    memmove_forward_SSE (mem, s4, plano_size);

    /* plano 4 */
    MapMask(8);
    memmove_forward_SSE (mem, s8, plano_size);

    MapMask(15);
#endif
}

void clear_entry (int x, int y, int w, int h);
void
clear_entry (int x, int y, int w, int h)
{
	int i;
	unsigned char *source;
	unsigned char *objective;

	for (i=0;i<h;i++)
	{
		source = (unsigned char *)IMAGE_BUFFER + x*current_bytes_per_pixel + y*current_bytes_per_scanline + i*current_bytes_per_scanline;
		objective = (unsigned char *)current_phys_base + x*current_bytes_per_pixel + y*current_bytes_per_scanline + i*current_bytes_per_scanline;
		grub_memmove (objective,source,w*current_bytes_per_pixel);
	}
}

void vbe_fill_color (unsigned long color);

void
vbe_fill_color (unsigned long color)
{
  int i;
  unsigned char *p;
  
	for (i=0;i<(current_x_resolution*current_y_resolution);i++)
	{
		p = (unsigned char *)IMAGE_BUFFER + 16 + i*current_bytes_per_pixel;
		switch (current_bits_per_pixel)
		{
			case 32:
				*(unsigned long *)p = color;
				break;
			case 24:
				*(unsigned short *)p = (unsigned short)(color);
				*(p+2) = (unsigned char)(color>>16);
				break;
			default:
				*(unsigned short *)p = (unsigned short)pixel_shift(color);
				break;
		}
	}
}


int animated (void);
int use_phys_base=0;
unsigned long delay0, delay1, name_len;
char num;
unsigned char animated_enable;
unsigned char animated_enable_backup;

int animated (void)
{
	char tmp[128];
	unsigned long long val;
	char *p;
	int i, j;

  if (animated_delay)
  {
		delay0 = animated_delay;
		animated_delay=0;
    name_len=grub_strlen(animated_name);
		if ((num=animated_type & 0x0f) || (animated_type & 0x20))
		{
			if (!(splashimage_loaded & 2) || !(cursor_state & 2))
			{
				setcursor (2);
				cls ();
			}
		}
    if (!(animated_type & 0x20))
    {
      delay1 = 1;
      animated_enable = 1;
      animated_enable_backup = 1;
      return 1;
    }
	} 
	while (animated_type)
	{
    if (!(animated_type & 0x20))
    {
      if (--delay1)
        return 1;
    }

    for (i=0; i<animated_last_num; i++)
    {
			if (((animated_type & 0x0f) && !num) || (animated_type & 0x20 && (console_checkkey () != -1) && console_getkey ()))
			{
        animated_enable = 0;
        animated_type = 0;
        return 0x3c00;
			}

      use_phys_base=1;
      sprintf(tmp,"--offset=%d=%d=%d %s",(animated_type & 0x80),animated_offset_x,animated_offset_y,animated_name);
      splashimage_func(tmp,1);
      use_phys_base=0;

      p = &animated_name[name_len-5];
      while(*p>=0x30 && *p<=0x39) p--;
      j = &animated_name[name_len-5] - p++;
      safe_parse_maxint (&p, &val);
      if ((unsigned char)val == animated_last_num)
      {
				if ((animated_type & 0x0f))
					num--;
        val = 1;
      }
      else
        val++;

      if (j==1)
        sprintf(tmp,"%d",val); 
      else if (j==2)
        sprintf(tmp,"%02d",val);
      else if (j==3)
        sprintf(tmp,"%03d",val);
      else
        sprintf(tmp,"%04d",val);
      grub_memmove(&animated_name[name_len-5-j+1], tmp, j);
      
      delay1 = delay0;
      if (!(animated_type & 0x20))
        return 1;
      else
        defer (delay1);
		}
  } 
	return 0;
}

unsigned char R0,G0,B0,R1,G1,B1;
int only;

static int read_image_bmp(int type)
{
	//struct { /* bmfh */ 
	//	unsigned short bfType;
	//	unsigned long bfSize; 
	//	unsigned long bfReserved1; 
	//	unsigned long bfOffBits;
	//	} __attribute__ ((packed)) bmfh;
	struct { /* bmih */ 
		unsigned long  biSize; 
		unsigned long  biWidth; 
		unsigned long  biHeight; 
		unsigned short biPlanes; 
		unsigned short biBitCount; 
		unsigned long  biCompression; 
		unsigned long  biSizeImage; 
		unsigned long  biXPelsPerMeter; 
		unsigned long  biYPelsPerMeter; 
		unsigned long  biClrUsed; 
		unsigned long  biClrImportant;
	} __attribute__ ((packed)) bmih;
	unsigned long bftmp,bfbit;
	int x,y;
	unsigned long source = 0;
	unsigned char R,G,B;
	only = 0;
	if (type == 0)
		return 0;
	filepos = 10;
	if (!grub_read((unsigned long long)(unsigned int)(char*)&bftmp,4, GRUB_READ) || ! grub_read((unsigned long long)(unsigned int)&bmih,sizeof(bmih),GRUB_READ) || bmih.biBitCount < 24)
	{
		//return !printf("Error:Read BMP Head\n");
		return !(errnum = ERR_EXEC_FORMAT);
	}
	filepos = bftmp;
	bfbit = bmih.biBitCount>>3;
	bftmp = 0;
	//bftmp = (bmih.biWidth*(bmih.biBitCount>>3)+3)&~3;
//	SPLASH_W = bmih.biWidth;
//	SPLASH_H = bmih.biHeight;
//	unsigned long *bmp = SPLASH_IMAGE;
	unsigned char *bmp;
//	if (debug > 0)
//		printf("Loading splashimage...\n");
	for(y=bmih.biHeight-1;y>=0;--y)
	{
//		bmp = SPLASH_IMAGE+y*SPLASH_W;
		for(x=0;x<bmih.biWidth;++x)
		{
			grub_read((unsigned long long)(unsigned int)(char*)&bftmp,bfbit, GRUB_READ);
      if (only==0)
      {
        only=1;
        B = (bftmp & 0xff0000)>>16;
        G = (bftmp & 0xff00)>>8;
        R = bftmp & 0xff;
        if (B <= 0xef)
          B1 = B+0x10;
        else
          B1 = 0xff;
            
        if (G <= 0xef)
          G1 = G+0x10;
        else
          G1 = 0xff;
            
        if (R <= 0xef)
          R1 = R+0x10;
        else
          R1 = 0xff;
            
        if (B >= 0x10)
          B0 = B-0x10;
        else
          B0 = 0;
            
        if (G >= 0x10)
          G0 = G-0x10;
        else
          G0 = 0;
            
        if (R >= 0x10)
          R0 = R-0x10;
        else
          R0 = 0;
      }
			if(y < bmih.biHeight && x < bmih.biWidth)
				if((y+Y_offset) < current_y_resolution && (x+X_offset) < current_x_resolution)
				{
					if (use_phys_base == 0)
						bmp = (unsigned char *)SPLASH_IMAGE+(x+X_offset)*current_bytes_per_pixel+(y+Y_offset)*current_bytes_per_scanline;
					else
						bmp = (unsigned char *)current_phys_base+(x+X_offset)*current_bytes_per_pixel+(y+Y_offset)*current_bytes_per_scanline;
					
          B = (bftmp & 0xff0000)>>16;
          G = (bftmp & 0xff00)>>8;
          R = bftmp & 0xff;
					if ((R>=R0) && (R<=R1) && (G>=G0) && (G<=G1) && (B>=B0) && (B<=B1))
					{
						if (background_transparent || (graphic_enable && (graphic_type & 0x80) && !(is_highlight && (graphic_type & 8))))
							source = *(unsigned long *)((unsigned char *)SPLASH_IMAGE+(x+X_offset)*current_bytes_per_pixel+(y+Y_offset)*current_bytes_per_scanline);
						else if (graphic_enable && is_highlight && (graphic_type & 8))
							source = current_color_64bit & 0xffffffff;
						else if (!graphic_enable || (graphic_enable && !(graphic_type & 0x80) && !(is_highlight && (graphic_type & 8))))
							source = bftmp;
					}
					else
					{
						if (graphic_enable && is_highlight && (graphic_type & 1))	
							source = current_color_64bit & 0xffffffff;
						else if (graphic_enable && is_highlight && (graphic_type & 2))
							source = bftmp ^ 0xffffffff;
						else
							source = bftmp;
					}
				
					if(current_bits_per_pixel == 32)
//				bmp[x] = bftmp;		//
						*(unsigned long *)bmp = source;
					else if(current_bits_per_pixel == 24)
					{
						*(unsigned short *)bmp = (unsigned short)source;
						*(bmp+2) = (unsigned char)(source>>16);
					}
					else
						*(unsigned short *)bmp = (unsigned short)pixel_shift(source);
				}
		}
		filepos += ((bmih.biWidth*bfbit&3)?(4-(bmih.biWidth*bfbit&3)):0);
	}
	background_transparent=0;
	return 2;
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Jpeg functions
static int read_image_jpg(int type);
static int InitTag();
static void InitTable();
static int Decode();
static int DecodeMCUBlock();
static int HufBlock(unsigned char dchufindex,unsigned char achufindex);
static int DecodeElement();
static void IQtIZzMCUComponent(short flag);
static void IQtIZzBlock(short *s ,int *d,short flag);
static void GetYUV(short flag);
static void StoreBuffer();
static unsigned char ReadByte();
static void Initialize_Fast_IDCT();
static void Fast_IDCT(int * block);
static void idctrow(int * blk);
static void idctcol(int * blk);
//////////////////////////////////////////////////
//variables used in jpeg function
short SampRate_Y_H,SampRate_Y_V;
short SampRate_U_H,SampRate_U_V;
short SampRate_V_H,SampRate_V_V;
short H_YtoU,V_YtoU,H_YtoV,V_YtoV;
short Y_in_MCU,U_in_MCU,V_in_MCU;
unsigned char *lp;
short qt_table[3][64];
short comp_num;
unsigned char comp_index[3];
unsigned char YDcIndex,YAcIndex,UVDcIndex,UVAcIndex;
unsigned char HufTabIndex;
short *YQtTable,*UQtTable,*VQtTable;
unsigned char And[9]={0,1,3,7,0xf,0x1f,0x3f,0x7f,0xff};
short code_pos_table[4][16],code_len_table[4][16];
unsigned short code_value_table[4][256];
unsigned short huf_max_value[4][16],huf_min_value[4][16];
short BitPos,CurByte;
short rrun,vvalue;
short MCUBuffer[10*64];
int QtZzMCUBuffer[10*64];
short BlockBuffer[64];
short ycoef,ucoef,vcoef;
int IntervalFlag;
short interval;
int Y[4*64],U[4*64],V[4*64];
unsigned long sizei,sizej;
short restart;
static long iclip[1024];
static long	*iclp;
unsigned long long size;
///////////////////////////////////////////////
static void GetYUV(short flag)
{
	short	H=SampRate_Y_H;
	short	VV=SampRate_Y_V;
	short	i,j,k,h;
	int *buf=Y;
	int *pQtZzMCU=QtZzMCUBuffer;
	switch(flag)
	{
	case 1:
		H=SampRate_U_H;
		VV=SampRate_U_V;
		buf=U;
		pQtZzMCU=QtZzMCUBuffer+Y_in_MCU*64;
		break;
	case 2:
		H=SampRate_V_H;
		VV=SampRate_V_V;
		buf=V;
		pQtZzMCU=QtZzMCUBuffer+(Y_in_MCU+U_in_MCU)*64;
		break;
	}
	for (i=0;i<VV;i++)
		for(j=0;j<H;j++)
			for(k=0;k<8;k++)
				for(h=0;h<8;h++)
					buf[(i*8+k)*SampRate_Y_H*8+j*8+h]=*pQtZzMCU++;
}
///////////////////////////////////////////////////////////////////////////////
static void StoreBuffer()
{
	short i,j;
	unsigned char *lfb;
	unsigned char R,G,B;
	int y,u,v,rr,gg,bb;
	unsigned long color;
	unsigned long source = 0;

	for(i=0;i<SampRate_Y_V*8;i++)
	{
		if((sizei+i)<SPLASH_H && (sizei+i+Y_offset)<current_y_resolution)
		{
			for(j=0;j<SampRate_Y_H*8;j++)
			{
				if((sizej+j)<SPLASH_W && (sizej+j+X_offset)<current_x_resolution)
				{
					y=Y[i*8*SampRate_Y_H+j];
					u=U[(i/V_YtoU)*8*SampRate_Y_H+j/H_YtoU];
					v=V[(i/V_YtoV)*8*SampRate_Y_H+j/H_YtoV];																						
					rr=((y<<8)+18*u+367*v)>>8;	
					gg=((y<<8)-159*u-220*v)>>8;
					bb=((y<<8)+411*u-29*v)>>8;
					R=(unsigned char)rr;
					G=(unsigned char)gg;
					B=(unsigned char)bb;
					if (rr&0xffffff00)
					{
						if (rr>255)
							R=255;
						else if (rr<0)
							R=0;
					}
					if (gg&0xffffff00)
					{
						if (gg>255)
							G=255;
						else if (gg<0)
							G=0;
					}
					if (bb&0xffffff00)
					{
						if (bb>255)
							B=255;
						else if (bb<0)
							B=0;
					}
					
					if (only==0)
					{
            only=1;
						if (B <= 0xef)
              B1 = B+0x10;
            else
              B1 = 0xff;
            
            if (G <= 0xef)
              G1 = G+0x10;
            else
              G1 = 0xff;
            
            if (R <= 0xef)
              R1 = R+0x10;
            else
              R1 = 0xff;
            
            if (B >= 0x10)
              B0 = B-0x10;
            else
              B0 = 0;
            
            if (G >= 0x10)
              G0 = G-0x10;
            else
              G0 = 0;
            
            if (R >= 0x10)
              R0 = R-0x10;
            else
              R0 = 0;
					}
						
					if (use_phys_base == 0)
						lfb = (unsigned char *)SPLASH_IMAGE + (sizei+i+Y_offset)*current_bytes_per_scanline + (sizej+j+X_offset)*current_bytes_per_pixel;
					else
						lfb = (unsigned char *)current_phys_base + (sizei+i+Y_offset)*current_bytes_per_scanline + (sizej+j+X_offset)*current_bytes_per_pixel;
					
					color = (((unsigned long)R)<<16) | (((unsigned long)G)<<8) | (unsigned long)B;
					
					if ((R>=R0) && (R<=R1) && (G>=G0) && (G<=G1) && (B>=B0) && (B<=B1))
					{
						if (background_transparent || (graphic_enable && (graphic_type & 0x80) && !(is_highlight && (graphic_type & 8))))
							source = *(unsigned long *)((unsigned char *)SPLASH_IMAGE + (sizei+i+Y_offset)*current_bytes_per_scanline + (sizej+j+X_offset)*current_bytes_per_pixel);
						else if (graphic_enable && is_highlight && (graphic_type & 8))
							source = current_color_64bit & 0xffffffff;
						else if (!graphic_enable || (graphic_enable && !(graphic_type & 0x80) && !(is_highlight && (graphic_type & 8))))
							source = color;
					}
					else
					{
						if (graphic_enable && is_highlight && (graphic_type & 1))
							source = current_color_64bit & 0xffffffff;
						else if (graphic_enable && is_highlight && (graphic_type & 2))
							source = color ^ 0xffffffff;
						else
							source = color;
					}
				
					if(current_bits_per_pixel == 32)
						*(unsigned long *)lfb = source;
					else if(current_bits_per_pixel == 24)
					{
						*(unsigned short *)lfb = (unsigned short)source;
						*(lfb+2) = (unsigned char)(source>>16);
					}
					else
						*(unsigned short *)lfb = (unsigned short)pixel_shift(source);
				}
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
static int DecodeMCUBlock()
{
	short *lpMCUBuffer;
	short i,j;
	int funcret;

	if (IntervalFlag)
	{
		lp+=2;
		size-=2;
		ycoef=ucoef=vcoef=0;
		BitPos=0;
		CurByte=0;
	}
	switch(comp_num)
	{
	case 3:
		lpMCUBuffer=MCUBuffer;
		for (i=0;i<SampRate_Y_H*SampRate_Y_V;i++)  //Y
		{
			funcret=HufBlock(YDcIndex,YAcIndex);
			if (funcret!=1)
				return funcret;
			BlockBuffer[0]=BlockBuffer[0]+ycoef;
			ycoef=BlockBuffer[0];
			for (j=0;j<64;j++)
				*lpMCUBuffer++=BlockBuffer[j];
		}
		for (i=0;i<SampRate_U_H*SampRate_U_V;i++)  //U
		{
			funcret=HufBlock(UVDcIndex,UVAcIndex);
			if (funcret!=1)
				return funcret;
			BlockBuffer[0]=BlockBuffer[0]+ucoef;
			ucoef=BlockBuffer[0];
			for (j=0;j<64;j++)
				*lpMCUBuffer++=BlockBuffer[j];
		}
		for (i=0;i<SampRate_V_H*SampRate_V_V;i++)  //V
		{
			funcret=HufBlock(UVDcIndex,UVAcIndex);
			if (funcret!=1)
				return funcret;
			BlockBuffer[0]=BlockBuffer[0]+vcoef;
			vcoef=BlockBuffer[0];
			for (j=0;j<64;j++)
				*lpMCUBuffer++=BlockBuffer[j];
		}
		break;
	case 1:
		lpMCUBuffer=MCUBuffer;
		funcret=HufBlock(YDcIndex,YAcIndex);
		if (funcret!=1)
			return funcret;
		BlockBuffer[0]=BlockBuffer[0]+ycoef;
		ycoef=BlockBuffer[0];
		for (j=0;j<64;j++)
			*lpMCUBuffer++=BlockBuffer[j];
		for (i=0;i<128;i++)
			*lpMCUBuffer++=0;
		break;
	default:
		return 0;
	}
	return 1;
}

static unsigned char ReadByte()
{
	unsigned char  i;
	unsigned long long len;

	i=*(lp++);
	size--;
	if(i==0xff)
	{
		lp++;
		size--;
	}
	BitPos=8;
	CurByte=i;
	
	if(size <= 16)
	{
		grub_memmove64((unsigned long long)(int)JPG_FILE,(unsigned long long)(int)lp,(unsigned long long)size);
		len=grub_read((unsigned long long)(unsigned int)(char*)JPG_FILE+size, 0x7e00, GRUB_READ);
		size+=len;
		lp=(unsigned char*)JPG_FILE;
	}
	return i;
}
///////////////////////////////////////////////////////////////////////

static int DecodeElement()
{
	int thiscode,tempcode;
	unsigned short temp,valueex;
	short codelen;
	unsigned char hufexbyte,runsize,tempsize,sign;
	unsigned char newbyte,lastbyte;

	if(BitPos >= 1)
	{
		BitPos--;
		thiscode=(unsigned char)CurByte>>BitPos;
		CurByte=CurByte&And[BitPos];
	}
	else
	{
		lastbyte=ReadByte();
		BitPos--;
		newbyte=CurByte&And[BitPos];
		thiscode=lastbyte>>7;
		CurByte=newbyte;
	}
	codelen=1;
	while ((thiscode<huf_min_value[HufTabIndex][codelen-1])||
		  (code_len_table[HufTabIndex][codelen-1]==0)||
		  (thiscode>huf_max_value[HufTabIndex][codelen-1]))
	{
		if(BitPos>=1)
		{
			BitPos--;
			tempcode=(unsigned char)CurByte>>BitPos;
			CurByte=CurByte&And[BitPos];
		}
		else
		{
			lastbyte=ReadByte();
			BitPos--;
			newbyte=CurByte&And[BitPos];
			tempcode=(unsigned char)lastbyte>>7;
			CurByte=newbyte;
		}
		thiscode=(thiscode<<1)+tempcode;
		codelen++;
		if(codelen>16)
			return 0;
	}  //while
	temp=thiscode-huf_min_value[HufTabIndex][codelen-1]+code_pos_table[HufTabIndex][codelen-1],HufTabIndex;
	hufexbyte=(unsigned char)code_value_table[HufTabIndex][temp];
	rrun=(short)(hufexbyte>>4);
	runsize=hufexbyte&0x0f;
	if(runsize==0)
	{
		vvalue=0;
		return 1;
	}
	tempsize=runsize;
	if(BitPos>=runsize)
	{
		BitPos-=runsize;
		valueex=(unsigned char)CurByte>>BitPos;
		CurByte=CurByte&And[BitPos];
	}
	else
	{
		valueex=CurByte;
		tempsize-=BitPos;
		while(tempsize>8)
		{
			lastbyte=ReadByte();
			valueex=(valueex<<8)+(unsigned char)lastbyte;
			tempsize-=8;
		}  //while
		lastbyte=ReadByte();
		BitPos-=tempsize;
		valueex=(valueex<<tempsize)+(lastbyte>>BitPos);
		CurByte=lastbyte&And[BitPos];
	}  //else
	sign=valueex>>(runsize-1);
	if(sign)
		vvalue=valueex;
	else
	{
		valueex=valueex^0xffff;
		temp=0xffff<<runsize;
		vvalue=-(short)(valueex^temp);
	}
	return 1;
}
//////////////////////////////////////////////////////////////////
static int HufBlock(unsigned char dchufindex,unsigned char achufindex)
{
	short count=0;
	short i;
	int funcret;

	//dc
	HufTabIndex=dchufindex;
	funcret=DecodeElement();
	if(funcret!=1)
		return funcret;

	BlockBuffer[count++]=vvalue;
	//ac
	HufTabIndex=achufindex;
	while (count<64)
	{
		funcret=DecodeElement();
		if(funcret!=1)
			return funcret;
		if ((rrun==0)&&(vvalue==0))
		{
			for (i=count;i<64;i++)
				BlockBuffer[i]=0;
			count=64;
		}
		else
		{
			for (i=0;i<rrun;i++)
				BlockBuffer[count++]=0;
			BlockBuffer[count++]=vvalue;
		}
	}
	return 1;
}
/////////////////////////////////////////////////////////////////////////////////////
static void IQtIZzMCUComponent(short flag)
{
	short H=SampRate_Y_H;
	short VV=SampRate_Y_V;
	short i,j;
	int *pQtZzMCUBuffer=QtZzMCUBuffer;
	short  *pMCUBuffer=MCUBuffer;

	switch(flag)
	{
	case 1:
		H=SampRate_U_H;
		VV=SampRate_U_V;
		pMCUBuffer=MCUBuffer+Y_in_MCU*64;
		pQtZzMCUBuffer=QtZzMCUBuffer+Y_in_MCU*64;
		break;
	case 2:
		H=SampRate_V_H;
		VV=SampRate_V_V;
		pMCUBuffer=MCUBuffer+(Y_in_MCU+U_in_MCU)*64;
		pQtZzMCUBuffer=QtZzMCUBuffer+(Y_in_MCU+U_in_MCU)*64;
		break;
	}
	for(i=0;i<VV;i++)
		for (j=0;j<H;j++)
			IQtIZzBlock(pMCUBuffer+(i*H+j)*64,pQtZzMCUBuffer+(i*H+j)*64,flag);
}
//////////////////////////////////////////////////////////////////////////////////////////
static void IQtIZzBlock(short  *s ,int * d,short flag)
{
	short i,j;
	short tag;
	short *pQt=YQtTable;
	int buffer2[8][8];
	int *buffer1;
	short offset=128;

	switch(flag)
	{
	case 1:
		pQt=UQtTable;
		offset=0;
		break;
	case 2:
		pQt=VQtTable;
		offset=0;
		break;
	}

	for(i=0;i<8;i++)
		for(j=0;j<8;j++)
		{
			tag=Zig_Zag[i][j];
			buffer2[i][j]=(int)s[tag]*(int)pQt[tag];
		}
	buffer1=(int *)buffer2;
	Fast_IDCT(buffer1);
	for(i=0;i<8;i++)
		for(j=0;j<8;j++)
			d[i*8+j]=buffer2[i][j]+offset;
}
///////////////////////////////////////////////////////////////////////
static void Fast_IDCT(int * block)
{
	short i;

	for (i=0; i<8; i++)
		idctrow(block+8*i);

	for (i=0; i<8; i++)
		idctcol(block+i);
}
///////////////////////////////////////////////////////////////////////
static void Initialize_Fast_IDCT()
{
	short i;

	iclp = iclip + 512;
	for(i=-512; i<512; i++)
		iclp[i] = (i<-256)?-256:((i>255)?255:i);
}
////////////////////////////////////////////////////////////////////////
static void idctrow(int * blk)
{
	int x0, x01, x2, x3, x4, x5, x6, x7, x8;
	//intcut
	if (!((x01 = blk[4]<<11) | (x2 = blk[6]) | (x3 = blk[2]) |
		(x4 = blk[1]) | (x5 = blk[7]) | (x6 = blk[5]) | (x7 = blk[3])))
	{
		blk[0]=blk[1]=blk[2]=blk[3]=blk[4]=blk[5]=blk[6]=blk[7]=blk[0]<<3;
		return;
	}
	x0 = (blk[0]<<11) + 128; // for proper rounding in the fourth stage
	//first stage
	x8 = W7*(x4+x5);
	x4 = x8 + (W1-W7)*x4;
	x5 = x8 - (W1+W7)*x5;
	x8 = W3*(x6+x7);
	x6 = x8 - (W3-W5)*x6;
	x7 = x8 - (W3+W5)*x7;
	//second stage
	x8 = x0 + x01;
	x0 -= x01;
	x01 = W6*(x3+x2);
	x2 = x01 - (W2+W6)*x2;
	x3 = x01 + (W2-W6)*x3;
	x01 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;
	//third stage
	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181*(x4+x5)+128)>>8;
	x4 = (181*(x4-x5)+128)>>8;
	//fourth stage
	blk[0] = (x7+x01)>>8;
	blk[1] = (x3+x2)>>8;
	blk[2] = (x0+x4)>>8;
	blk[3] = (x8+x6)>>8;
	blk[4] = (x8-x6)>>8;
	blk[5] = (x0-x4)>>8;
	blk[6] = (x3-x2)>>8;
	blk[7] = (x7-x01)>>8;
}
//////////////////////////////////////////////////////////////////////////////
static void idctcol(int * blk)
{
	int x0, x01, x2, x3, x4, x5, x6, x7, x8;
	//intcut
	if (!((x01 = (blk[8*4]<<8)) | (x2 = blk[8*6]) | (x3 = blk[8*2]) |
		(x4 = blk[8*1]) | (x5 = blk[8*7]) | (x6 = blk[8*5]) | (x7 = blk[8*3])))
	{
		blk[8*0]=blk[8*1]=blk[8*2]=blk[8*3]=blk[8*4]=blk[8*5]
			=blk[8*6]=blk[8*7]=iclp[(blk[8*0]+32)>>6];
		return;
	}
	x0 = (blk[8*0]<<8) + 8192;
	//first stage
	x8 = W7*(x4+x5) + 4;
	x4 = (x8+(W1-W7)*x4)>>3;
	x5 = (x8-(W1+W7)*x5)>>3;
	x8 = W3*(x6+x7) + 4;
	x6 = (x8-(W3-W5)*x6)>>3;
	x7 = (x8-(W3+W5)*x7)>>3;
	//second stage
	x8 = x0 + x01;
	x0 -= x01;
	x01 = W6*(x3+x2) + 4;
	x2 = (x01-(W2+W6)*x2)>>3;
	x3 = (x01+(W2-W6)*x3)>>3;
	x01 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;
	//third stage
	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181*(x4+x5)+128)>>8;
	x4 = (181*(x4-x5)+128)>>8;
	//fourth stage
	blk[8*0] = iclp[(x7+x01)>>14];
	blk[8*1] = iclp[(x3+x2)>>14];
	blk[8*2] = iclp[(x0+x4)>>14];
	blk[8*3] = iclp[(x8+x6)>>14];
	blk[8*4] = iclp[(x8-x6)>>14];
	blk[8*5] = iclp[(x0-x4)>>14];
	blk[8*6] = iclp[(x3-x2)>>14];
	blk[8*7] = iclp[(x7-x01)>>14];
}
//////////////////////////////////////////////////////////////////////////////
static int Decode()
{
	int funcret;
	size -= ((unsigned long)lp - (unsigned long)JPG_FILE);

	Y_in_MCU=SampRate_Y_H*SampRate_Y_V;		//2*2=4		2*1=2		1*1=1
	U_in_MCU=SampRate_U_H*SampRate_U_V;		//1:1=1
	V_in_MCU=SampRate_V_H*SampRate_V_V;		//1:1=1
	H_YtoU=SampRate_Y_H/SampRate_U_H;			//2/1=2						1/1=1
	V_YtoU=SampRate_Y_V/SampRate_U_V;			//2/1=2		1/1=1
	H_YtoV=SampRate_Y_H/SampRate_V_H;			//2/1=2						1/1=1
	V_YtoV=SampRate_Y_V/SampRate_V_V;			//2/1=2		1/1=1
	Initialize_Fast_IDCT();
	while((funcret=DecodeMCUBlock())==1)
	{
		interval++;
		if((restart)&&(interval % restart==0))
			 IntervalFlag=1;
		else
			IntervalFlag=0;
		IQtIZzMCUComponent(0);
		IQtIZzMCUComponent(1);
		IQtIZzMCUComponent(2);
		GetYUV(0);
		GetYUV(1);
		GetYUV(2);
		StoreBuffer();
		sizej+=SampRate_Y_H*8;
		if(sizej>=SPLASH_W)
		{
			sizej=0;
			sizei+=SampRate_Y_V*8;
		}
		if ((sizej==0)&&(sizei>=SPLASH_H))
			break;
	}
	return 1;
}
/////////////////////////////////////////////////////////////////////////////////////////
static void InitTable()
{
	short i,j;
	sizei=sizej=0;
	SPLASH_W=SPLASH_H=0;
	rrun=vvalue=0;
	BitPos=0;
	CurByte=0;
	IntervalFlag=0;
	restart=0;
	for(i=0;i<3;i++)
		for(j=0;j<64;j++)
			qt_table[i][j]=0;
	comp_num=0;
	HufTabIndex=0;
	for(i=0;i<3;i++)
		comp_index[i]=0;
	for(i=0;i<4;i++)
		for(j=0;j<16;j++)
		{
			code_len_table[i][j]=0;
			code_pos_table[i][j]=0;
			huf_max_value[i][j]=0;
			huf_min_value[i][j]=0;
		}
	for(i=0;i<4;i++)
		for(j=0;j<256;j++)
			code_value_table[i][j]=0;
	
	for(i=0;i<10*64;i++)
	{
		MCUBuffer[i]=0;
		QtZzMCUBuffer[i]=0;
	}
	for(i=0;i<4*64;i++)
	{
		Y[i]=0;
		U[i]=0;
		V[i]=0;		
	}
	for(i=0;i<64;i++)
		BlockBuffer[i]=0;
	for(i=0;i<1024;i++)
		iclip[i]=0;
	ycoef=ucoef=vcoef=0;
	interval=0;
  only=0;
//	return 1;
}
/////////////////////////////////////////////////////////////////////////
static int InitTag()
{
	int	finish=0;
	unsigned char	id;
	short	llength;
	short	i,j,k;
	short	huftab1,huftab2;
	short	huftabindex;
	unsigned char	hf_table_index;
	unsigned char	qt_table_index;
	unsigned char	comnum;

	unsigned char	*lptemp;
	short	ccount;

//	lp=lpJpegBuf+2;
	lp+=2;

	while (!finish)
	{
		while(*lp != 0xff)
			lp++;
		id=*(lp+1);
		lp+=2;
		switch (id)
		{
		case M_APP0:		//e0
			llength=MAKEWORD(*(lp+1),*lp);
			lp+=llength;
			break;
		case M_DQT:			//db
			llength=MAKEWORD(*(lp+1),*lp);
			qt_table_index=(*(lp+2))&0x0f;
			lptemp=lp+3;
			if(llength<80)
			{
				for(i=0;i<64;i++)
					qt_table[qt_table_index][i]=(short)*(lptemp++);
			}
			else
			{
				for(i=0;i<64;i++)
					qt_table[qt_table_index][i]=(short)*(lptemp++);
				qt_table_index=(*(lptemp++))&0x0f;
				for(i=0;i<64;i++)
					qt_table[qt_table_index][i]=(short)*(lptemp++);
			}
			lp+=llength;		
			break;
		case M_SOF2:		//c2
			return 0;
		case M_SOF0:		//c0
	 		llength=MAKEWORD(*(lp+1),*lp);
	 		SPLASH_H=MAKEWORD(*(lp+4),*(lp+3));
	 		SPLASH_W=MAKEWORD(*(lp+6),*(lp+5));
			comp_num=*(lp+7);
			if((comp_num!=1)&&(comp_num!=3))
				return 0;
			if(comp_num==3)
			{
				comp_index[0]=*(lp+8);
				SampRate_Y_H=(*(lp+9))>>4;
				SampRate_Y_V=(*(lp+9))&0x0f;
				YQtTable=(short *)qt_table[*(lp+10)];

				comp_index[1]=*(lp+11);
				SampRate_U_H=(*(lp+12))>>4;
				SampRate_U_V=(*(lp+12))&0x0f;
				UQtTable=(short *)qt_table[*(lp+13)];

				comp_index[2]=*(lp+14);
				SampRate_V_H=(*(lp+15))>>4;
				SampRate_V_V=(*(lp+15))&0x0f;
				VQtTable=(short *)qt_table[*(lp+16)];
			}
			else
			{
				comp_index[0]=*(lp+8);
				SampRate_Y_H=(*(lp+9))>>4;
				SampRate_Y_V=(*(lp+9))&0x0f;
				YQtTable=(short *)qt_table[*(lp+10)];

				comp_index[1]=*(lp+8);
				SampRate_U_H=1;
				SampRate_U_V=1;
				UQtTable=(short *)qt_table[*(lp+10)];

				comp_index[2]=*(lp+8);
				SampRate_V_H=1;
				SampRate_V_V=1;
				VQtTable=(short *)qt_table[*(lp+10)];
			}
			lp+=llength;						    
			break;
		case M_DHT:			//c4
			llength=MAKEWORD(*(lp+1),*lp);
			{
	 			hf_table_index=*(lp+2);
				lp+=2;
				while (hf_table_index!=0xff)
				{
					huftab1=(short)hf_table_index>>4;     //huftab1=0,1
			 		huftab2=(short)hf_table_index&0x0f;   //huftab2=0,1
					huftabindex=huftab1*2+huftab2;
					lptemp=lp+1;
					ccount=0;
					for (i=0; i<16; i++)
					{
						code_len_table[huftabindex][i]=(short)(*(lptemp++));
						ccount+=code_len_table[huftabindex][i];
					}
					ccount+=17;
					j=0;
					for (i=0; i<16; i++)
						if(code_len_table[huftabindex][i]!=0)
						{
							k=0;
							while(k<code_len_table[huftabindex][i])
							{
								code_value_table[huftabindex][k+j]=(short)(*(lptemp++));
								k++;
							}
							j+=k;
						}
					i=0;
					while (code_len_table[huftabindex][i]==0)
						i++;
					for (j=0;j<i;j++)
					{
						huf_min_value[huftabindex][j]=0;
						huf_max_value[huftabindex][j]=0;
					}
					huf_min_value[huftabindex][i]=0;
					huf_max_value[huftabindex][i]=code_len_table[huftabindex][i]-1;
					for (j=i+1;j<16;j++)
					{
						huf_min_value[huftabindex][j]=(huf_max_value[huftabindex][j-1]+1)<<1;
						huf_max_value[huftabindex][j]=huf_min_value[huftabindex][j]+code_len_table[huftabindex][j]-1;
					}
					code_pos_table[huftabindex][0]=0;
					for (j=1;j<16;j++)
						code_pos_table[huftabindex][j]=code_len_table[huftabindex][j-1]+code_pos_table[huftabindex][j-1];
					lp+=ccount;
					hf_table_index=*lp;
				}  //while
			}  //else
			break;
		case M_DRI:		//dd
			llength=MAKEWORD(*(lp+1),*lp);
			restart=MAKEWORD(*(lp+3),*(lp+2));
			lp+=llength;
			break;
		case M_SOS:		//da
			llength=MAKEWORD(*(lp+1),*lp);
			comnum=*(lp+2);
			if(comnum!=comp_num)
				return 0;
			lptemp=lp+3;
			for (i=0;i<comp_num;i++)
			{
				if(*lptemp==comp_index[0])
				{
					YDcIndex=(*(lptemp+1))>>4;   //Y
					YAcIndex=((*(lptemp+1))&0x0f)+2;
				}
				else{
					UVDcIndex=(*(lptemp+1))>>4;   //U,V
					UVAcIndex=((*(lptemp+1))&0x0f)+2;
				}
				lptemp+=2;
			}
			lp+=llength;
			finish=1;
			break;
		case M_EOI: 	//d9   
			return 0;
			break;
		default:
 			if ((id&0xf0)!=0xd0)
			{
				llength=MAKEWORD(*(lp+1),*lp);
	 			lp+=llength;
			}
			else lp+=2;
			break;
		}  //switch
	} //while
	return 1;
}
/////////////////////////////////////////////////////////////////
static int
read_image_jpg(int type)
{
	filepos = 0;
	lp = (unsigned char*)JPG_FILE;
	if (!(size=grub_read((unsigned long long)(unsigned int)(char*)lp, 0x8000, GRUB_READ)))
		return !printf("Error:Read JPG File\n");
	InitTable();
	if (!(InitTag()))
		return 1;
	Decode();
	background_transparent=0;
	return 2;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#if 0
/* Read in the splashscreen image and set the palette up appropriately.
 * Format of splashscreen is an xpm (can be gzipped) with 16 colors and
 * 640x480. */
static int
read_image_xpm (int type)
{
    char buf[32], pal[16];
    unsigned char c, base, mask;
    unsigned i, len, idx, colors, x, y, width, height;
    unsigned char *s1;
    unsigned char *s2;
    unsigned char *s4;
    unsigned char *s8;
	unsigned char *lfb;

    s1 = (unsigned char*)VSHADOW1;
    s2 = (unsigned char*)VSHADOW2;
    s4 = (unsigned char*)VSHADOW4;
    s8 = (unsigned char*)VSHADOW8;

    /* parse info */
    while (grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90)) {
        if (c == '"')
            break;
    }

    while (grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90) && (c == ' ' || c == '\t'))
        ;

    i = 0;
    width = c - '0';
    while (grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90)) {
        if (c >= '0' && c <= '9')
            width = width * 10 + c - '0';
        else
            break;
    }
    while (grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90) && (c == ' ' || c == '\t'))
        ;

    height = c - '0';
    while (grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90)) {
        if (c >= '0' && c <= '9')
            height = height * 10 + c - '0';
        else
            break;
    }
    while (grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90) && (c == ' ' || c == '\t'))
        ;

    colors = c - '0';
    while (grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90)) {
        if (c >= '0' && c <= '9')
            colors = colors * 10 + c - '0';
        else
            break;
    }

    base = 0;
    while (grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90) && c != '"')
        ;

    /* palette */
    for (i = 0, idx = 1; i < colors; i++) {
        len = 0;

        while (grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90) && c != '"')
            ;
        grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90);       /* char */
        base = c;
        grub_read((unsigned long long)(unsigned int)buf, 4, 0xedde0d90);      /* \t c # */

        while (grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90) && c != '"') {
            if (len < sizeof(buf))
                buf[len++] = c;
        }

        if (len == 6 && idx < 15) {
            int r = (hex(buf[0]) << 4) | hex(buf[1]);
            int g = (hex(buf[2]) << 4) | hex(buf[3]);
            int b = (hex(buf[4]) << 4) | hex(buf[5]);

            pal[idx] = base;
	    image_pal[idx] = (r<<16) | (g<<8) | b;
            ++idx;
        }
    }

    for (i = 0; i < plano_size / 4; i++)
	((long *)s1)[i] = ((long *)s2)[i] = ((long *)s4)[i] = ((long *)s8)[i] = 0;

    /* parse xpm data */
    for (y = len = 0; y < height && y < ypixels; ++y, len += x1) {
        while (1) {
            if (!grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90)) {
//                grub_close();
                return 0;
            }
            if (c == '"')
                break;
        }

        for (x = 0; grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90) && c != '"'; ++x)
	{
            if (x < width && x < xpixels)
            {
              for (i = 1; i < 15; i++)
                if (pal[i] == c) {
                    c = i;
                    break;
                }
              if (type == 0)
              {
              mask = 0x80 >> (x & 7);
              if (c & 1) s1[len + (x >> 3)] |= mask;
              if (c & 2) s2[len + (x >> 3)] |= mask;
              if (c & 4) s4[len + (x >> 3)] |= mask;
              if (c & 8) s8[len + (x >> 3)] |= mask;
              }
              else
              {
								lfb = (unsigned char *)SPLASH_IMAGE + x*current_bytes_per_pixel + y*width*current_bytes_per_scanline;
								if(current_bits_per_pixel == 24 || current_bits_per_pixel == 32)
//									SPLASH_IMAGE[y*width+x] = image_pal[c];
									*(unsigned long *)lfb = image_pal[c];
								else
									*(unsigned short *)lfb = (unsigned short)pixel_shift(image_pal[c]);
              }
            }
        }
    }

//    grub_close();
    if (type == 1)
    {
	SPLASH_W = width;
	SPLASH_H = height;
	return 2;
    }

//set_palette:
    image_pal[0] = background;
    image_pal[15] = foreground;
    for (i=0; i < 16;++i)
	graphics_set_palette(i,image_pal[i]);

    return 1;
}
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long rr, gg, bb;
unsigned long pixel_shift(unsigned long color)
{
	unsigned long r,g,b;

	b = color & 0xff;
	g = (color >> 8) & 0xff;
	r = (color >> 16) & 0xff;
	if((r += rr) >= 0x100)
		r = 0xff;
	if((g += gg) >= 0x100)
		g = 0xff;
	if((b += bb) >= 0x100)
		b = 0xff;
	rr = r & 0xf;
	gg = g & 07;
	bb = b & 0xf;

	if (current_bits_per_pixel == 16)
		color = (r>>3)<<11 | (g>>2)<<5 | b>>3;
	else
		color = (r>>3)<<10 | (g>>3)<<5 | b>>3;
	return color;
}


static int read_image()
{
	char buf[16];
	if (*splashimage == 1)
	{
		if (splashimage_loaded & 1)
		{
			int i=0;
			for (i=1 ;i<15;++i)
				graphics_set_palette(i,image_pal[i]);
		}
		return 1;
	}
	if (!*splashimage)
	{
		splashimage_loaded = 0;
		*splashimage = 1;
		if (graphics_mode < 0xFF)
		{
			return 0;
#if 0
			unsigned char *s1;
			unsigned char *s2;
			unsigned char *s4;
			unsigned char *s8;
			unsigned i;
			s1 = (unsigned char*)VSHADOW1;
			s2 = (unsigned char*)VSHADOW2;
			s4 = (unsigned char*)VSHADOW4;
			s8 = (unsigned char*)VSHADOW8;
			for (i = 0; i < plano_size / 4; i++)
				((long *)s1)[i] = ((long *)s2)[i] = ((long *)s4)[i] = ((long *)s8)[i] = 0;
			graphics_set_palette( 0, background);
			graphics_set_palette(15, foreground);
#endif
		}
		return 1;
	}
	if (! grub_open(splashimage))
	{
		return 0;
	}
	/* read header */
	grub_read((unsigned long long)(unsigned int)(char*)&buf, 10, 0xedde0d90);
	splashimage_loaded = IMAGE_BUFFER;
	if (*(unsigned short*)buf == 0x4d42) /*BMP */
	{
		splashimage_loaded |= read_image_bmp(graphics_mode > 0xFF);
	}
#if 0
	else if (grub_memcmp(buf, "/* XPM */\n", 10) == 0) /* XPM */
	{
		splashimage_loaded |= read_image_xpm(graphics_mode > 0xFF);
	}
#endif
	else if (*(unsigned short*)buf == 0xD8FF)
	{
		splashimage_loaded |= read_image_jpg(graphics_mode > 0xFF);
	}

	*splashimage = 1;
	grub_close();
	return splashimage_loaded & 0xf;
}

/* Convert a character which is a hex digit to the appropriate integer */
int
hex (int v)
{
#if 0
    if (v >= 'A' && v <= 'F')
        return (v - 'A' + 10);
    if (v >= 'a' && v <= 'f')
        return (v - 'a' + 10);
    return (v - '0');
#else
/*
   by chenall 2011-12-01.
   '0' & 0xf = 0 ... '9' & 0xf = 9;
   'a' & 0xf = 1 ... 'f' & 0xf = 6;
   'A' & 0xf = 1 ... 'F' & 0xf = 6;
*/
	if (v >= 'A')
		v += 9;
	return v & 0xf;
#endif
}


/* scroll the screen */
void bios_scroll_up();
static void
graphics_scroll (void)
{
    unsigned long i;
    unsigned long old_state = cursor_state;
    cursor_state &= ~1;
    unsigned long long clo64 = current_color_64bit;
    unsigned int clo = current_color;
    if (graphics_mode <= 0xFF)
    {/* VGA */
	bios_scroll_up ();
    }
    else
    {/* VBE */
#if 0
	memmove_forward_SSE ((char *)current_phys_base, (char *)current_phys_base + (current_bytes_per_scanline * (font_h + line_spacing)),
		    (y1 - 1) * current_bytes_per_scanline * (font_h + line_spacing));
#else
  grub_memcpy ((char *)current_phys_base, (char *)current_phys_base + (current_bytes_per_scanline * (font_h + line_spacing)),
		    (y1 - 1) * current_bytes_per_scanline * (font_h + line_spacing));
#endif
    }

		if (current_term->setcolorstate)
//			current_term->setcolorstate (COLOR_STATE_NORMAL);	//避免图形模式时，在命令行滚屏，第24行被有其他属性的空格清屏
			current_term->setcolorstate (COLOR_STATE_STANDARD);	//避免图形模式时，在命令行滚屏，第24行被有其他属性的空格清屏  2022-12-15
    for (i=0;i<current_term->chars_per_line;++i)
	graphics_putchar(' ',1);
		current_color_64bit = clo64;
		current_color = clo;
    gotoxy(0,fonty);
    cursor_state = old_state;
    return;
}

void rectangle(int left, int top, int length, int width, int line)
{
	unsigned char *lfb,*p;
	int x,y,z,i;
	if (!graphics_inited || graphics_mode < 0xff || !line)
		return;

	if (width)
		y = current_bytes_per_scanline * (width - line);
	else
		y = 0;
	z = current_bytes_per_pixel;
	lfb = (unsigned char *)(current_phys_base + top * current_bytes_per_scanline + left * z);

	if (!length)
		goto vert;	
	
	for (i=0;i<line;++i)
	{
		p = lfb + current_bytes_per_scanline*i;
		for (x=0;x<length;++x)
		{
			if (z == 3)
			{
				*(unsigned short *)(p+y) = *(unsigned short *)p = (unsigned short)current_color_64bit;
				*(p+y+2) = *(p+2) = (unsigned char)(current_color_64bit>>16);
			}
			else if(z == 4)
			{
				*(unsigned long *)(p+y) = *(unsigned long *)p = (unsigned long)current_color_64bit;
			}
			else
				*(unsigned short *)(p+y) = *(unsigned short *)p = (unsigned short)pixel_shift((unsigned long)current_color_64bit);
			p += z;
		}
	}
vert:
	if (!width)
		return;

	if (length)
	{
		y = z * (length - line);
		lfb += line*current_bytes_per_scanline;
	}
	else
		y = 0;

	for (i=0;i<line;++i)
	{
		p = lfb + z * i;
		for (x=(length ? (line*2) : 0);x<width;++x)
		{
			if (z == 3)
			{
				*(unsigned short *)(p+y) = *(unsigned short *)p = (unsigned short)current_color_64bit;
				*(p+y+2) = *(p+2) = (unsigned char)(current_color_64bit>>16);
			}
			else if(z == 4)
			{
				*(unsigned long *)(p+y) = *(unsigned long *)p = (unsigned long)current_color_64bit;
			}
			else
				*(unsigned short *)(p+y) = *(unsigned short *)p = (unsigned short)pixel_shift((unsigned long)current_color_64bit);
			p += current_bytes_per_scanline;
		}
	}
	return;
}

static void
vbe_cursor (int set)
{
    unsigned long x, y,j;

#if 1
	x = fontx * (font_w+font_spacing);
	y = fonty * (font_h+line_spacing);
	y += line_spacing>>1;
    /* invert the beginning 1 vertical lines of the char */
	for (j = 2; j < font_h - 2; ++j)
	{
	    XorPixel (x, y + j, -1);
	}
#else
    /* invert the beginning 2 vertical lines of the char */
    for (i = 0; i < 2; ++i)
    {
	for (j = 0; j < 16; ++j)
	{
	    XorPixel (fontx * 8 + i, fonty * 16 + j, -1);
	}
    }
#endif
}

#if 0
static unsigned char chr[16 << 2];
static unsigned char mask[16];

static void
graphics_cursor (int set)
{
    unsigned char *pat, *mem, *ptr;
    int i, ch, offset;

    offset = (fonty << 4) * x1 + fontx;

    ch = text[fonty * x1 + fontx];

    pat = font8x16 + (((unsigned long)((unsigned char)ch)) << 4);

    mem = (unsigned char*)VIDEOMEM + offset;

    if (set)
    {
	MapMask(15);
	ptr = mem;
	for (i = 0; i < 16; i++, ptr += x1)
	{
		*ptr = ~pat[i];
	}
	return;
    }

    if (outline)
      for (i = 0; i < 16; i++)
      {
        mask[i] = pat[i];
	if (i < 15)
		mask[i] |= pat[i+1];
	if (i > 0)
		mask[i] |= pat[i-1];
        mask[i] |= (mask[i] << 1) | (mask[i] >> 1);
	mask[i] = ~(mask[i]);
      }

    for (i = 0; i < 16; i++, offset += x1)
    {
	unsigned char m, p, c1, c2, c4, c8;

	p = pat[i];

	if (!(cursor_state & 2) || !(splashimage_loaded & 1))
		goto put_pattern;
	if (is_highlight)
	{
		p = ~p;
put_pattern:
		if (splashimage_loaded & 1)
		{
			chr[i]=chr[i+16]=chr[i+32]=chr[i+48]= p;
		}
		else
		{
			chr[i     ] = (current_color & 1)?p:0;
			chr[16 + i] = (current_color & 2)?p:0;
			chr[32 + i] = (current_color & 4)?p:0;
			chr[48 + i] = (current_color & 8)?p:0;
			p = ~p;
			chr[i     ] |= (current_color & 0x10)?p:0;
			chr[16 + i] |= (current_color & 0x20)?p:0;
			chr[32 + i] |= (current_color & 0x40)?p:0;
			chr[48 + i] |= (current_color & 0x80)?p:0;
		}
		continue;
	}

	c1 = ((unsigned char*)VSHADOW1)[offset];
	c2 = ((unsigned char*)VSHADOW2)[offset];
	c4 = ((unsigned char*)VSHADOW4)[offset];
	c8 = ((unsigned char*)VSHADOW8)[offset];

	if (outline)
	{
		m = mask[i];

		c1 &= m;
		c2 &= m;
		c4 &= m;
		c8 &= m;
	}
	
	chr[i     ] = c1 | p;
	chr[16 + i] = c2 | p;
	chr[32 + i] = c4 | p;
	chr[48 + i] = c8 | p;
    }

    offset = 0;
    for (i = 1; i < 16; i <<= 1, offset += 16)
    {
        int j;
        MapMask(i);
        ptr = mem;
        for (j = 0; j < 16; j++, ptr += x1)
            *ptr = chr[j + offset];
    }

    MapMask(15);
}
#endif
#endif /* SUPPORT_GRAPHICS */
