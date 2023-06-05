dnl Configure script for GRUB.
dnl Copyright 1999,2000,2001,2002,2003,2004,2005 Free Software Foundation, Inc.

dnl Permission to use, copy, modify and distribute this software and its
dnl documentation is hereby granted, provided that both the copyright
dnl notice and this permission notice appear in all copies of the
dnl software, derivative works or modified versions, and any portions
dnl thereof, and that both notices appear in supporting documentation.
dnl
dnl THE FREE SOFTWARE FOUNDATION ALLOWS FREE USE OF THIS SOFTWARE IN ITS
dnl "AS IS" CONDITION.  THE FREE SOFTWARE FOUNDATION DISCLAIMS ANY
dnl LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE
dnl USE OF THIS SOFTWARE.

AC_PREREQ(2.57)
AC_INIT([GRUB], [0.97], [bug-grub@gnu.org])
AC_CONFIG_SRCDIR([stage2/stage2.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
AM_SILENT_RULES([yes])
AM_PROG_AS

dnl Generate stage2/grub4dos_version.h
echo "#define GRUB4DOS_VERSION \"`cat grub4dos_version` `date -u +%Y-%m-%d`\"" > grub4dos_version.h
echo "#define GRUB4DOS_INT_VER `date -u +%Y%m%d`" > grub4dos_int_ver.h

AC_CANONICAL_HOST

case "$host_cpu" in
i[[3456]]86) host_cpu=i386 ;;
x86_64) host_cpu=x86_64 ;;
*) AC_MSG_ERROR([unsupported CPU type]) ;;
esac

AC_SUBST(host_cpu)
AC_SUBST(host_vendor)

#
# Options
#

AM_MAINTAINER_MODE
if test "x$enable_maintainer_mode" = xyes; then
  AC_PATH_PROG(PERL,perl)
  if test -z "$PERL"; then
    AC_MSG_ERROR([perl not found])
  fi
fi

# This should be checked before AC_PROG_CC
if test "x$ac_env_CFLAGS_set" = x; then
  default_CFLAGS=yes
fi

if test "x$host_cpu" = xx86_64; then
  CFLAGS="-m32 -mno-sse $CFLAGS"
fi

AC_SUBST(default_CFLAGS)
#
# Programs
#

AC_CHECK_TOOL(CC, gcc)
AC_PROG_CC
# We need this for older versions of Autoconf.
_AM_DEPENDENCIES(CC)

dnl Because recent automake complains about AS, set it here.
CCAS="$CC"
AC_SUBST(CCAS)

AC_ARG_WITH(binutils,
  [  --with-binutils=DIR     search the directory DIR to find binutils])

if test "x$with_binutils" != x; then
dnl AC_PATH_TOOL is not seen in autoconf 2.13, so use AC_PATH_PROG
dnl instead for now. It is preferable when you cross-compile GRUB.
dnl  AC_PATH_TOOL(RANLIB, ranlib, :, "$with_binutils:$PATH")
  AC_PATH_PROG(RANLIB, ranlib, :, "$with_binutils:$PATH")
else
  AC_PROG_RANLIB
fi

# optimization flags
if test "x$ac_cv_c_compiler_gnu" = xyes; then
  if test "x$default_CFLAGS" = xyes; then
    # Autoconf may set CFLAGS to -O2 and/or -g. So eliminate them.
    CFLAGS="`echo $CFLAGS | sed -e 's/-g//g' -e 's/-O[[0-9]]//g'` -g"
    # If the user specify the directory for binutils, add the option `-B'.
    if test "x$with_binutils" != x; then
      CFLAGS="-B$with_binutils/ $CFLAGS"
    fi
    STAGE1_CFLAGS="-O2"
    GRUB_CFLAGS="-O2"
    AC_CACHE_CHECK([whether optimization for size works], grub_cv_cc_Os, [
      saved_CFLAGS=$CFLAGS
      CFLAGS="-Os -g"
      AC_TRY_COMPILE(, , grub_cv_cc_Os=yes, grub_cv_cc_Os=no)
      CFLAGS=$saved_CFLAGS
    ])
    if test "x$grub_cv_cc_Os" = xyes; then
      STAGE2_CFLAGS="-Os -mpreferred-stack-boundary=2 -momit-leaf-frame-pointer -fno-strict-aliasing -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables"
    else
      STAGE2_CFLAGS="-O2 -fno-strength-reduce -fno-unroll-loops -mpreferred-stack-boundary=2 -momit-leaf-frame-pointer -fno-strict-aliasing -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables"
    fi
    # OpenBSD has a GCC extension for protecting applications from
    # stack smashing attacks, but GRUB doesn't want this feature.
    AC_CACHE_CHECK([whether gcc has -fno-stack-protector],
		   grub_cv_cc_no_stack_protector, [
      saved_CFLAGS=$CFLAGS
      CFLAGS="-fno-stack-protector"
      AC_TRY_COMPILE(,
		     ,
		     grub_cv_cc_no_stack_protector=yes,
		     grub_cv_cc_no_stack_protector=no)
      CFLAGS=$saved_CFLAGS
    ])
    if test "x$grub_cv_cc_no_stack_protector" = xyes; then
      STAGE2_CFLAGS="$STAGE2_CFLAGS -fno-stack-protector"
    fi
    # GCC >= 3.3 supports -fno-reorder-functions; this defends us against
    # unlikely-to-be-executed functions being linked before _start with GCC
    # >= 4.6.
    AC_CACHE_CHECK([whether gcc has -fno-reorder-functions],
           grub_cv_cc_no_reorder_functions_flag, [
      saved_CFLAGS=$CFLAGS
      CFLAGS="-fno-reorder-functions"
      AC_TRY_COMPILE(,
             ,
             grub_cv_cc_no_reorder_functions_flag=yes,
             grub_cv_cc_no_reorder_functions_flag=no)
      CFLAGS=$saved_CFLAGS
    ])
    if test "x$grub_cv_cc_no_reorder_functions_flag" = xyes; then
      STAGE2_CFLAGS="$STAGE2_CFLAGS -fno-reorder-functions"
    fi
  fi
fi

AC_SUBST(STAGE1_CFLAGS)
AC_SUBST(STAGE2_CFLAGS)
AC_SUBST(GRUB_CFLAGS)

# Enforce coding standards.
CPPFLAGS="$CPPFLAGS -Wall -Wmissing-prototypes -Wunused -Wshadow"
CPPFLAGS="$CPPFLAGS -Wpointer-arith"

AC_CACHE_CHECK([whether -Wundef works], grub_cv_cc_Wundef, [
  saved_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="-Wundef"
  AC_TRY_COMPILE(, , grub_cv_cc_Wundef=yes, grub_cv_cc_Wundef=no)
  CPPFLAGS="$saved_CPPFLAGS"
])

# The options `-falign-*' are supported by gcc 3.0 or later.
# Probably it is sufficient to only check for -falign-loops.
AC_CACHE_CHECK([whether -falign-loops works], [grub_cv_cc_falign_loop], [
  saved_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="-falign-loops=1"
  AC_TRY_COMPILE(, , [grub_cv_cc_falign_loop=yes], [grub_cv_cc_falign_loop=no])
  CPPFLAGS="$saved_CPPFLAGS"
])

# Force no alignment to save space.
if test "x$grub_cv_cc_falign_loop" = xyes; then
  CPPFLAGS="$CPPFLAGS -falign-jumps=1 -falign-loops=1 -falign-functions=1"
else
  CPPFLAGS="$CPPFLAGS -malign-jumps=1 -malign-loops=1 -malign-functions=1"
fi

if test "x$grub_cv_cc_Wundef" = xyes; then
  CPPFLAGS="$CPPFLAGS -Wundef"
fi

#gcc 5 and newer has builded with enable-default-pie by defualt so we add -no-pie to linker flag
AC_MSG_CHECKING([checking whether gcc has enable pie by default])
saved_LDFLAGS="$LDFLAGS"
LDFLAGS="-no-pie"
AC_TRY_LINK(, , no_pie_linker_flag=yes, no_pie_linker_flag=no)
AC_MSG_RESULT([$no_pie_linker_flag])
LDFLAGS=$saved_LDFLAGS
if test "x$no_pie_linker_flag" = xyes; then
  LDFLAGS="$LDFLAGS -no-pie"
fi

# Check if build ID can be disabled in the linker
AC_MSG_CHECKING([whether linker accepts `--build-id=none'])
save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -Wl,--build-id=none"
AC_TRY_LINK(, , build_id_flag=yes, build_id_flag=no)
AC_MSG_RESULT([$build_id_flag])
LDFLAGS="$save_LDFLAGS"
if test "x$build_id_flag" = xyes; then
  LDFLAGS="$LDFLAGS -Wl,--build-id=none"
fi

if test "x$with_binutils" != x; then
dnl  AC_PATH_TOOL(OBJCOPY, objcopy, , "$with_binutils:$PATH")
  AC_PATH_PROG(OBJCOPY, objcopy, , "$with_binutils:$PATH")
else
  AC_CHECK_TOOL(OBJCOPY, objcopy)
fi

LDFLAGS_SAVED="${LDFLAGS}"
case "$host_os" in
  mingw32 | cygwin)
    # Remove exception handling (objcopy can get confused with .eh_frame sections otherwise)
    CPPFLAGS="$CPPFLAGS -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables"
    BIN_LDFLAGS="-Wl,-T"'$(top_srcdir)'"/ldscript"
    LDFLAGS="${LDFLAGS} -Wl,-T${srcdir}/ldscript"
    BUILD_DIRS="stage2"
    ;;
  *)
    BIN_LDFLAGS="-Wl,-N"
    BUILD_DIRS="stage2"
    ;;
esac
AC_SUBST(BIN_LDFLAGS)
AC_SUBST(BUILD_DIRS)

# Defined in acinclude.m4.
grub_ASM_USCORE
grub_PROG_OBJCOPY_ABSOLUTE
if test "x$grub_cv_prog_objcopy_absolute" != xyes; then
  AC_MSG_ERROR([GRUB requires a working absolute objcopy; upgrade your binutils])
fi

grub_ASM_PREFIX_REQUIREMENT

grub_ASM_ADDR32
if test "x$grub_cv_asm_addr32" != xyes; then
  AC_MSG_ERROR([GRUB requires GAS .code16 addr32 support; upgrade your binutils])
fi

grub_ASM_ABSOLUTE_WITHOUT_ASTERISK

grub_CHECK_START_SYMBOL
grub_CHECK_USCORE_START_SYMBOL
if test "x$grub_cv_check_start_symbol" != "xyes" \
	-a "x$grub_cv_check_uscore_start_symbol" != "xyes"; then
  AC_MSG_ERROR([Neither start nor _start is defined])
fi

grub_CHECK_USCORE_USCORE_BSS_START_SYMBOL
grub_CHECK_USCORE_EDATA_SYMBOL
grub_CHECK_EDATA_SYMBOL
if test "x$grub_cv_check_uscore_uscore_bss_start_symbol" != "xyes" \
	-a "x$grub_cv_check_uscore_edata_symbol" != "xyes" \
	-a "x$grub_cv_check_edata_symbol" != "xyes"; then
  AC_MSG_ERROR([None of __bss_start, _edata, edata defined])
fi

grub_CHECK_END_SYMBOL
grub_CHECK_USCORE_END_SYMBOL
if test "x$grub_cv_check_end_symbol" != "xyes" \
	-a "x$grub_cv_check_uscore_end_symbol" != "xyes"; then
  AC_MSG_ERROR([Neither end nor _end is defined])
fi

LDFLAGS=${LDFLAGS_SAVED}

# Get the filename or the whole disk and open it.
# Known to work on NetBSD.
AC_CHECK_LIB(util, opendisk, [GRUB_LIBS="$GRUB_LIBS -lutil"
  AC_DEFINE(HAVE_OPENDISK, 1, [Define if opendisk() in -lutil can be used])])

AC_SUBST(GRUB_LIBS)

# Check for headers.
AC_CHECK_HEADERS(string.h strings.h)

# Check for user options.

# filesystems support.
AC_ARG_ENABLE(ext2fs,
  [  --disable-ext2fs        disable ext2fs support in Stage 2])

if test x"$enable_ext2fs" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_EXT2FS=1"
fi

AC_ARG_ENABLE(fat,
  [  --disable-fat           disable FAT support in Stage 2])

if test x"$enable_fat" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_FAT=1"
fi

AC_ARG_ENABLE(ntfs,
  [  --disable-ntfs          disable NTFS support in Stage 2])

if test x"$enable_ntfs" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_NTFS=1"
fi

AC_ARG_ENABLE(ffs,
  [  --disable-ffs           disable FFS support in Stage 2])

if test x"$enable_ffs" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_FFS=1"
fi

AC_ARG_ENABLE(ufs2,
  [  --disable-ufs2          disable UFS2 support in Stage 2])

if test x"$enable_ufs2" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_UFS2=1"
fi

AC_ARG_ENABLE(minix,
  [  --disable-minix         disable Minix fs support in Stage 2])

if test x"$enable_minix" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_MINIX=1"
fi

AC_ARG_ENABLE(reiserfs,
  [  --disable-reiserfs      disable ReiserFS support in Stage 2])

if test x"$enable_reiserfs" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_REISERFS=1"
fi

AC_ARG_ENABLE(vstafs,
  [  --disable-vstafs        disable VSTa FS support in Stage 2])

if test x"$enable_vstafs" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_VSTAFS=1"
fi

AC_ARG_ENABLE(jfs,
  [  --disable-jfs           disable IBM JFS support in Stage 2])

if test x"$enable_jfs" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_JFS=1"
fi

AC_ARG_ENABLE(xfs,
  [  --disable-xfs           disable SGI XFS support in Stage 2])

if test x"$enable_xfs" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_XFS=1"
fi

AC_ARG_ENABLE(iso9660,
  [  --disable-iso9660       disable ISO9660 support in Stage 2])

if test x"$enable_iso9660" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_ISO9660=1"
fi

AC_ARG_ENABLE(ipxe,
  [  --disable-ipxe          disable ipxe support in Stage 2])

AC_ARG_ENABLE(pxe,
  [  --disable-pxe           disable PXE support in Stage 2])

if test x"$enable_pxe" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_PXE=1"
  if test x"$enable_ipxe" != xno; then
    FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_IPXE=1"
  fi
fi





AC_ARG_ENABLE(initrdfs,
  [  --disable-initrdfs      disable initrdfs support in Stage 2])

if test x"$enable_initrdfs" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_INITRD=1"
fi

AC_ARG_ENABLE(fb,
  [  --disable-fb            disable FB support in Stage 2])

if test x"$enable_fb" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_FB=1"
fi

dnl AC_ARG_ENABLE(tftp,
dnl [  --enable-tftp           enable TFTP support in Stage 2])
dnl 
dnl #if test x"$enable_tftp" = xyes; then
dnl FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_TFTP=1"
dnl fi

AC_ARG_ENABLE(gunzip,
  [  --disable-gunzip        disable decompression in Stage 2])

if test x"$enable_gunzip" = xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DNO_DECOMPRESSION=1"
fi

AC_ARG_ENABLE(md5-password,
  [  --disable-md5-password  disable MD5 password support in Stage 2])
if test "x$enable_md5_password" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DUSE_MD5_PASSWORDS=1"
fi

dnl Graphical splashscreen support
AC_ARG_ENABLE(graphics,
  [  --disable-graphics      disable graphics terminal support])
AM_CONDITIONAL(GRAPHICS_SUPPORT, test "x$enable_graphics" != xno)

dnl gfxmenu support
AC_ARG_ENABLE(gfxmenu,
  [  --disable-gfxmenu       disable gfxmenu support])
AM_CONDITIONAL(GFX_SUPPORT, test "x$enable_gfxmenu" != xno)

dnl Hercules terminal
AC_ARG_ENABLE(hercules,
  [  --disable-hercules      disable hercules terminal support])
AM_CONDITIONAL(HERCULES_SUPPORT, test "x$enable_hercules" != xno)

dnl Serial terminal
AC_ARG_ENABLE(serial,
  [  --disable-serial        disable serial terminal support])
AM_CONDITIONAL(SERIAL_SUPPORT, test "x$enable_serial" != xno)

dnl Simulation of the slowness of a serial device.
AC_ARG_ENABLE(serial-speed-simulation,
  [  --enable-serial-speed-simulation
                          simulate the slowness of a serial device])
AM_CONDITIONAL(SERIAL_SPEED_SIMULATION,
  test "x$enable_serial_speed_simulation" = xyes)


dnl Embed a menu string in GRUB itself.
AC_ARG_ENABLE(preset-menu,
  [  --enable-preset-menu=FILE
                          preset a menu file FILE in Stage 2])
if test "x$enable_preset_menu" = x; then
  if test "x$NET_CFLAGS" = x; then
    rm -f preset_menu_used
  else
    AC_DEFINE(PRESET_MENU_STRING, "bootp\n", [if there is user specified preset menu string])
    echo -n -e "bootp\n\0" > preset_menu_used
  fi
else
  if test -r $enable_preset_menu; then
    grub_DEFINE_FILE(PRESET_MENU_STRING, [$enable_preset_menu],
    		     [Define if there is user specified preset menu string])
    cp $enable_preset_menu preset_menu_used
    echo -n -e "\0" >> preset_menu_used
  else
    rm -f preset_menu_used
    AC_MSG_ERROR([Cannot read the preset menu file $enable_preset_menu])
  fi
fi

dnl Build the example Multiboot kernel.
AC_ARG_ENABLE(example-kernel,
  [  --enable-example-kernel
                          build the example Multiboot kernel])
AM_CONDITIONAL(BUILD_EXAMPLE_KERNEL, test "x$enable_example_kernel" = xyes)

dnl Automatic Linux mem= option.
AC_ARG_ENABLE(auto-linux-mem-opt,
  [  --disable-auto-linux-mem-opt
                          don't pass Linux mem= option automatically])
if test "x$enable_auto_linux_mem_opt" = xno; then
  :
else
  AC_DEFINE(AUTO_LINUX_MEM_OPT, 1, [Define if you don't want to pass the mem= option to Linux])
fi

dnl Now substitute the variables.
AC_SUBST(FSYS_CFLAGS)

dnl Because recent automake complains about CCASFLAGS, set it here.
CCASFLAGS='$(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)'
AC_SUBST(CCASFLAGS)


dnl Output.
AC_CONFIG_FILES([Makefile stage2/Makefile])
AC_OUTPUT
