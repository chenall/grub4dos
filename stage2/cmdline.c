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
skip_to (int after_equal, char *cmdline)
{
  /* Skip until we hit whitespace, or maybe an equal sign. */
  while (*cmdline && *cmdline != ' ' && *cmdline != '\t' &&
	 ! (after_equal && *cmdline == '='))
  {
	if (*cmdline == '\\')
	{
		cmdline ++;
		if (*cmdline == 0)
			break;
	}
	cmdline ++;
  }

  /* Skip whitespace, and maybe equal signs. */
  while (*cmdline == ' ' || *cmdline == '\t' ||
	 (after_equal && *cmdline == '='))
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
      char *arg;
      grub_error_t errnum_old;

      errnum_old = errnum;
      *heap = 0;
      if (errnum && errorcheck)
	print_error ();
      errnum = ERR_NONE;

      /* Get the command-line with the minimal BASH-like interface.  */
      prompt = PACKAGE "> ";
      maxlen = 2048;
      echo_char = 0;
      readline = 1;
      if (get_cmdline (heap))
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
#else
		char *p;
		arg = heap;
		for (p = arg; *p != 0; p = skip_to (0, p))
		{
			if (*p == '!' && (p[1] == ' ' || p[1] == '\t'))
			{
				*p++ = 0;
				break;
			}
			if (((*p == '&' && p[1] == '&') || (*p == '|' && p[1] == '|')) && (p[2] == ' ' || p[2] == '\t'))
			{
				/* handle the AND / OR operator */
				int ret;
				*p = 0;
				builtin = find_command (arg);
				if ((int)builtin != -1)
				{
					if (! builtin || ! (builtin->flags & BUILTIN_CMDLINE))
					{
						errnum = ERR_UNRECOGNIZED;
						goto next;
					}
					ret = (builtin->func) (skip_to (1,arg), BUILTIN_CMDLINE);
				}
				else
					ret = command_func (arg, BUILTIN_CMDLINE);
				p++;
				errnum = 0;
				if ((*p == '&' && ret) || (*p == '|' && ! ret))
				{
					arg = skip_to (0, p);
				}
				else
				{
					for (;*p ; p = skip_to (0, p))
					{
						if (*p == '!' && (p[1] == ' ' || p[1] == '\t'))
						{
							arg = skip_to (0,p);
							break;
						}
					}
					if (*p == '!' && (p[1] == ' ' || p[1] == '\t'))
					{
						p = arg;
						continue;
					}
					goto next;
				}
			}
		}
	if (! *arg) goto next;
	/* Run BUILTIN->FUNC.  */
	builtin = find_command (arg);
	if ((int)builtin != -1)
	{
		arg = ((builtin->func) == commandline_func) ? heap : skip_to(1,arg);
		(builtin->func) (arg, BUILTIN_CMDLINE);
	}
	else
		command_func (arg, BUILTIN_CMDLINE);
#endif
next:
      /* Finish the line count.  */
      count_lines = -1;
    }
}
