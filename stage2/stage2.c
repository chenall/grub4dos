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
unsigned long is_highlight = 0;

/* preset_menu is defined in asm.S */
#define	preset_menu *(const char **)0x307FFC

static unsigned long preset_menu_offset;

static int
open_preset_menu (void)
{
  if (! use_preset_menu)
    return 0;

  if (preset_menu != (const char *)0x800)
	  goto lzma;
  
  preset_menu_offset = 0;
  return 1;//preset_menu != 0;

lzma:
  return grub_open ("(md)0x880+0x200");
}

static int
read_from_preset_menu (char *buf, int max_len)
{
  int len;

  if (! use_preset_menu)	//if (preset_menu == 0)
	return 0;

  if (preset_menu != (const char *)0x800)
	  goto lzma;
  len = grub_strlen (preset_menu + preset_menu_offset);

  if (len > max_len)
    len = max_len;

  grub_memmove (buf, preset_menu + preset_menu_offset, len);
  preset_menu_offset += len;

  return len;

lzma:
  return grub_read ((unsigned long long)(unsigned int)buf, max_len, 0xedde0d90);
}

#define MENU_BOX_X	((menu_border.menu_box_x > 2) ? menu_border.menu_box_x : 2)
#define MENU_BOX_W	((menu_border.menu_box_w && menu_border.menu_box_w < (current_term->chars_per_line - MENU_BOX_X - 1)) ? menu_border.menu_box_w : (current_term->chars_per_line - MENU_BOX_X * 2 + 1))
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

static long temp_entryno;
static short temp_num;
static char * *titles;	/* title array, point to 256 strings. */
extern int (*hotkey_func)(char *titles,int flags,int flags1);
static unsigned short *title_boot;
static int default_help_message_destoyed = 1;
extern int num_text_char(char *p);
/*
	unsigned char disp_ul;
	unsigned char disp_ur;
	unsigned char disp_ll;
	unsigned char disp_lr;
	unsigned char disp_horiz;
	unsigned char disp_vert;
	unsigned char menu_box_x; // line start 
	unsigned char c; // line width 
	unsigned char menu_box_y; // first line number 
	unsigned char menu_box_h;
	unsigned char menu_box_b;
	unsigned char border_w;
	unsigned char menu_help_x;
	unsigned char menu_help_w;
	unsigned char menu_keyhelp_y_offset;
*/
struct border menu_border = {218,191,192,217,196,179,2,0,2,0,0,2,0,0,0}; /* console */
//struct border menu_border = {20,21,22,19,15,14,2,0,2,0,0}; /* graphics */

char* print_color_characters (char *message, unsigned char *character, unsigned long long *color64_back, unsigned int *color_back);
char*
print_color_characters (char *message, unsigned char *character, unsigned long long *color64_back, unsigned int *color_back)
{
  if (message[2] == ']')
  {
    current_color_64bit = *color64_back;
    current_color = *color_back;
    message += 3;
    *character = *(message);
  }
  else if ((message[3] | 0x20) == 'x')
  {
    unsigned long long ull;
    message += 2;
    if (safe_parse_maxint((char **)&message,&ull) && *message == ']')
    {
      *color_back = current_color;
      *color64_back = current_color_64bit;
      current_color_64bit = ull;
      current_color = color_64_to_8 (current_color_64bit);
      *character = *(++message);
    }
  }
  return message;
}

static void print_help_message (const char *message,int flags)
{
	grub_u32_t j,x,k;
	grub_u8_t c = *message;
	int start_offcet = 0;
	unsigned long long current_color_64bit_back;
  unsigned int current_color_back;

	if (flags==2)	
	{
		for (j=0; j<NUM_LINE_ENTRYHELP; ++j)
		{
			if (MENU_BOX_B + 1 + j > current_term->max_lines)
				return;
			gotoxy (MENU_HELP_X, MENU_BOX_B + 1 + j);
			for (x = MENU_HELP_X; x < MENU_HELP_E; x++)
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
//					start_offcet = current_term->chars_per_line - MENU_HELP_X - num_text_char((char *)message);
					start_offcet = MENU_HELP_E - num_text_char((char *)message);
					if(start_offcet<MENU_HELP_X)
						start_offcet=MENU_HELP_X;
				}

				for (; fontx < MENU_HELP_E;)
				{
					if (c && c != '\n' && fontx >= start_offcet)
					{
						if (fontx == MENU_HELP_E - 1)
							if (c > 0x7F && !(menu_tab & 0x40))
							{
								grub_putchar (' ', 255);
								continue;
							}
              //处理菜单项注释中的彩色字符
              if (*(unsigned short*)message == 0x5B24)//$[
                message = print_color_characters ((char *)message,&c,&current_color_64bit_back,&current_color_back);
						grub_putchar (c, 255);
						c = *(++message);
					}
					else
						grub_putchar (' ', 255);
				}
				for (x = fontx; x < MENU_HELP_E; x++)
					grub_putchar (' ', 255);
			}
		}
	}
}

static void
print_default_help_message (char *config_entries)
{
	grub_u32_t	i;
	char		buff[256];

	if (menu_tab & 0x20)							//中文
		i = grub_sprintf (buff,"\n按↑和↓选择菜单。");
	else if (graphics_mode < 0xff)		//英文  在文本模式
		i = grub_sprintf (buff,"\nUse the %c and %c keys to highlight an entry.",
				(unsigned long)(unsigned char)DISP_UP, (unsigned long)(unsigned char)DISP_DOWN);
	else															//英文  在图形模式   2023-02-22
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
		current_color_64bit = keyhelp_color;
	print_help_message(buff,0);
	default_help_message_destoyed = 0;
}


// ADDED By STEVE6375
static unsigned int num_entries;
static char *
get_entry (char *list, int num)
{
  int i;

  if (list == (char *)titles)
	return num < num_entries ? titles[num] : 0;

  for (i = 0; i < num && *list; i++)
    {
	while (*(list++));
    }

  return list;
}

static char ntext[256];

static void
clean_entry (char *text)
{
  int i;
  //(void) strcpy (ntext, text);
  for (i = 0; i < 255 && text[i]; ++i)
  {
	if (text[i] < 32 || text[i] > 127)
		ntext[i] = 32;
	else
		ntext[i] = text[i];
  }
  ntext[i] = 0;
  return;
}

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
  //  printf ("Mv%dMv",value);
  return value;
}

// END OF STEVE6375 ADDED CODE

int
num_text_char(char *p)
{
	int i=0;
	unsigned long un16=0;

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
			un16=(long)(*p & 0xf)<<12 | (long)(*(p+1) & 0x3f)<<6 | (long)(*(p+2) & 0x3f);
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
extern char menu_cfg[];
extern unsigned char menu_num_ctrl[];
char graphic_file_shift[32];

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
  
  unsigned long long clo64_back = current_color_64bit;
  unsigned int clo_back = current_color;
  unsigned long long clo64_tmp;
  unsigned int clo_tmp;
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
			graphic_y_offset = (font_h+line_spacing)*text_y_offset-(graphic_high/2-(font_h+line_spacing)/2);
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
  if (*(unsigned short *)0x8308)
  {
  if(!(menu_tab & 0x40))
	{
		gotoxy (MENU_BOX_X - 1, y);
		grub_putchar(highlight ? menu_cfg[0] : ' ', 255);
	}
	else
	{
		gotoxy (MENU_BOX_E - 1, y);
		grub_putchar(highlight ? menu_cfg[1] : ' ', 255);
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
      unsigned int ret;

      ret = MENU_BOX_E - x - end_offcet;
      if (c && c != '\n' /* && x <= MENU_BOX_W*/ && x >= start_offcet)
	{
hotkey_start:
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
      clo64_tmp = current_color_64bit;
      clo_tmp = current_color;
      current_color_64bit = hotkey_color_64bit;
      current_color = hotkey_color;
      ret = grub_putchar ((unsigned char)c, ret);
      current_color_64bit = clo64_tmp;
      current_color = clo_tmp; 
      goto hotkey_end;
    }
color_no:    
    //处理菜单项中的彩色字符
    if (*(unsigned short*)entry == 0x5B24)//$[
    {
      entry = print_color_characters (entry,&c,&clo64_back,&clo_back);
      goto hotkey_start;
    }
		ret = grub_putchar ((unsigned char)c, ret);
hotkey_end:
		//is_highlight = 0;
		if ((long)ret < 0)
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
		clo64_tmp = current_color_64bit;
		if(splashimage_loaded & 2)
			current_color_64bit = 0;
		else
			if (current_term->setcolorstate)
				current_term->setcolorstate (COLOR_STATE_NORMAL);
		ret = grub_putchar (' ', ret);
		current_color_64bit = clo64_tmp;
//		if ((long)ret < 0)
//			break;
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
//	else if (default_help_message_destoyed)
	else
	{
		if(!(menu_border.menu_keyhelp_y_offset) && !(menu_tab & 4))
		{
//			if (current_term->setcolorstate)
//				current_term->setcolorstate (COLOR_STATE_HELPTEXT);
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
static void
print_entries (int first, int entryno, char *menu_entries)
{
  int i;
  //int main_menu = (menu_entries == (char *)titles);

  gotoxy (MENU_BOX_E, MENU_BOX_Y);

#ifdef SUPPORT_GRAPHICS
  if (!graphics_inited || graphics_mode < 0xff)
#endif
  {
    if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_BORDER);
  if (first)
    grub_putchar (DISP_UP, 255);
  else
    grub_putchar (DISP_VERT, 255);
  }
  //if (main_menu || *menu_entries)
  //  menu_entries = get_entry (menu_entries, first);
  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_NORMAL);
  for (i = 0; i < MENU_BOX_H/*size*/; i++)
    {
      print_entry (MENU_BOX_Y + i, entryno == i, first + i, menu_entries);

   //   if (main_menu)
   //   {
	//if (menu_entries)
	//{
	//    if ((++first) < 256)
	//	menu_entries = titles[first];
	//    else
	//	menu_entries = 0;
	//}
	//continue;
   //   }
   //   if (*menu_entries)
	//while (*(menu_entries++));
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
    grub_putchar (DISP_DOWN, 255);
  else
    grub_putchar (DISP_VERT, 255);
  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_STANDARD);
}
  gotoxy (MENU_BOX_E, MENU_BOX_Y + entryno);	/* XXX: Why? */
}

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
static int
run_script (char *script, char *heap)
{
  char *old_entry = 0;
  char *cur_entry = script;
  struct builtin *builtin = 0;
	char cmd_add[16];
	char *menu_bat;
	char *p;
//  char *arg;
//  grub_error_t errnum_old;//2010-12-16

  /* Initialize the data.  */
  //saved_drive = boot_drive;
  //saved_partition = install_partition;
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
	p = (char *)(((int)menu_bat + 511) & ~511);
	grub_memmove (p, script, cur_entry - script);
	grub_sprintf (cmd_add, "(md)%d+%d", (int)p >> 9, ((cur_entry - script + 10 + 511) & ~511) >> 9);
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

//      errnum_old = errnum; //2010-12-16
      /* Copy the first string in CUR_ENTRY to HEAP.  */
//      errnum = ERR_NONE;//2010-12-16
      old_entry = cur_entry;
      while (*cur_entry++)
	;

      grub_memmove (heap, old_entry, (int) cur_entry - (int) old_entry);
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
/* now command echoing is considered no use for a successful command. */
//      if (! (builtin->flags & BUILTIN_NO_ECHO))
//	grub_printf ("%s\n", old_entry);

/*comment by chenall on 2010-12-16,will do it in run_line*/
#if 0
      /* If BUILTIN cannot be run in the command-line, skip it.  */
      if ((int)builtin != -1 && ! (builtin->flags & BUILTIN_CMDLINE))
	{
	  errnum = ERR_UNRECOGNIZED;
	  continue;
	}

      /* Invalidate the cache, because the user may exchange removable
	 disks.  */
      buf_drive = -1;

      if ((int)builtin != -1)
      if ((builtin->func) == errnum_func || (builtin->func) == checkrange_func)
	errnum = errnum_old;

      /* find && and || */
#endif
	run_line (heap , BUILTIN_SCRIPT);
      if (! *old_entry)	/* HEAP holds the implicit BOOT command */
	break;
    } /* while (1) */

ppp:
  kernel_type = KERNEL_TYPE_NONE;

  /* If a fallback entry is defined, don't prompt a user's intervention.  */
  
  if (fallback_entryno < 0)
    {
#if 0
      if (! (builtin->flags & BUILTIN_NO_ECHO))
	grub_printf ("%s\n", heap);
#endif
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
    timeout_enable = 0;
    fallback_entryno = -1;
    if (! (current_term->flags & TERM_DUMB))
      gotoxy (MENU_BOX_E, MENU_BOX_Y + entryno);
  }
}

unsigned short beep_buf[256];
int new_menu;
int color_counting;
int password_x;
unsigned char timeout_enable = 0;
int time0 = 0, time1 = 0;
static int fallbacked_entries;
static int old_c;
static int old_c_count;
static int old_c_count_end;

void timeout_refresh(void);
void timeout_refresh(void)
{
	time0++;
	if (time0 == 1000)
	{
		time0 = 0;
		time1++;
	}
}

static void
run_menu (char *menu_entries, char *config_entries, /*int num_entries,*/ char *heap, int entryno)
{
  int c = 0, /*time1,*/ time2 = -1, first_entry = 0;
  char *cur_entry = 0;
  char *pass_config = 0;
	color_counting = 0;
//  struct term_entry *prev_term = NULL;
		  
 
  /*
   *  Main loop for menu UI.
   */
   if (password_buf)//Make sure that PASSWORD is NUL-terminated.
		pass_config = wee_skip_to(password_buf,SKIP_WITH_TERMINATE);
		
	/* clear keyboard buffer before boot */
  while (console_checkkey () == 0x1c0d) console_getkey ();

restart1:
  //clear temp_num when restart menu
  temp_num = 0;
	font_spacing = menu_font_spacing;
	line_spacing = menu_line_spacing;
	if (graphics_mode > 0xff)		//含字符间隙时，需重新计算  2023-02-22
	{
		current_term->max_lines = current_y_resolution / (font_h + line_spacing);
		current_term->chars_per_line = current_x_resolution / (font_w + font_spacing);
	}
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
//      while ((time1 = getrtsecs ()) == 0xFF)
//	;
		if (grub_timeout >= 0)
			timeout_enable = 1;

      while (1)
	{
	  /* Unhide the menu on any keypress.  */
	  if (checkkey () != -1 /*&& ASCII_CHAR (getkey ()) == '\e'*/)
	    {
	    	c = getkey();/* eat the key */
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
				timeout_enable = 0;
	      show_menu = 1;
	      break;
	    }

	  /* If GRUB_TIMEOUT is expired, boot the default entry.  */
	  if (grub_timeout >=0
//	      && (time1 = getrtsecs ()) != time2
				&& time1 != time2
	      /* && time1 != 0xFF */)
	    {
	      if (grub_timeout <= 0)
		{
//		  grub_timeout = -1;
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
      //setcursor (0);
			
		if (graphics_inited && graphics_mode > 0xff)/*vbe mode call rectangle_func*/
		{
			int i;
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
        char *p;
        unsigned long long current_color_64bit_back;
        unsigned int current_color_back;
        grub_u8_t c1;
				for (j=0; j<16; j++)
				{
					if (strings[j].enable == 0)
						continue;	
					if (strings[j].start_y < 0)
						y = strings[j].start_y + current_term->max_lines;
					else
						y = strings[j].start_y;
					gotoxy (strings[j].start_x, y);
					current_term->setcolorstate (COLOR_STATE_NORMAL);
					if ((strings[j].color & 0xffffffff00000000) == 0)
						current_color_64bit = strings[j].color | (current_color_64bit & 0xffffffff00000000);
					else
						current_color_64bit = strings[j].color | 0x1000000000000000;
          
          p = strings[j].string;
          while(*p)
          {
            //处理字符串中的彩色字符
            if (*(unsigned short*)p == 0x5B24)//$[
              p = print_color_characters (p,&c1,&current_color_64bit_back,&current_color_back);
            else
              c1 = *(p);
            grub_putchar (c1, 255); //123 4456 778
            p++;
          }
//					grub_printf("%s",strings[j].string);
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
		unsigned long x,y,w,h,i,j;
		i = font_w + font_spacing;
		j = font_h + line_spacing;
		x = (MENU_BOX_X - 2) * i + (font_w >> 1);
		y = (MENU_BOX_Y) * j - (font_h >> 1);
		if (menu_border.menu_box_w)					//如果设置了w，保持			2023-02-22
			w = (MENU_BOX_W + 2) * i;
		else																//否则，重新计数。因为含间隙字符时，水平像素处以i可能有余数，导致菜单框右边偏大
			w = current_x_resolution - (x<<1);
		if (graphic_type)
			h = (graphic_high+row_space) * graphic_row;
		else
		h = (MENU_BOX_H + 1) * j;
		rectangle(x,y,w,h,menu_border.border_w);
	}
	else
	#endif
	{
	  int i;

	if (current_term->setcolorstate)
		current_term->setcolorstate (COLOR_STATE_BORDER);
  
	  /* upper-left corner */
	  gotoxy (MENU_BOX_X - 2, MENU_BOX_Y - 1);
	  grub_putchar (DISP_UL, 255);

	  /* top horizontal line */
	  for (i = 0; i < MENU_BOX_W + 1; i++)
	    grub_putchar (DISP_HORIZ, 255);

	  /* upper-right corner */
	  grub_putchar (DISP_UR, 255);

	  for (i = 0; i < MENU_BOX_H; i++)
	    {
	      /* left vertical line */
	      gotoxy (MENU_BOX_X - 2, MENU_BOX_Y + i);
	      grub_putchar (DISP_VERT, 255);
	      /* right vertical line */
	      gotoxy (MENU_BOX_E, MENU_BOX_Y + i);
	      grub_putchar (DISP_VERT, 255);
	    }

	  /* lower-left corner */
	  gotoxy (MENU_BOX_X - 2, MENU_BOX_Y + MENU_BOX_H);
	  grub_putchar (DISP_LL, 255);

	  /* bottom horizontal line */
	  for (i = 0; i < MENU_BOX_W + 1; i++)
	    grub_putchar (DISP_HORIZ, 255);

	  /* lower-right corner */
	  grub_putchar (DISP_LR, 255);
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
//  while ((time1 = getrtsecs()) == 0xFF);
	if (grub_timeout >= 0)
		timeout_enable = 1;

  old_c = 0;
  old_c_count = 0;
  old_c_count_end = (force_lba & 4);	/* non-zero to disable single-key-selection feature. */
  temp_entryno = 0;
  /* Initialize to NULL just in case...  */
  cur_entry = NULL;
  while (1)
    {
      /* Initialize to NULL just in case...  */
      //cur_entry = NULL;
	//cur_entry = menu_entries; /* for modified menu */

//      if (grub_timeout >= 0 && (time1 = getrtsecs()) != time2 /* && time1 != 0xFF */)
		if (grub_timeout >= 0 && time1 != time2 /* && time1 != 0xFF */)
	{
	  if (grub_timeout <= 0)
	    {
	      grub_timeout = -1;
				timeout_enable = 0;
	      break;
	    }

	  /* else not booting yet! */
	  time2 = time1;

	  if (current_term->setcolorstate)
	      current_term->setcolorstate (COLOR_STATE_HELPTEXT);

	  if (current_term->flags & TERM_DUMB)
	      grub_printf ("\r    Entry %d will be booted automatically in %d seconds.   ", 
			   entryno, grub_timeout);
	  else
	    {
//	      unsigned long x;
//	      unsigned long ret;
//	      unsigned char tmp_buf[512];
//	      unsigned char *p;
//	      unsigned char ch = ' ';

//	      grub_sprintf ((char*)tmp_buf, " The highlighted entry will be booted automatically in %d seconds.", grub_timeout);
//	      gotoxy (0, MENU_BOX_B + 5);
//	      p = tmp_buf;
//	      for (x = 0; x < current_term->chars_per_line; x = fontx)
//	      {
//		if (ch >= ' ')
//			ch = *p++;
//		ret = current_term->chars_per_line - x;
//		ret = grub_putchar ((ch >= ' ' ? ch : ' '), ret);
//		if ((long)ret < 0)
//			break;
//	      }
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
#if 0		//不需要  20023-02-22
	if (grub_timeout >= 0)
	{
		defer(1);
		if (animated_enable)
			animated();
		if (beep_enable)
			beep_func((char *)beep_buf,1);
		if (DateTime_enable)
			DateTime_refresh();
	}
#endif
      /* Check for a keypress, however if TIMEOUT has been expired
	 (GRUB_TIMEOUT == -1) relax in GETKEY even if no key has been
	 pressed.  
	 This avoids polling (relevant in the grub-shell and later on
	 in grub if interrupt driven I/O is done).  */
      if ((c = checkkey ()) >= 0 || grub_timeout < 0)
	{
	  /* Key was pressed, show which entry is selected before GETKEY,
	     since we're comming in here also on GRUB_TIMEOUT == -1 and
	     hang in GETKEY */
	  if (current_term->flags & TERM_DUMB)
	    grub_printf ("\r    Highlighted entry is %d: ", entryno);
	  if (config_entries && hotkey_func)
	  {
      if (c > 0)
        clear_delay_display (entryno);
//			putchar_hooked = (unsigned char*)0x800;
			//0x4b40 flags HK,
			c = hotkey_func(0,-1,(0x4B40<<16)|(first_entry << 8) | entryno);
//			putchar_hooked = 0;
      
      if (beep_enable)
      {
        beep_enable = 0;
        beep_play = 0;
        beep_frequency = 0;
        console_beep();
      }
      
			if (c == -1)
			    goto restart1;
			if (c>>16)
			{
				temp_entryno = (long)(unsigned char)(c>>16);
				if (c & 1<<30)
					goto check_update;
				entryno = temp_entryno;
				first_entry = 0;
				goto boot_entry;
			}
	  }
	  else
		c = /*ASCII_CHAR*/ (getkey ());

    if (beep_enable)
    {
      beep_enable = 0;
      beep_play = 0;
      beep_frequency = 0;
      console_beep();
    }

	  if (! old_c_count_end)
	  {
	    if (old_c == 0)
		old_c = c;
	    if (c == old_c && old_c_count < 0x7FFFFFFF)
		old_c_count++;
	    if (c != old_c)
	    {
		old_c_count_end = 1;
	    }
	  }
#if 0
	  if (grub_timeout >= 0)
	    {
//	      unsigned long x;

	      if (current_term->setcolorstate)
		  current_term->setcolorstate (COLOR_STATE_HELPTEXT);

	      if (current_term->flags & TERM_DUMB)
		grub_putchar ('\r', 255);
//	      else
//		gotoxy (0, MENU_BOX_B + 5);
//	      for (x = 0; x < current_term->chars_per_line; x++)
//	      {
//		grub_putchar (' ', 255);
//	      }
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
		else
			if (current_term->setcolorstate)
				current_term->setcolorstate (COLOR_STATE_HIGHLIGHT);
	
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
		      //entryno++;
		      //cur_entry = get_entry (menu_entries, first_entry + entryno);
		      /* highlight the next entry */
		      ++entryno;
		      print_entry (MENU_BOX_Y + entryno, 1, first_entry + entryno, menu_entries);
		    }
		  else if (num_entries > MENU_BOX_H + first_entry)
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
//	      if (temp_entryno >= num_entries)
//		  temp_entryno = num_entries - 1;
	      if (temp_num >= num_entries)	/* too big an entryno */
	      {
		     temp_num = ((char)c - '0'>= num_entries)?0:(char)c - '0';
	      }
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
					{
						++j;
					}
					if (title_boot[j] == temp_num)
						temp_entryno = j;
				}
			}
			else
			{
				for (j = 0; j < num_entries; ++j)
				{
				//  grub_printf("TE%d C%dC A%dATE%s ",temp_entryno,checkvalue (get_entry (menu_entries,j) ),myatoi (get_entry (menu_entries,j)),get_entry(menu_entries,j));
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
			  //cur_entry = get_entry (menu_entries, first_entry + entryno);
			  /* un-highlight the current entry */
			  print_entry (MENU_BOX_Y + entryno, 0, first_entry + entryno, menu_entries);
			  entryno = temp_entryno - first_entry;
			  //cur_entry = get_entry (menu_entries, temp_entryno);
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

//	  if (debug > 0)
//	  {
//		gotoxy (MENU_BOX_E - 4, MENU_BOX_Y - 2);
//		grub_printf ("%3d ", (first_entry + entryno));
//	  }
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
		      grub_memmove (cur_entry + 2, cur_entry, ((int) heap) - ((int) cur_entry));

		      cur_entry[0] = ' ';
		      cur_entry[1] = 0;

		      heap += 2;

		      /*if (first_entry + entryno == num_entries - 1)
		      {
			cur_entry[2] = 0;
			heap++;
		      }*/

		      num_entries++;
		    }
		  else if (num_entries > 0)
		    {
		      char *ptr = get_entry (menu_entries, first_entry + entryno + 1);

		      grub_memmove (cur_entry, ptr, ((int) heap) - ((int) ptr));
		      heap -= (((int) ptr) - ((int) cur_entry));

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
	      //if (((char)c) == 'e' && c != 0xE065) /* repulse GigaByte Key-E attack */
	      if ( c == 0x1265) /* repulse GigaByte Key-E attack */
		{
		  int new_num_entries = 0;
		  int i = 0;
		  char *new_heap;
		  menu_font_spacing = font_spacing;			//恢复图形模式时需要  2023-02-22
		  menu_line_spacing =	line_spacing;
			font_spacing = 0;
			line_spacing = 0;

		  if (num_entries == 0)
		    {
		      first_entry = entryno = 0;
		      cur_entry = get_entry (menu_entries, 0);
		      grub_memmove (cur_entry + 2, cur_entry, ((int) heap) - ((int) cur_entry));

		      cur_entry[0] = ' ';
		      cur_entry[1] = 0;

		      heap += 2;

		      /*if (first_entry + entryno == num_entries - 1)
		      {
			cur_entry[2] = 0;
			heap++;
		      }*/

		      num_entries++;
		    }

		//  while (first_entry + entryno >= num_entries)
		//  {
		//      if (entryno > 0)
		//	  entryno--;
		//      else
		//	  first_entry--;
		//  }

		  if (config_entries)
		    {
		      new_heap = heap;
		      //cur_entry = get_entry (config_entries, first_entry + entryno, 1);
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
		  //else
		  {
		      animated_enable_backup = animated_enable;
		      animated_enable = 0;
		      setcursor (1); /* show cursor and disable splashimage */
		      cls ();
		      print_cmdline_message (0);

		      new_heap = heap + NEW_HEAPSIZE + 1;

		      saved_drive = boot_drive;
		      saved_partition = install_partition;
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
			  grub_memmove (cur_entry + j, cur_entry + i, (int) heap - ((int) cur_entry + i));

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
		  menu_font_spacing = font_spacing;			//恢复图形模式时需要  2023-02-22
		  menu_line_spacing =	line_spacing;
			font_spacing = 0;
			line_spacing = 0;
			animated_enable_backup = animated_enable;
			animated_enable = 0;
			setcursor (1);
			cls ();
			enter_cmdline (heap, 0);
		  goto restart1;
		}
	    }
	}
    }
  
  /* Attempt to boot an entry.  */
  
 boot_entry:
	grub_timeout = -1;
	timeout_enable = 0;
	if (ext_timer)
	{
		grub_free (ext_timer);
		ext_timer = 0;
	}
  animated_enable_backup = animated_enable;
  animated_enable = 0;
  setcursor (1); /* show cursor and disable splashimage */
  if (current_term->setcolorstate)
    current_term->setcolorstate (COLOR_STATE_STANDARD);
  cls ();
//  /* if our terminal needed initialization, we should shut it down
//   * before booting the kernel, but we want to save what it was so
//   * we can come back if needed */
//  prev_term = current_term;
////  if (current_term->shutdown)
////    {
////      (*current_term->shutdown)();
////      current_term = term_table; /* assumption: console is first */
////    }
	menu_font_spacing = font_spacing;			//恢复图形模式时需要  2023-02-22
	menu_line_spacing =	line_spacing;
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
		//printf ("  Booting \'%s\'\n\n", (((*p) & 0xF0) ? p : ++p));
		if (! ((*p) & 0xF0))
			p++;
		//(for Issue 123)changed by chenall 2013-04-19 also translate variables for booting message.
		#if 0
		unsigned char ch;
		grub_putstr ("  Booting ");
		while ((*p) && (ch = *p++) && ch != '\n') grub_putchar (ch, 255);
		grub_putchar ('\n', 255);
		grub_putchar ('\n', 255);
		#else
		char *p_t = (char *)SCRATCHADDR;
		while(*p && *p != '\n')
			*p_t++ = *p++;
		*p_t++ = 0;
		expand_var ((char *)SCRATCHADDR, p_t, 0x400);
		printf ("  Booting \'%s\'\n\n",p_t);
		#endif
	}
	else
		printf ("  Booting command-list\n\n");
      }

      /* Set CURRENT_ENTRYNO for the command "savedefault".  */
      current_entryno = first_entry + entryno;
      
      if (! cur_entry)
      {
	//cur_entry = get_entry (config_entries, first_entry + entryno, 1);
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

//  /* if we get back here, we should go back to what our term was before */
//  current_term = prev_term;
//  if (current_term->startup)
//      /* if our terminal fails to initialize, fall back to console since
//       * it should always work */
//      if ((*current_term->startup)() == 0)
//          current_term = term_table; /* we know that console is first */
  show_menu = 1;
  goto restart1;
}

// #define GFX_DEBUG

#ifdef SUPPORT_GFX

/* kernel + (grub-)module options */
#define GFX_CMD_BUF_SIZE 512

/* command line separator char */
#define GFX_CMD_SEP 1

/*
 * Search cpio archive for gfx file.
 */
static unsigned find_file (unsigned char *buf, unsigned len,
			   unsigned *gfx_file_start, unsigned *file_len,
			   unsigned *code_start)
{
  unsigned i, fname_len;

  *gfx_file_start = 0;

  for(i = 0; i < len;) 
    {
      if((len - i) >= 0x1a && (buf[i] + (buf[i + 1] << 8)) == 0x71c7)
      {
	fname_len = *(unsigned short *) (buf + i + 20);
	*file_len = *(unsigned short *) (buf + i + 24) + (*(unsigned short *) (buf + i + 22) << 16);
	i += 26 + fname_len;
	i = ((i + 1) & ~1);
	if (*(unsigned *) (buf + i) == 0x0b2d97f00)	/* magic id */
	  {
	    int v = buf[i + 4];

	    *code_start = *(unsigned *) (buf + i + 8);
	    *gfx_file_start = i;
	    if ((v>=5) && (v <= 7))	/* version 5 - 7 */
	      return 1;
	    else if (v == 8)		/* version 8 */
	      return 2;
	  }
	i += *file_len;
	i = ((i + 1) & ~1);
      }
    else
      break;
  }

  return 0;
}

#define MIN_GFX_FREE	0x1000

#define SC_BOOTLOADER		0
#define SC_FAILSAFE		3
#define SC_SYSCONFIG_SIZE	4
#define SC_BOOTLOADER_SEG	8
#define SC_XMEM_0		24
#define SC_XMEM_1		26
#define SC_XMEM_2		28
#define SC_XMEM_3		30
#define SC_FILE			32
#define SC_ARCHIVE_START	36
#define SC_ARCHIVE_END		40
#define SC_MEM0_START		44
#define SC_MEM0_END		48

unsigned long gfx_drive, gfx_partition;

/*
 * Does normally not return.
 */
static void
run_graphics_menu (char *menu_entries, char *config_entries, /*int num_entries,*/
	  char *heap, int entryno)
{
  unsigned char *buf, *buf_ext;
  unsigned buf_size, buf_ext_size, code_start = 0, file_start;
  char *cfg;
  int i, j, max_len, gfx_file_size, verbose;
  int selected_entry;
  gfx_data_v1_t *gfx1;
  gfx_data_v2_t *gfx2;
  //char *cmd_buf;
  unsigned mem0_start, mem0_end, file_len = 0;
  int version;
  unsigned long tmp_drive, tmp_partition;

  /*
   * check gfx_data_t struct offsets for consistency; gcc will optimize away
   * the whole block
   */

  /* dummy function to make ld fail */
  {
    extern void wrong_struct_size(void);
    #define gfx_ofs_check(a) if(gfx_ofs_v1_##a != (char *) &gfx1->a - (char *) &gfx1->ok) wrong_struct_size();
    gfx_ofs_check(ok);
    gfx_ofs_check(mem_start);
    gfx_ofs_check(mem_cur);
    gfx_ofs_check(mem_max);
    gfx_ofs_check(code_seg);
    gfx_ofs_check(jmp_table);
    gfx_ofs_check(sys_cfg);
    gfx_ofs_check(cmdline);
    gfx_ofs_check(cmdline_len);
    gfx_ofs_check(menu_list);
    gfx_ofs_check(menu_default_entry);
    gfx_ofs_check(menu_entries);
    gfx_ofs_check(menu_entry_len);
    gfx_ofs_check(args_list);
    gfx_ofs_check(args_entry_len);
    gfx_ofs_check(timeout);
    gfx_ofs_check(mem_file);
    gfx_ofs_check(mem_align);
    #undef gfx_ofs_check

    #define gfx_ofs_check(a) if(gfx_ofs_v2_##a != (char *) &gfx2->a - (char *) &gfx2->ok) wrong_struct_size();
    gfx_ofs_check(ok);
    gfx_ofs_check(code_seg);
    gfx_ofs_check(jmp_table);
    gfx_ofs_check(sys_cfg);
    gfx_ofs_check(cmdline);
    gfx_ofs_check(cmdline_len);
    gfx_ofs_check(menu_list);
    gfx_ofs_check(menu_default_entry);
    gfx_ofs_check(menu_entries);
    gfx_ofs_check(menu_entry_len);
    gfx_ofs_check(args_list);
    gfx_ofs_check(args_entry_len);
    gfx_ofs_check(timeout);
    #undef gfx_ofs_check
  }

  if(!num_entries) return;

  kernel_type = KERNEL_TYPE_NONE;

  gfx1 = (gfx_data_v1_t *) (heap = (char *)0x50000);
  heap = (char *) (((unsigned) heap + sizeof (*gfx1) + 0xF) & ~0xF);
  gfx2 = (gfx_data_v2_t *) heap;
  heap += sizeof *gfx2;
  memset((char *) gfx1, 0, (char *) heap - (char *) gfx1);
  
  //verbose = (*(unsigned char *) 0x417) & 3 ? 1 : 0;	/* SHIFT pressed */
  verbose = (debug > 1);
  
  /* setup command line edit buffer */

  gfx1->cmdline_len = gfx2->cmdline_len = 256;
  gfx1->cmdline = gfx2->cmdline = heap;
  heap += gfx1->cmdline_len;
  memset(gfx1->cmdline, 0, gfx1->cmdline_len);

  //cmd_buf = heap;
  heap += GFX_CMD_BUF_SIZE;

  /* setup menu entries */

  for (i = max_len = 0; i < num_entries; i++)
    {
	char ch;
	char *str = get_entry(menu_entries, i);

	j = 0;
	/* ending at LF, the start of help message */
	while ((ch = *str++) && ch != '\n')
		j++;

	if (max_len < j)
	    max_len = j;
    }

  if (!max_len) return;

  gfx1->menu_entry_len = gfx2->menu_entry_len = max_len;
  gfx1->menu_entries = gfx2->menu_entries = num_entries;

  gfx1->menu_list = gfx2->menu_list = heap;
  heap += gfx1->menu_entry_len * gfx1->menu_entries;

  memset(gfx1->menu_list, 0, gfx1->menu_entry_len * gfx1->menu_entries);

  for(i = 0; i < (int) gfx1->menu_entries; i++)
    {
	char ch;
	char *dest = gfx1->menu_list + i * gfx1->menu_entry_len;
	char *src = get_entry(menu_entries, i);

	/* Skip the leading '\t'.  */
	src++;
	
	/* ending at LF, the start of help message */
	while ((ch = *src++) && ch != '\n')
		*dest++ = ch;
	*dest = 0;
    }

  gfx1->menu_default_entry = gfx2->menu_default_entry = gfx1->menu_list + entryno * gfx1->menu_entry_len;

  gfx1->args_entry_len = gfx2->args_entry_len = 1;
  gfx1->args_list = gfx2->args_list = heap;
  heap += gfx1->args_entry_len * gfx1->menu_entries;
  memset(gfx1->args_list, 0, gfx1->args_entry_len * gfx1->menu_entries);

  /* use 1MB starting at 4MB as file buffer */
  buf_ext = (unsigned char *) (4 << 20);
  buf_ext_size = 1 << 20;

  /* must be 16-byte aligned */
  buf = (unsigned char *) (((unsigned) heap + 0xf) & ~0xf);

  buf_size = ((*((unsigned short *)0x413)) << 10) - (unsigned) buf;
  buf_size &= ~0xf;

  mem0_start = (unsigned) buf;
  mem0_end = mem0_start + buf_size;

#ifdef GFX_DEBUG
  if (verbose)
    {
      printf("low memory 0x%x - 0x%x (%d bytes)\n", mem0_start, mem0_end, buf_size);
    }
#endif

  /* read the file */
  tmp_drive = saved_drive;
  tmp_partition = saved_partition;
  saved_drive = gfx_drive;
  saved_partition = gfx_partition;
  if (!grub_open(graphics_file)) 
    {
      if (verbose)
	printf("%s: file not found\n", graphics_file);

      saved_drive = tmp_drive;
      saved_partition = tmp_partition;
      return;
    }

  gfx_file_size = grub_read ((unsigned long long)(unsigned int)(char *)buf_ext, buf_ext_size, 0xedde0d90);

  grub_close();

  saved_drive = tmp_drive;
  saved_partition = tmp_partition;

  if (gfx_file_size <= 0)
    {
      if (verbose)
	printf("%s: read error\n", graphics_file);

      return;
    }

#ifdef GFX_DEBUG
  if (verbose)
    {
      printf("%s: %d bytes (%d bytes left)\n", graphics_file, gfx_file_size, (buf_ext_size - gfx_file_size));
    }
#endif

  /* locate file inside cpio archive */
  if (!(version = find_file(buf_ext, gfx_file_size, &file_start, &file_len, &code_start)))
    {
      if (verbose)
	printf("%s: invalid file format\n", graphics_file);

      return;
    }

#ifdef GFX_DEBUG
  if (verbose)
    {
      printf("init: start 0x%x, len %d; code offset 0x%x\n", file_start, file_len, code_start);
    }
#endif

  if (version == 1)
    {
      unsigned u;

      if (gfx_file_size + MIN_GFX_FREE + 0xf >= (int) buf_size)
	{
	  if (verbose)
	    printf("not enough free memory: %d extra bytes need\n", (gfx_file_size + MIN_GFX_FREE - buf_size));

	  return;
	}

      memcpy ((void *) buf, (void *) buf_ext, gfx_file_size);

      gfx1->sys_cfg[0] = 2;	/* bootloader: grub */
      gfx1->timeout = grub_timeout >= 0 ? grub_timeout : 0;

      gfx1->mem_start = (unsigned) buf;
      gfx1->mem_max = gfx1->mem_start + buf_size;

      gfx1->mem_cur = gfx1->mem_start +
	((gfx_file_size + 0x0f + 3) & ~3);	/* align it */

      /* align it */
      u = (-(code_start + gfx1->mem_start + file_start)) & 0x0f;
      gfx1->mem_align = gfx1->mem_start + u;
      gfx1->mem_file = gfx1->mem_align + file_start;
      if (u)
	{
	  memcpy((void *) gfx1->mem_align, (void *) gfx1->mem_start, gfx_file_size);
	}

      code_start += gfx1->mem_file;
      gfx1->code_seg = code_start >> 4;
  
      for (i = 0; (unsigned) i < sizeof gfx1->jmp_table / sizeof *gfx1->jmp_table; i++) 
	{
	  gfx1->jmp_table[i] = (gfx1->code_seg << 16) + ((unsigned short *) code_start)[i];
	}
      
      if (gfx_init_v1 (gfx1))
	{
	  if (verbose)
	    printf("graphics initialization failed\n");

	  return;
	}
      
      gfx_setup_menu_v1 (gfx1);
      i = gfx_input_v1 (gfx1, &selected_entry);
      gfx_done_v1 (gfx1);
    }
  else
    {
      if (file_len - code_start + MIN_GFX_FREE > buf_size)
	{
	  if (verbose)
	    printf("not enough free memory: %d extra bytes need\n", (file_len - code_start + MIN_GFX_FREE - buf_size));

	  return;
	}

      gfx2->sys_cfg[SC_BOOTLOADER] = 2;			/* bootloader: grub */
      gfx2->sys_cfg[SC_SYSCONFIG_SIZE] = 52;		/* config data size */
      *(unsigned short *) (gfx2->sys_cfg + SC_BOOTLOADER_SEG) = (unsigned) gfx2 >> 4;	/* segment */
      gfx2->sys_cfg[SC_XMEM_0] = 0x21;			/* 1MB @ 2MB */
      gfx2->sys_cfg[SC_XMEM_1] = 0x41;			/* 1MB @ 4MB */
      gfx2->sys_cfg[SC_FAILSAFE] = verbose;
      gfx2->timeout = grub_timeout >= 0 ? grub_timeout : 0;

      memcpy((void *) buf, (void *) (buf_ext + file_start + code_start), file_len - code_start);

      mem0_start += file_len - code_start;
      mem0_start = (mem0_start + 3) & ~3;		/* align */

      /* init interface to graphics functions */

      *(unsigned *) (gfx2->sys_cfg + SC_FILE) = (unsigned) buf_ext + file_start;
      *(unsigned *) (gfx2->sys_cfg + SC_ARCHIVE_START) = (unsigned) buf_ext;
      *(unsigned *) (gfx2->sys_cfg + SC_ARCHIVE_END) = (unsigned) buf_ext + gfx_file_size;
      *(unsigned *) (gfx2->sys_cfg + SC_MEM0_START) = mem0_start;
      *(unsigned *) (gfx2->sys_cfg + SC_MEM0_END) = mem0_end;

      gfx2->code_seg = (unsigned) buf >> 4;

#ifdef GFX_DEBUG

      if (verbose)
	{
	  printf("init 0x%x, archive 0x%x - 0x%x, low mem 0x%x - 0x%x\ncode seg 0x%x\n",
		 ((unsigned) buf_ext + file_start),
		 (unsigned) buf_ext, ((unsigned) buf_ext + gfx_file_size),
		 mem0_start, mem0_end, gfx2->code_seg
	  );
	}
#endif

      for (i = 0; (unsigned) i < sizeof gfx2->jmp_table / sizeof *gfx2->jmp_table; i++)
	{
	  gfx2->jmp_table[i] = (gfx2->code_seg << 16) + ((unsigned short *) buf)[i];
	}

#ifdef GFX_DEBUG
      if (verbose)
	{
	  for(i = 0; i < 12; i++)
	    {
	      printf("%d: 0x%x\n", i, gfx2->jmp_table[i]);
	    }

	  for(i = 0; i < gfx2->menu_entries; i++) 
	    {
	      printf("\"%s\"  --  \"%s\"\n",
		     (gfx2->menu_list + i * gfx2->menu_entry_len),
		     (gfx2->args_list + i * gfx2->args_entry_len)
	      );
	    }

	  printf("default: \"%s\"\n", gfx2->menu_default_entry);
	}
#endif

      /* switch to graphics mode */
      if (gfx_init_v2 (gfx2))
	{
	  if (verbose)
	    printf("graphics initialization failed\n");

	  return;
	}

      gfx_setup_menu_v2 (gfx2);
      i = gfx_input_v2 (gfx2, &selected_entry);
      gfx_done_v2 (gfx2);
    }

  memset ((char *) HISTORY_BUF, 0, HISTORY_BUFLEN);
  /* ESC -> show text menu */

  if (i == 1)
    {
      grub_timeout = -1;
      timeout_enable = 0;
      return;
    }

  //heap = saved_heap;	/* free most of the graphics data */

  // printf("cmdline: >%s<, entry = %d\n", gfx_data->cmdline, selected_entry);

  if (selected_entry < 0 || selected_entry > num_entries)
    return;

  /* for 'savedefault' */
  current_entryno = selected_entry;

  cfg = get_entry(menu_entries, selected_entry);
  while (*(cfg++))
    ;

  run_script(cfg, heap);
}

#endif /* SUPPORT_GFX */

static int
get_line_from_config (char *cmdline, int max_len, int preset)
{
    unsigned long pos = 0, info = 0;//literal = 0, comment = 0;
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
	    if (! grub_read ((unsigned long long)(unsigned int)&c, 1, 0xedde0d90))
	    {
		if (errnum)
		{
			printf ("  Fatal! Read menu error!\n");
			print_error ();
		}
		break;
	    }
	}

	///* Skip UTF-8 Byte Order Mark: EF BB BF */
	//if ((c & 0x80) && pos < 3)
	//    continue;

	/* Replace CR with LF.  */
	if (c == '\r')
	    c = '\n';

	/* Replace tabs with spaces.  */
	if (c == '\t')//( || c == '\f' || c == '\v')
	    c = ' ';

	/* all other non-printable chars are illegal. */
	if (c != '\n' && (unsigned char)c < ' ')
	{
//	    pos = 0;	2023-02-11  压缩预置菜单最后一行没有回车符，会出错
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

	    if (pos < max_len)
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
static char *config_entries, *cur_entry;

static void
reset (void)
{
  count_lines = -1;
  config_len = 0;
  num_entries = 0;
  cur_entry = config_entries;

  /* Initialize the data for the configuration file.  */
  default_entry = 0;
  password_buf = 0;
  fallback_entryno = -1;
  fallback_entries[0] = -1;
  grub_timeout = -1;
  timeout_enable = 0;
  menu_num_ctrl[0] = 0;
}
  
extern struct builtin builtin_title;
extern struct builtin builtin_graphicsmode;
extern struct builtin builtin_debug;
static unsigned long attr = 0;
/* This is the starting function in C.  */
void
cmain (void)
{
    debug = debug_boot + 1;
    title_boot = (unsigned short *) init_free_mem_start;
    titles = (char * *)(init_free_mem_start + 1024);
    config_entries = (char*)init_free_mem_start + 1024 + 256 * sizeof (char *);
    saved_entryno = 0;
	new_menu = 0;
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
	unsigned long graphicsmode_in_menu_init = 0;
	unsigned long debug_in_menu_init = 0;
	//char *cmdline;
	int is_preset;
	grub_memset (graphic_file_shift, 0, 32);
	menu_init_script_file[0] = 0;
	{
	    int is_opened;

	    is_preset = is_opened = 0;
	    /* Try command-line menu first if it is specified. */
	    //if (preset_menu == (char *)0x0800/*&& ! *config_file*/)
	    if (use_preset_menu)
	    {
		is_opened = is_preset = open_preset_menu ();
	    }
	    if (! is_opened)
	    {
		/* Try config_file */
		if (*config_file)
		{
			is_opened = (configfile_opened || grub_open (config_file));
			#ifdef FSYS_IPXE
			if (is_opened && current_drive == PXE_DRIVE && current_partition == IPXE_PART)
				pxe_detect(IPXE_PART,config_file);
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
	    if ((int)builtin != -1 && builtin->flags == 0)	/* title command */
	    {
		if (builtin != &builtin_title)/*If title*/
		{
			unsigned long tmp_filpos;
			unsigned long tmp_drive = saved_drive;
			unsigned long tmp_partition = saved_partition;
			unsigned int rp;
			cmdline = skip_to(1, cmdline);

			/* save original file position. */
			tmp_filpos = (is_preset && preset_menu == (const char *)0x800) ?
					preset_menu_offset : filepos;

			/* close the already opened file for safety, in case 
			 * the builtin->func() below would call
			 * grub_open(). */
			if (! is_preset || preset_menu != (const char *)0x800)
				grub_close ();

			if (debug_boot)
			{
				putchar_hooked = 0;
				grub_printf("IFTITLE: %s->",cmdline);
				putchar_hooked = (unsigned char*)1;
			}

			rp = builtin->func(cmdline,BUILTIN_IFTITLE);
			saved_drive = tmp_drive;
			saved_partition = tmp_partition;
			current_drive = GRUB_INVALID_DRIVE;
			buf_drive = -1;

			/* re-open the config_file which is still in use by
			 * get_line_from_config(), and restore file position
			 * with the saved value. */
			if (is_preset)
			{
				open_preset_menu ();
				if (preset_menu == (const char *)0x800)
					preset_menu_offset = tmp_filpos;
				else
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

			if (debug_boot)
			{
				putchar_hooked = 0;
				grub_printf("  rp=%d\n", rp);
				DEBUG_SLEEP
				putchar_hooked = (unsigned char*)1;
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
			bt += (config_entries[attr] & 1);
		    num_entries++;	/* an entry is completed. */
		    config_entries[config_len++] = 0;	/* finish the entry. */
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
		    config_entries[config_len++] = 0;
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
				titles[num_entries] = config_entries + config_len;
				title_boot[num_entries] = bt;
			}
		    config_entries[config_len++] = 0x08;	/* attribute byte */
		    
		    len = parse_string (ptr);
		    ptr[len] = 0;
		    while ((config_entries[config_len++] = *(ptr++)) != 0);
		}
	    }
	    else if (state & 0x10) /*ignored menu by iftitle*/
		continue;
	    else if (! state)			/* menu init command */
	    {
		if ((int)builtin == -1 || builtin->flags & BUILTIN_MENU)
		{
		    char *ptr = cmdline;
		    /* Copy menu-specific commands to config area.  */
		    while ((config_entries[config_len++] = *ptr++) != 0);
		    prev_config_len = config_len;
		    if (builtin == &builtin_graphicsmode)
			graphicsmode_in_menu_init = 1;
		    if (builtin == &builtin_debug)
			debug_in_menu_init = 1;
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
		    while ((config_entries[config_len++] = *ptr++) != 0);
		    prev_config_len = config_len;
		}
		if ((int)builtin != -1)
			config_entries[attr] |= !!(builtin->flags & BUILTIN_BOOTING);
	    }
	} /* while (get_line_from_config()) */

	putchar_hooked = 0;

	/* file must be closed here, because the menu-specific commands
	 * below may also use the GRUB_OPEN command.  */
	if (is_preset)
	{
#ifdef SUPPORT_GRAPHICS
extern int font_func (char *, int);
extern int graphicsmode_func (char *, int);
	    //font_func (NULL, 0);	/* clear the font */
	    /* Since ROM VGA font already loaded in init_bios_info(), we
	     * should not load it again here. The same as below.
	     * See issue 160. */
	    /* Clear the narrow_char_indicator for the NULL char only. */
//	    *(unsigned long *)UNIFONT_START = 0; // Enable next font command.
	    if (use_preset_menu/* != (const char *)0x800*/)
	    {
		/* load the font embedded in preset menu. */
		char *menu = "(md)4+8";

		if (preset_menu != (const char *)0x800)
			menu = "(md)0x880+0x200";
		if (font_func (menu, 0))
		{
		    /* font exists, automatically enter graphics mode. */
		    if (! graphicsmode_in_menu_init)
		    {
			if (debug_boot)
				grub_printf ("\rSwitch to graphics mode ...             \r");
			graphicsmode_func ("-1 -1 -1 24:32", 0);
		    }
		}
		//font_func (NULL, 0);	/* clear the font */
		/* Clear the narrow_char_indicator for the NULL char only. */
//		*(unsigned long *)UNIFONT_START = 0;//Enable next font command.
	    }
#endif /* SUPPORT_GRAPHICS */

	    if (preset_menu != (const char *)0x800)
		grub_close ();
	    //use_preset_menu = 0;	/* Disable preset menu.  */
	}
	else
	{
	    grub_close ();
	    /* before showing menu, try loading font in the tail of config_file */
#ifdef SUPPORT_GRAPHICS
	    extern int font_func (char *, int);
	    //font_func (NULL, 0);	/* clear the font */
	    /* Clear the narrow_char_indicator for the NULL char only. */
//	    *(unsigned long *)UNIFONT_START = 0; // Enable next font command.
	    font_func (config_file, 0);
	    //font_func (NULL, 0);	/* clear the font */
	    /* Clear the narrow_char_indicator for the NULL char only. */
//	    *(unsigned long *)UNIFONT_START = 0; // Enable next font command.
#endif /* SUPPORT_GRAPHICS */
	}
	use_preset_menu = 0;	/* Disable preset menu.  */

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
	config_entries[config_len++] = 0;
	title_boot[num_entries] = -1;
	if (num_entries < 256)
		titles[num_entries] = 0;
	
	/* old MENU_BUF is not used any more. So MENU_BUF is a temp area,
	 * and can be moved to elsewhere. */

	/* config_entries contains these:
	 * 1. The array of menu init commands.
	 * 2. The array of menu item commands with leading titles.
	 */

	/* Display this info if the debug command is not present in the
	 * menu-init command set.
	 */
	if (debug_boot || ! debug_in_menu_init)
	{
#include "grub4dos_version.h"

#ifdef GRUB4DOS_VERSION
	    if ((unsigned long)saved_partition == 0xFFFFFF)
		printf_debug0 ("\rGRUB4DOS " GRUB4DOS_VERSION
			", root is (0x%X)%s\n",
			(unsigned long)saved_drive,
			saved_dir);
	    else
		printf_debug0 ("\rGRUB4DOS " GRUB4DOS_VERSION
			", root is (0x%X,%d)%s\n",
			(unsigned long)saved_drive,
			(unsigned char)(saved_partition >> 16),
			saved_dir);
#endif
	    if (is_preset)
	    {
		if (preset_menu == (const char *)0x800)
		    printf_debug0 ("\rProcessing the preset-menu ...\n");
		else
		    printf_debug0 ("\rProcessing the LZMA preset-menu ...\n");
	    }
	    else
		printf_debug0 ("\rProcessing menu file %s ...\n", config_file);
	    DEBUG_SLEEP
	}

	/* Run menu-specific commands before any other menu entry commands.  */

	{
	    static char *old_entry = NULL;
	    static char *heap = NULL; heap = config_entries + config_len;

	    static int old_debug = 1;

	    /* Initialize the data.  */
	    //saved_drive = boot_drive;
	    //saved_partition = install_partition;
	    current_drive = GRUB_INVALID_DRIVE;
	    count_lines = -1;
	    kernel_type = KERNEL_TYPE_NONE;
	    errnum = 0;

	    while (1)
	    {
//		struct builtin *builtin; //2010-12-16 commented by chenall
//		char *arg;
//		grub_error_t errnum_old;//2010-12-16 commented by chenall

		pxe_restart_config = 0;

#ifdef SUPPORT_GFX
		*graphics_file = 0;
#endif

//		errnum_old = errnum;//2010-12-16 commented by chenall
//		errnum = 0;//2010-12-16 commented by chenall

		if (debug_boot) {
		  grub_printf("\r%s\n", cur_entry);
		}
		//DEBUG_SLEEP  /* Only uncomment if you want to pause before processing every menu.lst line */
		/* Copy the first string in CUR_ENTRY to HEAP.  */
		old_entry = cur_entry;
		while (*cur_entry++);

		grub_memmove (heap, old_entry, (int) cur_entry - (int) old_entry);
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
		if (debug_boot)
		    if ((unsigned int)debug < 2)	/* debug == 0 or 1 */
		    {
			old_debug = debug;	/* save the new debug in old_debug */
			debug = 2;
		    }

#ifdef SUPPORT_GFX
		if (num_entries && ! errnum && *graphics_file && !password_buf && show_menu && grub_timeout)
		{
		  unsigned long pxe_restart_config_bak = pxe_restart_config;
		  pxe_restart_config = 1;	/* for configfile to work with gfxmenu. */

		  run_graphics_menu((char *)titles, cur_entry, /*num_entries,*/ config_entries + config_len, default_entry);

		  pxe_restart_config = pxe_restart_config_bak;	/* restore the original value. */

		}
#endif

		if (pxe_restart_config)
			goto restart_config;

original_config:

		if (! *old_entry)
		    break;
	    } /* while (1) */

	    kernel_type = KERNEL_TYPE_NONE;

	    if (debug_boot)
	    {
		debug = old_debug; /* restore user-specified debug level. */
		printf_debug0 ("\n\nEnd of menu init commands. Press any key to enter command-line or run menu...");
	    }
	    DEBUG_SLEEP
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

	use_preset_menu = 0;	/* Disable the preset menu.  */

	pxe_restart_config = 1;	/* pxe_detect will use configfile to run menu */

    /* go ahead and make sure the terminal is setup */
    if (current_term->startup)
	(*current_term->startup)();

    if (! num_entries)
    {
	/* no config file, goto command-line, starting heap from where the
	   config entries would have been stored if there were any.  */
	enter_cmdline (config_entries, 1);
    }
    else
    {
	/* Run menu interface.  */
	/* cur_entry point to the first menu item command. */
	if (hotkey_func)
		hotkey_func(0,0,-1);
	run_menu ((char *)titles, cur_entry, /*num_entries,*/ config_entries + config_len, default_entry);
    }
    goto restart2;
}
