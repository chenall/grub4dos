/*
 *  ipxe file system for grub4dos
 *
 *  Copyright (C) 2014 chenall (chenall.cn@gmail.com)
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

#ifdef FSYS_IPXE

#include "shared.h"
#include "filesys.h"
#include "ipxe.h"
#include "fsys_ipxe.h"

grub_u32_t has_ipxe = 0;
static grub_u32_t ipxe_funcs;
static grub_u32_t ipxe_file_opened;

s_PXE_FILE_FUNC ipxe_file_func = {ipxe_open,ipxe_get_size,ipxe_read_blk,ipxe_close,ipxe_unload};

static inline SEGOFF16_t FAR_PTR(void *ptr)
{
	SEGOFF16_t _fptr;
	_fptr.offset = (grub_u16_t)((grub_u32_t)ptr & 0xF);
	_fptr.segment = (grub_u16_t)((grub_u32_t)ptr >> 4);
	return _fptr;
}
/*
 * See if we have iPXE
 */
void ipxe_init(void)
{
	int err;

	pxenv.file_api_check.Size = sizeof(struct s_PXENV_FILE_API_CHECK);
	pxenv.file_api_check.Magic = 0x91d447b2;
	err = pxe_call(PXENV_FILE_API_CHECK, &pxenv.file_api_check);
	if (!err && pxenv.file_api_check.Magic == 0xe9c17b20)
		ipxe_funcs = pxenv.file_api_check.APIMask;

	/* Necessary functions for us to use the iPXE file API */
	has_ipxe = (~ipxe_funcs & 0x4b) == 0;

}

static void ipxe_unload(void)
{
//	pxe_call(PXENV_FILE_EXIT_HOOK,&pxenv.file_exit_hook);
	ipxe_funcs = 0;
	has_ipxe = 0;
}

static int ipxe_open(const char *dirname)
{
	grub_u8_t *filename;
	if (!has_ipxe) return 0;

	ipxe_close();

	char *ch = grub_strstr(dirname,":");

	if (!ch || (grub_u32_t)(ch - dirname) >= 10)
		filename = pxenv.tftp_open.FileName;
	else
		filename = (grub_u8_t*)dirname;

	while (*filename == '/' || *filename == ' ') ++filename;

	if ((unsigned long)debug >= 0x7FFFFFFF) printf("O:%s\n",filename);

	pxenv.file_open.FileName = FAR_PTR(filename);

	if (PXENV_EXIT_SUCCESS != pxe_call(PXENV_FILE_OPEN, &pxenv.file_open))
		return 0;

	ipxe_file_opened = pxenv.file_open.FileHandle;

	if (ipxe_file_opened > 1) printf_warning("Too many open files [1->%d]!",ipxe_file_opened);

	return 1;
}

static grub_u32_t ipxe_get_size(void)
{
	if (!ipxe_file_opened)
		return 0;

	pxenv.file_open.FileHandle = ipxe_file_opened;

	if (pxe_call(PXENV_GET_FILE_SIZE, &pxenv.get_file_size) != PXENV_EXIT_SUCCESS)
		return 0;
	filemax = pxenv.get_file_size.FileSize;
	return pxenv.get_file_size.FileSize;
}

static grub_u32_t ipxe_read_blk (grub_u32_t buf, grub_u32_t num)
{
	grub_u32_t read_len = 0;
	grub_u32_t max_len;
	grub_u32_t status;

	if (!ipxe_file_opened)
		return 0;
	/* disk cache will be destroyed, so invalidate it. */
	buf_drive = -1;
	buf_track = -1;

	max_len = num * pxe_blksize;
	pxenv.file_read.FileHandle  = ipxe_file_opened;
	while (read_len < max_len)
	{
		pxenv.file_read.Buffer      = FAR_PTR((void*)(buf + read_len));
		pxenv.file_read.BufferSize  = max_len - read_len;
		status = pxe_call(PXENV_FILE_READ, &pxenv.file_read);
		if (status !=  PXENV_EXIT_SUCCESS)
		{
			if (pxenv.file_read.Status == PXENV_STATUS_TFTP_OPEN)
				continue;
			return 0;
		}
		if (pxenv.file_read.BufferSize == 0)
			break;
		read_len += pxenv.file_read.BufferSize;
	}
	if ((unsigned long)debug >= 0x7FFFFFFF) printf("R[%d]: %d,%d,%d\n",ipxe_file_opened,num,pxe_blksize,read_len);
	return read_len;
}

static void ipxe_close (void)
{
	if (!ipxe_file_opened) return;
	pxenv.file_close.FileHandle = ipxe_file_opened;
	if (pxe_call(PXENV_FILE_CLOSE, &pxenv.file_close) == PXENV_EXIT_SUCCESS)
		ipxe_file_opened = 0;
}

int ipxe_func(char* arg,int flags)
{
	if (!(ipxe_funcs & (1<<(PXENV_FILE_EXEC - PXENV_FILE_MIN)))) return !(errnum = ERR_FUNC_CALL);
	memmove((void*)IPXE_BUF,arg,strlen(arg)+1);
	pxenv.file_exec.Command=FAR_PTR((void*)IPXE_BUF);
	return pxe_call(PXENV_FILE_EXEC, &pxenv.file_exec) == PXENV_EXIT_SUCCESS;
}
#endif /* FSYS_IPXE */
