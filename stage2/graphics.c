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

/* constants to define the viewable area */
const unsigned long x0 = 0;
unsigned long x1 = 80;
const unsigned long y0 = 0;
unsigned long y1 = 30;
unsigned long xpixels = 640;
unsigned long ypixels = 480;
unsigned long plano_size = 38400;
unsigned long graphics_mode = 0x12;
unsigned long current_x_resolution;
unsigned long current_y_resolution;
unsigned long current_bits_per_pixel;

/* why do these have to be kept here? */
unsigned long foreground = (63 << 16) | (63 << 8) | (63), background = 0, border = 0;

/* current position */
extern int fontx;
extern int fonty;

/* global state so that we don't try to recursively scroll or cursor */
static int no_scroll = 0;

/* color state */
extern int current_color;


/* graphics local functions */
static void graphics_setxy (int col, int row);
static void graphics_scroll (void);
static int read_image (char *s);
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



///* Set the splash image */
//void graphics_set_splash(char *splashfile) {
//    grub_strcpy(splashimage, splashfile);
//}

///* Get the current splash image */
//char *graphics_get_splash(void) {
//    return splashimage;
//}

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
	font8x16 = (unsigned char *) graphics_get_font (); /* code in asm.S */

	if (graphics_mode > 0xFF) /* VBE */
	{
	    if (graphics_mode == 0x102)
	    {
		if (set_vbe_mode (graphics_mode) != 0x004F)
			return !(errnum = ERR_SET_VBE_MODE);
		goto success;
	    }
	    if (set_vbe_mode (graphics_mode | (1 << 14)) != 0x004F)
		return !(errnum = ERR_SET_VBE_MODE);

	    /* here should read splashimage and font. */

	    /* initialize using the VGA font */
	    if (narrow_char_indicator == 0)	/* not initialized */
	    {
		unsigned long i, j, k;
		/* first, initialize all chars as narrow, each with
		 * an ugly pattern of its direct code! */
		for (i = 0; i < 0x10000; i++)
		{
		    /* set the new narrow_char_indicator to -1 */
		    *(unsigned long *)(UNIFONT_START + (i << 5)) = -1;
		    *(unsigned short *)(UNIFONT_START + (i << 5) + 16) = i;
		}
		/* then, initialize ASCII chars with VGA font. */
		for (i = 0; i < 0x7F; i++)
		{
		    for (j = 0; j < 8; j++)
		    {
			unsigned short tmp = 0;
			for (k = 0; k < 16; k++)
			{
			    tmp |= (((*(unsigned char *)(font8x16 + (i << 4)) + k) >> (7 - j)) & 1) << k;
			}
			((unsigned short *)(UNIFONT_START + (i << 5) + 16))[j] = tmp;
		    }
		}
	    }

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
				return !(errnum = ERR_SET_VGA_MODE);
			current_term->chars_per_line = x1 = 80;
			current_term->max_lines = y1 = 30;
			xpixels = 640;
			ypixels = 480;
			plano_size = (640 * 480) / 8;
		}
		else /* 800x600x4 */
		{
		    if (set_videomode (graphics_mode) != graphics_mode
			|| (*(unsigned char *)(0x8000 + 4) != graphics_mode)
			|| (*(unsigned short *)(0x8000 + 5) != 100)
			|| (*(unsigned char *)(0x8000 + 0x22) != 37)
			|| (*(unsigned short *)(0x8000 + 0x23) != 16)
			|| (*(unsigned short *)(0x8000 + 0x27) != 16)
			|| (*(unsigned char *)(0x8000 + 0x29) != 1)
			)
		    {
			/* probe 800x600x4 modes */
			for (tmp_mode = 0x15; tmp_mode < 0x78; tmp_mode++)
			{
			    if (set_videomode (tmp_mode) == tmp_mode
				&& (*(unsigned char *)(0x8000 + 4) == tmp_mode)
				&& (*(unsigned short *)(0x8000 + 5) == 100)
				&& (*(unsigned char *)(0x8000 + 0x22) == 37)
				&& (*(unsigned short *)(0x8000 + 0x23) == 16)
				&& (*(unsigned short *)(0x8000 + 0x27) == 16)
				&& (*(unsigned char *)(0x8000 + 0x29) == 1)
				)
			    {
				/* got it! */
				graphics_mode = tmp_mode;
				goto success;
			    }
			}
			set_videomode (3);
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
    }

    if (! read_image (splashimage))
    {
	//set_videomode (3/*saved_videomode*/);
	graphics_end ();
	return !(errnum = ERR_LOAD_SPLASHIMAGE);
    }

    graphics_inited = 1;

    return 1;
}

/* Leave graphics mode */
void
graphics_end (void)
{
    if (graphics_inited)
    {
        set_videomode (3/*saved_videomode*/);
        graphics_inited = 0;
    }
}

/* Print ch on the screen.  Handle any needed scrolling or the like */
void
graphics_putchar (unsigned int ch)
{
    //ch &= 0xff;

    //graphics_CURSOR(0);

    if ((char)ch == '\n') {
        if (fonty + 1 < y1)
            graphics_gotoxy(fontx, fonty + 1);
        else
	{
	    graphics_CURSOR(0);
            graphics_scroll();
	    graphics_CURSOR(1);
	}
        //graphics_CURSOR(1);
        return;
    } else if ((char)ch == '\r') {
        graphics_gotoxy(x0, fonty);
        //graphics_CURSOR(1);
        return;
    }

    //graphics_CURSOR(0);

    text[fonty * x1 + fontx] = (unsigned char)ch;
    //text[fonty * x1 + fontx] &= 0x00ff;
    //if (current_color & 0xf0)
    //    text[fonty * x1 + fontx] |= 0x10000;//0x100;

    graphics_CURSOR(0);

    if (fontx + 1 >= x1)
    {
        if (fonty + 1 < y1)
            graphics_setxy(x0, fonty + 1);
        else
	{
            graphics_setxy(x0, fonty);
            graphics_scroll();
	}
    } else {
        graphics_setxy(fontx + 1, fonty);
    }

    graphics_CURSOR(1);
}

/* get the current location of the cursor */
int
graphics_getxy(void)
{
    return (fontx << 8) | fonty;
}

void
graphics_gotoxy (int x, int y)
{
    graphics_CURSOR(0);

    graphics_setxy(x, y);

    graphics_CURSOR(1);
}

void
graphics_cls (void)
{
    int i;
    unsigned char *mem, *s1, *s2, *s4, *s8;

    graphics_CURSOR(0);
    graphics_gotoxy(x0, y0);

    mem = (unsigned char*)VIDEOMEM;
    s1 = (unsigned char*)VSHADOW1;
    s2 = (unsigned char*)VSHADOW2;
    s4 = (unsigned char*)VSHADOW4;
    s8 = (unsigned char*)VSHADOW8;

    for (i = 0; i < x1 * y1; i++)
        text[i] = ' ';
    graphics_CURSOR(1);

    BitMask(0xff);

    /* plano 1 */
    MapMask(1);
    grub_memcpy(mem, s1, plano_size);

    /* plano 2 */
    MapMask(2);
    grub_memcpy(mem, s2, plano_size);

    /* plano 3 */
    MapMask(4);
    grub_memcpy(mem, s4, plano_size);

    /* plano 4 */
    MapMask(8);
    grub_memcpy(mem, s8, plano_size);

    MapMask(15);
 
}

int
graphics_setcursor (int on)
{
    /* FIXME: we don't have a cursor in graphics */
    return 0;
}

/* Read in the splashscreen image and set the palette up appropriately.
 * Format of splashscreen is an xpm (can be gzipped) with 16 colors and
 * 640x480. */
static int
read_image (char *s)
{
    char buf[32], pal[16];
    unsigned char c, base, mask;
    unsigned i, len, idx, colors, x, y, width, height;

    unsigned char *s1 = (unsigned char*)VSHADOW1;
    unsigned char *s2 = (unsigned char*)VSHADOW2;
    unsigned char *s4 = (unsigned char*)VSHADOW4;
    unsigned char *s8 = (unsigned char*)VSHADOW8;

    if (! grub_open(s))
    {
	errnum = 0;
	//graphics_set_palette(1, 0, 0, 0);
	
	for (i = 0; i < plano_size / 4; i++)
		((long *)s1)[i] = ((long *)s2)[i] = ((long *)s4)[i] = ((long *)s8)[i] = 0;

	//for (y = 0, len = 0; y < 480; y++, len += 80) {
	//    for (x = 0; x < 640; x++) {
	//	s1[len + (x >> 3)] |= 0x80 >> (x & 7);
	//    }
	//}

        goto set_palette;	//return 0;
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
            int r = ((hex(buf[0]) << 4) | hex(buf[1])) >> 2;
            int g = ((hex(buf[2]) << 4) | hex(buf[3])) >> 2;
            int b = ((hex(buf[4]) << 4) | hex(buf[5])) >> 2;

            pal[idx] = base;
            graphics_set_palette(idx, r, g, b);
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
              if (c & 1)
                s1[len + (x >> 3)] |= mask;
              if (c & 2)
                s2[len + (x >> 3)] |= mask;
              if (c & 4)
                s4[len + (x >> 3)] |= mask;
              if (c & 8)
                s8[len + (x >> 3)] |= mask;
            }
        }
    }

    grub_close();

set_palette:

    graphics_set_palette(0, (background >> 16), (background >> 8) & 63, 
                background & 63);
    graphics_set_palette(15, (foreground >> 16), (foreground >> 8) & 63, 
                foreground & 63);
    graphics_set_palette(0x11, (border >> 16), (border >> 8) & 63, 
                         border & 63);

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


/* move the graphics cursor location to col, row */
static void
graphics_setxy (int col, int row)
{
    if (col >= x0 && col < x1)
    {
        fontx = col;
        cursorX = col << 3;
    }

    if (row >= y0 && row < y1)
    {
        fonty = row;
        cursorY = row << 4;
    }
}

/* scroll the screen */
static void
graphics_scroll (void)
{
    int i, j;

    /* we don't want to scroll recursively... that would be bad */
    if (no_scroll)
        return;
    no_scroll = 1;

    /* move everything up a line */
    for (j = y0 + 1; j < y1; j++)
    {
        graphics_gotoxy (x0, j - 1);

        for (i = x0; i < x1; i++)
       	{
            graphics_putchar (text[j * x1 + i]);
        }
    }

    /* last line should be blank */
    graphics_gotoxy (x0, y1 - 1);

    for (i = x0; i < x1; i++)
        graphics_putchar (' ');

    graphics_setxy (x0, y1 - 1);

    no_scroll = 0;
}


static unsigned char chr[16 << 2];
static unsigned char mask[16];

static void
graphics_cursor (int set)
{
    unsigned char *pat, *mem, *ptr;
    int i, ch, offset;

    if (set && no_scroll)
        return;

    offset = cursorY * x1 + fontx;

    ch = text[fonty * x1 + fontx];

    pat = font8x16 + (((unsigned long)((unsigned char)ch)) << 4);

    mem = (unsigned char*)VIDEOMEM + offset;

    if (set)
    {
        MapMask(15);
        ptr = mem;
        for (i = 0; i < 16; i++, ptr += x1)
       	{
            cursorBuf[i] = pat[i];
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

	if (is_highlight)
	{
		p = ~p;
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
	
	c1 |= p;
	c2 |= p;
	c4 |= p;
	c8 |= p;

	chr[i     ] = c1;
	chr[16 + i] = c2;
	chr[32 + i] = c4;
	chr[48 + i] = c8;
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
