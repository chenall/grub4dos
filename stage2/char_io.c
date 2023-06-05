/* char_io.c - basic console input and output */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2004  Free Software Foundation, Inc.
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

#ifdef SUPPORT_HERCULES
# include <hercules.h>
#endif

#ifdef SUPPORT_SERIAL
# include <serial.h>
#endif

struct term_entry term_table[] =
  {
    {
      "console",
      0,
      80,
      25,
      console_putchar,
      console_checkkey,
      console_getkey,
      console_getxy,
      console_gotoxy,
      console_cls,
      console_setcolorstate,
      console_setcolor,
      console_setcursor,
      0,
      0
    },
#ifdef SUPPORT_GRAPHICS
    { "graphics",
      0/*TERM_NEED_INIT*/, /* flags */
      80,
      30, /* number of lines */
      graphics_putchar, /* putchar */
      console_checkkey, /* checkkey */
      console_getkey, /* getkey */
      graphics_getxy, /* getxy */
      graphics_gotoxy, /* gotoxy */
      graphics_cls, /* cls */
      console_setcolorstate, // graphics_setcolorstate, /* setcolorstate */
      console_setcolor, // graphics_setcolor, /* setcolor */
      0/*graphics_setcursor*/, /* nocursor */
      graphics_init, /* initialize */
      graphics_end /* shutdown */
    },
#endif /* SUPPORT_GRAPHICS */
#ifdef SUPPORT_SERIAL
    {
      "serial",
      /* A serial device must be initialized.  */
      TERM_NEED_INIT,
      80,
      25,
      serial_putchar,
      serial_checkkey,
      serial_getkey,
      serial_getxy,
      serial_gotoxy,
      serial_cls,
      serial_setcolorstate,
      0,
      0,
      0,
      0
    },
#endif /* SUPPORT_SERIAL */
#ifdef SUPPORT_HERCULES
    {
      "hercules",
      0,
      80,
      25,
      hercules_putchar,
      console_checkkey,
      console_getkey,
      hercules_getxy,
      hercules_gotoxy,
      hercules_cls,
      console_setcolorstate,
      console_setcolor,
      hercules_setcursor,
      0,
      0
    },      
#endif /* SUPPORT_HERCULES */
    /* This must be the last entry.  */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
  };

int count_lines = -1;
int use_pager = 1;
void
print_error (void)
{
  if (errnum > ERR_NONE && errnum < MAX_ERR_NUM)
    grub_printf ("\nError %u:(http://grub4dos.chenall.net/e/%u)\n\t %s\n", errnum, errnum, err_list[errnum]);
}

char *
//convert_to_ascii (char *buf, int c,...)
convert_to_ascii (char *buf, int c, int lo, int hi) //适应gcc高版本  2023-05-24
{
  union {
    unsigned long long ll;
    struct {
	unsigned long lo;
	unsigned long hi;
    };
    struct {
	unsigned char l1;
	unsigned char l2;
	unsigned char l3;
	unsigned char l4;
	unsigned char h1;
	unsigned char h2;
	unsigned char h3;
	unsigned char h4;
    };
  } num;
  char *ptr = buf;

//  num.hi = *(unsigned long *)((&c) + 2);
//  num.lo = *(unsigned long *)((&c) + 1);
  num.hi = hi; //适应gcc高版本  2023-05-24
  num.lo = lo; //适应gcc高版本  2023-05-24

  if (c == 'x' || c == 'X')	/* hex */
  {
    do {
	int dig = num.l1 & 0xF;
	*(ptr++) = ((dig > 9) ? dig + c - 33 : '0' + dig);
    } while (num.ll >>= 4);
  }
  else				/* decimal */
  {
    if ((num.h4 & 0x80) && c == 'd')
    {
	//num.ll = (~num.ll) + 1;
	num.ll = - num.ll;
	*(ptr++) = '-';
	buf++;
    }

    do {
	unsigned long H0, H1, L0, L1;

	/* 0x100000000 == 4294967296 */
	/* num.ll == (H1 * 10 + H0) * 0x100000000 + L1 * 10 + L0 */
	H0 = num.hi % 10;
	H1 = num.hi / 10;
	L0 = num.lo % 10;
	L1 = num.lo / 10;
	/* num.ll == H1 * 10 * 0x100000000 + H0 * 0x100000000 + L1 * 10 + L0 */
	/* num.ll == H1 * 10 * 0x100000000 + H0 * 4294967290 + H0 * 6 + L1 * 10 + L0 */
	L0 += H0 * 6;
	L1 += L0 / 10;
	L0 %= 10;
	/* num.ll == H1 * 10 * 0x100000000 + H0 * 4294967290 + L1 * 10 + L0 */
	/* num.ll == (H1 * 0x100000000 + H0 * 429496729 + L1) * 10 + L0 */
	/* quo = (H1 * 0x100000000 + H0 * 429496729 + L1) */
	/* rem = L0 */
	num.hi = H1;
	num.lo = H0;
	num.lo *= 429496729UL;
	num.ll += L1;
	*(ptr++) = '0' + L0;
    } while (num.ll);
  }
  /* reorder to correct direction!! */
  {
    char *ptr1 = ptr - 1;
    char *ptr2 = buf;
    while (ptr1 > ptr2)
      {
	int tmp = *ptr1;
	*ptr1 = *ptr2;
	*ptr2 = tmp;
	ptr1--;
	ptr2++;
      }
  }

  return ptr;
}

void
grub_putstr (const char *str)
{
  while (*str)
    grub_putchar ((unsigned char)*str++, 255);
}

#if 0
/* (Patch from Jamey Sharp, 26 Jan 2009)
 * Replace grub_printf calls with grub_sprintf by #define, not asm magic.
 * define in shared.h:
 * 	#define grub_printf(...) grub_sprintf(NULL, __VA_ARGS__)
 */
//static int grub_printf_return_address;
//void
//grub_printf (const char *format, ...)
//{
//	/* sorry! this does not work :-( */
//	//return grub_sprintf (NULL, format,...);
//#if 1
//  asm volatile ("popl %ebp");	/* restore EBP */
//  //asm volatile ("ret");
//  asm volatile ("popl %0" : "=m"(grub_printf_return_address));
//  asm volatile ("pushl $0");	/* buffer = 0 for grub_sprintf */
//#ifdef HAVE_ASM_USCORE
//  asm volatile ("call _grub_sprintf");
//#else
//  asm volatile ("call grub_sprintf");
//#endif
//  asm volatile ("popl %eax");
//  asm volatile ("pushl %0" : : "m"(grub_printf_return_address));
//  asm volatile ("ret");
//#else
//  int *dataptr = (int *)(void *) &format;
//
//  dataptr--;	/* (*dataptr) is return address */
//  grub_printf_return_address = (*dataptr);	/* save return address */
//
//  asm volatile ("leave");	/* restore ESP and EBP */
//  //asm volatile ("ret");
//  asm volatile ("popl %eax");	/* discard return address */
//  asm volatile ("pushl $0");	/* buffer = 0 for grub_sprintf */
//  asm volatile ("call grub_sprintf");
//  asm volatile ("popl %eax");
//  asm volatile ("pushl %0" : : "m"(grub_printf_return_address));
//  asm volatile ("ret");
//#endif
//}
#endif

int
grub_sprintf (char *buffer, const char *format, ...)
{

  /* Call with buffer==NULL, and it will just printf().  */

  /* XXX hohmuth
     ugly hack -- should unify with printf() */
  unsigned long *dataptr = (unsigned long *)(((int *)(void *) &format) + 1);
  //unsigned long *dataptr = (unsigned long *)(((unsigned int *)(void *) &buffer));
//  char c, *ptr, str[32];
  unsigned char str[32];
  unsigned char pad;
  const unsigned char *ptr;
  unsigned char *bp = (unsigned char *)buffer;
  int width;
  unsigned int length;
  int align;
  unsigned int accuracy;
  unsigned char *putchar_hook_back=NULL;
  int stdout = 1;
  if (buffer && (grub_u32_t)buffer <= 3)
  {
     if (!debug_msg) return 1;
     if ((grub_u32_t)buffer != 3 && debug < (int)buffer)
        return 1;

     bp=NULL,buffer=NULL;//reset buffer and bp to NULL

     if (debug_msg < (grub_u32_t)buffer)
     {
       stdout = 0;
       putchar_hook_back = set_putchar_hook((grub_u8_t*)0);
     }
  }
  //dataptr++;
  //dataptr++;
#if 1
  while (*format)
  {
    if (*format == '%')
    {
	pad = ' ';
	width = 0;
	length = 0;
	accuracy = -1;
	align = 0;
	ptr = (const unsigned char *)format++;
	if (*format == '-')
	{
		++align,++format;
	}
	if (*format == '0')
	{
		pad = *format++;
	}
	if (*format == '*')
	{
		++format;
		width=*(dataptr++);
	}
	else
	{
		while (*format >= '0' && *format <= '9')
			width = width * 10 + *(format++) - '0';
	}
	if (*format == '.')
	{
		++format;
		if (*format == '*')
		{
			++format;
			accuracy = *(dataptr++);
		}
		else
		{
			accuracy = 0;
			while (*format >= '0' && *format <= '9')
				accuracy = accuracy * 10 + *(format++) - '0';
		}
	}

	if (*format == 'l')
	{
		++length,++format;
	}

	switch(*format)
	{
		case '%':
			ptr = (const unsigned char *)format;
			accuracy = 1;
			--width;
			break;
		case 'c':
			accuracy = 1;
			--width;
			ptr = (unsigned char *)dataptr++;
			break;
		case 's':
			ptr = (unsigned char *)(unsigned int) (*(dataptr++));
			if (ptr)
			{
				length = grub_strlen((char *)ptr);
				width -= (length > accuracy)?accuracy:length;
			}
			else
				accuracy = 0;
			break;
		case 'd': case 'x':	case 'X':  case 'u':
			{
				int lo, hi;

				lo = *(dataptr++);
				hi = (length ? (*(dataptr++)) : ((*format == 'd' && lo<0)?-1:0));
				*convert_to_ascii ((char *)str, *format, lo, hi) = 0;
			}
			accuracy = grub_strlen ((char *)str);
			width -= accuracy;
			ptr = str;
			break;
		default:
			format = (char *)ptr;
			goto next_c;
	}
	if (align == 0)
	{
		for(;width>0;--width)
		{
			if (buffer)
				*bp = pad; /* putchar(pad); */
			else
				grub_putchar (pad, 255);
			++bp;
		}
	}
	while (*ptr && accuracy)
	{
		if (buffer)
			*bp = *ptr;
		else
			grub_putchar (*ptr, accuracy);
		++bp,++ptr;
		--accuracy;
	}
	if (align)
	{
		for(;width>0;--width)
		{
			if (buffer)
				*bp = pad; /* putchar(pad); */
			else
				grub_putchar (pad, 255);
			++bp;
		}
	}
    }
    else
    {
    next_c:
	if (buffer)
		*bp = *format;
	else
		grub_putchar((unsigned char)*format, 255);
	++bp;
    } /* if */
    ++format;
  } /* while */
#else
  while ((c = *(format++)) != 0)
    {
      if (c != '%')
      {
	if (buffer)
	  *bp++ = c; /* putchar(c); */
	else
	{
	  grub_putchar (c, 255);
	  bp++;
	}
      }
      else
      {
	pad = ' ';
	width = 0;
	length = 0;
	accuracy = -1;

get_next_c:
	c = *(format++);

find_specifier:
	switch (c)
	{
		case '%':
			if (buffer)
				*bp = c;
			else
				grub_putchar(c, 255);
			bp++;
			break;
		case '.':
			accuracy = 0;
			while ((c = *(format++)) >= '0' && c <= '9')
				accuracy = accuracy * 10 + c - '0';
			goto find_specifier;
	  case 'd': case 'x':	case 'X':  case 'u':
	    {
		unsigned int lo, hi;

		lo = *(dataptr++);
		hi = (length ? (*(dataptr++)) : 0);
		*convert_to_ascii (str, c, lo, hi) = 0;
	    }
	    //dataptr++;
	    width -= grub_strlen (str);
	    if (width > 0)
	      {
		while(width--)
		    if (buffer)
			*bp++ = pad; /* putchar(pad); */
		    else
		    {
			grub_putchar (pad, 255);
			bp++;
		    }
	      }
	    ptr = str;
	    if (buffer)
	    {
		while (*ptr)
			*bp++ = *(ptr++); /* putchar(*(ptr++)); */
	    } else {
		while (*ptr)
		{
			grub_putchar (*(ptr++), 255);
			bp++;
		}
	    }
	    break;

	  case 'c':
	    if (length)
		break;		/* invalid */
	    if (width > 0)
	      {
		while(--width)
		    if (buffer)
			*bp++ = pad; /* putchar(pad); */
		    else
		    {
			grub_putchar (pad, 255);
			bp++;
		    }
	      }
	    if (buffer)
	    {
		*bp++ = (*(char *)(dataptr++)) /*& 0xff*/;
	    } else {
		grub_putchar ((*(char *)(dataptr++)) /*& 0xff*/, 255);
		bp++;
	    }
	    //dataptr++;
	    break;

	  case 's':
	    if (length)
		break;		/* invalid */
	    width -= grub_strlen ((char *) (unsigned int) *(dataptr));
	    if (width > 0)
	      {
		while(width--)
		    if (buffer)
			*bp++ = pad; /* putchar(pad); */
		    else
		    {
			grub_putchar (pad, 255);
			bp++;
		    }
	      }
	    ptr = (char *)(unsigned int) (*(dataptr++));
	    //dataptr++;
	    while ((c = *(ptr++)) && accuracy--)
	    {
	    	if (buffer)
	    		*bp = c;
	    	else
	    		grub_putchar (c, 255);
	    	bp++;
	    }
	    break;
	  case 'l':
	    if (length)
		break;		/* invalid */
	    length++;
	    //c = *(format++);	/* should be one of d, x, X, u */
	    goto get_next_c;
	  case '0':
	    if (length)
		break;		/* invalid */
	    pad = '0';
	  case '1' ... '9':
	    if (length)
		break;		/* invalid */
	    width = c - '0';
	    while ((c = *(format++)) >= '0' && c <= '9')
		width = width * 10 + c - '0';
	    goto find_specifier;
	  } /* switch */
       } /* if */
    } /* while */
#endif
  if (buffer)
	*bp = 0;
  if (stdout == 0)
	set_putchar_hook(putchar_hook_back);

  return bp - (unsigned char *)buffer;
}


#include "grub4dos_version.h"

#ifdef GRUB4DOS_VERSION

void
init_page (void)
{
  //int i;
  unsigned char tmp_buf[128];
  //unsigned char ch = ' ';

//  cls ();

	if(cursor_state==2)
	{
		if (current_term->setcolorstate)
      current_term->setcolorstate (COLOR_STATE_HEADING);
	}else
	{
		if (current_term->setcolorstate)
      current_term->setcolorstate (COLOR_STATE_STANDARD);
	}

  grub_sprintf ((char *)tmp_buf,
		" GRUB4DOS " GRUB4DOS_VERSION ", Mem: %dK/%dM/%ldM, End: %X",
		(unsigned long)saved_mem_lower,
		(unsigned long)(saved_mem_upper >> 10),
		(unsigned long long)(saved_mem_higher >> 10),
		(unsigned int)(((char *) init_free_mem_start) + 1024 + 256 * sizeof (char *) + config_len));
	grub_printf("%-*.*s",current_term->chars_per_line,current_term->chars_per_line,tmp_buf);
  if (current_term->setcolorstate)
      current_term->setcolorstate (COLOR_STATE_STANDARD);
}

#else

void
init_page (void)
{
//  cls ();

  grub_printf ("GNU GRUB  version %s  (%dK lower / %dK upper memory)\n",
	  version_string, saved_mem_lower, saved_mem_upper);
}

#endif

/* The number of the history entries.  */
static int num_history = 0;

/* Get the NOth history. If NO is less than zero or greater than or
   equal to NUM_HISTORY, return NULL. Otherwise return a valid string.  */
static char *
get_history (int no)
{
  int j;
  char *p = (char *) HISTORY_BUF;
  if (no < 0 || no >= num_history)
	return 0;
  /* get history NO */
  for (j = 0; j < no; j++)
  {
	p += *(unsigned short *)p;
	if (p > (char *) HISTORY_BUF + MAX_CMDLINE * HISTORY_SIZE)
	{
		num_history = j;
		return 0;
	}
  }

  return p + 2;
}

/* Add CMDLINE to the history buffer.  */
static void
add_history (const char *cmdline, int no)
{
  int j, len;
  char *p = (char *) HISTORY_BUF;
  /* get history NO */
  for (j = 0; j < no; j++)
  {
	p += *(unsigned short *)p;
	if (p > (char *) HISTORY_BUF + MAX_CMDLINE * HISTORY_SIZE)
		return;
  }
  /* get cmdline length */
  len = grub_strlen (cmdline) + 3;
  if (((char *) HISTORY_BUF + MAX_CMDLINE * HISTORY_SIZE) > (p + len))
	grub_memmove (p + len, p, ((char *) HISTORY_BUF + MAX_CMDLINE * HISTORY_SIZE) - (p + len));
  *(unsigned short *)p = len;
  grub_strcpy (p + 2, cmdline);
  if (num_history < 0x7FFFFFFF)
	num_history++;
}

/* XXX: These should be defined in shared.h, but I leave these here,
	until this code is freezed.  */
#define CMDLINE_WIDTH	(current_term->chars_per_line - 2)
#define CMDLINE_MARGIN	11

struct get_cmdline_arg get_cmdline_str;
static int xpos, lpos, section;

/* The length of PROMPT.  */
static int plen;
/* The length of the command-line.  */
static int llen;
/* The working buffer for the command-line.  */
static unsigned char *buf;

static void cl_refresh (int full, int len);
static void cl_backward (int count);
static void cl_forward (int count);
static void cl_insert (const char *str);
static void cl_delete (int count);
  
/* Move the cursor backward.  */
static void cl_backward (int count)
{
	unsigned long unicode = 0;
	unsigned char b = *(buf + lpos - 1);
	unsigned char a = *(buf + lpos - 2);
	//unsigned long count = 1;

	/* get the length of the current sequence */
	#ifdef SUPPORT_GRAPHICS
	if (b < 0x80 || !graphics_inited)
	    goto do_backward;	/* backward 1 ASCII byte */
	if ((b & 0x40))		/* not continuation byte 10xxxxxx */
	    goto do_backward;	/* backward 1 invalid byte */
	/* now b is continuation byte */
	if (! (a & 0x80))
	    goto do_backward;	/* backward 1 invalid byte */

	if (! (a & 0x40))	/* a is 10xxxxxx */
	{
	    if (((*(buf + lpos - 3)) >> 4) != 0xE)
		goto do_backward;	/* backward 1 invalid byte */
	    count = 3;		/* backward 3 bytes for the valid sequence */
	    unicode = (buf[lpos - 3] << 6) | (a & 0x3F);
	    unicode = (unicode << 6) | (b & 0x3F);
	}
	else			/* a is 11xxxxxx */
	{
	    if ((a & 0x20))	/* a is not 110xxxxx */
		goto do_backward;	/* backward 1 invalid byte */
	    /* backward 2 bytes for the valid sequence */
	    count++;		// count = 2;
	    unicode = (a << 6) | (b & 0x3F);
	}
	#endif
do_backward:

	if (lpos < count)
	{
		/* count > 1, but exceeding the buffer, so it is invalid. */
		count = 1;	/* backward 1 invalid byte */
	}

      lpos -= count;
      
      /* If the cursor is in the first section, display the first section
	 instead of the second.  */
      if (section == 1 && plen + lpos < CMDLINE_WIDTH)
	cl_refresh (1, 0);
      else if (xpos - count < 1)
	cl_refresh (1, 0);
      else
	{
	  if (current_term->flags & TERM_DUMB)
	    {
	      int i;
	      
	      xpos -= count;

	      for (i = 0; i < count; i++)
		grub_putchar ('\b', 255);
	    }
	  else
	    {
	      if (count > 1)
	      {
		/* get char width */
		count = 2;	/* initalize as wide char */
//		if (*(unsigned long *)(UNIFONT_START + (unicode << 5)) == narrow_char_indicator)
		if (((*(unsigned char *)((unsigned int)narrow_mem + unicode/8)) & (unsigned char)(1 << (unicode&7))) == 0)		//宽字符指示器使用内存分配		2023-02-22
			count--;	// count = 1;
	      }
	      xpos -= count;
	      gotoxy (xpos, fonty);
	    }
	}
}

/* Move the cursor forward.  */
static void cl_forward (int count)
{
	unsigned long unicode = 0;
	unsigned char b = *(buf + lpos);
	//unsigned long count = 1;

	/* get the length of the current sequence */
	#ifdef SUPPORT_GRAPHICS
	if (b < 0x80 || !graphics_inited)
	    goto do_forward;	/* forward 1 ASCII byte */
	if (! (b & 0x40))	/* continuation byte 10xxxxxx */
	    goto do_forward;	/* forward 1 invalid continuation byte */
	if (! (b & 0x20))	/* leading byte 110xxxxx */
	{
	    /* check the next 1 continuation byte */
	    if ((buf[lpos + 1] >> 6) == 2)
	    {
		/* forward 2 bytes for the valid sequence */
		count++;	// count = 2;
		unicode = (b << 6) | (buf[lpos + 1] & 0x3F);
	    }
	}	
	else if (! (b & 0x10))	/* leading byte 1110xxxx */
	{
	    /* check the next 2 continuation bytes */
	    if ((buf[lpos + 1] >> 6) == 2 && (buf[lpos + 2] >> 6) == 2)
	    {
		count = 3;	/* forward 3 bytes for the valid sequence */
		unicode = (b << 6) | (buf[lpos + 1] & 0x3F);
		unicode = (unicode << 6) | (buf[lpos + 2] & 0x3F);
	    }
	}
	#endif
	/* invalid byte */
do_forward:

	//lpos < llen;
	if (lpos + count > llen)
	{
		/* count > 1, but exceeding the buffer, so it is invalid. */
		count = 1;	/* forward 1 invalid byte */
	}

      lpos += count;

      /* If the cursor goes outside, scroll the screen to the right.  */
      if (xpos + count >= CMDLINE_WIDTH)
	cl_refresh (1, 0);
      else
	{
	  if (current_term->flags & TERM_DUMB)
	    {
	      int i;
	      
	      xpos += count;

	      for (i = lpos - count; i < lpos; i++)
		{
		  if (! get_cmdline_str.echo_char)
		    grub_putchar (buf[i], 255);
		  else
		    grub_putchar (get_cmdline_str.echo_char, 255);
		}
	    }
	  else
	    {
	      if (count > 1)
	      {
		/* get char width */
		count = 2;	/* initalize as wide char */
//		if (*(unsigned long *)(UNIFONT_START + (unicode << 5)) == narrow_char_indicator)
		if (((*(unsigned char *)((unsigned int)narrow_mem + unicode/8)) & (unsigned char)(1 << (unicode&7))) == 0)		//宽字符指示器使用内存分配		2023-02-22
			count--;	// count = 1;
	      }
	      xpos += count;
	      gotoxy (xpos, fonty);
	    }
	}
}

/* Refresh the screen. If FULL is true, redraw the full line, otherwise,
   only LEN characters from LPOS.  */
static void cl_refresh (int full, int len)
{
      unsigned long i;
      unsigned long start;
      unsigned long pos = xpos;
      unsigned long offset = 0;
      unsigned long lpos_fontx;
      
      if (full)
	{
	  /* From the start to the end.  */
	  len = CMDLINE_WIDTH;
	  pos = 0;
	  if(cursor_state==2)
			gotoxy (password_x, fonty);
		else
			gotoxy (0, fonty);

	  /* Recompute the section number.  */
	  if (lpos + plen < len)
	    {
	      section = 0;
	      grub_printf ("%s", get_cmdline_str.prompt);
	      plen = fontx;
	      len -= plen;
	      pos += plen;
	    }
	  else
	    {
	      section = (lpos + plen - CMDLINE_MARGIN) / (len - CMDLINE_MARGIN);
	      grub_putchar ('<', 255);
	      len--;
	      pos++;
	    }
	}

      /* Compute the index to start writing BUF and the resulting position
	 on the screen.  */
      if (section == 0)
	{
	  if (! full)	/* the current code use full=1, so always offset=0 */
	    offset = xpos - plen;
	  
	  start = offset;
	  xpos = lpos + plen;
	  //start += offset; // offset always be 0 in current implementation
	}
      else
	{
	  if (! full)	/* the current code use full=1, so always offset=0 */
	    offset = xpos - 1;
	  
	  start = section * (CMDLINE_WIDTH - CMDLINE_MARGIN) - plen + (CMDLINE_MARGIN / 2);

	  xpos = lpos + 1 - start;
	  start += offset; // offset always be 0 in current implementation

	  /* try to locate a leading byte for UTF8 sequence */
	  if ((unsigned char)(buf[start]) > 0x7F)
	  {
	    for (i = start; i > start - 3; i--)
	    {
		if (((buf[i] >> 5) == 6)	/* leading byte 110xxxxx */
		   ||  ((buf[i] >> 4) == 14)	/* leading byte 1110xxxx */
		   )
		{
			start = i;
			break;	/* success */
		}
	    }
	  }
	}

      lpos_fontx = 0;

      /* Print BUF. If ECHO_CHAR is not zero, put it instead.  */
      for (i = start; i < start + len && i < llen; i++)
	{
	  if (i == lpos)
		lpos_fontx = fontx;
	  if (! get_cmdline_str.echo_char)
	  {
	    grub_putchar (buf[i], 255);
	  }
	  else
	    grub_putchar (get_cmdline_str.echo_char, 255);

	  pos++;
	}
      
      /* print a dummy space to end the possible pending utf8 sequence. */
#ifdef SUPPORT_GRAPHICS
      if (graphics_inited && graphics_mode > 0xFF)
	grub_putchar (' ', 0);	/* width = 0 means no actual print */
#endif

      if (i == lpos)
		lpos_fontx = fontx;

      if (len && lpos_fontx == 0)
	printf_warning ("\nReport bug! lpos=%d, start=%d, len=%d, llen=%d, plen=%d, section=%d\n", lpos, start, len, llen, plen, section);

      /* Fill up the rest of the line with spaces.  */
      for (; i < start + len; i++)
	{
	  grub_putchar (' ', 255);
	  pos++;
	}
      
      /* If the cursor is at the last position, put `>' or a space,
	 depending on if there are more characters in BUF.  */
      if (pos == CMDLINE_WIDTH)
	{
	  if (start + len < llen)
	    grub_putchar ('>', 255);
	  else
	    grub_putchar (' ', 255);
	  
	  pos++;
	}
      
      /* Back to XPOS.  */
      if (current_term->flags & TERM_DUMB)
	{
	  for (i = 0; i < pos - xpos; i++)
	    grub_putchar ('\b', 255);
	}
      else
	gotoxy ((xpos = lpos_fontx), fonty);	//gotoxy (fontx-(pos - xpos), fonty);
}

/* Insert STR to BUF.  */
static void cl_insert (const char *str)
{
      int l = grub_strlen (str);

      if (llen + l < get_cmdline_str.maxlen)
	{
	  if (lpos == llen)
	    grub_memmove (buf + lpos, str, l + 1);
	  else
	    {
	      grub_memmove (buf + lpos + l, buf + lpos, llen - lpos + 1);
	      grub_memmove (buf + lpos, str, l);
	    }
	  
	  llen += l;
	  lpos += l;
	  cl_refresh (1, 0);
#if 0
	  if (xpos + l >= CMDLINE_WIDTH)
	    cl_refresh (1, 0);
	  else if (xpos + l + llen - lpos > CMDLINE_WIDTH)
	    cl_refresh (0, CMDLINE_WIDTH - xpos);
	  else
	    cl_refresh (0, l + llen - lpos);
#endif
	}
}

/* Delete COUNT characters in BUF.  */
static void cl_delete (int count)
{
	unsigned char b = *(buf + lpos);
	//unsigned long count = 1;

	/* get the length of the current sequence */
	#ifdef SUPPORT_GRAPHICS
	if (b < 0x80 || !graphics_inited)
	    goto do_delete;	/* delete 1 ASCII byte */
	if (! (b & 0x40))	/* continuation byte 10xxxxxx */
	    goto do_delete;	/* delete 1 invalid continuation byte */
	if (! (b & 0x20))	/* leading byte 110xxxxx */
	{
	    /* check the next 1 continuation byte */
	    count += ((buf[lpos + 1] >> 6) == 2);
	}	
	else if (! (b & 0x10))	/* leading byte 1110xxxx */
	{
	    /* check the next 2 continuation bytes */
	    if ((buf[lpos + 1] >> 6) == 2 && (buf[lpos + 2] >> 6) == 2)
		count = 3;	/* delete 3 bytes for the valid sequence */
	}
	#endif
	/* invalid byte */
do_delete:
      grub_memmove (buf + lpos, buf + lpos + count, llen - count + 1);
      llen -= count;
      
      cl_refresh (1, 0);
#if 0
      if (xpos + llen + count - lpos > CMDLINE_WIDTH)
	cl_refresh (0, CMDLINE_WIDTH - xpos);
      else
	cl_refresh (0, llen + count - lpos);
#endif
}

static int
real_get_cmdline (void)
{
  /* This is a rather complicated function. So explain the concept.
     
     A command-line consists of ``section''s. A section is a part of the
     line which may be displayed on the screen, but a section is never
     displayed with another section simultaneously.

     Each section is basically 77 or less characters, but the exception
     is the first section, which is 78 or less characters, because the
     starting point is special. See below.

     The first section contains a prompt and a command-line (or the
     first part of a command-line when it is too long to be fit in the
     screen). So, in the first section, the number of command-line
     characters displayed is 78 minus the length of the prompt (or
     less). If the command-line has more characters, `>' is put at the
     position 78 (zero-origin), to inform the user of the hidden
     characters.

     Other sections always have `<' at the first position, since there
     is absolutely a section before each section. If there is a section
     after another section, this section consists of 77 characters and
     `>' at the last position. The last section has 77 or less
     characters and doesn't have `>'.

     Each section other than the last shares some characters with the
     previous section. This region is called ``margin''. If the cursor
     is put at the magin which is shared by the first section and the
     second, the first section is displayed. Otherwise, a displayed
     section is switched to another section, only if the cursor is put
     outside that section.  */

  int c;
  int history = -1;	/* The index for the history.  */

  buf = (unsigned char *) CMDLINE_BUF;
  plen = grub_strlen ((const char *)get_cmdline_str.prompt);
  llen = grub_strlen ((const char *)get_cmdline_str.cmdline);

  if (get_cmdline_str.maxlen > MAX_CMDLINE)
    {
      get_cmdline_str.maxlen = MAX_CMDLINE;
      if (llen >= MAX_CMDLINE)
	{
	  llen = MAX_CMDLINE - 1;
	  get_cmdline_str.cmdline[MAX_CMDLINE] = 0;
	}
    }
  lpos = llen;
  grub_strcpy ((char *)buf, (const char *)get_cmdline_str.cmdline);

  if (fontx)
	grub_putchar ('\n', 255);
  cl_refresh (1, 0);  /* Print full line and set position here */

  if (get_cmdline_str.readline > 1)
  {
	int t1;
	int t2 = -1;
	int wait_t = get_cmdline_str.readline >> 8;
	while ((t2 = getrtsecs ()) == 0xFF);
	while (wait_t)
	{
		if (checkkey () != -1)
			break;
		if ((t1 = getrtsecs ()) != t2 && t1 != 0xFF)
		{
			t2 = t1;
			wait_t--;
		}
	}
	if (wait_t == 0)
		return 1;
  }
  get_cmdline_str.readline &= 1;

  while ((char)(c = /*ASCII_CHAR*/ (getkey ())) != '\n' && (char)c != '\r')
    {
      /* If READLINE is non-zero, handle readline-like key bindings.  */
      if (get_cmdline_str.readline)
	{
	  if ((char)c == 9)	/* TAB lists completions */
	      {
		int i;
		/* POS points to the first space after a command.  */
		int pos = 0;
		int ret;
		char *completion_buffer = (char *) COMPLETION_BUF;
		int equal_pos =-1;
		int is_filename;

		/* Find the first word.  */
		while (buf[pos] == ' ')
		  pos++;
		while (buf[pos] && buf[pos] != '=' && buf[pos] != ' ')
		  pos++;

		is_filename = (lpos > pos);

		/* Find the position of the equal character after a
		   command, and replace it with a space.  */
		for (i = pos; buf[i] && buf[i] != ' '; i++)
		  if (buf[i] == '=')
		    {
		      equal_pos = i;
		      buf[i] = ' ';
		      break;
		    }

		/* Find the position of the first character in this word.  */
		for (i = lpos; i > 0; i--)
		{
			if (buf[i - 1] == '"')
			{
				i--;
				while (buf[i - 1] != '"')
					i--;
			}
		    if (buf[i - 1] == ' ' || buf[i - 1] == '=')
		    {
			/* find backslashes immediately before the space */
			for (ret = i - 2; ret >= 0; ret--)
			{
			    if (buf[ret] != '\\')
				break;
			}

			if (! ((i - ret) & 1)) /* the space not backslashed */
				break;
		    }
		}

		/* Invalidate the cache, because the user may exchange
		   removable disks.  */
		buf_drive = -1;

		/* Copy this word to COMPLETION_BUFFER and do the completion.*/
		grub_memmove (completion_buffer, buf + i, lpos - i);
		completion_buffer[lpos - i] = 0;
		ret = print_completions (is_filename, 1);

		if (! is_filename && ret < 0)
		{
			ret = print_completions ((is_filename=1), 1);
		}
		errnum = ERR_NONE;

		if (ret >= 0)
		  {
		    /* Found, so insert COMPLETION_BUFFER.  */
		    cl_insert (completion_buffer + lpos - i);

		    if (ret > 0)
		      {
			/* There are more than one candidates, so print
			   the list.  */
			if (fontx)
			  grub_putchar ('\n', 255);
			print_completions (is_filename, 0);
			errnum = ERR_NONE;
		      }
		  }

		/* Restore the command-line.  */
		if (equal_pos >= 0)
		  buf[equal_pos] = '=';

		if (ret)
		{
		  if (fontx)
			grub_putchar ('\n', 255);
		  cl_refresh (1, 0);/* Print full line and set position here */
		}
	      }
	  else if (c == KEY_HOME/* || (char)c == 1*/)	/* C-a beginning */
		/* Home= 0x4700 for BIOS, 0x0106 for Linux */
	    {
	      //cl_backward (lpos);
		lpos = 0;
		cl_refresh (1, 0);
	    }
	  else if (c == KEY_END/* || (char)c == 5*/)	/* C-e end */
		/* End= 0x4F00 for BIOS, 0x0168 for Linux */
	    {
	      //cl_forward (llen - lpos);
		lpos = llen;
		cl_refresh (1, 0);
	    }
	  else if (c == KEY_RIGHT/* || (char)c == 6*/)	/* C-f forward */
		/* Right= 0x4D00 for BIOS, 0x0105 for Linux */
	      {
		if (lpos < llen)
		  cl_forward (1);
	      }
	  else if (c == KEY_LEFT/* || (char)c == 2*/)	/* C-b backward */
		/* Left= 0x4B00 for BIOS, 0x0104 for Linux */
	      {
		if (lpos > 0)
		  cl_backward (1);
	      }
	  else if (c == KEY_UP/* || (char)c == 16*/)	/* C-p previous */
		/* Up= 0x4800 for BIOS, 0x0153 for Linux */
	      {
		char *p;

		if (history < 0)
		  /* Save the working buffer.  */
		  grub_strcpy ((char *)get_cmdline_str.cmdline, (const char *)buf);
		else if (grub_strcmp (get_history (history), (const char *)buf) != 0)
		  /* If BUF is modified, add it into the history list.  */
		  add_history ((const char *)buf, history);

		history++;
		p = get_history (history);
		if (! p)
		  {
		    history--;
		  }
		else
		  {

		    grub_strcpy ((char *)buf, p);
		    llen = grub_strlen ((const char *)buf);
		    lpos = llen;
		    cl_refresh (1, 0);
		  }
	      }
	  else if (c == KEY_DOWN/* || (char)c == 14*/)	/* C-n next command */
		/* Down= 0x5000 for BIOS, 0x0152 for Linux */
	      {
		char *p;

		if (history >= 0)
		  {
		    if (grub_strcmp (get_history (history), (const char *)buf) != 0)
		      /* If BUF is modified, add it into the history list.  */
		      add_history ((const char *)buf, history);

		    history--;
		    p = get_history (history);
		    if (! p)
		      p = (char *)get_cmdline_str.cmdline;

		    grub_strcpy ((char *)buf, p);
		    llen = grub_strlen ((const char *)buf);
		    lpos = llen;
		    cl_refresh (1, 0);
		  }
	      }
	}

      /* ESC, C-d and C-h are always handled. Actually C-d is not
	 functional if READLINE is zero, as the cursor cannot go
	 backward, but that's ok.  */

		/* Ins= 0x5200 for BIOS, 0x014B for Linux */
		/* PgUp= 0x4900 for BIOS, 0x0153 for Linux */
		/* PgDn= 0x5100 for BIOS, 0x0152 for Linux */

	if ((char)c == 27)	/* ESC immediately return 1 */
	  return 1;
	else if (c == KEY_DC/* || (char)c == 4*/)	/* C-d delete */
		/* Del= 0x5300 for BIOS, 0x014A for Linux */
	  {
	    if (lpos != llen)
	    {
	      cl_delete (1);
	    }
	  }
	else if (c == KEY_BACKSPACE || (char)c == 8)	/* C-h backspace */
		/* Backspace= 0x0E08 for BIOS, 0x0107 for Linux */
	  {
	    if (lpos > 0)
	    {
	      cl_backward (1);
	      cl_delete (1);
	    }
	  }
	else		/* insert printable character into line */
	  {
	    if ((char)c >= ' ' && (char)c <= '~')
	    {
	      char str[2];

	      str[0] = (char)c;
	      str[1] = 0;
	      cl_insert (str);
	    }
	  }
    }

//  if (fontx)
//	grub_putchar ('\n', 255);

  /* If ECHO_CHAR is NUL, remove the leading spaces.  */
  lpos = 0;
  if (! get_cmdline_str.echo_char)
    while (buf[lpos] == ' ')
      lpos++;

  /* Copy the working buffer to CMDLINE.  */
  grub_memmove (get_cmdline_str.cmdline, buf + lpos, llen - lpos + 1);

  /* If the readline-like feature is turned on and CMDLINE is not
     empty, add it into the history list.  */
  if (get_cmdline_str.readline && lpos < llen)
    add_history ((const char *)get_cmdline_str.cmdline, 0);

  return 0;
}

int get_cmdline_obsolete (struct get_cmdline_arg cmdline);

int
get_cmdline_obsolete (struct get_cmdline_arg cmdline)
{
	get_cmdline_str = cmdline;
	return get_cmdline ();
}

/* Don't use this with a MAXLEN greater than 1600 or so!  The problem
   is that GET_CMDLINE depends on the everything fitting on the screen
   at once.  So, the whole screen is about 2000 characters, minus the
   PROMPT, and space for error and status lines, etc.  MAXLEN must be
   at least 1, and PROMPT and CMDLINE must be valid strings (not NULL
   or zero-length).

   If ECHO_CHAR is nonzero, echo it instead of the typed character. */
int
get_cmdline (void)
{
  unsigned long old_cursor = cursor_state;
  int ret;

  /* Because it is hard to deal with different conditions simultaneously,
     less functional cases are handled here. Assume that TERM_NO_ECHO
     implies TERM_NO_EDIT.  */
  if (current_term->flags & (TERM_NO_ECHO | TERM_NO_EDIT))
    {
      unsigned char *p = get_cmdline_str.cmdline;
      unsigned int c;

      setcursor (cursor_state | 1);
      /* Make sure that MAXLEN is not too large.  */
      if (get_cmdline_str.maxlen > MAX_CMDLINE)
		get_cmdline_str.maxlen = MAX_CMDLINE;

      /* Print only the prompt. The contents of CMDLINE is simply discarded,
	 even if it is not empty.  */
      grub_printf ("%s", get_cmdline_str.prompt);

      /* Gather characters until a newline is gotten.  */
      while ((c = ASCII_CHAR (getkey ())) != '\n' && c != '\r')
	{
	  /* Return immediately if ESC is pressed.  */
	  if (c == 27)
	    {
	      setcursor (old_cursor);
	      return 1;
	    }

	  /* Printable characters are added into CMDLINE.  */
	  if (c >= ' ' && c <= '~')
	    {
	      if (! (current_term->flags & TERM_NO_ECHO))
		grub_putchar (c, 255);

	      /* Preceding space characters must be ignored.  */
	      if (c != ' ' || p != get_cmdline_str.cmdline)
		*p++ = c;
	    }
	}

      *p = 0;

      if (! (current_term->flags & TERM_NO_ECHO))
	grub_putchar ('\n', 255);

      setcursor (old_cursor);
      return 0;
    }

  /* Complicated features are left to real_get_cmdline.  */
  ret = real_get_cmdline ();
  setcursor (old_cursor);
  return ret;
}

// Parse decimal or hexadecimal ASCII input string to 64-bit integer
// input number may have K,M,G,T or k,m,g,t suffix
// if unitshift is 0, the number is plain number.
//  1K=1024, 1M=1048576, 1G=1<<30, 1T=1<<40
// if unitshift is 9, the input number is number of 512-bytes sectors and suffixes means KBytes, MBytes,...
//  1K=2 sectors, 1M=2048 sectors, ...
// unitshift must be in the range 0-63
int
safe_parse_maxint_with_suffix (char **str_ptr, unsigned long long *myint_ptr, int unitshift)
{
  unsigned long long myint = 0;
  //unsigned long long mult = 10;
  char *ptr = *str_ptr;
  char found = 0;
  char negative = 0;

  /*
   *  The decimal numbers can be positive or negative, ranging from
   *  0x8000000000000000(the minimal long long) to 0x7fffffffffffffff(the maximal long long).
   *  The hex numbers are not checked.
   */
#if 0
  if (*ptr == '-') /* check whether or not the negative sign exists */
    {
      ptr++;
      negative = 1;
    }
#else
  switch(*ptr)
  {
    case '-':
    case '~':
    case '!':
      negative = *ptr++;
    default:
      break;
  }
#endif
  /*
   *  Is this a hex number?
   */
  if (*ptr == '0' && (ptr[1]|32) == 'x') // |32 convert A-Z to lower case, no need to make sure it is really A-Z
  //if (*ptr == '0' && tolower (*(ptr + 1)) == 'x')
  {
      ptr += 2;
      while(1)
      {
      /* A bit tricky. This below makes use of the equivalence:
	 (A <= B && A <= C) <=> ((A - B) <= (C - B))
	 when C > B and A is unsigned.  */
#if 1
        unsigned char digit;
	digit = (unsigned char)(*ptr-'0'); 
	// '0'...'9' become 0...9, 'A'...'F' become 17...22, 'a'...'f' become 49...54
	if (digit > 9) 
	{
	    digit = (digit|32)	// 'A'...'F' become 49...54, 'a'...'f' become 49...54
	            -49;	// 'A'...'F' become 0...5, 'a'...'f' become 0...5
	    if (digit > 5) 
	        break; // end of hexadecimal number
	    digit +=10;
	} // don't have to call tolower function
#else
	unsigned int digit;

	digit = tolower (*ptr) - '0';
	if (digit > 9)
	  {
	    digit -= 'a' - '0';
	    if (mult == 10 || digit > 5)
	      break;
	    digit += 10;
	  }
#endif
	found = 16;
	if ( myint>>(64-4) ) 
	{ // highest digit has already been filled with non-zero, another left shift will overflow
	  errnum = ERR_NUMBER_OVERFLOW;
	  return 0;
	}
	myint = (myint << 4) | digit;
        ptr++;
      }
  }
  else // separated loop for base-16 and base-10 number may be slightly faster
  {
      while (1)
      {
	unsigned char digit;
	digit = (unsigned char)(*ptr-'0'); 
	if (digit>9) 
	    break;
	found = 10;
#if 1
	if ( myint > ((-1ULL>>1)/10) // multiply with 10 will overflow (result > max signed long long)
	  || (myint = myint*10 + digit,
	      // numbers less than (1ULL<<63) are valid (positive or negative).
	      // overflow if bit63 is set, 
	      (long long)myint < 0 
	      // except for (1ULL<<63) which is valid if negative.
	      // (1ULL<<63)*2 truncated to 64 bit is 0
	      && ((myint+myint) || !negative )
	     )
	  )
	{ 
	    errnum = ERR_NUMBER_OVERFLOW;
	    return 0;
	}
#else
	/* we do not check for hex or negative */
	if (mult == 10 && ! negative)
	  /* 0xFFFFFFFFFFFFFFFF == 18446744073709551615ULL */
  //	if ((unsigned)myint > (((unsigned)(MAXINT - digit)) / (unsigned)mult))
	  if (myint > 1844674407370955161ULL ||
	     (myint == 1844674407370955161ULL && digit > 5))
	    {
	      errnum = ERR_NUMBER_OVERFLOW;
	      return 0;
	    }
	myint *= mult;
	myint += digit;
#endif
	  ptr++;
      }
  }
  if (!found)
    {
      errnum = ERR_NUMBER_PARSING;
      return 0;
    }
  else
  {
    unsigned long long myint2,myint3;
    int myshift;
    switch ((*ptr)|32)
    {
      case 'k': myshift = 10-unitshift; ++ptr; break;
      case 'm': myshift = 20-unitshift; ++ptr; break;
      case 'g': myshift = 30-unitshift; ++ptr; break;
      case 't': myshift = 40-unitshift; ++ptr; break;
      default:  myshift = 0;
    }
    if (myshift >= 0)
	myint3 = (myint2 = myint <<  myshift) >>  myshift;
    else
        myint3 = (myint2 = myint >> -myshift) << -myshift; 
    // myint3 should be equal to myint unless some bit were lost
    if( myint3 != myint // if some bits were lost
      || ((long long)(myint2 ^ myint)<0 && found==10) ) // or sign bit changed for decimal number
    {   
	errnum = ERR_NUMBER_OVERFLOW;
	return 0;
    }
//    *myint_ptr = negative? -myint2: myint2;
     switch(negative)
    {
      case '-':
        *myint_ptr = -myint2;
        break;
      case '!':
        *myint_ptr = !myint2;
        break;
      case '~':
        *myint_ptr = ~myint2;
         break;
      default:
        *myint_ptr = myint2;
        break;
    }
    *str_ptr = ptr;
    return 1;
  }
}
#if 0
int
safe_parse_maxint (char **str_ptr, unsigned long long *myint_ptr)
{
  char *ptr = *str_ptr;
  unsigned long long myint = 0;
  unsigned long long mult = 10;
  int found = 0;
  int negative = 0;

  /*
   *  The decimal numbers can be positive or negative, ranging from
   *  0x80000000(the minimal int) to 0x7fffffff(the maximal int).
   *  The hex numbers are not checked.
   */

  if (*ptr == '-') /* check whether or not the negative sign exists */
    {
      ptr++;
      negative = 1;
    }

  /*
   *  Is this a hex number?
   */
  if (*ptr == '0' && tolower (*(ptr + 1)) == 'x')
    {
      ptr += 2;
      mult = 16;
    }

  while (1)
    {
      /* A bit tricky. This below makes use of the equivalence:
	 (A >= B && A <= C) <=> ((A - B) <= (C - B))
	 when C > B and A is unsigned.  */
      unsigned int digit;

      digit = tolower (*ptr) - '0';
      if (digit > 9)
	{
	  digit -= 'a' - '0';
	  if (mult == 10 || digit > 5)
	    break;
	  digit += 10;
	}

      found = 1;
      /* we do not check for hex or negative */
      if (mult == 10 && ! negative)
	/* 0xFFFFFFFFFFFFFFFF == 18446744073709551615ULL */
//	if ((unsigned)myint > (((unsigned)(MAXINT - digit)) / (unsigned)mult))
	if (myint > 1844674407370955161ULL ||
	   (myint == 1844674407370955161ULL && digit > 5))
	  {
	    errnum = ERR_NUMBER_OVERFLOW;
	    return 0;
	  }
      myint *= mult;
      myint += digit;
      ptr++;
    }

  if (!found)
    {
      errnum = ERR_NUMBER_PARSING;
      return 0;
    }

  *str_ptr = ptr;
  *myint_ptr = negative ? -myint : myint;

  return 1;
}
#endif

int
grub_tolower (int c)
{
#if 0
  if (c >= 'A' && c <= 'Z')
    return (c + ('a' - 'A'));
#else
  if ((unsigned int)(c - 'A') < 26)
    return c|0x20;
#endif

  return c;
}

int
grub_isspace (int c)
{
  switch (c)
    {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
      return 1;
    default:
      break;
    }

  return 0;
}

/*
* CRC 010041
*/
static unsigned short crc16_table[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

unsigned short grub_crc16(unsigned char *data, int size);
unsigned short
grub_crc16(unsigned char *data, int size)
{
	unsigned short crc=0;
	int n = size?size:strlen((const char *)data);

	while (n-- > 0)
		crc = crc16_table[(crc>>8 ^ *data++) & 0xff] ^ (crc<<8);

	return crc;
}


static grub_u32_t crc32_tab[256] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static grub_u32_t calc_crc32(grub_u32_t crc, const void *data, grub_u32_t size)
{
  const grub_u8_t *p;

  p = data;
  crc ^= ~0U;

  while (size--)
    crc = crc32_tab[(crc^*p++) & 0xFF]^(crc >> 8);
  return crc^~0U;
}

int grub_crc32(char *data,grub_u32_t size)
{
  int crc = 0;
  int len = 0;

  if ((*data == '(' || * data== '/') && grub_open(data))
  {
    while((len = grub_read ((unsigned long long)(unsigned long)mbr, SECTOR_SIZE, GRUB_READ)))
      crc = calc_crc32(crc,mbr,len);
    grub_close();
  }
  else
  {
    errnum = 0;
    crc = calc_crc32(crc,data,size?size:strlen(data));
  }

  return crc;
}

int
grub_memcmp (const char *s1, const char *s2, int n)
{
  while (n)
    {
      if (*s1 < *s2)
	return -1;
      else if (*s1 > *s2)
	return 1;
      s1++;
      s2++;
      n--;
    }

  return 0;
}

int
grub_strncat (char *s1, const char *s2, int n)
{
  int i = -1;

  while (++i < n && s1[i] != 0);

  while (i < n && (s1[i++] = *(s2++)) != 0);

  s1[n - 1] = 0;

  if (i >= n)
    return 0;

  s1[i] = 0;

  return 1;
}

/* XXX: This below is an evil hack. Certainly, we should change the
   strategy to determine what should be defined and what shouldn't be
   defined for each image. For example, it would be better to create
   a static library supporting minimal standard C functions and link
   each image with the library. Complicated things should be left to
   computer, definitely. -okuji  */
int
grub_strcmp (const char *s1, const char *s2)
{
  while (*s1 || *s2)
    {
      if (*s1 < *s2)
	return -1;
      else if (*s1 > *s2)
	return 1;
      s1 ++;
      s2 ++;
    }

  return 0;
}

/* Wait for a keypress and return its code.  */
int
getkey (void)
{
  return current_term->getkey ();
}

/* Check if a key code is available.  */
int
checkkey (void)
{
  return current_term->checkkey ();
}

unsigned char *set_putchar_hook(unsigned char *hooked)
{
	unsigned char *re = putchar_hooked;
	putchar_hooked = hooked;
	return re;
}

/* FIXME: this is problematic! it could cause memory conflicts! */
/* Display an ASCII character.  */
unsigned int
_putchar (unsigned int c, unsigned int max_width)
{
  /* if it is a Line Feed, we insert a Carriage Return. */
	if (putchar_hooked)
	{
		if ((unsigned int)putchar_hooked > 0x800)
			*(unsigned long*)putchar_hooked++ = (unsigned char)c;
		return 1;
	}

	if (c == '\t'/* && current_term->getxy*/)
	{
		c = 8 - (fontx/*(current_term->getxy ())*/ & 7);
		if (max_width>c)
			max_width = c;
		else
			c = max_width;
		for (;c;--c)
		  current_term->putchar (' ', 1);
		return max_width;
	}
	if (c == '\n')
	{
		current_term->putchar ('\r', max_width);
	}

	int i = current_term->putchar (c, max_width);

	if (fontx == 0 && count_lines >= 0)
	{
		if (
		 #ifdef SUPPORT_GRAPHICS
		!graphics_inited && 
		#endif
		c != '\r')
			++count_lines;
		/* Internal `more'-like feature.  */
		if (count_lines >= current_term->max_lines - 1)
		{
			/* It's important to disable the feature temporarily, because
			the following grub_printf call will print newlines.  */
			count_lines = -1;
			if (! (current_term->flags & TERM_DUMB))
			{
				if (current_term->setcolorstate)
				current_term->setcolorstate (COLOR_STATE_HIGHLIGHT);

				grub_printf ("[Hit Q to quit, any other key to continue]");

				if ((getkey () & 0xDF) == 0x51)	/* 0x51 == 'Q' */
					quit_print = 1;

				if (current_term->setcolorstate)
					current_term->setcolorstate (COLOR_STATE_STANDARD);
				grub_printf ("\r%50s","\r");
			}
			/* Restart to count lines.  */
			count_lines = 0;
		}
	}
	return i;
}

inline void
debug_sleep(int l_debug_boot, int line, char *file)
{
  if (l_debug_boot) {
    static int count=0;
    grub_printf("<%d press key (%s,%d)>\n", ++count, file, line);
    console_getkey();
    //grub_printf("\r%*s\r", CMDLINE_WIDTH, " ");
  }
}

#ifdef DEBUG_TIME
inline void debug_time(const int line,const char*file)
{
	unsigned long date, time;
	get_datetime(&date, &time);
	printf("%s[%d]:%02X:%02X:%02X\n",file,line,(char)(time >> 24),(char)(time >> 16),(char)(time>>8));
}
#endif
void
gotoxy (int x, int y)
{
  current_term->gotoxy (x, y);
}

int
getxy (void)
{
  return current_term->getxy ();
}

void
cls (void)
{
  /* If the terminal is dumb, there is no way to clean the terminal.  */
  if (current_term->flags & TERM_DUMB)
    grub_putchar ('\n', 255);
  else
    current_term->cls ();
}

unsigned long
setcursor (unsigned long on)
{
  unsigned long old_state = cursor_state;
  cursor_state = on;
  
  if (current_term->setcursor)
      current_term->setcursor (on & 1);

  return old_state;
}
/* strncmpx Enhanced string comparison function by chenall 2011-12-13
	int strncmpx (const char * s1, const char * s2, unsigned long n, int case_insensitive)
	Compare two strings s1, s2. Length: n,
	If n is equal to 0, only the comparison to the end of the string.
	If case_insensitive non-zero, not case sensitive.
	Return value:
		When s1 < s2, the return value < 0
		When s1 = s2, the return value = 0
		If s1 > s2, the return value >0
	When n =0,
	return 0 when s1 = s2;
	return 1 s1 isn't a substring of S2
	return -1 s1 is a substring of S2.
*/
int strncmpx(const char *s1,const char *s2, unsigned long n, int case_insensitive)
{
	int x = (n?0:--n);
	while (n)
	{
		char c = *s1 - *s2;
		if (!x)
			--n;
		else if (!*s1)
			return c;
		if (c)
		{
			if (!case_insensitive || (c = tolower(*s1) - tolower(*s2)))
			{
				return c;
			}
		}
		++s1,++s2;
	};
	return 0;
}

int
substring (const char *s1, const char *s2, int case_insensitive)
{
  char ch1, ch2;
  
  for (;;)
    {
      ch1 = *(s1++);
      ch2 = *(s2++);
      
      if (case_insensitive)
      {
	ch1 = tolower(ch1);
	ch2 = tolower(ch2);
      }

      if (! ch1)	/* S1 is a substring of S2, or they match exactly */
	return ch2 ? -1 : 0;
      
      if (ch1 != ch2)
	return 1;	/* S1 isn't a substring of S2 */
    }
}

/* Terminate the string STR with NUL.  */
int
nul_terminate (char *str)
{
  int ch;
  
//  while (*str && ! grub_isspace (*str))
//    str++;

  while ((ch = *str) && ! grub_isspace (ch))
  {
		if (ch == '"')
		{
			str++;
			while (*str != '"')
				str++;
		}
	if (ch == '\\')
	{
		str++;
		if (! (ch = *str))
			break;
	}
	str++;
  }

//  ch = *str;
  *str = 0;
  return ch;
}

char *
grub_strstr (const char *s1, const char *s2)
{
  while (*s1)
    {
      const char *ptr, *tmp;

      ptr = s1;
      tmp = s2;

      while (*tmp && *ptr == *tmp)
	ptr++, tmp++;
      
      if (tmp > s2 && ! *tmp)
	return (char *) s1;

      s1++;
    }

  return 0;
}

int
grub_strlen (const char *str)
{
  int len = 0;

  while (*str++)
    len++;

  return len;
}

int
memcheck (unsigned long long addr, unsigned long long len)
{
  errnum = 0;
  if (! addr || (! is64bit && (addr >= 0x100000000ULL || addr + len > 0x100000000ULL)))
    errnum = ERR_WONT_FIT;

  return ! errnum;
}

#if 0
void
grub_memcpy(void *dest, const void *src, int len)
{
  int i;
  register char *d = (char*)dest, *s = (char*)src;

  for (i = 0; i < len; i++)
    d[i] = s[i];
}
#endif

#if 0
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
static inline void * _memcpy_backward(void *dst, const void *src, unsigned int len)
{
    int r0, r1, r2, r3;
    __asm__ __volatile__(
	"std; \n\t"
	"movl %%ecx, %0; andl $3, %%ecx; "	// now ESI,EDI point to end-1, ECX=(len % 4)
	"rep; movsb; "
	"subl $3, %%edi; subl $3, %%esi; "
	"movl %0, %%ecx; shrl $2, %%ecx; "	// now ESI,EDI point to end-4, ECX=(len / 4)
	"rep; movsl; \n\t"
	"cld; \n\t"
	: "=&r"(r0),"=&c"(r1), "=&D"(r2), "=&S"(r3)
	: "1"(len), "2"((long)dst+len-1), "3"((long)src+len-1)
	: "memory");
    return dst;
}
#endif
static inline int _memcmp(const void *str1, const void *str2, unsigned int len)
{
    int a, r1, r2, r3;
    __asm__ __volatile__(
	"movl %%ecx, %%eax; shrl $2, %%ecx; "	// ECX=len/4
	"repe; cmpsl; "
	"je   1f; "	// jump if n/4==0 or all longs are equal
	"movl $4, %%ecx; subl %%ecx,%%edi; subl %%ecx,%%esi; " // not equal, compare the previous 4 bytes again byte-by-byte
	"jmp  2f;"
	"\n1:\t"	// all longs are equal, compare the remaining 0-3 bytes
	"andl $3, %%eax; movl %%eax, %%ecx; "	// ECX=len%4
	"\n2:\t"
	"repe; cmpsb; "	// final comparison
	"seta %%al; "	// AL = (str1>str2)? 1:0, 3 high byte of EAX is already 0
	"setb %%cl; "	// CL = (str1<str2)? 1:0, 3 high byte of ECX is already 0
	"subl %%ecx,%%eax; "	// if (str1<str2) EAX = -1
	: "=&a"(a), "=&c"(r1), "=&S"(r2), "=&D"(r3)
	: "1"(len), "2"((long)str1), "3"((long)str2)
	: "memory");
    return a;
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


/* struct copy needs the memcpy function */
/* #undef memcpy */
#if 1
void * grub_memcpy(void * to, const void * from, unsigned int n)
{
       /* This assembly code is stolen from
	* linux-2.4.22/include/asm-i386/string.h
	* It assumes ds=es=data space, this should be normal.
	*/
	int d0, d1, d2;
	__asm__ __volatile__(
		"rep ; movsl\n\t"
		"testb $2,%b4\n\t"
		"je 1f\n\t"
		"movsw\n"
		"1:\ttestb $1,%b4\n\t"
		"je 2f\n\t"
		"movsb\n"
		"2:"
		: "=&c" (d0), "=&D" (d1), "=&S" (d2)
		:"0" (n/4), "q" (n),"1" ((long) to),"2" ((long) from)
		: "memory");
	return to;
}
#else
/* just in case the assembly version of grub_memcpy does not work. */
void * grub_memcpy(void * to, const void * from, unsigned int n)
{
	char *d = (char *)to, *s = (char *)from;

	while (n--)
		*d++ = *s++;

	return to;
}
#endif

void *
grub_memmove (void *to, const void *from, int len)
{
#if 0
   if (memcheck ((int) to, len))
     {
       /* This assembly code is stolen from
	  linux-2.2.2/include/asm-i386/string.h. This is not very fast
	  but compact.  */
       int d0, d1, d2;

       if (to < from)
	 {
	   asm volatile ("cld\n\t"
			 "rep\n\t"
			 "movsb"
			 : "=&c" (d0), "=&S" (d1), "=&D" (d2)
			 : "0" (len),"1" (from),"2" (to)
			 : "memory");
	 }
       else
	 {
	   asm volatile ("std\n\t"
			 "rep\n\t"
			 "movsb\n\t"
			 "cld"
			 : "=&c" (d0), "=&S" (d1), "=&D" (d2)
			 : "0" (len),
			 "1" (len - 1 + (const char *) from),
			 "2" (len - 1 + (char *) to)
			 : "memory");
	 }
     }
#else  
  int r0, r1, r2, r3;
  if (to < from)
  {
    __asm__ __volatile__(
    "movl %%ecx, %0; shrl $2, %%ecx; "
    "rep; movsl; "
    "movl %0, %%ecx; andl $3, %%ecx; "
    "rep; movsb; "
    : "=&r"(r0), "=&c"(r1), "=&D"(r2), "=&S"(r3)
    : "1"(len), "2"((long)to), "3"((long)from)
    : "memory");
  }
  else
  {
    __asm__ __volatile__(
    "std; \n\t"
    "movl %%ecx, %0; andl $3, %%ecx; "
    "rep; movsb; "
    "subl $3, %%edi; subl $3, %%esi; "
    "movl %0, %%ecx; shrl $2, %%ecx; "
    "rep; movsl; \n\t"
    "cld; \n\t"
    : "=&r"(r0),"=&c"(r1), "=&D"(r2), "=&S"(r3)
    : "1"(len), "2"((long)to+len-1), "3"((long)from+len-1)
    : "memory");
  }
#endif
   return errnum ? NULL : to;
}

void *
grub_memset (void *start, int c, int len)
{
  char *p = start;

  if (memcheck ((unsigned int)start, len))
    {
      while (len -- > 0)
	*p ++ = c;
    }

  return errnum ? NULL : start;
}

char *
grub_strcpy (char *dest, const char *src)
{
  grub_memmove (dest, src, grub_strlen (src) + 1);
  return dest;
}

/* The strtok.c comes from reactos. It follows GPLv2. */

/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
char*
grub_strtok(char *s, const char *delim)
{
  const char *spanp;
  int c, sc;
  char *tok;
  static char *last;
   
  if (s == NULL && (s = last) == NULL)
    return (NULL);

  /*
   * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
   */
 cont:
  c = *s++;
  for (spanp = delim; (sc = *spanp++) != 0;) {
    if (c == sc)
      goto cont;
  }

  if (c == 0) {			/* no non-delimiter characters */
    last = NULL;
    return (NULL);
  }
  tok = s - 1;

  /*
   * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
   * Note that delim must have one NUL; we stop if we see that, too.
   */
  for (;;) {
    c = *s++;
    spanp = delim;
    do {
      if ((sc = *spanp++) == c) {
	if (c == 0)
	  s = NULL;
	else
	  s[-1] = 0;
	last = s;
	return (tok);
      }
    } while (sc != 0);
  }
  /* NOTREACHED */
}

# undef memcpy
/* GCC emits references to memcpy() for struct copies etc.  */
void *memcpy (void *dest, const void *src, int n)  __attribute__ ((alias ("grub_memmove")));

#if 0
int
grub_memcmp64_lm (const unsigned long long s1, const unsigned long long s2, unsigned long long n)
{
	if (((unsigned long *)&s1)[1] || ((unsigned long *)&s2)[1] || ((unsigned long *)&n)[1] || (s1 + n) > 0x100000000ULL || (s2 + n) > 0x100000000ULL)
	{
		return mem64 (2, s1, s2, n);	/* 2 for CMP */
	}

	return grub_memcmp ((char *)(unsigned int)s1, (char *)(unsigned int)s2, n);
}

void
grub_memmove64 (unsigned long long to, const unsigned long long from, unsigned long long len)
{
	if (((unsigned long *)&to)[1] || ((unsigned long *)&from)[1] || ((unsigned long *)&len)[1] || (to + len) > 0x100000000ULL || (from + len) > 0x100000000ULL)
	{
	}

	grub_memmove ((void *)(unsigned int)to, (void *)(unsigned int)from, len);
}

void
grub_memset64 (unsigned long long start, unsigned long long c, unsigned long long len)
{
	if (((unsigned long *)&start)[1] || ((unsigned long *)&len)[1] || (start + len) > 0x100000000ULL)
	{
		mem64 (3, start, c, len);	/* 3 for SET */
		return;
	}

	grub_memset ((void *)(unsigned int)start, c, len);
}

#endif

#define PAGING_PML4_ADDR (PAGING_TABLES_BUF+0x0000)
#define PAGING_PDPT_ADDR (PAGING_TABLES_BUF+0x1000)
#define PAGING_PD_ADDR   (PAGING_TABLES_BUF+0x2000)

// If this value is changed, memory_paging_map_for_transfer must also be modified.
#define PAGINGTXSTEP 0x800000

#define DST_VIRTUAL_BASE  0x1000000UL
#define SRC_VIRTUAL_BASE  0x2000000UL
#define DST_VIRTUAL_ADDR(addr) (((unsigned long)(addr) & 0x1FFFFFUL)+DST_VIRTUAL_BASE)
#define SRC_VIRTUAL_ADDR(addr) (((unsigned long)(addr) & 0x1FFFFFUL)+SRC_VIRTUAL_BASE)
#define DST_VIRTUAL_PTR(addr) ((void*)DST_VIRTUAL_ADDR(addr))
#define SRC_VIRTUAL_PTR(addr) ((void*)SRC_VIRTUAL_ADDR(addr))

// Set to 0 to test mem64 function
#define DISABLE_AMD64 0

extern void memory_paging_init(void);
extern void memory_paging_enable(void);
extern void memory_paging_disable(void);
extern void memory_paging_map_for_transfer(unsigned long long dst_addr, unsigned long long src_addr);

unsigned char memory_paging_initialized = 0;

void memory_paging_init()
{
    // prepare PDP, PDT
    unsigned long long *paging_PML4 = (unsigned long long *)PAGING_PML4_ADDR;
    unsigned long long *paging_PDPT = (unsigned long long *)PAGING_PDPT_ADDR;
    unsigned long long *paging_PD   = (unsigned long long *)PAGING_PD_ADDR;
    unsigned long long a;
    
    paging_PML4[0] = PAGING_PDPT_ADDR | PML4E_P;
    _memset(paging_PML4+1,0,4096-8*1);
    
    paging_PDPT[0] = PAGING_PD_ADDR | PDPTE_P;
    _memset(paging_PDPT+1,0,4096-8*1);
    
    // virtual address 0-16MB = physical address 0-16MB
    paging_PD[ 0] = a = 0ULL | (PDE_P|PDE_RW|PDE_US|PDE_PS|PDE_G); 
    paging_PD[ 1] = (a += 0x200000); 
    paging_PD[ 2] = (a += 0x200000); 
    paging_PD[ 3] = (a += 0x200000); 
    paging_PD[ 4] = (a += 0x200000); 
    paging_PD[ 5] = (a += 0x200000); 
    paging_PD[ 6] = (a += 0x200000); 
    paging_PD[ 7] = (a += 0x200000); 
    _memset(paging_PD+8,0,4096-8*8);
    
    memory_paging_initialized = 1;
}
void memory_paging_map_for_transfer(unsigned long long dst_addr, unsigned long long src_addr)
{
    unsigned long long *paging_PD = (unsigned long long *)PAGING_PD_ADDR;
    unsigned long long a;
    if (!memory_paging_initialized) 
	memory_paging_init();
    // map 5 2MB-pages for transfer up to 4*2MB.
    // virtual address 16MB 0x01000000
    paging_PD[ 8] = a = (dst_addr&(-2ULL<<20)) | (PDE_P|PDE_RW|PDE_US|PDE_PS); 
    paging_PD[ 9] = (a += 0x200000); 
    paging_PD[10] = (a += 0x200000); 
    paging_PD[11] = (a += 0x200000); 
    paging_PD[12] = (a += 0x200000); 
    // virtual address 32MB 0x02000000
    paging_PD[16] = a = (src_addr&(-2ULL<<20)) | (PDE_P|PDE_RW|PDE_US|PDE_PS); 
    paging_PD[17] = (a += 0x200000); 
    paging_PD[18] = (a += 0x200000); 
    paging_PD[19] = (a += 0x200000); 
    paging_PD[20] = (a += 0x200000); 
    // invalidate non-global TLB entries
    { int r0; 
      asm volatile ("movl %%cr3,%0; movl %0,%%cr3" : "=&q"(r0) : : "memory");
    }
}
void memory_paging_enable()
{
    if (!memory_paging_initialized) 
	memory_paging_init();
    // enable paging
    {
      int r0,r1;
      asm volatile (
	"movl %%cr0, %0; movl %%cr4, %1; \n\t"
	"orl  $0x80000001,%0; \n\t" // CR0.PE(bit0)|PG(bit31)
	"orl  $0x00000030,%1; \n\t" // CR4.PAE(bit5)|PSE(bit4)
	"movl %1, %%cr4; \n\t"  // set PAE|PSE
	"movl %2, %%cr3; \n\t"  // point to PDPT
	"movl %0, %%cr0; \n\t"  // set PE|PG
	"ljmp %3,$(1f) \n1:\t"  // flush instruction cache
	//"movl %4, %0; movl %0, %%ds; movl %0, %%es; movl %0, %%ss;" // reload DS,ES,SS
	"btsl $7, %1;    \n\t"  // CR4.PGE(bit7)
	"movl %1, %%cr4; \n\t"  // set PGE
	:"=&r"(r0),"=&r"(r1)
	:"r"(PAGING_PDPT_ADDR),
	 "i"(PROT_MODE_CSEG),
	 "i"(PROT_MODE_DSEG)
	:"memory");
    }
}
void memory_paging_disable()
{
    int r0;
    asm volatile (
	"movl %%cr0,%0;  andl %1,%0;  movl %0,%%cr0; \n\t"
	"ljmp %3,$(1f) \n1:\t"  // flush instruction cache
	"movl %%cr4,%0;  andl %2,%0;  movl %0,%%cr4; \n\t"
	"                xorl %0,%0;  movl %0,%%cr3; \n\t"
	:"=&a"(r0)
	:"i"(~(CR0_PG)),
	 "i"(~(CR4_PSE|CR4_PAE|CR4_PGE)),
	 "i"(PROT_MODE_CSEG)
	:"memory");
}

/*
Transfer data in memory.
Limitation:
code must be below 16MB as mapped by memory_paging_init function
*/
unsigned long long
grub_memmove64(unsigned long long dst_addr, unsigned long long src_addr, unsigned long long len)
{
    if (!len)      { errnum = 0; return dst_addr; }
    if (!dst_addr) { errnum = ERR_WONT_FIT; return 0; }

    // forward copy should be faster than backward copy
    // If src_addr < dst_addr < src_addr+len, forward copy is not safe, so we do backward copy in that case. 
#if 0
    unsigned char backward = ((src_addr < dst_addr) && (dst_addr < src_addr+len));  
#endif
    unsigned long highaddr = (unsigned long)( dst_addr       >>32)
			   | (unsigned long)((dst_addr+len-1)>>32)
			   | (unsigned long)( src_addr       >>32)
			   | (unsigned long)((src_addr+len-1)>>32);
    if ( highaddr==0 )
    { // below 4GB just copy it normally
	void *pdst = (void*)(unsigned long)dst_addr; 
	void *psrc = (void*)(unsigned long)src_addr; 
#if 0
	if (backward)
	    _memcpy_backward(pdst, psrc, len);
	else
	    _memcpy_forward(pdst, psrc, len);
#else
  grub_memmove(pdst, psrc, len);
#endif
	errnum = 0; return dst_addr;
    }
    else if ( (highaddr>>(52-32))==0 && (is64bit & IS64BIT_AMD64) && !DISABLE_AMD64)
    { // AMD64/IA32-e paging
	mem64 (1, dst_addr, src_addr, len);	/* 1 for MOVE */
	return dst_addr;
    }
    else if ( (highaddr>>(52-32))==0 && (is64bit & IS64BIT_PAE))
    { // PAE paging
	void *pdst = DST_VIRTUAL_PTR(dst_addr); 
	void *psrc = SRC_VIRTUAL_PTR(src_addr); 
	unsigned long long dsta = dst_addr, srca = src_addr;
	memory_paging_enable();
	memory_paging_map_for_transfer(dsta, srca);
	// transfer
	unsigned long long nr = len; // number of bytes remaining
	while (1)
	{
	    unsigned long n1 = (nr>=PAGINGTXSTEP)? PAGINGTXSTEP: (unsigned long)nr;  // number of bytes per round (8MB)
	    // Copy
#if 0
	    if (backward)
		_memcpy_backward(pdst, psrc, n1);
	    else
		_memcpy_forward(pdst, psrc, n1);
#else
		grub_memmove(pdst, psrc, n1);
#endif
	    // update loop variables
	    if ((nr -= n1)==0) break;
	    memory_paging_map_for_transfer((dsta+=n1), (srca+=n1));
	}
	memory_paging_disable();
	errnum = 0; return dst_addr;
    }
    else
    {
	errnum = ERR_WONT_FIT; return 0;
    }
}
unsigned long long 
grub_memset64(unsigned long long dst_addr, unsigned int data, unsigned long long len)
{
    if (!len)      { errnum=0; return dst_addr; }
    if (!dst_addr) { errnum = ERR_WONT_FIT; return 0; }
    unsigned long highaddr = (unsigned long)( dst_addr       >>32)
			   | (unsigned long)((dst_addr+len-1)>>32);
    if ( highaddr==0 )
    { // below 4GB
	_memset((void*)(unsigned long)dst_addr, data, len);
	errnum = 0; return dst_addr;
    }
    else if ( (highaddr>>(52-32))==0 && (is64bit & IS64BIT_AMD64) && !DISABLE_AMD64)
    { // AMD64/IA32-e paging
	mem64 (3, dst_addr, data, len);	/* 3 for SET */
	return dst_addr;
    }
    else if ( (highaddr>>(52-32))==0 && (is64bit & IS64BIT_PAE))
    { // PAE paging
	void *pdst = DST_VIRTUAL_PTR(dst_addr); 
	unsigned long long dsta = dst_addr;
	memory_paging_enable();
	memory_paging_map_for_transfer(dsta, dsta);
	// transfer
	unsigned long long nr = len; // number of bytes remaining
	while (1)
	{
	    unsigned long n1 = (nr>=PAGINGTXSTEP)? PAGINGTXSTEP: (unsigned long)nr;  // number of bytes per round (8MB)
	    // Copy
	    _memset(pdst, data, n1);
	    // update loop variables
	    if ((nr -= n1)==0) break;
	    dsta+=n1;
	    memory_paging_map_for_transfer(dsta, dsta);
	}
	memory_paging_disable();
	errnum = 0; return dst_addr;
    }
    else
    {
	errnum = ERR_WONT_FIT; return 0;
    }
}
int 
grub_memcmp64(unsigned long long str1addr, unsigned long long str2addr, unsigned long long len)
{
    if (!len)      { errnum=0; return 0; }
    unsigned long highaddr = (unsigned long)( str1addr       >>32)
			   | (unsigned long)((str1addr+len-1)>>32)
			   | (unsigned long)( str2addr       >>32)
			   | (unsigned long)((str2addr+len-1)>>32);
    if ( highaddr==0 )
    { // below 4GB
	return _memcmp((const char*)(unsigned long)str1addr,
	    (const char*)(unsigned long)str2addr, (unsigned long)len);
    }
    else if ( (highaddr>>(52-32))==0 && (is64bit & IS64BIT_AMD64) && !DISABLE_AMD64)
    { // AMD64/IA32-e paging
	return mem64 (2, str1addr, str2addr, len);	/* 2 for CMP */
    }
    else if ( (highaddr>>(52-32))==0 && (is64bit & IS64BIT_PAE))
    { // PAE paging
	void *p1 = DST_VIRTUAL_PTR(str1addr); 
	void *p2 = SRC_VIRTUAL_PTR(str2addr); 
	unsigned long long a1=str1addr, a2= str2addr;
	unsigned long long nr = len; // number of bytes remaining
	int r=0;
	memory_paging_enable();
	memory_paging_map_for_transfer(a1, a2);
	{
	    while (1)
	    {
		unsigned long n1 = (nr>=PAGINGTXSTEP)? PAGINGTXSTEP: (unsigned long)nr;  // number of bytes per round (8MB)
		// Compare
		r = _memcmp(p1, p2, n1);
		if (r) break;
		// update loop variables
		if ((nr -= n1)==0) break;
		memory_paging_map_for_transfer((a1+=n1), (a2+=n1));
	    }
	}
	memory_paging_disable();
	return r;
    }
    else
    {
	errnum = ERR_WONT_FIT; return 0;
    }
}
