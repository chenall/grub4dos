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

/* Find the next word from CMDLINE and return the pointer. If
   AFTER_EQUAL is non-zero, assume that the character `=' is treated as
   a space. Caution: this assumption is for backward compatibility.  */
char *
skip_to (int flags, char *cmdline)
{
	if (flags & 0x100)//skip to next line
	{
		char eol = flags & 0xff;
		if (eol == '\0')
			eol = ':';
		while (*cmdline)
		{
			if (*cmdline == '\r' || *cmdline == '\n')
			{
				*cmdline++ = 0;
				while (*cmdline == '\r' || *cmdline == '\n' || *cmdline == ' ' || *cmdline == '\t')
					cmdline++;
				if (*cmdline != eol)
					break;
			}
			cmdline++;
		}
		return *cmdline?cmdline:0;
	}
  /* Skip until we hit whitespace, or maybe an equal sign. */
  while (*cmdline && *cmdline != ' ' && *cmdline != '\t' &&
	 ! (flags && *cmdline == '='))
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

  /* Skip whitespace, and maybe equal signs. */
  while (*cmdline == ' ' || *cmdline == '\t' ||
	 (flags && *cmdline == '='))
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

#define PRINTF_BUFFER ((char *)0x1011000)
#define CMD_BUFFER ((char *)0x1010000)
int run_line (char *heap,int flags)
{
//	char *p;
	char *arg = heap;
	int ret = 0;
	int status = 0;
	struct builtin *builtin;
	int status_t = 0;
	int stat_bak = putchar_st.flag;
	while (*heap && (arg = heap))
	{
		putchar_st.flag = 0;
		heap = skip_to_next_cmd(heap,&status,0);//next cmd
		switch(status_t)
		{
			case 1:// operator "|"
				status_t = grub_strlen(arg);
				grub_memmove(CMD_BUFFER,arg,status_t);
				arg = skip_to (0, arg);
				if (*arg == 0)
					CMD_BUFFER[status_t++] = ' ';
				if (putchar_st.addr >= PRINTF_BUFFER + 0xC00)
					return !(errnum = ERR_WONT_FIT);
				grub_memmove(CMD_BUFFER + status_t,PRINTF_BUFFER,putchar_st.addr - PRINTF_BUFFER);
				arg = CMD_BUFFER;
				break;
			case 2:// operator ">"
			case 3:// operator ">>"
					if (! grub_open (arg))
						return 0;
					if (status_t & 1)//>> append
					{
						char *f_buf = CMD_BUFFER;
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
						grub_memset((char *)putchar_st.addr,0,filemax);
						putchar_st.addr = PRINTF_BUFFER + filemax;
					}

					if (grub_read ((unsigned long long)(int)PRINTF_BUFFER,putchar_st.addr - PRINTF_BUFFER,GRUB_WRITE) == 0)
					{
						grub_close();
						return 0;
					}
					grub_close();
					goto check_status;
			default:
				break;
		}

		if (status & 8)
		{
			putchar_st.addr = PRINTF_BUFFER;
			putchar_st.flag = status & 3;
		}

		builtin = find_command (arg);
		if ((int)builtin != -1)
		{
			if (! builtin || ! (builtin->flags & flags))
			{
				errnum = ERR_UNRECOGNIZED;
				break;
			}
			ret = (builtin->func) (skip_to (1,arg), flags);
		}
		else
			ret = command_func (arg,flags);

		if (status & 8)
		{
			status_t = status & 3;
			*putchar_st.addr++ = 0;
			continue;
		}

		check_status:
		if (status == 0 || (status & 12))
			break;
		errnum = 0;
		status_t = 0;
		if ((status == 1 && ret == 0) || (status == 2 && ret))
		{
			heap = skip_to_next_cmd(heap,&status,4);
		}
	}

	putchar_st.flag = stat_bak;
	putchar_st.addr = PRINTF_BUFFER;
	return ret;
}
#undef PRINTF_BUFFER 


/* Enter the command-line interface. HEAP is used for the command-line
   buffer. Return only if FOREVER is nonzero and get_cmdline returns
   nonzero (ESC is pushed).  */
void
enter_cmdline (char *heap, int forever)
{
  if (! debug)
      debug++;

  //grub_setjmp (restart_cmdline_env);

  /* Initialize the data and print a message.  */
  current_drive = GRUB_INVALID_DRIVE;
  count_lines = -1;
  kernel_type = KERNEL_TYPE_NONE;
  errnum = 0;
  errorcheck = 1;	/* errorcheck on */
  init_page ();
  grub_putchar ('\n');
#ifdef SUPPORT_DISKLESS
  print_network_configuration ();
  grub_putchar ('\n');
#endif
  print_cmdline_message (forever);
  
  while (1)
    {
      struct builtin *builtin;
//      char *arg;
      grub_error_t errnum_old;

      errnum_old = errnum;
      *heap = 0;
      if (errnum && errorcheck)
	print_error ();
      errnum = ERR_NONE;

      /* Get the command-line with the minimal BASH-like interface.  */
      get_cmdline_str.prompt = PACKAGE "> ";
      get_cmdline_str.maxlen = 2048;
      get_cmdline_str.echo_char = 0;
      get_cmdline_str.readline = 1;
      get_cmdline_str.cmdline=heap;
      if (get_cmdline (get_cmdline_str))
	{
	  kernel_type = KERNEL_TYPE_NONE;
	  return;
	}

      /* If there was no command, grab a new one. */
      if (! heap[0])
	continue;

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

      /* Start to count lines, only if the internal pager is in use.  */
      if (use_pager)
	count_lines = 0;
      
      if ((int)builtin != -1)
      if ((builtin->func) == errnum_func || (builtin->func) == checkrange_func)
	errnum = errnum_old;

      /* find && and || */
#if 0
      for (arg = skip_to (0, heap); *arg != 0; arg = skip_to (0, arg))
      {
	struct builtin *builtin1;
	int ret;
	char *arg1;
	arg1 = arg;
        if (*arg == '&' && arg[1] == '&' && (arg[2] == ' ' || arg[2] == '\t'))
        {
		/* handle the AND operator */
		arg = skip_to (0, arg);
		builtin1 = find_command (arg);
		if ((int)builtin1 != -1)
		if (! builtin1 || ! (builtin1->flags & BUILTIN_CMDLINE))
		{
			errnum = ERR_UNRECOGNIZED;
			goto next;
		}

		*arg1 = 0;
		if ((int)builtin != -1)
			ret = (builtin->func) (skip_to (1, heap), BUILTIN_CMDLINE);
		else
			ret = command_func (heap, BUILTIN_CMDLINE);
		*arg1 = '&';
		if (ret)
		{
			if ((int)builtin1 == -1 || ((builtin1->func) != errnum_func && (builtin1->func) != checkrange_func))
				errnum = 0;
			if ((int)builtin1 != -1)
				(builtin1->func) (skip_to (1, arg), BUILTIN_CMDLINE);
			else
				command_func (arg, BUILTIN_CMDLINE);
		} else
			errnum = 0;
		goto next;
	} else if (*arg == '|' && arg[1] == '|' && (arg[2] == ' ' || arg[2] == '\t'))
	{
		/* handle the OR operator */
		arg = skip_to (0, arg);
		builtin1 = find_command (arg);
		if ((int)builtin1 != -1)
		if (! builtin1 || ! (builtin1->flags & BUILTIN_CMDLINE))
		{
			errnum = ERR_UNRECOGNIZED;
			goto next;
		}

		*arg1 = 0;
		if ((int)builtin != -1)
			ret = (builtin->func) (skip_to (1, heap), BUILTIN_CMDLINE);
		else
			ret = command_func (heap, BUILTIN_CMDLINE);
		*arg1 = '|';
		if (! ret)
		{
			if ((int)builtin1 == -1 || ((builtin1->func) != errnum_func && (builtin1->func) != checkrange_func))
				errnum = 0;
			if ((int)builtin1 != -1)
				(builtin1->func) (skip_to (1, arg), BUILTIN_CMDLINE);
			else
				command_func (arg, BUILTIN_CMDLINE);
		} else
			errnum = 0;
		goto next;
	}
      }

	/* Run BUILTIN->FUNC.  */
	if ((int)builtin != -1)
	{
		arg = ((builtin->func) == commandline_func) ? heap : skip_to (1, heap);
		(builtin->func) (arg, BUILTIN_CMDLINE);
	}
	else
		command_func (heap, BUILTIN_CMDLINE);
#endif
	
	run_line (heap , BUILTIN_CMDLINE);
      /* Finish the line count.  */
      count_lines = -1;
    }
}
