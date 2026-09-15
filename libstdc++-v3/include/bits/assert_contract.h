// P3290 assert integration -*- C++ -*-

// Copyright The GNU Toolchain Authors.
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

/** @file bits/assert_contract.h
 *  This is an internal header file, included by <cassert> and <assert.h>.
 *  Do not attempt to use it directly. @headername{cassert}
 */

// P3290 assert integration, shared between <cassert> and <assert.h>.
//
// This header intentionally has NO include guard: like <cassert>, the `assert`
// macro must be (re)defined according to the current NDEBUG on each textual
// inclusion.  Including translation units must include the platform <assert.h>
// (via #include_next) BEFORE this header so that the contract definition of
// `assert` wins.
//
// The entire body is gated on __gcc_contracts_p3290 (predefined by the
// compiler for -fcontracts-p3290 / -fcontracts-p3850).  When that flag is not
// active this header expands to nothing, so <assert.h> and <cassert> behave
// exactly as they do without contracts support.

#if defined(__cplusplus) && __cplusplus > 202302L \
    && defined(__gcc_contracts_p3290)

// Define the __cpp_lib_assert_can_use_contracts feature-test macro (P3290).
#define __glibcxx_want_assert_can_use_contracts
#include <bits/version.h>

#ifdef __cpp_lib_assert_can_use_contracts
#if !defined(NDEBUG) && defined(__STDC_WANT_ASSERT_USES_CONTRACTS__)
#undef assert
#include <source_location>
// Shared assert-integration entry point (P3290): reports assertion_kind=cassert
// and detection_mode=predicate_false to the contract-violation handler, and
// terminates via std::abort() on any completion of the handler (a normal
// return or an escaping exception).  libc++ provides the same symbol.  The
// source_location is passed from the macro (rather than via a default argument)
// so this declaration is safe to re-process across repeated inclusions.
extern "C++" [[noreturn]] void
__cxa_handle_cassert_violation(const char*, std::source_location) noexcept;
#define assert(...) \
  (static_cast<bool>(__VA_ARGS__) \
   ? static_cast<void>(0) \
   : __cxa_handle_cassert_violation(#__VA_ARGS__, \
				    std::source_location::current()))
#endif // !NDEBUG && __STDC_WANT_ASSERT_USES_CONTRACTS__
#endif // __cpp_lib_assert_can_use_contracts

#endif // C++26 && __gcc_contracts_p3290
