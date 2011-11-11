/* graphics.c - graphics mode support for GRUB */
/* Implemented as a terminal type by Jeremy Katz <katzj@redhat.com> based
 * on a patch by Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
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
char splashimage[64];

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

static unsigned long splashimage_loaded = 0;

/* constants to define the viewable area */
unsigned long x1 = 80;
unsigned long y1 = 30;
unsigned long xpixels = 640;
unsigned long ypixels = 480;
unsigned long plano_size = 38400;
unsigned long graphics_mode = 0x12;
unsigned long current_x_resolution;
unsigned long current_y_resolution;
unsigned long current_bits_per_pixel;
unsigned long current_bytes_per_scanline;
unsigned long current_phys_base;

/* why do these have to be kept here? */
unsigned long foreground = 0xFFFFFF; //(63 << 16) | (63 << 8) | (63)
unsigned long background = 0;

/* global state so that we don't try to recursively scroll or cursor */
//static int no_scroll = 0;

/* graphics local functions */
static void graphics_scroll (void);
static void vbe_scroll (void);
void SetPixel (unsigned long x, unsigned long y, unsigned long color);
void XorPixel (unsigned long x, unsigned long y, unsigned long color);
static int read_image (void);
static void graphics_cursor (int set);
static void vbe_cursor (int set);
extern void (*graphics_CURSOR) (int set);
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

	lfb = (unsigned char *)(current_phys_base + (y * current_bytes_per_scanline) + (x * ((current_bits_per_pixel + 7) / 8)));
	switch (current_bits_per_pixel)
	{
	case 24:
		*(unsigned short *)lfb = (unsigned short)color;
		lfb[2] = (unsigned char)(color >> 16);
		break;
	case 32:
		*(unsigned long *)lfb = (unsigned long)color;
		break;
	}
}

void XorPixel (unsigned long x, unsigned long y, unsigned long color)
{
	unsigned char *lfb;

	if (x < 0 || y < 0 || x >= current_x_resolution || y >= current_y_resolution)
		return;

	lfb = (unsigned char *)(current_phys_base + (y * current_bytes_per_scanline) + (x * ((current_bits_per_pixel + 7) / 8)));
	switch (current_bits_per_pixel)
	{
	case 24:
		*(unsigned short *)lfb ^= (unsigned short)color;
		lfb[2] ^= (unsigned char)(color >> 16);
		break;
	case 32:
		*(unsigned long *)lfb ^= (unsigned long)color;
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
	if (! font8x16)
	    font8x16 = graphics_get_font ();

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

	    current_term->chars_per_line = x1 = current_x_resolution / 8;
	    current_term->max_lines = y1 = current_y_resolution / 16;

	    /* here should read splashimage. */

	    menu_broder.disp_ul = 0x14;
	    menu_broder.disp_ur = 0x15;
	    menu_broder.disp_ll = 0x16;
	    menu_broder.disp_lr = 0x13;
	    menu_broder.disp_horiz = 0x0F;
	    menu_broder.disp_vert = 0x0E;

	    graphics_CURSOR = (void *)&vbe_cursor;

	    graphics_inited = 1;
	    return 1;
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

	    menu_broder.disp_ul = 218;
	    menu_broder.disp_ur = 191;
	    menu_broder.disp_ll = 192;
	    menu_broder.disp_lr = 217;
	    menu_broder.disp_horiz = 196;
	    menu_broder.disp_vert = 179;
	}
    }

    if (! read_image ())
    {
	//set_videomode (3/*saved_videomode*/);
	graphics_end ();
	return !(errnum = ERR_LOAD_SPLASHIMAGE);
    }

    fontx = fonty = 0;
    graphics_inited = 1;

    return 1;
}

/* Leave graphics mode */
void
graphics_end (void)
{
	current_term = term_table; /* set terminal to console */
	set_videomode (3); /* set normal 80x25 text mode. */
	set_videomode (3); /* set it once more for buggy BIOSes. */

	menu_broder.disp_ul = 218;
	menu_broder.disp_ur = 191;
	menu_broder.disp_ll = 192;
	menu_broder.disp_lr = 217;
	menu_broder.disp_horiz = 196;
	menu_broder.disp_vert = 179;

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
	vbe_scroll();
	--fonty;
    }
}

static unsigned long
print_unicode (unsigned long max_width)
{
    unsigned long i, j;
    unsigned long pat;
    unsigned long char_width;

    char_width = 2;				/* wide char */
    pat = UNIFONT_START + (unicode << 5);

    if (*(unsigned long *)pat == narrow_char_indicator)
	{ --char_width; pat += 16; }		/* narrow char */

    if (max_width < char_width)
	return (1 << 31) | invalid | (byte_SN << 8); // printed width = 0

    if (cursor_state)
	vbe_cursor(0);

    /* print CRLF and scroll if needed */
    if (fontx + char_width > x1)
	{ fontx = 0; check_scroll (); }

    /* print dot matrix of the unicode char */
    for (i = 0; i < char_width * 8; ++i)
    {
	unsigned long tmp_x = fontx * 8 + i;
	unsigned long column = ((unsigned short *)pat)[i];
	for (j = 0; j < 16; ++j)
	{
	    /* print char using foreground and background colors. */
	    SetPixel (tmp_x, fonty * 16 + j, ((column >> j) & 1) ? current_color_64bit : (current_color_64bit >> 32));
	}
    }

    fontx += char_width;
    if (cursor_state)
    {
	if (fontx >= x1)
	    { fontx = 0; check_scroll (); }
	vbe_cursor(1);
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
	unicode |= 0xDCC0;
    return print_unicode (max_width) + (unsigned char)ret;
}

/* Print ch on the screen.  Handle any needed scrolling or the like */
unsigned int
graphics_putchar (unsigned int ch, unsigned int max_width)
{
    unsigned long i, j;
    unsigned long pat;
    unsigned long ret;

    if (graphics_mode <= 0xFF)
	goto vga;

    /* VBE */

    invalid = 0;

    if ((unsigned char)ch > 0x7F)	/* multi-byte */
	goto multibyte;

    if (pending)	/* print (byte_SN) invalid bytes */
    {
	pending = 0;	/* end the utf8 byte sequence */
	ret = print_invalid_pending_bytes (max_width);
	if ((long)ret < 0)
	    return ret;
	max_width -= (unsigned char)ret;
    }

    if (! max_width)
	return (1 << 31);	/* printed width = 0 */

    if (cursor_state)
	vbe_cursor(0);

    if (fontx >= x1)
        { fontx = 0; check_scroll (); }

    if ((char)ch == '\n')
    {
	check_scroll ();
	if (cursor_state)
	    vbe_cursor(1);
	return 1;
    }
    if ((char)ch == '\r')
    {
	fontx = 0;
	if (cursor_state)
	    vbe_cursor(1);
	return 1;
    }

    /* we know that the ASCII chars are all narrow */
    pat = UNIFONT_START + 16 + (((unsigned long)(unsigned char)ch) << 5);

    /* print dot matrix of the ASCII char */
    for (i = 0; i < 8; ++i)
    {
	unsigned long tmp_x = fontx * 8 + i;
	unsigned long column = ((unsigned short *)pat)[i];
	for (j = 0; j < 16; ++j)
	{
	    /* print char using foreground and background colors. */
	    SetPixel (tmp_x, fonty * 16 + j, ((column >> j) & 1) ? current_color_64bit : (current_color_64bit >> 32));
	}
    }

    fontx++;
    if (cursor_state)
    {
	if (fontx >= x1)
	    { fontx = 0; check_scroll (); }
	vbe_cursor(1);
    }
    return 1; //char_width;

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
    if ((char)ch == '\n') {
	if (cursor_state)
	    graphics_CURSOR(0);
	if (fonty + 1 < y1)
	{
	    fonty++;
	}
	else
	{
	    graphics_scroll();
	}
	if (cursor_state)
	    graphics_CURSOR(1);
	return 1;
    } else if ((char)ch == '\r') {
	if (cursor_state)
	    graphics_CURSOR(0);
	fontx = 0;
	if (cursor_state)
	    graphics_CURSOR(1);
	return 1;
    }

    text[fonty * x1 + fontx] = (unsigned char)ch;

    graphics_CURSOR(0);

    if (fontx + 1 >= x1)
    {
        if (fonty + 1 < y1)
	{
	    fontx = 0;
	    fonty++;
	}
        else
	{
	    fontx = 0;
            graphics_scroll();
	}
    } else {
	fontx++;
    }

    if (cursor_state)
	graphics_CURSOR(1);
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
    if (cursor_state)
	graphics_CURSOR(0);

    fontx = x;
    fonty = y;

    if (cursor_state)
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

    _memset ((char *)current_phys_base, 0, current_y_resolution * current_bytes_per_scanline);
    if (cursor_state)
	    vbe_cursor(1);
    return;

vga:
    mem = (unsigned char*)VIDEOMEM;

    for (i = 0; i < x1 * y1; i++)
        text[i] = ' ';

    if (cursor_state)
    {
	MapMask(15);
	_memset (mem, 0, plano_size);
	return;
    }

    s1 = (unsigned char*)VSHADOW1;
    s2 = (unsigned char*)VSHADOW2;
    s4 = (unsigned char*)VSHADOW4;
    s8 = (unsigned char*)VSHADOW8;

    BitMask(0xff);

    /* plano 1 */
    MapMask(1);
    _memcpy_forward (mem, s1, plano_size);

    /* plano 2 */
    MapMask(2);
    _memcpy_forward (mem, s2, plano_size);

    /* plano 3 */
    MapMask(4);
    _memcpy_forward (mem, s4, plano_size);

    /* plano 4 */
    MapMask(8);
    _memcpy_forward (mem, s8, plano_size);

    MapMask(15);
 
}

/* Read in the splashscreen image and set the palette up appropriately.
 * Format of splashscreen is an xpm (can be gzipped) with 16 colors and
 * 640x480. */
static int
read_image (void)
{
    char buf[32], pal[16];
    unsigned char c, base, mask;
    unsigned i, len, idx, colors, x, y, width, height;
    unsigned char *s1;
    unsigned char *s2;
    unsigned char *s4;
    unsigned char *s8;

    s1 = (unsigned char*)VSHADOW1;
    s2 = (unsigned char*)VSHADOW2;
    s4 = (unsigned char*)VSHADOW4;
    s8 = (unsigned char*)VSHADOW8;

    if (! grub_open(splashimage))
    {
	errnum = 0;

	if (splashimage_loaded)
		return 1;

	for (i = 0; i < plano_size / 4; i++)
		((long *)s1)[i] = ((long *)s2)[i] = ((long *)s4)[i] = ((long *)s8)[i] = 0;

        goto set_palette;
    }

    /* read header */
    if (! grub_read((unsigned long long)(unsigned int)(char*)&buf, 10, 0xedde0d90) || grub_memcmp(buf, "/* XPM */\n", 10)) {
        grub_close();
        return 0;
    }
    
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
            graphics_set_palette(idx, (r<<16) | (g<<8) | b);
            ++idx;
        }
    }

    for (i = 0; i < plano_size / 4; i++)
	((long *)s1)[i] = ((long *)s2)[i] = ((long *)s4)[i] = ((long *)s8)[i] = 0;

    /* parse xpm data */
    for (y = len = 0; y < height && y < ypixels; ++y, len += x1) {
        while (1) {
            if (!grub_read((unsigned long long)(unsigned int)(char *)&c, 1, 0xedde0d90)) {
                grub_close();
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

              mask = 0x80 >> (x & 7);
              if (c & 1) s1[len + (x >> 3)] |= mask;
              if (c & 2) s2[len + (x >> 3)] |= mask;
              if (c & 4) s4[len + (x >> 3)] |= mask;
              if (c & 8) s8[len + (x >> 3)] |= mask;
            }
        }
    }

    grub_close();
    splashimage_loaded = 1;

set_palette:

    graphics_set_palette( 0, background);
    graphics_set_palette(15, foreground);

    return 1;
}


/* Convert a character which is a hex digit to the appropriate integer */
int
hex (int v)
{
    if (v >= 'A' && v <= 'F')
        return (v - 'A' + 10);
    if (v >= 'a' && v <= 'f')
        return (v - 'a' + 10);
    return (v - '0');
}


/* scroll the screen */
static void
graphics_scroll (void)
{
#if 1
    unsigned long i, j;

    fontx = 0;
    fonty = y1 - 1;
#if 0
    if (graphics_mode <= 0xFF)
	goto vga;

    /* VBE */

    _memcpy_forward ((char *)current_phys_base, (char *)current_phys_base + (current_bytes_per_scanline << 4),
		    /*((y1 - 1) << 4)*/ current_y_resolution * current_bytes_per_scanline);
    return;

vga:
#endif
    bios_scroll_up ();

    for (i = x1 * (y1 - 1), j = x1 * y1; i < j; i++)
	text[i] = ' ';

#else
    int i, j;

    /* we don't want to scroll recursively... that would be bad */
    if (no_scroll)
        return;
    no_scroll = 1;

    /* move everything up a line */
    for (j = 0 + 1; j < y1; j++)
    {
	//graphics_gotoxy (0, j - 1);
        fontx = 0;
	fonty = j - 1;

        for (i = 0; i < x1; i++)
       	{
            graphics_putchar (text[j * x1 + i], 255);
        }
    }

    /* last line should be blank */
    //graphics_gotoxy (0, y1 - 1);
    fontx = 0;
    fonty = y1 - 1;

    for (i = 0; i < x1; i++)
        graphics_putchar (' ', 255);

    fontx = 0;
    fonty = y1 - 1;

    no_scroll = 0;
#endif
}

static void
vbe_scroll (void)
{
#if 1
    _memcpy_forward ((char *)current_phys_base, (char *)current_phys_base + (current_bytes_per_scanline << 4),
		    /*((y1 - 1) << 4)*/ current_y_resolution * current_bytes_per_scanline);
#else
    bios_scroll_up ();
#endif
}

static void
vbe_cursor (int set)
{
    unsigned long i, j;

#if 1
    /* invert the beginning 1 vertical lines of the char */
	for (j = 0; j < 16; ++j)
	{
	    XorPixel (fontx * 8, fonty * 16 + j, -1);
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

    //if (set && no_scroll)
    //    return;

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

	if (cursor_state)
		goto put_pattern;
	if (is_highlight)
	{
		p = ~p;
put_pattern:
		chr[i     ] = p;
		chr[16 + i] = p;
		chr[32 + i] = p;
		chr[48 + i] = p;
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
