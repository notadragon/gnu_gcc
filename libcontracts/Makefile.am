## Makefile for the GNU Contracts Runtime Library (libcontracts).
##
## Copyright (C) 2026 Free Software Foundation, Inc.
##
## This file is part of the GNU Contracts Runtime Library (libcontracts).
##
## Libcontracts is free software; you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## Libcontracts is distributed in the hope that it will be useful, but WITHOUT
## ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
## FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
## more details.
##
## Under Section 7 of GPL version 3, you are granted additional
## permissions described in the GCC Runtime Library Exception, version
## 3.1, as published by the Free Software Foundation.
##
## You should have received a copy of the GNU General Public License and
## a copy of the GCC Runtime Library Exception along with this program;
## see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
## <http://www.gnu.org/licenses/>.

AUTOMAKE_OPTIONS = 1.8 foreign
ACLOCAL_AMFLAGS = -I .. -I ../config

## May be used by toolexeclibdir.
gcc_version := $(shell @get_gcc_base_ver@ $(top_srcdir)/../gcc/BASE-VER)

## The library is installed in the compiler's target library directory.
toolexeclib_LTLIBRARIES = libcontracts.la

## The pure-C contracts ABI core: dispatch + chain walking + non-noexcept
## entry points, C accessor functions, and the P3290 C API / check helpers.
libcontracts_la_SOURCES = dispatch.c accessors.c c_api.c

noinst_HEADERS = contracts-abi.h

## Wire the linker version script directly (LIBCONTRACTS_1.0 node) and set the
## SONAME via libtool -version-info (CURRENT-AGE = 1 => libcontracts.so.1).
libcontracts_version_script = \
	-Wl,--version-script,$(srcdir)/libcontracts.map
if ENABLE_DARWIN_AT_RPATH
libcontracts_darwin_rpath = -Wc,-nodefaultrpaths
libcontracts_darwin_rpath += -Wl,-rpath,@loader_path
endif
libcontracts_la_LDFLAGS = \
	-version-info $(libtool_VERSION) $(libcontracts_version_script) \
	$(lt_host_flags) $(libcontracts_darwin_rpath)
libcontracts_la_DEPENDENCIES = $(srcdir)/libcontracts.map

# libcontracts calls the user's violation handler, which may throw through
# it: build it with unwind information on every target.
AM_CFLAGS = $(XCFLAGS) -fexceptions
