/* term_console.c - console input and output */
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

#include <shared.h>
#include <term.h>

/* These functions are defined in asm.S instead of this file:
   console_putchar, console_checkkey, console_getkey, console_getxy,
   console_gotoxy, console_cls, and console_nocursor.  */

//extern void toggle_blinking (void);
int console_color[COLOR_STATE_MAX] = {
  [COLOR_STATE_STANDARD] = A_NORMAL,
  /* represents the user defined colors for normal text */
  [COLOR_STATE_NORMAL] = A_NORMAL,
  /* represents the user defined colors for highlighted text */
  [COLOR_STATE_HIGHLIGHT] = A_REVERSE,
  /* represents the user defined colors for help text */
  [COLOR_STATE_HELPTEXT] = A_NORMAL,
  /* represents the user defined colors for heading line */
  [COLOR_STATE_HEADING] = A_NORMAL,
  /* represents the user defined colors for border */
  [COLOR_STATE_BORDER] = A_NORMAL
};

unsigned long long console_color_64bit[COLOR_STATE_MAX] = {
  [COLOR_STATE_STANDARD] = 0xAAAAAA,
  /* represents the user defined colors for normal text */
  [COLOR_STATE_NORMAL] = 0xAAAAAA,
  /* represents the user defined colors for highlighted text */
  [COLOR_STATE_HIGHLIGHT] = 0xFFFFFF,								//0xAAAAAA00000000ULL,
  /* represents the user defined colors for help text */
  [COLOR_STATE_HELPTEXT] = 0xAAAAAA,
  /* represents the user defined colors for heading line */
  [COLOR_STATE_HEADING] = 0xAAAAAA,
  /* represents the user defined colors for notes */
  [COLOR_STATE_BORDER] = 0x3399

};


void
console_setcolorstate (color_state state)
{
	if (state >= COLOR_STATE_MAX)
		state = COLOR_STATE_STANDARD;
	current_color = console_color[state];
	current_color_64bit = console_color_64bit[state];
	if (state == COLOR_STATE_BORDER)
	{
		current_color &= 0xf;
		current_color |= console_color[COLOR_STATE_NORMAL] & 0xf0;
		current_color_64bit &= 0xffffff;
		current_color_64bit |= console_color_64bit[COLOR_STATE_NORMAL] & 0xffffff00000000;
	}
}

unsigned char color_64_to_8 (unsigned long long color64);
unsigned char color_32_to_4 (unsigned long color32);
unsigned char
color_32_to_4 (unsigned long color32)
{
  unsigned char r, g, b, col32, col4=0;
	r = color32 >> 16;
	g = (color32 >> 8) & 0xff;
	b = color32 & 0xff;
	
	if (r >= g)
	{
		if (r >= b)
			col32 = r;
		else
			col32 = b;
	}
	else
	{
		if (g >= b)
			col32 = g;
		else
			col32 = b;
	}
	
	if (col32 > 0xaa)
		col4 |= 8;
	if (r > col32/2)
		col4 |= 4;
	if (g > col32/2)
		col4 |= 2;
	if (b > col32/2)
		col4 |= 1;

	return col4;
}

unsigned char
color_64_to_8 (unsigned long long color64)
{
    return (color_32_to_4 (color64 >> 32) << 4) | color_32_to_4 (color64 & 0xffffff);
}


unsigned long long
color_4_to_32 (unsigned char color4)
{
    switch (color4)
    {
	case 0x00: return 0;
	case 0x01: return 0x0000AA;
	case 0x02: return 0x00AA00;
	case 0x03: return 0x00AAAA;
	case 0x04: return 0xAA0000;
	case 0x05: return 0xAA00AA;
	case 0x06: return 0xAA5500;
	case 0x07: return 0xAAAAAA;
	case 0x08: return 0x555555;
	case 0x09: return 0x5555FF;
	case 0x0A: return 0x55FF55;
	case 0x0B: return 0x55FFFF;
	case 0x0C: return 0xFF5555;
	case 0x0D: return 0xFF55FF;
	case 0x0E: return 0xFFFF55;
	case 0x0F: return 0xFFFFFF;
	default: return 0;
    }
}

unsigned long long
color_8_to_64 (unsigned char color8)
{
    return (color_4_to_32 (color8 >> 4) << 32) | color_4_to_32 (color8 & 15);
}

void
console_setcolor(unsigned long state,unsigned long long color[])
{
	int i;
	for(i=0;i< COLOR_STATE_MAX;++i)
	{
		if (!(state & (1<<i)))
			continue;
		console_color[i] = color_64_to_8(color[i]);
		console_color_64bit[i] = color[i];
	}
//	if (current_term == term_table)	/* console */
//		toggle_blinking ();
	console_setcolorstate(COLOR_STATE_STANDARD);
}
