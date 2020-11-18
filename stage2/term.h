/* term.h - definitions for terminal handling */
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

#ifndef GRUB_TERM_HEADER
#define GRUB_TERM_HEADER	1
#include "shared.h"
/* These are used to represent the various color states we use */
typedef enum
{
  /* represents the color used to display all text that does not use the user
   * defined colors below
   */
  COLOR_STATE_STANDARD,
  /* represents the user defined colors for normal text */
  COLOR_STATE_NORMAL,
  /* represents the user defined colors for highlighted text */
  COLOR_STATE_HIGHLIGHT,
  /* represents the user defined colors for help text */
  COLOR_STATE_HELPTEXT,
  /* represents the user defined colors for heading line */
  COLOR_STATE_HEADING,
  /* represents the user defined colors for border */
  COLOR_STATE_BORDER,
  /*Number of user defined colors*/
  COLOR_STATE_MAX
} color_state;

/* Flags for representing the capabilities of a terminal.  */
/* Some notes about the flags:
   - These flags are used by higher-level functions but not terminals
   themselves.
   - If a terminal is dumb, you may assume that only putchar, getkey and
   checkkey are called.
   - Some fancy features (nocursor, setcolor, and highlight) can be set to
   NULL.  */

/* Set when input characters shouldn't be echoed back.  */
#define TERM_NO_ECHO		(1 << 0)
/* Set when the editing feature should be disabled.  */
#define TERM_NO_EDIT		(1 << 1)
/* Set when the terminal cannot do fancy things.  */
#define TERM_DUMB		(1 << 2)
/* Set when the terminal needs to be initialized.  */
#define TERM_NEED_INIT		(1 << 16)

#if 0
//#ifdef SUPPORT_SERIAL
unsigned int serial_putchar (unsigned int c, unsigned int max_width);
int serial_checkkey (void);
int serial_getkey (void);
int serial_getxy (void);
void serial_gotoxy (int x, int y);
void serial_cls (void);
void serial_setcolorstate (color_state state);
//#endif
#endif

#if 0
//#ifdef SUPPORT_HERCULES
unsigned int hercules_putchar (unsigned int c, unsigned int max_width);
int hercules_getxy (void);
void hercules_gotoxy (int x, int y);
void hercules_cls (void);
unsigned int hercules_setcursor (unsigned int on);
//#endif
#endif

#ifdef SUPPORT_GRAPHICS
extern unsigned int foreground, background, graphics_inited;

void graphics_set_splash(char *splashfile);
int set_videomode (int mode);
unsigned int graphics_putchar (unsigned int c, unsigned int max_width);
int graphics_getxy(void);
void graphics_gotoxy(int x, int y);
void graphics_cls(void);
int graphics_init(void);
void graphics_end(void);

#endif /* SUPPORT_GRAPHICS */

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//F:\grub4dos_dev\g4d_dev\home\dev\grub32\include\grub\unicode.h
struct grub_unicode_compact_range
{
  unsigned start:21;
  unsigned len:9;
  unsigned bidi_type:5;
  unsigned comb_type:8;
  unsigned bidi_mirror:1;
  unsigned join_type:3;
} GRUB_PACKED;

enum grub_comb_type
  {
    GRUB_UNICODE_COMB_NONE = 0,
    GRUB_UNICODE_COMB_OVERLAY = 1,
    GRUB_UNICODE_COMB_HEBREW_SHEVA = 10,
    GRUB_UNICODE_COMB_HEBREW_HATAF_SEGOL = 11,
    GRUB_UNICODE_COMB_HEBREW_HATAF_PATAH = 12,
    GRUB_UNICODE_COMB_HEBREW_HATAF_QAMATS = 13,
    GRUB_UNICODE_COMB_HEBREW_HIRIQ = 14,
    GRUB_UNICODE_COMB_HEBREW_TSERE = 15,
    GRUB_UNICODE_COMB_HEBREW_SEGOL = 16,
    GRUB_UNICODE_COMB_HEBREW_PATAH = 17,
    GRUB_UNICODE_COMB_HEBREW_QAMATS = 18,
    GRUB_UNICODE_COMB_HEBREW_HOLAM = 19,
    GRUB_UNICODE_COMB_HEBREW_QUBUTS = 20,
    GRUB_UNICODE_COMB_HEBREW_DAGESH = 21,
    GRUB_UNICODE_COMB_HEBREW_METEG = 22,
    GRUB_UNICODE_COMB_HEBREW_RAFE = 23,
    GRUB_UNICODE_COMB_HEBREW_SHIN_DOT = 24,
    GRUB_UNICODE_COMB_HEBREW_SIN_DOT = 25,
    GRUB_UNICODE_COMB_HEBREW_VARIKA = 26,
    GRUB_UNICODE_COMB_ARABIC_FATHATAN = 27,
    GRUB_UNICODE_COMB_ARABIC_DAMMATAN = 28,
    GRUB_UNICODE_COMB_ARABIC_KASRATAN = 29,
    GRUB_UNICODE_COMB_ARABIC_FATHAH = 30,
    GRUB_UNICODE_COMB_ARABIC_DAMMAH = 31,
    GRUB_UNICODE_COMB_ARABIC_KASRA = 32,
    GRUB_UNICODE_COMB_ARABIC_SHADDA = 33,
    GRUB_UNICODE_COMB_ARABIC_SUKUN = 34,
    GRUB_UNICODE_COMB_ARABIC_SUPERSCRIPT_ALIF = 35,
    GRUB_UNICODE_COMB_SYRIAC_SUPERSCRIPT_ALAPH = 36,
    GRUB_UNICODE_STACK_ATTACHED_BELOW = 202,
    GRUB_UNICODE_STACK_ATTACHED_ABOVE = 214,
    GRUB_UNICODE_COMB_ATTACHED_ABOVE_RIGHT = 216,
    GRUB_UNICODE_STACK_BELOW = 220,
    GRUB_UNICODE_COMB_BELOW_RIGHT = 222,
    GRUB_UNICODE_COMB_ABOVE_LEFT = 228,
    GRUB_UNICODE_STACK_ABOVE = 230,
    GRUB_UNICODE_COMB_ABOVE_RIGHT = 232,
    GRUB_UNICODE_COMB_YPOGEGRAMMENI = 240,
    /* If combining nature is indicated only by class and
       not "combining type".  */
    GRUB_UNICODE_COMB_ME = 253,
    GRUB_UNICODE_COMB_MC = 254,
    GRUB_UNICODE_COMB_MN = 255,
  };
	
#define GRUB_UNICODE_MAX_CACHED_CHAR 0x20000
/*  Unicode mandates an arbitrary limit.  */
#define GRUB_BIDI_MAX_EXPLICIT_LEVEL 61
extern struct grub_unicode_compact_range grub_unicode_compact[1];
struct grub_unicode_combining
{
  unsigned int code:21;
  enum grub_comb_type type:8;
};

enum
  {
    GRUB_UNICODE_DOTLESS_LOWERCASE_I       = 0x0131,
    GRUB_UNICODE_DOTLESS_LOWERCASE_J       = 0x0237,
    GRUB_UNICODE_COMBINING_GRAPHEME_JOINER = 0x034f,
    GRUB_UNICODE_HEBREW_WAW                = 0x05d5,
    GRUB_UNICODE_ARABIC_START              = 0x0600,
    GRUB_UNICODE_ARABIC_END                = 0x0700,
    GRUB_UNICODE_THAANA_ABAFILI            = 0x07a6,
    GRUB_UNICODE_THAANA_AABAAFILI          = 0x07a7,
    GRUB_UNICODE_THAANA_IBIFILI            = 0x07a8,
    GRUB_UNICODE_THAANA_EEBEEFILI          = 0x07a9,
    GRUB_UNICODE_THAANA_UBUFILI            = 0x07aa,
    GRUB_UNICODE_THAANA_OOBOOFILI          = 0x07ab,
    GRUB_UNICODE_THAANA_EBEFILI            = 0x07ac,
    GRUB_UNICODE_THAANA_EYBEYFILI          = 0x07ad,
    GRUB_UNICODE_THAANA_OBOFILI            = 0x07ae,
    GRUB_UNICODE_THAANA_OABOAFILI          = 0x07af,
    GRUB_UNICODE_THAANA_SUKUN              = 0x07b0,
    GRUB_UNICODE_ZWNJ                      = 0x200c,
    GRUB_UNICODE_ZWJ                       = 0x200d,
    GRUB_UNICODE_LRM                       = 0x200e,
    GRUB_UNICODE_RLM                       = 0x200f,
    GRUB_UNICODE_LRE                       = 0x202a,
    GRUB_UNICODE_RLE                       = 0x202b,
    GRUB_UNICODE_PDF                       = 0x202c,
    GRUB_UNICODE_LRO                       = 0x202d,
    GRUB_UNICODE_RLO                       = 0x202e,
    GRUB_UNICODE_LEFTARROW                 = 0x2190,
    GRUB_UNICODE_UPARROW                   = 0x2191,
    GRUB_UNICODE_RIGHTARROW                = 0x2192,
    GRUB_UNICODE_DOWNARROW                 = 0x2193,
    GRUB_UNICODE_UPDOWNARROW               = 0x2195,
    GRUB_UNICODE_LIGHT_HLINE               = 0x2500,
    GRUB_UNICODE_HLINE                     = 0x2501,
    GRUB_UNICODE_LIGHT_VLINE               = 0x2502,
    GRUB_UNICODE_VLINE                     = 0x2503,
    GRUB_UNICODE_LIGHT_CORNER_UL           = 0x250c,
    GRUB_UNICODE_CORNER_UL                 = 0x250f,
    GRUB_UNICODE_LIGHT_CORNER_UR           = 0x2510,
    GRUB_UNICODE_CORNER_UR                 = 0x2513,
    GRUB_UNICODE_LIGHT_CORNER_LL           = 0x2514,
    GRUB_UNICODE_CORNER_LL                 = 0x2517,
    GRUB_UNICODE_LIGHT_CORNER_LR           = 0x2518,
    GRUB_UNICODE_CORNER_LR                 = 0x251b,
    GRUB_UNICODE_BLACK_UP_TRIANGLE         = 0x25b2,
    GRUB_UNICODE_BLACK_RIGHT_TRIANGLE      = 0x25ba,
    GRUB_UNICODE_BLACK_DOWN_TRIANGLE       = 0x25bc,
    GRUB_UNICODE_BLACK_LEFT_TRIANGLE       = 0x25c4,
    GRUB_UNICODE_VARIATION_SELECTOR_1      = 0xfe00,
    GRUB_UNICODE_VARIATION_SELECTOR_16     = 0xfe0f,
    GRUB_UNICODE_TAG_START                 = 0xe0000,
    GRUB_UNICODE_TAG_END                   = 0xe007f,
    GRUB_UNICODE_VARIATION_SELECTOR_17     = 0xe0100,
    GRUB_UNICODE_VARIATION_SELECTOR_256    = 0xe01ef,
    GRUB_UNICODE_LAST_VALID                = 0x10ffff
  };

enum grub_bidi_type
  {
    GRUB_BIDI_TYPE_L = 0,
    GRUB_BIDI_TYPE_LRE,
    GRUB_BIDI_TYPE_LRO,
    GRUB_BIDI_TYPE_R,
    GRUB_BIDI_TYPE_AL,
    GRUB_BIDI_TYPE_RLE,
    GRUB_BIDI_TYPE_RLO,
    GRUB_BIDI_TYPE_PDF,
    GRUB_BIDI_TYPE_EN,
    GRUB_BIDI_TYPE_ES,
    GRUB_BIDI_TYPE_ET,
    GRUB_BIDI_TYPE_AN,
    GRUB_BIDI_TYPE_CS,
    GRUB_BIDI_TYPE_NSM,
    GRUB_BIDI_TYPE_BN,
    GRUB_BIDI_TYPE_B,
    GRUB_BIDI_TYPE_S,
    GRUB_BIDI_TYPE_WS,
    GRUB_BIDI_TYPE_ON
  };

/* This structure describes a glyph as opposed to character.  这个结构描述了一个字形，而不是字符。*/
struct grub_unicode_glyph
{
  unsigned int base:23; /* minimum: 21 */
  unsigned short variant:9; /* minimum: 9 */

  unsigned char attributes:5; /* minimum: 5 */
  unsigned char bidi_level:6; /* minimum: 6 */
  enum grub_bidi_type bidi_type:5; /* minimum: :5 */

  unsigned ncomb:8;
  /* Hint by unicode subsystem how wide this character usually is.
     Real width is determined by font. Set only in UTF-8 stream.  */
  int estimated_width:8;

  unsigned int orig_pos;
  union
  {
    struct grub_unicode_combining combining_inline[sizeof (void *)
						   / sizeof (struct grub_unicode_combining)];
    struct grub_unicode_combining *combining_ptr;
  };
};


struct term_entry
{
  /* The name of a terminal.  */
  const char *name;
  /* The feature flags defined above.  */
  unsigned int flags;
  /* Default for screen width in chars if not specified */
  unsigned short chars_per_line;
  /* Default for maximum number of lines if not specified */
  unsigned short max_lines;
  /* Put a character.  */
  unsigned int (*putchar) (unsigned int c, unsigned int max_width);
  /* Check if any input character is available.  */
  int (*checkkey) (void);
  /* Get a character.  */
  int (*getkey) (void);
  /* Get the cursor position. The return value is ((X << 8) | Y).  */
  int (*getxy) (void);
  /* Go to the position (X, Y).  */
  void (*gotoxy) (int x, int y);
  /* Clear the screen.  */
  void (*cls) (void);
  /* Set the current color to be used */
  void (*setcolorstate) (color_state state);
  /* Set the normal color and the highlight color. The format of each
     color is VGA's.  */
  void (*setcolor) (unsigned int state,unsigned long long color[]);
  /* Turn on/off the cursor.  */
  unsigned int (*setcursor) (unsigned int on);

  /* function to start a terminal */
  int (*startup) (void);
  /* function to use to shutdown a terminal */
  void (*shutdown) (void);
};

/* This lists up available terminals.  */
extern struct term_entry term_table[];
/* This points to the current terminal. This is useful, because only
   a single terminal is enabled normally.  */
extern struct term_entry *current_term;

/* The console stuff.  */
extern unsigned int console_putchar (unsigned int c, unsigned int max_width);
extern int console_checkkey (void);
extern int console_getkey (void);
extern int console_getxy (void);
extern void console_gotoxy (int x, int y);
extern void console_cls (void);
extern void console_setcolorstate (color_state state);
extern void console_setcolor(unsigned int state,unsigned long long color[]);
extern unsigned int console_setcursor (unsigned int on);
extern int console_startup (void);
extern void console_shutdown (void);
extern int grub_console_getkey (int flags);


//F:\grub4dos_dev\g4d_dev\home\dev\grub32\include\grub\list.h
struct grub_list
{
  struct grub_list *next;
  struct grub_list **prev;
};
typedef struct grub_list *grub_list_t;

#define FOR_LIST_ELEMENTS(var, list) for ((var) = (list); (var); (var) = (var)->next)
#define FOR_LIST_ELEMENTS_SAFE(var, nxt, list) for ((var) = (list), (nxt) = ((var) ? (var)->next : 0); (var); (var) = (nxt), ((nxt) = (var) ? (var)->next : 0))



//F:\grub4dos_dev\g4d_dev\home\dev\grub32\include\grub\sysbol.h
#define EXPORT_VAR(x)	x
#define EXPORT_FUNC(x)	x

//F:\grub4dos_dev\g4d_dev\home\dev\grub32\include\grub\types.h
//typedef unsigned char		grub_uint8_t;
//typedef unsigned short	grub_uint16_t;
//typedef unsigned long		grub_uint32_t;    //不要修改long
//typedef grub_uint32_t	grub_size_t;


//F:\grub4dos_dev\g4d_dev\home\dev\grub32\include\grub\term.h
#define GRUB_TERM_NO_KEY        0

/* Internal codes used by GRUB to represent terminal input. GRUB用来表示终端输入的内部代码 */
/* Only for keys otherwise not having shifted modification. 仅用于键，否则没有移位修改  */
#define GRUB_TERM_SHIFT         0x01000000
#define GRUB_TERM_CTRL          0x02000000
#define GRUB_TERM_ALT           0x04000000

/* Keys without associated character. 没有关联字符的键 */
#define GRUB_TERM_EXTENDED      0x00800000    //扩展
#define GRUB_TERM_KEY_MASK      0x00ffffff    //多重移幅键控

#define GRUB_TERM_KEY_LEFT      (GRUB_TERM_EXTENDED | 0x4b)
#define GRUB_TERM_KEY_RIGHT     (GRUB_TERM_EXTENDED | 0x4d)
#define GRUB_TERM_KEY_UP        (GRUB_TERM_EXTENDED | 0x48)
#define GRUB_TERM_KEY_DOWN      (GRUB_TERM_EXTENDED | 0x50)
#define GRUB_TERM_KEY_HOME      (GRUB_TERM_EXTENDED | 0x47)
#define GRUB_TERM_KEY_END       (GRUB_TERM_EXTENDED | 0x4f)
#define GRUB_TERM_KEY_DC        (GRUB_TERM_EXTENDED | 0x53)
#define GRUB_TERM_KEY_PPAGE     (GRUB_TERM_EXTENDED | 0x49)
#define GRUB_TERM_KEY_NPAGE     (GRUB_TERM_EXTENDED | 0x51)
#define GRUB_TERM_KEY_F1        (GRUB_TERM_EXTENDED | 0x3b)
#define GRUB_TERM_KEY_F2        (GRUB_TERM_EXTENDED | 0x3c)
#define GRUB_TERM_KEY_F3        (GRUB_TERM_EXTENDED | 0x3d)
#define GRUB_TERM_KEY_F4        (GRUB_TERM_EXTENDED | 0x3e)
#define GRUB_TERM_KEY_F5        (GRUB_TERM_EXTENDED | 0x3f)
#define GRUB_TERM_KEY_F6        (GRUB_TERM_EXTENDED | 0x40)
#define GRUB_TERM_KEY_F7        (GRUB_TERM_EXTENDED | 0x41)
#define GRUB_TERM_KEY_F8        (GRUB_TERM_EXTENDED | 0x42)
#define GRUB_TERM_KEY_F9        (GRUB_TERM_EXTENDED | 0x43)
#define GRUB_TERM_KEY_F10       (GRUB_TERM_EXTENDED | 0x44)
#define GRUB_TERM_KEY_F11       (GRUB_TERM_EXTENDED | 0x57)
#define GRUB_TERM_KEY_F12       (GRUB_TERM_EXTENDED | 0x58)
#define GRUB_TERM_KEY_INSERT    (GRUB_TERM_EXTENDED | 0x52)
#define GRUB_TERM_KEY_CENTER    (GRUB_TERM_EXTENDED | 0x4c)

/* Hex value is used for ESC, since '\e' is nonstandard. 十六进制值用于ESC，因为“\e”是非标准的 */
#define GRUB_TERM_ESC		0x1b
#define GRUB_TERM_TAB		'\t'
#define GRUB_TERM_BACKSPACE	'\b'

#define GRUB_PROGRESS_NO_UPDATE -1  //没有进行更新
#define GRUB_PROGRESS_FAST      0   //进展快速
#define GRUB_PROGRESS_SLOW      2   //进展缓慢

/* These are used to represent the various color states we use. 这些被用来表示我们使用的各种颜色状态 */
typedef enum
  {
    /* The color used to display all text that does not use the
       user defined colors below. 用于显示不使用用户定义颜色的所有文本的颜色 */
    GRUB_TERM_COLOR_STANDARD,
    /* The user defined colors for normal text.用于正常文本的用户定义颜色  */
    GRUB_TERM_COLOR_NORMAL,
    /* The user defined colors for highlighted text. 用于突出显示文本的用户定义颜色 */
    GRUB_TERM_COLOR_HIGHLIGHT
  }
grub_term_color_state;

/* Flags for representing the capabilities of a terminal. 表示终端能力的标志 */
/* Some notes about the flags: 关于标记的几点注释
   - These flags are used by higher-level functions but not terminals
   themselves. 这些标志是由高级函数使用的，而不是终端本身使用的
   - If a terminal is dumb, you may assume that only putchar, getkey and
   checkkey are called. 如果终端是愚蠢的，你可以假设只打印字符，获得键，检查键被调用
   - Some fancy features (setcolorstate, setcolor and setcursor) can be set
   to NULL. 一些奇特的特征(设置颜色状态，设置颜色和设置游标)可以设置为空  */

/* Set when input characters shouldn't be echoed back. 设置输入字符不回写 1 */
#define GRUB_TERM_NO_ECHO	        (1 << 0)
/* Set when the editing feature should be disabled. 设置禁用编辑 2 */
#define GRUB_TERM_NO_EDIT	        (1 << 1)
/* Set when the terminal cannot do fancy things. 设置终端不能花哨的事 4 */
#define GRUB_TERM_DUMB		        (1 << 2)
/* Which encoding does terminal expect stream to be. 终端期望哪种编码 */
#define GRUB_TERM_CODE_TYPE_SHIFT       3                                   //换挡  3
#define GRUB_TERM_CODE_TYPE_MASK	        (7 << GRUB_TERM_CODE_TYPE_SHIFT)  //多重移幅键控  0x38
/* Only ASCII characters accepted. 只有ASCII字符被接受 0 */
#define GRUB_TERM_CODE_TYPE_ASCII	        (0 << GRUB_TERM_CODE_TYPE_SHIFT)
/* Expects CP-437 characters (ASCII + pseudographics). 期待CP-437字符(ASCII +假字符) 8 */
#define GRUB_TERM_CODE_TYPE_CP437	                (1 << GRUB_TERM_CODE_TYPE_SHIFT)
/* UTF-8 stream in logical order. Usually used for terminals UTF-8按逻辑顺序进行流. 通常用于仅将流转发到另一台计算机的终端。 0x10
   which just forward the stream to another computer.  */
#define GRUB_TERM_CODE_TYPE_UTF8_LOGICAL       	(2 << GRUB_TERM_CODE_TYPE_SHIFT)
/* UTF-8 in visual order. Like UTF-8 logical but for buggy endpoints. 视觉顺序为UTF-8.类似UTF-8逻辑，但对于buggy终端 0x18  */
#define GRUB_TERM_CODE_TYPE_UTF8_VISUAL	        (3 << GRUB_TERM_CODE_TYPE_SHIFT)
/* Glyph description in visual order. 视觉顺序中的字形描述 0x20 */
#define GRUB_TERM_CODE_TYPE_VISUAL_GLYPHS       (4 << GRUB_TERM_CODE_TYPE_SHIFT)


/* Bitmasks for modifier keys returned by grub_getkeystatus. '获得键状态'返回的修改键的位掩码 */
#define GRUB_TERM_STATUS_RSHIFT	(1 << 0)
#define GRUB_TERM_STATUS_LSHIFT	(1 << 1)
#define GRUB_TERM_STATUS_RCTRL	(1 << 2)
#define GRUB_TERM_STATUS_RALT	(1 << 3)
#define GRUB_TERM_STATUS_SCROLL	(1 << 4)
#define GRUB_TERM_STATUS_NUM	(1 << 5)
#define GRUB_TERM_STATUS_CAPS	(1 << 6)
#define GRUB_TERM_STATUS_LCTRL	(1 << 8)
#define GRUB_TERM_STATUS_LALT	(1 << 9)

/* Menu-related geometrical constants. 菜单相关的几何常数 */

/* The number of columns/lines between messages/borders/etc. 列/消息之间的行/边界/等等的数 */
#define GRUB_TERM_MARGIN	1

/* The number of columns of scroll information. 滚动信息列数 */
#define GRUB_TERM_SCROLL_WIDTH	1

struct grub_term_input    //终端输入结构
{
  /* The next terminal. 下一终端 0 */
  struct grub_term_input *next;
  struct grub_term_input **prev;

  /* The terminal name. 终端名称 10 */
  const char *name;

  /* Initialize the terminal. 初始化终端 18 */
  grub_err_t (*init) (struct grub_term_input *term);

  /* Clean up the terminal. 清理终端 20 */
  grub_err_t (*fini) (struct grub_term_input *term);

  /* Get a character if any input character is available. Otherwise return -1. 如果任何输入字符可用，则获取字符。否则返回-1 28 */
  int (*getkey) (struct grub_term_input *term);

  /* Get keyboard modifier status. 获取键盘修改器状态 30 */
  int (*getkeystatus) (struct grub_term_input *term);

  void *data; //38
};
typedef struct grub_term_input *grub_term_input_t;  //终端输入结构

/* Made in a way to fit into uint32_t and so be passed in a register. 一种适合uint32_t的方法并且所以在一个寄存器中通过 */
struct grub_term_coordinate //终端坐标
{
  grub_uint16_t x;
  grub_uint16_t y;
};

/*
表106		支持的Unicode控制字符 
助记符		Unicode		说明
Null			U+0000		接收时忽略空字符
BS				U+0008		退格键。将光标向左移动一列。如果光标位于左边距，则不执行任何操作。
TAB				U+0009		制表键.
LF				U+000A		换行。将光标移到下一行。 
CR				U+000D		回车。将光标移到当前行的左边距。

表107		EFI简单文本输入协议的EFI扫描代码  scan_code
EFI扫描代码		描述
0x00					空扫描代码
0x01					将光标向上移动1行
0x02					将光标向下移动1行
0x03					将光标向右移动1列
0x04					将光标向左移动1列。
0x05					Home.				首行
0x06					End.				尾行
0x07					Insert.			插入
0x08					Delete.			删除
0x09					Page Up.		上一页
0x0a					Page Down.	下一页
0x0b					Function 1.	功能1
0x0c					Function 2.
0x0d					Function 3.
0x0e					Function 4.
0x0f					Function 5.
0x10					Function 6.
0x11					Function 7.
0x12					Function 8.
0x13					Function 9.
0x14					Function 10.
0x17					Escape.			逃离
表108		EFI简单文本输入扩展协议的EFI扫描代码
0x15					Function 11
0x16					Function 12
0x68					Function 13
0x69					Function 14
0x6A					Function 15
0x6B					Function 16
0x6C					Function 17
0x6D					Function 18
0x6E					Function 19
0x6F					Function 20
0x70					Function 21
0x71					Function 22
0x72					Function 23
0x73					Function 24
0x7F					Mute						静音
0x80					Volume Up				音量增加
0x81					Volume Down			音量减少
0x100					Brightness Up		亮度提高
0x101					Brightness Down	亮度降低
0x102					Suspend					暂停
0x103					Hibernate				休眠
0x104					Toggle Display	切换显示
0x105					Recovery				恢复 
0x106					Eject						弹出
0x8000-0xFFFF	OEM Reserved		OEM保留
*/
#endif /* ! GRUB_TERM_HEADER */
