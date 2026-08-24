# Process this file with autoconf to produce a configure script.
#  Copyright (C) 2026 Free Software Foundation, Inc.
#
#  This file is part of the GNU Contracts Runtime Library (libcontracts).
#
#  Libcontracts is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  Libcontracts is distributed in the hope that it will be useful, but WITHOUT
#  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
#  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
#  more details.
#
#  Under Section 7 of GPL version 3, you are granted additional
#  permissions described in the GCC Runtime Library Exception, version
#  3.1, as published by the Free Software Foundation.
#
#  You should have received a copy of the GNU General Public License and
#  a copy of the GCC Runtime Library Exception along with this program;
#  see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
#  <http://www.gnu.org/licenses/>.

AC_INIT([GNU Contracts Runtime Library], 1.0,,[libcontracts])

# --enable-version-specific-runtime-libs (standard GCC target-lib option).
AC_ARG_ENABLE(version-specific-runtime-libs,
  AS_HELP_STRING([--enable-version-specific-runtime-libs],
    [specify that runtime libraries should be installed in a
     compiler-specific directory]),
  [case "$enableval" in
     yes|no) ;;
     *) AC_MSG_ERROR([Unknown argument to enable/disable
                      version-specific libs]);;
   esac],
  [enable_version_specific_runtime_libs=no])

# Gets build, host, target, *_vendor, *_cpu, *_os, etc.  When building this
# library the top-level /target/ becomes the library's /host/.
AC_CANONICAL_SYSTEM
target_alias=${target_alias-$host_alias}

# Sets up automake.  Must come after AC_CANONICAL_SYSTEM.
AM_INIT_AUTOMAKE([1.9.0 foreign no-dist -Wall -Wno-portability -Wno-override])
AM_ENABLE_MULTILIB(, ..)

GCC_WITH_TOOLEXECLIBDIR

# Calculate toolexeclibdir (verbatim from the other GCC target libraries).
case ${enable_version_specific_runtime_libs} in
  yes)
    toolexecdir='$(libdir)/gcc/$(target_alias)'
    toolexeclibdir='$(toolexecdir)/$(gcc_version)$(MULTISUBDIR)'
    ;;
  no)
    if test -n "$with_cross_host" &&
       test x"$with_cross_host" != x"no"; then
      toolexecdir='$(exec_prefix)/$(target_alias)'
      case ${with_toolexeclibdir} in
	no)
	  toolexeclibdir='$(toolexecdir)/lib'
	  ;;
	*)
	  toolexeclibdir=${with_toolexeclibdir}
	  ;;
      esac
    else
      toolexecdir='$(libdir)/gcc-lib/$(target_alias)'
      toolexeclibdir='$(libdir)'
    fi
    multi_os_directory=`$CC -print-multi-os-directory`
    case $multi_os_directory in
      .) ;; # Avoid trailing /.
      *) toolexeclibdir=$toolexeclibdir/$multi_os_directory ;;
    esac
    ;;
esac
AC_SUBST(toolexecdir)
AC_SUBST(toolexeclibdir)

AC_PROG_CC
AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(NM, nm)
AC_CHECK_TOOL(RANLIB, ranlib, ranlib-not-found-in-path-error)
AC_PROG_INSTALL

# Configure libtool.
AM_PROG_LIBTOOL
ACX_LT_HOST_FLAGS
AC_SUBST(enable_shared)
AC_SUBST(enable_static)
AM_MAINTAINER_MODE

AM_CONDITIONAL([ENABLE_DARWIN_AT_RPATH],
	       [test x$enable_darwin_at_rpath = xyes])

# For libtool versioning info, format is CURRENT:REVISION:AGE.
# CURRENT - AGE = 1  =>  SONAME libcontracts.so.1.
libtool_VERSION=1:0:0
AC_SUBST(libtool_VERSION)

# Get target configury (sets UNSUPPORTED non-empty for targets without a C
# runtime).
. ${srcdir}/configure.tgt
if test -n "$UNSUPPORTED"; then
  AC_MSG_ERROR([Configuration ${target} is unsupported.])
fi

# Add -Wall -Werror when using GCC.
AC_ARG_ENABLE([werror],
  [AS_HELP_STRING([--disable-werror], [disable building with -Werror])])
AS_IF([test "x$GCC" = "xyes"], [XCFLAGS="$XCFLAGS -Wall"])
AS_IF([test "x$enable_werror" != "xno" && test "x$GCC" = "xyes"],
  [XCFLAGS="$XCFLAGS -Werror"])

# Add CET specific flags if CET is enabled.
GCC_CET_FLAGS(CET_FLAGS)
XCFLAGS="$XCFLAGS $CET_FLAGS"
AC_SUBST(XCFLAGS)

# Determine what GCC version number to use in filesystem paths.
GCC_BASE_VER

if test ${multilib} = yes; then
  multilib_arg="--enable-multilib"
else
  multilib_arg=
fi

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
