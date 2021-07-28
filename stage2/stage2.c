/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2002,2004,2005  Free Software Foundation, Inc.
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

/* indicates whether or not the next char printed will be highlighted */
unsigned int is_highlight = 0;

static int open_preset_menu (void);	
static int
open_preset_menu (void)		
{
  if (! use_preset_menu)	//如果不存在预置菜单
    return 0;							//返回0

  return grub_open (preset_menu);		//打开压缩菜单 位置0x110000  最大尺寸0x40000=256k
}

static int read_from_preset_menu (char *buf, int max_len);
static int
read_from_preset_menu (char *buf, int max_len)
{
  if (! use_preset_menu)	//if (preset_menu == 0)
    return 0;

  return grub_read ((unsigned long long)(grub_size_t)buf, max_len, 0xedde0d90);
}

#define MENU_BOX_X	((menu_border.menu_box_x > 2) ? menu_border.menu_box_x : 2)
#define MENU_BOX_W	((menu_border.menu_box_w && menu_border.menu_box_w < (current_term->chars_per_line - MENU_BOX_X - 1)) ? menu_border.menu_box_w : (current_term->chars_per_line - MENU_BOX_X * 2 - 1))
#define MENU_BOX_Y	(menu_border.menu_box_y)
#define MENU_KEYHELP_Y_OFFSET  ((menu_border.menu_keyhelp_y_offset < 5) ? menu_border.menu_keyhelp_y_offset : 0)
/* window height */
#define MENU_BOX_H	((menu_border.menu_box_h && menu_border.menu_box_h < (current_term->max_lines - MENU_BOX_Y - 6 - MENU_KEYHELP_Y_OFFSET)) ? menu_border.menu_box_h : (current_term->max_lines - MENU_BOX_Y - 6 - MENU_KEYHELP_Y_OFFSET))
/* line end */
#define MENU_BOX_E	(MENU_BOX_X + MENU_BOX_W)

/* window bottom */
#define MENU_BOX_B	((menu_border.menu_box_b) ? menu_border.menu_box_b : (current_term->max_lines - 6 - MENU_KEYHELP_Y_OFFSET))
#define MENU_HELP_X ((menu_border.menu_help_x && menu_border.menu_help_x < current_term->chars_per_line) ? menu_border.menu_help_x : (MENU_BOX_X - 2))
#define MENU_HELP_E (menu_border.menu_help_x ? (menu_border.menu_help_w ? (menu_border.menu_help_x + menu_border.menu_help_w) : (current_term->chars_per_line - MENU_HELP_X)) : (MENU_BOX_E + 1))
#define NUM_LINE_ENTRYHELP ((MENU_KEYHELP_Y_OFFSET == 0) ? 4 : MENU_KEYHELP_Y_OFFSET)	//

static int temp_entryno;
static short temp_num;
static char * *titles;	/* title array, point to 256 strings. 标题数组，指向256个字符串*/
static unsigned short *title_boot;
static int default_help_message_destoyed = 1;
extern int num_text_char(char *p);
struct border menu_border = {218,191,192,217,196,179,2,0,2,0,0,2,0,0,0}; /* console */

static void print_help_message (const char *message,int flags);
static void print_help_message (const char *message,int flags)
{
	grub_u32_t j,x,k;
	grub_u8_t c = *message;
	int start_offcet = 0;

	if (flags==2)	
	{
		for (j=0; j<NUM_LINE_ENTRYHELP; ++j)
		{
			if (MENU_BOX_B + 1 + j > current_term->max_lines)
				return;
			gotoxy (MENU_HELP_X, MENU_BOX_B + 1 + j);
			for (x = MENU_HELP_X; x < (grub_u32_t)MENU_HELP_E; x++)
				grub_putchar (' ', 255);
		}
	}
	else
	{
		if(flags==0)
			k = 4;
		else
			k = NUM_LINE_ENTRYHELP;

		for (j=0; j<k; ++j)
		{
			if(flags==0)
			{
				if (MENU_BOX_B + 1 + menu_border.menu_keyhelp_y_offset + j > current_term->max_lines)
					return;
				gotoxy (MENU_HELP_X, MENU_BOX_B + 1 + menu_border.menu_keyhelp_y_offset + j);
			}
			else if(flags==1)
			{
				if (MENU_BOX_B + 1 + j > current_term->max_lines)
					return;
				gotoxy (MENU_HELP_X, MENU_BOX_B + 1 + j);
			}

			if(flags==1 || flags==0)
			{
				if (c == '\n')
					c = *(++message);
				
				if((menu_tab & 8)	&& flags==1)
          start_offcet = MENU_HELP_X + ((MENU_HELP_E - MENU_HELP_X - num_text_char((char *)message))>>1);
				else if((menu_tab & 0x40) && flags==1)
				{
					start_offcet = MENU_HELP_E - num_text_char((char *)message);
					if(start_offcet<MENU_HELP_X)
						start_offcet=MENU_HELP_X;
				}

				for (; fontx < (grub_u32_t)MENU_HELP_E;)
				{
					if (c && c != '\n' && fontx >= (grub_u32_t)start_offcet)
					{
						if (fontx == (grub_u32_t)MENU_HELP_E - (grub_u32_t)1)
							if (c > 0x7F && !(menu_tab & 0x40))
							{
								grub_putchar (' ', 255);
								continue;
							}
						grub_putchar (c, 255);
						c = *(++message);
					}
					else
						grub_putchar (' ', 255);
				}
				for (x = fontx; x < (grub_u32_t)MENU_HELP_E; x++)
					grub_putchar (' ', 255);
			}
		}
	}
}

static void print_default_help_message (char *config_entries);
static void
print_default_help_message (char *config_entries)
{
	grub_u32_t	i;
	char		buff[256];

if (menu_tab & 0x20)
		i = grub_sprintf (buff,"\n按↑和↓选择菜单。");
	else
		i = grub_sprintf (buff,"\nUse the \xe2\x86\x91 and \xe2\x86\x93 keys to highlight an entry.");
      if (! auth && password_buf)
	{
		if (menu_tab & 0x20)
			grub_strcpy (buff + i,"按回车或b启动。\n"
			"按p获得特权控制。");
		else
			grub_strcpy (buff + i,"Press ENTER or \'b\' to boot.\n"
			"Press \'p\' to gain privileged control.");
	}
	else
	{
		if (menu_tab & 0x20)
			grub_strcpy(buff + i,(config_entries?("按回车或b启动。\n"
			"按e可在启动前编辑菜单命令,按c进入命令行。")
			:("在当前行按e进行编辑,按d进行删除。\n"
			"按O和o分别在其之前或之后插入新行。\n"
			"编辑结束时按b启动,按c进入命令行,按ESC退回到主菜单。")));
		else
			grub_strcpy(buff + i,(config_entries?(" Press ENTER or \'b\' to boot.\n"
			"Press \'e\' to edit the commands before booting, or \'c\' for a command-line.")
			:(" At a selected line, press \'e\' to edit, \'d\' to delete,\n"
			"or \'O\'/\'o\' to open a new line before/after.\n"
			"When done, press \'b\' to boot, \'c\' for a command-line,\n"
			"or ESC to go back to the main menu.")));
	}
	if(keyhelp_color)
	{
		if (!(splashimage_loaded & 2))
		{
			current_color_64bit = keyhelp_color | (console_color_64bit[COLOR_STATE_NORMAL] & 0x00ffffff00000000);
		}
		else
			current_color_64bit = keyhelp_color;
		current_term->setcolorstate (color_64_to_8 (current_color_64bit) | 0x100);
	}
	print_help_message(buff,0);
	default_help_message_destoyed = 0;
}

// ADDED By STEVE6375
static int num_entries;

static char *get_entry (char *list, int num);
static char *
get_entry (char *list, int num)
{
  int i;

  if (list == (char *)titles)
	return num < num_entries ? titles[num] : (int)0;

  for (i = 0; i < num && *list; i++)
    {
	while (*(list++));
    }

  return list;
}

static char ntext[256];

static void clean_entry (char *text);
static void
clean_entry (char *text)
{
  int i;
  //(void) strcpy (ntext, text);
  for (i = 0; i < 255 && text[i]; ++i)
  {
    int a = (int)(char)text[i];
//	if (text[i] < (char)32 || text[i] > (char)127)
  if (a < (int)32 || a > (int)127)
		ntext[i] = 32;
	else
		ntext[i] = text[i];
  }
  ntext[i] = 0;
  return;
}

static int checkvalue (void);
static int
checkvalue (void)
{
  int i, value;
// Converts a string of ASCII numbers in a string to an integer
// leading spaces are allowed
// followed by a number of digits terminated by any non-digit character or EOS nul character
// return value is number of valid numeric characters found  (0 = no valid number found)
  for (i = 0, value = 0; ntext[i] != '\0'; ++i)
  {
    if ( (ntext[i] - '0' >= 0) && (ntext[i] - '0' <= 9) )
	++value;
    if ( (value == 0)  && (ntext[i] != 32)
		&& (ntext[i] - '0' < 0 || ntext[i] - '0' > 9) )
	return 0;
  }
  return value;
}

static int myatoi (void);
static int
myatoi (void)
{
  int i, value, j;

  for (i = 0, j = 0, value = 0; ntext[i] != '\0'; ++i)
  {
    if ( (ntext[i] - '0' >= 0) && (ntext[i] - '0' <= 9) ) 
     {
	value *= 10;
	value += ntext[i] - '0';
	j = 1;
     }
     else 
     {
	if ( ntext[i] != ' ' || j )
		return value;
     }
  }
  return value;
}

// END OF STEVE6375 ADDED CODE
int num_text_char(char *p);
int
num_text_char(char *p)
{
	int i=0;
	unsigned int un16=0;

	for(;*p && *p != '\n' && *p != '\r';)
	{
		if((*p & 0x80) == 0)
		{
			p++;
			i++;
			continue;
		}else if((*p & 0xe0) == 0xc0)
		{
			p+=2;
			i++;
			continue;
		}else if((*p & 0xf0) == 0xe0)
		{
			un16=(int)(*p & 0xf)<<12 | (int)(*(p+1) & 0x3f)<<6 | (int)(*(p+2) & 0x3f);
			if(un16 >= 0x4e00 && un16 <= 0x9fff)
				i+=2;
			else
				i++;
			p+=3;
		}
	}

	return i;
}

/* Print an entry in a line of the menu box.  */
char menu_cfg[2];
unsigned char menu_num_ctrl[4];
int use_phys_base;
char graphic_file_shift[32];

static void print_entry (int y, int highlight,int entryno, char *config_entries);
static void
print_entry (int y, int highlight,int entryno, char *config_entries)
{
  int x;
  unsigned char c = 0;
  char *entry = get_entry (config_entries, entryno);
	int start_offcet = 0;
	int end_offcet = 0;
	int first_brackets = 0;
	
  if (current_term->setcolorstate)
    current_term->setcolorstate (highlight ? COLOR_STATE_HIGHLIGHT : COLOR_STATE_NORMAL);
  
	unsigned long long clo64 = current_color_64bit;
	unsigned int clo = current_color;
  is_highlight = highlight;
	if (graphic_type)
	{
		char tmp[128];
		int file_len=grub_strlen(graphic_file);
		int www = (font_w * MENU_BOX_W) / graphic_list;
		int graphic_x_offset, graphic_y_offset, text_x_offset;
	
    int text_y_offset = MENU_BOX_Y +
        (((y-MENU_BOX_Y)/graphic_list) * (graphic_high+row_space) + (graphic_high-font_h)/2) / (font_h+line_spacing);
		graphic_x_offset = font_w*MENU_BOX_X+((y-MENU_BOX_Y)%graphic_list)*(www);

		if (graphic_type & 0x10)
      graphic_y_offset = (font_h+line_spacing)*text_y_offset - (graphic_high-font_h)/2;
		else
			graphic_y_offset = font_h*MENU_BOX_Y+((y-MENU_BOX_Y)/graphic_list)*(graphic_high+row_space);

		if (entryno >= num_entries)
		{
			clear_entry (graphic_x_offset,graphic_y_offset,graphic_wide,graphic_high);
			entry++;
		}
		else
		{
		graphic_file[file_len-6] = ((entryno + graphic_file_shift[entryno]) / 10) | 0x30;
		graphic_file[file_len-5] = ((entryno + graphic_file_shift[entryno]) % 10) | 0x30;
		sprintf(tmp,"--offset=%d=%d=%d %s",0,graphic_x_offset,graphic_y_offset,graphic_file);
		use_phys_base=1;
		graphic_enable = 1;
		splashimage_func(tmp,1);
		graphic_enable = 0;
		use_phys_base=0;
		if ((graphic_type & 4) && highlight)
			rectangle(graphic_x_offset,graphic_y_offset,graphic_wide,graphic_high,3);
		}
		c = *entry;
		if (graphic_type & 0x10)
		{
			text_x_offset = (graphic_x_offset + graphic_wide + font_w -1) / (font_w + font_spacing);
			end_offcet = MENU_BOX_E - ((www - graphic_wide) / (font_w + font_spacing)) - text_x_offset;
			gotoxy (text_x_offset, text_y_offset);
			goto graphic_mixing;
		}	
		else
			goto graphic_end;
	}

  gotoxy (MENU_BOX_X, y);
  if(menu_tab & 0x40)
    end_offcet = 1;
  if (highlight)
		menu_num_ctrl[2] = entryno;
//  if (*(unsigned short *)IMG(0x8308))
  if (*(unsigned short *)IMG(0x82c8))
  {
  if(!(menu_tab & 0x40))
	{
		gotoxy (MENU_BOX_X - 1, y);
		if (graphics_inited && graphics_mode > 0xff)	//在图像模式
			grub_putchar(highlight ? (menu_num_ctrl[2] = entryno, menu_cfg[0]) : ' ', 255);
		else
			grub_putstr(highlight ? (menu_num_ctrl[2] = entryno, "\xe2\x96\xba") : "\x20");
	}
	else
	{
		gotoxy (MENU_BOX_E - 1, y);
		if (graphics_inited && graphics_mode > 0xff)	//在图像模式
			grub_putchar(highlight ? (menu_num_ctrl[2] = entryno,menu_cfg[1]) : ' ', 255);
		else
			grub_putstr(highlight ? (menu_num_ctrl[2] = entryno, "\xe2\x97\x84") : "\x20");
	}
  }
graphic_mixing:
  if (entry)
  {
	if (config_entries == (char*)titles)
	{
		c = *entry++;
		expand_var (entry, (char *)SCRATCHADDR, 0x400);
		entry = (char *)SCRATCHADDR;
		if (menu_num_ctrl[0])
		{
      if(menu_tab & 0x40)
      {
        gotoxy (MENU_BOX_E - 4, y);
        end_offcet = 4;
      }

			if ((!(c & menu_num_ctrl[0]) && menu_num_ctrl[0] == 1) || !*entry || *entry == '\n')
				printf("   ");
			else
			{
				if(!(menu_tab & 0x40))
					printf("%2d%c",(menu_num_ctrl[0] > 1)?entryno:title_boot[entryno],menu_num_ctrl[1]);
				else
					printf("%c%2d",menu_num_ctrl[1],(menu_num_ctrl[0] > 1)?entryno:title_boot[entryno]);
			}
		}
	}
	c = *entry;
  }

  if(menu_tab & 0x40)
    gotoxy (MENU_BOX_X, y);

  if(entry)
  {
  if(menu_tab & 8)
  {
    if(!(menu_tab & 0x40))
      start_offcet = MENU_BOX_X + ((MENU_BOX_W - num_text_char(entry) + (menu_num_ctrl[0]?3:0)) >> 1);
    else
      start_offcet = MENU_BOX_X + ((MENU_BOX_W - num_text_char(entry) - end_offcet) >> 1);
  }
  else if((menu_tab & 0x40))
	{
    start_offcet = MENU_BOX_E - num_text_char(entry) - end_offcet;
		if(start_offcet < MENU_BOX_X)
			start_offcet = MENU_BOX_X;
	}
  }

  for (x = fontx; x < MENU_BOX_E - end_offcet; x = fontx)
    {
      int ret;

      ret = MENU_BOX_E - x - end_offcet;
      if (c && c != '\n' /* && x <= MENU_BOX_W*/ && x >= start_offcet)
	{
		
    if (hotkey_func && hotkey_color_64bit)
    {
      if (c == '^')
      {      
        c = *(++entry);
        goto color;
      }
      else if (first_brackets == 0 && c == '[')
      {
        first_brackets = 1;
        goto color_no;
      }
      else if (first_brackets == 1 && c == ']')
      {
        first_brackets = 2;
        goto color_no;
      }
      else if (first_brackets == 0 || first_brackets == 2)
      {
        first_brackets = 2;
        goto color_no;
      }
color:
      current_color_64bit = hotkey_color_64bit;
      current_color = hotkey_color;
    }
color_no:
		ret = grub_putchar ((unsigned char)c, ret);
    current_color_64bit = clo64;
    current_color = clo;
		//is_highlight = 0;
		if (ret < (int)0)
		{
			c = 0;
			continue;
		}
		c = *(++entry);
	}
      else
	{
		if (!(menu_tab & 0x10))
		{
		clo64 = current_color_64bit;
		if(splashimage_loaded & 2)
			current_color_64bit = 0;
		else
			if (current_term->setcolorstate)
				current_term->setcolorstate (COLOR_STATE_NORMAL);
		ret = grub_putchar (' ', ret);
		current_color_64bit = clo64;
		}
		else
			grub_putchar (' ', ret);
	}
    }
graphic_end:
  is_highlight = 0;

  if (highlight && ((config_entries == (char*)titles)))
  {
	if (current_term->setcolorstate)
	    current_term->setcolorstate (COLOR_STATE_HELPTEXT);
	
	while (c && c != '\n')
		c = *(++entry);

	if (c == '\n')
	{
		default_help_message_destoyed = 1;
		print_help_message(entry,1);
	}
	else
	{
		if(!(menu_border.menu_keyhelp_y_offset) && !(menu_tab & 4))
		{
			if (default_help_message_destoyed)
				print_default_help_message (config_entries);
		}
		else
			print_help_message(entry,2);
	}
  }
  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_STANDARD);
}

/* Print entries in the menu box.  */
static void print_entries (int first, int entryno, char *menu_entries);
static void
print_entries (int first, int entryno, char *menu_entries)
{
  int i;

  gotoxy (MENU_BOX_E, MENU_BOX_Y);

#ifdef SUPPORT_GRAPHICS
  if (!graphics_inited || graphics_mode < 0xff)
#endif
  {
    if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_BORDER);
  if (first)
		grub_putstr ("\xe2\x86\x91");
  else
		grub_putstr ("\xe2\x94\x82");
  }
  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_NORMAL);
  for (i = 0; i < MENU_BOX_H/*size*/; i++)
    {
      print_entry (MENU_BOX_Y + i, entryno == i, first + i, menu_entries);
    }

#ifdef SUPPORT_GRAPHICS
  if (!graphics_inited || graphics_mode < 0xff)
#endif
{
  gotoxy (MENU_BOX_E, MENU_BOX_Y - 1 + MENU_BOX_H/*size*/);

  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_BORDER);
  char *last_entry = get_entry(menu_entries,first+MENU_BOX_H);
  if (last_entry && *last_entry)
		grub_putstr ("\xe2\x86\x93");
  else
		grub_putstr ("\xe2\x94\x82");
  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_STANDARD);
}
  gotoxy (MENU_BOX_E, MENU_BOX_Y + entryno);	/* XXX: Why? */
}

static void print_entries_raw (int size, int first, char *menu_entries);
static void
print_entries_raw (int size, int first, char *menu_entries)
{
  int i;
  char *p;

  for (i = 0; i < MENU_BOX_W; i++)
    grub_putchar ('-', 255);
  grub_putchar ('\n', 255);

  for (i = first; i < size; i++)
    {
      p = get_entry (menu_entries, i);
      grub_printf ("%02d: %s\n", i, (((*p) & 0xF0) ? p : ++p));
    }

  for (i = 0; i < MENU_BOX_W; i++)
    grub_putchar ('-', 255);
  grub_putchar ('\n', 255);
  grub_putchar ('\n', 255);
}


extern int command_func (char *arg1, int flags);
extern int commandline_func (char *arg1, int flags);
extern int errnum_func (char *arg1, int flags);
extern int checkrange_func (char *arg1, int flags);

/* Run an entry from the script SCRIPT. HEAP is used for the
   command-line buffer. If an error occurs, return non-zero, otherwise
   return zero.  */
static int run_script (char *script, char *heap);
static int
run_script (char *script, char *heap)
{
  char *old_entry = 0;
  char *cur_entry = script;
  struct builtin *builtin = 0;
	char cmd_add[16];
	char *menu_bat;
	char *p;

  /* Initialize the data.  */
  current_drive = GRUB_INVALID_DRIVE;
  count_lines = -1;
  
  /* Initialize the data for the builtin commands.  */
  kernel_type = KERNEL_TYPE_NONE;
  errnum = 0;
  errorcheck = 1;	/* errorcheck on */
  if (grub_memcmp (cur_entry, "!BAT", 4) == 0)
  {
    while (1)
    {
      while (*cur_entry++);
      if (! *cur_entry)
        break;
      *(cur_entry - 1) = 0x0a;
    }
    menu_bat = (char *)grub_malloc(cur_entry - script + 10 + 512);
    if (menu_bat == NULL)
      return 0;
    p = (char *)(((grub_size_t)menu_bat + 511) & ~511);
    grub_memmove (p, script, cur_entry - script);
    grub_sprintf (cmd_add, "(md)%d+%d", (grub_size_t)p >> 9, ((cur_entry - script + 10 + 511) & ~511) >> 9);
    command_func (cmd_add, BUILTIN_SCRIPT);
    grub_free(menu_bat);
    menu_bat = 0;
    if (errnum >= 1000)
    {
      errnum=ERR_NONE;
      return 0;
    }
    if (errnum && errorcheck)
    goto ppp;
	
    /* If any kernel is not loaded, just exit successfully.  */
    if (kernel_type == KERNEL_TYPE_NONE)
      return 0;	/* return to main menu. */
    /* Otherwise, the command boot is run implicitly.  */
    grub_sprintf (cmd_add, "boot", 5);
    run_line (cmd_add , BUILTIN_SCRIPT);

    goto ppp;
  }

  while (1)
  {
    if (errnum == MAX_ERR_NUM)
		{
			errnum=ERR_NONE;
			return 0;
		}
    if (errnum && errorcheck)
      break;

    old_entry = cur_entry;
    while (*cur_entry++)
    ;

    grub_memmove (heap, old_entry, (grub_size_t) cur_entry - (grub_size_t) old_entry);
    if (! *heap)
    {
      /* If there is no more command in SCRIPT...  */
      /* If any kernel is not loaded, just exit successfully.  */
      if (kernel_type == KERNEL_TYPE_NONE)
        return 0;	/* return to main menu. */

      /* Otherwise, the command boot is run implicitly.  */
      grub_memmove (heap, "boot", 5);
    }

    /* Find a builtin.  */
    builtin = find_command (heap);
    if (! builtin)
    {
      grub_printf ("%s\n", old_entry);
      continue;
    }
    run_line (heap , BUILTIN_SCRIPT);

    if (! *old_entry)	/* HEAP holds the implicit BOOT command */
      break;
  } /* while (1) */

ppp:
  kernel_type = KERNEL_TYPE_NONE;

  /* If a fallback entry is defined, don't prompt a user's intervention.  */
  
  if (fallback_entryno < 0)
  {
    print_error ();

    grub_printf ("\nPress any key to continue...");
    (void) getkey ();
  }
	  
  errnum = ERR_NONE;
  return 1;	/* use fallback. */
}

void clear_delay_display (int entryno);
void
clear_delay_display (int entryno)
{
  if (grub_timeout >= 0)
  {
    if (current_term->setcolorstate)
		  current_term->setcolorstate (COLOR_STATE_HELPTEXT);
    if (current_term->flags & TERM_DUMB)
      grub_putchar ('\r', 255);
    
		if(timeout_x || timeout_y)
			gotoxy (timeout_x,timeout_y);
		else
		{
      if(!(menu_tab & 0x40))
        gotoxy (MENU_BOX_E - 2, MENU_BOX_Y + entryno);
      else
        gotoxy (MENU_BOX_X - 1, MENU_BOX_Y + entryno);
		}
    
		if(timeout_color)
		{
			current_term->setcolorstate (COLOR_STATE_NORMAL);
			if ((timeout_color & 0xffffffff00000000) == 0)
				current_color_64bit = timeout_color | (current_color_64bit & 0xffffffff00000000);
			else
				current_color_64bit = timeout_color;
		}
		else if (current_term->setcolorstate)
      current_term->setcolorstate (COLOR_STATE_HIGHLIGHT);
	
    grub_printf("  ");
    if (current_term->setcolorstate)
      current_term->setcolorstate (COLOR_STATE_HELPTEXT);
				
    grub_timeout = -1;
    fallback_entryno = -1;
    if (! (current_term->flags & TERM_DUMB))
      gotoxy (MENU_BOX_E, MENU_BOX_Y + entryno);
  }
}

unsigned short beep_buf[256];
int new_menu;
int new_hotkey;
int color_counting;
int password_x;
static int fallbacked_entries;
static int old_c;
static int old_c_count;
static int old_c_count_end;

static void run_menu (char *menu_entries, char *config_entries, /*int num_entries,*/ char *heap, int entryno);
static void
run_menu (char *menu_entries, char *config_entries, /*int num_entries,*/ char *heap, int entryno)
{
  int i, time1, time2 = -1, first_entry = 0;
	unsigned short c;
  char *cur_entry = 0;
  char *pass_config = 0;
	color_counting = 0;
		  
  /*
   *  Main loop for menu UI.
   */
  if (password_buf)//Make sure that PASSWORD is NUL-terminated.
    pass_config = wee_skip_to(password_buf,SKIP_WITH_TERMINATE);
		
	/* clear keyboard buffer before boot */
	while ((unsigned short)console_checkkey () == 0x000d);

restart1:
  //clear temp_num when restart menu
  temp_num = 0;
	font_spacing = menu_font_spacing;
	line_spacing = menu_line_spacing;
  /* Dumb terminal always use all entries for display 
     invariant for TERM_DUMB: first_entry == 0  */
  if (! (current_term->flags & TERM_DUMB))
  {
    if (entryno > MENU_BOX_H - 1)
    {
      first_entry += entryno - (MENU_BOX_H - 1);
      entryno = MENU_BOX_H - 1;
    }
  }

  /* If the timeout was expired or wasn't set, force to show the menu
     interface. */
  if (grub_timeout < 0)
    show_menu = 1;
  
	cls();
  /* If SHOW_MENU is false, don't display the menu until a key-press.  */
  if (! show_menu)
  {
    /* Get current time.  */
    while ((time1 = getrtsecs ()) == 0xFF)
    ;
  
    while (1)
    {
      /* Unhide the menu on any keypress.  */
      if ((i = checkkey ()) != (int)-1 /*&& ASCII_CHAR (getkey ()) == '\e'*/)
	    {
        c = i;
	      if (silent_hiddenmenu > 1)
	      {
          if (c != silent_hiddenmenu)
            goto boot_entry;
          if (password_buf)
	      	{
            if (check_password (password_buf, password_type))
            {
              grub_printf ("auth failed!\n");
              grub_timeout = 5;
              continue;
            }
            if (*pass_config)
            {
              strcpy(config_file,pass_config);
              auth = 0;
              return;
            }
            auth = 1;
	      	}
	      }
	      grub_timeout = -1;
	      show_menu = 1;
	      break;
	    }

      /* If GRUB_TIMEOUT is expired, boot the default entry.  */
      if (grub_timeout >=0
          && (time1 = getrtsecs ()) != time2
	      /* && time1 != 0xFF */)
	    {
	      if (grub_timeout <= 0)
        {
          grub_timeout = -1;
          goto boot_entry;
        }
	      
	      time2 = time1;
	      grub_timeout--;
	      
	      /* Print a message.  */
	      if (! silent_hiddenmenu)
	      {
          grub_printf ("\rPress any key to enter the menu... %d   ",
          grub_timeout);
	      }
	    }
    }
  }

  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_NORMAL);

  setcursor (2);
  cls();	//clear screen so splash image will work
  if (animated_type)
    animated_enable = animated_enable_backup;

  /* Only display the menu if the user wants to see it. */
  if (show_menu)
  {
    cls ();
    if (!(menu_tab & 0x80))
      init_page ();
		if (graphics_inited && graphics_mode > 0xff)/*vbe mode call rectangle_func*/
		{
			unsigned long long col = current_color_64bit;
			for (i=0; i<16; i++)
			{
				if (DrawBox[i].enable == 0)
					continue;	
				current_color_64bit = DrawBox[i].color;
				rectangle(DrawBox[i].start_x, DrawBox[i].start_y, DrawBox[i].horiz, DrawBox[i].vert, DrawBox[i].linewidth);
			}
			current_color_64bit = col;
		}
	
    if (num_string)
    {
      int j;
      char y;
      for (j=0; j<16; j++)
      {
        if (strings[j].enable == 0)
          continue;	
        if (strings[j].start_y < 0)
          y = strings[j].start_y + current_term->max_lines;
        else
          y = strings[j].start_y;
        gotoxy (strings[j].start_x, y);

        if (!(strings[j].color & 0xffffffff00000000))
        {
          if (!(splashimage_loaded & 2))
              current_color_64bit = strings[j].color | (console_color_64bit[COLOR_STATE_NORMAL] & 0xffffffff00000000);
          else
            current_color_64bit = strings[j].color | (current_color_64bit & 0xffffffff00000000);
        }
        else
          current_color_64bit = strings[j].color | 0x1000000000000000;
        
        current_term->setcolorstate (color_64_to_8 (current_color_64bit & 0x00ffffffffffffff) | 0x100);
        grub_printf("%s",strings[j].string);
        current_term->setcolorstate (COLOR_STATE_NORMAL);
      }
    }
			
    if (current_term->flags & TERM_DUMB)
      print_entries_raw (num_entries, first_entry, menu_entries);
    else /* print border */
#ifdef SUPPORT_GRAPHICS
    if (graphics_inited && graphics_mode > 0xff)/*vbe mode call rectangle_func*/
    {
      if (current_term->setcolorstate)
        current_term->setcolorstate (COLOR_STATE_BORDER);
      unsigned int x,y,w,h,j;
      i = font_w + font_spacing;
      j = font_h + line_spacing;
      x = (MENU_BOX_X - 2) * i + (i>>1);
      y = (MENU_BOX_Y)*j-(j>>1);
      w = (MENU_BOX_W + 2) * i;
      if (graphic_type)
        h = (graphic_high+row_space) * graphic_row;
      else
      h = (MENU_BOX_H + 1) * j;
      rectangle(x,y,w,h,menu_border.border_w);
    }
    else
#endif
    {
      if (current_term->setcolorstate)
        current_term->setcolorstate (COLOR_STATE_BORDER);
  
        /* upper-left corner */
      gotoxy (MENU_BOX_X - 2, MENU_BOX_Y - 1);
      grub_putstr("\xe2\x94\x8c");	

      /* top horizontal line */
      for (i = 0; i < MENU_BOX_W + 1; i++)
        grub_putstr ("\xe2\x94\x80");

      /* upper-right corner */
      grub_putstr ("\xe2\x94\x90");

      for (i = 0; i < MENU_BOX_H; i++)
	    {
	      /* left vertical line */
	      gotoxy (MENU_BOX_X - 2, MENU_BOX_Y + i);
				grub_putstr ("\xe2\x94\x82");
	      /* right vertical line */
	      gotoxy (MENU_BOX_E, MENU_BOX_Y + i);
				grub_putstr ("\xe2\x94\x82");
	    }

      /* lower-left corner */
      gotoxy (MENU_BOX_X - 2, MENU_BOX_Y + MENU_BOX_H);
      grub_putstr ("\xe2\x94\x94");

      /* bottom horizontal line */
      for (i = 0; i < MENU_BOX_W + 1; i++)
        grub_putstr ("\xe2\x94\x80");

      /* lower-right corner */
      grub_putstr ("\xe2\x94\x98");
    }

    if (current_term->setcolorstate)
      current_term->setcolorstate (COLOR_STATE_HELPTEXT);

		if (!(menu_tab & 4))
      print_default_help_message (config_entries);

    if (current_term->flags & TERM_DUMB)
      grub_printf ("\n\nThe selected entry is %d ", entryno);
    else
      print_entries (first_entry, entryno, menu_entries);
  }
  if (menu_init_script_file[0] != 0 )	
    command_func(menu_init_script_file,BUILTIN_MENU);
  /* XX using RT clock now, need to initialize value */
  while ((time1 = getrtsecs()) == 0xFF);

  old_c = 0;
  old_c_count = 0;
  old_c_count_end = (force_lba & 4);	/* non-zero to disable single-key-selection feature. */
  temp_entryno = 0;
  /* Initialize to NULL just in case...  */
  cur_entry = NULL;

  while (1)
  {
    /* Initialize to NULL just in case...  */
    if (grub_timeout >= 0 && (time1 = getrtsecs()) != time2 /* && time1 != 0xFF */)
    {
      if (grub_timeout <= 0)
	    {
	      grub_timeout = -1;
	      break;
	    }

      /* else not booting yet! */
      time2 = time1;

      if (current_term->setcolorstate)
	      current_term->setcolorstate (COLOR_STATE_HELPTEXT);

      if (current_term->flags & TERM_DUMB)
	      grub_printf ("\r    Entry %d will be booted automatically in %d seconds.   ", entryno, grub_timeout);
      else
	    {
        if(timeout_x || timeout_y)
          gotoxy (timeout_x,timeout_y);
        else
        {
          if(!(menu_tab & 0x40))
            gotoxy (MENU_BOX_E - 2, MENU_BOX_Y + entryno);
          else
            gotoxy (MENU_BOX_X - 1, MENU_BOX_Y + entryno);	
        }

        if(timeout_color)
        {
          if (!(timeout_color & 0xffffffff00000000))
          {
            if (!(splashimage_loaded & 2))
              current_color_64bit = timeout_color | (console_color_64bit[COLOR_STATE_NORMAL] & 0xffffffff00000000);
            else
              current_color_64bit = timeout_color;
          }
          else
            current_color_64bit = timeout_color | 0x1000000000000000;

          current_term->setcolorstate (color_64_to_8 (current_color_64bit & 0x00ffffffffffffff) | 0x100);
        }
        else
				if (current_term->setcolorstate)
					current_term->setcolorstate (COLOR_STATE_HIGHLIGHT);

				grub_printf("%2d",grub_timeout);

				if (current_term->setcolorstate)
	      current_term->setcolorstate (COLOR_STATE_HELPTEXT);
				
	      gotoxy (MENU_BOX_E, MENU_BOX_Y + entryno);
	    }
	  
      grub_timeout--;
    }

    if (grub_timeout >= 0)
    {
      defer(1);
      if (animated_enable)
        animated();
      if (DateTime_enable)
        DateTime_refresh();
    }
    /* Check for a keypress, however if TIMEOUT has been expired
      (GRUB_TIMEOUT == -1) relax in GETKEY even if no key has been pressed.  
      This avoids polling (relevant in the grub-shell and later on
      in grub if interrupt driven I/O is done).  */
    if ((i = checkkey ()) >= 0 || grub_timeout < 0)
    {
      /* Key was pressed, show which entry is selected before GETKEY,
	     since we're comming in here also on GRUB_TIMEOUT == -1 and
	     hang in GETKEY */
      if (current_term->flags & TERM_DUMB)
        grub_printf ("\r    Highlighted entry is %d: ", entryno);
		
      if (i < 0)
        i = getkey ();
      else
        clear_delay_display (entryno);
      c = i;
#if !HOTKEY		//外置热键
//      if (config_entries && hotkey_func)
      if (config_entries && *(int *)IMG(0x8260))
      {
        hotkey_func = (void *)(grub_size_t)(*(grub_size_t *)IMG(0x8260));
        //由于checkkey后，uefi键盘缓存已经清除，只能保存和使用其返回值i
//        i = ((int (*)())(*(int *)IMG(0x8260)))(0,-1,(0x4B40<<16)|(first_entry << 8) | entryno,i);
        i = (*hotkey_func)(0,-1,(0x4B40<<16)|(first_entry << 8) | entryno,i);
#else
      if (config_entries && hotkey_func_enable)
      {
        //0x4b40 flags HK,
        i = hotkey_func(0,-1,(0x4B40<<16)|(first_entry << 8) | entryno,i);
#endif
//        putchar_hooked = 0;
        c = i;  
        if (i == -1)
			    goto restart1;
        if (i>>16)
        {
          temp_entryno = (int)(unsigned char)(i>>16);
          if (i & 1<<30)
            goto check_update;
          entryno = temp_entryno;
          first_entry = 0;
          goto boot_entry;
        }
      }
      if (! old_c_count_end)
      {
        if (old_c == 0)
          old_c = c;
        if (c == old_c && old_c_count < 0x7FFFFFFF)
          old_c_count++;
        if (c != old_c)
          old_c_count_end = 1;
      }
#if 0
      if (grub_timeout >= 0)
	    {
	      if (current_term->setcolorstate)
          current_term->setcolorstate (COLOR_STATE_HELPTEXT);

	      if (current_term->flags & TERM_DUMB)
          grub_putchar ('\r', 255);
        if(timeout_x || timeout_y)
          gotoxy (timeout_x,timeout_y);
        else
        {
          if(!(menu_tab & 0x40))
            gotoxy (MENU_BOX_E - 2, MENU_BOX_Y + entryno);
          else
            gotoxy (MENU_BOX_X - 1, MENU_BOX_Y + entryno);
        }
        current_term->setcolorstate (COLOR_STATE_NORMAL);
        grub_printf("  ");
        if (current_term->setcolorstate)
          current_term->setcolorstate (COLOR_STATE_HELPTEXT);
		
        grub_timeout = -1;
        fallback_entryno = -1;
        if (! (current_term->flags & TERM_DUMB))
          gotoxy (MENU_BOX_E, MENU_BOX_Y + entryno);
      }
#else
      clear_delay_display (entryno);
#endif
      if (num_entries == 0)
      {
        first_entry = entryno = 0;
        goto done_key_handling;
      }
			
      if (c==0x3c00)
      {
        if (animated_type)
          animated_enable ^= 1;
        animated_enable_backup = animated_enable;
        goto restart1;
      }

      /* We told them above (at least in SUPPORT_SERIAL) to use
        '^' or 'v' so accept these keys.  */
      if (c == KEY_UP/*16*/ || c == KEY_LEFT || ((char)c) == '^')
      {
        temp_entryno = 0;
        if (current_term->flags & TERM_DUMB)
        {
          if (entryno > 0)
            entryno--;
        }
        else
        {
          if (c == KEY_UP)
          {
            temp_entryno = first_entry + entryno;
            for (;;)
            {
              temp_entryno = (temp_entryno + num_entries - 1) % num_entries;
              if (temp_entryno == first_entry + entryno)
                goto done_key_handling;
              cur_entry = get_entry (menu_entries, temp_entryno);
              if (*cur_entry != 0x08)
                goto check_update;
            }
          }

          if (entryno > 0)
          {
            //cur_entry = get_entry (menu_entries, first_entry + entryno);
            /* un-highlight the current entry */
            print_entry (MENU_BOX_Y + entryno, 0, first_entry + entryno, menu_entries);
            --entryno;
            //cur_entry = get_entry (menu_entries, first_entry + entryno);
            /* highlight the previous entry */
            print_entry (MENU_BOX_Y + entryno, 1, first_entry + entryno, menu_entries);
          }
          else if (first_entry > 0)
          {
            first_entry--;
            print_entries (first_entry, entryno, menu_entries);
          }
          else	/* loop forward to END */
          {
            temp_entryno = num_entries - 1;
            goto check_update;
          }
        }
      }
      else if ((c == KEY_DOWN/*14*/ || c == KEY_RIGHT || ((char)c) == 'v' || (!old_c_count_end && c == old_c && old_c_count >= 8))
        /* && first_entry + entryno + 1 < num_entries */)
      {
        temp_entryno = 0;
        if (current_term->flags & TERM_DUMB)
        {
          if (first_entry + entryno + 1 < num_entries)
          entryno++;
        }
        else
        {
          if (c == KEY_DOWN)
          {
            temp_entryno = first_entry + entryno;
            for (;;)
            {
              temp_entryno = (temp_entryno + 1) % num_entries;
              if (temp_entryno == first_entry + entryno)
                goto done_key_handling;
              cur_entry = get_entry (menu_entries, temp_entryno);
              if (*cur_entry != 0x08)
                goto check_update;
            }
          }

          if (first_entry + entryno + 1 >= num_entries)
          {
            temp_entryno = 0;	/* loop backward to HOME */
            goto check_update;
          }
          if (entryno < MENU_BOX_H - 1)
          {
            //cur_entry = get_entry (menu_entries, first_entry + entryno);
            /* un-highlight the current entry */
            print_entry (MENU_BOX_Y + entryno, 0, first_entry + entryno, menu_entries);
            /* highlight the next entry */
            ++entryno;
            print_entry (MENU_BOX_Y + entryno, 1, first_entry + entryno, menu_entries);
          }
          else if ((int)num_entries > MENU_BOX_H + first_entry)
          {
            first_entry++;
            print_entries (first_entry, entryno, menu_entries);
          }
        }
      }
      else if (c == KEY_PPAGE/*7*/)
      {
        /* Page Up */
        temp_entryno = 0;
        first_entry -= MENU_BOX_H;
        if (first_entry < 0)
        {
          entryno += first_entry;
          first_entry = 0;
          if (entryno < 0)
            entryno = 0;
        }
        print_entries (first_entry, entryno, menu_entries);
      }
      else if (c == KEY_NPAGE/*3*/)
      {
        /* Page Down */
        temp_entryno = 0;
        first_entry += MENU_BOX_H;
        if (first_entry + entryno + 1 >= num_entries)
        {
          first_entry = num_entries - MENU_BOX_H;
          if (first_entry < 0)
            first_entry = 0;
          entryno = num_entries - first_entry - 1;
        }
        print_entries (first_entry, entryno, menu_entries);
      }
      else if ( ((char)c) >= '0' && ((char)c) <= '9')
      {
        temp_num *= 10;
        temp_num += ((char)c) - '0';
        if (temp_num >= num_entries)	/* too big an entryno */
          temp_num = ((char)c - '0'>= num_entries)?0:(char)c - '0';
        temp_entryno = temp_num;
        if (temp_entryno != 0 || (char)c == '0')
        {
// temp_entryno has users number - check if it matches a title number
// If menu items are numbered then there must be no unnumbered items in the first few entries
// e.g. if you have 35 menu items, then menu entries 0 - 3 must all numbered - otherwise double-digit user entry will not work - e.g. 34 will not work
          int j;
          if (menu_num_ctrl[0])
          {
            if (menu_num_ctrl[0] == 1)
            {
              j = temp_num;
              while(title_boot[j+1]<=temp_num)
                ++j;
              if (title_boot[j] == temp_num)
                temp_entryno = j;
            }
          }
          else
          {
            for (j = 0; j < num_entries; ++j)
            {
              clean_entry (get_entry (menu_entries, j));
              if (checkvalue () > 0 && myatoi () == temp_entryno)
              {
                temp_entryno = j;
                j = num_entries;
              }
            }
          }

check_update:
          if (temp_entryno != first_entry + entryno)
          {
            /* check if entry temp_entryno is out of the screen */
            if (temp_entryno < first_entry || temp_entryno >= first_entry + MENU_BOX_H)
            {
              first_entry = (temp_entryno / MENU_BOX_H) * MENU_BOX_H;
              entryno = temp_entryno % MENU_BOX_H;
              print_entries (first_entry, entryno, menu_entries);
            } else {
              /* entry temp_entryno is on the screen, its relative entry number is
              * (temp_entryno - first_entry) */
              print_entry (MENU_BOX_Y + entryno, 0, first_entry + entryno, menu_entries);
              entryno = temp_entryno - first_entry;
              /* highlight entry temp_entryno */
              print_entry (MENU_BOX_Y + entryno, 1, temp_entryno, menu_entries);
            }
          }
        }
        if (temp_entryno >= num_entries)
          temp_entryno = 0;
      }
      else if (c == KEY_HOME/*1*/)
      {
        temp_entryno = 0;
        goto check_update;
      }
      else if (c == KEY_END/*5*/)
      {
        temp_entryno = num_entries - 1;
        goto check_update;
      }
      else
        temp_entryno = 0;

done_key_handling:

      if (current_term->setcolorstate)
        current_term->setcolorstate (COLOR_STATE_HEADING);

      gotoxy (MENU_BOX_E, MENU_BOX_Y + entryno);

      if (current_term->setcolorstate)
        current_term->setcolorstate (COLOR_STATE_STANDARD);

      if (!old_c_count_end && c == old_c && (old_c_count >= 30 || (old_c_count >= 8 && c != KEY_DOWN /*&& c != KEY_RIGHT*/ && c != KEY_UP /*&& c != KEY_LEFT*/)))
        grub_timeout = 5;

      cur_entry = NULL;

      if (config_entries)
      {
        if ((((char)c) == '\n') || (((char)c) == '\r') || (((char)c) == 'b') || (((char)c) == '#') || (((char)c) == '*'))
          break;
      }
      else
      {
        if ((((char)c) == 'd') || (((char)c) == 'o') || (((char)c) == 'O'))
        {
          if (! (current_term->flags & TERM_DUMB))
            print_entry (MENU_BOX_Y + entryno, 0,first_entry + entryno, menu_entries);

          /* insert after is almost exactly like insert before */
          if (((char)c) == 'o')
          {
            /* But `o' differs from `O', since it may causes
            the menu screen to scroll up.  */
            if (num_entries > 0)
            {
              if (entryno < MENU_BOX_H - 1 || (current_term->flags & TERM_DUMB))
                entryno++;
              else
                first_entry++;
            }
            c = 'O';
          }

          cur_entry = get_entry (menu_entries, first_entry + entryno);

          if (((char)c) == 'O')
          {
            grub_memmove (cur_entry + 2, cur_entry, ((grub_size_t) heap) - ((grub_size_t) cur_entry));
            cur_entry[0] = ' ';
            cur_entry[1] = 0;
            heap += 2;
            num_entries++;
          }
          else if (num_entries > 0)
          {
            char *ptr = get_entry (menu_entries, first_entry + entryno + 1);

            grub_memmove (cur_entry, ptr, ((grub_size_t) heap) - ((grub_size_t) ptr));
            heap -= (((grub_size_t) ptr) - ((grub_size_t) cur_entry));

            num_entries--;

            if (/*entryno >= num_entries && */entryno > 0)
              entryno--;
            else if (first_entry/* && num_entries < MENU_BOX_H + first_entry*/)
              first_entry--;
          }

          if (current_term->flags & TERM_DUMB)
          {
            grub_putchar ('\n', 255);
            grub_putchar ('\n', 255);
            print_entries_raw (num_entries, first_entry, menu_entries);
          }
          else if (num_entries > 0)
            print_entries (first_entry, entryno, menu_entries);
          else
          print_entry (MENU_BOX_Y, 0, first_entry + entryno, menu_entries);
        }

        cur_entry = menu_entries;
        if (((char)c) == 27)
          return;
        if (((char)c) == 'b')
          break;
      }

      if (! auth && password_buf)
      {
        if (((char)c) == 'p')
        {
          /* Do password check here! */
          if (current_term->flags & TERM_DUMB)
            grub_printf ("\r                                    ");
          else
          {
            if (MENU_BOX_B + 1 > current_term->max_lines)
              gotoxy (MENU_HELP_X, MENU_BOX_H);
            else
            gotoxy (MENU_HELP_X, (MENU_HELP_X ? MENU_BOX_B : (MENU_BOX_B + 1)));
          }

          password_x = MENU_HELP_X;
          if (current_term->setcolorstate)
            current_term->setcolorstate (COLOR_STATE_HELPTEXT);

          if (! check_password (password_buf, password_type))
          {
            /* If *pass_config is NUL, then allow the user to use
              privileged instructions, otherwise, load
              another configuration file.  */
            if (*pass_config)
            {
              strcpy(config_file,pass_config);
              /* Make sure that the user will not have
                authority in the next configuration.  */
              auth = 0;
              return;
            }
            else
            {
              /* Now the user is superhuman.  */
              auth = 1;
              goto restart1;
            }
          }
          else
          {
            grub_printf ("Failed! Press any key to continue...");
            getkey ();
            goto restart1;
          }
        }
      }
      else
      {
        if (((char)c) == 'e' && c != 0xE065) /* repulse GigaByte Key-E attack */
          //if ( c == 0x1265) /* repulse GigaByte Key-E attack */
        {
          int new_num_entries = 0;
          i = 0;
          char *new_heap;
          font_spacing = 0;
          line_spacing = 0;

          if (num_entries == 0)
          {
            first_entry = entryno = 0;
            cur_entry = get_entry (menu_entries, 0);
            grub_memmove (cur_entry + 2, cur_entry, ((grub_size_t) heap) - ((grub_size_t) cur_entry));

            cur_entry[0] = ' ';
            cur_entry[1] = 0;
            heap += 2;
            num_entries++;
          }

          if (config_entries)
          {
            new_heap = heap;
            cur_entry = titles[first_entry + entryno];
            while (*cur_entry++);	/* the first entry */
          }
          else
          {
            /* safe area! */
            new_heap = heap + NEW_HEAPSIZE + 1;
            cur_entry = get_entry (menu_entries, first_entry + entryno);
          }

          do
          {
            while ((*(new_heap++) = cur_entry[i++]) != 0);
            new_num_entries++;
          }
          while (config_entries && cur_entry[i]);

          /* this only needs to be done if config_entries is non-NULL,
            but it doesn't hurt to do it always */
          *(new_heap++) = 0;

          if (config_entries && new_num_entries)
          {
            int old_num_entries = num_entries;
            unsigned char graphic_type_back = graphic_type;
            graphic_type = 0;
            num_entries = new_num_entries;
            run_menu (heap, NULL, /*new_num_entries,*/ new_heap, 0);	/* recursive!! */
            num_entries = old_num_entries;
            graphic_type = graphic_type_back;
            goto restart1;
          }

          {
            animated_enable_backup = animated_enable;
            animated_enable = 0;
            setcursor (1); /* show cursor and disable splashimage */
            if (current_term->setcolorstate)
              current_term->setcolorstate (COLOR_STATE_STANDARD);
            cls ();
            print_cmdline_message (0);
            new_heap = heap + NEW_HEAPSIZE + 1;
            current_drive = GRUB_INVALID_DRIVE;
            get_cmdline_str.prompt = (unsigned char*)PACKAGE " edit> ";
            get_cmdline_str.maxlen = NEW_HEAPSIZE + 1;
            get_cmdline_str.echo_char = 0;
            get_cmdline_str.readline = 1;
            get_cmdline_str.cmdline= (unsigned char*)new_heap;
            if (! get_cmdline ())
            {
              int j = 0;

              /* get length of new command */
              while (new_heap[j++])
              ;

              if (j < 2)
              {
              j = 2;
                new_heap[0] = ' ';
                new_heap[1] = 0;
              }

              /* align rest of commands properly */
              grub_memmove (cur_entry + j, cur_entry + i, (grub_size_t) heap - ((grub_size_t) cur_entry + i));

              /* copy command to correct area */
              grub_memmove (cur_entry, new_heap, j);

              heap += (j - i);
              if (first_entry + entryno == num_entries - 1)
              {
                cur_entry[j] = 0;
                heap++;
              }
            }
          }

          goto restart1;
        }
        if (((char)c) == 'c')
        {
          font_spacing = 0;
          line_spacing = 0;
          animated_enable_backup = animated_enable;
          animated_enable = 0;
          setcursor (1);
          if (current_term->setcolorstate)
            current_term->setcolorstate (COLOR_STATE_STANDARD);
          cls ();
          enter_cmdline (heap, 0);
          goto restart1;
        }
      }
    }//if ((i = checkkey ()) >= 0 || grub_timeout < 0)
  }//while (1)
  
  /* Attempt to boot an entry. */
boot_entry:
  setcursor (1); /* show cursor and disable splashimage */
  animated_enable_backup = animated_enable;
  animated_enable = 0;

  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_STANDARD);
  cls ();
	font_spacing = 0;
	line_spacing = 0;
  fallbacked_entries = 0;
  while (1)
  {
    if (debug > 0)
    {
      if (config_entries)
      {
        char *p;

        p = get_entry (menu_entries, first_entry + entryno);
        if (! ((*p) & 0xF0))
          p++;
        char *p_t = (char *)SCRATCHADDR;
        while(*p && *p != '\n')
          *p_t++ = *p++;
        *p_t++ = 0;
        expand_var ((char *)SCRATCHADDR, p_t, 0x400);
        printf ("  Booting \'%s\'\n\n",p_t);
      }
      else
        printf ("  Booting command-list\n\n");
    }

    /* Set CURRENT_ENTRYNO for the command "savedefault".  */
    current_entryno = first_entry + entryno;   
    if (! cur_entry)
    {
      cur_entry = titles[current_entryno];
      while (*cur_entry++);
    }

    if (current_entryno >= num_entries)//Max entries
      break; 
    if (! run_script (cur_entry, heap))
      break;
    if (fallback_entryno < 0)
      break;
    cur_entry = NULL;
    first_entry = 0;
    entryno = fallback_entries[fallback_entryno];
    fallback_entryno++;
    if (fallback_entryno >= MAX_FALLBACK_ENTRIES || fallback_entries[fallback_entryno] < 0)
    fallback_entryno = -1;
    fallbacked_entries++;
    if (fallbacked_entries > num_entries * num_entries * 4)
    {
      printf ("\nEndless fallback loop detected(entry=%d)! Press any key to exit...", current_entryno);
      (void) getkey ();
      break;
    }
  }
  show_menu = 1;
  goto restart1;
}

static int get_line_from_config (char *cmdline, int max_len, int preset);
static int
get_line_from_config (char *cmdline, int max_len, int preset)
{
    unsigned int pos = 0, info = 0;//literal = 0, comment = 0;
    char c;  /* since we're loading it a byte at a time! */
 
    while (1)
    {
	if (preset)
	{
    if (! read_from_preset_menu (&c, 1))
      break;
	}
	else
	{
	    if (! grub_read ((unsigned long long)(grub_size_t)&c, 1, 0xedde0d90))
	    {
		if (errnum)
		{
			printf ("  Fatal! Read menu error!\n");
			print_error ();
		}
		break;
	    }
	}

	/* Replace CR with LF.  */
	if (c == '\r')
	    c = '\n';

	/* Replace tabs with spaces.  */
	if (c == '\t')//( || c == '\f' || c == '\v')
	    c = ' ';

	/* all other non-printable chars are illegal. */
	if (c != '\n' && (unsigned char)c < ' ')
	{
	    pos = 0;
	    break;
	}

	if (info & 2)	/* bit 1 for comment */
	{
	    if (c == '\n')
		info &= 0xFFFFFFFD;	//comment = 0;
	    /* Skip all comment chars upto end of line. */
	}
	else if (! pos)
	{
	    /* At the very beginning of the line... */
	    if (c == '#')
	    {
		info |= 2;	//comment = 1;
		/* Skip the comment char. */
	    }
	    else
	    {
		/* Skip non-printable chars, including the UTF-8 Byte Order Mark: EF BB BF */
		if ((unsigned char)c > ' ' && (unsigned char)c <= 0x7F) //((c != ' ') && (c != '\t') && (c != '\n') && (c != '\r'))
		{
		    cmdline[pos++] = c;
		    if (c >= '0' && c <= '9')
			info |= 8; // all hex digit
		    if (c >= 'A' && c <= 'F')
			info |= 8; // all hex digit
		}
	    }
	}
	else
	{
	    if (c == '\n')
		break;

	    if (!(info & 4) && pos == 4 && c == ':' && (info & 8))	/* font line, end this file */
	    {
		pos = 0;
		break;
	    }

	    if (info & 8) // all hex digit
	    {
		    if ((c < '0' || c > '9') && (c < 'A' && c > 'F'))
			info &= ~8; // not all hex digit
	    }

	    if (pos < (unsigned long long)max_len)
	    {
		if (!(info & 4) && c == '=')
		    c = ' ';
		if (c == ' ')
		    info |= 4;	//argument = 1;
		cmdline[pos++] = c;
	    }
	}
    }

    cmdline[pos] = 0;
    return pos;
}

int config_len;
static char *cur_entry;
char *CONFIG_ENTRIES;

static void reset (void);
static void
reset (void)
{
  count_lines = -1;
  config_len = 0;
  num_entries = 0;
  cur_entry = CONFIG_ENTRIES;

  /* Initialize the data for the configuration file.  */
  default_entry = 0;
  password_buf = 0;
  fallback_entryno = -1;
  fallback_entries[0] = -1;
  grub_timeout = -1;
  menu_num_ctrl[0] = 0;
}
  
extern struct builtin builtin_title;
//extern struct builtin builtin_graphicsmode;
extern struct builtin builtin_debug;
static unsigned int attr = 0;
int font_func (char *arg, int flags);
/* This is the starting function in C.  */
void cmain (void);
void
cmain (void)
{
  if (!menu_mem)
  {
    menu_mem = grub_zalloc (0x40e00);     //分配内存, 并清零
    menu_mem = (char *)(grub_size_t)(((unsigned long long)(grub_size_t)menu_mem + 511) & 0xfffffffffffffe00);
    title_boot = (unsigned short *)menu_mem;
    titles = (char * *)(menu_mem + 1024);
    CONFIG_ENTRIES = menu_mem + 1024 + 256 * sizeof (char *);
  }
  else
    grub_memset (menu_mem, 0, 0x40e00 - 0x200);

    saved_entryno = 0;
	new_menu = 0;
	new_hotkey = 0;
	setcursor (0);  //避免由预置菜单加载主菜单时，图形菜单出现一个奇怪的光标
    /* Never return.  */
restart2:
    reset ();       
    /* Here load the configuration file.  */
    if (! use_config_file)
	goto done_config_file;

    pxe_restart_config = 0;

restart_config:

    {
	/* STATE 0: Menu init, i.e., before any title command.
	   STATE 1: In a title command.
	   STATE 2: In a entry after a title command.  
	*/
	int state = 0, prev_config_len = 0, bt = 0;
	int is_preset;
	grub_memset (graphic_file_shift, 0, 32);
	menu_init_script_file[0] = 0;
	{
	    int is_opened;
	    is_preset = is_opened = 0;
	    /* Try command-line menu first if it is specified. */
    if (use_preset_menu)
    {
      is_opened = is_preset = open_preset_menu ();
    }
    if (! is_opened)
    {
		if (*config_file)
		{
			is_opened = (configfile_opened || grub_open (config_file));
      if (! is_opened)
        goto done_config_file;
			#ifdef FSYS_IPXE
//			if (is_opened && current_drive == PXE_DRIVE && current_partition == IPXE_PART)
//				pxe_detect(IPXE_PART,config_file);
			#endif
		}
    }
	    errnum = 0;
	    configfile_opened = 0;         
	    if (! is_opened)
	    { 
		if (pxe_restart_config)
			goto original_config;
		/* Try the preset menu. This will succeed at most once,
		 * because the preset menu will be disabled(see below).  */ 
      is_opened = is_preset = open_preset_menu ();
	    }
	    if (! is_opened)
		goto done_config_file;
	}
 	
	/* This is necessary, because the menu must be overrided.  */
	reset ();
#if 1 //值1表示:  从现在开始, 不在屏幕显示任何信息
	putchar_hooked = (unsigned char*)1;/*stop displaying on screen*/  //禁止显示
#else
  putchar_hooked = 0; //允许显示
#endif
	while (get_line_from_config ((char *) CMDLINE_BUF, NEW_HEAPSIZE, is_preset))
	{
	    struct builtin *builtin = 0;
	    char *cmdline = (char *) CMDLINE_BUF;  
	    /* Get the pointer to the builtin structure.  */
    if (*cmdline == ':' || *cmdline == '!' || *cmdline == '{' || *cmdline == '}')
    {
      builtin->flags = 8;
      goto sss;
    }
	    builtin = find_command (cmdline);
	    errnum = 0;
	    if (! builtin)
		/* Unknown command. Just skip now.  */
		continue;
sss:
	    if ((grub_size_t)builtin != (grub_size_t)-1 && builtin->flags == (int)0)	/* title command */
	    {
		if (builtin != &builtin_title)/*If title*/
		{
			unsigned int tmp_filpos;
			unsigned int tmp_drive = saved_drive;
			unsigned int tmp_partition = saved_partition;
			unsigned int rp;
			cmdline = skip_to(1, cmdline);
			/* save original file position. */
			tmp_filpos = /*(is_preset && preset_menu == (const char *)0x800) ?
					preset_menu_offset : */filepos;
			/* close the already opened file for safety, in case 
			 * the builtin->func() below would call
			 * grub_open(). */
				grub_close ();
			rp = builtin->func(cmdline,BUILTIN_IFTITLE);
			saved_drive = tmp_drive;
			saved_partition = tmp_partition;
			current_drive = GRUB_INVALID_DRIVE;
			buf_drive = -1;
			/* re-open the config_file which is still in use by
			 * get_line_from_config(), and restore file position
			 * with the saved value. */
      if (is_preset)  //是预设
      {
        open_preset_menu ();
        filepos = (unsigned long long)tmp_filpos;
      }
      else
			{
				if (! grub_open (config_file))
				{
					printf ("  Fatal! Re-open %s failed!\n", config_file);
					print_error ();
				}
				filepos = (unsigned long long)tmp_filpos;
			}

			if (rp)
			{
				cmdline += rp;
			}
			else
			{
				int i;
				for (i = num_entries + ((state & 0xf) ? 1 : 0); i < 32; i++)
					graphic_file_shift[i] += 1;
				state |= 0x10;
				continue;
			}
		}

		/* Finish the menu init commands or previous menu items.  */
		if (state & 2)
		{
		    /* The next title is found.  */
		    if (num_entries >= 256)
			break;
			bt += (CONFIG_ENTRIES[attr] & 1);
		    num_entries++;	/* an entry is completed. */
		    CONFIG_ENTRIES[config_len++] = 0;	/* finish the entry. */
		    prev_config_len = config_len;
		}
		else if (state & 1)		/* state == 1 */
		{
		    /* previous is an invalid title, overwrite it.  */
		    config_len = prev_config_len;
		}
		else			/* state == 0 */
		{
		    /* The first title. So finish the menu init commands. */
		    CONFIG_ENTRIES[config_len++] = 0;
		}
		/* Reset the state.  */
		state = 1;

		/* Copy title into config area.  */
		{
		    int len;
		    char *ptr = cmdline;
		    while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '=')
			ptr++;
		    if (*ptr)
			ptr++;
		    attr = config_len;
		    if (num_entries < 256)
			{
				titles[num_entries] = CONFIG_ENTRIES + config_len;
				title_boot[num_entries] = bt;
			}
		    CONFIG_ENTRIES[config_len++] = 0x08;	/* attribute byte */
		    
		    len = parse_string (ptr);
		    ptr[len] = 0;
		    while ((CONFIG_ENTRIES[config_len++] = *(ptr++)) != 0);
		}
	    }
	    else if (state & 0x10) /*ignored menu by iftitle*/
      {
		continue;
      }
	    else if (! state)			/* menu init command */
	    {
		if ((grub_size_t)builtin == (grub_size_t)-1 || builtin->flags & BUILTIN_MENU)
		{
		    char *ptr = cmdline;
		    /* Copy menu-specific commands to config area.  */
		    while ((CONFIG_ENTRIES[config_len++] = *ptr++) != 0);
		    prev_config_len = config_len;
		}
		else
		    /* Ignored.  */
		    continue;
	    }
	    else				/* menu item command */
	    {
		state = 2;
		/* Copy config file data to config area.  */
		{
		    char *ptr = cmdline;
		    while ((CONFIG_ENTRIES[config_len++] = *ptr++) != 0);
		    prev_config_len = config_len;
		}
		if ((grub_size_t)builtin != (grub_size_t)-1)
    {
			CONFIG_ENTRIES[attr] |= !!(builtin->flags & BUILTIN_BOOTING);
    }
	    }
	} /* while (get_line_from_config()) */
//从现在开始, 在屏幕显示信息
	putchar_hooked = 0;
	/* file must be closed here, because the menu-specific commands
	 * below may also use the GRUB_OPEN command.  */
  if (is_preset)
  {
    if (use_preset_menu/* != (const char *)0x800*/)
    {
      /* load the font embedded in preset menu. */			//载入预置菜单中的字体。
      if (font_func (preset_menu, 0))  //0   如果字体加载成功
      {
        /* font exists, automatically enter graphics mode. */  //字体存在，自动进入图形模式。
        if (! IMAGE_BUFFER)  //如果不在图形模式，尝试设置图形模式
        {
          graphicsmode_func ("-1 800", 0);
        }
      }
    }
    grub_close ();
  }
  else
	{
	    grub_close ();
      font_func (config_file, 0);
	    /* before showing menu, try loading font in the tail of config_file */
	}
  use_preset_menu = 0;	/* Disable preset menu.  */  //禁用预置菜单
	if (state & 2)
	{
	    if (num_entries < 256)
	        num_entries++;	/* the last entry is completed. */
	}
	else// if (state)
	{
	    config_len = prev_config_len;
	}

	/* Finish the last entry or the menu init commands.  */
	CONFIG_ENTRIES[config_len++] = 0;
	title_boot[num_entries] = -1;
	if (num_entries < 256)
		titles[num_entries] = 0;
	/* old MENU_BUF is not used any more. So MENU_BUF is a temp area,
	 * and can be moved to elsewhere. */

	/* CONFIG_ENTRIES contains these:
	 * 1. The array of menu init commands.
	 * 2. The array of menu item commands with leading titles.
	 */

	/* Display this info if the debug command is not present in the
	 * menu-init command set.
	 */
	/* Run menu-specific commands before any other menu entry commands.  */
;
	{
	    static char *old_entry = NULL;
	    static char *heap = NULL; heap = CONFIG_ENTRIES + config_len;

	    /* Initialize the data.  */
	    current_drive = GRUB_INVALID_DRIVE;
	    count_lines = -1;
	    kernel_type = KERNEL_TYPE_NONE;
	    errnum = 0;
	    while (1)
	    {
		pxe_restart_config = 0;

#ifdef SUPPORT_GFX
//		*graphics_file = 0;
#endif
		//DEBUG_SLEEP  /* Only uncomment if you want to pause before processing every menu.lst line */
		/* Copy the first string in CUR_ENTRY to HEAP.  */
		old_entry = cur_entry;
		while (*cur_entry++);

		grub_memmove (heap, old_entry, (grub_size_t) cur_entry - (grub_size_t) old_entry);
		if (! *heap)
		{
		    /* If there is no more command in SCRIPT...  */
		    /* If no kernel is loaded, just exit successfully.  */
		    if (kernel_type == KERNEL_TYPE_NONE)
			break;

		    /* Otherwise, the command boot is run implicitly.  */
		    grub_memmove (heap, "boot", 5);
		}
		run_line (heap , BUILTIN_MENU);

		/* if the INSERT key was pressed at startup, debug is not allowed to be turned off. */
		if (pxe_restart_config)
			goto restart_config;
original_config:
		if (! *old_entry)
		    break;
	    } /* while (1) */

	    kernel_type = KERNEL_TYPE_NONE;
	} /* while (1) */

	/* End of menu-specific commands.  */

	errnum = 0;
	/* Make sure that all fallback entries are valid.  */
	if (fallback_entryno >= 0)
	{
	    int i;

	    for (i = 0; i < MAX_FALLBACK_ENTRIES; i++)
	    {
		if (fallback_entries[i] < 0)
		    break;
		if (fallback_entries[i] >= num_entries)
		{
		    grub_memmove (fallback_entries + i, fallback_entries + i + 1, ((MAX_FALLBACK_ENTRIES - i - 1) * sizeof (int)));
		    i--;
		}
	    }

	    if (fallback_entries[0] < 0)
		fallback_entryno = -1;
	}
	/* Check if the default entry is present. Otherwise reset it to
	   fallback if fallback is valid, or to DEFAULT_ENTRY if not.  */
	if (default_entry >= num_entries)
	{
	    if (fallback_entryno >= 0)
	    {
		default_entry = fallback_entries[0];
		fallback_entryno++;
		if (fallback_entryno >= MAX_FALLBACK_ENTRIES || fallback_entries[fallback_entryno] < 0)
		    fallback_entryno = -1;
	    }
	    else
		default_entry = 0;
	}
    }

done_config_file:
  use_preset_menu = 0;	/* Disable the preset menu.  */	//禁用预设菜单
//	pxe_restart_config = 1;	/* pxe_detect will use configfile to run menu */
  /* go ahead and make sure the terminal is setup */	//继续前进，确保终端的安装
	if (current_term->startup)
		(*current_term->startup)();

    if (! num_entries)
    {
	/* no config file, goto command-line, starting heap from where the
	   config entries would have been stored if there were any.  */
	enter_cmdline (CONFIG_ENTRIES, 1);
    }
    else
    {
	/* Run menu interface.  */
	/* cur_entry point to the first menu item command. */
#if !HOTKEY		//外置热键
//	if (hotkey_func)
  if (*(int *)IMG(0x8260))
  {
    hotkey_func = (void *)(grub_size_t)(*(grub_size_t *)IMG(0x8260));
//    ((int (*)())(*(int *)IMG(0x8260)))(0,0,-1,0);
    (*hotkey_func)(0,0,-1,0);
  }
#else
	if (hotkey_func_enable)
		hotkey_func(0,0,-1,0);
#endif
	run_menu ((char *)titles, cur_entry, /*num_entries,*/ CONFIG_ENTRIES + config_len, default_entry);
    }
    goto restart2;
}


//----------------------------------------------------------------------------------------------------------------------
//hotkey   chenall
#if HOTKEY		//内置热键
//extern int hotkey_func(char *titles,int flags,int flags1);		标题,标记,标记1=4B400000 | 第一入口*100 | 项目号
//hotkey_func(0,0,-1);
//hotkey_func(0,-1,(0x4B40<<16)|(first_entry << 8) | entryno);
#define HOTKEY_MAGIC 0X79654B48						//魔术
#define HOTKEY_FLAGS_AUTO_HOTKEY (1<<12)	//热键标志	自动热键		1000		-A参数		使用菜单首字母选择菜单
#define HOTKEY_FLAGS_AUTO_HOTKEY1 (1<<11)	//热键标志	自动热键1		800
#define HOTKEY_FLAGS_NOT_CONTROL (1<<13)	//热键标志	不控制			2000		-nc参数		阻止使用'c','p','e'等功能
#define HOTKEY_FLAGS_NOT_BOOT	 (1<<14)		//热键标志	不启动			4000		-nb参数		按键热后只选择对应菜单项，不启动
#define BUILTIN_CMDLINE		0x1	/* Run in the command-line.  	运行在命令行=1	*/
#define BUILTIN_MENU			(1 << 1)/* Run in the menu.  			运行在菜单=2		*/

typedef struct
{
  unsigned short code;	//键代码
  char name[10];					//键名称
} __attribute__ ((packed)) key_tab_t;	//键盘表

typedef struct 
{
	unsigned short key_code;	//键代码
	unsigned short title_num;	//标题号
} __attribute__ ((packed)) hotkey_t;	//热键

typedef struct
{
	int key_code;	//键代码
	char *cmd;		//命令
} hotkey_c;	//热键字符

union
{
	int flags;
	struct
	{
		unsigned char sel;		//选择 
		unsigned char first;	//首先
		unsigned short flag;	//标记
	} k;
} __attribute__ ((packed)) cur_menu_flag;	//当前菜单标记

typedef struct
{
	int cmd_pos;					//命令位置
	hotkey_c hk_cmd[64];	//热键字符数组
	char cmd[4096];				//命令
} hkey_data_t;	//热键数据

static key_tab_t key_table[] = {						//键盘表
  {0x0031, "1"},
  {0x0032, "2"},
  {0x0033, "3"},
  {0x0034, "4"},
  {0x0035, "5"},
  {0x0036, "6"},
  {0x0037, "7"},
  {0x0038, "8"},
  {0x0039, "9"},
  {0x0030, "0"},
  {0x002d, "-"},
  {0x003d, "="},
  {0x0071, "q"},
  {0x0077, "w"},
  {0x0065, "e"},
  {0x0072, "r"},
  {0x0074, "t"},
  {0x0079, "y"},
  {0x0075, "u"},
  {0x0069, "i"},
  {0x006f, "o"},
  {0x0070, "p"},
  {0x005b, "["},
  {0x005d, "]"},
  {0x0061, "a"},
  {0x0073, "s"},
  {0x0064, "d"},
  {0x0066, "f"},
  {0x0067, "g"},
  {0x0068, "h"},
  {0x006a, "j"},
  {0x006b, "k"},
  {0x006c, "l"},
  {0x003b, ";"},
  {0x0027, "'"},
  {0x0060, "`"},
  {0x005c, "\\"},
  {0x007a, "z"},
  {0x0078, "x"},
  {0x0063, "c"},
  {0x0076, "v"},
  {0x0062, "b"},
  {0x006e, "n"},
  {0x006d, "m"},
  {0x002c, ","},
  {0x002e, "."},
  {0x002f, "/"},
  {0x3b00, "f1"},
  {0x3c00, "f2"},
  {0x3d00, "f3"},
  {0x3e00, "f4"},
  {0x3f00, "f5"},
  {0x4000, "f6"},
  {0x4100, "f7"},
  {0x4200, "f8"},
  {0x4300, "f9"},
  {0x4400, "f10"},
  {0x5200, "ins"},
  {0x5300, "del"},
  {0x8500, "f11"},
  {0x8600, "f12"},
  {0x0000, "\0"}
};

static unsigned short allow_key[9] = {	//方向键
/*KEY_ENTER       */0x000D,
/*KEY_HOME        */0x4700,
/*KEY_UP          */0x4800,
/*KEY_PPAGE       */0x4900,
/*KEY_LEFT        */0x4B00,
/*KEY_RIGHT       */0x4D00,
/*KEY_END         */0x4F00,
/*KEY_DOWN        */0x5000,
/*KEY_NPAGE       */0x5100
};

static int my_app_id = 0;
static char keyname_buf[16];
static int get_keycode (char* key);
static int check_hotkey(char **title,int flags);
static char *get_keyname (int code);
static int check_allow_key(unsigned short key);

/* gcc treat the following as data only if a global initialization like the		gcc仅在发生与上述行类似的全局初始化时才将以下内容视为数据。
 * above line occurs.
 */
static hkey_data_t hotkey_data = {0};	//HOTKEY 数据保留区
//返回: -1=刷新;		2字节=热键项目号;		3字节&40=0/1=启动/检查更新
int hotkey_func (char *arg,int flags,int flags1,int key);
int hotkey_func (char *arg,int flags,int flags1,int key)
{
	int i;
	char *base_addr;
	static hotkey_t *hotkey;
	unsigned short hotkey_flags;
	unsigned short *p_hotkey_flags;
  base_addr = menu_mem + 512;	//非压缩菜单在此
	hotkey = (hotkey_t*)(menu_mem + 512);
	p_hotkey_flags = (unsigned short*)(menu_mem + 512 + 508);
	cur_menu_flag.flags = flags1;

	if (flags == HOTKEY_MAGIC)	//如果标记=魔术
	{
		if (arg && *(int *)arg == 0x54494E49)	//INIT 初始数数据
		{
			hotkey_data.hk_cmd[0].cmd = hotkey_data.cmd;
			hotkey_data.cmd_pos = 0;
		}
		return (int)(grub_size_t)&hotkey_data;
	}

	if (flags == -1)	//如果标记='-1'
	{
		int c;
		hotkey_c *hkc = hotkey_data.hk_cmd;
		if (my_app_id != HOTKEY_MAGIC/* || (!hotkey->key_code && !hkc->key_code)*/)
		{
			return key;
		}
		hotkey_flags = *p_hotkey_flags;
		c = (key & 0xf00ffff);
		if (!c || check_allow_key(c))	//检测当前的按键码是否方向键或回车键, 是则返回
			return c;
		for(i=0;i<64;++i)
		{
			if (!hkc[i].key_code)
				break;
			if (hkc[i].key_code == (unsigned short)c)
			{
				grub_error_t err_old = errnum;
				if (hkc[i].cmd[0] == '@')		//静默方式运行(没有任何显示)
			    builtin_cmd(NULL,hkc[i].cmd + 1,BUILTIN_CMDLINE);
				else
				{
					putchar_hooked = 0;
					setcursor (1); /* show cursor and disable splashimage */
					if (current_term->setcolorstate)
				    current_term->setcolorstate(COLOR_STATE_STANDARD);
					cls();
					if (debug > 0)
				    printf(" Hotkey Boot: %s\n",hkc[i].cmd);
			    builtin_cmd(NULL,hkc[i].cmd,BUILTIN_CMDLINE);
				}
				if (putchar_hooked == 0 && errnum > ERR_NONE && errnum < MAX_ERR_NUM)
				{
			    printf("\nError %u\n",errnum);
			    getkey();
				}
				errnum = err_old;
				return -1;
			}
		}
		for (;hotkey->key_code;++hotkey)
		{
			if (hotkey->key_code == (unsigned short)c)
				return (hotkey_flags|hotkey->title_num)<<16;
		}
		if ((hotkey_flags & HOTKEY_FLAGS_AUTO_HOTKEY) && (char)c > 0x20)
		{
			char h = tolower(c&0xff);	//小写
			char *old_c_hotkey = (char*)(p_hotkey_flags+1);
			char *old_t = old_c_hotkey + 1;
			int find = -1,n = *old_t;
			int cur_sel = -1;
			char **titles1;
			if (c >= 'A' && c <= 'Z')	//按下Shift键
				return h;
			if ((c & 0xf000000))	//如果按下Ctrl或Alt键
			{
				goto chk_control;
			}
			if (cur_menu_flag.k.flag == 0x4B40)
				cur_sel = cur_menu_flag.k.sel + cur_menu_flag.k.first;
			titles1 = (char **)(base_addr + 512);
			if (*old_c_hotkey != h)
			{//不同按键清除记录
				*old_c_hotkey = h;
				n = *old_t = 0;
			}
			for(i=0;i<256;++i)
			{
				if (!*titles1)
					break;
				if (check_hotkey(titles1,h))	//从菜单标题中提取热键代码
				{//第几次按键跳到第几个匹配的菜单(无匹配转第一个)
					if (cur_sel != -1)
					{
						if (cur_sel < i)
						{
							find = i;
							break;
						}
					}
					else if (n-- == 0)
					{
						find = i;
						break;
					}
					if (find == -1) find = i;
				}
				++titles1;
			}
			if (find != -1)
			{
				if (find != i) *old_t = 1;
				else (*old_t)++;
				return (hotkey_flags|HOTKEY_FLAGS_NOT_BOOT|find)<<16;	//不启动
			}
			if (h > '9')
				return 0;
		}
		chk_control:
		if ((hotkey_flags & HOTKEY_FLAGS_NOT_CONTROL) || (c & 0xf000000))	//不控制, 或者按了控制键/上档键/替换键又没有查到
			return 0;
		return c;
	}
	else if (flags == 0)	//在菜单main中执行						//从菜单标题中提取热键代码
	{
		char **titles1;
		titles1 = (char **)(base_addr + 512);	//标题

		for(i=0;i<126;++i)
		{
			if (!*titles1)	//没有标题,退出
				break;
			if ((hotkey->key_code = check_hotkey(titles1,0)))	//如果键代码=从菜单标题中提取热键代码
			{
				hotkey->title_num = i;	//标题号
				++hotkey;								//下一热键数组
			}
			++titles1;									//下一标题
		}
		hotkey->key_code = 0;				//结束符
		return 1;
	}
	
	if (debug > 0)
		printf("Hotkey for grub4dos by chenall,%s\n",__DATE__);
	if ((flags & BUILTIN_CMDLINE) && (!arg || !*arg))
	{
		printf("Usage:\n\thotkey -nb\tonly selected menu when press menu hotkey\n\thotkey -nc\tdisable control key\n\thotkey -A\tSelect the menu item with the first letter of the menu\n\thotkey [HOTKEY] \"COMMAND\"\tregister new hotkey\n\te.g.\n\t\t hotkey [F9] \"reboot\"\n\thotkey [HOTKEY]\tDisable Registered hotkey HOTKEY\n\n\tCommand keys such as p, b, c and e will only work if SHIFT is pressed when hotkey -A\n\n");
	}
	hotkey_flags = 1<<15;	//8000
	while (*arg == '-')
	{
		++arg;
		if (*(unsigned short*)arg == 0x626E) //nb not boot
			hotkey_flags |= HOTKEY_FLAGS_NOT_BOOT;		//4000		不启动
		else if (*(unsigned short*)arg == 0x636E) //nc not control
			hotkey_flags |= HOTKEY_FLAGS_NOT_CONTROL;	//2000		不控制
		else if (*arg == 'A')
		{
			hotkey_flags |= HOTKEY_FLAGS_AUTO_HOTKEY;	//1000
		}
		else if (*arg == 'u')
		{
			hotkey_func_enable = (int)0;
			return builtin_cmd((char *)"delmod",(char *)"hotkey",flags);
		}
		arg = wee_skip_to(arg,0);
	}
	if (!hotkey_func_enable)	//原来在菜单加载热键时执行
	{
		*p_hotkey_flags = hotkey_flags;
		//HOTKEY程序驻留内存，直接复制自身代码到指定位置。
		my_app_id = HOTKEY_MAGIC;
		//开启HOTKEY支持，即设置hotkey_func函数地址。
		hotkey_func_enable = (int)1;
		//获取程序执行时的路径的文件名。
		i = hotkey_func((char *)"INIT",HOTKEY_MAGIC,0,0);//获取HOTKEY数据位置并作一些初使化????
		if (debug > 0)
		{
			printf("Hotkey Installed!\n");
		}
	}
	else
		*p_hotkey_flags |= hotkey_flags;
	if (arg)
	{
		hkey_data_t *hkd = (hkey_data_t *)(grub_size_t)hotkey_func(NULL,HOTKEY_MAGIC,0,0); //????
		hotkey_c *hkc = hkd->hk_cmd;

		while (*arg && *arg <= ' ')
			++arg;
		int key_code,cmd_len;
		int exist_key = -1;
		int disabled_key = -1;
		if (*arg != '[')//显示当前已注册热键
		{
			if (!(flags & BUILTIN_CMDLINE) || debug < 1)//必须在命令行下并且DEBUG 非 OFF 模式才会显示
		    return 1;
			if (debug > 1)
		    printf("hotkey_data_addr: 0x%X\n",hkd);
			if (hkc->key_code)
		    printf("Current registered hotkey:\n");
			while(hkc->key_code)
			{
		    if (hkc->key_code != -1)
		    {
					if (debug > 1)
						printf("0x%X ",hkc->cmd);
					printf("%s=>%s\n",get_keyname(hkc->key_code),hkc->cmd);
		    }
		    ++hkc;
			}
			return -1;
		}
		key_code = check_hotkey(&arg,0);	//从菜单标题中提取热键代码

		if (!key_code)
			return 0;
		while(*arg)
		{
			if (*arg++ == ']')
		    break;
		}
		while (*arg && *arg <= ' ')
			++arg;

		if (*arg == '"')
		{
			cmd_len = strlen(arg);
			++arg;
			--cmd_len;
			while(cmd_len--)
			{
		    if (arg[cmd_len] == '"')
		    {
    			arg[cmd_len] = 0;
					break;
		    }
			}
		}
		cmd_len = strlen(arg) + 1;	
		for(i=0;i<64;++i)
		{
			if (!hkc[i].key_code)
		    break;
			if (hkc[i].key_code == key_code)
		    exist_key = i;
			else if (hkc[i].key_code == -1)
		    disabled_key = i;
		}

		if (disabled_key != -1 && exist_key == -1)
		{//有禁用的热键,直接使用该位置
			exist_key = disabled_key;
			hkc[exist_key].key_code = key_code;
		}

		if (i==64 && exist_key == -1)
		{
			printf("Max 64 hotkey cmds limit!");
			return 0;
		}

		if (exist_key != -1)//已经存在
		{
			if (strlen(hkc[exist_key].cmd) >= cmd_len)//新的命令长度没有超过旧命令长度
				i = -1;
		}
		else//新增热键
		{
			exist_key = i;
			hkc[i].key_code = key_code;
		}
  
		if (cmd_len <= 1)//禁用热键
		{
			hkc[exist_key].key_code = -1;
			return 1;
		}

		if (hkd->cmd_pos + cmd_len >= (int)sizeof(hkd->cmd))
		{
			printf("error: not enough space!\n");
			return 0;
		}
	    
		if (i >= 0)//需要更新地址
		{
			hkc[exist_key].cmd = hkd->cmd + hkd->cmd_pos;
			hkd->cmd_pos += cmd_len;//命令数据区
		}

		memmove(hkc[exist_key].cmd,arg,cmd_len );
		if (debug > 0)
			printf("%d [%s] registered!\n",exist_key,get_keyname(key_code));
		return 1;
	}
	return 0;
}

/*
	检测当前的按键码是否方向键或回车键
*/
static int check_allow_key(unsigned short key);
static int check_allow_key(unsigned short key)
{
	int i;
	for (i=0;i<9;++i)
	{
		if (key == allow_key[i])
			return key;
	}
	return 0;
}


/*
	从菜单标题中提取热键代码
	返回热键对应的按键码。
	flags 非0 时判断菜单的首字母.-A参数.
*/
static int check_hotkey(char **title,int flags);
static int check_hotkey(char **title,int flags)
{
	char *arg = *title;
	unsigned short code;
	while (*arg && *arg <= ' ')
		++arg;

	if (flags)	//如果标记非0
		return tolower(*arg) == flags;
	if (*arg == '^')
	{
		++arg;
		sprintf(keyname_buf,"%.15s",arg);//最多复制15个字符
		nul_terminate(keyname_buf);//在空格处截断
		if ((code = (unsigned short)get_keycode(keyname_buf)))
		{
			//设置新的菜单标题位置
			arg+=strlen(keyname_buf);
			if (*arg == 0)
				--arg;
			*arg = *(*title);
			*title = arg;
			return code;
		}
	}
	else if (*arg == '[')
	{
		int i;
		++arg;
		while (*arg == ' ' || *arg == '\t')
			++arg;
		for(i = 0;i<15;++arg)
		{
			if (!*arg || *arg == ' ' || *arg == ']' )
			{
				break;
			}
			keyname_buf[i++] = *arg;
		}
		while (*arg == ' ')
			++arg;
		if (*arg != ']')
			return 0;
		keyname_buf[i] = 0;
		code = (unsigned short)get_keycode(keyname_buf);
		return code;
	}
	return 0;
}

static int get_keycode (char *key);
static int
get_keycode (char *key)	//获得键代码
{
  int idx, i;
  char *str;

  if (*(unsigned short*)key == 0x7830)/* == 0x*/
	{
		unsigned long long code;
		if (!safe_parse_maxint(&key,&code))
			return 0;
		return (((long long)code <= 0) || ((long long)code >= 0xFFFF)) ? 0 : (int)code;
	}
  str = key;
  idx = 0;
  if (!strnicmp (str, "shift", 5))
	{
		idx = 1;
		str += 6;
	}
  else if (!strnicmp (str, "ctrl", 4))
	{
		idx = 2;
		str += 5;
	}
  else if (!strnicmp (str, "alt", 3))
	{
		idx = 4;
		str += 4;
	}
  for (i = 0; key_table[i].name[0]; i++)
	{
		if (!strncmpx(str,key_table[i].name,0,1))
		{
			return (int)key_table[i].code | idx << 24; 
		}
	}
  return 0;
}


static char *get_keyname (int code);
static char *
get_keyname (int code)	//获得键名称
{
  int i;

  for (i = 0; key_table[i].name; i++)
	{
		int j;

			j = code & 0xf000000;
	      char *p = 0;
	      
	      switch (j)
				{
					case 0:
						p = (char *)"";
						break;
					case 1:
						p = (char *)"Shift-";
						break;
					case 2:
						p = (char *)"Ctrl-";
						break;
					case 4:
						p = (char *)"Alt-";
						break;
				}
	      sprintf (keyname_buf, "%s%s", p, key_table[i].name);
	      return (char *)keyname_buf;
	}

  sprintf (keyname_buf, "0x%x", code);
  return (char *)keyname_buf;
}
#endif