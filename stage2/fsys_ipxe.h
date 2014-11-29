#ifdef FSYS_IPXE
#ifndef _FSYS_IPXE_H_
#define _FSYS_IPXE_H_

#define PKTBUF_SIZE     2048
#define IPXE_BUF	BUFFERADDR
#define iPXE_BUFLEN	0xFE00
int ipxe_open(const char *dirname);
grub_u32_t ipxe_get_size(void);
grub_u32_t ipxe_read_blk (grub_u32_t buf, grub_u32_t num);
void ipxe_close (void);
void ipxe_unload(void);
#endif /* _FSYS_IPXE_H_ */
#endif