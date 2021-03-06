/* fsys_jfs.c - an implementation for the IBM JFS file system */
/*  
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2001,2002  Free Software Foundation, Inc.
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

#ifdef FSYS_JFS

#include "shared.h"
#include "filesys.h"
#include "jfs.h"

#define MAX_LINK_COUNT	8

#define DTTYPE_INLINE	0
#define DTTYPE_PAGE	1

struct jfs_info
{
	unsigned long bsize;
	unsigned long l2bsize;
	unsigned long bdlog;
	unsigned long xindex;
	unsigned long xlastindex;
	unsigned long sindex;
	unsigned long slastindex;
	unsigned long de_index;
	unsigned long dttype;
	struct xad *xad;
	ldtentry_t *de;
};

static struct jfs_info jfs;

#define xtPage		((xtpage_t *)FSYS_BUF)
#define dtPage		((dtpage_t *)((char *)FSYS_BUF + 4096))
#define fileSet		((dinode_t *)((char *)FSYS_BUF + 8192))
#define iNode		((dinode_t *)((char *)FSYS_BUF + 8192 + sizeof(dinode_t)))
#define dtRoot		((dtroot_t *)(&iNode->di_xtroot))

static char *linkbuf = (char *)(FSYS_BUF - JFS_PATH_MAX);	/* buffer for following symbolic links */
static char *namebuf = (char *)(FSYS_BUF - JFS_PATH_MAX - JFS_NAME_MAX - 1);

static ldtentry_t de_always[2] = {
	{1, -1, 2, {'.', '.'}},
	{1, -1, 1, {'.'}}
};

static int
isinxt (s64 key, s64 offset, s64 len)
{
	return (key >= offset) ? (key < offset + len ? 1 : 0) : 0;
}

static struct xad *
first_extent (dinode_t *di)
{
	xtpage_t *xtp;

	jfs.xindex = 2;
	xtp = (xtpage_t *)&di->di_xtroot;
	jfs.xad = &xtp->xad[2];
	if (xtp->header.flag & BT_LEAF) {
	    	jfs.xlastindex = xtp->header.nextindex;
	} else {
		do {
			devread (addressXAD (jfs.xad) << jfs.bdlog, 0,
				 sizeof(xtpage_t), (unsigned long long)(unsigned int)(char *)xtPage, 0xedde0d90);
			jfs.xad = &xtPage->xad[2];
		} while (!(xtPage->header.flag & BT_LEAF));
		jfs.xlastindex = xtPage->header.nextindex;
	}

	return jfs.xad;
}

static struct xad *
next_extent (void)
{
	if (++jfs.xindex < jfs.xlastindex) {
	} else if (xtPage->header.next) {
		devread (xtPage->header.next << jfs.bdlog, 0,
			 sizeof(xtpage_t), (unsigned long long)(unsigned int)(char *)xtPage, 0xedde0d90);
		jfs.xlastindex = xtPage->header.nextindex;
		jfs.xindex = XTENTRYSTART;
		jfs.xad = &xtPage->xad[XTENTRYSTART];
	} else {
		return NULL;
	}
	return ++jfs.xad;
}


static void
di_read (u32 inum, dinode_t *di)
{
	s64 key;
	u32 xd, ioffset;
	s64 offset;
	struct xad *xad;
	pxd_t pxd;		/* struct size = 8 */

	key = (((inum >> L2INOSPERIAG) << L2INOSPERIAG) + 4096) >> jfs.l2bsize;
	xd = (inum & (INOSPERIAG - 1)) >> L2INOSPEREXT;
	ioffset = ((inum & (INOSPERIAG - 1)) & (INOSPEREXT - 1)) << L2DISIZE;
	xad = first_extent (fileSet);
	do {
		offset = offsetXAD (xad);
		if (isinxt (key, offset, lengthXAD (xad))) {
			devread ((addressXAD (xad) + key - offset) << jfs.bdlog,
				 3072 + xd*sizeof(pxd_t), sizeof(pxd_t), (unsigned long long)(unsigned int)(char *)&pxd, 0xedde0d90);
			devread (addressPXD (&pxd) << jfs.bdlog,
				 ioffset, DISIZE, (unsigned long long)(unsigned int)(char *)di, 0xedde0d90);
			break;
		}
	} while ((xad = next_extent ()));
}

static ldtentry_t *
next_dentry (void)
{
	ldtentry_t *de;
	s8 *stbl;

	if (jfs.dttype == DTTYPE_INLINE) {
		if (jfs.sindex < jfs.slastindex) {
			return (ldtentry_t *)&dtRoot->slot[(int)dtRoot->header.stbl[jfs.sindex++]];
		}
	} else {
		de = (ldtentry_t *)dtPage->slot;
		stbl = (s8 *)&de[(int)dtPage->header.stblindex];
		if (jfs.sindex < jfs.slastindex) {
			return &de[(int)stbl[jfs.sindex++]];
		} else if (dtPage->header.next) {
			devread (dtPage->header.next << jfs.bdlog, 0,
				 sizeof(dtpage_t), (unsigned long long)(unsigned int)(char *)dtPage, 0xedde0d90);
			jfs.slastindex = dtPage->header.nextindex;
			jfs.sindex = 1;
			return &de[(int)((s8 *)&de[(int)dtPage->header.stblindex])[0]];
		}
	}

	return (jfs.de_index < 2) ? &de_always[jfs.de_index++] : NULL;
}

static ldtentry_t *
first_dentry (void)
{
	dtroot_t *dtr;
	pxd_t *xd;
	idtentry_t *de;

	dtr = (dtroot_t *)&iNode->di_xtroot;
	jfs.sindex = 0;
	jfs.de_index = 0;

	de_always[0].inumber = iNode->di_parent;
	de_always[1].inumber = iNode->di_number;
	if (dtr->header.flag & BT_LEAF) {
		jfs.dttype = DTTYPE_INLINE;
		jfs.slastindex = dtr->header.nextindex;
	} else {
		de = (idtentry_t *)dtPage->slot;
		jfs.dttype = DTTYPE_PAGE;
		xd = &((idtentry_t *)dtr->slot)[(int)dtr->header.stbl[0]].xd;
		for (;;) {
			devread (addressPXD (xd) << jfs.bdlog, 0,
				 sizeof(dtpage_t), (unsigned long long)(unsigned int)(char *)dtPage, 0xedde0d90);
			if (dtPage->header.flag & BT_LEAF)
				break;
			xd = &de[(int)((s8 *)&de[(int)dtPage->header.stblindex])[0]].xd;
		}
		jfs.slastindex = dtPage->header.nextindex;
	}

	return next_dentry ();
}


static dtslot_t *
next_dslot (int next)
{
	return (jfs.dttype == DTTYPE_INLINE)
		? (dtslot_t *)&dtRoot->slot[next]
		: &((dtslot_t *)dtPage->slot)[next];
}

#if 0
static void
uni2ansi (UniChar *uni, char *ansi, int len)
{
	for (; len; len--, uni++)
		*ansi++ = (*uni & 0xff80) ? '?' : *(char *)uni;
}
#else
#define uni2ansi unicode_to_utf8
#endif

int
jfs_mount (void)
{
	struct jfs_superblock super;	/* struct size = 160 */

	if ((unsigned long)part_length < MINJFS >> SECTOR_BITS
	    || !devread (SUPER1_OFF >> SECTOR_BITS, 0,
			 sizeof(struct jfs_superblock), (unsigned long long)(unsigned int)(char *)&super, 0xedde0d90)
	    || (super.s_magic != JFS_MAGIC)
	    || !devread ((AITBL_OFF >> SECTOR_BITS) + FILESYSTEM_I,
			 0, DISIZE, (unsigned long long)(unsigned int)(char*)fileSet, 0xedde0d90)) {
		return 0;
	}

	jfs.bsize = super.s_bsize;
	jfs.l2bsize = super.s_l2bsize;
	jfs.bdlog = jfs.l2bsize - SECTOR_BITS;

	return 1;
}

unsigned long long
jfs_read (unsigned long long buf, unsigned long long len, unsigned long write)
{
	struct xad *xad;
	s64 endofprev, endofcur;
	s64 offset, xadlen;
	unsigned long toread, startpos, endpos;

	startpos = filepos;
	endpos = filepos + len;
	endofprev = (1ULL << 62) - 1;
	xad = first_extent (iNode);
	do {
		offset = offsetXAD (xad);
		xadlen = lengthXAD (xad);
		if (isinxt (filepos >> jfs.l2bsize, offset, xadlen)) {
			endofcur = (offset + xadlen) << jfs.l2bsize; 
			toread = (endofcur >= endpos)
				  ? len : (endofcur - filepos);

			disk_read_func = disk_read_hook;
			devread (addressXAD (xad) << jfs.bdlog,
				 filepos - (offset << jfs.l2bsize), toread, buf, write);
			disk_read_func = NULL;

			if (buf)
				buf += toread;
			len -= toread;		/* len always >= 0 */
			filepos += toread;
		} else if (offset > endofprev) {
			toread = ((offset << jfs.l2bsize) >= endpos)
				  ? len : ((offset - endofprev) << jfs.l2bsize);
			filepos += toread;
#if 0
			{
				unsigned long tmp_pos = toread;
				for (; tmp_pos; tmp_pos--) {
					if (buf)
						*(buf++) = 0;
				}
			}
#else
			if (buf)
			{
				grub_memset64 (buf, 0, toread);
				buf += toread;
			}
#endif
			if (len <= toread)
				break;
			len -= toread;
			continue;
		}
		endofprev = offset + xadlen; 
		xad = next_extent ();
	} while (len > 0 && xad);

	return filepos - startpos;
}

int
jfs_dir (char *dirname)
{
	char *ptr, *rest, ch;
	ldtentry_t *de;
	dtslot_t *ds;
	u32 inum, parent_inum;
	s64 di_size;
	u32 di_mode;
	int cmp;
	unsigned long namlen, n, link_count;
//	char namebuf[JFS_NAME_MAX + 1], linkbuf[JFS_PATH_MAX];

	parent_inum = inum = ROOT_I;
	link_count = 0;
	for (;;) {
		di_read (inum, iNode);
		di_size = iNode->di_size;
		di_mode = iNode->di_mode;

		if ((di_mode & IFMT) == IFLNK) {
			if (++link_count > MAX_LINK_COUNT) {
				errnum = ERR_SYMLINK_LOOP;
				return 0;
			}
			if (di_size < (di_mode & INLINEEA ? 256 : 128)) {
				grub_memmove (linkbuf, iNode->di_fastsymlink, di_size);
				n = di_size;
			} else if (di_size < JFS_PATH_MAX - 1) {
				filepos = 0;
				filemax = di_size;
				n = jfs_read ((unsigned long long)(unsigned int)linkbuf, filemax, 0xedde0d90);
			} else {
				errnum = ERR_FILELENGTH;
				return 0;
			}

			inum = (linkbuf[0] == '/') ? ROOT_I : parent_inum;
			while (n < (JFS_PATH_MAX - 1) && (linkbuf[n++] = *dirname++));
			linkbuf[n] = 0;
			dirname = linkbuf;
			continue;
		}

		if (!*dirname || isspace (*dirname)) {
			if ((di_mode & IFMT) != IFREG) {
				errnum = ERR_BAD_FILETYPE;
				return 0;
			}
			filepos = 0;
			filemax = di_size;
			return 1;
		}

		if ((di_mode & IFMT) != IFDIR) {
			errnum = ERR_BAD_FILETYPE;
			return 0;
		}

		for (; *dirname == '/'; dirname++);

		//for (rest = dirname; (ch = *rest) && !isspace (ch) && ch != '/'; rest++);
		for (rest = dirname; (ch = *rest) /*&& !isspace (ch)*/ && ch != '/'; rest++)
		{
#if 0
			if (ch == '\\')
			{
				rest++;
				if (! (ch = *rest))
					break;
			}
#endif
		}

		*rest = 0;

		de = first_dentry ();
		for (;;) {
			namlen = de->namlen;
			if (de->next == -1) {
				uni2ansi (de->name, (unsigned char *)namebuf, namlen);
				namebuf[namlen] = 0;
			} else {
				uni2ansi (de->name, (unsigned char *)namebuf, DTLHDRDATALEN);
				ptr = namebuf;
				ptr += DTLHDRDATALEN;
				namlen -= DTLHDRDATALEN;
				ds = next_dslot (de->next);
				while (ds->next != -1) {
					uni2ansi (ds->name, (unsigned char *)ptr, DTSLOTDATALEN);
					ptr += DTSLOTDATALEN;
					namlen -= DTSLOTDATALEN;
					ds = next_dslot (ds->next);
				}
				uni2ansi (ds->name, (unsigned char *)ptr, namlen);
				ptr += namlen;
				*ptr = 0;
			}

			cmp = (!*dirname) ? -1 : substring (dirname, namebuf, 0);
			if (print_possibilities && ch != '/'
			    && cmp <= 0) {
				if (print_possibilities > 0)
					print_possibilities = -print_possibilities;
				print_a_completion (namebuf, 0);
			} else
			if (cmp == 0) {
				parent_inum = inum;
				inum = de->inumber;
		        	*(dirname = rest) = ch;
				break;
			}
			de = next_dentry ();
			if (de == NULL) {
				if (print_possibilities < 0)
					return 1;

				errnum = ERR_FILE_NOT_FOUND;
				*rest = ch;
				return 0;
			}
		}
	}
}

unsigned long
jfs_embed (unsigned long *start_sector, unsigned long needed_sectors)
{
	struct jfs_superblock super;	/* struct size = 160 */

	if (needed_sectors > 63
	    || !devread (SUPER1_OFF >> SECTOR_BITS, 0,
			 sizeof (struct jfs_superblock),
			 (unsigned long long)(unsigned int)(char *)&super, 0xedde0d90)
	    || (super.s_magic != JFS_MAGIC)) {
		return 0;
	}

	*start_sector = 1;
	return 1;
}

#endif /* FSYS_JFS */
