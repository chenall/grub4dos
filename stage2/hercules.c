/* hercules.c - hercules console interface */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2001,2002  Free Software Foundation, Inc.
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

#ifdef SUPPORT_HERCULES

#include <shared.h>
#include <hercules.h>
#include <term.h>

/* The position of the cursor.  */
static int herc_x;
static int herc_y;

extern int current_color;
static int herc_cursor_state = 1;

/* Write a byte to a port.  */
static inline void
outb (unsigned short port, unsigned char value)
{
  asm volatile ("outb	%b0, %w1" : : "a" (value), "Nd" (port));
}

static void
herc_set_cursor (void)
{
  unsigned offset = herc_y * HERCULES_WIDTH + herc_x;
  
  outb (HERCULES_INDEX_REG, 0x0f);
  outb (0x80, 0);
  outb (HERCULES_DATA_REG, offset & 0xFF);
  outb (0x80, 0);
  
  outb (HERCULES_INDEX_REG, 0x0e);
  outb (0x80, 0);
  outb (HERCULES_DATA_REG, offset >> 8);
  outb (0x80, 0);
}

void
hercules_putchar (unsigned int c)
{
  switch (c)
    {
    case '\b':
      if (herc_x > 0)
	herc_x--;
      break;
      
    case '\n':
      herc_y++;
      break;
      
    case '\r':
      herc_x = 0;
      break;

    case '\a':
      break;

    default:
      {
	volatile unsigned short *video
	  = (unsigned short *) HERCULES_VIDEO_ADDR;
	
	video[herc_y * HERCULES_WIDTH + herc_x]
	  = (current_color << 8) | c;
	herc_x++;
	if (herc_x >= HERCULES_WIDTH)
	  {
	    herc_x = 0;
	    herc_y++;
	  }
      }
      break;
    }

  if (herc_y >= HERCULES_HEIGHT)
    {
      volatile unsigned long *video = (unsigned long *) HERCULES_VIDEO_ADDR;
      int i;
      
      herc_y = HERCULES_HEIGHT - 1;
      grub_memmove ((char *) HERCULES_VIDEO_ADDR,
		    (char *) HERCULES_VIDEO_ADDR + HERCULES_WIDTH * 2,
		    HERCULES_WIDTH * (HERCULES_HEIGHT - 1) * 2);
      for (i = HERCULES_WIDTH * (HERCULES_HEIGHT - 1) / 2;
	   i < HERCULES_WIDTH * HERCULES_HEIGHT / 2;
	   i++)
	video[i] = 0x07200720;
    }
}

void
hercules_cls (void)
{
  int i;
  volatile unsigned long *video = (unsigned long *) HERCULES_VIDEO_ADDR;
  
  for (i = 0; i < HERCULES_WIDTH * HERCULES_HEIGHT / 2; i++)
    video[i] = 0x07200720;

  herc_x = herc_y = 0;
  herc_set_cursor ();
}

int
hercules_getxy (void)
{
  return (herc_x << 8) | herc_y;
}

void
hercules_gotoxy (int x, int y)
{
  herc_x = x;
  herc_y = y;
  herc_set_cursor ();
}

int
hercules_setcursor (int on)
{
  int old_state = herc_cursor_state;
  
  outb (HERCULES_INDEX_REG, 0x0a);
  outb (0x80, 0);
  outb (HERCULES_DATA_REG, on ? 0 : (1 << 5));
  outb (0x80, 0);
  herc_cursor_state = on;

  return old_state;
}

#endif /* SUPPORT_HERCULES */
