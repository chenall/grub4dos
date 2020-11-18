/* iamath.h - inline asm mathematic functions */
/*
 *  GRUB4DOS
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004  Free Software Foundation, Inc.
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
#ifndef __IAMATH_H_INCLUDED
#define __IAMATH_H_INCLUDED

//#define IAMATH_NO_ASM

#ifndef IAMATH_NO_ASM
// return bit index of most significant 1 bit, undefined if all bits are 0.
static inline __attribute__((always_inline))
unsigned int log2_tmp(unsigned int n)
{
  unsigned int i;
  asm("bsrl %1,%0":"=r"(i):"rm"(n));
  return i;
}

static inline __attribute__((always_inline))
unsigned long long divmodu64u32(unsigned long long num, unsigned int divisor, unsigned int *p_remainder)
{
  unsigned long long qhi,qlo,r;
  asm("divl %6; xchg %eax,%ecx; divl %6; "
    :"=d"(r),"=a"(qlo),"=c"(qhi)
    :"0"(0),"1"((unsigned int)(num>>32)),"2"((unsigned int)num),"rm"(divisor)
    );
  if (p_remainder) *p_remainder = r;
  return ((unsigned long long)qhi<<32)|qlo;
}

#else
unsigned int log2_tmp(unsigned int n);

#ifdef IAMATH_NO_ASM_DEFINITION
unsigned int log2_tmp(unsigned int n)
{
    unsigned int a,b;
    unsigned int i;
    i=0; a=n;
    b=a>>16; if (b) { a=b; i+=16; }
    b=a>> 8; if (b) { a=b; i+= 8; }
    b=a>> 4; if (b) { a=b; i+= 4; }
    b=a>> 2; if (b) { a=b; i+= 2; }
    b=a>> 1; if (b) { a=b; i+= 1; }
    return i;
}
#endif // IAMATH_NO_ASM_DEFINITION
#endif // IAMATH_NO_ASM

#endif // !__IAMATH_H_INCLUDED
