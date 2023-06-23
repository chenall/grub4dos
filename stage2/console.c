/* term_console.c - console input and output */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002  Free Software Foundation, Inc.
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

/* These functions are defined in asm.S instead of this file:
   console_putchar, console_checkkey, console_getkey, console_getxy,
   console_gotoxy, console_cls, and console_nocursor.  */

//extern void toggle_blinking (void);
int console_color[COLOR_STATE_MAX] = {
  [COLOR_STATE_STANDARD] = A_NORMAL,
  /* represents the user defined colors for normal text */
  [COLOR_STATE_NORMAL] = A_NORMAL,
  /* represents the user defined colors for highlighted text */
  [COLOR_STATE_HIGHLIGHT] = A_REVERSE,
  /* represents the user defined colors for help text */
  [COLOR_STATE_HELPTEXT] = A_NORMAL,
  /* represents the user defined colors for heading line */
  [COLOR_STATE_HEADING] = A_NORMAL,
  /* represents the user defined colors for border */
  [COLOR_STATE_BORDER] = A_NORMAL
};

unsigned long long console_color_64bit[COLOR_STATE_MAX] = {
  [COLOR_STATE_STANDARD] = 0xAAAAAA,
  /* represents the user defined colors for normal text */
  [COLOR_STATE_NORMAL] = 0xAAAAAA,
  /* represents the user defined colors for highlighted text */
  [COLOR_STATE_HIGHLIGHT] = 0xFFFFFF,								//0xAAAAAA00000000ULL,
  /* represents the user defined colors for help text */
  [COLOR_STATE_HELPTEXT] = 0xAAAAAA,
  /* represents the user defined colors for heading line */
  [COLOR_STATE_HEADING] = 0xAAAAAA,
  /* represents the user defined colors for notes */
//  [COLOR_STATE_BORDER] = 0x3399
  [COLOR_STATE_BORDER] = 0xAAAAAA

};

unsigned char color_64_to_8 (unsigned long long color64);
unsigned char color_32_to_4 (unsigned int color32);
int grub_console_getkey (int flags);
static int console_startup_input (void);
void grub_console_init (void);
grub_efi_simple_text_input_ex_interface_t *text_input_ex;



unsigned char
color_32_to_4 (unsigned int color32)
{
  unsigned char r, g, b, col32, col4=0;
	r = color32 >> 16;
	g = (color32 >> 8) & 0xff;
	b = color32 & 0xff;
	
	if (r >= g)
	{
		if (r >= b)
			col32 = r;
		else
			col32 = b;
	}
	else
	{
		if (g >= b)
			col32 = g;
		else
			col32 = b;
	}
	
	if (col32 > 0xaa)
		col4 |= 8;
	if (r > col32/2)
		col4 |= 4;
	if (g > col32/2)
		col4 |= 2;
	if (b > col32/2)
		col4 |= 1;

	return col4;
}

unsigned char
color_64_to_8 (unsigned long long color64)
{
    return (color_32_to_4 (color64 >> 32) << 4) | color_32_to_4 (color64 & 0xffffff);
}

unsigned long long color_4_to_32 (unsigned char color4);
unsigned long long
color_4_to_32 (unsigned char color4)
{
    switch (color4)
    {
	case 0x00: return 0;
	case 0x01: return 0x0000AA;
	case 0x02: return 0x00AA00;
	case 0x03: return 0x00AAAA;
	case 0x04: return 0xAA0000;
	case 0x05: return 0xAA00AA;
	case 0x06: return 0xAA5500;
	case 0x07: return 0xAAAAAA;
	case 0x08: return 0x555555;
	case 0x09: return 0x5555FF;
	case 0x0A: return 0x55FF55;
	case 0x0B: return 0x55FFFF;
	case 0x0C: return 0xFF5555;
	case 0x0D: return 0xFF55FF;
	case 0x0E: return 0xFFFF55;
	case 0x0F: return 0xFFFFFF;
	default: return 0;
    }
}

unsigned long long color_8_to_64 (unsigned char color8);
unsigned long long
color_8_to_64 (unsigned char color8)
{
    return (color_4_to_32 (color8 >> 4) << 32) | color_4_to_32 (color8 & 15);
}

void console_setcolor(unsigned int state,unsigned long long color[]);
void
console_setcolor(unsigned int state,unsigned long long color[])
{
	int i;
	for(i=0;i< COLOR_STATE_MAX;++i)
	{
		if (!(state & (1<<i)))
			continue;
		console_color[i] = color_64_to_8(color[i]);
		console_color_64bit[i] = color[i];
	}
	console_setcolorstate(COLOR_STATE_STANDARD);
}

//----------------------------------------------------------------------------------------
grub_uint8_t EXPORT_VAR(grub_term_normal_color);
grub_uint8_t EXPORT_VAR(grub_term_highlight_color);


//tern/efi/consile.c

unsigned int console_print_unicode (unsigned int unicode, unsigned int max_width);
unsigned int
console_print_unicode (unsigned int unicode, unsigned int max_width) //控制台打印字符
{
	grub_efi_simple_text_output_interface_t *o;	//简单文本输出接口
	o = grub_efi_system_table->con_out;	//系统表->简单文本输出接口

	unsigned short str[3];
	int i = 0;

	if (unicode == '\n')
		str[i++] = '\r';
	str[i++] = unicode;	//Unicode码
	str[i] = 0;	//结束符
  efi_call_2 (o->output_string, o, str);	//回车，换行各行其是。到行尾可以自动换行。unicode16小尾。0x00结束。
	console_getxy ();

	return 1;
}

void console_putstr_utf8 (char *str);
void
console_putstr_utf8 (char *str) //控制台打印utf字符串
{
  while (*str)
    console_print_unicode ((unsigned int)*str++, 255);
}

void console_putstr_utf16(unsigned short *str);
void
console_putstr_utf16(unsigned short *str)  //控制台打印unicode16字符串
{
  while (*str)
    console_print_unicode ((unsigned int)*str++, 255);
}

struct uefi_keysym
{
  unsigned short scan_code;			//扫描码
  unsigned short unicode_char;	//unicode字符
};

/* The table for key symbols. If the "shifted" member of an entry is
   NULL, the entry does not have shifted state.  */
static struct uefi_keysym uefi_keysym_table[] =
{
//	{0x00, 0x0},		//空扫描代码
	{0x01, 0x4800},	//将光标上移一行
	{0x02, 0x5000},	//将光标下移一行.
	{0x03, 0x4d00},	//将光标右移1列
	{0x04, 0x4b00},	//将光标左移1列.
	{0x05, 0x4700},	//回家 Home
	{0x06, 0x4f00},	//结束 end.
	{0x07, 0x5200},	//插入 Insert.
	{0x08, 0x5300},	//删除 Delete
	{0x09, 0x4900},	//向上翻页 Page Up
	{0x0a, 0x5100},	//向下翻页 Page Down
	{0x0b, 0x3b00},	//F1
	{0x0c, 0x3c00},	//F2
	{0x0d, 0x3d00},	//F3
	{0x0e, 0x3e00},	//F4
	{0x0f, 0x3f00},	//F5
	{0x10, 0x4000},	//F6
	{0x11, 0x4100},	//F7
	{0x12, 0x4200},	//F8
	{0x13, 0x4300},	//F9
	{0x14, 0x4400},	//F10
	{0x17, 0x011b},	//逃离 Escape
	{0x15, 0x8500},	//F11		uefi简单文本扩展协议
	{0x16, 0x8600},	//F12		uefi简单文本扩展协议
};

static unsigned int find_uefi_unicode_char (unsigned short scan_code);
static unsigned int
find_uefi_unicode_char (unsigned short scan_code)
{
	int i;
      
	for (i = 0; i < (int)(sizeof (uefi_keysym_table) / sizeof (uefi_keysym_table[0]));i++)
	{
	  if (scan_code == uefi_keysym_table[i].scan_code)
	    return uefi_keysym_table[i].unicode_char;
	}

	return 0;
}

static int grub_efi_translate_key (grub_efi_input_key_t key);
static int
grub_efi_translate_key (grub_efi_input_key_t key) //翻译键		ok
{
	if (key.scan_code != 0)	//如果扫描码=0
		key.unicode_char = find_uefi_unicode_char (key.scan_code);

	return (key.scan_code << 16 | key.unicode_char);
}

static int grub_console_getkey_con (struct grub_term_input *term __attribute__ ((unused)));
static int
grub_console_getkey_con (struct grub_term_input *term __attribute__ ((unused)))	//控制台获得键_常规		ok
{
  grub_efi_simple_input_interface_t *i;	//简单输入接口
  grub_efi_input_key_t key;							//返回键值
  grub_efi_status_t status;							//读击键成败

  i = grub_efi_system_table->con_in;		//系统表->简单输入接口
  status = efi_call_2 (i->read_key_stroke, i, &key);	//读击键

  if (status != GRUB_EFI_SUCCESS)				//失败
    return GRUB_TERM_NO_KEY;

  return grub_efi_translate_key(key);		//翻译键
}


static int grub_console_getkey_ex(void);
static int
grub_console_getkey_ex(void) //控制台获得键扩展		ok
{
  grub_efi_key_data_t key_data;
  grub_efi_status_t status;
  grub_efi_uint32_t kss;
  grub_efi_uint32_t key;

  grub_efi_simple_text_input_ex_interface_t *text_input = text_input_ex;

  status = efi_call_2 (text_input->read_key_stroke, text_input, &key_data);

  if (status != GRUB_EFI_SUCCESS)
    return GRUB_TERM_NO_KEY;

  kss = key_data.key_state.key_shift_state;		//键移位状态			32位
  key = grub_efi_translate_key(key_data.key);	//翻译键(键信息)	从输入设备返回的EFI扫描代码和Unicode值。32位

  if (key == GRUB_TERM_NO_KEY)	//失败
    return GRUB_TERM_NO_KEY;

  if (kss & GRUB_EFI_SHIFT_STATE_VALID)		//如果移位状态有效
	{
		if ((kss & GRUB_EFI_LEFT_SHIFT_PRESSED			//如果按下左移位键	0x00000002	//左SHIFT
				|| kss & GRUB_EFI_RIGHT_SHIFT_PRESSED)	//或者按下右移位键	0x00000001	//右SHIFT
				/*&& (key & GRUB_TERM_EXTENDED)*/)			//并且有扩展标记		0x00800000  //扩展
			key |= GRUB_TERM_SHIFT;										//则增加SHIFT标记		0x01000000
		if (kss & GRUB_EFI_LEFT_ALT_PRESSED					//如果按下左更改键	0x00000020	//左ALT
				|| kss & GRUB_EFI_RIGHT_ALT_PRESSED)		//或者按下右更改键	0x00000010	//右ALT
			key |= GRUB_TERM_ALT;											//则增加ALT标记			0x04000000
		if (kss & GRUB_EFI_LEFT_CONTROL_PRESSED			//如果按下左控制键	0x00000008	//左CONTROL
				|| kss & GRUB_EFI_RIGHT_CONTROL_PRESSED)//或者按下右控制键	0x00000004	//右CONTROL
			key |= GRUB_TERM_CTRL;										//则增加CTRL标记		0x02000000
	}

  return key;
}

static int console_startup_input (void);
static int
console_startup_input (void)  //控制台输入初始化  判断是否支持扩展输入		ok
{
  grub_efi_guid_t text_input_ex_guid =
    GRUB_EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;

  grub_efi_simple_text_input_ex_interface_t *text_input = text_input_ex;
  if (text_input)
    return 0;

  text_input = grub_efi_open_protocol(grub_efi_system_table->console_in_handler,
				      &text_input_ex_guid,
				      GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL); //打开协议(系统表->控制台输入程序,guid,获得协议)
  text_input_ex = (void *)text_input;

  return 0;
}


//如果键按下，返回：ah=键扫描码,al=ASCII字符;		如果没有按键: flags=0/1=返回-1/一直等待.
int grub_console_getkey (int flags);
int
grub_console_getkey (int flags)  //控制台获得键		控制台终端输入->获得键			ok
{
	int key, tem;
	struct grub_term_input *term;

repeat:
  if (text_input_ex)	//如果支持扩展
    key = grub_console_getkey_ex();	//控制台获得键扩展
  else								//不支持扩展
    key = grub_console_getkey_con(term);	//控制台获得键常规
		
	defer(1);
	if (animated_enable)
	{
		tem = animated();
		if (tem == 0x3c00)
			return 0x3c00;
	}
	if (DateTime_enable)
		DateTime_refresh();
	if (ext_timer)
		(*ext_timer)(0,-1);
	if (timeout_enable)
		timeout_refresh();

	if (!key)
	{
		if (!flags)
			return -1;
		else
			goto repeat;
	}

	return remap_ascii_char(key);
}

int console_checkkey (void);
int console_checkkey (void)	//ok
{
	return grub_console_getkey(0);
}

int console_getkey (void);
int console_getkey (void)	//ok
{
	return grub_console_getkey(1);
}

void get_console_mode (void);
void
get_console_mode (void) //获得控制台模式信息		ok
{
  grub_efi_simple_text_output_interface_t *o;	//简单文本输出接口
  o = grub_efi_system_table->con_out;	//系统表->简单文本输出接口

  grub_efi_status_t status;
  int columns, rows;
  int i, j=3;
  for (i=0; i<j; i++)
  {
    status = efi_call_4 (o->query_mode, o, i, (grub_efi_uintn_t *)(grub_size_t)&columns, (grub_efi_uintn_t *)(grub_size_t)&rows);
    if (status == GRUB_EFI_SUCCESS)
      j = o->mode->max_mode;
      
    grub_printf("\nmax_mode=%d,mode=%d,attribute=%d,column=%d,row=%d,visible=%d",
        o->mode->max_mode,i,o->mode->attribute,columns,rows,o->mode->cursor_visible);
  }
}

int console_getxy (void);
int
console_getxy (void)	//行(0为顶部)  x=列(0为左部) 返回低4位;   y=行(0为顶部)	返回高4位		ok
{
  grub_efi_simple_text_output_interface_t *o;	//简单文本输出接口

  o = grub_efi_system_table->con_out;	//系统表->简单文本输出接口
	fontx = o->mode->cursor_column;
	fonty = o->mode->cursor_row;
  return (int) { fontx | (fonty<<4) };	//返回: 光标列,光标行
}

void console_gotoxy (int x, int y);
void
console_gotoxy (int x, int y)	//控制台设置光标		ok
{
  grub_efi_simple_text_output_interface_t *o;	//简单文本输出接口

  o = grub_efi_system_table->con_out;	//系统表->简单文本输出接口
	efi_call_3 (o->set_cursor_position, o, x, y);	//执行: 设置光标位置
	fontx = x;
	fonty = y;
}

void console_cls (void);
void
console_cls (void)	//控制台清屏		ok
{
  grub_efi_simple_text_output_interface_t *o;	//简单文本输出接口
  grub_efi_int32_t orig_attr;

  o = grub_efi_system_table->con_out;	//系统表->简单文本输出接口
  orig_attr = o->mode->attribute;			//属性
  efi_call_2 (o->set_attributes, o, current_color & 0xf0);			//执行: 设置属性	GRUB_EFI_BACKGROUND_BLACK=00
  efi_call_1 (o->clear_screen, o);															//执行:	清除屏幕
  efi_call_2 (o->set_attributes, o, orig_attr);									//执行: 设置属性
	fontx = 0;
	fonty = 0;
}

void console_setcolorstate (color_state state);
void
console_setcolorstate (color_state state)	//设置控制台颜色--取颜色表中指定序号的颜色赋给控制台, 或者直接输入颜色		ok
{
  grub_efi_simple_text_output_interface_t *o;	//简单文本输出接口
  o = grub_efi_system_table->con_out;	//系统表->简单文本输出接口

	if (state & 0x100)		//是直接输入颜色
	{
		current_color = state & 0x7f;		//背景色是0-7
		goto abc;
	}
	if (state >= COLOR_STATE_MAX)
 		state = COLOR_STATE_STANDARD;
 	current_color = console_color[state];
	current_color_64bit = console_color_64bit[state];
 	if (state == COLOR_STATE_BORDER)
 	{
 		current_color &= 0xf;
 		current_color |= console_color[COLOR_STATE_NORMAL] & 0xf0;
		current_color_64bit &= 0xffffff;
		current_color_64bit |= console_color_64bit[COLOR_STATE_NORMAL] & 0xffffff00000000;
 	}
abc:
	efi_call_2 (o->set_attributes, o, current_color);	
}

unsigned int console_setcursor (unsigned int on);
unsigned int
console_setcursor (unsigned int on) //控制台设置光标  0/1=不显示/显示
{
  grub_efi_simple_text_output_interface_t *o;	//简单文本输出接口

  o = grub_efi_system_table->con_out;	//系统表->简单文本输出接口
  efi_call_2 (o->enable_cursor, o, on);
	return on;
}

int console_startup (void);
int
console_startup (void)  //控制台输出初始化	ok
{
  grub_efi_set_text_mode (1);		//efi设置文本模式
  return 0;
}

void console_shutdown (void);
void
console_shutdown (void)	//控制台输出关闭
{
  grub_efi_set_text_mode (0);
}

void grub_console_init (void);
void
grub_console_init (void)  //控制台初始化  ok
{
  /* FIXME: it is necessary to consider the case where no console control 有必要考虑没有控制台控件存在的情况，但是默认情况下已经在文本模式中。
     is present but the default is already in text mode.  */
	current_term = term_table; /* set terminal to console 将终端设置为控制台*/
  console_startup();
	console_startup_input();
}
//-------------------------------------------------------------------------

void grub_efidisk_fini (void);
void grub_efi_fini (void);
void
grub_efi_fini (void)
{
  grub_efidisk_fini ();
  grub_efi_memory_fini ();
}

void grub_machine_fini (void);
void
grub_machine_fini (void)
{
  grub_efi_fini ();
}



