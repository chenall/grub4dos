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
//				if (*cmdline != eol && *(unsigned short *)cmdline != 0x3A3A)
				if (*cmdline != eol && *(unsigned short *)cmdline != 0x3A3A && *(unsigned short *)cmdline != 0x2023)
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
		if (*cmdline == QUOTE_CHAR)
		{
			while (*++cmdline && *cmdline != QUOTE_CHAR)
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
extern int else_disabled;  //else禁止
extern int brace_nesting;  //大括弧嵌套数

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
#define OPT_MULTI_CMD_FLAG 	0x3B3B
#define OPT_MULTI_CMD		(1<<4)
#define OPT_MULTI_CMD_AND_FLAG 	0x3B26
#define OPT_MULTI_CMD_AND	(1<<5)
#define OPT_MULTI_CMD_OR_FLAG  	0x3B7C
#define OPT_MULTI_CMD_OR	(1<<6)
static char *get_next_arg(char *arg)
{
	while(*arg && !isspace(*arg))
	{
		if (*arg == QUOTE_CHAR) while (*++arg && *arg != QUOTE_CHAR);
		if (*arg == '\\') ++arg;
		if (*arg) ++arg;
	}
	while (isspace(*arg)) ++arg;
	return arg;
}

static char *skip_to_next_cmd (char *cmd,int *status,int flags)
{
//	*status = 0;
	if (cmd == NULL || *cmd == 0)
		return NULL;

	while (*(cmd = get_next_arg(cmd)))
	{
		switch (*(unsigned short *)cmd)
		{
			case 0x2626://	operator AND "&&"
				*status = 1;
				break;
			case 0x7C7C://	operator OR "||"
				*status = 2;
				break;
			case 0x2021://	! else
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
			case OPT_MULTI_CMD_FLAG: //;;
				*status = OPT_MULTI_CMD;
				break;
			case OPT_MULTI_CMD_AND_FLAG:// &;
				*status = OPT_MULTI_CMD_AND;
				break;
			case OPT_MULTI_CMD_OR_FLAG:// |;
				*status = OPT_MULTI_CMD_OR;
				break;
      case 0x207b:  //  '{'
      case 0x007b:
      case 0x207d:  //  '}'
      case 0x007d:
        *(cmd - 1) = '\0';
        *(cmd + 1) = '\0';
        return cmd;
			default:
				continue;
		}

		char *p = cmd + 1;

		if ((flags == 0 || (*status & flags)) && (!*p || *p == ' ' || p[1] == ' '))
		{
			*(cmd - 1) = '\0';
			cmd = get_next_arg(cmd);
			break;
		}
//		*status = 0;
	}

	if (*cmd == '\0')
		*status = 0;
	return cmd;
}

#define PRINTF_BUFFER ((unsigned char *)SYSTEM_RESERVED_MEMORY + 0x20000)
//char *pre_cmdline = (char *)0x4CB08;
static char *cmd_buffer = ((char *)0x3A9000);

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
  if (len_max == 0x400)
  {
    char *q0 = out_start;
    char *q1 = out_start;

    while (*q0)
    {
      if (*q0 == '\\' && *(q0+1) == 'n')
      {
        *q1++ = '\n';
        q0 += 2;
      }
      else
        *q1++=*q0++;
    }
    *q1 = '\0';
    out = q1;
  }
	return out - out_start;
}
static int run_cmd_line (char *heap,int flags);
int run_line (char *heap,int flags)
{
   char *cmdline_buf = cmd_buffer;
   char *arg;
   int status = 0;
   int ret = 0;
   int arg_len = strlen(heap) + 1;
#if 0
   //吸收命令行尾部空格		会使得外部命令SISO、RUN列表文件时，扩展名只显示前2个！
   char *p = heap;
   while (*p++);
   while (*(p - 2) == ' ' || *(p - 2) == '\t')
   {
     *(p - 2) = 0;
     p--;
   }
#endif
   cmd_buffer += (arg_len + 0xf) & -0x10;
   memmove(cmdline_buf,heap,arg_len);
   heap = cmdline_buf;
#if 0
   __asm__ __volatile__ ("movl %%esp,%0" ::"m"(arg_len):"memory");
   if (arg_len < 0x3000)
   {
     errnum = ERR_BAD_ARGUMENT;
     printf("\nFAULT: <<<<<<<<<<SYSTETM STATCK RUNOUT>>>>>>>>>\n");
     return 0;
   }

   if (debug > 10) printf("SP:0x%X\n[%s]\n",arg_len,heap);
#endif
   while(*heap && (arg = heap))
   {
      heap = skip_to_next_cmd(heap,&status,OPT_MULTI_CMD_AND | OPT_MULTI_CMD_OR | OPT_MULTI_CMD);//next cmd
      ret = run_cmd_line(arg,flags);
      if (errnum > 1000) break;
    if (errnum == ERR_BAT_BRACE_END) break; //如果是批处理大括弧结束, 则退出
      if (((status & OPT_MULTI_CMD_AND) && !ret) || ((status & OPT_MULTI_CMD_OR) && ret))
      {
	 errnum = ERR_NONE;
	 heap = skip_to_next_cmd(heap,&status,OPT_MULTI_CMD);//next cmd
      }
   }
   cmd_buffer = cmdline_buf;
   return ret;
}

static int run_cmd_line (char *heap,int flags)
{
	char *arg = heap;
#define ret *(int*)0x4CB00
//	int ret = 0;
	int status = 0;
	struct builtin *builtin;
	int status_t = 0;
	unsigned char *hook_buff = 0;
	int i;
	grub_error_t errnum_old = errnum;
	char *cmdline_buf = cmd_buffer;
	char *cmdBuff = NULL;
	cmd_buffer += (expand_var(heap,cmdline_buf,0x600)+0x10)&-0x10;
	heap = cmdline_buf;
	errnum = ERR_NONE;
	while (*heap == 0x20 || *heap == '\t')
		++heap;
	if (*heap == 0 || *(unsigned short *)heap == 0x2023 || *(unsigned short *)heap == 0x3A3A)
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
//        if (skip_to (0, arg) - arg == i)
				if (skip_to (0, arg) - arg == i || cmdBuff[skip_to (0, arg) - arg] == ':') //修正找不到标签
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

		if (debug > 10 || debug_bat)
			printf("r0:[0x%X]:[%s]\n",arg,arg);

		if (status & 8)
		{
			if (substring(heap,"nul",1) == 0)
				hook_buff = set_putchar_hook((unsigned char*)0x800);
			else
			{
				grub_memset(PRINTF_BUFFER,0,0x40000);
				hook_buff = set_putchar_hook(PRINTF_BUFFER);
			}
		}

		builtin = find_command (arg);
    
    if (*arg == '{') //左大括弧
    {
      if (!ret)
        return !(errnum = ERR_BAT_BRACE_END);
      
      errnum = ERR_NONE;  //消除错误号 
      brace_nesting++;    //大括弧嵌套数+1
      return 1;
    }

    if (*arg == '}') //右大括弧
    {
      errnum = ERR_NONE;  //消除错误号 
      brace_nesting--;    //大括弧嵌套数-1
      else_disabled |= 1 << brace_nesting;  //设置else禁止位
      return 1;
    }
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
				#ifndef NO_DECOMPRESSION
				int no_decompression_bak = no_decompression;
				if (builtin->flags & BUILTIN_NO_DECOMPRESSION)
					no_decompression = 1;
				#endif
				ret = (builtin->func) (skip_to (1,arg), flags);
				#ifndef NO_DECOMPRESSION
				if (builtin->flags & BUILTIN_NO_DECOMPRESSION)
					no_decompression = no_decompression_bak;
				#endif
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
		    sprintf(CMD_RUN_ON_EXIT,"\xEC%.224s",arg);
		}
		if (*CMD_RUN_ON_EXIT == '\xEC')
		{
		    errnum = -1;
		    break;
		}
		if (errnum == MAX_ERR_NUM || errnum >= 2000 || status == 0 || (status & 12))
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
  print_cmdline_message (forever);
  
  while (1)
    {
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
	  if (debug == 1) debug = debug_old;
	  return;
	}

      /* If there was no command, grab a new one. */
      if (! heap[0])
	continue;
      /* Start to count lines, only if the internal pager is in use.  */
      if (use_pager)
	count_lines = 0;

      errnum = errnum_old;
	if (memcmp(heap,"clear",5))
	    putchar('\n',255);
	run_line (heap , BUILTIN_CMDLINE);
      /* Finish the line count.  */
      count_lines = -1;
    }
}
