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

extern void toggle_blinking (void);
static int console_standard_color = A_NORMAL;
static int console_normal_color = A_NORMAL;
static int console_highlight_color = A_REVERSE;
static int console_helptext_color = A_NORMAL;
static int console_heading_color = A_NORMAL;
static color_state console_color_state = COLOR_STATE_STANDARD;

static unsigned long long console_standard_color_64bit = 0xAAAAAA;
static unsigned long long console_normal_color_64bit = 0xAAAAAA;
static unsigned long long console_highlight_color_64bit = 0xAAAAAA00000000ULL;
static unsigned long long console_helptext_color_64bit = 0xAAAAAA;
static unsigned long long console_heading_color_64bit = 0xAAAAAA;

void
console_setcolorstate (color_state state)
{
  switch (state) {
    case COLOR_STATE_STANDARD:
      current_color = console_standard_color;
      current_color_64bit = console_standard_color_64bit;
      break;
    case COLOR_STATE_NORMAL:
      current_color = console_normal_color;
      current_color_64bit = console_normal_color_64bit;
      break;
    case COLOR_STATE_HIGHLIGHT:
      current_color = console_highlight_color;
      current_color_64bit = console_highlight_color_64bit;
      break;
    case COLOR_STATE_HELPTEXT:
      current_color = console_helptext_color;
      current_color_64bit = console_helptext_color_64bit;
      break;
    case COLOR_STATE_HEADING:
      current_color = console_heading_color;
      current_color_64bit = console_heading_color_64bit;
      break;
    default:
      current_color = console_standard_color;
      current_color_64bit = console_standard_color_64bit;
      break;
  }

  console_color_state = state;
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
console_setcolor (unsigned long long normal_color, unsigned long long highlight_color, unsigned long long helptext_color, unsigned long long heading_color)
{
  if ((normal_color | highlight_color | helptext_color | heading_color) >> 8)
	goto color_64bit;
  console_normal_color = normal_color;
  console_highlight_color = highlight_color;
  console_helptext_color = helptext_color;
  console_heading_color = heading_color;

  console_setcolorstate (console_color_state);
  if (current_term == term_table)	/* console */
	toggle_blinking ();

  /* translate to 64-bit colors */
  console_normal_color_64bit = color_8_to_64 (normal_color);
  console_highlight_color_64bit = color_8_to_64 (highlight_color);
  console_helptext_color_64bit = color_8_to_64 (helptext_color);
  console_heading_color_64bit = color_8_to_64 (heading_color);
  return;

color_64bit:

  /* 64-bit color has foreground in low DWORD and background in high DWORD */

  console_normal_color_64bit = normal_color;
  console_highlight_color_64bit = highlight_color;
  console_helptext_color_64bit = helptext_color;
  console_heading_color_64bit = heading_color;

  console_setcolorstate (console_color_state);
}
