/* common.c - miscellaneous shared variables and routines */
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
#include <iso9660.h>
#include "pxe.h"

/*
 *  Shared BIOS/boot data.
 */

char saved_dir[256];
unsigned int saved_mem_lower;

unsigned int extended_memory;	/* extended memory in KB */
extern int errorcheck;
int errorcheck = 1;
unsigned int grub_sleep (unsigned int seconds);
#if i386
int check_64bit_and_PAE  (void);
#endif
extern unsigned int prog_pid;

/*
 *  Error code stuff.
 */

char *err_list[] =
{
  [ERR_NONE] = 0,
  [ERR_BAD_ARGUMENT] = "Invalid argument",
  [ERR_BAD_FILENAME] = "Filename must be either an absolute pathname or blocklist",
  [ERR_BAD_FILETYPE] = "Bad file or directory type",
  [ERR_BAD_GZIP_DATA] = "Bad or corrupt data while decompressing file",
  [ERR_BAD_GZIP_HEADER] = "Bad or incompatible header in compressed file",
  [ERR_BAD_PART_TABLE] = "Partition table invalid or corrupt",
  [ERR_BAD_VERSION] = "Mismatched or corrupt version of stage1/stage2",
  [ERR_BELOW_1MB] = "Loading below 1MB is not supported",
  [ERR_BOOT_COMMAND] = "Kernel must be loaded before booting",
  [ERR_BOOT_FAILURE] = "Unknown boot failure",
  [ERR_BOOT_FEATURES] = "Unsupported Multiboot features requested",
  [ERR_DEV_FORMAT] = "Unrecognized device string, or you omitted the required DEVICE part which should lead the filename.",
  [ERR_DEV_NEED_INIT] = "Device not initialized yet",
  [ERR_DEV_VALUES] = "Invalid device requested",
  [ERR_EXEC_FORMAT] = "Invalid or unsupported executable format",
  [ERR_FILELENGTH] = "Filesystem compatibility error, cannot read whole file",
  [ERR_FILENAME_FORMAT] = "The leading DEVICE of the filename to find must be stripped off,\n\tand DIR for set-root must begin in a slash(/).",
  [ERR_FILE_NOT_FOUND] = "File not found",
  [ERR_FSYS_CORRUPT] = "Inconsistent filesystem structure",
  [ERR_FSYS_MOUNT] = "Cannot mount selected partition",
  [ERR_GEOM] = "Selected cylinder exceeds maximum supported by BIOS",
  [ERR_HD_VOL_START_0] = "The BPB hidden_sectors should not be zero for a hard-disk partition boot sector",
  [ERR_IN_SITU_FLOPPY] = "Only hard drives could be mapped in situ.",
  [ERR_IN_SITU_MEM] = "Should not use --mem together with --in-situ.",
  [ERR_NEED_LX_KERNEL] = "Linux kernel must be loaded before initrd",
  [ERR_NEED_MB_KERNEL] = "Multiboot kernel must be loaded before modules",
  [ERR_NO_DISK] = "Selected disk does not exist",
  [ERR_NO_DISK_SPACE] = "No spare sectors on the disk",
  [ERR_NO_PART] = "No such partition",
  [ERR_NO_HEADS] = "The number of heads must be specified. The `--heads=0' option tells map to choose a value(but maybe unsuitable) for you",
  [ERR_NO_SECTORS] = "The number of sectors per track must be specified. The `--sectors-per-track=0' option tells map to choose a value(but maybe unsuitable) for you",
  [ERR_NON_CONTIGUOUS] = "File for drive emulation must be in one contiguous disk area",
	[ERR_MANY_FRAGMENTS] = "Too many fragments.",
  [ERR_NUMBER_OVERFLOW] = "Overflow while parsing number",
  [ERR_NUMBER_PARSING] = "Error while parsing number",
  [ERR_OUTSIDE_PART] = "Attempt to access block outside partition",
  [ERR_PRIVILEGED] = "Must be authenticated",
  [ERR_READ] = "Disk read error",
  [ERR_SYMLINK_LOOP] = "Too many symbolic links",
  [ERR_UNALIGNED] = "File is not sector aligned",
  [ERR_UNRECOGNIZED] = "Unrecognized command",
  [ERR_WONT_FIT] = "Selected item cannot fit into memory",
  [ERR_WRITE] = "Disk write error",
  [ERR_INT13_ON_HOOK] = "The int13 handler already on hook",
  [ERR_INT13_OFF_HOOK] = "The int13 handler not yet on hook",
  [ERR_NO_DRIVE_MAPPED] = "Refuse to hook int13 because of empty drive map table",
  [ERR_INVALID_HEADS] = "Invalid heads. Should be between 0 and 256(0 means auto)",
  [ERR_INVALID_SECTORS] = "Invalid sectors. Should be between 0 and 63(0 means auto)",
  [ERR_SPECIFY_GEOM] = "Should not specify geometry when mapping a whole drive or when emulating a hard disk with a logical partition",
  [ERR_EXTENDED_PARTITION] = "Extended partition table is invalid, or its CHS values conflict with the BPB in a logical partition",
  [ERR_DEL_MEM_DRIVE] = "You should delete other mem drive first, or use `--mem' option to force the deletion",
  [ERR_SPECIFY_MEM] = "Should not specify `--mem' when mapping a whole drive",
  [ERR_SPECIFY_RESTRICTION] = "Options --read-only, --fake-write and --unsafe-boot are mutually exclusive. Should not specify them repeatedly.",
  [ERR_INVALID_FLOPPIES] = "Invalid floppies. Should be between 0 and 2",
  [ERR_INVALID_HARDDRIVES] = "Invalid harddrives. Should be between 0 and 127",
  [ERR_INVALID_LOAD_SEGMENT] = "Invalid load segment. Should be between 0 and 0x9FFF",
  [ERR_INVALID_LOAD_OFFSET] = "Invalid load offset. Should be between 0 and 0xF800",
  [ERR_INVALID_LOAD_LENGTH] = "Invalid load length. Should be between 512 and 0xA0000",
  [ERR_INVALID_SKIP_LENGTH] = "Invalid skip length. Should be less than the file size",
  [ERR_INVALID_BOOT_CS] = "Invalid boot CS. Should be between 0 and 0xFFFF",
  [ERR_INVALID_BOOT_IP] = "Invalid boot IP. Should be between 0 and 0xFFFF",
  [ERR_INVALID_RAM_DRIVE] = "Invalid ram_drive. Should be between 0 and 254",
//  [ERR_INVALID_RD_BASE] = "Invalid rd_base. Should not be 0xffffffff",
//  [ERR_INVALID_RD_SIZE] = "Invalid rd_size. Should not be 0",
  [ERR_MD_BASE] = "When mapping whole mem device at a fixed location, you must specify --mem to a value > 0.",
  [ERR_RD_BASE] = "RD_BASE must be sector-aligned and non-zero for mapping at a fixed location",
  [ERR_DOS_BACKUP] = "GRUB was not booted from DOS, or the backup copy of DOS at physical\naddress 0x200000 is corrupt",
  [ERR_ENABLE_A20] = "Failed to turn on Gate A20!",
  [ERR_DISABLE_A20] = "Failed to turn off Gate A20!",
  [ERR_DEFAULT_FILE] = "Invalid DEFAULT file format. Please copy a valid DEFAULT file from the grub4dos release and try again. Also note that the DEFAULT file must be uncompressed.",
  [ERR_PARTITION_TABLE_FULL] = "Cannot use --in-situ because the partition table is full(i.e., all the 4 entries are in use).",
  [ERR_MD5_FORMAT] = "Unrecognized md5 string. You must create it using the MD5CRYPT command.",
  [ERR_WRITE_GZIP_FILE] = "Attempt to write a gzip file",
  [ERR_FUNC_CALL] = "Invalid function call",
  [ERR_INTERNAL_CHECK] = "Internal check failed. Please report this bug.",
  [ERR_KERNEL_WITH_PROGRAM] = "Kernel cannot load if there is an active process",
  [ERR_HALT] = "Halt failed.",
  [ERR_PARTITION_LOOP] = "Too many partitions.",
//  [ERR_WRITE_TO_NON_MEM_DRIVE] = "Only RAM drives can be written when running in a script",
  [ERR_NOT_ENOUGH_MEMORY] = "Not enough memory",
  [ERR_BAT_GOTO] = "The syntax of GOTO is incorrect.",
  [ERR_BAT_CALL] = "The syntax of CALL is incorrect.",
  [ERR_NO_VBE_BIOS] = "VBE not detected.",
  [ERR_BAD_VBE_SIGNATURE] = "VESA signature not present.",
  [ERR_LOW_VBE_VERSION] = "VBE version too old. Must be 2.0+.",
  [ERR_NO_VBE_MODES] = "No modes detected for 16/24/32 bits per pixel.",
  [ERR_SET_VBE_MODE] = "Set VBE mode failed.",
  [ERR_SET_VGA_MODE] = "Set VGA mode failed.",
  [ERR_LOAD_SPLASHIMAGE] = "Failed loading splashimage.",
  [ERR_UNIFONT_FORMAT] = "Wrong unifont format.",
//  [ERR_UNIFONT_RELOAD] = "Unifont already loaded.",
  [ERR_DIVISION_BY_ZERO] = "Division by zero",

};



unsigned int grub_sleep (unsigned int seconds);
unsigned int
grub_sleep (unsigned int seconds) //暂停(秒)
{
	unsigned long long j;
	int time1;
	int time2;

	/* Get current time.  */
	while ((time2 = getrtsecs ()) == 0xFF);

	for (j = 0; seconds; j++)
	{
	  if ((time1 = getrtsecs ()) != time2 && time1 != 0xFF)
	    {
		time2 = time1;
		seconds--;
		j = 0;
		continue;
	    }
	  if (j == 0x100000000ULL)
	    {
		seconds--;
		j = 0;
		continue;
	    }
	}

	return seconds;
}

#if i386
int check_64bit_and_PAE ()
{
    unsigned int has_cpuid_instruction;
    // check for CPUID instruction
    asm ( "pushfl; popl %%eax;"	// get original EFLAGS
	  "movl %%eax, %%edx;"
	  "xorl $(1<<21), %%eax;"
	  "pushl %%eax; popfl;"	// try flip bit 21 of EFLAGS
	  "pushfl; popl %%eax;"	// get the modified EFLAGS
	  "pushl %%edx; popfl;"	// restore original EFLAGS
	  "xorl %%edx, %0; shrl $21, %%eax; and $1, %%eax;"	// check for bit 21 difference
	: "=a"(has_cpuid_instruction) : : "%edx" );
    if (!has_cpuid_instruction)
	return 0;
	unsigned int *sig = grub_malloc (0x20);
    unsigned int maxfn,feature;
    int x=0;
    asm ("cpuid;"
		: "=a"(maxfn), "=b"(sig[4]), "=d"(sig[5]), "=c"(sig[6])
		: "0"(0x00000000)
		/* : "%ebx","%ecx","%edx" */);
    if (maxfn >= 0x00000001)
    {
	asm ("cpuid;" : "=d" (feature), "=c" (sig[3]) : "a" (1) : "%ebx");
	if (feature & (1<<6)) // PAE
	    x |= IS64BIT_PAE;
	sig[7] = feature;
    }
    asm ("cpuid;" : "=a"(maxfn): "0"(0x80000000) : "%ebx","%ecx","%edx");
    if (maxfn >= 0x80000001)
    {
	asm ("cpuid;" : "=d" (feature) : "a" (0x80000001) : "%ebx", "%ecx");
	if (feature & (1<<29)) // AMD64, EM64T/IA-32e/Intel64
	    x |= IS64BIT_AMD64;
    }

    /* Get vm_cpu_signature */
    unsigned int leaf = 0x40000000;

    asm volatile (
	"xchgl %%ebx,%1;"	// save EBX
	"xorl %%ebx,%%ebx;"	// clear EBX for VMMs
	"xorl %%ecx,%%ecx;"	// clear ECX for VMMs
	"xorl %%edx,%%edx;"	// clear EDX for VMMs
	"cpuid;"
	"xchgl %%ebx,%1"	// restore EBX, and put result into %1.
	: "=a" (leaf), "+r" (sig[0]), "=c" (sig[1]), "=d" (sig[2])
	: "0" (leaf));
	grub_free(sig);
    return x;
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////
//kern/efi/efi.c
/* The handle of GRUB itself. Filled in by the startup code. GRUB本身的处理。由启动代码填写。*/
grub_efi_handle_t grub_efi_image_handle;	//这是个句柄.

/* The pointer to a system table. Filled in by the startup code. 指向系统表的指针。由启动代码填写 */
grub_efi_system_table_t *grub_efi_system_table;

static grub_efi_guid_t console_control_guid = GRUB_EFI_CONSOLE_CONTROL_GUID;
static grub_efi_guid_t loaded_image_guid = GRUB_EFI_LOADED_IMAGE_GUID;
static grub_efi_guid_t device_path_guid = GRUB_EFI_DEVICE_PATH_GUID;

void *grub_efi_locate_protocol (grub_efi_guid_t *protocol, void *registration);
void *
grub_efi_locate_protocol (grub_efi_guid_t *protocol, void *registration)  //EFI定位协议
{
  void *interface;
  grub_efi_status_t status;

  status = efi_call_3 (grub_efi_system_table->boot_services->locate_protocol,
                       protocol, registration, &interface);
  if (status != GRUB_EFI_SUCCESS)
    return 0;

  return interface;
}

/* Return the array of handles which meet the requirement. If successful, 返回满足要求的句柄数组。
   the number of handles is stored in NUM_HANDLES. The array is allocated 如果成功，句柄的数量存储在NUM_HANDLES。数组是从堆中分配的。
   from the heap.  */
grub_efi_handle_t *grub_efi_locate_handle (grub_efi_locate_search_type_t search_type,	grub_efi_guid_t *protocol,
			void *search_key,	grub_efi_uintn_t *num_handles);
grub_efi_handle_t *
grub_efi_locate_handle (grub_efi_locate_search_type_t search_type,
			grub_efi_guid_t *protocol,
			void *search_key,
			grub_efi_uintn_t *num_handles)  //EFI定位句柄
{
  grub_efi_boot_services_t *b;	//引导协议
  grub_efi_status_t status;			//状态
  grub_efi_handle_t *buffer;		//缓存
  grub_efi_uintn_t buffer_size = 8 * sizeof (grub_efi_handle_t);	//缓存尺寸

  buffer = grub_malloc (buffer_size);	//分配内存
  if (! buffer)	//分配失败
    return 0;

  b = grub_efi_system_table->boot_services;	//引导协议
  status = efi_call_5 (b->locate_handle, search_type, protocol, search_key,
			     &buffer_size, buffer);
  if (status == GRUB_EFI_BUFFER_TOO_SMALL)	//如果缓存太小
	{
		grub_free (buffer);	//释放内存
		buffer = grub_malloc (buffer_size);	//又分配同样大小的内存? 或许是调用函数返回了需要的尺寸!!
		if (! buffer)
			return 0;

		status = efi_call_5 (b->locate_handle, search_type, protocol, search_key,
				 &buffer_size, buffer);
	}

  if (status != GRUB_EFI_SUCCESS)	//如果不成功
	{
		grub_free (buffer);	//释放内存
		return 0;
	}

  *num_handles = buffer_size / sizeof (grub_efi_handle_t);	//句柄数
  return buffer;	//返回句柄地址
}

void *grub_efi_open_protocol (grub_efi_handle_t handle,	grub_efi_guid_t *protocol,	grub_efi_uint32_t attributes);
void *
grub_efi_open_protocol (grub_efi_handle_t handle,
			grub_efi_guid_t *protocol,
			grub_efi_uint32_t attributes) //efi打开协议
{
  grub_efi_boot_services_t *b;
  grub_efi_status_t status;
  void *interface;

  b = grub_efi_system_table->boot_services;
  status = efi_call_6 (b->open_protocol, handle,
		       protocol,
		       &interface,
		       grub_efi_image_handle,
		       0,
		       attributes); //调用efi系统表->引导服务->打开协议

  if (status != GRUB_EFI_SUCCESS) //如果状态 != 成功
    return 0;                     //返回错误

  return interface; //成功返回接口
}

int grub_efi_set_text_mode (int on);
int 
grub_efi_set_text_mode (int on) //efi设置文本模式		ok
{
  grub_efi_console_control_protocol_t *c;
  grub_efi_screen_mode_t mode, new_mode;

  c = grub_efi_locate_protocol (&console_control_guid, 0);	
  if (! c)
    /* No console control protocol instance available, assume it is 没有控制台控制协议实例可用，假设它已经在文本模式中。
       already in text mode. */
    return 1;

  if (efi_call_4 (c->get_mode, c, &mode, 0, 0) != GRUB_EFI_SUCCESS)
    return 0;

  new_mode = on ? GRUB_EFI_SCREEN_TEXT : GRUB_EFI_SCREEN_GRAPHICS;
  if (mode != new_mode)
    if (efi_call_2 (c->set_mode, c, new_mode) != GRUB_EFI_SUCCESS)
      return 0;

  return 1;
}

void grub_efi_stall (grub_efi_uintn_t microseconds);
void 
grub_efi_stall (grub_efi_uintn_t microseconds)  //efi失速
{
  efi_call_1 (grub_efi_system_table->boot_services->stall, microseconds);
}

#if 0
grub_efi_loaded_image_t *grub_efi_get_loaded_image (grub_efi_handle_t image_handle);
grub_efi_loaded_image_t *
grub_efi_get_loaded_image (grub_efi_handle_t image_handle)  //获得加载映像
{
  return grub_efi_open_protocol (image_handle,
				 &loaded_image_guid,
				 GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);  //打开协议(映像句柄,guid,获得协议)
}
#endif

void grub_reboot (void);
void
grub_reboot (void)  //热重新启动
{
  grub_machine_fini ();
  efi_call_4 (grub_efi_system_table->runtime_services->reset_system,	//系统表->运行时服务->重置系统
              GRUB_EFI_RESET_WARM, GRUB_EFI_SUCCESS, 0, NULL);				//冷复位,成功 ,0,NULL
  for (;;) ;
}

void grub_halt (void);
void
grub_halt (void)  //关机
{
  efi_call_4 (grub_efi_system_table->runtime_services->reset_system,	//系统表->运行时服务->重置系统
              GRUB_EFI_RESET_SHUTDOWN, GRUB_EFI_SUCCESS, 0, NULL);				//冷复位,成功 ,0,NULL
  for (;;) ;
}

grub_efi_device_path_t *grub_efi_get_device_path (grub_efi_handle_t handle);
grub_efi_device_path_t *
grub_efi_get_device_path (grub_efi_handle_t handle) //efi获得设备路径
{
  return grub_efi_open_protocol (handle, &device_path_guid,
				 GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);  //打开协议 获得协议
}

/* Return the device path node right before the end node. 在结束节点之前返回设备路径节点 */
grub_efi_device_path_t *grub_efi_find_last_device_path (const grub_efi_device_path_t *dp);
grub_efi_device_path_t *
grub_efi_find_last_device_path (const grub_efi_device_path_t *dp) //efi查找最后设备路径
{
  grub_efi_device_path_t *next, *p;

  if (GRUB_EFI_END_ENTIRE_DEVICE_PATH (dp))	//结束整个设备路径		类型=7f,子类型=ff
    return 0;

  for (p = (grub_efi_device_path_t *) dp, next = GRUB_EFI_NEXT_DEVICE_PATH (p);
       ! GRUB_EFI_END_ENTIRE_DEVICE_PATH (next);
       p = next, next = GRUB_EFI_NEXT_DEVICE_PATH (next))//;	//下一个类型!=7f,子类型!=ff
       ;

  return p;
}

/* Duplicate a device path. 复制设备路径 */
grub_efi_device_path_t *grub_efi_duplicate_device_path (const grub_efi_device_path_t *dp);
grub_efi_device_path_t *
grub_efi_duplicate_device_path (const grub_efi_device_path_t *dp) //efi复制设备路径
{
  grub_efi_device_path_t *p;	//设备路径
  grub_size_t total_size = 0;	//总尺寸
	//计算总尺寸
  for (p = (grub_efi_device_path_t *) dp;
       ;
       p = GRUB_EFI_NEXT_DEVICE_PATH (p))			//设备路径存在
	{
		total_size += GRUB_EFI_DEVICE_PATH_LENGTH (p);	//总尺寸+设备路径尺寸
		if (GRUB_EFI_END_ENTIRE_DEVICE_PATH (p))	//如果是结束
			break;	//退出循环
	}

  p = grub_malloc (total_size);	//分配内存
  if (! p)	//如果失败
    return 0;

  grub_memcpy (p, dp, total_size);	//复制dp到p
  return p;	//返回内存地址
}

/* Print the chain of Device Path nodes. This is mainly for debugging. 打印设备路径节点的链。这主要是为了调试*/
void grub_efi_print_device_path (grub_efi_device_path_t *dp);
void 
grub_efi_print_device_path (grub_efi_device_path_t *dp) //efi打印设备路径
{
  unsigned short *str;
  grub_efi_device_to_text_protocol_t *DPTTP;  //设备路径到文本协议
  static grub_efi_guid_t dpttp_guid = GRUB_EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID;

  DPTTP = grub_efi_locate_protocol (&dpttp_guid, 0);  //EFI定位协议
  str = (unsigned short *)efi_call_3 (DPTTP->ConvertDevicePathToText,
                       dp, FALSE, FALSE);

  grub_putstr_utf16 (str);
  grub_putchar ('\n', 255);
}

/* Compare device paths.  */
int grub_efi_compare_device_paths (const grub_efi_device_path_t *dp1, const grub_efi_device_path_t *dp2);
int 
grub_efi_compare_device_paths (const grub_efi_device_path_t *dp1,
			       const grub_efi_device_path_t *dp2) //efi比较设备路径 	返回: 0/非0=成功/失败
{
  if (! dp1 || ! dp2)	//如果dp1或者dp2为零, 错误
    /* Return non-zero.  */
    return 1;

  while (1)
	{
		grub_efi_uint8_t type1, type2;
		grub_efi_uint8_t subtype1, subtype2;
		grub_efi_uint16_t len1, len2;
		int ret;

		type1 = GRUB_EFI_DEVICE_PATH_TYPE (dp1);
		type2 = GRUB_EFI_DEVICE_PATH_TYPE (dp2);

		if (type1 != type2)	//如果设备路径类型不同
			return (int) type2 - (int) type1;

		subtype1 = GRUB_EFI_DEVICE_PATH_SUBTYPE (dp1);
		subtype2 = GRUB_EFI_DEVICE_PATH_SUBTYPE (dp2);

		if (subtype1 != subtype2)	//如果设备路径子类型不同
			return (int) subtype1 - (int) subtype2;

		len1 = GRUB_EFI_DEVICE_PATH_LENGTH (dp1);
		len2 = GRUB_EFI_DEVICE_PATH_LENGTH (dp2);

		if (len1 != len2)	//如果设备路径尺寸不同
			return (int) len1 - (int) len2;

		ret = grub_memcmp ((const char *)dp1, (const char *)dp2, len1);	//比较数据
		if (ret != 0)	//如果数据不同
			return ret;

		if (GRUB_EFI_END_ENTIRE_DEVICE_PATH (dp1))	//如果是结束, 退出循环
			break;

		dp1 = (grub_efi_device_path_t *) ((char *) dp1 + len1);	//下一设备路径
		dp2 = (grub_efi_device_path_t *) ((char *) dp2 + len2);
	}

  return 0;
}
//-------------------------------------------------------------------------------------;
//kern/mm.c

/* Magic words.  魔术词 */
#define GRUB_MM_FREE_MAGIC	0x2d3c2808
#define GRUB_MM_ALLOC_MAGIC	0x6db08fa4

typedef struct grub_mm_header		//内存头
{
  struct grub_mm_header *next;	//下一个内存头
  grub_size_t size;							//尺寸
  grub_size_t magic;						//魔术
#if GRUB_CPU_SIZEOF_VOID_P == 4
  char padding[4];							//衬垫
#elif GRUB_CPU_SIZEOF_VOID_P == 8
  char padding[8];
#else
# error "unknown word size"
#endif
}
*grub_mm_header_t;	//32位: 0x10   64位: 0x20

#if GRUB_CPU_SIZEOF_VOID_P == 4
# define GRUB_MM_ALIGN_LOG2	4
#elif GRUB_CPU_SIZEOF_VOID_P == 8
# define GRUB_MM_ALIGN_LOG2	5
#endif

#define GRUB_MM_ALIGN	(1 << GRUB_MM_ALIGN_LOG2)	//0x10

typedef struct grub_mm_region		//内存区域 
{
  struct grub_mm_header *first;	//第一内存头
  struct grub_mm_region *next;	//下一个内存区域
  grub_size_t pre_size;					//预先调整尺寸
  grub_size_t size;							//尺寸
}
*grub_mm_region_t;	//0x10

#ifndef GRUB_MACHINE_EMU
extern grub_mm_region_t EXPORT_VAR (grub_mm_base);
#endif
grub_mm_region_t grub_mm_base;	//内存基址

/* Get a header from the pointer PTR, and set *P and *R to a pointer		从指针PTR获取头，并分别将*P和*R设置为指向头的指针和指向其区域的指针,
   to the header and a pointer to its region, respectively. PTR must		必须分配PTR。
   be allocated.  */
static void get_header_from_pointer (void *ptr, grub_mm_header_t *p, grub_mm_region_t *r);
static void
get_header_from_pointer (void *ptr, grub_mm_header_t *p, grub_mm_region_t *r)
{
  if ((grub_addr_t) ptr & (GRUB_MM_ALIGN - 1))
    printf_debug ("unaligned pointer %x", ptr);

  for (*r = grub_mm_base; *r; *r = (*r)->next)
    if ((grub_addr_t) ptr > (grub_addr_t) ((*r) + 1)
				&& (grub_addr_t) ptr <= (grub_addr_t) ((*r) + 1) + (*r)->size)
      break;

  if (! *r)
    printf_errinfo ("out of range pointer %x\n", ptr); //超出范围指针％x  

  *p = (grub_mm_header_t) ptr - 1;
  if ((*p)->magic == GRUB_MM_FREE_MAGIC)	//魔术	0x2d3c2808
    printf_debug ("double free at %x", *p);
  if ((*p)->magic != GRUB_MM_ALLOC_MAGIC)	//魔术	0x6db08fa4
    printf_debug ("alloc magic is broken at %x: %x", *p,(*p)->magic);   //eb985d0:0
}

/* Deallocate the pointer PTR.  释放指针PTR。 */
void grub_free (void *ptr);
void
grub_free (void *ptr)
{
  grub_mm_header_t p;
  grub_mm_region_t r;

  if (! ptr)
    return;

  get_header_from_pointer (ptr, &p, &r);
  if (r->first->magic == GRUB_MM_ALLOC_MAGIC)
	{
		p->magic = GRUB_MM_FREE_MAGIC;
		r->first = p->next = p;
	}
  else
	{
		grub_mm_header_t q, s;
		for (s = r->first, q = s->next; q <= p || q->next >= p; s = q, q = s->next)
		{
			if (q->magic != GRUB_MM_FREE_MAGIC)
				printf_debug ("free magic is broken at %x: 0x%x", q, q->magic);
			if (q <= q->next && (q > p || q->next < p))
				break;
		}

		p->magic = GRUB_MM_FREE_MAGIC;
		p->next = q->next;
		q->next = p;
		if (p->next + p->next->size == p)
		{
			p->magic = 0;

			p->next->size += p->size;
			q->next = p->next;
			p = p->next;
		}

		r->first = q;
		if (q == p + p->size)
		{
			q->magic = 0;
			p->size += q->size;
			if (q == s)
				s = p;
			s->next = p;
			q = s;
		}

		r->first = q;
	}
  ptr = 0;
}

/*
typedef struct grub_mm_header		//内存头        first      q             p
{                               //              1000000   cc00010       cc00010
  struct grub_mm_header *next;	//下一个内存头  -1        cc00010       cc00010
  grub_size_t size;							//尺寸          -1        1ffbfe-401    1ffbfe
  grub_size_t magic;						//魔术          -1        2d3c2808      2d3c2808
  char padding[4];							//衬垫
}
*/
/* Allocate the number of units N with the alignment ALIGN from the ring		从*FIRST开始，从缓冲区中分配对齐方式为ALIGN的N个单元。对齐必须是2的幂。
   buffer starting from *FIRST.  ALIGN must be a power of two. Both N and		N和ALIGN都以GRUB_MM_ALIGN为单位(32位系统是0x10)。
   ALIGN are in units of GRUB_MM_ALIGN.  Return a non-NULL if successful,		如果成功，则返回非空值，否则返回空值。
   otherwise return NULL.  */
static void *grub_real_malloc (grub_mm_header_t *first, grub_size_t n, grub_size_t align);
static void *
grub_real_malloc (grub_mm_header_t *first, grub_size_t n, grub_size_t align)	//真实分配内存(插槽起始内存头, 分配N个单元, 对齐方式)
{
  grub_mm_header_t p, q;	//内存头
  /* When everything is allocated side effect is that *first will have alloc	当所有东西都被分配时，副作用是*first会标记alloc magic，
     magic marked, meaning that there is no room in this region.  */					//这意味着这个区域没有空间。
  if ((*first)->magic == GRUB_MM_ALLOC_MAGIC)	//如果没有空间,错误		0x6db08fa4
    return 0;

  /* Try to search free slot for allocation in this memory region.  尝试在此内存区域中搜索空闲插槽以进行分配。*/
  for (q = *first, p = q->next; ; q = p, p = p->next)	//搜索空闲插槽		起始: q=插槽起始内存头, p=下一个;  循环: q=下一个, p=下下一个
	{
		grub_off_t extra;	//额外的  64位

		extra = ((grub_addr_t) (p + 1) >> GRUB_MM_ALIGN_LOG2) & (align - 1);	//额外的=(下下一个/0x10)&(对齐方式-1)

		if (extra)	//如果额外的存在
			extra = align - extra;	//额外的=对齐方式-额外的

		if (! p)	//如果下一个结束
			printf_debug ("null in the ring");	//环中的NULL 

		if (p->magic != GRUB_MM_FREE_MAGIC)	//如果魔术错误  2d3c2808
			printf_errinfo ("free magic is broken at %x: 0x%x", (grub_size_t)p, p->magic);	//自由魔法在%p被打破 

		if (p->size >= n + extra)	//如果下一个->尺寸 >= N+额外的
		{
			extra += (p->size - extra - n) & (~(align - 1));
			if (extra == 0 && p->size == n)
	    {
	      /* There is no special alignment requirement and memory block		没有特殊的对齐要求，内存块完全匹配。
	         is complete match.

	         1. Just mark memory block as allocated and remove it from		只需将内存块标记为已分配，并将其从空闲列表中删除。 
	            free list.

	         Result:																											结果： 
	         +---------------+ previous block's next																		下一个块的下一个
	         | alloc, size=n |          |																	分配,尺寸=n
	         +---------------+          v
	       */
	      q->next = p->next;
	    }
			else if (align == 1 || p->size == n + extra)
	    {
	      /* There might be alignment requirement, when taking it into		当考虑到内存块适合时，可能需要对齐。
	         account memory block fits in.

	         1. Allocate new area at end of memory block.									1. 在内存块末尾分配新区域。 
	         2. Reduce size of available blocks from original node.				2. 从原始节点减小可用块的尺寸。
	         3. Mark new area as allocated and "remove" it from free			3. 将新区域标记为已分配，并将其从释放列表中“删除”。 
	            list.

	         Result:																											结果：
	         +---------------+
	         | free, size-=n | next --+																		释放,尺寸-n		下一个
	         +---------------+        |
	         | alloc, size=n |        |																		分配,尺寸=n
	         +---------------+        v
	       */

	      p->size -= n; //dbffff-401=dbfbfe
	      p += p->size; //1000010+dbfbfe*10=ebfbff0	
	    }
			else if (extra == 0)
	    {
	      grub_mm_header_t r;
	      
	      r = p + extra + n;
	      r->magic = GRUB_MM_FREE_MAGIC;
	      r->size = p->size - extra - n;
	      r->next = p->next;
	      q->next = r;
	      if (q == p)
				{
					q = r;
					r->next = r;
				}
			}
			else
	    {
	      /* There is alignment requirement and there is room in memory			内存块中有对齐要求和空间。
	         block.  Split memory block to three pieces.										把内存块分成三块。

	         1. Create new memory block right after section being						1. 在分配分区后立即创建新的内存块。
	            allocated.  Mark it as free.																		标记为释放。
	         2. Add new memory block to free chain.													2. 将新的内存块添加到空闲链。
	         3. Mark current memory block having only extra blocks.					3. 标记只有额外块的当前内存块。
	         4. Advance to aligned block and mark that as allocated and			4. 向对齐的块发送，并将其标记为已分配，然后将其从空闲列表中“删除”。 
	            "remove" it from free list.

	         Result:																												结果：
	         +------------------------------+
	         | free, size=extra             | next --+											释放,尺寸=extra
	         +------------------------------+        |
	         | alloc, size=n                |        |											分配,尺寸=n
	         +------------------------------+        |
	         | free, size=orig.size-extra-n | <------+, next --+						释放,尺寸=orig.size-extra-n
	         +------------------------------+                  v
	       */
	      grub_mm_header_t r;

	      r = p + extra + n;
	      r->magic = GRUB_MM_FREE_MAGIC;
	      r->size = p->size - extra - n;
	      r->next = p;

	      p->size = extra;
	      q->next = r;
	      p += extra;
	    }

			p->magic = GRUB_MM_ALLOC_MAGIC;
			p->size = n;
			/* Mark find as a start marker for next allocation to fasten it.			标记“查找”作为下次分配的开始标记，以固定它。 
				This will have side effect of fragmenting memory as small						这将产生副作用的碎片内存作为小块之前，这将是不使用。
				pieces before this will be un-used.  */
			/* So do it only for chunks under 64K.  所以只能对64K以下的块执行此操作。*/
			if (n < (0x8000 >> GRUB_MM_ALIGN_LOG2)	//如果20<800, 或者a8dd010=ebffdf0
					|| *first == p)
				*first = q;														//*first=a8dd010	下一循环的内存头
			return p + 1;	//ebffe00
		}//if (p->size >= n + extra)

		/* Search was completed without result.  搜索已完成，但没有结果。*/
		if (p == *first)
			break;
	}//for (q = *first, p = q->next; ; q = p, p = p->next)

  return 0;
}


/* Allocate SIZE bytes with the alignment ALIGN and return the pointer.  使用对齐方式分配内存字节并返回指针。*/
void * grub_memalign (grub_size_t align, grub_size_t size);
void *
grub_memalign (grub_size_t align, grub_size_t size)	//内存对齐(对齐,尺寸)
{
  grub_mm_region_t r;	//内存区域
  grub_size_t n = ((size + GRUB_MM_ALIGN - 1) >> GRUB_MM_ALIGN_LOG2) + 1;	//(size+0x10-1)/0x10	节对齐
  int count = 0;

  if (!grub_mm_base)	//如果内存基址=0
    goto fail;	//失败

  if (size > ~(grub_size_t) align)	//如果尺寸大于对齐  size > ~align
    goto fail;	//失败

  /* We currently assume at least a 32-bit grub_size_t,			我们目前假设至少有32位grub_size_t，
     so limiting allocations to <adress space size> - 1MiB	因此，以健全的名义将分配限制在"地址空间尺寸"-1MiB是有益的。 
     in name of sanity is beneficial. */
  if ((size + align) > ~(grub_size_t) 0x100000)	//如果(尺寸+对齐) > 0x100000
    goto fail;	//失败

  align = (align >> GRUB_MM_ALIGN_LOG2);	//对齐/0x10
  if (align == 0)
    align = 1;

again:
  for (r = grub_mm_base; r; r = r->next)	//从内存基址开始,直到遇到0结束
	{
		void *p;
		p = grub_real_malloc (&(r->first), n, align);
		if (p)
			return p;
	}

  /* If failed, increase free memory somehow.  如果失败，以某种方式增加可用内存。*/
  switch (count)
	{
    case 0:
      /* Invalidate disk caches. 使磁盘缓存无效  */
      count++;
      goto again;
    default:
      break;
	}

fail:
  printf_errinfo ("out of memory");	//内存不足
  return 0;
}

/* Allocate SIZE bytes and return the pointer.  */
void *grub_malloc (grub_size_t size);
void *
grub_malloc (grub_size_t size)	//分配内存尺寸字节,并返回指针。
{
  return grub_memalign (0, size);	//内存对齐(对齐,尺寸)
}

/* Allocate SIZE bytes, clear them and return the pointer.  分配SIZE字节，清除它们并返回指针*/
void *grub_zalloc (grub_size_t size);
void *
grub_zalloc (grub_size_t size)
{
  void *ret;

  ret = grub_memalign (0, size);
  if (ret)
    grub_memset (ret, 0, size);

  return ret;
}

/* Reallocate SIZE bytes and return the pointer. The contents will be		重新分配SIZE字节并返回指针。内容与PTR相同。
   the same as that of PTR.  */
void * grub_realloc (void *ptr, grub_size_t size);
void *
grub_realloc (void *ptr, grub_size_t size)
{
  grub_mm_header_t p;
  grub_mm_region_t r;
  void *q;
  grub_size_t n;

  if (! ptr)
    return grub_malloc (size);

  if (! size)
    {
      grub_free (ptr);
      return 0;
    }

  /* FIXME: Not optimal.  */
  n = ((size + GRUB_MM_ALIGN - 1) >> GRUB_MM_ALIGN_LOG2) + 1;
  get_header_from_pointer (ptr, &p, &r);

  if (p->size >= n)
    return ptr;

  q = grub_malloc (size);
  if (! q)
    return q;

  /* We've already checked that p->size < n.  */
  grub_memcpy (q, ptr, p->size << GRUB_MM_ALIGN_LOG2);
  grub_free (ptr);
  return q;
}


//-------------------------------------------------------------------------------------
//kern/efi/mm.c

#define ALIGN_UP(addr, align) \
	((addr + (typeof (addr)) align - 1) & ~((typeof (addr)) align - 1))
#define ALIGN_UP_OVERHEAD(addr, align) ((-(addr)) & ((typeof (addr)) (align) - 1))
#define ALIGN_DOWN(addr, align) \
	((addr) & ~((typeof (addr)) align - 1))
#define ARRAY_SIZE(array) (sizeof (array) / sizeof (array[0]))
#define COMPILE_TIME_ASSERT(cond) switch (0) { case 1: case !(cond): ; }

#define GRUB_EFI_PAGE_SHIFT             12
#define GRUB_EFI_PAGE_SIZE              (1 << GRUB_EFI_PAGE_SHIFT)
#define GRUB_EFI_BYTES_TO_PAGES(bytes)  (((bytes) + 0xfff) >> GRUB_EFI_PAGE_SHIFT)

#define GRUB_MMAP_REGISTER_BY_FIRMWARE  1

#define GRUB_EFI_MAX_USABLE_ADDRESS 0xffffffff	//64位不同

#define BYTES_TO_PAGES(bytes)	(((bytes) + 0xfff) >> 12) //字节转页  向上舍入
#define BYTES_TO_PAGES_DOWN(bytes)	((bytes) >> 12)     //字节转页  向下舍入
#define PAGES_TO_BYTES(pages)	((pages) << 12)           //页转字节

/* The size of a memory map obtained from the firmware. This must be  从固件获得的内存映射的大小。这必须是4kb的倍数.
   a multiplier of 4KB.  */
#define MEMORY_MAP_SIZE	0x3000  //内存映射尺寸

/* The minimum and maximum heap size for GRUB itself. GRUB自身的最小和最大堆尺寸 */
#define MIN_HEAP_SIZE	0x100000          //最小堆尺寸 1MB     100页
#define MAX_HEAP_SIZE	(1600 * 0x100000) //最大堆尺寸 1600MB  160000页

static void *finish_mmap_buf = 0;
static grub_efi_uintn_t finish_mmap_size = 0;
static grub_efi_uintn_t finish_key = 0;
static grub_efi_uintn_t finish_desc_size;
static grub_efi_uint32_t finish_desc_version;
int grub_efi_is_finished = 0;	//EFI完成

/*
 * We need to roll back EFI allocations on exit. Remember allocations that  我们需要在退出时备份EFI分配。记住我们将在退出时释放的分配。
 * we'll free on exit.
 */
struct efi_allocation;    //分配
struct efi_allocation {
	grub_efi_physical_address_t address;  //地址		8位
	grub_efi_uint64_t pages;              //页			8位
	struct efi_allocation *next;          //下一个	4位		0=结束符
};
static struct efi_allocation *efi_allocated_memory;	//0x14位		静态,地址不变

static void grub_efi_store_alloc (grub_efi_physical_address_t address, grub_efi_uintn_t pages);
static void 
grub_efi_store_alloc (grub_efi_physical_address_t address,
                         grub_efi_uintn_t pages)  //分配池(地址,页)  向"启动服务数据=12bafd24"写数据
{
  grub_efi_boot_services_t *b;  //引导服务
  struct efi_allocation *alloc; //分配				动态地址,变化
  grub_efi_status_t status;     //状态
  b = grub_efi_system_table->boot_services; //系统表->引导服务
  status = efi_call_3 (b->allocate_pool, GRUB_EFI_LOADER_DATA,  //装载数据            2
                           sizeof(*alloc), (void**)&alloc); //(分配池,存储器类型->装载数据,分配字节,返回分配地址}

  if (status == GRUB_EFI_SUCCESS) //如果分配成功
	{																			//插槽位置
		alloc->next = efi_allocated_memory; //下一个
		alloc->address = address;           //地址
		alloc->pages = pages;               //页
		efi_allocated_memory = alloc;       //当前
	}
  else  //失败
		printf_errinfo ("Could not malloc memory to remember EFI allocation. "
				"Exiting GRUB won't free all memory.\n");
}

static void grub_efi_drop_alloc (grub_efi_physical_address_t address, grub_efi_uintn_t pages);
static void 
grub_efi_drop_alloc (grub_efi_physical_address_t address,
                           grub_efi_uintn_t pages)  //释放池(地址,页)
{
  struct efi_allocation *ea, *eap;  //分配表
  grub_efi_boot_services_t *b;      //引导服务

  b = grub_efi_system_table->boot_services; //系统表->引导服务
  for (eap = NULL, ea = efi_allocated_memory; ea; eap = ea, ea = ea->next)
	{
		if (ea->address != address || ea->pages != pages)	//如果分配表地址不同, 或者分配表也不同
			continue;	//继续

		/* Remove the current entry from the list. 从列表中删除当前条目 */
		if (eap)
			eap->next = ea->next;
		else
			efi_allocated_memory = ea->next;	//第一次, 改变内存分配表
      /* Then free the memory backing it. 然后释放记忆支持它 */
		efi_call_1 (b->free_pool, ea);	//释放池(分配)

      /* And leave, we're done. 离开，我们完成 */
		break;
	}
}

//第一次分配页时,会多分配一页,用于记录分配结构. 似乎每一结构占用0x80字节(实际结构每一这么大).此页不会释放.
//分配页后,紧接分配池.并记录分配结构.
/* Allocate pages. Return the pointer to the first of allocated pages. 分配页面。返回指向第一个分配页面的指针 */
void *grub_efi_allocate_pages_real (grub_efi_physical_address_t address,
			      grub_efi_uintn_t pages,
			      grub_efi_allocate_type_t alloctype,
			      grub_efi_memory_type_t memtype);
void *
grub_efi_allocate_pages_real (grub_efi_physical_address_t address,
			      grub_efi_uintn_t pages,
			      grub_efi_allocate_type_t alloctype,
			      grub_efi_memory_type_t memtype) //真实分配页面(地址,页,分配类型,存储类型)
{
  grub_efi_status_t status;     //状态
  grub_efi_boot_services_t *b;  //引导服务
  /* Limit the memory access to less than 4GB for 32-bit platforms. 限制访问32位内存平台限制小于4GB的。 */
  if (address > GRUB_EFI_MAX_USABLE_ADDRESS)  //地址>最大可用地址4Gb
    return 0;	//退出

  b = grub_efi_system_table->boot_services; //系统表->引导服务
  status = efi_call_4 (b->allocate_pages, alloctype, memtype, pages, &address); //(分配页,分配类型,存储类型,页,返回分配地址)
  if (status != GRUB_EFI_SUCCESS) //如果失败
    return 0;	//退出

  if (address == 0) //如果地址=0		对于分配最大地址页,似乎又重复了一次!
	{
      /* Uggh, the address 0 was allocated... This is too annoying, 地址0被分配…这太烦人了，所以重新分配另一个。
	 so reallocate another one.  */
		address = GRUB_EFI_MAX_USABLE_ADDRESS;  //地址=最大可用地址4Gb
		status = efi_call_4 (b->allocate_pages, alloctype, memtype, pages, &address); //分配页,分配类型,存储类型,页,地址
		grub_efi_free_pages (0, pages); //释放页
		if (status != GRUB_EFI_SUCCESS) //如果失败
			return 0;
	}

  grub_efi_store_alloc (address, pages);  //分配池(地址,页)

  return (void *) ((grub_addr_t) address);//返回地址
}

void *grub_efi_allocate_any_pages (grub_efi_uintn_t pages);
void *
grub_efi_allocate_any_pages (grub_efi_uintn_t pages)  //分配最大地址页(请求页数)
{
  return grub_efi_allocate_pages_real (GRUB_EFI_MAX_USABLE_ADDRESS,  //地址=最大可用地址4Gb
				       pages, GRUB_EFI_ALLOCATE_MAX_ADDRESS,	//请求页数,分配类型->最大地址 1
				       GRUB_EFI_LOADER_DATA);	//存储器类型->装载数据	2
}

void *grub_efi_allocate_fixed (grub_efi_physical_address_t address, grub_efi_uintn_t pages);
void *
grub_efi_allocate_fixed (grub_efi_physical_address_t address,
			 grub_efi_uintn_t pages)  //分配固定页
{
  return grub_efi_allocate_pages_real (address, pages,	//地址=给定地址,请求页数,
				       GRUB_EFI_ALLOCATE_ADDRESS,	//分配类型->地址	2
				       GRUB_EFI_LOADER_DATA);	//	//存储器类型->装载数据	2
}

//首先释放页,然后释放池. 再更改分配记录. 
/* Free pages starting from ADDRESS. 从ADDRESS释放页 */
void grub_efi_free_pages (grub_efi_physical_address_t address, grub_efi_uintn_t pages);
void 
grub_efi_free_pages (grub_efi_physical_address_t address,
		     grub_efi_uintn_t pages) //释放页(地址,页)
{
  grub_efi_boot_services_t *b;

  b = grub_efi_system_table->boot_services;//系统表->引导服务
  efi_call_2 (b->free_pages, address, pages);	//(释放页,地址,页)

  grub_efi_drop_alloc (address, pages);	//释放池 
}

grub_err_t grub_efi_finish_boot_services (grub_efi_uintn_t *outbuf_size, void *outbuf,
			       grub_efi_uintn_t *map_key,
			       grub_efi_uintn_t *efi_desc_size,
			       grub_efi_uint32_t *efi_desc_version);
grub_err_t 
grub_efi_finish_boot_services (grub_efi_uintn_t *outbuf_size, void *outbuf,
			       grub_efi_uintn_t *map_key,
			       grub_efi_uintn_t *efi_desc_size,
			       grub_efi_uint32_t *efi_desc_version) //完成引导服务(输出缓存尺寸,输出缓存,映射键,描述尺寸,描述版本) 
{
  grub_efi_boot_services_t *b;
  grub_efi_status_t status;

  while (1)
	{
		if (grub_efi_get_memory_map (&finish_mmap_size, finish_mmap_buf, &finish_key,
				   &finish_desc_size, &finish_desc_version) < 0)	//获取EFI规范中定义的内存映射失败
			return printf_errinfo ("couldn't retrieve memory map\n");	//无法检索内存映射 

		if (outbuf && *outbuf_size < finish_mmap_size)
			return printf_errinfo ("memory map buffer is too small\n");	//内存映射缓冲区太小 

		finish_mmap_buf = grub_malloc (finish_mmap_size);	//分配内存

		if (grub_efi_get_memory_map (&finish_mmap_size, finish_mmap_buf, &finish_key,
				   &finish_desc_size, &finish_desc_version) <= 0)	//获取EFI规范中定义的内存映射失败
		{
			grub_free (finish_mmap_buf);	//释放
			return printf_errinfo ("couldn't retrieve memory map\n");	//无法检索内存映射 
		}

		b = grub_efi_system_table->boot_services;//系统表->引导服务
		status = efi_call_2 (b->exit_boot_services, grub_efi_image_handle,
			   finish_key);	//退出引导服务 
		if (status == GRUB_EFI_SUCCESS)	//如果成功, 退出
			break;

		if (status != GRUB_EFI_INVALID_PARAMETER)	//如果错误不是无效参数
		{
			grub_free (finish_mmap_buf);	//释放
			return printf_errinfo ("couldn't terminate EFI services\n");	//无法终止EFI服务
		}

		grub_free (finish_mmap_buf);	//释放
		printf_debug ("Trying to terminate EFI services again\n");	//再次尝试终止EFI服务
	}
  grub_efi_is_finished = 1;	//EFI完成
  if (outbuf_size)	//如果有输出缓存尺寸
    *outbuf_size = finish_mmap_size;
  if (outbuf)	//如果有输出缓存
    grub_memcpy (outbuf, finish_mmap_buf, finish_mmap_size);
  if (map_key)	//如果有映射键
    *map_key = finish_key;
  if (efi_desc_size)	//如果有描述尺寸
    *efi_desc_size = finish_desc_size;
  if (efi_desc_version)	//如果有描述版本
    *efi_desc_version = finish_desc_version;

  return GRUB_ERR_NONE;
}

#if 0
/*
 * To obtain the UEFI memory map, we must pass a buffer of sufficient size  要获得UEFI内存映射，我们必须通过足够大小的缓冲区来保存整个映射。
 * to hold the entire map. This function returns a sane start value for     这个函数返回理智的起始缓冲区尺寸值。
 * buffer size.
 */
grub_efi_uintn_t grub_efi_find_mmap_size (void);
grub_efi_uintn_t
grub_efi_find_mmap_size (void)  //查找内存映射尺寸
{
  grub_efi_uintn_t mmap_size = 0;
  grub_efi_uintn_t desc_size;

  if (grub_efi_get_memory_map (&mmap_size, NULL, NULL, &desc_size, 0) < 0) //获得内存映射(内存映射尺寸,内存映射地址,映射键,描述符尺寸,描述符版本)
    {
      printf_errinfo ("cannot get EFI memory map size\n");	//无法获取EFI内存映射尺寸
      return 0;
    }

  /*
   * Add an extra page, since UEFI can alter the memory map itself on 添加一个额外的页面，因为UEFI可以通过回调或显式调用来改变内存映射本身，包括控制台输出.
   * callbacks or explicit calls, including console output.
   */
  return ALIGN_UP (mmap_size + GRUB_EFI_PAGE_SIZE, GRUB_EFI_PAGE_SIZE);	//GRUB_EFI_PAGE_SIZE=1页=0x1000		向上舍入
}
#endif

/* Get the memory map as defined in the EFI spec. Return 1 if successful, 获取EFI规范中定义的内存映射。如果成功返回1，如果部分返回0, 如果错误返回-1.
   return 0 if partial, or return -1 if an error occurs.  */
int grub_efi_get_memory_map (grub_efi_uintn_t *memory_map_size, grub_efi_memory_descriptor_t *memory_map, grub_efi_uintn_t *map_key,
			 grub_efi_uintn_t *descriptor_size, grub_efi_uint32_t *descriptor_version);
int
grub_efi_get_memory_map (grub_efi_uintn_t *memory_map_size,	//指向缓冲区尺寸(以字节为单位)的指针。在输入时,这是调用方分配的缓冲区尺寸。在输出时,它是固件返回的缓冲区尺寸
			 grub_efi_memory_descriptor_t *memory_map,	//指向固件返回当前内存映射的缓冲区的指针。
			 grub_efi_uintn_t *map_key,									//指向固件返回当前内存映射键的位置的指针。
			 grub_efi_uintn_t *descriptor_size,					//指向固件返回单个EFI内存描述符尺寸（以字节为单位）的位置的指针。
			 grub_efi_uint32_t *descriptor_version) 		//指向固件返回与EFI内存描述符关联的版本号的位置的指针。
																							//获得内存映射(缓冲区尺寸指针,缓冲区指针,映射键指针,单个内存描述符尺寸指针,描述符版本指针)
{																							//返回: 0/1/-1="请求尺寸<完成尺寸"/"请求尺寸>=完成尺寸"/错误
  grub_efi_status_t status;
  grub_efi_boot_services_t *b;
  grub_efi_uintn_t key;
  grub_efi_uint32_t version;
  grub_efi_uintn_t size;

  if (grub_efi_is_finished) //如果EFI完成
	{
		int ret = 1;
		if (*memory_map_size < finish_mmap_size)  //如果请求尺寸<完成尺寸
		{
			grub_memcpy (memory_map, finish_mmap_buf, *memory_map_size);  //复制finish_mmap_buf到memory_map, memory_map_size字节
			ret = 0;
		}
		else
		{
			grub_memcpy (memory_map, finish_mmap_buf, finish_mmap_size);
			ret = 1;
		}
		*memory_map_size = finish_mmap_size;
		if (map_key)
			*map_key = finish_key;
		if (descriptor_size)
			*descriptor_size = finish_desc_size;
		if (descriptor_version)
			*descriptor_version = finish_desc_version;
		return ret;
	}
	 //如果EFI未完成
  /* Allow some parameters to be missing. 允许缺少一些参数 */
  if (! map_key)
    map_key = &key;
  if (! descriptor_version)
    descriptor_version = &version;
  if (! descriptor_size)
    descriptor_size = &size;

  b = grub_efi_system_table->boot_services;//系统表->引导服务
  status = efi_call_5 (b->get_memory_map, memory_map_size, memory_map, map_key,
			descriptor_size, descriptor_version); //获得内存映射(给定请求尺寸,给定映射地址,映射键,描述尺寸,描述符版本)
            //从给定映射抽取可用内存,然后给出可用内存集相对于给定映射地址的偏移memory_map_size. 回带描述符尺寸memory_map_size和描述符版本descriptor_version.
            
  if (*descriptor_size == 0)
    *descriptor_size = sizeof (grub_efi_memory_descriptor_t);
  if (status == GRUB_EFI_SUCCESS) //完成
    return 1;
  else if (status == GRUB_EFI_BUFFER_TOO_SMALL) //缓冲器太小
    return 0;
  else
    return -1;
}

/* Filter the descriptors. GRUB needs only available memory. 筛选内存映射。GRUB只需要可用内存 */
static grub_efi_memory_descriptor_t *filter_memory_map (grub_efi_memory_descriptor_t *memory_map, grub_efi_memory_descriptor_t *filtered_memory_map,
		   grub_efi_uintn_t desc_size, grub_efi_memory_descriptor_t *memory_map_end);
static grub_efi_memory_descriptor_t *
filter_memory_map (grub_efi_memory_descriptor_t *memory_map,	//映射起始
		   grub_efi_memory_descriptor_t *filtered_memory_map,			//筛选映射起始
		   grub_efi_uintn_t desc_size,														//描述符尺寸
		   grub_efi_memory_descriptor_t *memory_map_end)  				//映射结束
{
  grub_efi_memory_descriptor_t *desc;
  grub_efi_memory_descriptor_t *filtered_desc;

  for (desc = memory_map, filtered_desc = filtered_memory_map;
       desc < memory_map_end;
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size))//下一个内存描述符(描述符,尺寸)
	{
		if (desc->type == GRUB_EFI_CONVENTIONAL_MEMORY        			//如果是常规内存
				&& desc->physical_start < 0x100000)											//并且物理起始 < 1Mb
    {
			min_con_mem_start = (int)desc->physical_start;  //最低可用内存起始
      min_con_mem_size = (int)(desc->num_pages * 0x1000);
    }

		if (desc->type == GRUB_EFI_CONVENTIONAL_MEMORY        			//如果是常规内存
#if 1
				&& desc->physical_start <= GRUB_EFI_MAX_USABLE_ADDRESS  //并且物理起始<=最大可用地址4Gb
#endif
				&& desc->physical_start + PAGES_TO_BYTES (desc->num_pages) > 0x100000 //并且物理起始+使用内存 > 1Mb
				&& desc->num_pages != 0)                                //并且页数非0
		{
			grub_memcpy (filtered_desc, desc, desc_size);		//把常规内存映射, 复制到筛选映射起始

	  /* Avoid less than 1MB, because some loaders seem to be confused.  避免小于1MB，因为有些装载机看起来很混乱。*/
			if (desc->physical_start < 0x100000)		//如果物理起始 < 1Mb
	    {
	      desc->num_pages -= BYTES_TO_PAGES (0x100000
						 - desc->physical_start);					//减少使用内存
	      desc->physical_start = 0x100000;			//调整物理起始 = 1Mb
	    }

#if 1
			if (BYTES_TO_PAGES (filtered_desc->physical_start)			
					+ filtered_desc->num_pages
					> BYTES_TO_PAGES_DOWN (GRUB_EFI_MAX_USABLE_ADDRESS))	//如果物理起始+使用内存 > 4Gb
				filtered_desc->num_pages
						= (BYTES_TO_PAGES_DOWN (GRUB_EFI_MAX_USABLE_ADDRESS)//减少使用内存
						- BYTES_TO_PAGES (filtered_desc->physical_start));
#endif
			if (filtered_desc->num_pages == 0)	//如果使用内存=0
				continue;

			filtered_desc = NEXT_MEMORY_DESCRIPTOR (filtered_desc, desc_size);//下一个内存描述符(描述符,尺寸)
		}
	}

  return filtered_desc;
}

void grub_mm_init_region (void *addr, grub_size_t size);
//在使用内存堆头部,建立内存头,用于记录将来在堆内分配和释放内存.
/* Initialize a region starting from ADDR and whose size is SIZE,
   to use it as free space.  从addr开始初始化一个大小为size的区域，将其用作可用空间。*/
void grub_mm_init_region (void *addr, grub_size_t size);
void
grub_mm_init_region (void *addr, grub_size_t size)	//
{
  grub_mm_header_t h;					//内存头
  grub_mm_region_t r, *p, q;	//内存区域
  /* Exclude last 4K to avoid overflows. 排除最后4K以避免溢出。*/
  /* If addr + 0x1000 overflows then whole region is in excluded zone.  如果addr+0x1000溢出，则整个区域都在排除区域中。*/
  if ((grub_addr_t) addr > ~((grub_addr_t) 0x1000))
    return;
  /* If addr + 0x1000 + size overflows then decrease size.  如果addr+0x1000+size溢出，则减小size*/
  if (((grub_addr_t) addr + 0x1000) > ~(grub_addr_t) size)
    size = ((grub_addr_t) -0x1000) - (grub_addr_t) addr;
  
  for (p = &grub_mm_base, q = *p; q; p = &(q->next), q = *p)
	{	
    if ((grub_uint8_t *) addr + size + q->pre_size == (grub_uint8_t *) q)	//如果地址+尺寸+第一内存头->预先调整尺寸 = 第一内存头
		{
			r = (grub_mm_region_t) ALIGN_UP ((grub_addr_t) addr, GRUB_MM_ALIGN);
			*r = *q;
			r->pre_size += size;
			if (r->pre_size >> GRUB_MM_ALIGN_LOG2)
			{
				h = (grub_mm_header_t) (r + 1);
				h->size = (r->pre_size >> GRUB_MM_ALIGN_LOG2);
				h->magic = GRUB_MM_ALLOC_MAGIC;
				r->size += h->size << GRUB_MM_ALIGN_LOG2;
				r->pre_size &= (GRUB_MM_ALIGN - 1);
				*p = r;
				grub_free (h + 1);
			}
			*p = r;
			return;
		}
	}

  /* Allocate a region from the head.  从头部分配一个区域。*/
  r = (grub_mm_region_t) ALIGN_UP ((grub_addr_t) addr, GRUB_MM_ALIGN);	//节对齐
  /* If this region is too small, ignore it.  如果这个区域太小，忽略它。*/
  if (size < GRUB_MM_ALIGN + (char *) r - (char *) addr + sizeof (*r))
    return;

  size -= (char *) r - (char *) addr + sizeof (*r);

  h = (grub_mm_header_t) (r + 1);
  h->next = h;														//下一个内存头
  h->magic = GRUB_MM_FREE_MAGIC;					//魔术
  h->size = (size >> GRUB_MM_ALIGN_LOG2);	//尺寸

  r->first = h;																				//第一内存头
  r->pre_size = (grub_addr_t) r - (grub_addr_t) addr;	//预先调整尺寸
  r->size = (h->size << GRUB_MM_ALIGN_LOG2);					//尺寸
  /* Find where to insert this region. Put a smaller one before bigger ones,	找到插入此区域的位置把小的放在大的之前，以防止碎片化。
     to prevent fragmentation.  */
  for (p = &grub_mm_base, q = *p; q; p = &(q->next), q = *p)	//
    if (q->size > r->size)	//如果第一内存区域尺寸>当前内存区域尺寸			
      break;								//结束

  *p = r;				//a8dd000
  r->next = q;
}


//分配一块将来由自己直接分配和释放的内存区域.
/* Add memory regions. 添加内存区域  */
static void add_memory_regions (grub_efi_memory_descriptor_t *memory_map, grub_efi_uintn_t desc_size,
		    grub_efi_memory_descriptor_t *memory_map_end, grub_efi_uint64_t required_pages);
static void
add_memory_regions (grub_efi_memory_descriptor_t *memory_map,		//排序后的内存映射起始
		    grub_efi_uintn_t desc_size,															//描述符尺寸
		    grub_efi_memory_descriptor_t *memory_map_end,						//排序后的内存映射结束
		    grub_efi_uint64_t required_pages) 											//请求页数
{
  grub_efi_memory_descriptor_t *desc;	//内存描述符
  for (desc = memory_map;
       desc < memory_map_end;
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size))//下一个内存描述符(描述符,尺寸)
	{
		grub_efi_uint64_t pages;
		grub_efi_physical_address_t start;
		void *addr;

		start = desc->physical_start; //物理起始
		pages = desc->num_pages;      //页数
    if (pages < required_pages)   //如果页数>请求页数
      continue;

    start += PAGES_TO_BYTES (pages - required_pages); //物理起始+页转字节(页数-请求页数)
    pages = required_pages;                           //页数=请求页数		4323

		addr = grub_efi_allocate_pages_real (start, required_pages,
					   GRUB_EFI_ALLOCATE_ADDRESS,										//指定地址
					   GRUB_EFI_LOADER_CODE);   //真实分配页面(物理起始,请求页数,分配类型->地址,存储类型->装载程序代码)  
		if (! addr)
    {
			printf_errinfo ("cannot allocate conventional memory %p with %u pages",
		    (void *) ((grub_addr_t) start),
		    (unsigned) pages);
      break;
    }
		grub_mm_init_region (addr, PAGES_TO_BYTES (pages)); //初始化内存区域
		break;
	}
}

void grub_efi_memory_fini (void);
void
grub_efi_memory_fini (void) //内存结束
{
  /*
   * Free all stale allocations. grub_efi_free_pages() will remove    释放所有stale分配. 
   * the found entry from the list and it will always find the first  grub_efi_free_pages() 将从列表中移除找到的条目，它总是会找到第一个列表条目. 
   * list entry (efi_allocated_memory is the list start). Hence we    (efi_allocated_memory是列表开始)
   * remove all entries from the list until none is left altogether.  因此，我们从列表中删除所有条目，直到没有剩下。
   */
  while (efi_allocated_memory)
      grub_efi_free_pages (efi_allocated_memory->address,
                           efi_allocated_memory->pages); //释放页
}

#if 0   //打印内存分布
/* Print the memory map.  打印内存分布*/
static void print_memory_map (grub_efi_memory_descriptor_t *memory_map, grub_efi_uintn_t desc_size,	grub_efi_memory_descriptor_t *memory_map_end);
static void
print_memory_map (grub_efi_memory_descriptor_t *memory_map,	//内存起始 
		  grub_efi_uintn_t desc_size,														//描述符尺寸
		  grub_efi_memory_descriptor_t *memory_map_end)					//内存结束
{
  grub_efi_memory_descriptor_t *desc;
  int i;

  for (desc = memory_map, i = 0;
       desc < memory_map_end;
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size), i++)//下一个内存描述符(描述符,尺寸)
    {
      grub_printf ("MD: t=%x, p=%lx, v=%lx, n=%lx, a=%lx\n",
		   desc->type, desc->physical_start, desc->virtual_start,
		   desc->num_pages, desc->attribute);
			 console_getkey();
    }
}
#endif

void grub_efi_mm_init (void);
void
grub_efi_mm_init (void)  //内存管理初始化
{
  grub_efi_memory_descriptor_t *memory_map;               //内存映射地址
  grub_efi_memory_descriptor_t *memory_map_end;           //内存映射结束地址
  grub_efi_memory_descriptor_t *filtered_memory_map;      //过滤内存映射地址
  grub_efi_memory_descriptor_t *filtered_memory_map_end;  //过滤内存映射结束地址
  grub_efi_uintn_t map_size;        //映射尺寸
  grub_efi_uintn_t desc_size;       //描述尺寸
  grub_efi_uint64_t required_pages; //请求页数
  int mm_status;										//分配内存状态=1/0/-1=成功/部分/失败

  /* Prepare a memory region to store two memory maps. 准备存储区域来存储两个内存映射 共6页*/
  memory_map = grub_efi_allocate_any_pages (2 * BYTES_TO_PAGES (MEMORY_MAP_SIZE));  //分配最大地址页(请求页数)  (字节转页  向上舍入)
  if (! memory_map) //如果失败
    printf_errinfo ("cannot allocate memory");	//无法分配内存 

  /* Obtain descriptors for available memory. 获取可用内存的描述符 */
  map_size = MEMORY_MAP_SIZE; //内存映射尺寸  0x3000
  mm_status = grub_efi_get_memory_map (&map_size, memory_map, 0, &desc_size, 0);  //获得内存映射(映射尺寸,映射页,0,描述尺寸,0)  返回1/0/-1=成功/部分/失败
                                                                                  //获得内存描述符尺寸desc_size, 获得可用内存描述符集地址偏移map_size
  if (mm_status == 0) //如果是部分
	{
		grub_efi_free_pages
			((grub_efi_physical_address_t) ((grub_addr_t) memory_map),
			2 * BYTES_TO_PAGES (MEMORY_MAP_SIZE)); //释放页

		/* Freeing/allocating operations may increase memory map size. 释放/分配操作可以增加内存映射的大小 */
		map_size += desc_size * 32; //增大内存尺寸
		memory_map = grub_efi_allocate_any_pages (2 * BYTES_TO_PAGES (map_size)); //再次分配最大地址页(请求页数)(6)
		if (! memory_map) //如果失败
			printf_errinfo ("cannot allocate memory");	//无法分配内存 

		mm_status = grub_efi_get_memory_map (&map_size, memory_map, 0,
				&desc_size, 0);  //获得内存映射
	}
	
  if (mm_status < 0)  //如果失败
    printf_errinfo ("cannot get memory map");	//无法分配内存

  memory_map_end = NEXT_MEMORY_DESCRIPTOR (memory_map, map_size); //全部内存映射结尾

  filtered_memory_map = memory_map_end; //筛选内存映射开始

  filtered_memory_map_end = filter_memory_map (memory_map, filtered_memory_map,
			desc_size, memory_map_end);  //筛选内存映射结尾

  required_pages = 0x2000;  //分配32Mb
  /* Allocate memory regions for GRUB's memory management. 为GRUB's内存管理分配内存区域 */
  add_memory_regions (filtered_memory_map, desc_size,
			filtered_memory_map_end, required_pages);

#if 0 //打印内存分布
  /* For debug.  */
  map_size = MEMORY_MAP_SIZE;

  if (grub_efi_get_memory_map (&map_size, memory_map, 0, &desc_size, 0) < 0)
    grub_printf ("cannot get memory map");

  grub_printf ("printing memory map\n");
  print_memory_map (memory_map, desc_size,
		    NEXT_MEMORY_DESCRIPTOR (memory_map, map_size));
#endif

  /* Release the memory maps. 释放内存映射 */
  grub_efi_free_pages ((grub_addr_t) memory_map,
			2 * BYTES_TO_PAGES (MEMORY_MAP_SIZE)); //释放页
}
//-----------------------------------------------------------------------------
void get_datetime (struct grub_datetime *datetime);
void
get_datetime (struct grub_datetime *datetime)
{
  grub_efi_status_t status;
  struct grub_efi_time efi_time;

  status = efi_call_2 (grub_efi_system_table->runtime_services->get_time,
                       &efi_time, 0);

  if (status)
    printf_debug ("can\'t get datetime using efi");
  else
	{
		datetime->year = efi_time.year;
		datetime->month = efi_time.month;
		datetime->day = efi_time.day;
		datetime->hour = efi_time.hour;
		datetime->minute = efi_time.minute;
		datetime->second = efi_time.second;
	}
}

int getrtsecs (void);
int
getrtsecs (void)
{
	struct grub_datetime datetime;
	get_datetime (&datetime);
	return datetime.second;
}

void defer (unsigned short millisecond);  //毫秒 
void
defer (unsigned short millisecond)
{
	efi_call_1 (grub_efi_system_table->boot_services->stall, millisecond * 1000);
}


grub_efi_physical_address_t grub4dos_self_address = 0;
void copy_grub4dos_self_address (void);
void
copy_grub4dos_self_address (void)
{
  grub_efi_status_t status;     //状态
  grub_efi_boot_services_t *b;  //引导服务
  b = grub_efi_system_table->boot_services; //系统表->引导服务
  
  if (min_con_mem_start > 0x9F000)
    return;
  if (min_con_mem_start + min_con_mem_size < 0x9F000)
    grub4dos_self_address = min_con_mem_start + min_con_mem_size - 0x1000;
  else
    grub4dos_self_address = 0x9F000;
  
  status = efi_call_4 (b->allocate_pages, GRUB_EFI_ALLOCATE_ADDRESS, 
          GRUB_EFI_RUNTIME_SERVICES_DATA, 1, &grub4dos_self_address); //(分配页,分配类型=指定地址,存储类型=运行时数据,页数=1,返回分配地址)
  if (status)
    return;  
  //复制特定字符串
  grub_memmove ((void *)(grub4dos_self_address + 0xe0), "   $INT13SFGRUB4DOS", 19);  
  //复制bootx64.efi自身地址
  *(grub_size_t*)((char *)grub4dos_self_address + 0x100) = (grub_size_t)grub_image;
}

char *grub_image;
char *PAGING_TABLES_BUF;
unsigned char *PRINTF_BUFFER;
char *MENU_TITLE;
char *cmd_buffer;
char *FSYS_BUF;
char *CMDLINE_BUF;
char *COMPLETION_BUF;
char *UNIQUE_BUF;
char *HISTORY_BUF;
char *WENV_ENVI;
char *BASE_ADDR;
char *BUFFERADDR;
char *CMD_RUN_ON_EXIT;
char *SCRATCHADDR;
char *mbr;
//char *
//char *

void grub_console_init (void);
void grub_efidisk_init (void);
void grub_init (void);
grub_efi_loaded_image_t *image;

void
grub_init (void)
{
	grub_console_init ();

//  image = grub_efi_get_loaded_image (grub_efi_image_handle);  //通过映像句柄,获得加载映像grub_efi_loaded_image结构
  image =  grub_efi_open_protocol (grub_efi_image_handle,
				 &loaded_image_guid,
				 GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);  //打开协议(映像句柄,guid,获得协议)
	grub_image = image->image_base;	//通过加载映像,获得BOOIA32.EFI映像基址 	前部是映像头		struct grub_pe32_header   //PE32 头
																	//grub_image偏移400是grldr起始,也就是bios模式的8200处.可使用*((char *)(grub_image)+0x508))取单字节的值.

	grub_efi_mm_init ();  //内存管理初始化
  copy_grub4dos_self_address ();


	PAGING_TABLES_BUF = grub_malloc (0x4000); //分页表
	PRINTF_BUFFER = grub_malloc (0x40000);    //打印
	MENU_TITLE = grub_malloc (0x800);         //菜单标题
	cmd_buffer = grub_malloc (0x1000);        //命令
	CMDLINE_BUF = grub_malloc (0x640);        //命令行
	COMPLETION_BUF = grub_malloc (0x640);     //完成
	UNIQUE_BUF = grub_malloc (0x640);         //
	HISTORY_BUF = grub_malloc (0x4000);       //网络
	WENV_ENVI = grub_zalloc (0x200);          //
	BASE_ADDR = grub_zalloc (0x200);          //
	FSYS_BUF = grub_malloc (0x9010);          //
	BUFFERADDR = grub_malloc (0x10000);       //磁盘读写 
	CMD_RUN_ON_EXIT = grub_malloc (256);      //
	SCRATCHADDR = grub_malloc (0x1000);       //临时
  mbr = grub_malloc (0x1000);               //mbr
//buffer=grub_malloc (byte)  分配内存
//buffer=grub_zalloc (byte)  分配内存, 并清零
//grub_free (buffer)  释放内存

	efi_call_4 (grub_efi_system_table->boot_services->set_watchdog_timer,   //引导服务->设置看门狗定时器  避免设备5分钟就重启.
	      0, 0, 0, NULL);
				
	grub_efidisk_init ();  //efidisk初始化
}