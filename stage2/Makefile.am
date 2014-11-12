# For test target.
#TESTS = size_test
TESTS = 
noinst_SCRIPTS = $(TESTS)

# For silencing OBJCOPY output.
AM_V_OBJCOPY_0 = @echo "  OBJ    $@";$(OBJCOPY)
AM_V_OBJCOPY_1 = $(OBJCOPY)
AM_V_OBJCOPY_  = $(AM_V_OBJCOPY_$(AM_DEFAULT_VERBOSITY))
AM_V_OBJCOPY   = $(AM_V_OBJCOPY_$(V))

# For dist target.
noinst_HEADERS = fat.h filesys.h freebsd.h hercules.h \
	iso9660.h jfs.h mb_header.h mb_info.h md5.h \
	pc_slice.h serial.h shared.h term.h \
	terminfo.h tparm.h ufs2.h vstafs.h xfs.h pxe.h graphics.h fsys_initrd.h cpio.h
EXTRA_DIST = $(noinst_SCRIPTS)

# Stage 2 and Stage 1.5's.
datalibdir = $(libdir)/$(PACKAGE)/$(host_cpu)-$(host_vendor)

EXTRA_PROGRAMS = nbloader.exec pxeloader.exec diskless.exec

datalib_DATA = grub.exe badgrub.exe grldr grldr.mbr bootlace.com hmload.com eltorito.sys
noinst_DATA = pre_stage2 dosstart badgrubstart grldrstart mbrstart bootlacestart hmloadstart pre_stage2_fullsize pre_stage2_head pre_stage2_tail
noinst_PROGRAMS = pre_stage2.exec dosstart.exec badgrubstart.exec grldrstart.exec mbrstart.exec bootlacestart.exec hmloadstart.exec

MOSTLYCLEANFILES = $(noinst_PROGRAMS)

PRE_STAGE2_LINK = -nostdlib -Wl,-N -Wl,-Ttext -Wl,8200 $(BIN_LDFLAGS)
START_LINK = -nostdlib -Wl,-N -Wl,-Ttext -Wl,8000 $(BIN_LDFLAGS)

if SERIAL_SUPPORT
SERIAL_FLAGS = -DSUPPORT_SERIAL=1
else
SERIAL_FLAGS =
endif

if HERCULES_SUPPORT
HERCULES_FLAGS = -DSUPPORT_HERCULES=1
else
HERCULES_FLAGS =
endif

if GRAPHICS_SUPPORT
GRAPHICS_FLAGS = -DSUPPORT_GRAPHICS=1
else
GRAPHICS_FLAGS =
endif

if GFX_SUPPORT
GFX_FLAGS = -DSUPPORT_GFX=1
else
GFX_FLAGS =
endif

STAGE2_COMPILE = $(STAGE2_CFLAGS) -fno-builtin -nostdinc \
	$(SERIAL_FLAGS) $(HERCULES_FLAGS) $(GRAPHICS_FLAGS) \
	$(GFX_FLAGS)

# For stage2 target.
pre_stage2_exec_SOURCES = asm.S bios.c boot.c builtins.c char_io.c \
	cmdline.c common.c console.c dec_lzma.c disk_io.c fsys_ext2fs.c \
	fsys_fat.c fsys_ntfs.c fsys_ffs.c fsys_iso9660.c fsys_jfs.c fsys_minix.c \
	fsys_reiserfs.c fsys_ufs2.c fsys_vstafs.c fsys_xfs.c fsys_pxe.c fsys_initrd.c fsys_fb.c gunzip.c \
	hercules.c md5.c serial.c stage2.c terminfo.c tparm.c graphics.c
pre_stage2_exec_CFLAGS = $(STAGE2_COMPILE) $(FSYS_CFLAGS)
pre_stage2_exec_CCASFLAGS = $(STAGE2_COMPILE) $(FSYS_CFLAGS)
pre_stage2_exec_LDFLAGS = $(PRE_STAGE2_LINK)

CLEANFILES = $(datalib_DATA) $(noinst_DATA) $(BUILT_SOURCES)

stage2_size.h: pre_stage2
	$(AM_V_at)-rm -f stage2_size.h
	$(AM_V_at)set dummy `ls -l pre_stage2`; \
	echo "#define STAGE2_SIZE $$6" > stage2_size.h

dosstart_exec_SOURCES = dosstart.S
dosstart_exec_CCASFLAGS = $(STAGE2_COMPILE)
dosstart_exec_LDFLAGS = $(START_LINK)

dosstart_exec-dosstart.$(OBJEXT): stage2_size.h

grub.exe: pre_stage2 dosstart
	$(AM_V_GEN)
	$(AM_V_at)-rm -f grub.exe
	$(AM_V_at)echo -n -e "\000\000\000\000\000\000\000\000\000\000\000\000" | cat dosstart pre_stage2 - > grub.exe
	$(AM_V_at)if [ -f ../preset_menu_used ]; then cat ../preset_menu_used >> grub.exe ;fi
	$(AM_V_at)dd if=/dev/zero bs=4128 count=1 >> grub.exe 2>/dev/null || echo "DD failed!"

badgrubstart_exec_SOURCES = badgrubstart.S
badgrubstart_exec_CCASFLAGS = $(STAGE2_COMPILE)
badgrubstart_exec_LDFLAGS = $(START_LINK)

badgrubstart_exec-badgrubstart.$(OBJEXT): stage2_size.h dosstart.S

badgrub.exe: pre_stage2 badgrubstart
	$(AM_V_GEN)
	$(AM_V_at)-rm -f badgrub.exe
	$(AM_V_at)echo -n -e "\000\000\000\000\000\000\000\000\000\000\000\000" | cat badgrubstart pre_stage2 - > badgrub.exe
	$(AM_V_at)if [ -f ../preset_menu_used ]; then cat ../preset_menu_used >> badgrub.exe ;fi
	$(AM_V_at)dd if=/dev/zero bs=4128 count=1 >> badgrub.exe 2>/dev/null || echo "DD failed!"

grldrstart_exec_SOURCES = grldrstart.S
grldrstart_exec_CCASFLAGS = $(STAGE2_COMPILE)
grldrstart_exec_LDFLAGS = $(START_LINK)

grldrstart_exec-grldrstart.$(OBJEXT): stage2_size.h

grldr: pre_stage2 grldrstart
	$(AM_V_GEN)
	$(AM_V_at)-rm -f grldr
	$(AM_V_at)echo -n -e "\000\000\000\000\000\000\000\000\000\000\000\000" | cat grldrstart pre_stage2 - > grldr
	$(AM_V_at)if [ -f ../preset_menu_used ]; then cat ../preset_menu_used >> grldr ;fi

mbrstart_exec_SOURCES = mbrstart.S
mbrstart_exec_CCASFLAGS = $(STAGE2_COMPILE)
mbrstart_exec_LDFLAGS = $(START_LINK)

mbrstart_exec-mbrstart.$(OBJEXT): stage2_size.h grldrstart.S

grldr.mbr: mbrstart
	$(AM_V_GEN)
	$(AM_V_at)-rm -f grldr.mbr
	$(AM_V_at)cat mbrstart > grldr.mbr

bootlacestart_exec_SOURCES = bootlacestart.S
bootlacestart_exec_CCASFLAGS = $(STAGE2_COMPILE)
bootlacestart_exec_LDFLAGS = -nostdlib -Wl,-N -Wl,-Ttext -Wl,08048000 $(BIN_LDFLAGS)

bootlacestart_exec-bootlacestart.$(OBJEXT): stage2_size.h bootlace.inc grldrstart.S

bootlace.com: bootlacestart
	$(AM_V_GEN)
	$(AM_V_at)-rm -f bootlace.com
	$(AM_V_at)cp bootlacestart bootlace.com && chmod a+x bootlace.com

hmloadstart_exec_SOURCES = hmloadstart.S
hmloadstart_exec_CCASFLAGS = $(STAGE2_COMPILE)
hmloadstart_exec_LDFLAGS = -nostdlib -Wl,-N -Wl,-Ttext -Wl,08048000 $(BIN_LDFLAGS)

hmloadstart_exec-hmloadstart.$(OBJEXT): stage2_size.h

hmload.com: hmloadstart
	$(AM_V_GEN)
	$(AM_V_at)-rm -f hmload.com
	$(AM_V_at)cp hmloadstart hmload.com && chmod a-x hmload.com

eltorito.sys: eltorito.asm
	$(AM_V_at)nasm -Ox -f bin -o $@ $< || echo "Failed in building eltorito.sys!"
	$(AM_V_at)upx --ultra-brute --lzma $@ || upx --ultra-brute $@ || echo "upx command not found!"

asm.S: a20.inc
	touch asm.S

dosstart.S: a20.inc
	touch dosstart.S

badgrubstart.S: a20.inc
	touch badgrubstart.S

# General rule for making a raw binary.
SUFFIXES = .exec
%: %.exec$(EXEEXT)
	$(AM_V_OBJCOPY) -O binary $< $@
	$(AM_V_at)if [ $@ = pre_stage2 ]; then dd if=pre_stage2 of=pre_stage2_fullsize bs=65536 skip=48 2>/dev/null || echo "DD failed!" ;fi
	$(AM_V_at)if [ $@ = pre_stage2 ]; then sed -e "1s/^\x00*//" pre_stage2_fullsize > pre_stage2_tail ;fi
	$(AM_V_at)if [ $@ = pre_stage2 ]; then set dummy `ls -l pre_stage2_fullsize`; set $$6 `ls -l pre_stage2_tail`; if [ $$1 -le $$6 ]; then set `cmp pre_stage2_fullsize /dev/zero` ; while [ "$$1" != "" ]; do t1="$${1%%[^0-9]*}"; if [ "$$t1" != "" ]; then if [ "$$t1" -gt 2000 ]; then t1=`expr $$t1 - 1`; dd if=pre_stage2_fullsize of=pre_stage2_tail bs=1 skip=$$t1  2>/dev/null || echo "DD failed!" ; break; fi; fi; shift; done; fi; fi
	$(AM_V_at)if [ $@ = pre_stage2 ]; then set dummy `ls -l pre_stage2_fullsize`; set $$6 `ls -l pre_stage2_tail`; dd if=pre_stage2 of=pre_stage2_head bs=`expr $$1 - $$6` count=1 2>/dev/null || echo "DD failed!" ; cat pre_stage2_head pre_stage2_tail > pre_stage2 ;fi
	$(AM_V_at)if [ $@ = pre_stage2 ]; then echo -n -e "\0260\002\032\0316" >> $@ ;fi
