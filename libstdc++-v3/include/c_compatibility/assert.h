// -*- C++ -*- compatibility header.

// Copyright (C) 2002-2026 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file assert.h
 *  This is a Standard C++ Library header.
 */

// No include guards: the `assert` macro must be (re)defined according to the
// current NDEBUG (and __STDC_WANT_ASSERT_USES_CONTRACTS__) on each inclusion.

// Get the platform's <assert.h> first.  When the P3290 assert integration is
// not enabled this leaves behavior unchanged from the platform header.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic" // include_next
#include_next <assert.h>
#pragma GCC diagnostic pop

// P3290 assert integration and its feature-test macro, shared with <cassert>.
// This expands to nothing unless -fcontracts-p3290 is active.
#include <bits/assert_contract.h>
