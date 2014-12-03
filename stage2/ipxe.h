#ifdef FSYS_IPXE
#ifndef iPXE_API_H
#define iPXE_API_H

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * As an alternative, at your option, you may use this file under the
 * following terms, known as the "MIT license":
 *
 * Copyright (c) 2005-2009 Michael Brown <mbrown@fensystems.co.uk>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pxe.h"

/** Minimum possible opcode used within PXE FILE API */
#define PXENV_FILE_MIN 0x00e0
/** Minimum possible opcode used within PXE FILE API */
#define PXENV_FILE_MAX 0x00ef
/** PXE API function code for pxenv_file_open() */
#define PXENV_FILE_OPEN			0x00e0
/** Parameter block for pxenv_file_open() */
typedef grub_u16_t PXENV_STATUS_t;
typedef struct s_SEGOFF16 {
	grub_u16_t offset;         /**< Offset within the segment */
	grub_u16_t segment;        /**< Segment selector */
} __attribute__ ((packed)) SEGOFF16_t;

struct s_PXENV_FILE_OPEN {
	PXENV_STATUS_t Status;		/**< PXE status code */
	grub_u16_t FileHandle;		/**< File handle */
	SEGOFF16_t FileName;		/**< File URL */
	grub_u32_t Reserved;		/**< Reserved */
} __attribute__ (( packed ));

typedef struct s_PXENV_FILE_OPEN PXENV_FILE_OPEN_t;

/** PXE API function code for pxenv_file_close() */
#define PXENV_FILE_CLOSE		0x00e1

/** Parameter block for pxenv_file_close() */
struct s_PXENV_FILE_CLOSE {
	PXENV_STATUS_t Status;		/**< PXE status code */
	grub_u16_t FileHandle;		/**< File handle */
} __attribute__ (( packed ));

typedef struct s_PXENV_FILE_CLOSE PXENV_FILE_CLOSE_t;

/** PXE API function code for pxenv_file_select() */
#define PXENV_FILE_SELECT		0x00e2
/** File is ready for reading */
#define RDY_READ			0x0001

/** Parameter block for pxenv_file_select() */
struct s_PXENV_FILE_SELECT {
	PXENV_STATUS_t Status;		/**< PXE status code */
	grub_u16_t FileHandle;		/**< File handle */
	grub_u16_t Ready;			/**< Indication of readiness */
} __attribute__ (( packed ));

typedef struct s_PXENV_FILE_SELECT PXENV_FILE_SELECT_t;

/** PXE API function code for pxenv_file_read() */
#define PXENV_FILE_READ		0x00e3
/** Parameter block for pxenv_file_read() */
struct s_PXENV_FILE_READ {
	PXENV_STATUS_t Status;		/**< PXE status code */
	grub_u16_t FileHandle;		/**< File handle */
	grub_u16_t BufferSize;		/**< Data buffer size */
	SEGOFF16_t Buffer;		/**< Data buffer */
} __attribute__ (( packed ));

typedef struct s_PXENV_FILE_READ PXENV_FILE_READ_t;

/** PXE API function code for pxenv_get_file_size() */
#define PXENV_GET_FILE_SIZE		0x00e4
/** Parameter block for pxenv_get_file_size() */
struct s_PXENV_GET_FILE_SIZE {
	PXENV_STATUS_t Status;		/**< PXE status code */
	grub_u16_t FileHandle;		/**< File handle */
	grub_u32_t FileSize;		/**< File size */
} __attribute__ (( packed ));

typedef struct s_PXENV_GET_FILE_SIZE PXENV_GET_FILE_SIZE_t;

/** PXE API function code for pxenv_file_exec() */
#define PXENV_FILE_EXEC			0x00e5
/** Parameter block for pxenv_file_exec() */
struct s_PXENV_FILE_EXEC {
	PXENV_STATUS_t Status;		/**< PXE status code */
	SEGOFF16_t Command;		/**< Command to execute */
} __attribute__ (( packed ));

typedef struct s_PXENV_FILE_EXEC PXENV_FILE_EXEC_t;

/** PXE API function code for pxenv_file_api_check() */
#define PXENV_FILE_API_CHECK		0x00e6
/** Parameter block for pxenv_file_api_check() */
struct s_PXENV_FILE_API_CHECK {
	PXENV_STATUS_t Status;		/**< PXE status code */
	grub_u16_t Size;			/**< Size of structure  */
	grub_u32_t Magic;			/**< Magic number */
	grub_u32_t Provider;		/**< Implementation identifier */
	grub_u32_t APIMask;		/**< Supported API functions */
	grub_u32_t Flags;			/**< Reserved for the future */
} __attribute__ (( packed ));

typedef struct s_PXENV_FILE_API_CHECK PXENV_FILE_API_CHECK_t;

/** PXE API function code for pxenv_file_exit_hook() */
#define PXENV_FILE_EXIT_HOOK			0x00e7
/** Parameter block for pxenv_file_exit_hook() */
struct s_PXENV_FILE_EXIT_HOOK {
	PXENV_STATUS_t Status;		/**< PXE status code */
	SEGOFF16_t Hook;		/**< SEG16:OFF16 to jump to */
} __attribute__ (( packed ));

typedef struct s_PXENV_FILE_EXIT_HOOK PXENV_FILE_EXIT_HOOK_t;

/** PXE API function code for pxenv_file_cmdline() */
#define PXENV_FILE_CMDLINE			0x00e8
/** Parameter block for pxenv_file_cmdline() */
struct s_PXENV_FILE_CMDLINE {
	PXENV_STATUS_t Status;		/**< PXE status code */
	grub_u16_t BufferSize;		/**< Data buffer size */
	SEGOFF16_t Buffer;		/**< Data buffer */
} __attribute__ (( packed ));

typedef struct s_PXENV_FILE_CMDLINE PXENV_FILE_CMDLINE_t;

union u_PXENV_ANY {
	/* Make it easy to read status for any operation */
	PXENV_STATUS_t				Status;
	PXENV_GET_CACHED_INFO_t			get_cached_info;
	PXENV_TFTP_OPEN_t			tftp_open;
	PXENV_TFTP_CLOSE_t			tftp_close;
	PXENV_TFTP_READ_t			tftp_read;
	PXENV_TFTP_GET_FSIZE_t			tftp_get_size;
	struct s_PXENV_FILE_OPEN		file_open;
	struct s_PXENV_FILE_CLOSE		file_close;
	struct s_PXENV_FILE_SELECT		file_select;
	struct s_PXENV_FILE_READ		file_read;
	struct s_PXENV_GET_FILE_SIZE		get_file_size;
	struct s_PXENV_FILE_EXEC		file_exec;
	struct s_PXENV_FILE_API_CHECK		file_api_check;
	struct s_PXENV_FILE_EXIT_HOOK		file_exit_hook;
	struct s_PXENV_FILE_CMDLINE		file_cmdline;
};

typedef union u_PXENV_ANY iPXENV_ANY_t;
typedef union u_PXENV_ANY PXENV_ANY_t;
extern PXENV_ANY_t pxenv;

extern grub_u32_t has_ipxe;
extern s_PXE_FILE_FUNC ipxe_file_func;
#endif /* iPXE_API_H */
#endif /* ifdef FSYS_IPXE */
