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

extern unsigned long splashimage_loaded;

/* constants to define the viewable area */
unsigned long x1 = 80;
unsigned long y1 = 30;
unsigned long font_w = 8;
unsigned long font_h = 16;
unsigned long font_spacing = 0;
unsigned long line_spacing = 0;
unsigned long xpixels = 640;
unsigned long ypixels = 480;
unsigned long plano_size = 38400;
unsigned long graphics_mode = 0x12;
unsigned long current_x_resolution;
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
static void graphics_cursor (int set);
static void vbe_cursor (int set);
void rectangle(int left, int top, int length, int width, int line);
extern void (*graphics_CURSOR) (int set);
unsigned long pixel_shift(unsigned long color);
/* FIXME: where do these really belong? */
static inline void outb(unsigned short port, unsigned char val)
{
    __asm __volatile ("outb %0,%1"::"a" (val), "d" (port));
}

static void MapMask(int value) {
    outb(0x3c4, 2);
    outb(0x3c5, value);
}

/* bit mask register */
static void BitMask(int value) {
    outb(0x3ce, 8);
    outb(0x3cf, value);
}

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
    if (! graphics_CURSOR)
	  graphics_CURSOR = (void *)&graphics_cursor;
	
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
		if (set_vbe_mode (graphics_mode) != 0x004F)
		{
			graphics_end ();
			return !(errnum = ERR_SET_VBE_MODE);
		}
		graphics_mode = 0x6A;
		goto success;
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
	menu_border.disp_ul = 0x14;
	menu_border.disp_ur = 0x15;
	menu_border.disp_ll = 0x16;
	menu_border.disp_lr = 0x13;
	menu_border.disp_horiz = 0x0F;
	menu_border.disp_vert = 0x0E;
    }

    if (! read_image ())
    {
	//set_videomode (3/*saved_videomode*/);
	graphics_end ();
	return !(errnum = ERR_LOAD_SPLASHIMAGE);
    }

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
    if (fonty >= y1)
    {
	--fonty;
	graphics_scroll();
    }
    if (count_lines >= 0)
		++count_lines;
}

static unsigned long
print_unicode (unsigned long max_width)
{
    unsigned long i, j;
    unsigned long pat;
    unsigned long char_width;
    unsigned long bgcolor;
    unsigned long CursorX,CursorY;
	unsigned char *lfb;

    char_width = 2;				/* wide char */
    pat = UNIFONT_START + (unicode << 5);

    if (*(unsigned long *)pat == narrow_char_indicator || unicode < 0x80)
	{ --char_width; pat += 16; }		/* narrow char */

    if (max_width < char_width)
	return (1 << 31) | invalid | (byte_SN << 8); // printed width = 0

    if (cursor_state & 1)
	graphics_CURSOR(0);

    /* print CRLF and scroll if needed */
    if (fontx + char_width > x1)
	{ fontx = 0; check_scroll (); }

	if (!(splashimage_loaded & 2) || !(cursor_state & 2) || (is_highlight && current_color_64bit >> 32))
		bgcolor = current_color_64bit >> 32 | 0x1000000;
	else
		bgcolor = 0;

	CursorX = fontx * (font_w + font_spacing);
	CursorY = fonty * (font_h + line_spacing);
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

    fontx += char_width;
    if (cursor_state & 1)
    {
	if (fontx >= x1)
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

    if (fontx >= x1)
        { fontx = 0; check_scroll (); }

    if (graphics_mode <= 0xFF)
	goto vga;

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
    int i;
    unsigned char *mem, *s1, *s2, *s4, *s8;

    fontx = 0;
    fonty = 0;

    if (graphics_mode <= 0xFF)
	goto vga;

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
				if(current_bits_per_pixel == 24 || current_bits_per_pixel == 32)
					*(unsigned long *)mem = *(unsigned long *)lfb;
				else
					*(unsigned short *)mem = *(unsigned short *)lfb;
			}
			else
			{
				if(current_bits_per_pixel == 24 || current_bits_per_pixel == 32)
					*(unsigned long *)mem = color;
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
 
}

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
	SPLASH_W = bmih.biWidth;
	SPLASH_H = bmih.biHeight;
//	unsigned long *bmp = SPLASH_IMAGE;
	unsigned char *bmp;
	if (debug > 0)
		printf("Loading splashimage...\n");
	for(y=bmih.biHeight-1;y>=0;--y)
	{
//		bmp = SPLASH_IMAGE+y*SPLASH_W;
		for(x=0;x<bmih.biWidth;++x)
		{
			grub_read((unsigned long long)(unsigned int)(char*)&bftmp,bfbit, GRUB_READ);
			bmp = (unsigned char *)SPLASH_IMAGE+x*current_bytes_per_pixel+y*current_bytes_per_scanline;
			if(current_bits_per_pixel == 24 || current_bits_per_pixel == 32)
//				bmp[x] = bftmp;
				*(unsigned long *)bmp = bftmp;
			else
				*(unsigned short *)bmp = (unsigned short)pixel_shift(bftmp);
		}
	}
	return 2;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*	本程序代码来源于：百度文库>专业资料>IT/计算机>计算机软件及应  “JPEG解码程序”  笔者：winbyL 
		资料链接：http://pan.baidu.com/share/link?shareid=460931&uk=2500441596
		本程序由 yaya 于 2015_07_07 补充、完善。
*/

static int read_image_jpg(int type);
void Initialize_Fast_IDCT(void);
void idctrow(short *blk);
void idctcol(short *blk);
void Fast_IDCT(short *block);
long EstablishHuffman(unsigned short *p, long m);
long ScanningHuffman(unsigned short *p1, unsigned short *p2, long n, long DCAC);
int FF00(void);
int RST(void);
unsigned long rr=0, gg=0, bb=0;
short	YDC=0, CbDC=0, CrDC=0;
short	DU[6][8][8]={};
unsigned char	*pTmp = (unsigned char*)JPG_FILE;
unsigned long long size;
long iJPG, lJPG, mJPG, xJPG, yJPG, zJPG;
short DC=0;
long data_size=0;

static int read_image_jpg(int type)
{
	long i, j, k, l, m, n, x, y;
	unsigned char	w0, w1, w2, w3, h0, h1, h2, h3;
	long Red, Green, Blue;
	unsigned long	MCU;												//Minimum Coded Unit
	unsigned char	MCU_count;
	unsigned char	QTN;
	unsigned char	HTN;

	unsigned char	ColorNum;
	unsigned char	HY,VY;
	unsigned char	res_Interval;
	unsigned char	Y_QT,Cb_QT,Cr_QT;
	unsigned char	YDC_HT,YAC_HT,CbDC_HT,CbAC_HT,CrDC_HT,CrAC_HT;
	
	unsigned char	*pSOF0=0, *pSOS=0, *pDRI=0;
	unsigned char	*pDQT[4]={0,0,0,0};
	unsigned char	*pDHT[4]={0,0,0,0};
	unsigned short	QT[4][8][8]={};
	unsigned short	DCHT[2][256][3]={};
	unsigned short	ACHT[2][256][3]={};
	short	vector[8][8]={};
	

	unsigned char *lfb;
	short	*pVec=0;	
	

	filepos = 0;
	if (!(size=grub_read((unsigned long long)(unsigned int)(char*)pTmp, 0x8000, GRUB_READ)))
	{
		return !printf("Error:Read JPG FileLoading JPG image ......\n");
	}
	printf("Loading JPG image ......\n");
	pTmp += 2;
	//--------------------------------------------------------------确定主要段的位置
	i = 0;
	j = 0;
	while(1)
	{
		while(pTmp[0]!=0xff)
			pTmp++;
		switch(pTmp[1])
		{
			case 0xc0:				//SOF0段
				pSOF0 = pTmp;
				break;
			case 0xda:				//SOS段
				pSOS = pTmp;
				break;
			case 0xdd:				//DRI段
				pDRI = pTmp;
				break;
			case 0xdb:				//DQT段
				pDQT[i] = pTmp;
				i++;
				break;
			case 0xc4:				//DHT段
				pDHT[j] = pTmp;
				j++;
				break;
			default:
				break;
		}
		if(pTmp[1] == 0xda)		//SOS段后即为图片数据部分
			break;
		else if((pTmp[1]&0xe0)==0xe0)
			pTmp += MAKEWORD(pTmp[3],pTmp[2])+2;  	//pTmp前移到下一个段
		else
			pTmp += 2;
	}
	//-----------------------------------------------------------------获取图片相关属性
	SPLASH_H = MAKEWORD(pSOF0[6],pSOF0[5]);
	SPLASH_W = MAKEWORD(pSOF0[8],pSOF0[7]);
	ColorNum = pSOF0[9];
	
	//***********获取采样系数，对应QT表及HT表
	switch(ColorNum)					
	{
		case 1:						//灰度图
			HY = pSOF0[11] >> 4;
			VY = pSOF0[11] & 0x0f;
			Y_QT = pSOF0[12];
			YDC_HT = pSOS[6] >> 4;
			YAC_HT = pSOS[6] & 0x0f;
			data_size = HY*VY;
			break;
		
		case 3:						//YCbCr彩色图
			for(i=0; i<3; i++)  //---QT
				switch(pSOF0[10 + i*3])
				{
					case 1:
						HY = pSOF0[11 + i*3] >> 4;
						VY = pSOF0[11 + i*3] & 0x0f;
						Y_QT = pSOF0[12 + i*3];
						break;
					case 2:
						data_size += (pSOF0[11 + i*3] >> 4)*(pSOF0[11 + i*3] & 0x0f);
						Cb_QT = pSOF0[12 + i*3];
						break;
					case 3:
						data_size += (pSOF0[11 + i*3] >> 4)*(pSOF0[11 + i*3] & 0x0f);
						Cr_QT = pSOF0[12 + i*3];
						break;
					default:
						break;
				}
			for(i=0; i<3; i++)  //---HT
				switch(pSOS[5 + i*2])
				{
					case 1:
						YDC_HT = pSOS[6 + i*2] >> 4;
						YAC_HT = pSOS[6 + i*2] & 0x0f;
						break;
					case 2:
						CbDC_HT = pSOS[6 + i*2] >> 4;
						CbAC_HT = pSOS[6 + i*2] & 0x0f;
						break;
					case 3:
						CrDC_HT = pSOS[6 + i*2] >> 4;
						CrAC_HT = pSOS[6 + i*2] & 0x0f;
						break;
					default:
						break;
				}
			data_size += HY*VY;
			break;
		
		default:					//暂不支持CMYK彩色图
			break;
	}

	//*****************重新开始间隔
	if(pDRI)
	{
		res_Interval = MAKEWORD(pDRI[5],pDRI[4]);	//MCU重新开始间隔
	}
	else
		res_Interval = 0;
	//--------------------------------------------------------------------建立QT表  量化表
	for(i=0; i<4; i++)
	{
		if(pDQT[i])
		{
			l = MAKEWORD(pDQT[i][3],pDQT[i][2]);						//段长度
			pTmp = pDQT[i]+4;
			
			while(l > 2)	//1个DQT段可能有多个QT表
			{
				j = pTmp[0] & 0x0f;
				k = pTmp[0] >> 4;
				if(k == 0)				//8位精度
				{
					for(x=0; x<8; x++)
						for(y=0; y<8; y++)
							QT[j][x][y] = (unsigned short)(pTmp[1+8*x+y]);
				}
				else							//16位精度
				{
					for(x=0; x<8; x++)
						for(y=0; y<8; y++)
							QT[j][x][y] = MAKEWORD(pTmp[2+16*x+2*y],pTmp[1+16*x+2*y]);
				}
				pTmp += 64*(k+1) + 1;
				l -= 64*(k+1) + 1;
			}
		}	
	}
	//--------------------------------------------------------------------建立Huffman树	
	for(i=0; i<4; i++)
	{
		if(pDHT[i])
		{
			l = MAKEWORD(pDHT[i][3],pDHT[i][2]);	//段长度
			pTmp = pDHT[i]+4;
			
			while(l > 2)													//1个DHT段中可能有多个HT
			{
				m = 0;
				
				j = pTmp[0] & 0x0f;
				k = pTmp[0] >> 4;
				if(k == 0)													//DC
					m=EstablishHuffman((unsigned short *)&DCHT[j][0][0],m);
				else
					m=EstablishHuffman((unsigned short *)&ACHT[j][0][0],m);				

				pTmp += m+17;
				l -= m+17;
			}
		}
	}
	//--------------------------------------------------------------------------------读取图片数据，并进行解码
		MCU = ((SPLASH_W+8*HY-1)/(8*HY))*((SPLASH_H+8*VY-1)/(8*VY));	//计算有几个MCU
		pTmp = pSOS +	MAKEWORD(pSOS[3],pSOS[2]) + 2;
		size -= ((unsigned long)pTmp - (unsigned long)JPG_FILE);
		iJPG = 0;																				//当前位数
		MCU_count = 0;																	//记录已处理的MCU个数，当有RST标记时使用
		w3 = 0; h3 = 0;																	//每块MCU的起始偏移
//======================================================================================================
//======================================================================================================
	while(MCU--)
	{
	//---------------------------------------------------------------读取1个MCU
		for(n=0; n<data_size; n++)
		{
			zJPG = 0;	  //读出来的数据
			mJPG = 1;   //已读位数
			lJPG = 0;	  //DU中已有的数量
			xJPG = 0;	  //DU中的x坐标
			yJPG = 0;	  //DU中的y坐标	
			//------------------------------------读取1个DU
			while(lJPG<64)
			{
				RST();	
				for(; iJPG<8; iJPG++,mJPG++)
				{
					zJPG = (zJPG<<1) | (((*pTmp)>>(7-iJPG)) & 0x01);
					if(lJPG == 0)				//---DC
					{
						if(n==data_size-2)
						{
							HTN = CbDC_HT;
							DC = CbDC;
						}
						else if(n==data_size-1)
						{
							HTN= CrDC_HT;
							DC = CrDC;
						}
						else
						{
							HTN = YDC_HT;
							DC = YDC;
						}

						ScanningHuffman((unsigned short *)&DCHT[HTN][0][0],(unsigned short *)&DU[n][0][0],n,0);	
					}
					else					//---AC
					{					
						if(n==data_size-2)
							HTN = CbAC_HT;
						else if(n==data_size-1)
							HTN = CrAC_HT;
						else
							HTN = YAC_HT;

						ScanningHuffman((unsigned short *)&ACHT[HTN][0][0],(unsigned short *)&DU[n][0][0],n,1);
					}
					if(lJPG == 64)																		//已读完一个DU
						break;
				}
				if(lJPG == 64)																			//若因为已读完一个DU而退出for循环，将i前移（因为前面break，不会执行i++）
				{
					iJPG++;
					if(iJPG == 8)
					{
						iJPG = 0;					
						FF00();
					}
				}
				else																						//若是因为读完一个字节而退出for循环，i=0（即下一字节）
				{
					iJPG = 0;
					FF00();
				}
			}
		}
		//*********注意：i和pTmp记录了当前读到的数据位置，因此，下面的处理动作不能改变i和pTmp*********//
		//---------------------------------------------------------------处理1个MCU
		for(n=0; n<data_size; n++)
		{					
			//-------------------------反Zig-zag编码
			pVec = &DU[n][0][0];
			for(x=0; x<8; x++)															//参考《zigzag扫描》文件红色部分//
				for(y=0; y<8; y++)
					vector[x][y] = pVec[Zig_Zag[x][y]];
			for(x=0; x<8; x++)
				for(y=0; y<8; y++)
					DU[n][x][y] = vector[x][y];
			//--------------------------反量化
			if(n < data_size-2)
				QTN = Y_QT;
			else if(n == data_size)
				QTN = Cb_QT;
			else
				QTN = Cr_QT;
			for(x=0; x<8; x++)
				for(y=0; y<8; y++)
					DU[n][x][y] = (short)(DU[n][x][y] * QT[QTN][x][y]);	
			//---------------------------IDCT
			Fast_IDCT(&DU[n][0][0]);
		}							
		for(n=0; n<data_size-2; n++)		//Y值统一加上128
			for(x=0; x<8; x++)
				for(y=0; y<8; y++)
					DU[n][x][y] += 128;
			//-------------------------------------------------------------YCrCb模型转换成RGB模型，并重组图片
		for(n=0,h2=0; h2<VY; h2++)
			for(w2=0; w2<HY; w2++,n++)
			{
				for(h1=0; h1<8/VY; h1++)
					for(w1=0; w1<8/HY; w1++)
					{
						for(h0=0; h0<VY; h0++)
							for(w0=0; w0<HY; w0++)
							{
								if((h3*8*VY+h2*8+h1*VY+h0) < SPLASH_H && (w3*8*HY+w2*8+w1*HY+w0) < SPLASH_W)
									if((h3*8*VY+h2*8+h1*VY+h0) < current_y_resolution && (w3*8*HY+w2*8+w1*HY+w0) < current_x_resolution)
									{
										Red = (long)(DU[n][h1*VY+h0][w1*HY+w0]
												+ DU[data_size-1][4*h2+h1][4*w2+w1] 
												+ DU[data_size-1][4*h2+h1][4*w2+w1]/2 - DU[data_size-1][4*h2+h1][4*w2+w1]/16 - DU[data_size-1][4*h2+h1][4*w2+w1]/32);
										Green = (long)(DU[n][h1*VY+h0][w1*HY+w0]
												- DU[data_size-2][4*h2+h1][4*w2+w1]/4 - DU[data_size-2][4*h2+h1][4*w2+w1]/16 - DU[data_size-2][4*h2+h1][4*w2+w1]/32 
												- DU[data_size-1][4*h2+h1][4*w2+w1]/2 -DU[data_size-1][4*h2+h1][4*w2+w1]/4 + DU[data_size-1][4*h2+h1][4*w2+w1]/32);
										Blue = (long)(DU[n][h1*VY+h0][w1*HY+w0] 
												+ DU[data_size-2][4*h2+h1][4*w2+w1]
												+ DU[data_size-2][4*h2+h1][4*w2+w1]/2 + DU[data_size-2][4*h2+h1][4*w2+w1]/4 + DU[data_size-2][4*h2+h1][4*w2+w1]/32);
										Red = (Red>255)?255:((Red<0)?0:Red);		//注意R，G，B的范围
										Green = (Green>255)?255:((Green<0)?0:Green);
										Blue = (Blue>255)?255:((Blue<0)?0:Blue);
										lfb = (unsigned char *)SPLASH_IMAGE + (w3*8*HY+w2*8+w1*HY+w0)*current_bytes_per_pixel + (h3*8*VY+h2*8+h1*VY+h0)*current_bytes_per_scanline;
										if(current_bits_per_pixel == 24 || current_bits_per_pixel == 32)
										{
											*lfb++ = Blue;
											*lfb++ = Green;
											*lfb++ = Red;
										}
										else
										{
											Blue = Red<<16 | Green<<8 | Blue;
											*(unsigned short *)lfb = (unsigned short)pixel_shift((unsigned long)Blue);
										}
									}
							}
					}
			}
		if((++w3) == (SPLASH_W+8*HY-1)/(8*HY))	//下一块16*16(MCU)
		{
			h3++;
			w3 = 0;
		}
		if(res_Interval && (++MCU_count) == res_Interval)		//若存在重新开始间隔，则作出处理
		{
			if(iJPG)
			{
				pTmp += 1;
				size--;
				iJPG = 0;
			}
			YDC=0; CbDC=0; CrDC=0;
			MCU_count = 0;
			while(pTmp[0]!=0xff)
			{
				pTmp++;
				size--;
			}
		}
	}
	return 2;
}

int FF00(void)
{
	if(pTmp[0]==0xff && pTmp[1]==0x00)
	{
		pTmp+=2;
		size-=2;
	}
	else
	{
		pTmp+=1;
		size--;
	}
	return 1;
}	

int RST(void)
{
	while(pTmp[0]==0xff)
	{
		if(pTmp[1]==0x00)
			break;
		else if(pTmp[1]>=0xd0 && pTmp[1]<=0xd7) //RST标记
		{
			pTmp+=2;
			size-=2;
		}
		else
		{
			pTmp++;
			size--;
		}
	}
	if(size <= 16)
	{
		grub_memmove64((unsigned long long)(int)JPG_FILE,(unsigned long long)(int)pTmp,(unsigned long long)size);
		size+=grub_read((unsigned long long)(unsigned int)(char*)JPG_FILE+size, 0x7e00, GRUB_READ);
		pTmp=(unsigned char*)JPG_FILE;
	}
	return 1;
}


long EstablishHuffman(unsigned short *p, long m)
{
	long x;
	long y = 1;
	long z = 0;
	p[1] = 0;
	for(x=0; x<16; x++)
		if(pTmp[1+x])
		{
			p[0] = x + 1;
			pTmp[1+x]--;
			m++;															//记录表长	
			break;
		}		
	for(; x<16; x++)
	{	 
		if(pTmp[1+x])
		{
			p[y*3] = x + 1;
			if((p[(y-1)*3+1]+1) == (1<<x))		//若加1后，已够位，则不用左移
				p[y*3+1] = p[(y-1)*3+1] + 1;
			else															//否则，左移到够位
				p[y*3+1] = (p[(y-1)*3+1] + 1) << z;
			y++;
			pTmp[1+x]--;
			m++;
			while(pTmp[1+x]--)
			{
				p[y*3] = x + 1;
				p[y*3+1] = p[(y-1)*3+1] + 1;
				y++;
				m++;
			}
			z = 1;
		}
		else
			z++;				
	}	
	for(x=0; x<m; x++)
		p[x*3+2] = pTmp[17+x];
	return m;
}

long ScanningHuffman(unsigned short *p1, unsigned short *p2, long n, long DCAC)
{
	short Diff;
	long j;
	long k=0;																		//Huffman树的行号
	while(1)																		//***扫描Huffman树***  dc//
	{
		if(p1[k*3] > mJPG)
			break;
		if(p1[k*3] == mJPG && p1[k*3+1] == zJPG)
		{
			zJPG = 0;
			
			if(DCAC==1)
			{
				if(p1[k*3+2] == 0)									//EOB：即此DU后面都是0
				{
					for(; xJPG<8; xJPG++)
					{
						for(; yJPG<8; yJPG++)
							p2[xJPG*8+yJPG]=0;
						yJPG = 0;
					}
					lJPG = 64;
					break;
				}						
				j = p1[k*3+2] >> 4;									//高四位：充零个数
				while(j--)
				{
					p2[xJPG*8+yJPG]=0;
					lJPG++;
					yJPG++;
					if(yJPG == 8)
						{	xJPG++; yJPG=0;	}
				}						
				j = p1[k*3+2] & 0x0f;								//低四位：后面数字的位数
			}
			else 									
				j = p1[k*3+2];											//记录权值，表示Diff的位数，即接下来要读入的位数
			
			while(j--)
			{
				if((++iJPG) == 8)										//前移pTmp时，同样要处理图片数据中的0xff
				{
					FF00();
					RST();
					iJPG = 0;
				}
				zJPG = (zJPG<<1) | (((*pTmp)>>(7-iJPG)) & 0x01);	//读数据
			}
			if(DCAC==1)
				j = p1[k*3+2] & 0x0f;
			else						
				j = p1[k*3+2] & 0x0f;								//恢复j的值
			
			if(zJPG < (1<<(j-1)))									//处理"按位数存储"
				Diff = (short)(zJPG - ((2<<(j-1)) - 1));
			else
				Diff = (short)zJPG;
			
			if(DCAC==1)										
				p2[xJPG*8+yJPG]=Diff;								//*-*存储AC*-*//
			else
			{							
				p2[xJPG*8+yJPG]=DC + Diff;					//*-*存储第一个值DC*-*//
				if(n==data_size-2)
					CbDC = p2[xJPG*8+yJPG];
				else if(n==data_size-1)
					CrDC = p2[xJPG*8+yJPG];
				else
					YDC = p2[xJPG*8+yJPG];
			}
			yJPG++;
			if(DCAC==1)
				if(yJPG == 8)
					{	xJPG++; yJPG=0;	}
			
			lJPG++;
			zJPG = 0;
			mJPG = 0;
			break;
		}
		else
			k++;
	}
	return 1;
}

unsigned long pixel_shift(unsigned long color)
{
	unsigned long r,g,b;
	//颜色补偿
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
	//颜色合成
	color = (r>>3)<<11 | (g>>2)<<5 | b>>3;
	return color;
}

static short iclip[1024];
static short *iclp=0;

void Initialize_Fast_IDCT(void)
{
	short i;

	iclp = iclip + 512;
	for(i=-512; i<512; i++)
		iclp[i] = (i<-256)?-256:((i>255)?255:i);
}

void idctrow(short *blk)
{
	int x0, x11, x2, x3, x4, x5, x6, x7, x8;
	
	//intcut
	if (!((x11 = blk[4]<<11) | (x2 = blk[6]) | (x3 = blk[2]) |
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
	x8 = x0 + x11;
	x0 -= x11;
	x11 = W6*(x3+x2);
	x2 = x11 - (W2+W6)*x2;
	x3 = x11 + (W2-W6)*x3;
	x11 = x4 + x6;
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
	blk[0] = (x7+x11)>>8;
	blk[1] = (x3+x2)>>8;
	blk[2] = (x0+x4)>>8;
	blk[3] = (x8+x6)>>8;
	blk[4] = (x8-x6)>>8;
	blk[5] = (x0-x4)>>8;
	blk[6] = (x3-x2)>>8;
	blk[7] = (x7-x11)>>8;
}

void idctcol(short *blk)
{
	int x0, x11, x2, x3, x4, x5, x6, x7, x8;

	//intcut
	if(!((x11 = (blk[8*4]<<8)) | (x2 = blk[8*6]) | (x3 = blk[8*2]) |
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
	x8 = x0 + x11;
	x0 -= x11;
	x11 = W6*(x3+x2) + 4;
	x2 = (x11-(W2+W6)*x2)>>3;
	x3 = (x11+(W2-W6)*x3)>>3;
	x11 = x4 + x6;
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
	blk[8*0] = iclp[(x7+x11)>>14];
	blk[8*1] = iclp[(x3+x2)>>14];
	blk[8*2] = iclp[(x0+x4)>>14];
	blk[8*3] = iclp[(x8+x6)>>14];
	blk[8*4] = iclp[(x8-x6)>>14];
	blk[8*5] = iclp[(x0-x4)>>14];
	blk[8*6] = iclp[(x3-x2)>>14];
	blk[8*7] = iclp[(x7-x11)>>14];
}

void Fast_IDCT(short *block)
{
	short i;
	
	Initialize_Fast_IDCT();
	
	for (i=0; i<8; i++)
		idctrow(block+8*i);

	for(i=0; i<8; i++)
		idctcol(block+i);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	else if (grub_memcmp(buf, "/* XPM */\n", 10) == 0) /* XPM */
	{
		splashimage_loaded |= read_image_xpm(graphics_mode > 0xFF);
	}
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
    if (graphics_mode <= 0xFF)
    {/* VGA */
	bios_scroll_up ();
    }
    else
    {/* VBE */

	memmove_forward_SSE ((char *)current_phys_base, (char *)current_phys_base + (current_bytes_per_scanline * (font_h + line_spacing)),
		    /*((y1 - 1) << 4)*/ current_y_resolution * current_bytes_per_scanline);
    }

    for (i=0;i<x1;++i)
	graphics_putchar(' ',1);
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

	y = current_bytes_per_scanline * (width - line);
	z = current_bytes_per_pixel;
	lfb = (unsigned char *)(current_phys_base + top * current_bytes_per_scanline + left * z);

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

	y = z * (length - line);
	lfb += line*current_bytes_per_scanline;
	for (i=0;i<line;++i)
	{
		p = lfb + z * i;
		for (x=line*2;x<width;++x)
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
	for (j = 2; j < 14; ++j)
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

#endif /* SUPPORT_GRAPHICS */
