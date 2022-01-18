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
//extern unsigned char *font8x16;

int outline = 0;

#define VSHADOW VSHADOW1

//extern unsigned long splashimage_loaded;

/* constants to define the viewable area */
unsigned int x1 = 80;
unsigned int y1 = 30;
unsigned int font_w = 8;
unsigned int font_h = 16;
unsigned char num_wide = (16+7)/8;
unsigned int font_spacing = 0;
unsigned int line_spacing = 0;
unsigned int xpixels = 640;
unsigned int ypixels = 480;
unsigned int plano_size = 38400;
unsigned int graphics_mode = 3;
//unsigned int current_x_resolution;
unsigned int current_y_resolution;
unsigned int current_bits_per_pixel;
unsigned int current_bytes_per_scanline;
unsigned int current_bytes_per_pixel;
unsigned long long current_phys_base;
unsigned int image_pal[16];
extern int use_phys_base;
int use_phys_base=0;

/* why do these have to be kept here? */
unsigned int foreground = 0xFFFFFF; //(63 << 16) | (63 << 8) | (63)
unsigned int background = 0;

/* global state so that we don't try to recursively scroll or cursor */
//static int no_scroll = 0;

/* graphics local functions */
static void graphics_scroll (void);
void SetPixel (unsigned int x, unsigned int y, unsigned int color);
void XorPixel (unsigned int x, unsigned int y, unsigned int color);
static int read_image (void);
//static void graphics_cursor (int set);
static void vbe_cursor (int set);
void rectangle(int left, int top, int length, int width, int line);
extern void (*graphics_CURSOR) (int set);
unsigned int pixel_shift(unsigned int color);
/* FIXME: where do these really belong? */

extern void memmove_forward_SSE (void *dst, const void *src, unsigned int len);

void SetPixel (unsigned int x, unsigned int y, unsigned int color)
{
	unsigned char *lfb;

	if (x < 0 || y < 0 || x >= current_x_resolution || y >= current_y_resolution)
		return;

	lfb = (unsigned char *)(grub_size_t)(current_phys_base + (y * current_bytes_per_scanline) + (x * current_bytes_per_pixel));
	switch (current_bits_per_pixel)
	{
	case 24:
		*(unsigned short *)lfb = (unsigned short)color;
		lfb[2] = (unsigned char)(color >> 16);
		break;
	case 32:
		*(unsigned int *)lfb = (unsigned int)color;
		break;
	default:
		*(unsigned short *)lfb = (unsigned short)color;
		break;
	}
}

void XorPixel (unsigned int x, unsigned int y, unsigned int color)
{
	unsigned char *lfb;

	if (x >= current_x_resolution || y >= current_y_resolution)
		return;

	lfb = (unsigned char *)(grub_size_t)(current_phys_base + (y * current_bytes_per_scanline) + (x * current_bytes_per_pixel));
	switch (current_bits_per_pixel)
	{
	case 24:
		*(unsigned short *)lfb ^= (unsigned short)color;
		lfb[2] ^= (unsigned char)(color >> 16);
		break;
	case 32:
		*(unsigned int *)lfb ^= (unsigned int)color;
		break;
	default:
		*(unsigned short *)lfb ^= (unsigned short)color;
		break;
	}
}

/* Initialize a vga16 graphics display with the palette based off of
 * the image in splashimage.  If the image doesn't exist, leave graphics
 * mode.  */
int graphics_init (void);
int
graphics_init (void)
{
  if (! graphics_inited)		//如果不在图形模式
  {
	/* get font info before seting mode! some buggy BIOSes destroyed the
	 * red planar of the VGA on geting font info call. So we should set
	 * mode only after the get font info call.
	 */
    if (graphics_mode > 0xFF) /* VBE */
    {
	    current_term->chars_per_line = x1 = current_x_resolution / (font_w + font_spacing);
	    current_term->max_lines = y1 = current_y_resolution / (font_h + line_spacing);

	    /* here should read splashimage. */
	    graphics_CURSOR = (void *)&vbe_cursor;
			graphics_inited = 1;
    }
  }

  if (fill_color)//如果是满屏单色
  {
    if (use_phys_base == 0)
      {
        splashimage_loaded = (unsigned int)(grub_size_t)IMAGE_BUFFER;
        splashimage_loaded |= 2;
//        *splashimage = 1;
      }

    vbe_fill_color(fill_color);
    fill_color = 0;
  }
  else if (splashimage[0] && splashimage[0] != 0xd && splashimage[0] != 0xa)
  {
    if (! read_image ())  //如果加载图像失败
    {
      return !(errnum = ERR_LOAD_SPLASHIMAGE);
    }
  }
  
  fontx = fonty = 0;
  graphics_inited = graphics_mode;
  return 1;
}

/* Leave graphics mode */
void graphics_end (void);
void
graphics_end (void)
{ 
  current_term = term_table; /* set terminal to console */
  current_term->startup();
	graphics_CURSOR = 0;
	fontx = fonty = 0;
	graphics_inited = 0;
}

static unsigned int pending = 0;
static unsigned int byte_SN;
static unsigned int unicode;
static unsigned int invalid;

static void check_scroll (void);
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

int scroll_state = 0;
unsigned int ged_unifont_simp (unsigned int unicode);
static unsigned int graphics_print_unicode (unsigned int max_width);
static unsigned int
graphics_print_unicode (unsigned int max_width)
{
    unsigned int i, j/*, k*/;
//    unsigned int pat;
    unsigned int char_width;
    unsigned int bgcolor;
    unsigned int CursorX,CursorY;
	unsigned char *lfb, *pat, *p;
	unsigned char column;
	unsigned long long dot_matrix;
	unsigned long long back_color_64bit;
	unsigned int back_color;
	
	CursorX = fontx * (font_w + font_spacing);
	CursorY = fonty * (font_h + line_spacing);
;
	//print triangle	打印三角形
	if (unicode==0x10 || unicode==0x11)
	{
		lfb = (unsigned char *)(grub_size_t)(current_phys_base + CursorY*current_bytes_per_scanline + CursorX*current_bytes_per_pixel);
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
					*(unsigned int *)p = (unsigned int)current_color_64bit;
				else
					*(unsigned short *)p = (unsigned short)pixel_shift((unsigned int)current_color_64bit);
				
				p += current_bytes_per_scanline;
			}
		}
		char_width = 1;
		goto triangle;
	}

	if (unifont_simp_on)
		unicode = ged_unifont_simp (unicode);
    char_width = 2;				/* wide char */
		pat = (unsigned char *)UNIFONT_START + unicode*num_wide*font_h;

//		if (*(unsigned int *)pat == narrow_char_indicator || unicode < 0x80)
  if (((*(unsigned char *)(narrow_mem + unicode/8)) & (1 << (unicode&7))) == 0)
		{
			--char_width;
			pat += num_wide*font_w;
		}		/* narrow char */

//    if (max_width < char_width)
  if (max_width < char_width && unicode > 0x80)
	return (1 << 31) | invalid | (byte_SN << 8); // printed width = 0

		if (cursor_state & 1)
	graphics_CURSOR(0);
//打印字符背景色
    /* print CRLF and scroll if needed */
    if (fontx + char_width > current_term->chars_per_line)
	{ fontx = 0; check_scroll (); }

	if ((cursor_state & 2) && 															//如果在菜单界面, 并且
					(!(splashimage_loaded & 2)											//没有加载背景图像.
					|| (is_highlight && current_color_64bit >> 32)	//或者,菜单高亮并且有背景色(无论加载图像与否)
					|| (current_color_64bit & 0x1000000000000000)))	//或者,强制背景色(无论加载图像与否)
		bgcolor = current_color_64bit >> 32 | 0x1000000;			//则显示字符背景色
	else if ((cursor_state < 2) || 	scroll_state || OnCommandLine)  //如果在命令行界面, 或者滚屏状态
	{                                                       //增加OnCommandLine变量, 是为了确保在命令行, 避免胡乱使用 Fn.70 0
		back_color_64bit = current_color_64bit;
		back_color = current_color;
		current_term->setcolorstate (COLOR_STATE_STANDARD);		//显示控制台背景色
		bgcolor = current_color_64bit >> 32 | 0x1000000;
		current_color_64bit = back_color_64bit;
		current_color = back_color;
	}
	else																										//否则显示图像(背景透明)
		bgcolor = 0;

	for (i = 0; i<char_width * (font_w+font_spacing);++i)
	{
		unsigned int tmp_x = CursorX + i;
		for (j = 0;j<font_h+line_spacing;++j)
		{
			lfb = (unsigned char *)SPLASH_IMAGE + tmp_x*current_bytes_per_pixel + (CursorY+j)*current_bytes_per_scanline;
			SetPixel (tmp_x, CursorY + j, bgcolor ? bgcolor : *(unsigned int *)lfb);
		}
	}
//打印字符前景色
	CursorX += font_spacing>>1;
	CursorY += line_spacing>>1;

	/* print dot matrix of the unicode char */
	for (i = 0; i < char_width * font_w; ++i)
	{
		unsigned int tmp_x = CursorX + i;
		dot_matrix = 0;
		for(j = 0; j < num_wide; j++)
		{
			column = *(unsigned char *)pat;
			pat++;

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

  if (unicode < 0x80)
    char_width = 1;
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

static unsigned int print_unicode (unsigned int max_width);
static unsigned int
print_unicode (unsigned int max_width)
{
	if (current_term != term_table)
		return graphics_print_unicode (max_width);
	else
		console_print_unicode (unicode, max_width);

	return invalid | (byte_SN << 8) | (unicode < 0x80 ? 1 : 2);
}

static unsigned int print_invalid_pending_bytes (unsigned int max_width);
static unsigned int
print_invalid_pending_bytes (unsigned int max_width)
{
    unsigned int tmpcode = unicode;
    unsigned int ret = 0;
    /* now pending is on, so byte_SN can tell the number of bytes, and
     * unicode can tell the value of each byte.
     */
    invalid = (1 << 15);	/* set the error bit for return */
    if (byte_SN == 2)	/* print 2 bytes */
    {
	unicode = (unicode >> 6) | 0xDCE0;	/* the 1st byte */
	ret = print_unicode (max_width);
	if ((int)ret < 0)
	    return ret;
	max_width -= (unsigned char)ret;
	unicode = (tmpcode & 0x3F) | 0xDC80;	/* the 2nd byte */
    }
    else // byte_SN == 1
	unicode |= 0xDCC0 | ((pending - 1) << 5);
    return print_unicode (max_width) + (unsigned char)ret;
}

/* Print ch on the screen.  Handle any needed scrolling or the like */
unsigned int graphics_putchar (unsigned int ch, unsigned int max_width);
unsigned int
graphics_putchar (unsigned int ch, unsigned int max_width)
{
	unsigned int ret;
	invalid = 0;

	if (current_term != term_table)
	{
    if (fontx >= current_term->chars_per_line)
        { fontx = 0; check_scroll (); }

    if (graphics_mode <= 0xFF)
			return 0;

    if (cursor_state & 1)
	graphics_CURSOR(0);

    if ((char)ch == '\n')
    {
	fontx = 0;
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
	}
	
    if ((unsigned char)ch > 0x7F)	/* multi-byte */
	goto multibyte;

    if (pending)	/* print (byte_SN) invalid bytes */
    {
	ret = print_invalid_pending_bytes (max_width);
	pending = 0;	/* end the utf8 byte sequence */
	if ((int)ret < (int)0)
	    return ret;
	max_width -= (unsigned char)ret;
    }

    if (! max_width)
	return (1 << 31);	/* printed width = 0 */


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
	    ret = print_invalid_pending_bytes (max_width);;
	    if ((int)ret < (int)0)
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
	    if ((int)ret < (int)0)
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
	if ((int)ret < (int)0)
	    return ret;
	max_width -= (unsigned char)ret;
    }

    unicode = 0xDC00 | (unsigned char)ch; /* the invalid code point */
    invalid = (1 << 15);	/* set the error bit for return */
    pending = 0;	/* end the utf8 byte sequence */
    /* print the current invalid byte */
    return print_unicode (max_width);
}

/* get the current location of the cursor */
int graphics_getxy(void);
int
graphics_getxy(void)
{
    return (fonty << 8) | fontx;
}

void graphics_gotoxy (int x, int y);
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

void graphics_cls (void);
void
graphics_cls (void)
{
	unsigned char *mem,*s1;
    fontx = 0;
    fonty = 0;

    if (graphics_mode <= 0xFF)
			return;

    /* VBE */
	s1 = (unsigned char *)(grub_size_t)current_phys_base;
	unsigned int color = current_color_64bit >> 32;
	unsigned int y,x,z;
	unsigned char *lfb;
	z = current_bytes_per_pixel;
	
	for(y=0;y<current_y_resolution;++y)
	{
		mem = s1;
		for(x=0;x<current_x_resolution;++x)
		{
			if (graphics_mode > 0xff && (splashimage_loaded & 2) && (cursor_state & 2))
			{
				lfb = (unsigned char *)SPLASH_IMAGE + x*current_bytes_per_pixel + y*current_bytes_per_scanline;
				if(current_bits_per_pixel == 32)
					*(unsigned int *)mem = *(unsigned int *)lfb;
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
					*(unsigned int *)mem = color;
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

    if (cursor_state & 1)
	    graphics_CURSOR(1);
    return;
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
		objective = (unsigned char *)(grub_size_t)current_phys_base + x*current_bytes_per_pixel + y*current_bytes_per_scanline + i*current_bytes_per_scanline;
		grub_memmove (objective,source,w*current_bytes_per_pixel);
	}
}

void vbe_fill_color (unsigned int color);

void
vbe_fill_color (unsigned int color)
{
  unsigned int i;
  unsigned char *p;
  
	for (i=0;i<(current_x_resolution*current_y_resolution);i++)
	{
		p = (unsigned char *)IMAGE_BUFFER + 16 + i*current_bytes_per_pixel;
		switch (current_bits_per_pixel)
		{
			case 32:
				*(unsigned int *)p = color;
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
unsigned int delay0, delay1, name_len;
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
			if (((animated_type & 0x0f) && !num) || (animated_type & 0x20 && (console_checkkey () != (int)-1)/* && console_getkey ()*/))
			{
        animated_enable = 0;
        animated_type = 0;;
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

static int read_image_bmp(int type);
static int read_image_bmp(int type)
{
	struct { /* bmih */ 
		unsigned int  biSize; 
		unsigned int  biWidth; 
		unsigned int  biHeight; 
		unsigned short biPlanes; 
		unsigned short biBitCount; 
		unsigned int  biCompression; 
		unsigned int  biSizeImage; 
		unsigned int  biXPelsPerMeter; 
		unsigned int  biYPelsPerMeter; 
		unsigned int  biClrUsed; 
		unsigned int  biClrImportant;
	} __attribute__ ((packed)) bmih;
	unsigned int bftmp,bfbit;
	int x,y;
	unsigned int source = 0;
	unsigned char R,G,B;
	only = 0;
	if (type == 0)
		return 0;
	filepos = 10;

	if (!grub_read((unsigned long long)(grub_size_t)&bftmp,4, GRUB_READ) || ! grub_read((unsigned long long)(grub_size_t)&bmih,sizeof(bmih),GRUB_READ) || bmih.biBitCount < 24)
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
	unsigned char *bmp;
 
  if (use_phys_base == 0)
    splashimage_loaded = (grub_size_t)IMAGE_BUFFER;

	for(y=bmih.biHeight-1;y>=0;--y)
	{
		for(x=0;x<(int)bmih.biWidth;++x)
		{
			grub_read((unsigned long long)(grub_size_t)&bftmp,bfbit, GRUB_READ);
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
			if(y < (int)bmih.biHeight && x < (int)bmih.biWidth)
				if((y+Y_offset) < current_y_resolution && (x+X_offset) < current_x_resolution)
				{
					if (use_phys_base == 0)
						bmp = (unsigned char *)SPLASH_IMAGE+(x+X_offset)*current_bytes_per_pixel+(y+Y_offset)*current_bytes_per_scanline;
					else
						bmp = (unsigned char *)(grub_size_t)current_phys_base+(x+X_offset)*current_bytes_per_pixel+(y+Y_offset)*current_bytes_per_scanline;
					
          B = (bftmp & 0xff0000)>>16;
          G = (bftmp & 0xff00)>>8;
          R = bftmp & 0xff;
					if ((R>=R0) && (R<=R1) && (G>=G0) && (G<=G1) && (B>=B0) && (B<=B1))
					{
						if (background_transparent || (graphic_enable && (graphic_type & 0x80) && !(is_highlight && (graphic_type & 8))))
							source = *(unsigned int *)((unsigned char *)SPLASH_IMAGE+(x+X_offset)*current_bytes_per_pixel+(y+Y_offset)*current_bytes_per_scanline);
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
						*(unsigned int *)bmp = source;
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
//	use_phys_base=0;
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
unsigned int sizei,sizej;
short restart;
static int iclip[1024];
static int	*iclp;
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
	unsigned int color;
	unsigned int source = 0;

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
						lfb = (unsigned char *)(grub_size_t)current_phys_base + (sizei+i+Y_offset)*current_bytes_per_scanline + (sizej+j+X_offset)*current_bytes_per_pixel;
					
					color = (((unsigned int)R)<<16) | (((unsigned int)G)<<8) | (unsigned int)B;
					
					if ((R>=R0) && (R<=R1) && (G>=G0) && (G<=G1) && (B>=B0) && (B<=B1))
					{
						if (background_transparent || (graphic_enable && (graphic_type & 0x80) && !(is_highlight && (graphic_type & 8))))
							source = *(unsigned int *)((unsigned char *)SPLASH_IMAGE + (sizei+i+Y_offset)*current_bytes_per_scanline + (sizej+j+X_offset)*current_bytes_per_pixel);
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
						*(unsigned int *)lfb = source;
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
		grub_memmove64((unsigned long long)(grub_size_t)JPG_FILE,(unsigned long long)(grub_size_t)lp,size);
		len=grub_read((unsigned long long)(grub_size_t)JPG_FILE+size, 0x7e00, GRUB_READ);
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
  
  efi_call_1 (grub_efi_system_table->boot_services->stall, 1);  //微妙    不加延时,莫名其妙的卡住了!

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
	size -= (grub_size_t)(lp - JPG_FILE);

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
      if (use_phys_base == 0)
        splashimage_loaded = (grub_size_t)IMAGE_BUFFER;
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

	if (!(size=grub_read((unsigned long long)(grub_size_t)lp, 0x8000, GRUB_READ)))
		return !printf("Error:Read JPG File\n");

	InitTable();
	if (!(InitTag()))
		return 1;

	Decode();
	background_transparent=0;
//	use_phys_base=0;
	return 2;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int rr, gg, bb;
unsigned int pixel_shift(unsigned int color)
{
	unsigned int r,g,b;

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

static int read_image();
static int read_image()
{
	char buf[16];
#if 0
	if (*splashimage == 1)
		return 1;

	if (!*splashimage)
	{
		splashimage_loaded = 0;
		*splashimage = 1;
		return 1;
	}
#endif
	if (! grub_open(splashimage))
	{
		return 0;
	}

	/* read header */
	grub_read((unsigned long long)(grub_size_t)&buf, 10, 0xedde0d90);
  unsigned short* a = (unsigned short*)buf;
//	if (*(unsigned short*)buf == 0x4d42) /*BMP */
	if (*a == 0x4d42) /*BMP */
	{
		splashimage_loaded |= read_image_bmp(graphics_mode > 0xFF);
	}
//	else if (*(unsigned short*)buf == 0xD8FF)
	else if (*a == 0xD8FF)
	{
		splashimage_loaded |= read_image_jpg(graphics_mode > 0xFF);
	}

//	*splashimage = 1;
	grub_close();
	return splashimage_loaded & 0xf;
}

/* Convert a character which is a hex digit to the appropriate integer */
int hex (int v);
int
hex (int v)
{
/*
   by chenall 2011-12-01.
   '0' & 0xf = 0 ... '9' & 0xf = 9;
   'a' & 0xf = 1 ... 'f' & 0xf = 6;
   'A' & 0xf = 1 ... 'F' & 0xf = 6;
*/
	if (v >= 'A')
		v += 9;
	return v & 0xf;
}


/* scroll the screen */
//void bios_scroll_up();
static void
graphics_scroll (void)
{
    unsigned int i;
    unsigned int old_state = cursor_state;
    cursor_state &= ~1;
#if 0   //滚屏速度太慢，尤其在实体机
  grub_memcpy ((char *)(grub_size_t)current_phys_base, (char *)(grub_size_t)current_phys_base + (current_bytes_per_scanline * (font_h + line_spacing)),
		    (current_term->max_lines - 1) * current_bytes_per_scanline * (font_h + line_spacing));
#else
  static grub_efi_guid_t graphics_output_guid = GRUB_EFI_GOP_GUID;
  static struct grub_efi_gop *gop;
  grub_efi_handle_t *handles;
  grub_efi_uintn_t num_handles;

  handles = grub_efi_locate_handle (GRUB_EFI_BY_PROTOCOL,
				    &graphics_output_guid, NULL, &num_handles);	//定位手柄(通过协议)
  if (!handles || num_handles == 0)	//如果句柄为零, 或者句柄数为零
    return;												  //错误
  gop = grub_efi_open_protocol (handles[0], &graphics_output_guid,
          GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);	//打开协议
  efi_call_10 (gop->blt, gop, (void *)(grub_size_t)current_phys_base,      //没有此函数，在实体机启动时，滚动的是固件的内容！
          GRUB_EFI_BLT_BUFFER_TO_VIDEO, 0, 0, 0, 0,
          current_x_resolution, current_y_resolution,
          0);
  efi_call_10 (gop->blt, gop, 0,
          GRUB_EFI_BLT_VIDEO_TO_VIDEO, 0, font_h, 0, 0,
          current_x_resolution, current_y_resolution - font_h,
          0);
#endif
	if (old_state & 1)
		scroll_state = 1;		//避免空格背景杂乱无章
    for (i=0;i<current_term->chars_per_line;++i)
	graphics_putchar(' ',1);	
    gotoxy(0,fonty);
    cursor_state = old_state;
	scroll_state = 0;
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
	lfb = (unsigned char *)(grub_size_t)(current_phys_base + top * current_bytes_per_scanline + left * z);

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
				*(unsigned int *)(p+y) = *(unsigned int *)p = (unsigned int)current_color_64bit;
			}
			else
				*(unsigned short *)(p+y) = *(unsigned short *)p = (unsigned short)pixel_shift((unsigned int)current_color_64bit);
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
				*(unsigned int *)(p+y) = *(unsigned int *)p = (unsigned int)current_color_64bit;
			}
			else
				*(unsigned short *)(p+y) = *(unsigned short *)p = (unsigned short)pixel_shift((unsigned int)current_color_64bit);
			p += current_bytes_per_scanline;
		}
	}
	return;
}

static void
vbe_cursor (int set)		//VBE的光标
{
    unsigned int x, y,j;

	x = fontx * (font_w+font_spacing);
	y = fonty * (font_h+line_spacing);
	y += line_spacing>>1;
    /* invert the beginning 1 vertical lines of the char */
	for (j = 2; j < font_h - 2; ++j)
	{
	    XorPixel (x, y + j, -1);
	}
}

#endif /* SUPPORT_GRAPHICS */
