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
#include "term.h"

grub_u32_t cur_pxe_type = 0;  //当前传输协议类型  0/1=tftp/http
grub_u32_t http_feature;  //1=支持断点续传
int map_pd;
int only_tftp = 0;
struct grub_efi_pxe *pxe_entry;
grub_u32_t pxe_sip;        //服务器IP  tftp使用
grub_u32_t pxe_yip;        //自己的IP
grub_u32_t station_ip;     //站IP
grub_u32_t subnet_mask;	 	//子网掩码
grub_u8_t *bootfile;
MAC_ADDR pxe_mac;
static void print_ip (grub_u32_t ip);
static int pxe_opened = 0;
static char filename[128];
static char *pxe_name = filename;
static int read_status;
static char *http_range = 0;
grub_u32_t pxe_http_type = 0; //0/1=http/https
static int pxe_need_read = 0; //0/1=不用读/需要读
int is_ip6 = 0;
static char default_server[128];  //默认服务器IPv4  http使用
//static grub_efi_net_interface_t *net_interface;
struct grub_efi_net_device *net_devices = 0;
grub_efi_pxe_dhcpv4_packet_t *discover_reply = 0;		//引导播放器
unsigned int max_packet_size;  //最大包尺寸
grub_efi_simple_network_t *net0;
unsigned long long hex;
char *g4e_options;
unsigned int g4e_options_size;
int ipxe = 0;

static int pxe_open (char* name);
int pxe_mount (void);
int pxe_dir (char *dirname);
unsigned long long pxe_read (unsigned long long buf, unsigned long long len, unsigned int write);
void pxe_close (void);
void pxe_unload (void);
int pxe_allocate(void);

static int tftp_open(void);
//static grub_u32_t tftp_get_size(void);
static unsigned long long  tftp_read (char *buf, grub_u64_t len);
int tftp_write (const char *name);

static int http_open(void);
static unsigned long long http_read (char *buf, grub_u64_t len);
static void pxe_configure (void);
static void http_configure (void);
//static grub_efi_ip6_config_manual_address_t *efi_ip6_config_manual_address (grub_efi_ip6_config_protocol_t *ip6_config);
//static grub_efi_ip4_config2_manual_address_t * efi_ip4_config_manual_address (grub_efi_ip4_config2_protocol_t *ip4_config);
static grub_err_t efihttp_request (grub_efi_http_t *http, char *server, char *name, int use_https, int headeronly, char *range);
unsigned long grub_strtoul (const char * restrict str, const char ** const restrict end, int base);
//static inline char *grub_lltoa (char *str, int c, unsigned long long n);
//grub_size_t grub_utf8_to_utf16 (grub_uint16_t *dest, grub_size_t destsize, const grub_uint8_t *src, grub_size_t srcsize, const grub_uint8_t **srcend);
grub_uint8_t *grub_utf16_to_utf8 (grub_uint8_t *dest, const grub_uint16_t *src, grub_size_t size);
//static int grub_efi_ip4_interface_set_manual_address (struct grub_efi_net_device *dev, grub_efi_net_ip_manual_address_t *net_ip, int with_subnet);
//static grub_efi_net_interface_t * grub_efi_ip4_interface_match (struct grub_efi_net_device *dev, grub_efi_net_ip_address_t *ip_address);
//static grub_efi_net_interface_t * grub_efi_ip6_interface_match (struct grub_efi_net_device *dev, grub_efi_net_ip_address_t *ip_address);
//static int grub_efi_net_add_pxebc_to_cards (void);
static void set_ip_policy_to_static (void);
static grub_efi_handle_t grub_efi_service_binding (grub_efi_handle_t dev, grub_efi_guid_t *service_binding_guid);


static grub_efi_guid_t ip4_config_guid = GRUB_EFI_IP4_CONFIG2_PROTOCOL_GUID;
//static grub_efi_guid_t ip6_config_guid = GRUB_EFI_IP6_CONFIG_PROTOCOL_GUID;
static grub_efi_guid_t http_service_binding_guid = GRUB_EFI_HTTP_SERVICE_BINDING_PROTOCOL_GUID;
static grub_efi_guid_t http_guid = GRUB_EFI_HTTP_PROTOCOL_GUID;
static grub_efi_guid_t pxe_io_guid = GRUB_EFI_PXE_GUID;
static grub_efi_guid_t net_io_guid = GRUB_EFI_SIMPLE_NETWORK_GUID;	//简单网络

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

//对于tftp，判断是否首次打开文件名，确定是否必须读
//配置tftp/http
//打开tftp/http
static int pxe_open (char* name)	//pxe打开
{
  grub_strcpy (pxe_name, name);

  pxe_need_read = 1;

  if (!cur_pxe_type)
  {
    pxe_configure ();   //网络接口
    pxe_opened = tftp_open();
  }
  else
  {
    http_configure();   //网络接口
    pxe_opened = http_open();
  }
  
  if (!pxe_opened)
    return !(errnum = ERR_FILE_NOT_FOUND);

  return 1;
}

int pxe_mount (void)	//pxe挂载
{
  if (current_drive != PXE_DRIVE || ! pxe_entry)	//0x21
    return 0;

  return 1;
}

/* Check if the file DIRNAME really exists. Get the size and save it in		检查文件DIRNAME是否确实存在
   FILEMAX. return 1 if succeed, 0 if fail.  */		//获取尺寸并将其保存在FILEMAX中。 如果成功则返回1，如果失败则返回0
struct pxe_dir_info	//目录信息
{
	char path[512];			//路径 尺寸0x200    e3d64c0  /boot/dir.txt
	char *dir[512];			//目录 尺寸0x1000   e3d66c0  e3d76c0 e3d76c5 e3d76d2 ...    
	char data[];				//数据 尺寸0x2e00   e3d76c0  bcd\0\a bcdedit.exe\0\a boot.sdi\0\a bootmgr.exe\0\a wimboot\0\a
} *P_DIR_INFO = NULL;//尺寸0x4000

int pxe_dir (char *dirname)	//pxe查目录
{
  int ret;
  char ch;
  ret = 1;
  ch = nul_terminate (dirname);		//以00替换止字符串的空格,回车,换行,水平制表符
 
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
//		if (P_DIR_INFO || (P_DIR_INFO = (struct pxe_dir_info*)grub_zalloc(16384)))	//建立目录信息缓存
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
//					if (pxe_read((unsigned long long)(grub_size_t)P_DIR_INFO->data,13312,GRUB_READ))  //13312计算错误
					if (pxe_read((unsigned long long)(grub_size_t)P_DIR_INFO->data,filemax,GRUB_READ))  //替换filemax，是因为读长了会把后面无用的字符串读入  2023-11-24
					{
						P_DIR_INFO->dir[0] = P_DIR_INFO->data;
						for (i = 1;i < 512 && (p = skip_to(0x100,p));++i) //遇到首个"回车,换行",使用'\0'替换.然后跳过之后的"回车,换行,空格,水平制表符",
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
//				if (*dirname == 0 || substring (dirname, p, 1) < 1)
				if (*dirname == 0 || substring (dirname, p, 1) == 0)  //*dirname为0,是'/''\0',打印全部; 其他只打印比较后一致的。
				{
					ret = 1;
          unsigned long long clo64 = current_color_64bit; //在dir.txt中，文件夹大写，文件小写。这样打印时文件夹高亮。
          unsigned int clo = current_color;
          if (*p < 0x61)
          {
						if (current_term->setcolorstate)
							current_term->setcolorstate (COLOR_STATE_HIGHLIGHT);
						current_color_64bit = (current_color_64bit & 0xffffff) | (clo64 & 0xffffff00000000);
						current_color = (current_color & 0x0f) | (clo & 0xf0);
						console_setcolorstate (current_color | 0x100);	//设置控制台文本模式的颜色(UEFI)
          }
          print_a_completion(p, 1); //EXT,lt.jmp,menu.lst,unifont.hex
          if (cursor_state & 1)
            current_term->setcolorstate (COLOR_STATE_STANDARD);
          else
            current_term->setcolorstate (COLOR_STATE_NORMAL);
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

/* Read up to SIZE bytes, returned in ADDR.  读取最多SIZE个字节，返回ADDR*/
//对于tftp，如果必须读且文件指针为0，则网络读
//从efi_pxe_buf复制len字节到buf （适用于buf/http）
unsigned long long
pxe_read (unsigned long long buf, unsigned long long len, unsigned int write)	//pxe读
{
  if (write == GRUB_WRITE)	//如果写, 则错误
    return !(errnum = ERR_WRITE);
  if (write == GRUB_LISTBLK)
    return 0;

  if (only_tftp)
  {
    printf_warning ("The system does not support the HTTP protocol.\n");
    printf_warning ("Now enable TFTP protocol.\n");
    cur_pxe_type = 0;
  }
  else if (cur_pxe_type)
    http_configure();   //http配置  必须每次读以前开启通道

  if (pxe_need_read && (!cur_pxe_type || !http_feature))  //tftp或者http_200
  {
    pxe_need_read = 0;
    pxe_allocate(); //分配内存
//    pxe_file_func[cur_pxe_type]->read(efi_pxe_buf, filemax);
    if (!cur_pxe_type)
      tftp_read(efi_pxe_buf, filemax);
    else
      http_read(efi_pxe_buf, filemax);
    printf_debug ("pxe_read: efi_pxe_buf, %x,%x\n",efi_pxe_buf,filemax);
  }
  else if (cur_pxe_type && http_feature)  //http_206
  {
    http_read ((char *)(grub_size_t)buf, len);
    printf_debug ("pxe_read: buf, %x,%x\n",buf,len);
    goto aaa;
  }
  printf_debug ("pxe_read: memmove, %x,%x,%x\n",buf,(efi_pxe_buf + filepos),len);
  grub_memmove64 (buf, (unsigned long long)(grub_size_t)(char*)(efi_pxe_buf + filepos), len);
aaa:
  filepos += len;
  return len;
}

void pxe_close (void)	//pxe关闭		grub_pxe_close (struct grub_net_card *dev __attribute__ ((unused)))
{
	if (pxe_opened)
	{
    pxe_http_type = 0; //0/1=http/https
//    pxe_need_read = 0; //0/1=不用读/需要读
	}
}

void pxe_unload (void)	//pxe卸载
{
}

//如果是map操作，则分配页，不考虑释放
//如果是其他操作，释放缓存，然后分配池
//池清零
int pxe_allocate(void) //分配内存
{
	grub_efi_status_t status;
  grub_efi_boot_services_t *b;  //引导服务
  b = grub_efi_system_table->boot_services; //系统表->引导服务
  unsigned long long bytes_needed;

  if ((*(char *)IMG(0x8205) & 0x80)) //如果8205位7置1，pxe_open不要分配内存，使用efi_pxe_buf即可。
    return 1;

  if (map_pd) //不释放内存
  {
    bytes_needed = ((filemax+4095)&(-4096ULL));
    status = efi_call_4 (b->allocate_pages, GRUB_EFI_ALLOCATE_ANY_PAGES,   
          GRUB_EFI_RESERVED_MEMORY_TYPE,                        //保留内存类型        0
          (grub_efi_uintn_t)bytes_needed >> 12, (unsigned long long *)(grub_size_t)&efi_pxe_buf);	//调用(分配页面,分配类型->任意页面,存储类型->运行时服务数据(6),分配页,地址)  
    if (status != GRUB_EFI_SUCCESS)	//如果失败
    {
      printf_errinfo ("out of map memory: %d\n",(int)status);
      errnum = ERR_WONT_FIT;
      return 0;
    }
  }
  else
  {
    if (efi_pxe_buf)
      efi_call_1 (b->free_pool, efi_pxe_buf);	//调用(释放池,释放数据)
    status = efi_call_3 (b->allocate_pool, GRUB_EFI_BOOT_SERVICES_DATA, //启动服务数据        4
                           filemax, (void **)(grub_size_t)&efi_pxe_buf); //(分配池,存储器类型->装载数据,分配字节,返回分配地址}
    if (status != GRUB_EFI_SUCCESS)		//失败
    {
      printf_errinfo ("Couldn't allocate pool.");
      return !(errnum = 0x1234);
    }
  }

	memset(efi_pxe_buf,0,filemax);  //2023-11-24
  return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//返回filemax, filepos置0
static int tftp_open(void)		//tftp打开
{
	grub_efi_status_t status;

  //将 UTF-8 转为 GBK 编码
/*
1. 在 mtftp 中，文件名使用 ansi 编码。
2. 在 mtftp 中，中文使用 gbk 码。
3. 在 mtftp 中，空格可以直接使用，无需转换。
*/
  grub_size_t len = grub_strlen (pxe_name);
  utf8_to_multimode ((void *)pxe_name, (unsigned char *)pxe_name, len, 0);

	status = efi_call_10 (pxe_entry->mtftp,					//tftp功能
	    pxe_entry,																	//pxe结构
	    GRUB_EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE,	//TFTP获得文件尺寸
	    NULL,																				//缓存
	    0,
			(grub_efi_uint64_t *)(grub_size_t)&filemax, //缓存尺寸
	    NULL,																				//块尺寸
	    (grub_u32_t *)(grub_size_t)&pxe_sip,			  //服务器IP
	    pxe_name,															      //文件名
	    NULL,
	    0);
  if (status != GRUB_EFI_SUCCESS)		//失败
	{
		printf_errinfo ("Couldn't get file size. %d\n",(int)status);
		return 0;
	}
	filepos = 0;
	return 1;
}

//读filemax字节尺寸到buf
static unsigned long long 
tftp_read (char *buf, grub_u64_t len)  //efi读
{
  grub_efi_status_t status;
	printf ("Copy data from the network via TFTP, please wait......\r");
	status = efi_call_10 (pxe_entry->mtftp,				//tftp功能
				pxe_entry,															//pxe结构
				GRUB_EFI_PXE_BASE_CODE_TFTP_READ_FILE,	//TFTP读文件
				buf,                                    //缓存
				0,
				(grub_efi_uint64_t *)(grub_size_t)&len,//缓存尺寸
				NULL,																		//块尺寸
				(grub_u32_t *)(grub_size_t)&pxe_sip,	  //服务器IP
				pxe_name,												        //文件名
				NULL,
				0);
  if (status != GRUB_EFI_SUCCESS)		//失败
	{
		printf_errinfo ("Couldn't read file.");
		return 0;
	}

  return len;
}

//写filemax字节尺寸从efi_pxe_buf
int tftp_write (const char *name)		//tftp写  2023-11-24
{
	grub_efi_status_t status;

	status = efi_call_10 (pxe_entry->mtftp,				//tftp功能
				pxe_entry,															//pxe结构
				GRUB_EFI_PXE_BASE_CODE_TFTP_WRITE_FILE, //TFTP写文件
				(char *)efi_pxe_buf,                    //缓存
				1,                                      //可以覆盖服务器上的文件
				(grub_efi_uint64_t *)(grub_size_t)&filemax,//缓存尺寸
				NULL,																		//块尺寸
				(grub_u32_t *)(grub_size_t)&pxe_sip,    //服务器IP
				(char *)name,													  //文件名
				NULL,
				0);
  if (status != GRUB_EFI_SUCCESS)		//失败
	{
		printf_errinfo ("Couldn't write file.");
    return !(errnum = 0x1234);
	}

  return 1;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//net/efi/http.c
#define GRUB_MAX_UTF16_PER_UTF8 1
#define GRUB_EFI_IP6_PREFIX_LENGTH 64
int prefer_ip6;
static grub_efi_boolean_t request_callback_done;
static grub_efi_boolean_t response_callback_done;

static void
grub_efi_http_request_callback (grub_efi_event_t event __attribute__ ((unused)),
				void *context __attribute__ ((unused))) //请求回调
{
  request_callback_done = 1;
}

static void
grub_efi_http_response_callback (grub_efi_event_t event __attribute__ ((unused)),
				void *context __attribute__ ((unused))) //响应回调
{
  response_callback_done = 1;
}

/*
响应/请求的结构，不能放在http_read内。即只能放在堆，不能放在栈。
如果放在http_read内，随机发生错误：
1. 不返回响应。response_callback_done始终为零，response_token.status始终为6。
2. 此时，response_message.body_length > len (正常为1460)。
3. 不知何故，在‘while (len)’循环内，每次循环开始，响应结构都会重置：
   起始：response_message.body_length = e62e19e
   ‘response_message.body_length = len;’后， response_message.body_length = 934f4
   ‘status = efi_call_2 (http->response, http, &response_token);’后，response_message.body_length = 958ee
   循环一阕返回‘while (len)’起始，response_message.body_length = e62e19e
*/
static grub_efi_http_message_t response_message;     //响应消息
static grub_efi_http_response_data_t response_data;  //响应数据
static grub_efi_http_token_t response_token;         //响应令牌
static grub_efi_http_message_t request_message;      //请求消息
static grub_efi_http_request_data_t request_data;    //请求数据
static grub_efi_http_token_t request_token;          //请求令牌

static grub_err_t
efihttp_request (grub_efi_http_t *http, char *server, char *name, int use_https, int headeronly, char *range) //http请求
{
  grub_efi_http_header_t request_headers[4];
  grub_efi_status_t status;
  grub_efi_boot_services_t *b = grub_efi_system_table->boot_services;
  char *url = grub_malloc (128);
  read_status = 200;  //不支持断点续传

  //请求标头
  request_headers[0].field_name = (grub_efi_char8_t *)"Host";               //请求标头.字段名称   主机，服务机
  request_headers[0].field_value = (grub_efi_char8_t *)default_server;      //请求标头.字段值      "192.168.114.1" 
  request_headers[1].field_name = (grub_efi_char8_t *)"Accept";             //请求标头.字段名称   接受
  request_headers[1].field_value = (grub_efi_char8_t *)"*/*";               //请求标头.字段值
  request_headers[2].field_name = (grub_efi_char8_t *)"User-Agent";         //请求标头.字段名称   用户代理
  request_headers[2].field_value = (grub_efi_char8_t *)"UefiHttpBoot/1.1";  //请求标头.字段值
  request_headers[3].field_name = (grub_efi_char8_t *)"";                   //请求标头.字段名称   连接        
  request_headers[3].field_value = (grub_efi_char8_t *)"";                  //请求标头.字段值     状态
  request_headers[3].field_name = (grub_efi_char8_t *)"Range";             //请求标头.字段名称   范围
  request_headers[3].field_value = (grub_efi_char8_t *)range;              //请求标头.字段值     字节范围

  grub_efi_char16_t *ucs2_url;        //ucs2网址
  grub_size_t url_len, ucs2_url_len;  //网址尺寸, ucs2网址尺寸

  if (is_ip6)  //是ip6
    grub_sprintf (url, "%s://[%s]%s", "http", default_server, pxe_name);   //协议,服务器,名称
  else  //ip4地址   
    grub_sprintf (url, "%s://%s%s", "http", default_server, pxe_name);

  url_len = grub_strlen (url);                        //网址尺寸
  ucs2_url_len = url_len + url_len/2;   //ucs2网址尺寸
  ucs2_url = grub_zalloc ((ucs2_url_len + 1) * 2);
  if (!ucs2_url)
  {
    grub_free (url);
    return 1;
  }

  //将 UTF-8 转为 URL 编码
  ucs2_url_len = utf8_to_multimode ((void *)ucs2_url, (unsigned char *)url, url_len, 2); //2024-12-30
  grub_free (url);
  ucs2_url[ucs2_url_len] = 0;                 //结束符
  request_data.url = ucs2_url;                //请求信息.url
/*
空格探测
    /ifu+352.iso
2f 00 69 00 66 00 75 00 - 2b 00 33 00 35 00 32 00
2e 00 69 00 73 00 6f 00 - 00 00                     服务器：/ifu 352.iso   http状态码：35(内部服务器错误)
    /ifu\+352.iso
2f 00 69 00 66 00 75 00 - 5c 00 2b 00 33 00 35 00
32 00 2e 00 69 00 73 00 - 6f 00 00 00               服务器：/ifu/ 352.iso  http状态码：35(内部服务器错误)
    /ifu%'2''0'352.iso
2f 00 69 00 66 00 75 00 - 25 00 32 00 30 00 33 00
35 00 32 00 2e 00 69 00 - 73 00 6f 00 00 00         服务器：/ifu 352.iso   http状态码：3(ok)

中文探测 
gbk       /ab中国cd.iso                             服务器没有请求文件
2f 61 62 d6 d0 b9 fa 63 - 64 2e 69 73 6f 00
utf16     /ab中国cd.iso                             /ab-齝d.iso
2f 00 61 00 62 00 2d 4e - fd 56 63 00 64 00 2e 00
69 00 73 00 6f 00 00 00
gbk->utf8_to_multimode(2)                           /ab中国cd.iso   ok!
61 00 62 00 d6 00 d0 00 - b9 00 fa 00 63 00
64 00 2e 00 69 00 73 00 - 6f 00 00 00
结论：
1. linux的url采用utf16方法表示。
2. url中中文使用gbk码，一般使用两字节表示。在url里，中文的两字节要分别放在两个utf16中。
3. 空格使用 %20 表示。即 25 00 32 00 30 00
*/
  //请求数据.方法
  request_data.method = (headeronly > 0) ? GRUB_EFI_HTTPMETHODHEAD : GRUB_EFI_HTTPMETHODGET;  //头朝前?头:获得
  //请求信息
  request_message.data.request = &request_data; //请求信息.数据请求
  request_message.headers = request_headers;    //请求信息.标头
  request_message.body_length = 0;              //请求信息.体尺寸
  request_message.body = NULL;                  //请求信息.体
  //请求令牌
  request_token.event = NULL;                   //请求令牌.事件
  request_token.status = GRUB_EFI_NOT_READY;    //请求令牌.状态  未准备好
  request_token.message = &request_message;     //请求令牌.消息

  request_callback_done = 0;                    //请求回调完成=0
  status = efi_call_5 (b->create_event,                  //创建事件
                       GRUB_EFI_EVT_NOTIFY_SIGNAL,       //事件的类型       通知信号
//使用GRUB_EFI_TPL_CALLBACK(回调)优先级别时，执行b->connect_controller、b->load_image、b->start_image时，回调函数不执行
//                       GRUB_EFI_TPL_CALLBACK,            //事件的优先级     回调
                       GRUB_EFI_TPL_NOTIFY,              //事件的优先级     通知
                       grub_efi_http_request_callback,   //事件处理函数     请求回调
                       NULL,                             //传递给事件处理函数的参数
                       &request_token.event);
  if (status != GRUB_EFI_SUCCESS) //失败
  {
    grub_free (request_data.url);
    printf_errinfo ("Fail to create an event status=%x\n", status);
    return 1;
  }

  status = efi_call_2 (http->request, http, &request_token); //请求       有时莫名其妙地死在这里，必须重新启动虚拟机!!!!
  if (status != GRUB_EFI_SUCCESS) //失败
  {
    efi_call_1 (b->close_event, request_token.event);   //关闭事件
    grub_free (request_data.url);
    printf_errinfo ("Fail to send a request status=%x\n", status); //12超时  f拒绝访问
    return 1;
  }
  /* TODO: Add Timeout */
  int qqq = 0;
  while (!request_callback_done)  //等待请求回调完成
  {
    efi_call_1(http->poll, http); //获得
    qqq++;
    if (qqq >= 0xff)
    {
      printf_debug ("request_poll,%x\n",qqq);
      break;
    }
  }
  //响应数据
  response_data.status_code = GRUB_EFI_HTTP_STATUS_UNSUPPORTED_STATUS;  //响应数据.状态代码  0=不受支持的状态
  //响应消息
  response_message.data.response = &response_data;  //响应数据.数据响应
  //herader_count将由HTTP驱动程序在响应时更新
  response_message.header_count = 0;                //响应数据.标头计数
  //标头将由驱动程序在响应时填充
  response_message.headers = NULL;                  //响应数据.标头
  //使用零BodyLength仅接收响应标头
  response_message.body_length = 0;                 //响应数据.体尺寸
  response_message.body = NULL;                     //响应数据.体
  //响应令牌.事件
  response_token.event = NULL;

  status = efi_call_5 (b->create_event,         //创建事件
              GRUB_EFI_EVT_NOTIFY_SIGNAL,       //事件的类型       通知信号
//              GRUB_EFI_TPL_CALLBACK,            //事件的优先级     回调
              GRUB_EFI_TPL_NOTIFY,              //事件的优先级     通知
              grub_efi_http_response_callback,  //事件处理函数     响应回调
              NULL,                             //传递给事件处理函数的参数
              &response_token.event);
  if (status != GRUB_EFI_SUCCESS)
  {
    efi_call_1 (b->close_event, request_token.event);   //关闭事件
    grub_free (request_data.url);
    printf_errinfo ("Fail to create an event\n status=%x\n", status);
    return 1;
  }
  //响应令牌
  response_token.status = GRUB_EFI_SUCCESS;   //响应令牌.状态  成功
  response_token.message = &response_message; //响应令牌.消息

  //等待HTTP响应
  response_callback_done = 0;   //响应回调完成=0
  status = efi_call_2 (http->response, http, &response_token);  //响应
  if (status != GRUB_EFI_SUCCESS)
  {
    efi_call_1 (b->close_event, response_token.event);   //关闭事件
    efi_call_1 (b->close_event, request_token.event);   //关闭事件
    grub_free (request_data.url);
    printf_errinfo ("Fail to receive a response! status=%x\n", status); //12超时
    return 1;  
  }

  /* TODO: Add Timeout */
  qqq = 0;
  while (!response_callback_done)   //等待响应回调完成
  {
    efi_call_1 (http->poll, http);  //获得
    qqq++;
    if (qqq >= 0xff)
    {
      printf_debug ("response_poll,%x\n",qqq);
      break;
    }
  }

  //返回部分内容，是我们请求了范围，不是错误
  if (response_message.data.response->status_code == GRUB_EFI_HTTP_STATUS_206_PARTIAL_CONTENT)
  {
    read_status = 206;  //支持断点续传
    goto aaa;
  }

  if (response_message.data.response->status_code != GRUB_EFI_HTTP_STATUS_200_OK)
  {
    grub_efi_http_status_code_t status_code = response_message.data.response->status_code;

    if (response_message.headers)
      efi_call_1 (b->free_pool, response_message.headers);
    efi_call_1 (b->close_event, response_token.event);   //关闭事件
    efi_call_1 (b->close_event, request_token.event);   //关闭事件
    grub_free (request_data.url);
    if (status_code == GRUB_EFI_HTTP_STATUS_404_NOT_FOUND)  //未找到
      printf_errinfo ("404: file `%s' not found\n", pxe_name);  
    else if (status_code == GRUB_EFI_HTTP_STATUS_416_REQUESTED_RANGE_NOT_SATISFIED) //范围不满足要求
      printf_errinfo ("416: The scope does not meet the requirements\n");
    else
      printf_errinfo ("unsupported uefi http status code %d\n", status_code); //不支持的uefi http

    return 1; 
  }

aaa:;
  int i;
  //从ContentLength标头解析文件的长度
  for (i = 0; i < (int)response_message.header_count; ++i)
  {
//Connection          close                         连接:         关闭
//Content-Type        application/octet-stream      内容类型:     应用程序/八位字节流
//Content-Length      6637568                       内容尺寸:     6637568(ASCII码)
//Server              Indy/9.00.10                  服务器:       Indy/9.00.10
//Range               0-1023                        范围:         0-1023字节
//Last-Modified       Thu, 30 Jun 2022 08:53:38 GMT 最后修改时间: Thu, 30 Jun 2022 08:53:38 GMT
    if (!grub_strcmp((const char*)response_message.headers[i].field_name, "Content-Length"))
    {
      safe_parse_maxint ((char**)&response_message.headers[i].field_value, &hex);
      if (read_status == 200)
        filemax = hex;
      filesize = hex;
    }
    else if (!grub_strcmp((const char*)response_message.headers[i].field_name, "Content-Range"))
    {
      char *value = (char *)response_message.headers[i].field_value;
      while (*value != '/')
      value++;
      value++;
      safe_parse_maxint (&value, &filemax);
    }
  }

  if (response_message.headers)
    efi_call_1 (b->free_pool, response_message.headers);
  efi_call_1 (b->close_event, response_token.event);   //关闭事件
  efi_call_1 (b->close_event, request_token.event);   //关闭事件
  grub_free (request_data.url);

  if (!filemax) //很不幸，dhcpsrv2.5.2自带的http服务(dhcpsrv)，执行HEAD操作，返回filemax=0！
  {
    char tmp[4] = {0};
    char range0[16] = {0};
    http_range = range0;
    grub_sprintf (range0, "bytes=1-1");
    http_read (tmp, 1);
    http_range = 0;
  }
  return GRUB_ERR_NONE;
}

//返回filemax, filepos置0
//读filemax字节尺寸到efi_pxe_buf
static int http_open(void)   //http打开
{
  int err;
  printf_debug ("http_open,%s\n",pxe_name);
  request_message.header_count = 3;             //请求信息.标头计数
  err = efihttp_request (net_devices->http, (char *)default_server, (char *)pxe_name, 0, 1, 0);  //请求头部，返回尺寸
  if (err)
    return 0;

  printf_debug ("filemax=%x\n",filemax);
	filepos = 0;
  return 1;
}

//读len字节尺寸到buf
static unsigned long long
http_read (char *buf, grub_u64_t len)  //efi读
{
  grub_efi_status_t status;                 //状态
  grub_size_t sum = 0, sum1 = 0;            //和
  grub_efi_boot_services_t *b = grub_efi_system_table->boot_services; //引导服务
  grub_efi_http_t *http = net_devices->http;        //http入口
  grub_u64_t back_len = len;
  char *back_buf = buf;
  char r[32];
  char *range = r;
  int err;
#if 0
  //用于响应回调函数不执行的情况
  grub_u64_t range_len = len;
#endif
  grub_u64_t range_end = filepos+len-1;  //len在下面的循环中会改变

  printf_debug ("http_read: %x, %x, %x\n",buf,len,filepos);
  if (!len) //尺寸为零
  {
    printf_errinfo ("Invalid arguments to EFI HTTP Read\n");  //EFI HTTP读取的参数无效
    return 0;
  }

  request_message.header_count = 4;             //请求信息.标头计数
  if (!http_range)
  {
    if (!http_feature)
      grub_sprintf (range, "bytes=0-");
    else
      grub_sprintf (range, "bytes=%d-%d",filepos, range_end);
  }
  else
    range = http_range;

  if (!http_feature)
    printf ("Copy data from the network via HTTP, please wait......\r");

repeat:
  printf_debug ("read_range: %s;    read_len: %x\n",r,len);

  err = efihttp_request (net_devices->http, (char *)default_server, (char *)pxe_name, 0, 0, range); //请求获得
  if (err)
    return 0;

/* 
1.如果客户端读尺寸不等于服务器写尺寸，会搞乱通讯指针。使得下一次读取错误。
  len不能大于filesize。在这里判断及纠正。
2.len也不能小于filesize，在调用http_read或者pxe_read前，需获得filemax。
*/
  if (len > filesize) //len是客户端读尺寸，filesize是服务器写尺寸。
    len = filesize;

  if (http_feature)
    printf_debug ("206: ");
  else
    printf_debug ("200: ");
  printf_debug ("filesize=%x, filemax=%x, len=%x\n",filesize,filemax,len);
    
  response_token.event = NULL;  //增加
  status = efi_call_5 (b->create_event,         //创建事件
              GRUB_EFI_EVT_NOTIFY_SIGNAL,       //事件的类型       通知信号
//              GRUB_EFI_TPL_CALLBACK,            //事件的优先级     回调
              GRUB_EFI_TPL_NOTIFY,              //事件的优先级     通知
              grub_efi_http_response_callback,  //事件处理函数     响应回调
              NULL,                             //传递给事件处理函数的参数
              &response_token.event);           //创建的事件
  if (status != GRUB_EFI_SUCCESS) //失败
  {
    printf_errinfo ("Fail to create an event_response, status=%x\n", status);
    return 0;
  }

//  efi_call_1 (grub_efi_system_table->boot_services->stall, 10000);  //延时10毫秒
  while ((long long)len > 0)
  {
    //响应消息
    response_message.data.response = NULL;      //响应消息.数据.响应
    response_message.header_count = 0;          //响应消息.标头计数
    response_message.headers = NULL;            //响应消息.标头
#if 0
    //用于响应回调函数不执行的情况
    response_message.body_length = range_len;   //响应消息.体长   设置为654800，他实际读5b4，
#else
    response_message.body_length = len;         //响应消息.体长
#endif
    response_message.body = (void *)(grub_size_t)buf; //响应消息.体

    //响应令牌
    response_token.message = &response_message; //响应令牌.消息
    response_token.status = GRUB_EFI_NOT_READY; //响应令牌.状态    还没准备好
    response_callback_done = 0;   //响应回调已完成=0

    status = efi_call_2 (http->response, http, &response_token);  //响应
    if (status != GRUB_EFI_SUCCESS) //失败
    {
      printf_warning ("Fail to http->response! status=%x,len=%x,sum=%x\n", (int)status,len,sum);   //f 拒绝访问;  68  通信对等体已关闭连接，并且实例的接收缓冲区中没有更多数据。
//printf ("111,%d,%d,%d\n",response_message.body_length,len,sum);
//68,1b1800;              f,d69eab5;
//1b1800,4a3000,654800;   d69eab5,55af024,12c4dad9
//68,1be
//491,491,21
      if (status == GRUB_EFI_CONNECTION_FIN)
      {
        efi_call_1 (b->close_event, response_token.event);   //关闭事件
        efi_call_2 (http->cancel, http, NULL);
        errnum = 0;
        if (http_feature)  //支持断点续传
        {
          //不采用"bytes=%d-"，是为了照顾TinyPXEServer-1.0.0.23自带http服务(Indy/9.00.10)
//          grub_sprintf (r, "bytes=%d-%d", sum, sum + len - 1);
          grub_sprintf (r, "bytes=%d-%d", filepos + sum, range_end);  //不考虑http_range存在的情况
        }
        else
        {
          buf = back_buf;
          len = back_len;
          grub_sprintf (r, "bytes=0-");
        }

        if (debug > 1)
          getkey();
        http_configure();   //配置网络接口
        goto repeat;
      }
      if (status == GRUB_EFI_ACCESS_DENIED)
        printf_errinfo ("The host has closed the TCP connection.\n");

      return 0;
    }
//    efi_call_1 (grub_efi_system_table->boot_services->stall, 1);  //延时1微妙  必要

    int qqq = 0;
    grub_u64_t first = 0;
    while (!response_callback_done)
    {
      efi_call_1(http->poll, http); //获得
      qqq++;
#if 0
      //以下用于响应回调函数不执行的情况
      if (qqq == 1)
      {
        if (response_message.body_length != filesize)
          break;
        first = response_message.body_length;
      }
      else if (qqq < 0xfff)
      {
        if (response_message.body_length != first)
        {
          printf_debug ("read_poll,%x,%x,%x\n",qqq,response_message.body_length,first);
          //257,5d4,800;  4,5d4,1000;  vm
          break;
        }
      }
      else
#endif
      if (qqq >= 0xfff)
      {
        printf_debug ("read_poll,%x,%x,%x,%x,%x,%x,%x\n",qqq,filepos,response_message.body_length,len,filesize,first,sum);
//fff,2abe000,100000,d8000,100000,100000,2800;   qemu
//fff,0,800,800,800,800,0;  fff,8000,800,800,800,800,0;  vm
//1a800,20000,9da0,20000,20000,16260
//3a800,20000,9da0,20000,20000,16260
//5a800,20000,9da0,20000,20000,16260
//7a800,20000,9da0,20000,20000,16260
//9a800,20000,9da0,20000,20000,16260
//0,800,800,800,800,0
        efi_call_1 (b->close_event, response_token.event);   //关闭事件
        efi_call_2 (http->cancel, http, NULL);
        errnum = 0;
        if (http_feature)  //支持断点续传
        {
          //不采用"bytes=%d-"，是为了照顾TinyPXEServer-1.0.0.23自带http服务(Indy/9.00.10)
          grub_sprintf (r, "bytes=%d-%d", filepos + sum, range_end);
        }
        else
        {
          buf = back_buf;
          len = back_len;
          grub_sprintf (r, "bytes=0-");
        }

        if (debug > 1)
          getkey();
        http_configure();   //配置网络接口
        goto repeat;
      }
    }

#if 0
    //用于响应回调函数不执行的情况
    //修正下一次参数
    if (response_message.body_length > len)
      response_message.body_length = len;
#endif
    sum += response_message.body_length;  //和
    len -= response_message.body_length;  //剩余尺寸
    buf += response_message.body_length;  //缓存

    sum1 += response_message.body_length;  //打印计数
    if (sum1 >= 0x800000) // 8MB打印一次
    {
      grub_printf("[%ldM/%ldM]\r",sum>>20,filesize>>20);
      sum1 -= 0x800000;
    }

/* 使用qemu虚拟机，启动ifu352.iso，可以产生"Fail to http->response!68"，用于测试断点续传。//
    if (debug == 0)
      printf ("000,%x,%x,%x\n",response_message.body_length,len,sum);
5a0,1b2625,4a21db
b40,1b1ae5,4a2d1b
2e5,1b1800,4a3000

5a0,b40,10e0, zemu
5b4, vm

read_poll,f,5a0,654800;
1440,6479397,158171;
read_poll,fff,0,654800,62de25,654800,654800,269db;
6637568,-158171,6795739;
*/
  }

//  efi_call_1 (grub_efi_system_table->boot_services->stall, 10000);  //延时10毫秒  必需，否则只能持续读3个文件
  efi_call_1 (b->close_event, response_token.event);   //关闭事件
  return sum; //返回读尺寸
}

static void
http_configure (void)  //http配置
{
  grub_efi_http_config_data_t http_config;    //HTTP配置数据
  grub_efi_httpv4_access_point_t httpv4_node; //HTTPv4访问点
//  grub_efi_httpv6_access_point_t httpv6_node; //HTTPv6访问点
  grub_efi_status_t status;
  grub_efi_http_t *http = net_devices->http;  //HTTP入口

  grub_memset (&http_config, 0, sizeof(http_config));  //初始化HTTP配置数据
  http_config.http_version = GRUB_EFI_HTTPVERSION11;    //HTTP配置数据.版本=11
  http_config.timeout_millisec = 5000;                  //HTTP配置数据.超时=5000毫秒
#if 0
  if (prefer_ip6) //如果首选ip6
  {
    grub_efi_uintn_t sz;
    grub_efi_ip6_config_manual_address_t manual_address;//ip6配置手动地址

    http_config.local_address_is_ipv6 = 1;              //HTTP配置数据.本地地址是ipv6 = 0
    sz = sizeof (manual_address);                       //ip6配置手动地址尺寸
    status = efi_call_4 (net_devices->ip6_config->get_data, net_devices->ip6_config,
        GRUB_EFI_IP6_CONFIG_DATA_TYPE_MANUAL_ADDRESS,
        &sz, &manual_address);                          //ip6配置获得手动地址

    if (status == GRUB_EFI_NOT_FOUND)
    {
      printf_errinfo ("The MANUAL ADDRESS is not found\n");
      errnum = 0x1234;
      return ;
    }

    //手动界面将返回缓冲区太小!!!
    if (status != GRUB_EFI_SUCCESS)
    {
      printf_errinfo ("??? %d\n",(int) status);
      errnum = 0x1234;
      return;
    }

    grub_memcpy (httpv6_node.local_address, manual_address.address, sizeof (httpv6_node.local_address));
    httpv6_node.local_port = 0;
    http_config.access_point.ipv6_node = &httpv6_node;
  }
  else  //是ip4
#endif
  {
    http_config.local_address_is_ipv6 = 0;               //HTTP配置数据.本地地址是ipv6 = 0
    grub_memset (&httpv4_node, 0, sizeof(httpv4_node)); //HTTPv4访问点初始化
    httpv4_node.use_default_address = 1;                 //HTTPv4访问点.

    //在此处使用随机端口
    //请参阅edk2/NetworkPkg/TcpDxe/TcpDispatcher中的TcpBind().c
    httpv4_node.local_port = 0;
    http_config.access_point.ipv4_node = &httpv4_node;
  }
  status = efi_call_2 (http->configure, http, NULL);    //停止
//  efi_call_1 (grub_efi_system_table->boot_services->stall, 10);  //延时10微秒  非必需
#if 0
  //设置手动地址
  grub_efi_net_ip_manual_address_t net_ip;
  grub_efi_ip4_config2_manual_address_t *address = &net_ip.ip4;
  *(int*)net_ip.ip4.address = station_ip;
  *(int*)net_ip.ip4.subnet_mask = subnet_mask;
  net_ip.is_ip6 = 0;
  
  status = efi_call_4 (net_devices->ip4_config->set_data, net_devices->ip4_config,
		    GRUB_EFI_IP4_CONFIG2_DATA_TYPE_MANUAL_ADDRESS,  //手动地址
		    sizeof(*address), address);
  printf_debug ("status=%x\n",status);
  if (status != GRUB_EFI_SUCCESS)
    return;
#endif
  status = efi_call_2 (http->configure, http, &http_config);  //配置
//  efi_call_1 (grub_efi_system_table->boot_services->stall, 10);  //延时10微秒  非必需
  if (status != GRUB_EFI_SUCCESS)
  {
    printf_errinfo ("couldn't configure http protocol, reason: %d\n", (int)status);
    errnum = 0x1234;
    return;
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//net/efi/pxe.c
static void
pxe_configure (void) //pxe配置
{
//  grub_efi_pxe_t *pxe = (is_ip6) ? net_devices->ip6_pxe : net_devices->ip4_pxe; //首选ip6,则选择ip6_pxe，否则选择ip4_pxe
  grub_efi_pxe_t *pxe = pxe_entry;
  grub_efi_pxe_mode_t *mode = pxe->mode;

  if (!mode->started) //如果未启动
  {
    grub_efi_status_t status;
    status = efi_call_2 (pxe->start, pxe, is_ip6);  //启动
    if (status != GRUB_EFI_SUCCESS) //失败
      printf_debug ("Couldn't start PXE\n"); //无法启动PXE
  }
  //PXE指针    PXE状态 0/1/2=停止/已启动/已初始化    PXE使用ipv4/ipv6=0/1
  printf_debug ("pxe: %x    pxe started: %u    net started: %u\n", pxe,mode->started,net0->mode->state);
#if 0
  if (mode->using_ipv6) //如果使用ipv6
  {
    grub_efi_ip6_config_manual_address_t *manual_address;
    manual_address = efi_ip6_config_manual_address (net_devices->ip6_config);  //获得ip6配置手动地址
    printf_debug ("ip6_manual_address=%x\n", manual_address);

    if (manual_address &&
            grub_memcmp ((const char *)manual_address->address, (const char *)mode->station_ip.v6, sizeof (manual_address->address)) != 0)  //复制站ipv6作为手动地址成功
    {
      grub_efi_status_t status;
      grub_efi_pxe_ip_address_t station_ip;

      grub_memcpy (station_ip.v6.addr, manual_address->address, sizeof (station_ip.v6.addr));
      status = efi_call_3 (pxe->set_station_ip, pxe, (grub_u32_t *)(grub_size_t)&station_ip, NULL);  //设置站ip

      if (status != GRUB_EFI_SUCCESS)
	      printf_debug ("Couldn't set station ip\n");

      grub_free (manual_address);
    }
  }
  else
  {
    grub_efi_ip4_config2_manual_address_t *manual_address;
    manual_address = efi_ip4_config_manual_address (net_devices->ip4_config);  //获得配置ip4地址
    printf_debug ("ip4_manual_address=%x\n", manual_address);
    printf_debug ("mode->station_ip.v4=%x,%x\n", mode->station_ip.v4,*(int*)mode->station_ip.v4);
    printf_debug ("manual_address->address=%x,%x,%x,%x\n", manual_address->address,*(int*)manual_address->address,mode->station_ip.v4,*(int*)mode->station_ip.v4);
    printf_debug ("manual_address->subnet_mask=%x,%x,%x,%x\n", manual_address->subnet_mask,*(int*)manual_address->subnet_mask,mode->subnet_mask.v4,*(int*)mode->subnet_mask.v4);
    if (manual_address &&
            grub_memcmp ((const char *)manual_address->address, (const char *)mode->station_ip.v4, sizeof (manual_address->address)) != 0)  //比较站ip，不同则复制
    {
      grub_efi_status_t status;
      grub_efi_pxe_ip_address_t station_ip0;
      grub_efi_pxe_ip_address_t subnet_mask0;

      grub_memcpy (station_ip0.v4.addr, manual_address->address, sizeof (station_ip0.v4.addr));
      grub_memcpy (subnet_mask0.v4.addr, manual_address->subnet_mask, sizeof (subnet_mask0.v4.addr));
      printf_debug("manual_address->address=%s\n", manual_address->address);
      printf_debug("manual_address->subnet_mask=%s\n", manual_address->subnet_mask);

      status = efi_call_3 (pxe->set_station_ip, pxe, (grub_u32_t *)(grub_size_t)&station_ip0, (grub_u32_t *)(grub_size_t)&subnet_mask0);//设置站ip
      if (status != GRUB_EFI_SUCCESS)
	      printf_debug ("Couldn't set station ip\n");

      grub_free (manual_address);
    }
  }

  if (mode->using_ipv6)
  {
    printf_debug ("PXE STATION IP: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",   //PXE站IP:
          mode->station_ip.v6.addr[0],
          mode->station_ip.v6.addr[1],
          mode->station_ip.v6.addr[2],
          mode->station_ip.v6.addr[3],
          mode->station_ip.v6.addr[4],
          mode->station_ip.v6.addr[5],
          mode->station_ip.v6.addr[6],
          mode->station_ip.v6.addr[7],
          mode->station_ip.v6.addr[8],
          mode->station_ip.v6.addr[9],
          mode->station_ip.v6.addr[10],
          mode->station_ip.v6.addr[11],
          mode->station_ip.v6.addr[12],
          mode->station_ip.v6.addr[13],
          mode->station_ip.v6.addr[14],
          mode->station_ip.v6.addr[15]);
  }
  else
  {
    printf_debug ("PXE STATION IP: %d.%d.%d.%d\n",   //PXE站IP:
          mode->station_ip.v4[0],
          mode->station_ip.v4[1],
          mode->station_ip.v4[2],
          mode->station_ip.v4[3]);
    printf_debug ("PXE SUBNET MASK: %d.%d.%d.%d\n",  //PXE子网掩码:
          mode->subnet_mask.v4[0],
          mode->subnet_mask.v4[1],
          mode->subnet_mask.v4[2],
          mode->subnet_mask.v4[3]);
  }
#endif
  /* TODO: Set The Station IP to the IP2 Config 将站IP设置为IP2配置*/
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//net->efi->net.c

char card_name[10];
static int grub_efi_net_find_cards (void);
static int
grub_efi_net_find_cards (void)   //查找支持ip4配置2的卡  初始化http
{
  grub_efi_uintn_t num_handles;
  grub_efi_handle_t *handles;
  grub_efi_handle_t *handle;
  int id;
  printf_debug ("grub_efi_net_find_cards:\n");
  //查找支持ip4配置的句柄
  handles = grub_efi_locate_handle (GRUB_EFI_BY_PROTOCOL, &ip4_config_guid,
				    0, &num_handles);	//定位ip4句柄
  printf_debug ("ip4_handles=%x, num_handles=%x\n",handles,num_handles);
  if (!handles)
  {
    printf_errinfo ("Does not support EFI_IP4FHIR G2-POTOCOL!\n");
    return 1;
  }

  for (id = 0, handle = handles; num_handles--; handle++, id++)
  {
    grub_efi_device_path_t *dp;
    grub_efi_ip4_config2_protocol_t *ip4_config;
//    grub_efi_ip6_config_protocol_t *ip6_config;
    grub_efi_handle_t http_handle;
    grub_efi_http_t *http;
    struct grub_efi_net_device *d;

    printf_debug ("*handle=%x\n",*handle);
    dp = grub_efi_get_device_path (*handle);  //获得设备路径
    if (!dp)
      continue;
    if (debug > 1)
      grub_efi_print_device_path(dp); //打印路径

    ip4_config = grub_efi_open_protocol (*handle, &ip4_config_guid,
				    GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL); //打开ip4协议     
    if (!ip4_config)  //不支持ip4
      continue;
    printf_debug ("ip4_config=%x\n",ip4_config);

//    ip6_config = grub_efi_open_protocol (*handle, &ip6_config_guid,
//				    GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL); //打开ip6协议
    http_handle = grub_efi_service_binding (*handle, &http_service_binding_guid); //http服务绑定
    http = (http_handle) 
          ? grub_efi_open_protocol (http_handle, &http_guid, GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL) //http服务绑定成功,打开http协议
          : NULL;
    printf_debug ("http=%x\n",http);
    if (!http)
    {
      printf_errinfo ("Does not support EFI_HTTP_PROTOCOL!\n");
      errnum = 0x1234;
      goto err;
    }

    d = grub_malloc (sizeof (*d));  //分配内存
    if (!d) //如果失败
    {
      while (net_devices)
	    {
	      d = net_devices->next;
	      grub_free (net_devices);
	      net_devices = d;
	    }
      goto err;
    }
    //创建网络设备
    d->handle = *handle;              //句柄
    d->ip4_config = ip4_config;       //ip4配置
//    d->ip6_config = ip6_config;       //ip6配置
    d->http_handle = http_handle;     //http句柄
    d->http = http;                   //http入口
    d->next = net_devices;            //下一个
    grub_sprintf (card_name,"efinet%d", id);
    d->card_name = card_name;         //网卡名称
    d->net_interfaces = NULL;         //网络接口
    net_devices = d;                  //网络设备入口
    printf_debug ("net_devices=%x\n",net_devices);
    if (debug > 1)
      getkey();
  }

  grub_free (handles);
  set_ip_policy_to_static (); //将ip策略设置为静态
 
  //设置手动地址
  grub_efi_status_t status;
  grub_efi_net_ip_manual_address_t net_ip;
  grub_efi_ip4_config2_manual_address_t *address = &net_ip.ip4;
  printf_debug ("station_ip=%x\n",station_ip);
  printf_debug ("subnet_mask=%x\n",subnet_mask);

  *(int*)net_ip.ip4.address = station_ip;
  *(int*)net_ip.ip4.subnet_mask = subnet_mask;
  net_ip.is_ip6 = 0;
  status = efi_call_4 (net_devices->ip4_config->set_data, net_devices->ip4_config,
		    GRUB_EFI_IP4_CONFIG2_DATA_TYPE_MANUAL_ADDRESS,  //手动地址
		    sizeof(*address), address);
  printf_debug ("status=%x\n",status);
  if (status != GRUB_EFI_SUCCESS)
    goto err;

  if (debug > 1)
    getkey();
  return 0;
  
err:
  grub_free (handles);
  return 1;
}

static void
set_ip_policy_to_static (void) //将ip策略设置为静态
{
  struct grub_efi_net_device *dev;
  printf_debug ("set_ip_policy_to_static:\n");

  for (dev = net_devices; dev; dev = dev->next)
  {
    grub_efi_ip4_config2_policy_t ip4_policy = GRUB_EFI_IP4_CONFIG2_POLICY_STATIC;  //静态
    printf_debug ("dev=%x, ip4_config=%x\n",dev,dev->ip4_config);

    if (efi_call_4 (dev->ip4_config->set_data, dev->ip4_config,
            GRUB_EFI_IP4_CONFIG2_DATA_TYPE_POLICY,                                  //策略
            sizeof (ip4_policy), &ip4_policy) != GRUB_EFI_SUCCESS)
      printf_debug ("could not set GRUB_EFI_IP4_CONFIG2_POLICY_STATIC on dev `%s'\n", dev->card_name);  //无法在dev上设置GRUBEFI_IP4_CONFIG2_POLICY_STATIC
#if 0
    printf_debug ("ip6_config=%x\n",dev->ip6_config);
    if (dev->ip6_config)
    {
      grub_efi_ip6_config_policy_t ip6_policy = GRUB_EFI_IP6_CONFIG_POLICY_MANUAL;

      if (efi_call_4 (dev->ip6_config->set_data, dev->ip6_config,
              GRUB_EFI_IP6_CONFIG_DATA_TYPE_POLICY,
              sizeof (ip6_policy), &ip6_policy) != GRUB_EFI_SUCCESS)
        printf_debug ("could not set GRUB_EFI_IP6_CONFIG_POLICY_MANUAL on dev `%s'\n", dev->card_name); //无法在dev上设置GRUB_EFI_IP6_CONFIG_POLICY_MANUAL
    }
#endif
  }
}

static grub_efi_handle_t
grub_efi_service_binding (grub_efi_handle_t dev, grub_efi_guid_t *service_binding_guid) //dev服务绑定
{
  grub_efi_service_binding_t *service;
  grub_efi_status_t status;
  grub_efi_handle_t child_dev = NULL;

  service = grub_efi_open_protocol (dev, service_binding_guid, GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);  //打开服务绑定协议
  if (!service) //失败
  {
    printf_errinfo ("couldn't open efi service binding protocol\n"); //无法打开efi服务绑定协议
    return NULL;
  }

  status = efi_call_2 (service->create_child, service, &child_dev); //服务->创建子项
  if (status != GRUB_EFI_SUCCESS) //失败
  {
    printf_errinfo ("Failed to create child device of http service\n"); //无法创建http服务的子设备
    return NULL;
  }

  return child_dev; //子设备
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//net\drivers\efi\efinet.c

grub_u32_t get_dhcp_option_66(grub_efi_pxe_dhcpv4_packet_t *bp);
grub_u32_t get_dhcp_option_66(grub_efi_pxe_dhcpv4_packet_t *bp)  //选项66支持   由江南一根葱提供
{
  unsigned char *option_ptr = bp->dhcp_options;

//01 客户端子网掩码; 03 路由器IP; 05 名称服务器IP; 06 DNS服务器IP; 12(0c) 指定客户端主机名; 15(0f) DNS域名; 28(1c) 广播地址;
//51(33) IP地址租用时间; 54(36) 标识服务器IP; 55(37) 客户端请求特定选项; 61(3d) 客户端硬件类型+MAC地址; 66(42) 标识TFTP服务器名称;
//67(43) 指定引导文件名; 150(96) TFTP服务器地址;
//TinyPXEServe dhcp
//63 82 53 63 35 01 05 36 - 04 c0 a8 58 01 3c 09 50   36:192.168.88.1
//58 45 43 6c 69 65 6e 74 - 61 11 00 56 4d 5c a3 70
//77 b7 3a 9b 4a 2b 44 b7 - 83 7a 6b 01 04 ff ff ff   01:255.255.255.0
//00 03 04 c0 a8 58 01 06 - 04 c0 a8 58 01 33 04 00   03:192.168.88.1   06:192.168.88.1   33:0.1.81.128
//01 51 80 ff 00 00 00 00
//TinyPXEServe proxy
//63 82 53 63 35 01 05 36 - 04 c0 a8 58 fe 33 04 00   36:192.168.88.254
//00 07 08 01 04 ff ff ff - 00 03 04 c0 a8 58 02 06   03:192.168.88.2   06:192.168.88.2
//04 c0 a8 58 02 0f 0b 6c - 6f 63 61 6c 64 6f 6d 61   1c:192.168.88.255
//69 6e 1c 04 c0 a8 58 ff - 3a 04 00 00 03 84 3b 04
//00 00 06 27 ff 00 00 00

  //如果存在魔术饼干则跳过(0x63825363)
//  if (option_ptr[0] == 0x63 && option_ptr[1] == 0x82 && option_ptr[2] == 0x53 && option_ptr[3] == 0x63)
//    option_ptr += 4;
  while (*option_ptr != 0xFF) //循环到结束选项(0xFF)
  {
    unsigned char option_code = *option_ptr;
    unsigned char option_len = *(option_ptr + 1);
    if (option_code == 0x42) { //选项66：下一个服务器IP地址
      if (option_len == 4) //确保它是IPv4地址
        return *(grub_u32_t *)(option_ptr + 2);
  }
    option_ptr += (2 + option_len); //移动到下一个选项（代码+长度+值）
    //基本健全性检查，防止对格式错误的数据包进行无限循环
    if (option_ptr >= (unsigned char *)bp + sizeof(grub_efi_pxe_dhcpv4_packet_t))
      break;
  }
  return 0; //没有找到
}

static int grub_efinet_findcards (void);
static int
grub_efinet_findcards (void)	//查找支持简单网络接口的卡  初始化tftp
{
  grub_efi_uintn_t num_handles;
  grub_efi_handle_t *handles;
  grub_efi_handle_t *handle;
  printf_debug ("grub_efinet_findcards:\n");
 
  /* 查找支持简单网络接口的句柄 */
  handles = grub_efi_locate_handle (GRUB_EFI_BY_PROTOCOL, &net_io_guid,
				    0, &num_handles);	//定位句柄
  if (! handles)	//失败
    return 1;
  printf_debug ("handles=%x, num_handles=%x\n",handles,num_handles);//e59bd80,3

  struct grub_efi_pxe *pxe = 0;
  //查找MAC消息设备
  for (handle = handles; num_handles--; handle++)
	{
		grub_efi_simple_network_t *net;	//简单网络
		grub_efi_device_path_t *dp, *parent = NULL, *child = NULL;	//设备路径

    /* EDK2 UEFI PXE驱动程序将IPv4和IPv6消息设备创建为主MAC消息设备的子设备。
       我们只需要每个物理卡一个绑定SNP的设备，否则它们在轮询传入数据包时会相互竞争。*/
    printf_debug ("*handle=%x\n",*handle);//fcb9798
		dp = grub_efi_get_device_path (*handle);	//设备路径
		if (!dp)	//失败
			continue;
    if (debug > 1)
      grub_efi_print_device_path(dp); //打印路径

    //尝试打开pxe协议
    if (!pxe)
    {
      pxe = grub_efi_open_protocol (*handle, &pxe_io_guid,  //VM在MAC，QEMU在MAC/IPV4
          GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);  
      if (pxe)	//失败
      {
        pxe_entry = pxe;
        pd_handle = *handle;
        pd_dp = dp;
        printf_debug ("pxe_entry=%x\n",pxe_entry);
      }
    }

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

    //通过句柄打开网络协议.
		net = grub_efi_open_protocol (*handle, &net_io_guid,  //VM及QEMU都在MAC
				GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
		if (! net)	//失败
			continue;

    //启动并初始化网络设备
    printf_debug ("net_start_state=%x\n",net->mode->state);  //0/1/2=网络停止/网络起动/已初始化         2
		if (net->mode->state == GRUB_EFI_NETWORK_STOPPED					//如果网络停止
				&& efi_call_1 (net->start, net) != GRUB_EFI_SUCCESS)	//则启动网络
			continue;                                               //启动失败,继续

		if (net->mode->state == GRUB_EFI_NETWORK_STOPPED)					//如果网络停止,继续
			continue;

		if (net->mode->state == GRUB_EFI_NETWORK_STARTED					//如果网络起动了
				&& efi_call_3 (net->initialize, net, 0, 0) != GRUB_EFI_SUCCESS)	//则网络初始化失败
			continue;                                              //如果初始化失败,继续
    printf_debug ("net_set_state=%x\n",net->mode->state);  //0/1/2=网络停止/网络起动/已初始化         2
    printf_debug ("max_packet_size=%x\n",net->mode->max_packet_size);//5dc
    max_packet_size = net->mode->max_packet_size;
    net0 = net;
	}
  grub_free (handles);	//释放
  
#if 0
  //通过启动句柄打开pxe协议
  pxe_entry = grub_efi_open_protocol (image->device_handle, &pxe_io_guid,
				  GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
#endif
  if (! pxe_entry)	//失败
    return 1;

  if (!*(grub_u32_t *)&pxe_entry->mode->station_ip.v4)   //非网起时，开启网络功能
  {
    printf ("Running DHCP request IP, please wait ......\n");
    grub_efi_status_t status;
    status = efi_call_2 (pxe_entry->start,  //启动pxe
	    pxe_entry, 0);
    
    status = efi_call_2 (pxe_entry->dhcp,   //尝试完成DHCPv4(发现/提供/请求/确认)
	    pxe_entry, 0);
    if (status == GRUB_EFI_SUCCESS)
      printf_debug ("dhcp ok\n");
    else
      printf_debug ("dhcp failure\n");
  }

  // ===================== PROXY DHCP MODIFICATION START =====================
  //由江南一根葱提供
  //从引导播放器获取IP地址
  struct grub_efi_pxe_mode *pxe_mode = pxe_entry->mode;	//模式
  //添加循环以等待PXE回复
  int i;
  for (i = 0; i < 5; i++) //尝试5次
  {
    if (pxe_entry->mode->pxe_reply_received)  //pxe收到回复
      break;
    efi_call_1(grub_efi_system_table->boot_services->stall, 1000000); //延时1秒
  }

  //从引导播放器获取IP地址
  //首先，将 discover_reply 指向 dhcp_ack，这是基础信息源
  discover_reply = (grub_efi_pxe_dhcpv4_packet_t *)((char *)&pxe_entry->mode->dhcp_ack.dhcpv4);	//引导播放器
  image = grub_efi_get_loaded_image (grub_efi_image_handle);  //通过映像句柄,获得加载映像grub_efi_loaded_image结构
  g4e_options = image->load_options;
  g4e_options_size = image->load_options_size;

  //始终使用EFI PXE协议栈确定的客户端IP
  pxe_yip = *(grub_u32_t *)&pxe_entry->mode->station_ip.v4;    //站IP
  station_ip = *(int*)pxe_entry->mode->station_ip.v4;   //站IP
  subnet_mask = *(int*)pxe_entry->mode->subnet_mask.v4; //子网掩码
  grub_memmove (&pxe_mac, &discover_reply->bootp_hw_addr, 6);   //MAC

  //添加调试信息
  printf_debug("DHCP ACK received: %u\n", pxe_mode->dhcp_ack_received);       //收到dhcp ack
  printf_debug("Proxy Offer received: %u\n", pxe_mode->proxy_offer_received); //收到代理报文

  // <<< 这是实现 Proxy DHCP 支持的核心逻辑 >>>
  //检查是否收到了 Proxy Offer，并且主 DHCP ACK 中没有提供启动文件
  pxe_sip = 0;
  if (pxe_entry->mode->proxy_offer_received && discover_reply->bootp_boot_file[0] == '\0')
  {
    //从Proxy Offer中复制启动文件名。
    grub_memcpy(discover_reply->bootp_boot_file, pxe_entry->mode->proxy_offer.dhcpv4.bootp_boot_file, sizeof(discover_reply->bootp_boot_file)); //代理提供
    
    //从Proxy Offer中获取服务器IP。
    pxe_sip = pxe_entry->mode->proxy_offer.dhcpv4.bootp_si_addr; 
    printf_debug("Use proxy offer information. %x,(%x)\n",pxe_sip,discover_reply->bootp_si_addr);
  }
  else if (g4e_options_size)
  {
    //如果从ipxe引导，使用其传入的服务器IP
    unsigned char a0,a1,a2,a3;
    unsigned long long val = 0;
    char *utf8 = grub_malloc (g4e_options_size);
    char *p = utf8;
    unicode_to_utf8 ((unsigned short *)g4e_options, (unsigned char *)utf8, g4e_options_size);

    for (;*p;)
    {
      if (grub_memcmp ((const char *)p, "ipxe=", 5) == 0)  
      {
        ipxe = 1;
        p += 5;
        if (safe_parse_maxint (&p, &val))
          a0 = val;
        else
          break;
        p++;
        safe_parse_maxint (&p, &val);
        a1 = val;
        p++;
        safe_parse_maxint (&p, &val);
        a2 = val;
        p++;
        safe_parse_maxint (&p, &val);
        a3 = val;
        pxe_sip = a0 + (a1 << 8) + (a2 << 16) + (a3 << 24);
        printf_debug("Use g4e_options information. %x \n",pxe_sip);
        break;
      }
      p = skip_to (0, p);
    }
    grub_free (utf8);
  }

  if (pxe_sip == 0)
  {
    //如果没有 Proxy Offer，或者主 DHCP ACK 已经包含了启动信息，则直接使用 dhcp_ack 的信息
    pxe_sip = discover_reply->bootp_si_addr;
    printf_debug("Use standard DHCP ACK information. %x,(%x)\n",pxe_sip,pxe_entry->mode->proxy_offer.dhcpv4.bootp_si_addr);
  }

  //最后的 fallback：如果 pxe_sip 仍然是0，尝试从 DHCP 选项66 获取
  if (pxe_sip == 0)
  {
    printf_debug("Server IP is empty, try DHCP option 66...\n");
    grub_u32_t next_server_ip = get_dhcp_option_66(discover_reply);
    if (next_server_ip)
    {
      pxe_sip = next_server_ip;
      printf_debug("Use DHCP option 66 information.. %x\n",pxe_sip);
    }
    else
      return 0;
  }

  //显式调用 pxe_entry->set_station_ip 函数，将DHCP分配的客户端IP和子网掩码传递给EFI PXE协议栈
  grub_efi_status_t status = efi_call_3(pxe_entry->set_station_ip, pxe_entry,
                                        (grub_u32_t *)(grub_size_t)&station_ip,
                                        (grub_u32_t *)(grub_size_t)&subnet_mask);
  if (status != GRUB_EFI_SUCCESS)
    printf_errinfo("Failed to set the IP address for the PXE station: %x\n", (int)status);

  // ===================== PROXY DHCP MODIFICATION END =====================

  bootfile = discover_reply->bootp_boot_file;
  grub_sprintf (default_server,  "%d.%d.%d.%d",
	     ((grub_uint8_t *) &pxe_sip)[0],
	     ((grub_uint8_t *) &pxe_sip)[1],
	     ((grub_uint8_t *) &pxe_sip)[2],
	     ((grub_uint8_t *) &pxe_sip)[3]);
  // 打印最终结果用于调试
  if (debug > 1)
  {
    printf("YIP : ");
    print_ip (pxe_yip) ;
    printf("\nSIP : ");
    print_ip (pxe_sip);
    printf("\n");
    printf("default_server=%s\n",default_server);
    printf("bootfile: %s\n",bootfile);
    getkey();
  }

  return 0;
 }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void print_ip (grub_u32_t ip)
{
  int i;

  for (i = 0; i < 3; i++)
    {
      grub_printf ("%d.", (unsigned long)(unsigned char)ip);
      ip >>= 8;
    }
  grub_printf ("%d", (unsigned long)(unsigned char)ip);
}

int pxe_func (char *arg, int flags);
int
pxe_func (char *arg, int flags)
{
  if (grub_memcmp(arg, "init", 4) == 0)
  {
    pxe_init ();
    if (!only_tftp)
      cur_pxe_type = 1;
    saved_drive = PXE_DRIVE;
    current_drive = PXE_DRIVE;
    saved_partition = 0xFFFFFF;
    current_partition = 0xFFFFFF;

    return 1;
  }

  if (! pxe_entry)
    return 0;

  if (*arg == 0)  //获得PXE信息
  {
    char buf[4], *pc;
    int i;
    grub_efi_pxe_t *pxe = pxe_entry;
    grub_efi_pxe_mode_t *mode = pxe->mode;
    grub_printf ("client ip\n");  //这一行不能打印，不知道为什么？
    grub_printf ("client ip     : ");
    print_ip (pxe_yip);
    grub_printf ("\nserver ip     : ");
    print_ip (pxe_sip);
    grub_printf ("\npacket_size   : %d",max_packet_size);
    grub_printf ("\nmac           : ");
    for (i = 0; i < 6; i++)
    {
      pc = buf;
      pc = pxe_outhex (pc, pxe_mac[i]);
      *pc = 0;
      grub_printf ("%s%c", buf, ((i == 5) ? ' ' : '-'));
    }
    grub_printf ("\nbootfile      : %s",bootfile);
    grub_printf ("\npxe_type      : ");
    if (cur_pxe_type)
      grub_printf ("http");
    else
      grub_printf ("tftp");
    grub_printf ("\nhttp_type     : %d\n",(http_feature ? 206 : 200));
    
    grub_printf ("mode->started : %d\n",mode->started);     //pxe状态 0/1=停止/已启动
    grub_printf ("net0->state   : %d\n",net0->mode->state); //net状态 0/1/2=停止/已启动/已初始化

    return 1;
  }
  else if (grub_memcmp(arg, "read", 4) == 0)  //读取文件
  {
/*
用法：pxe read /path/file range_start - range_end
例1： pxe read /boot/10pe.wim 64 - 83    //从第64字节开始读，至第83字节止，共读20字节。(文件从0字节开始)
例2： pxe read /boot/10pe.wim - 8        //从文件末尾读8字节。
例3： pxe read /boot/10pe.wim 64 -       //从文件第64字节开始读至文件结束。
*/
    char *p = pxe_name;
    char tmp[32] = {0};
    char range[64] = {0};
    http_range = range;
    grub_u64_t range_start = 0, range_end = 0;
    
    char *buf = grub_zalloc (256);  //分配内存, 并清零;

    arg = skip_to (0, arg);
    while (*arg != ' ' && *arg != 0)
      *p++ = *arg++;
    *p = 0;

    //获取文件尺寸
    pxe_open (pxe_name);
    printf_debug ("filemax=%x\n",filemax);

    arg = skip_to (0, arg);
    if (*arg == 0)
    {
      grub_sprintf (range, "bytes=0-");
    }
    else if (*arg != '-')
    {
      safe_parse_maxint (&arg, &range_start);
      arg = skip_to (0, arg);
      if (*arg == '-')
      {
        arg = skip_to (0, arg);
        if (safe_parse_maxint (&arg, &range_end)) //例1
          grub_sprintf (range, "bytes=%d-%d", range_start,range_end);
        else                                      //例3
          grub_sprintf (range, "bytes=%d-", range_start);
      }
      else
        return 0;
    }
    else                                          //例2
    {
      arg = skip_to (0, arg);
      if (safe_parse_maxint (&arg, &range_end))
        grub_sprintf (range, "bytes=-%d", range_end);
      else
       return 0; 
    }

    pxe_read ((unsigned long long)(grub_size_t)buf, 256, GRUB_READ);

    grub_sprintf (tmp, "echo --mem=%d=%d", buf, 256);
    run_line (tmp,flags);
    http_range = 0;
    grub_free (buf);
    return 1;
  }
  return 0;
}

int pxe_init (void);
int
pxe_init (void)
{
//  debug = 3;
  int err;
  
  printf_debug ("UEFI revision: %x\n",grub_efi_system_table->hdr.revision);

  err = grub_efinet_findcards ();		//查找支持简单网络接口的卡  初始化tftp
  if (err)
    return 1;
  err = grub_efi_net_find_cards (); //查找支持ip4配置2的卡  初始化http
  if (err)
  {
    only_tftp = 1;
    printf_debug ("only_tftp!\n");
  }
  else
  {
    //测试是否支持断点续传
    http_feature = 0;
    cur_pxe_type = 1;
    http_range = "bytes=0-";
    pxe_name = "/efi/grub/menu.lst";
    pxe_open (pxe_name);
    char *buf = grub_zalloc (filemax);
    http_configure();     //http配置
    http_read (buf, filemax);
    if (read_status == 206)
      http_feature = 1;  //支持断点续传
    grub_free (buf);
    http_range = 0;
    if (ipxe)  //ipxe引导g4e，损坏了tftp,只能使用http。
      cur_pxe_type = 1;
  }
  
  if (!ipxe)
  {
    cur_pxe_type = 0;   //默认网起使用tftp。即'/'使用tftp。如果使用http，必需指明，即(http)/。或者使用 "set http"。
    pxe_configure ();   //pxe配置  只要不以外关闭，设置一次即可。
  }

  return 0;
}

#endif	//ifdef FSYS_PXE
