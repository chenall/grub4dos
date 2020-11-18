/*
 *  PXE file system for GRUB
 *
 *  Copyright (C) 2007 Bean (bean123@126.com)
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
#ifdef FSYS_PXE

#include "shared.h"
#include "filesys.h"
#include "pxe.h"
#include "ipxe.h"

#if !defined(__constant_htonl)
#define __constant_htonl(x) \
        ((unsigned long long)((((unsigned long long)(x) & 0x000000ffU) << 24) | \
                             (((unsigned long long)(x) & 0x0000ff00U) <<  8) | \
                             (((unsigned long long)(x) & 0x00ff0000U) >>  8) | \
                             (((unsigned long long)(x) & 0xff000000U) >> 24)))
#endif
#if !defined(__constant_htons)
#define __constant_htons(x) \
        ((unsigned int)((((unsigned int)(x) & 0x00ff) << 8) | \
                              (((unsigned int)(x) & 0xff00) >> 8)))
#endif

#define ntohl(x) \
(__builtin_constant_p(x) ? \
 __constant_htonl((x)) : \
 __swap32(x))
#define htonl(x) \
(__builtin_constant_p(x) ? \
 __constant_htonl((x)) : \
 __swap32(x))
#define ntohs(x) \
(__builtin_constant_p(x) ? \
 __constant_htons((x)) : \
 __swap16(x))
#define htons(x) \
(__builtin_constant_p(x) ? \
 __constant_htons((x)) : \
 __swap16(x))

static inline unsigned long long __swap32(unsigned long long x)
{
	__asm__("xchgb %b0,%h0\n\t"
		"rorl $16,%0\n\t"
		"xchgb %b0,%h0"
		: "=q" (x)
		: "0" (x));
	return x;
}

static inline unsigned int __swap16(unsigned int x)
{
	__asm__("xchgb %b0,%h0"
		: "=q" (x)
		: "0" (x));
	return x;
}


#ifndef TFTP_PORT
#define TFTP_PORT	69
#endif

#define PXE_MIN_BLKSIZE	128
#define PXE_MAX_BLKSIZE	16384

#define DOT_SIZE	1048576


/* use disk buffer for PXE_BUF */
#define PXE_BUF		BUFFERADDR
#define PXE_BUFLEN	BUFFERLEN

unsigned int pxe_blksize = 512; /*PXE_MAX_BLKSIZE*/
struct grub_efi_pxe *pxe_entry = 0;
unsigned short pxe_basemem, pxe_freemem;
unsigned int pxe_keep;

IP4 pxe_yip, pxe_sip, pxe_gip;
grub_u8_t pxe_mac_len, pxe_mac_type;
MAC_ADDR pxe_mac;
static grub_u8_t pxe_tftp_opened;
static unsigned int pxe_saved_pos, pxe_cur_ofs, pxe_read_ofs;	//保存的指针,当前偏移,读偏移

extern PXENV_TFTP_OPEN_t pxe_tftp_open;	/* now it is defined in asm.S 现在它在asm.S中定义*/			//TFTP打开
static char filename[128];
static char *pxe_tftp_name = filename;
char *efi_pxe_buf = 0;

//extern unsigned int ROM_int15;
//extern unsigned int ROM_int13;
//extern unsigned int ROM_int13_dup;
extern struct drive_map_slot bios_drive_map[DRIVE_MAP_SIZE + 1];

static int pxe_open (char* name);
grub_u32_t pxe_read_blk (grub_u32_t buf, grub_u32_t num);

static int tftp_open(const char *dirname);
static grub_u32_t tftp_get_size(void);
static grub_u32_t tftp_read_blk (grub_u32_t buf, grub_u32_t num);
static void tftp_close (void);
static void tftp_unload(void);

s_PXE_FILE_FUNC tftp_file_func = {tftp_open,tftp_get_size,tftp_read_blk,tftp_close,tftp_unload};

grub_u32_t cur_pxe_type = 0;
grub_u32_t def_pxe_type = 0;
s_PXE_FILE_FUNC *pxe_file_func[2]={
	&tftp_file_func,
  #if 0
	#ifdef FSYS_IPXE
	&ipxe_file_func,
	#endif
  #endif
};

static char* pxe_outhex (char* pc, unsigned char c);
static char* pxe_outhex (char* pc, unsigned char c)		//pxe十六进制
{
  int i;

  pc += 2;
  for (i = 1; i <= 2; i++)
    {
      unsigned char t;

      t = c & 0xF;
      if (t >= 10)
        t += 'A' - 10;
      else
        t += '0';
      *(pc - i) = t;
      c = c >> 4;
    }
  return pc;
}

grub_u32_t pxe_read_blk (grub_u32_t buf, grub_u32_t num);
grub_u32_t pxe_read_blk (grub_u32_t buf, grub_u32_t num)	//pxe读大块
{
	grub_u32_t ret = pxe_file_func[cur_pxe_type]->readblk(buf,num);
	return ret;
}


/*
   return 0 for seccess, 1 for failure   返回0表示成功，返回1表示失败
*/
static int try_blksize (int tmp);
static int try_blksize (int tmp)	//尝试块尺寸
{
	unsigned int nr;
#if 0
#ifdef FSYS_IPXE
	if (cur_pxe_type == PXE_FILE_TYPE_IPXE) return 0;		//1
#endif
#endif
	pxe_blksize = tmp;
	printf_debug0 ("\nTry block size %d ...\n", pxe_blksize);	//尝试块尺寸
	nr = 0;
	tmp = pxe_open(pxe_tftp_name);

	if (!tmp)
	{
		printf_debug0 ("\nFailure: bootfile not found.\n");	//失败：找不到启动文件
		return 1;
	}

	if (filemax <= pxe_blksize)
	{
		printf_debug0 ("\nFailure: Size %ld too small.\n", filemax);	//失败：filemax尺寸太小
		pxe_close ();
		return 1;
	}

	nr = pxe_read_blk ((grub_u32_t)(grub_size_t)(char*)PXE_BUF, 1);	//pxe读大块

	if (nr == PXE_ERR_LEN)
	{
		printf_debug0 ("\nFailure: Cannot read the first block.\n");	//失败：无法读取第一个块
		pxe_close ();
		return 1;
	}

	if (pxe_blksize != nr && filemax >= nr && nr <= PXE_MAX_BLKSIZE && nr >= PXE_MIN_BLKSIZE)
	{
		printf_debug0 ("\npxe_blksize tuned from %d to %d\n", pxe_blksize, nr);	//pxe_blksize从％d调整为%d
		pxe_blksize = nr;
	}

	printf_debug0 ("\nUse block size %d\n", pxe_blksize);	//使用块尺寸pxe_blksize

	pxe_close ();
	return 0;
}

static unsigned int config_already_restarted = 0;
static unsigned int server_is_dos = 0;
BOOTPLAYER *discover_reply = 0;		//引导播放器

static void set_basedir(char *config);
static void set_basedir(char *config)	//设置基本目录 
{
	unsigned int n;
	grub_u8_t path_sep = (cur_pxe_type == PXE_FILE_TYPE_TFTP && server_is_dos) ? '\\' : '/';	//路径分隔
	n = grub_strlen (config);	//字符串尺寸

	if (n > 126)
	{
		printf_warning("Warning! base name(%d chars) too long (> 126 chars).\n", n);	//警告！ 基本名称（％d个字符）太长
		n = 126;
		config[n] = 0;
	}

	if (*config != path_sep)
	{
    #if 0
		#ifdef FSYS_IPXE
		char *ch = strstr(config,":");
		if (has_ipxe && ch && (grub_u32_t)(ch - config) < 10)
		{
			pxe_tftp_name = (char*)&pxe_tftp_open.FileName;
			cur_pxe_type = def_pxe_type = PXE_FILE_TYPE_IPXE;		//1
		}
		else
		{
			if (cur_pxe_type == PXE_FILE_TYPE_TFTP)
		#endif
    #endif
				pxe_tftp_name = (char*)&pxe_tftp_open.FileName;	//tftp名称地址
			*pxe_tftp_name++ = path_sep;	//设置tftp名称根目录
    #if 0
		#ifdef FSYS_IPXE
		}
		#endif
    #endif
	}

	grub_memmove(pxe_tftp_name, config, n);	//复制名称

	while (n >= 0) if (pxe_tftp_name[--n] == path_sep) break;	//查找路径分隔符

	pxe_tftp_name += n;	//指向分隔符后
}

static void print_ip (IP4 ip);

int pxe_detect (int blksize, char *config);
int pxe_detect (int blksize, char *config)	//pxe探测(块尺寸, 配置)
{
  unsigned int tmp;
  char *pc;
  int i, ret;

  if (! pxe_entry)
    return 0;

  if (discover_reply->sip)	//如果存在引导播放器->服务器IP
		pxe_sip = discover_reply->sip;
  if (discover_reply->gip)	//如果存在引导播放器->网关IP
		pxe_gip = discover_reply->gip;
  else	//否则
  {//get route gateway	获取路由网关 
		grub_u8_t *p = discover_reply->vendor.d;	//引导播放器->供应商.引导DHCPVEND
		if (*(int*)p == 0x63538263)//DHCP magic cookie 99.130.83.99
		{
			for(i=4;i<BOOTP_DHCPVEND;i += p[i] + 1)
			{
				grub_u8_t code = p[i++];
				if (!code || code == '\xff')
					break;
				if (code == '\x3')//Router Option	路由器选项
				{
					pxe_gip = *(IP4*)(p + i + 1);
					break;
				}
			}
		}
  }
#if 0
#ifdef FSYS_IPXE
  if (blksize == IPXE_PART)	//0x45585069
  {
		if (*config == '(')
		{
			config = grub_strstr(config,"/");
			if (!config) return 0;
				++config;
		}
		set_basedir(config);
		return 1;
  }
#endif
#endif
  if (!pxe_sip && cur_pxe_type == PXE_FILE_TYPE_TFTP) return 0;//pxe server not found?	找不到pxe服务器？

  if (discover_reply->bootfile[0])	//如果存在引导播放器->引导文件
	{
		unsigned int n;

		for (n = 0; n < 127; n++)
		{
			if (discover_reply->bootfile[n] == '\\')	//引导播放器->引导文件
			{
				server_is_dos = 1;
				break;
			}
		}

		grub_printf("\nBoot Server: ");	//启动服务器		
		print_ip(pxe_sip);							//打印ip
		grub_printf("\tBoot File: %s\n", discover_reply->bootfile);	//引导播放器->引导文件：

		set_basedir((char*)discover_reply->bootfile);	//设置基本目录(引导播放器->引导文件)

		/* read the boot file to determine the block size. 读取启动文件以确定块大小*/

		if (blksize)	//如果存在块尺寸
			pxe_blksize = blksize;
		else if (try_blksize (1408) && try_blksize (512))	//如果块尺寸尝试1408及512失败
		{
			pxe_blksize = 512;	/* default to 512 默认为512*/
			printf_warning ("Warning! Cannot open bootfile. pxe_blksize set to default 512.\n");	//警告！ 无法打开启动文件。 pxe_blksize设置为默认512
		}
	}
  else	//如果不存在引导播放器->引导文件
	{
		pxe_blksize = (blksize ? blksize : 512);	/* default to 512 */
		printf_warning ("\nNo bootfile! pxe_blksize set to %d\n", pxe_blksize);	//没有启动文件！ pxe_blksize设置为
		pxe_tftp_name = (char*)&pxe_tftp_open.FileName[0];	//设置tftp名称地址
	}

  pxe_tftp_opened = 0;	//没有打开

  ret = 0;

  grub_memcpy ((char *) saved_pxe_mac, (char *) pxe_mac, 6);	//备份pxe_mac		没有使用!!
  saved_pxe_ip = pxe_yip;	//备份客户IP		没有使用!!

  if (config)	//如果存在配置
  {
		if ((ret = grub_open(config)))	//打开配置
		{
			set_basedir(config);	//设置基本目录
			grub_close();					//关闭
			goto done;						//完成
		}
		return 1;	//退出
	}
	//没有配置
	grub_strcpy (pxe_tftp_name, "/menu.lst");	//设置名称
	ret = pxe_dir (pxe_tftp_name);	//目录
	if (ret && filemax)	//如果成功,并且获得文件尺寸
		goto done;	//完成
	if (pxe_tftp_open.Status != PXENV_STATUS_TFTP_FILE_NOT_FOUND)//坏服务器		找不到PXENV状态TFTP文件
		goto done;	//完成
  /* Reports from Ruymbeke: opening /menu.lst will hang if it is a dir.		来自Ruymbeke的报告：如果/menu.lst是目录，则打开/menu.lst将挂起。
   * Do NOT use /menu.lst as a dir any more!! Use /menu for it instead.		不要再将/menu.lst用作目录！ 使用/menu代替。
   */
	if (!config)	//如果配置为零
		config = (char *)"/menu.lst/";

  grub_strcpy (pxe_tftp_name, config);	//设置名称

	int MENU_DIR_NAME_LENGTH = grub_strlen(config);	//菜单目录名称长度

  pc = pxe_tftp_name + MENU_DIR_NAME_LENGTH;
  pc = pxe_outhex (pc, pxe_mac_type);		//pxe十六进制
  for (i = 0; i < pxe_mac_len; i++)
	{
		*(pc++) = '-';
		pc = pxe_outhex (pc, pxe_mac[i]);		//pxe十六进制
	}
  *pc = 0;
  grub_printf ("\n%s\n", pxe_tftp_open.FileName);
  if (pxe_dir (pxe_tftp_name))	//如果目录完成
	{
		ret = 1;
		goto done;	//完成
	}

  pc = pxe_tftp_name + MENU_DIR_NAME_LENGTH;
  tmp = pxe_yip;
  for (i = 0; i < 4; i++)
	{
		pc = pxe_outhex (pc, tmp & 0xFF);		//pxe十六进制
		tmp >>= 8;
	}
  *pc = 0;
  do
	{
		grub_printf ("%s\n", pxe_tftp_open.FileName);
		if (pxe_dir (pxe_tftp_name))
		{
			ret = 1;
			goto done;	//完成
		}
		if (checkkey() == 0x11b) break;
		*(--pc) = 0;
	} while (pc > pxe_tftp_name + MENU_DIR_NAME_LENGTH);
  grub_strcpy (pc, "default");
  grub_printf ("%s\n", pxe_tftp_open.FileName);
  ret = pxe_dir (pxe_tftp_name);

#undef MENU_DIR_NAME_LENGTH

done:	//完成

  if (ret && filemax)
	{
		char *new_config = config_file;
		char *filename1 = pxe_tftp_name;
		if (debug > 1)
		{
			printf_debug("\rPXE boot configfile:%s\n",(char *)pxe_tftp_open.FileName);	//PXE引导配置文件：
//			DEBUG_SLEEP
		}
		pxe_close ();
		/* got file name. put it in config_file 有文件名。 把它放在config_file*/
		if (grub_strlen (filename1) >= ((char *)IMG(0x8270) - new_config))
			return ! (errnum = ERR_WONT_FIT);
		/* set (pd) as root device. 设置（PD）为根设备*/
		saved_drive = PXE_DRIVE;	//0x21
		saved_partition = current_partition;
		/* Copy FILENAME to CONFIG_FILE.  将FILENAME复制到CONFIG_FILE*/
		while ((*new_config++ = *filename1++) != 0);
		if (pxe_restart_config == 0)	//如果pxe重新启动配置=0
		{
			if (config_already_restarted == 0)	//如果配置已重新启动=0
			{
				pxe_restart_config = 1;						//重新启动配置=1
				config_already_restarted = 1;			//配置已重新启动=1
			}
			return ret;	//退出
		}
		use_config_file = 1;	//使用配置文件

		/* Make sure that the user will not be authoritative.  确保用户不具有权威性。*/
		auth = 0;
  
		buf_drive = -1;	/* invalidate disk cache. 使磁盘缓存无效*/
		buf_track = -1;	/* invalidate disk cache. */
		saved_entryno = 0;	//保存的条目号
		boot_drive = saved_drive;
		install_partition = saved_partition;
		current_drive = GRUB_INVALID_DRIVE;	//0xFFFFFFFF
		current_partition = 0xFFFFFF;
		fsys_type = NUM_FSYS;
		boot_part_addr = 0;
		current_slice = 0;

	/* Restart cmain.  重新启动cmain*/
		asm volatile ("movl $0x7000, %esp");	/* set stack to STACKOFF */
#ifdef HAVE_ASM_USCORE
		asm volatile ("call _cmain");
		grub_halt();
#else
		asm volatile ("call cmain");
		grub_halt();
#endif

	/* Never reach here.  永不到达这里*/
	}
  return ret;
}

static int pxe_reopen (void);
static int pxe_reopen (void)	//pxe重新打开
{
  pxe_close ();
  pxe_tftp_opened = pxe_file_func[cur_pxe_type]->open(pxe_tftp_name);
  return pxe_tftp_opened;
}

static int pxe_open (char* name);
static int pxe_open (char* name)	//pxe打开
{
	if (name != pxe_tftp_name)
	{
		grub_strcpy (pxe_tftp_name, name);

		if (cur_pxe_type == PXE_FILE_TYPE_TFTP && server_is_dos)	//服务器是DOS
		{
			unsigned int n;
			for (n = 0; n < 128; n++)
			{
				if (pxe_tftp_open.FileName[n] == '/')
					pxe_tftp_open.FileName[n] = '\\';
			}
		}
		name = pxe_tftp_name;
	}
	pxe_close ();
	/*
	We always use pxe_tftp_open.FileName for full file path.	我们始终使用pxe_tftp_open.FileName作为完整文件路径。名称是相对路径。
	name is a relative path.
	*/
	pxe_tftp_opened = pxe_file_func[cur_pxe_type]->open(name);
	if (!pxe_tftp_opened)
	{
		if ((unsigned int)debug >= 0x7FFFFFFF) printf("Err: %d\n",pxe_tftp_open.Status);
		return 0;
	}
	if (pxe_file_func[cur_pxe_type]->getsize())
		return 1;
	pxe_close ();
	return (pxe_tftp_opened = 0);
}

void pxe_close (void);
void pxe_close (void)	//pxe关闭		grub_pxe_close (struct grub_net_card *dev __attribute__ ((unused)))
{
	if (pxe_tftp_opened)
	{
		pxe_file_func[cur_pxe_type]->close();		//调用关闭
		pxe_saved_pos = pxe_cur_ofs = pxe_read_ofs = 0;
		pxe_tftp_opened = 0;
	}
}

static unsigned int pxe_read_len (unsigned long long buf, unsigned long long len);
static unsigned int pxe_read_len (unsigned long long buf, unsigned long long len)	//读长度(目的缓存, 尺寸字节)
{
  unsigned int old_ofs, sz;

  if (len == 0)
    return 0;

  sz = 0;
  old_ofs = pxe_cur_ofs;	//当前偏移, 也就是起始字节
  pxe_cur_ofs += len;			//结束字节
  if (pxe_cur_ofs > pxe_read_ofs)	//如果结束字节>读偏移
    {
      unsigned int nb, nr;
      int nb_del, nb_pos;
// 原点      起始            读偏移           结束
//   |---------|------sz-------|---------------|
//PXE_BUF   old_ofs       pxe_read_ofs    pxe_cur_ofs

      sz = (pxe_read_ofs - old_ofs);	//读偏移-当前偏移=读尺寸
      if ((buf) && (sz))	//如果缓存存在, 并且读尺寸不为零
        {
          grub_memmove64 (buf, (unsigned long long)(grub_size_t)(char*)(PXE_BUF + old_ofs), sz);	//(PXE_BUF + old_ofs) -> buf
          buf += sz;
        }
      pxe_cur_ofs -= pxe_read_ofs;	/* bytes to read 要读取的字节*/	//当前偏移-读偏移=要读取的字节
      nb = pxe_cur_ofs / pxe_blksize;	/* blocks to read 读取块*/		//要读取的字节/块尺寸=要读取的块
      nb_del = DOT_SIZE / pxe_blksize;															//1000000/块尺寸=dot块
      if ((unsigned int)nb_del > nb)	//如果dot块>要读取的块
        {
          nb_del = 0;		//dot块=0
          nb_pos = -1;	//
        }
      else							//如果dot块<=要读取的块
        nb_pos = nb - nb_del;	//要读取的块-dot块
      pxe_cur_ofs -= pxe_blksize * nb;	/* bytes residual 剩余字节数*/	//要读取的字节-块尺寸*要读取的块=剩余字节数
      if (pxe_read_ofs + pxe_blksize > PXE_BUFLEN)	//如果读偏移+块尺寸 > 10000
        pxe_read_ofs = 0;														//则读偏移=0
      while (nb > 0)	//存在要读取的块
        {
          unsigned int nn;

          nn = (PXE_BUFLEN - pxe_read_ofs) / pxe_blksize;	//(缓存尺寸-读偏移)/块尺寸=实际读取的块
          if (nn > nb)	//如果实际读取的块>要读取的块
            nn = nb;		//则实际读取的块=要读取的块
          nr = pxe_read_blk ((grub_u32_t)(grub_size_t)(char*)PXE_BUF + pxe_read_ofs, nn);	//pxe读大块
					
          if (nr == PXE_ERR_LEN)	//如果实际读字节=0
            return nr;	//返回0
          sz += nr;			//读尺寸+实际读字节  应当等于len
          if (buf)	//如果存在缓存
            {
              grub_memmove64 (buf, (unsigned long long)(grub_size_t)(char*)(PXE_BUF + pxe_read_ofs), nr); //(PXE_BUF + old_ofs) -> buf
              buf += nr;	//调整缓存位置
            }
          if (nr < nn * pxe_blksize)	//如果实际读字节<实际读取的块*块尺寸, 完成了
            {
              pxe_read_ofs += nr;					//读偏移+实际读字节=下一读偏移
              pxe_cur_ofs = pxe_read_ofs;	//当前偏移=读偏移
              return sz;	//返回从起始读的字节
            }
					//未完成
          nb -= nn;	//要读取的块-实际读取的块
          if (nb)	//如果有要读取的块
            pxe_read_ofs = 0;	//读偏移=0
          else	//如果没有要读取的块
            pxe_read_ofs += nr;	//读偏移+实际读字节
          if ((int)nb <= nb_pos)	//如果要读取的块<nb_pos
            {
              grub_putchar ('.', 255);
              nb_pos -= nb_del;
            }
        }

      if (nb_del)
        {
          grub_putchar ('\r', 255);
          grub_putchar ('\n', 255);
        }

      if (pxe_cur_ofs)	//如果有剩余字节数
        {
          if (pxe_read_ofs + pxe_blksize > PXE_BUFLEN)	//如果读偏移+块尺寸 > pxe缓存尺寸
            pxe_read_ofs = 0;	//读偏移=0

          nr = pxe_read_blk ((grub_u32_t)(grub_size_t)(char*)PXE_BUF + pxe_read_ofs, 1);	//pxe读大块
          if (nr == PXE_ERR_LEN)	//如果已读字节=0
            return nr;						//返回0
          if (pxe_cur_ofs > nr)		//如果剩余字节数 > 已读字节
            pxe_cur_ofs = nr;			//剩余字节数 = 已读字节
          sz += pxe_cur_ofs;			//读尺寸+剩余字节数
          if (buf)	//如果存在缓存
            grub_memmove64 (buf, (unsigned long long)(grub_size_t)(char*)(PXE_BUF + pxe_read_ofs), pxe_cur_ofs);
          pxe_cur_ofs += pxe_read_ofs;	//当前偏移=剩余字节数+读偏移
          pxe_read_ofs += nr;						//读偏移=读偏移+已读字节
        }
      else	//如果没有剩余字节数
        pxe_cur_ofs = pxe_read_ofs;	//当前偏移=读偏移
    }
  else	//如果结束字节<=读偏移
    {
      sz += len;	//读尺寸+len
      if (buf)	//如果存在缓存
        grub_memmove64 (buf, (unsigned long long)(grub_size_t)(char *)PXE_BUF + old_ofs, len);
    }
  return sz;
}

/* Mount the network drive. If the drive is ready, return 1, otherwise		安装网络驱动器。如果驱动器准备好了，返回1，否则返回0。
   return 0. */
int pxe_mount (void);
int pxe_mount (void)	//pxe挂载
{
  if (current_drive != PXE_DRIVE || ! pxe_entry)	//0x21
    return 0;
#if 0
#ifdef FSYS_IPXE
  if (current_partition != IPXE_PART)	//0x45585069
    cur_pxe_type = def_pxe_type;
  else if (has_ipxe)
    cur_pxe_type = PXE_FILE_TYPE_IPXE;	//1
  else
    return 0;
#endif
#endif
  return 1;
}


//char old_name[128];
/* Read up to SIZE bytes, returned in ADDR.  读取最多SIZE个字节，返回ADDR*/
unsigned long long pxe_read (unsigned long long buf, unsigned long long len, unsigned int write);
unsigned long long
pxe_read (unsigned long long buf, unsigned long long len, unsigned int write)	//pxe读
{
  unsigned int nr;

  if (write == GRUB_WRITE)	//如果写, 则错误
    return !(errnum = ERR_WRITE);

  if (! pxe_tftp_opened)	//如果pxe没有打开, 则错误
    return PXE_ERR_LEN;

	if (cur_pxe_type == PXE_FILE_TYPE_TFTP) //TFTP读, 从此完成
	{
    if (!buf || write == GRUB_LISTBLK)
      return 0;

		grub_memmove64 (buf, (unsigned long long)(grub_size_t)(char*)(efi_pxe_buf + filepos), len);
		filepos += len;
		return len;
	}

  if (pxe_saved_pos != filepos)	//如果保存的指针不等于文件指针
    {
//PXE_BUF   filepos                           pxe_saved_pos					filepos+pxe_cur_ofs
// 原点     文件指针                            保存的指针						文件指针+当前偏移
//   |---------|------------------------------------|------------------------|
//						 |---------------------------旧当前偏移------------------------|
//                                                  |------新当前偏移--------|
      if ((filepos < pxe_saved_pos) && (filepos+pxe_cur_ofs >= pxe_saved_pos))	//如果文件指针<保存的指针 , 并且(文件指针+当前偏移)>=保存的指针
        pxe_cur_ofs -= pxe_saved_pos - filepos;	//当前偏移=当前偏移-(保存的指针-文件指针)  文件指针及保存的指针没有改变
      else	//如果文件指针>=保存的指针 , 或者(文件指针+当前偏移)<保存的指针
        {	
//PXE_BUF   filepos             filepos+pxe_cur_ofs                    pxe_saved_pos
// 原点     文件指针             文件指针+当前偏移                       保存的指针
//   |---------|------------------------|------------------------------------|	
//						 |--------当前偏移--------|			
          if (pxe_saved_pos > filepos)	//如果文件指针<保存的指针
            {
              if (! pxe_reopen ())  //pxe重新打开, 如果失败
                return PXE_ERR_LEN; //返回0
            }
//PXE_BUF  pxe_saved_pos                                                   filepos
// 原点     保存的指针                                                     文件指针
//   |---------|------------------------|-------------------------------------|	
//						 |------------------(filepos-pxe_saved_pos)=nr------------------|			
          nr = pxe_read_len (0ULL, filepos - pxe_saved_pos);  //不是实际读, 只返回尺寸
          if ((nr == PXE_ERR_LEN) || (pxe_saved_pos + nr != filepos)) //如果返回0, 或者(pxe_saved_pos + nr != filepos), 错误
            return PXE_ERR_LEN;
        }
      pxe_saved_pos = filepos;  //更新保存的指针
    }
		
  nr = pxe_read_len (buf, len); //实际读
  if (nr != PXE_ERR_LEN)
    {
      filepos += nr;
      pxe_saved_pos = filepos;
    }
  return nr;
}

/* Check if the file DIRNAME really exists. Get the size and save it in		检查文件DIRNAME是否确实存在
   FILEMAX. return 1 if succeed, 0 if fail.  */		//获取尺寸并将其保存在FILEMAX中。 如果成功则返回1，如果失败则返回0
struct pxe_dir_info	//目录信息
{
	char path[512];			//路径
	char *dir[512];			//目录
	char data[];				//数据
} *P_DIR_INFO = NULL;

int pxe_dir (char *dirname);
int pxe_dir (char *dirname)	//pxe查目录
{
  int ret;
  char ch;
  ret = 1;
  ch = nul_terminate (dirname);		//用"0"替换"\0"

	if (print_possibilities)	//如果存在打印可能性
	{
		char dir_tmp[128];
		char *p_dir;
		ret = grub_strlen(dirname);	//目录尺寸
		p_dir = &dirname[ret];			//目录结束地址

		if (ret && ret <=120)				//存在目录尺寸,并且<=120
		{
			while (ret && dirname[ret] != '/') 	//取子目录
			{
				ret--;
			}
			grub_memmove(dir_tmp,dirname,ret);	//复制子目录
		}
		else
			ret = 0;

		grub_strcpy(&dir_tmp[ret],"/dir.txt");//追加"/dir.txt"
		if (P_DIR_INFO || (P_DIR_INFO = (struct pxe_dir_info*)grub_malloc(16384)))	//建立目录信息缓存
		{
			int i;
			char *p = P_DIR_INFO->data;
			if (substring(dir_tmp,P_DIR_INFO->path,1) != 0)	//判断子字符串
			{
				memset(P_DIR_INFO,0,16384);
				grub_strcpy(P_DIR_INFO->path,dir_tmp);
				if (pxe_open(dir_tmp))
				{
					if (pxe_read((unsigned long long)(grub_size_t)P_DIR_INFO->data,13312,GRUB_READ))
					{
						P_DIR_INFO->dir[0] = P_DIR_INFO->data;
						for (i = 1;i < 512 && (p = skip_to(0x100,p));++i)
						{
							P_DIR_INFO->dir[i] = p;
						}
					}
					pxe_close();
				}
			}
			dirname += ret + 1;
			ret = 0;
			for (i = 0; i < 512 && (p = P_DIR_INFO->dir[i]);++i)
			{
				if (*dirname == 0 || substring (dirname, p, 1) < 1)
				{
					ret = 1;
					print_a_completion(p, 1);
				}
			}
		}
		else
			ret = 0;
		if (!ret)
			errnum = ERR_FILE_NOT_FOUND;
		*p_dir = ch;
		return ret;
  }
  pxe_close ();
  if (! pxe_open (dirname))
    {
      errnum = ERR_FILE_NOT_FOUND;
      ret = 0;
    }

  dirname[grub_strlen(dirname)] = ch;
  return ret;
}

void pxe_unload (void);
void pxe_unload (void)	//pxe卸载
{
#if 0
#ifdef FSYS_IPXE
	if (has_ipxe) pxe_file_func[PXE_FILE_TYPE_IPXE]->unload();
#endif
#endif
	pxe_file_func[PXE_FILE_TYPE_TFTP]->unload();
}

static int tftp_open(const char *name);
static int tftp_open(const char *name)		//tftp打开
{
	grub_efi_status_t status;
  grub_efi_boot_services_t *b;  //引导服务
  b = grub_efi_system_table->boot_services; //系统表->引导服务

  if (!tftp_get_size())
		return 0;

  tftp_close ();
  status = efi_call_3 (b->allocate_pool, GRUB_EFI_BOOT_SERVICES_DATA, //启动服务数据        4
                           filemax + 0x200, (void**)&efi_pxe_buf); //(分配池,存储器类型->装载数据,分配字节,返回分配地址}
  if (status != GRUB_EFI_SUCCESS)		//失败
	{
		printf_errinfo ("Couldn't allocate pool.");
    return 0;
	}

	status = efi_call_10 (pxe_entry->mtftp,				//tftp功能
				pxe_entry,															//pxe结构
				GRUB_EFI_PXE_BASE_CODE_TFTP_READ_FILE,	//TFTP读文件
				efi_pxe_buf,														//缓存
				0,
				(grub_efi_uint64_t *)(grub_size_t)&filemax,//缓存尺寸
				NULL,																		//块尺寸
				(IP4 *)(grub_size_t)&pxe_sip,					  //服务器IP
				(char *)name,													  //文件名
				NULL,
				0);

  if (status != GRUB_EFI_SUCCESS)		//失败
	{
		printf_errinfo ("Couldn't open file.");
    return 0;
	}

  pxe_tftp_opened = 1;
	filepos = 0;
  return 1;
}

static grub_u32_t tftp_get_size(void);
static grub_u32_t tftp_get_size(void)			//TFTP获得文件尺寸
{
	grub_efi_status_t status;

	status = efi_call_10 (pxe_entry->mtftp,					//tftp功能
	    pxe_entry,																	//pxe结构
	    GRUB_EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE,	//TFTP获得文件尺寸
	    NULL,																				//缓存
	    0,
			(grub_efi_uint64_t *)(grub_size_t)&filemax, //缓存尺寸
	    NULL,																				//块尺寸
	    (IP4 *)(grub_size_t)&pxe_sip,						    //服务器IP
	    pxe_tftp_name,															//文件名
	    NULL,
	    0);

  if (status != GRUB_EFI_SUCCESS)		//失败
	{
		printf_errinfo ("Couldn't get file size");
    return 0;
	}

	return filemax;
}

static grub_u32_t tftp_read_blk (grub_u32_t buf, grub_u32_t num);
static grub_u32_t tftp_read_blk (grub_u32_t buf, grub_u32_t num)
{
	return 0;
}

static void tftp_close (void);
static void tftp_close (void)		//tftp关闭
{
  grub_efi_boot_services_t *b;  //引导服务
  b = grub_efi_system_table->boot_services; //系统表->引导服务
  if (efi_pxe_buf)
    efi_call_1 (b->free_pool, efi_pxe_buf);	//调用(释放池,释放数据)
	pxe_tftp_opened = 0;
	efi_pxe_buf = 0;
}

static void tftp_unload(void);
static void tftp_unload(void)		//tftp卸载
{
  if (! pxe_entry)
    return;

  pxe_close ();

  if (pxe_keep)
    return;
}
#endif	//ifdef FSYS_PXE

static void print_ip (IP4 ip);
static void print_ip (IP4 ip)		//打印ip
{
  int i;

  for (i = 0; i < 3; i++)
    {
      grub_printf ("%d.", (grub_size_t)(unsigned char)ip);
      ip >>= 8;
    }
  grub_printf ("%d", (grub_size_t)(unsigned char)ip);
}

int pxe_func (char *arg, int flags);
int pxe_func (char *arg, int flags)	//pxe函数
{
  if (! pxe_entry)
    {
      goto bad_argument;
    }
  if (*arg == 0)  //无参数,打印信息
    {
      char buf[4], *pc;
      int i;

      pxe_tftp_name[0] = '/';
      pxe_tftp_name[1] = 0;
      grub_printf ("blksize: %d [%s]\n", pxe_blksize,def_pxe_type==1?"iPXE":"pxe");			//512[pxe]			1408[pxe]
      grub_printf ("basedir: %s\n",pxe_tftp_open.FileName);															//							/
      grub_printf ("bootfile: %s\n", discover_reply->bootfile);	//引导播放器->引导文件	//							menu.ipxe
      grub_printf ("client ip  : ");																										//2.1.6.0				192.168.56.6
      print_ip (pxe_yip);
      grub_printf ("\nserver ip  : ");																									//75.78.30.32		192.168.56.1
      print_ip (pxe_sip);
      grub_printf ("\ngateway ip : ");																									//0.0.128.0			0.0.0.0
      print_ip (pxe_gip);
      grub_printf ("\nmac : ");																													//							00-0c-29-8d-cc-d9
      for (i = 0; i < pxe_mac_len; i++)
        {
          pc = buf;
          pc = pxe_outhex (pc, pxe_mac[i]);
          *pc = 0;
          grub_printf ("%s%c", buf, ((i == pxe_mac_len - 1) ? '\n' : '-'));
        }
    }
  else if (grub_memcmp(arg, "blksize", sizeof("blksize") - 1) == 0)	//设置块尺寸
    {
      unsigned long long val;
      grub_u32_t force=arg[7] != '=';
      arg = skip_to (1, arg);
      if (! safe_parse_maxint (&arg, &val))
        return 0;
      if (val > PXE_MAX_BLKSIZE)		//16384		0x4000
        val = PXE_MAX_BLKSIZE;
      if (val < PXE_MIN_BLKSIZE)		//128			0x80
        val = PXE_MIN_BLKSIZE;
      pxe_blksize = val;
      if (!force) try_blksize(val);
    }
  else if (grub_memcmp (arg, "basedir", sizeof("basedir") - 1) == 0)	//基本目录 
    {
      arg = skip_to (0, arg);
      if (*arg == 0)
        {
          grub_printf ("No pathname\n");
	  goto bad_argument;
        }
	set_basedir(arg);	//设置基本目录
    }
  else if (grub_memcmp (arg, "keep", sizeof("keep") - 1) == 0)	    //保持
    pxe_keep = 1;
  else if (grub_memcmp (arg, "nokeep", sizeof("nokeep") - 1) == 0)	//不保留
    pxe_keep = 0;
  else if (grub_memcmp (arg, "unload", sizeof("unload") - 1) == 0)	//卸载 
    {
      pxe_keep = 0;
      pxe_unload ();
    }
  else if (grub_memcmp (arg, "detect", sizeof("detect") - 1) == 0)	//探测
    {
	unsigned long long blksize = 0;
	/* pxe_detect should be done before any other command. 探测应该在任何其他命令之前完成。*/
	arg = skip_to (0, arg);

		if (*arg >= '0' && *arg <= '9')
		{
			if (! safe_parse_maxint (&arg, &blksize))	//块尺寸
				return 0;
			arg = skip_to (0, arg);
			if (*arg == 0)
				arg = 0;
		} else if (*arg == 0)
			arg = 0;
	return pxe_detect ((int)blksize, arg);	//pxe探测
    }
    #if 0
    #ifdef FSYS_IPXE
    else if (has_ipxe && grub_memcmp (arg ,"type",sizeof("type") - 1) == 0)	//设置ipxe类型
    {
	arg = skip_to (0, arg);
	switch(*arg)
	{
		case '0':
		case '1':
			def_pxe_type = *arg - '0';
			break;
		default:
			goto bad_argument;
	}
    }
    #endif
    #endif
  else
    {
bad_argument:
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }
  return 1;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//efinet.c
/* GUID.  */
struct grub_net_card *grub_net_cards = NULL;
static grub_efi_guid_t net_io_guid = GRUB_EFI_SIMPLE_NETWORK_GUID;	//简单网络
static grub_efi_guid_t pxe_io_guid = GRUB_EFI_PXE_GUID;

static struct grub_net_card_driver efidriver =
  {
		#if 0
    .name = "efinet",
    .open = open_card,				//打开
    .close = close_card,			//关闭
    .send = send_card_buffer,	//发送
    .recv = get_card_packet		//接收
		#endif
  };


//============================================================================================================================
static void grub_efinet_findcards (void);
static void
grub_efinet_findcards (void)	//查找卡
{
  grub_efi_uintn_t num_handles;
  grub_efi_handle_t *handles;
  grub_efi_handle_t *handle;

  /* Find handles which support the disk io interface.  查找支持磁盘io接口的句柄。 */
  handles = grub_efi_locate_handle (GRUB_EFI_BY_PROTOCOL, &net_io_guid,
				    0, &num_handles);	//定位句柄
  if (! handles)	//失败
    return;

  for (handle = handles; num_handles--; handle++)
	{
		grub_efi_simple_network_t *net;	//简单网络
		struct grub_net_card *card;			//网卡
		grub_efi_device_path_t *dp, *parent = NULL, *child = NULL;	//设备路径

      /* EDK2 UEFI PXE driver creates IPv4 and IPv6 messaging devices as		EDK2 UEFI PXE驱动程序将IPv4和IPv6消息设备创建为主MAC消息设备的子设备。
	 children of main MAC messaging device. We only need one device with			我们只需要每个物理卡一个绑定SNP的设备，否则它们在轮询传入数据包时会相互竞争。
	 bound SNP per physical card, otherwise they compete with each other
	 when polling for incoming packets.
       */
		dp = grub_efi_get_device_path (*handle);	//设备路径
		if (!dp)	//失败
			continue;

		for (; ! GRUB_EFI_END_ENTIRE_DEVICE_PATH (dp); dp = GRUB_EFI_NEXT_DEVICE_PATH (dp))
		{
			parent = child;
			child = dp;
		}
		if (child
				&& GRUB_EFI_DEVICE_PATH_TYPE (child) == GRUB_EFI_MESSAGING_DEVICE_PATH_TYPE			//并且是通讯设备路径 3
				&& (GRUB_EFI_DEVICE_PATH_SUBTYPE (child) == GRUB_EFI_IPV4_DEVICE_PATH_SUBTYPE		//并且是IPV4设备子路径 12
	      || GRUB_EFI_DEVICE_PATH_SUBTYPE (child) == GRUB_EFI_IPV6_DEVICE_PATH_SUBTYPE)		//		或者是IPV6设备子路径	13
				&& parent
				&& GRUB_EFI_DEVICE_PATH_TYPE (parent) == GRUB_EFI_MESSAGING_DEVICE_PATH_TYPE		//并且是通讯设备路径 3
				&& GRUB_EFI_DEVICE_PATH_SUBTYPE (parent) == GRUB_EFI_MAC_ADDRESS_DEVICE_PATH_SUBTYPE)	//并且是MAC地址设备子路径 11
			continue;

		net = grub_efi_open_protocol (*handle, &net_io_guid,
				GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);	//打开协议.通过句柄
		if (! net)	//失败
			/* This should not happen... Why?  这不应该发生...为什么？ */
			continue;

		if (net->mode->state == GRUB_EFI_NETWORK_STOPPED					//如果网络停止 0
				&& efi_call_1 (net->start, net) != GRUB_EFI_SUCCESS)	//并且启动网络失败
			continue;

		if (net->mode->state == GRUB_EFI_NETWORK_STOPPED)					//如果网络停止 0
			continue;

		if (net->mode->state == GRUB_EFI_NETWORK_STARTED					//如果网络起动 1
				&& efi_call_3 (net->initialize, net, 0, 0) != GRUB_EFI_SUCCESS)	//并且网络初始化失败
			continue;

		card = grub_zalloc (sizeof (struct grub_net_card));	//分配卡缓存
		if (!card)	//失败
		{
			grub_free (handles);
			return;
		}
		card->mtu = net->mode->max_packet_size;							//最大包尺寸	5dc
		card->txbufsize = ALIGN_UP (card->mtu, 64) + 256;		//tx缓存尺寸	700
		card->txbuf = grub_zalloc (card->txbufsize);				//分配tx缓存	101c14c0
		if (!card->txbuf)	//失败
		{
			grub_free (handles);
			grub_free (card);
			return;
		}
		card->txbusy = 0;		//忙碌	0

		card->rcvbufsize = ALIGN_UP (card->mtu, 64) + 256;	//rcv缓存尺寸		700
		card->driver = &efidriver;			//驱动器	10e66094
		card->flags = 0;								//标记		0
		card->default_address.type = GRUB_NET_LINK_LEVEL_PROTOCOL_ETHERNET;	//默认地址类型=以太网		0
		grub_memcpy (card->default_address.mac,		//mac				101c1be4
		net->mode->current_address,								//当前地址	111b3280
		sizeof (card->default_address.mac));			//尺寸			6
		card->efi_net = net;				//简单网络
		card->efi_handle = *handle;	//句柄				111bb290
		grub_net_cards = card;
		grub_free (card);	//释放
	}
  grub_free (handles);	//释放
}

static void grub_efi_net_config_real (grub_efi_handle_t hnd, char **device, char **path);
static void
grub_efi_net_config_real (grub_efi_handle_t hnd, char **device,
			  char **path)	//实际网络配置 
{
  struct grub_net_card *card;	//网卡
  grub_efi_device_path_t *dp;	//设备路径

  dp = grub_efi_get_device_path (hnd);	//获得设备路径
  if (! dp)	//失败
    return;

  FOR_NET_CARDS (card)	//查找    define FOR_NET_CARDS(var) for (var = grub_net_cards; var; var = var->next)
  {		
    grub_efi_device_path_t *cdp;
    struct grub_efi_pxe_mode *pxe_mode;
    if (card->driver != &efidriver)	//不是efidriver
      continue;
    cdp = grub_efi_get_device_path (card->efi_handle);	//获得设备路径
    if (! cdp)	//失败
      continue;

    if (grub_efi_compare_device_paths (dp, cdp) != 0)		//比较设备路径
		{
			grub_efi_device_path_t *ldp, *dup_dp, *dup_ldp;
			int match;

		/* EDK2 UEFI PXE driver creates pseudo devices with type IPv4/IPv6		EDK2 UEFI PXE驱动程序创建类型为IPv4/IPv6的伪设备作为以太网卡的子代. 
	   as children of Ethernet card and binds PXE and Load File protocols		并将PXE和加载文件协议与其绑定。
	   to it. Loaded Image Device Path protocol will point to these pseudo	加载的图像设备路径协议将指向这些伪设备。
	   devices. We skip them when enumerating cards, so here we need to			我们在枚举卡时会跳过它们，因此在这里我们需要找到匹配的MAC设备。
	   find matching MAC device.
         */
			ldp = grub_efi_find_last_device_path (dp);	//查找最后设备路径
			if (GRUB_EFI_DEVICE_PATH_TYPE (ldp) != GRUB_EFI_MESSAGING_DEVICE_PATH_TYPE				//如果不是通讯设备路径 3
					|| (GRUB_EFI_DEVICE_PATH_SUBTYPE (ldp) != GRUB_EFI_IPV4_DEVICE_PATH_SUBTYPE		//不是IPV4设备子路径 	12
					&& GRUB_EFI_DEVICE_PATH_SUBTYPE (ldp) != GRUB_EFI_IPV6_DEVICE_PATH_SUBTYPE))	//不是IPV6设备子路径	13
				continue;
			dup_dp = grub_efi_duplicate_device_path (dp);	//复制设备路径
			if (!dup_dp)	//失败
				continue;
			dup_ldp = grub_efi_find_last_device_path (dup_dp);	//查找最后设备路径
			dup_ldp->type = GRUB_EFI_END_DEVICE_PATH_TYPE;			//设备路径节点的结束 0x7f
			dup_ldp->subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;	//0xff
			dup_ldp->length = sizeof (*dup_ldp);	//尺寸
			match = grub_efi_compare_device_paths (dup_dp, cdp) == 0;	//比较设备路径,返回真假
			grub_free (dup_dp);	//释放
			if (!match)	//如果假
				continue;
		}
    pxe_entry = grub_efi_open_protocol (hnd, &pxe_io_guid,
				  GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);	//打开协议
    if (! pxe_entry)	//失败
      continue;
    pxe_mode = pxe_entry->mode;	//模式
		discover_reply = (BOOTPLAYER *)((char *)&pxe_mode->dhcp_ack.dhcpv4);	//引导播放器	

		pxe_yip = discover_reply->yip;	//客户IP		//02 01 06 00
		pxe_sip = discover_reply->sip;	//服务器IP
		pxe_gip = discover_reply->gip;	//网关IP
		pxe_mac_type = discover_reply->Hardware;	//硬件类型
		pxe_mac_len = discover_reply->Hardlen;		//硬件地址长度
		pxe_blksize = grub_net_cards->mtu;				//最大块尺寸
		cur_pxe_type = PXE_FILE_TYPE_TFTP;				//当前类型
		set_basedir((char*)discover_reply->bootfile);	//设置基本目录(引导播放器->引导文件)
		grub_memcpy ((char *)pxe_mac, (char *)&discover_reply->CAddr, pxe_mac_len);

#if 0
#ifdef FSYS_IPXE
		ipxe_init();
#endif
#endif
    return;
  }
}

void pxe_init (void);
void
pxe_init (void)
{
//	grub_efi_loaded_image_t *image = NULL;
//	image = grub_efi_get_loaded_image (grub_efi_image_handle);	//efi获得装载映像
  grub_efinet_findcards ();		//查找卡
	grub_efi_net_config_real (image->device_handle, 0, 0);	//实际网络配置 
}


