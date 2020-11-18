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

#ifndef __PXE_H
#define __PXE_H

#define PXENV_TFTP_OPEN			0x0020
#define PXENV_TFTP_CLOSE		0x0021
#define PXENV_TFTP_READ			0x0022
#define PXENV_TFTP_READ_FILE		0x0023
#define PXENV_TFTP_READ_FILE_PMODE	0x0024
#define PXENV_TFTP_GET_FSIZE		0x0025

#define PXENV_UDP_OPEN			0x0030
#define PXENV_UDP_CLOSE			0x0031
#define PXENV_UDP_READ			0x0032
#define PXENV_UDP_WRITE			0x0033

#define PXENV_START_UNDI		0x0000
#define PXENV_UNDI_STARTUP		0x0001
#define PXENV_UNDI_CLEANUP		0x0002
#define PXENV_UNDI_INITIALIZE		0x0003
#define PXENV_UNDI_RESET_NIC		0x0004
#define PXENV_UNDI_SHUTDOWN		0x0005
#define PXENV_UNDI_OPEN			0x0006
#define PXENV_UNDI_CLOSE		0x0007
#define PXENV_UNDI_TRANSMIT		0x0008
#define PXENV_UNDI_SET_MCAST_ADDR	0x0009
#define PXENV_UNDI_SET_STATION_ADDR	0x000A
#define PXENV_UNDI_SET_PACKET_FILTER	0x000B
#define PXENV_UNDI_GET_INFORMATION	0x000C
#define PXENV_UNDI_GET_STATISTICS	0x000D
#define PXENV_UNDI_CLEAR_STATISTICS	0x000E
#define PXENV_UNDI_INITIATE_DIAGS	0x000F
#define PXENV_UNDI_FORCE_INTERRUPT	0x0010
#define PXENV_UNDI_GET_MCAST_ADDR	0x0011
#define PXENV_UNDI_GET_NIC_TYPE		0x0012
#define PXENV_UNDI_GET_IFACE_INFO	0x0013
#define PXENV_UNDI_ISR			0x0014
#define	PXENV_STOP_UNDI			0x0015	// Overlap...?
#define PXENV_UNDI_GET_STATE		0x0015	// Overlap...?

#define PXENV_UNLOAD_STACK		0x0070
#define PXENV_GET_CACHED_INFO		0x0071
#define PXENV_RESTART_DHCP		0x0072
#define PXENV_RESTART_TFTP		0x0073
#define PXENV_MODE_SWITCH		0x0074
#define PXENV_START_BASE		0x0075
#define PXENV_STOP_BASE			0x0076

#define PXENV_EXIT_SUCCESS 0x0000
#define PXENV_EXIT_FAILURE 0x0001

#define PXENV_STATUS_SUCCESS 0x00
#define PXENV_STATUS_FAILURE 0x01
#define PXENV_STATUS_BAD_FUNC 0x02
#define PXENV_STATUS_UNSUPPORTED 0x03
#define PXENV_STATUS_KEEP_UNDI 0x04
#define PXENV_STATUS_KEEP_ALL 0x05
#define PXENV_STATUS_OUT_OF_RESOURCES 0x06
#define PXENV_STATUS_ARP_TIMEOUT 0x11
#define PXENV_STATUS_UDP_CLOSED 0x18
#define PXENV_STATUS_UDP_OPEN 0x19
#define PXENV_STATUS_TFTP_CLOSED 0x1A
#define PXENV_STATUS_TFTP_OPEN 0x1B
#define PXENV_STATUS_MCOPY_PROBLEM 0x20
#define PXENV_STATUS_BIS_INTEGRITY_FAILURE 0x21
#define PXENV_STATUS_BIS_VALIDATE_FAILURE 0x22
#define PXENV_STATUS_BIS_INIT_FAILURE 0x23
#define PXENV_STATUS_BIS_SHUTDOWN_FAILURE 0x24
#define PXENV_STATUS_BIS_GBOA_FAILURE 0x25
#define PXENV_STATUS_BIS_FREE_FAILURE 0x26
#define PXENV_STATUS_BIS_GSI_FAILURE 0x27
#define PXENV_STATUS_BIS_BAD_CKSUM 0x28
#define PXENV_STATUS_TFTP_CANNOT_ARP_ADDRESS 0x30
#define PXENV_STATUS_TFTP_OPEN_TIMEOUT	0x32

#define PXENV_STATUS_TFTP_UNKNOWN_OPCODE 0x33
#define PXENV_STATUS_TFTP_READ_TIMEOUT 0x35
#define PXENV_STATUS_TFTP_ERROR_OPCODE 0x36
#define PXENV_STATUS_TFTP_CANNOT_OPEN_CONNECTION 0x38
#define PXENV_STATUS_TFTP_CANNOT_READ_FROM_CONNECTION 0x39
#define PXENV_STATUS_TFTP_TOO_MANY_PACKAGES 0x3A
#define PXENV_STATUS_TFTP_FILE_NOT_FOUND 0x3B
#define PXENV_STATUS_TFTP_ACCESS_VIOLATION 0x3C
#define PXENV_STATUS_TFTP_NO_MCAST_ADDRESS 0x3D
#define PXENV_STATUS_TFTP_NO_FILESIZE 0x3E
#define PXENV_STATUS_TFTP_INVALID_PACKET_SIZE 0x3F
#define PXENV_STATUS_DHCP_TIMEOUT 0x51
#define PXENV_STATUS_DHCP_NO_IP_ADDRESS 0x52
#define PXENV_STATUS_DHCP_NO_BOOTFILE_NAME 0x53
#define PXENV_STATUS_DHCP_BAD_IP_ADDRESS 0x54
#define PXENV_STATUS_UNDI_INVALID_FUNCTION 0x60
#define PXENV_STATUS_UNDI_MEDIATEST_FAILED 0x61
#define PXENV_STATUS_UNDI_CANNOT_INIT_NIC_FOR_MCAST 0x62
#define PXENV_STATUS_UNDI_CANNOT_INITIALIZE_NIC 0x63
#define PXENV_STATUS_UNDI_CANNOT_INITIALIZE_PHY 0x64
#define PXENV_STATUS_UNDI_CANNOT_READ_CONFIG_DATA 0x65
#define PXENV_STATUS_UNDI_CANNOT_READ_INIT_DATA 0x66
#define PXENV_STATUS_UNDI_BAD_MAC_ADDRESS 0x67
#define PXENV_STATUS_UNDI_BAD_EEPROM_CHECKSUM 0x68
#define PXENV_STATUS_UNDI_ERROR_SETTING_ISR 0x69
#define PXENV_STATUS_UNDI_INVALID_STATE 0x6A
#define PXENV_STATUS_UNDI_TRANSMIT_ERROR 0x6B
#define PXENV_STATUS_UNDI_INVALID_PARAMETER 0x6C
#define PXENV_STATUS_BSTRAP_PROMPT_MENU 0x74
#define PXENV_STATUS_BSTRAP_MCAST_ADDR 0x76
#define PXENV_STATUS_BSTRAP_MISSING_LIST 0x77
#define PXENV_STATUS_BSTRAP_NO_RESPONSE 0x78
#define PXENV_STATUS_BSTRAP_FILE_TOO_BIG 0x79
#define PXENV_STATUS_BINL_CANCELED_BY_KEYSTROKE 0xA0
#define PXENV_STATUS_BINL_NO_PXE_SERVER 0xA1
#define PXENV_STATUS_NOT_AVAILABLE_IN_PMODE 0xA2
#define PXENV_STATUS_NOT_AVAILABLE_IN_RMODE 0xA3
#define PXENV_STATUS_BUSD_DEVICE_NOT_SUPPORTED 0xB0
#define PXENV_STATUS_LOADER_NO_FREE_BASE_MEMORY 0xC0
#define PXENV_STATUS_LOADER_NO_BC_ROMID 0xC1
#define PXENV_STATUS_LOADER_BAD_BC_ROMID 0xC2
#define PXENV_STATUS_LOADER_BAD_BC_RUNTIME_IMAGE 0xC3
#define PXENV_STATUS_LOADER_NO_UNDI_ROMID 0xC4
#define PXENV_STATUS_LOADER_BAD_UNDI_ROMID 0xC5
#define PXENV_STATUS_LOADER_BAD_UNDI_DRIVER_IMAGE 0xC6
#define PXENV_STATUS_LOADER_NO_PXE_STRUCT 0xC8
#define PXENV_STATUS_LOADER_NO_PXENV_STRUCT 0xC9
#define PXENV_STATUS_LOADER_UNDI_START 0xCA
#define PXENV_STATUS_LOADER_BC_START 0xCB

#define PACKED		__attribute__ ((packed))

#define SEGMENT(x)	((x) >> 4)													//分割		123456	->	12345
#define OFFSET(x)	((x) & 0xF)														//偏移		123456	->	6
#define SEGOFS(x)	((SEGMENT(x)<<16)+OFFSET(x))					//段偏移	123450000 + 6 = 123450006
#define LINEAR(x)	(void*)(((x >> 16) <<4)+(x & 0xFFFF))	//线性		123450006	-> 123450 + 6 = 123456

//#define PXE_ERR_LEN	0xFFFFFFFF
#define PXE_ERR_LEN	0

typedef grub_u16_t		PXENV_STATUS;
typedef grub_u32_t		SEGOFS16;
typedef grub_u32_t		IP4;
typedef grub_u16_t		UDP_PORT;

#define MAC_ADDR_LEN	16										//MAC地址尺寸
typedef grub_u8_t		MAC_ADDR[MAC_ADDR_LEN];	//MAC地址

#define PXENV_PACKET_TYPE_DHCP_DISCOVER	1		//包类型  DHCP发现
#define PXENV_PACKET_TYPE_DHCP_ACK			2		//包类型  DHCP确认
#define PXENV_PACKET_TYPE_CACHED_REPLY	3		//包类型  回复

typedef struct {
  PXENV_STATUS	Status;			//状态					grub_uint32_t state
  grub_u16_t	PacketType;		//数据包类型 
  grub_u16_t	BufferSize;		//缓存尺寸			grub_uint32_t nvram_size
  SEGOFS16	Buffer;					//缓存			
  grub_u16_t	BufferLimit;	//缓冲限制			grub_uint32_t nvram_access_size
} PACKED PXENV_GET_CACHED_INFO_t;		//获取信息	pppp

#define BOOTP_REQ	1					//引导TP需求
#define BOOTP_REP	2					//引导TP

#define BOOTP_BCAST	0x8000	//引导

#if 1
#define BOOTP_DHCPVEND  1024    /* DHCP extended vendor field size DHCP扩展供应商字段大小*/
#else
#define BOOTP_DHCPVEND  312	/* DHCP standard vendor field size */
#endif

#ifndef	VM_RFC1048
#define	VM_RFC1048	0x63825363L
#endif




typedef enum grub_network_level_protocol_id 
{
  GRUB_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV,
  GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV4,
  GRUB_NET_NETWORK_LEVEL_PROTOCOL_IPV6
} grub_network_level_protocol_id_t;

typedef struct grub_net_network_level_netaddress
{
  grub_network_level_protocol_id_t type;
  union
  {
    struct {
      grub_uint32_t base;
      int masksize; 
    } ipv4;
    struct {
      grub_uint64_t base[2];
      int masksize; 
    } ipv6;
  };
} grub_net_network_level_netaddress_t;

typedef struct {
  grub_u8_t		opcode;		//操作码																												00
  grub_u8_t		Hardware;	/* hardware type 硬件类型*/																		//00
  grub_u8_t		Hardlen;	/* hardware addr len 硬件地址长度*/														//00
  grub_u8_t		Gatehops;	/* zero it 归零*/																							//00
  grub_u32_t	ident;		/* random number chosen by client 客户选择的随机数*/					//00 00 00 00
  grub_u16_t	seconds;	/* seconds since did initial bootstrap 自初始引导以来的秒数*/	//00 00
  grub_u16_t	Flags;		/* seconds since did initial bootstrap */											//00 00
  IP4		cip;		/* Client IP 客户IP*/																									//00 00 00 00
  IP4		yip;		/* Your IP 你的IP*/																										//02 01 06 00
  IP4		sip;		/* IP to use for next boot stage 服务器IP*/														//f3 da 2f 8d
  IP4		gip;		/* Relay IP ? 网关IP*/																								//00 00 80 00
  MAC_ADDR	CAddr;		/* Client hardware address 客户端硬件地址*/											//00 00 00 00 c0 a8 38 02 c0 a8 38 01 00 00 00 00
//	grub_u32_t CAddr[4];
  grub_u8_t		Sname[64];	/* Server's hostname (Optional) 服务器的主机名（可选）*/		//00 0c 29 8d cc d9 00 00 00 00 00 00 00 00 00 00
//	grub_u32_t Sname[8];																																						//PC-201311212111
  grub_u8_t		bootfile[128];	/* boot filename 引导文件名*/														//00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
//	grub_u32_t bootfile[16];																																										//bootia.EFI
  union {
    grub_u8_t	d[BOOTP_DHCPVEND];	/* raw array of vendor/dhcp options 供应商/ dhcp选项的原始数组*/
    struct {
      grub_u8_t	magic[4];	/* DHCP magic cookie DHCP魔术曲奇*/
      grub_u32_t	flags;		/* bootp flags/opcodes bootp标志/操作码*/
      grub_u8_t	pad[56];
    } v;
  } vendor;		//供应商
} PACKED BOOTPLAYER;		//引导播放器


#define GRUB_NET_BOOTP_MAC_ADDR_LEN	16

typedef grub_uint8_t grub_net_bootp_mac_addr_t[GRUB_NET_BOOTP_MAC_ADDR_LEN];

#define	GRUB_NET_BOOTP_RFC1048_MAGIC_0	0x63
#define	GRUB_NET_BOOTP_RFC1048_MAGIC_1	0x82
#define	GRUB_NET_BOOTP_RFC1048_MAGIC_2	0x53
#define	GRUB_NET_BOOTP_RFC1048_MAGIC_3	0x63

enum
  {
    GRUB_NET_BOOTP_PAD = 0x00,
    GRUB_NET_BOOTP_NETMASK = 0x01,
    GRUB_NET_BOOTP_ROUTER = 0x03,
    GRUB_NET_BOOTP_DNS = 0x06,
    GRUB_NET_BOOTP_HOSTNAME = 0x0c,
    GRUB_NET_BOOTP_DOMAIN = 0x0f,
    GRUB_NET_BOOTP_ROOT_PATH = 0x11,
    GRUB_NET_BOOTP_EXTENSIONS_PATH = 0x12,
    GRUB_NET_BOOTP_END = 0xff
  };


typedef struct {
  PXENV_STATUS	Status;			//状态
  IP4		ServerIPAddress;		//服务器的IP地址
  IP4		GatewayIPAddress;		//网关IP地址
  grub_u8_t	FileName[128];	//文件名
  UDP_PORT	TFTPPort;				//TFTP端口
  grub_u16_t	PacketSize;		//包尺寸
} PACKED PXENV_TFTP_OPEN_t;	//TFTP打开

typedef struct {
  PXENV_STATUS	Status;			//状态
} PACKED PXENV_TFTP_CLOSE_t;//TFTP关闭

typedef struct {
  PXENV_STATUS	Status;			//状态
  grub_u16_t	PacketNumber;	//包号
  grub_u16_t	BufferSize;		//缓存尺寸
  SEGOFS16	Buffer;					//缓存地址
} PACKED PXENV_TFTP_READ_t; //TFTP读

typedef struct {
  PXENV_STATUS	Status;			//状态
  IP4		ServerIPAddress;		//服务器的IP地址
  IP4		GatewayIPAddress;		//网关IP地址
  grub_u8_t	FileName[128];	//文件名
  grub_u32_t	FileSize;			//文件尺寸
} PACKED PXENV_TFTP_GET_FSIZE_t; //TFTP获得文件尺寸

typedef struct {
  PXENV_STATUS	Status;			//状态
  IP4		src_ip;							//源ip
} PACKED PXENV_UDP_OPEN_t;	//UDP 打开

typedef struct {
  PXENV_STATUS	Status;			//状态
} PACKED PXENV_UDP_CLOSE_t;	//UDP 关闭

typedef struct {
  PXENV_STATUS	Status;			//状态
  IP4		ip;									//IP4		ip
  IP4		gw;									//IP4		gw
  UDP_PORT	src_port;				//源端口
  UDP_PORT	dst_port;				//目的端口
  grub_u16_t	buffer_size;	//缓存尺寸
  SEGOFS16	buffer;					//缓存地址
} PACKED PXENV_UDP_WRITE_t;	 //UDP 写

typedef struct {
  PXENV_STATUS	Status;			//状态
  IP4		src_ip;							//源ip
  IP4		dst_ip;							//目的ip
  UDP_PORT	src_port;				//源端口
  UDP_PORT	dst_port;				//目的端口
  grub_u16_t	buffer_size;	//缓存尺寸
  SEGOFS16	buffer;					//缓存地址
} PACKED PXENV_UDP_READ_t;	//UDP 读

typedef struct {
  PXENV_STATUS	Status;			//状态
  grub_u8_t		reserved[10];	//保留
} PACKED PXENV_UNLOAD_STACK_t;		 //卸载堆栈 

typedef struct {
	int (*open)(const char *name);												//打开			tftp_open				ipxe_open
	grub_u32_t (*getsize)(void);													//获得尺寸	tftp_get_size		ipxe_get_size
	grub_u32_t (*readblk)(grub_u32_t buf,grub_u32_t num);	//读				tftp_read_blk		ipxe_read_blk
	void (*close)(void);																	//关闭 			tftp_close			ipxe_close
	void (*unload)(void);																	//卸载			tftp_unload			ipxe_unload
} s_PXE_FILE_FUNC;																			//pxe文件功能
extern s_PXE_FILE_FUNC *pxe_file_func[2];
extern unsigned int pxe_blksize;
#define PXE_FILE_TYPE_TFTP 0
#define PXE_FILE_TYPE_IPXE 1
/////////////////////////////////////////////////////////////////////////
extern struct grub_net_card *grub_net_cards;
#define FOR_NET_CARDS(var) for (var = grub_net_cards; var; var = var->next)
#define FOR_NET_CARDS_SAFE(var, next) for (var = grub_net_cards, next = (var ? var->next : 0); var; var = next, next = (var ? var->next : 0))
#define NETBUFF_ALIGN 2048
#define NETBUFFMINLEN 64
#define ALIGN_UP(addr, align) \
	((addr + (typeof (addr)) align - 1) & ~((typeof (addr)) align - 1))
#define ALIGN_UP_OVERHEAD(addr, align) ((-(addr)) & ((typeof (addr)) (align) - 1))
#define ALIGN_DOWN(addr, align) \
	((addr) & ~((typeof (addr)) align - 1))
#define ARRAY_SIZE(array) (sizeof (array) / sizeof (array[0]))
#define COMPILE_TIME_ASSERT(cond) switch (0) { case 1: case !(cond): ; }

typedef enum
{
  DNS_OPTION_IPV4,
  DNS_OPTION_IPV6,
  DNS_OPTION_PREFER_IPV4,
  DNS_OPTION_PREFER_IPV6
} grub_dns_option_t;

typedef enum grub_net_interface_flags
  {
    GRUB_NET_INTERFACE_HWADDRESS_IMMUTABLE = 1,	//网络接口硬件地址不可变
    GRUB_NET_INTERFACE_ADDRESS_IMMUTABLE = 2,		//网络接口地址不可变
    GRUB_NET_INTERFACE_PERMANENT = 4						//网络接口常驻
  } grub_net_interface_flags_t;		//网络接口标记

struct grub_env_var;
typedef const char *(*grub_env_read_hook_t) (struct grub_env_var *var, const char *val);
typedef char *(*grub_env_write_hook_t) (struct grub_env_var *var, const char *val);

struct grub_env_var
{
  char *name;
  char *value;
  grub_env_read_hook_t read_hook;
  grub_env_write_hook_t write_hook;
  struct grub_env_var *next;
  struct grub_env_var **prevp;
  struct grub_env_var *sorted_next;
  int global;
};

///////////////////////////////////////////////////////
#undef PACKED


#endif /* __PXE_H */
