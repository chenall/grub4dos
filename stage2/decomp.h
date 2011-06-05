/* decomp.h - abstract decompression interface */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2004  Free Software Foundation, Inc.
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

#ifndef ASM_FILE

struct decomp_entry
{
  char *name;
  int (*open_func) (void);
  void (*close_func) (void);
  unsigned long long (*read_func) (unsigned long long buf, unsigned long long len, unsigned long write);
};

#define NUM_DECOM 2

extern struct decomp_entry decomp_table[NUM_DECOM];
extern int decomp_type;

#endif
