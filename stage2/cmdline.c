/* cmdline.c - the device-independent GRUB text command line */
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

#ifdef SUPPORT_DISKLESS
# define GRUB	1
# include <etherboot.h>
#endif

//grub_jmp_buf restart_cmdline_env;

char *
wee_skip_to (char *cmdline, int flags)
{
	return skip_to (flags, cmdline);
}

/* Find the next word from CMDLINE and return the pointer. If
   AFTER_EQUAL is non-zero, assume that the character `=' is treated as
   a space. Caution: this assumption is for backward compatibility.  */
char *
skip_to (int flags, char *cmdline)
{
	if (flags & SKIP_LINE)//skip to next line
	{
		char eol = flags & 0xff;
		if (eol == '\0')
			eol = '#';
		while (*cmdline)
		{
			if (*cmdline == '\r' || *cmdline == '\n')
			{
				*cmdline++ = 0;
				while (*cmdline == '\r' || *cmdline == '\n' || *cmdline == ' ' || *cmdline == '\t')
					cmdline++;
				if (*cmdline != eol && *(unsigned short *)cmdline != 0x3A3A)
					break;
			}
			cmdline++;
		}
		return *cmdline?cmdline:0;
	}
  /* Skip until we hit whitespace, or maybe an equal sign. */
  while (*cmdline && !grub_isspace(*cmdline) &&
	 ! ((flags & 1) && *cmdline == '='))
  {
		if (*cmdline == '\"')
		{
			while (*++cmdline && *cmdline != '\"')
				;
		}
		else if (*cmdline == '\\')
		{
			cmdline ++;
		}

		if (*cmdline)
			cmdline ++;
  }
	//with Terminate
	if ((flags & SKIP_WITH_TERMINATE) && *cmdline)
		*cmdline++ ='\0';

  /* Skip whitespace, and maybe equal signs. */
  while (grub_isspace(*cmdline) ||
	 ((flags & 1)  && *cmdline == '='))
    cmdline ++;

  return cmdline;
}

/* Print a helpful message for the command-line interface.  */
void
print_cmdline_message (int forever)
{
  printf (" [ Minimal BASH-like line editing is supported.  For the first word, TAB\n"
	  "   lists possible command completions.  Anywhere else TAB lists the possible\n"
	  "   completions of a device/filename.%s ]\n",
	  (forever ? "" : "  ESC at any time exits."));
}

extern int command_func (char *arg, int flags);
extern int commandline_func (char *arg, int flags);
extern int errnum_func (char *arg, int flags);
extern int checkrange_func (char *arg, int flags);

/* Find the builtin whose command name is COMMAND and return the
   pointer. If not found, return 0.  */
struct builtin *
find_command (char *command)
{
  char *ptr;
  char c;
  struct builtin **builtin;

  if (! command)
	return 0;

  while (*command == ' ' || *command == '\t')command++;

  if (! *command)
	return 0;

  /* Find the first space and terminate the command name.  */
  ptr = command;
  while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '=')
    ptr ++;

  c = *ptr;
  *ptr = 0;

  /* Seek out the builtin whose command name is COMMAND.  */
  for (builtin = builtin_table; *builtin != 0; builtin++)
    {
      int ret = grub_strcmp (command, (*builtin)->name);

      if (ret == 0)
	{
	  /* Find the builtin for COMMAND.  */
	  *ptr = c;
	  return *builtin;
	}
      else if (ret < 0)
	break;
    }

  *ptr = c;

  /* Cannot find builtin COMMAND. Check if it is an executable file.  */
  if (command_func (command, 0))
    {
	return (struct builtin *)(char *)(-1);
    }

  /* Cannot find COMMAND.  */
  errnum = ERR_UNRECOGNIZED;
  return 0;
}

static char *skip_to_next_cmd (char *cmd,int *status,int flags)
{
//	*status = 0;
	if (cmd == NULL || *cmd == 0)
		return NULL;
	while (*(cmd = skip_to (0, cmd)))
	{
		switch (*(unsigned short *)cmd)
		{
			case 0x2626://	operator AND "&&"
				*status = 1;
				break;
			case 0x7C7C://	operator OR "||"
				*status = 2;
				break;
			case 0x2021://	! 
				*status = 4;
				break;
			case 0x207C:// |
			case 0x007C:
				*status = 8 | 1;
				break;
			case 0x203e: // >
				*status = 8 | 2;
				break;
			case 0x3e3e: // >>
				*status = 8 | 3;
				break;
			default:
				continue;
		}

		if (flags == 0 || *status == flags)
		{
			*(cmd - 1) = '\0';
			cmd = skip_to (0, cmd);
			break;
		}
	}

	if (*cmd == '\0')
		*status = 0;
	return cmd;
}

#define PRINTF_BUFFER ((unsigned char *)SYSTEM_RESERVED_MEMORY + 0x20000)
//char *pre_cmdline = (char *)0x4CB08;
static char *cmd_buffer = ((char *)SYSTEM_RESERVED_MEMORY - 0x10000);

int expand_var(const char *str,char *out,const unsigned int len_max)
{
	const char *p;
	int i;
	char *out_start = out;
	char *out_end = out + len_max - 1;
	while (*str && out < out_end)
	{
		if (*str != '%' || str[1] < '?' || !(i=envi_cmd(str,NULL,4)))
		{
			*out++=*str++;
			continue;
		}
		p = str + i++;
		if (*p != '%' && out+16 < out_end)
		{
			memmove(out,str,i);
			out += i;
			if (*p=='^')
			{
				--out,++i;
				*out++=p[1];
			}
		}
		else if (out + 0x200 < out_end)
		{
			out += envi_cmd(str,out,1);
		}
		str += i;
	}
	*out = '\0';
	return out - out_start;
}

int run_line (char *heap,int flags)
{
	char *arg = heap;
#define ret *(int*)0x4cb00
//	int ret = 0;
	int status = 0;
	struct builtin *builtin;
	int status_t = 0;
	unsigned char *hook_buff = 0;
	int i;
	grub_error_t errnum_old = errnum;
	char *cmdline_buf = cmd_buffer;
	char *cmdBuff = NULL;
	cmd_buffer += (expand_var(heap,cmdline_buf,0x600)+0x200)&-512;
	heap = cmdline_buf;
	errnum = ERR_NONE;
	while (*heap == 0x20 || *heap == '\t')
		++heap;
	if (*heap == 0)
		return 1;
	/* Invalidate the cache, because the user may exchange removable disks.  */
	buf_drive = -1;
	while (*heap && (arg = heap))
	{
		heap = skip_to_next_cmd(heap,&status,0);//next cmd
		switch(status_t)
		{
			case 1:// operator "|"
				cmdBuff = grub_malloc(0x20000);
				if (cmdBuff == NULL)
				{
					cmd_buffer = cmdline_buf;
					return 0;
				}
				i = grub_strlen(arg);
				grub_memmove(cmdBuff,arg,i);
				if (skip_to (0, arg) - arg == i)
					cmdBuff[i++] = ' ';
				cmdBuff[i] = 0;
				grub_strncat(cmdBuff,(const char *)PRINTF_BUFFER,0x20000);
				arg = cmdBuff;
				break;
			case 2:// operator ">"
			case 3:// operator ">>"
				if (substring(arg,"nul",1) == 0)
					goto restart_st;
				i = no_decompression;
				no_decompression = 1;
				if (! grub_open (arg))
				{
					no_decompression = i;
					goto restart_st;
				}
				no_decompression = i;
				if (status_t & 1)//>> append
				{
					char *f_buf = cmd_buffer;
					int t_read,t_len;
					while ((t_read = grub_read ((unsigned long long)(int)f_buf,0x400,GRUB_READ)))
					{
						f_buf[t_read] = 0;
						t_len = grub_strlen(f_buf);
						if (t_len < t_read)
						{
							filepos -= t_read - t_len;
							break;
						}
					}
				}
				else if (filemax < 0x40000)
				{
					grub_memset(hook_buff,0,filemax);
					hook_buff = PRINTF_BUFFER + filemax;
				}

				grub_read ((unsigned long long)(int)PRINTF_BUFFER,hook_buff - PRINTF_BUFFER,GRUB_WRITE);
				grub_close();

				restart_st:
				errnum = errnum_old;
				goto check_status;
			default:
				break;
		}

		if (status & 8)
		{
			if (substring(heap,"nul",1) == 0)
				hook_buff = set_putchar_hook((unsigned char *)0x800);	//hook_buff = set_putchar_hook(0x800);	2013.02.07
			else
				hook_buff = set_putchar_hook(PRINTF_BUFFER);
		}

		builtin = find_command (arg);

		if ((int)builtin != -1)
		{
			if (! builtin || ! (builtin->flags & flags))
			{
				errnum = ERR_UNRECOGNIZED;
				ret = 0;
			}
			else 
			{
				if ((builtin->func) == errnum_func || (builtin->func) == checkrange_func)
					errnum = errnum_old;
				ret = (builtin->func) (skip_to (1,arg), flags);
			}
		}
		else
			ret = command_func (arg,flags);

		errnum_old = errnum;
		if (arg == cmdBuff)
			grub_free(cmdBuff);

		if (status & 8)
		{
			status_t = status & 3;
			hook_buff = set_putchar_hook(hook_buff);
			if (substring(heap,"nul",1) != 0)
				*hook_buff++ = 0;
			continue;
		}

		check_status:
		if (*CMD_RUN_ON_EXIT == '\xEB')
		{
			errnum = -1;
			sprintf(CMD_RUN_ON_EXIT,"\xEC%.224s",arg);
			break;
		}
		if (errnum >= 1255 || status == 0 || (status & 12))
		{
			break;
		}
		if (errnum == ERR_BAT_GOTO && ret)
		{
			break;
		}
		status_t = 0;
		errnum = ERR_NONE;
		if ((status == 1 && !ret) || (status == 2 && ret))
		{
			heap = skip_to_next_cmd(heap,&status,4);
		}
	}
	cmd_buffer = cmdline_buf;
	return (errnum > 0 && errnum<MAX_ERR_NUM)?0:ret;
#undef ret
}
#undef PRINTF_BUFFER 


/* Enter the command-line interface. HEAP is used for the command-line
   buffer. Return only if FOREVER is nonzero and get_cmdline returns
   nonzero (ESC is pushed).  */
void
enter_cmdline (char *heap, int forever)
{
  int debug_old = debug;
  debug = 1;
  //grub_setjmp (restart_cmdline_env);

  /* show cursor and disable splashimage. */
  setcursor (1);

  /* Initialize the data and print a message.  */
  current_drive = GRUB_INVALID_DRIVE;
  count_lines = -1;
  kernel_type = KERNEL_TYPE_NONE;
  errnum = 0;
  errorcheck = 1;	/* errorcheck on */
  init_page ();
  grub_putchar ('\n', 255);
#ifdef SUPPORT_DISKLESS
  print_network_configuration ();
  grub_putchar ('\n', 255);
#endif
  print_cmdline_message (forever);
  
  while (1)
    {

//      struct builtin *builtin;
//      char *arg;
      grub_error_t errnum_old;

      errnum_old = errnum;

      *heap = 0;
      if (errnum && errorcheck)
	print_error ();
      errnum = ERR_NONE;

      /* Get the command-line with the minimal BASH-like interface.  */
      get_cmdline_str.prompt = (unsigned char*)PACKAGE "> ";
      get_cmdline_str.maxlen = 2048;
      get_cmdline_str.echo_char = 0;
      get_cmdline_str.readline = 1;
      get_cmdline_str.cmdline=(unsigned char*)heap;
      if (get_cmdline ())
	{
	  kernel_type = KERNEL_TYPE_NONE;
	  debug = debug_old;
	  return;
	}

      /* If there was no command, grab a new one. */
      if (! heap[0])
	continue;
/*commented by chenall 2010-12-16,will do it in run_line*/
#if 0
      /* Find a builtin.  */
      builtin = find_command (heap);
      if (! builtin)
	continue;

      /* If BUILTIN cannot be run in the command-line, skip it.  */
      if ((int)builtin != -1 && ! (builtin->flags & BUILTIN_CMDLINE))
	{
	  errnum = ERR_UNRECOGNIZED;
	  continue;
	}

      /* Invalidate the cache, because the user may exchange removable
	 disks.  */
      buf_drive = -1;
#endif
      /* Start to count lines, only if the internal pager is in use.  */
      if (use_pager)
	count_lines = 0;
/*commented by chenall 2010-12-16,will do it in run_line
      if ((int)builtin != -1)
      if ((builtin->func) == errnum_func || (builtin->func) == checkrange_func)
	errnum = errnum_old;
*/
	errnum = errnum_old;
	if (memcmp(heap,"clear",5))
	    putchar('\n',255);
	run_line (heap , BUILTIN_CMDLINE);
      /* Finish the line count.  */
      count_lines = -1;
    }
}
