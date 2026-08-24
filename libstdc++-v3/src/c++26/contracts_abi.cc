// -*- C++ -*- Contracts ABI noexcept entry points

// Copyright The GNU Toolchain Authors.
//
// This file is part of GCC.
//
// GCC is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// GCC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

// The contracts ABI dispatch core, chain walking, and the non-noexcept entry
// points live in the pure-C libcontracts.  What remains here are the C++
// _noexcept entry-point variants: each wraps a libcontracts dispatch
// primitive in a noexcept terminate-on-throw barrier (a handler that exits
// via an exception at a noexcept site terminates the program via
// std::terminate(), with the exception still active).  This is the one part of
// the contracts runtime that requires C++ exception handling, so it cannot
// live in the pure-C core.

#include <contracts>
#include <bits/contracts_abi.h>
#include <exception>

#ifndef __cpp_lib_contracts
# error "This file requires C++26 contracts support to be enabled"
#endif

using namespace __cxxabiv1;

// Universal noexcept entry point.
extern "C" void
__cxa_contract_violation_noexcept (void* __data) noexcept
{
  auto* __chain = static_cast<const __cxa_contract_data_block*>(__data);
  __UINT8_TYPE__ __sem = __cxa_find_field_value<__UINT8_TYPE__>(
      __chain, CXA_FIELD_EVALUATION_SEMANTIC, CXA_ES_UNSPECIFIED);
  try { __contract_dispatch_core(__chain, __sem); }
  catch (...) { std::terminate(); }
}

// Noexcept terminate-on-throw wrapper around __contract_dispatch_core, invoked
// with an explicit core semantic.  The pure-C libcontracts declares this as a
// weak reference (contracts-abi.h) and its sanitizer-report routing entry point
// calls it in place of the raw core: that entry runs on the sanitizer runtime's
// noexcept report path under the D4298 noexcept evaluation semantics, so a
// handler that exits via an exception must terminate the program here rather
// than escape into frames that cannot unwind it.
extern "C" void
__contract_dispatch_core_noexcept (const __cxa_contract_data_block* __chain,
				   __UINT8_TYPE__ __semantic) noexcept
{
  try { __contract_dispatch_core(__chain, __semantic); }
  catch (...) { std::terminate(); }
}

// Specialized noexcept entry points: observe.

#define CXA_OBSERVE_NX(kind_name, kind_val, mode_name, mode_val)		\
extern "C" void								\
__cxa_contract_violation_##kind_name##_observe_##mode_name##_noexcept	\
    (void* __data) noexcept						\
{									\
  try { __dispatch_with_override_core(					\
	    __data, kind_val, CXA_ES_OBSERVE, mode_val); }		\
  catch (...) { std::terminate(); }					\
}

CXA_OBSERVE_NX(pre,    CXA_AK_PRE,    pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE_NX(pre,    CXA_AK_PRE,    ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_OBSERVE_NX(post,   CXA_AK_POST,   pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE_NX(post,   CXA_AK_POST,   ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_OBSERVE_NX(assert, CXA_AK_ASSERT, pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE_NX(assert, CXA_AK_ASSERT, ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_OBSERVE_NX(post_capture, CXA_AK_POST_CAPTURE, pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE_NX(post_capture, CXA_AK_POST_CAPTURE, ex,
               CXA_DM_EVALUATION_EXCEPTION)
CXA_OBSERVE_NX(implicit,     CXA_AK_IMPLICIT,     pf, CXA_DM_PREDICATE_FALSE)
CXA_OBSERVE_NX(implicit,     CXA_AK_IMPLICIT,     ex,
               CXA_DM_EVALUATION_EXCEPTION)

#undef CXA_OBSERVE_NX

// Specialized noexcept entry points: enforce ([[noreturn]]).

#define CXA_ENFORCE_NX(kind_name, kind_val, mode_name, mode_val)		\
extern "C" [[noreturn]] void						\
__cxa_contract_violation_##kind_name##_enforce_##mode_name##_noexcept	\
    (void* __data) noexcept						\
{									\
  try { __dispatch_with_override_core(					\
	    __data, kind_val, CXA_ES_ENFORCE, mode_val); }		\
  catch (...) { std::terminate(); }					\
  __builtin_unreachable();						\
}

CXA_ENFORCE_NX(pre,    CXA_AK_PRE,    pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE_NX(pre,    CXA_AK_PRE,    ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_ENFORCE_NX(post,   CXA_AK_POST,   pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE_NX(post,   CXA_AK_POST,   ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_ENFORCE_NX(assert, CXA_AK_ASSERT, pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE_NX(assert, CXA_AK_ASSERT, ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_ENFORCE_NX(post_capture, CXA_AK_POST_CAPTURE, pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE_NX(post_capture, CXA_AK_POST_CAPTURE, ex,
               CXA_DM_EVALUATION_EXCEPTION)
CXA_ENFORCE_NX(implicit,     CXA_AK_IMPLICIT,     pf, CXA_DM_PREDICATE_FALSE)
CXA_ENFORCE_NX(implicit,     CXA_AK_IMPLICIT,     ex,
               CXA_DM_EVALUATION_EXCEPTION)

#undef CXA_ENFORCE_NX

// Specialized entry points: noexcept_observe (D4298).  Always dispatches
// through the noexcept-terminating path.

#define CXA_NOEXCEPT_OBSERVE(kind_name, kind_val, mode_name, mode_val)	   \
extern "C" void								   \
__cxa_contract_violation_##kind_name##_noexcept_observe_##mode_name##_noexcept \
    (void* __data) noexcept						   \
{									   \
  try { __dispatch_with_override_core(					   \
	    __data, kind_val, CXA_ES_NOEXCEPT_OBSERVE, mode_val); }	   \
  catch (...) { std::terminate(); }					   \
}

CXA_NOEXCEPT_OBSERVE(pre,    CXA_AK_PRE,    pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_OBSERVE(pre,    CXA_AK_PRE,    ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_OBSERVE(post,   CXA_AK_POST,   pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_OBSERVE(post,   CXA_AK_POST,   ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_OBSERVE(assert, CXA_AK_ASSERT, pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_OBSERVE(assert, CXA_AK_ASSERT, ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_OBSERVE(post_capture, CXA_AK_POST_CAPTURE, pf,
                     CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_OBSERVE(post_capture, CXA_AK_POST_CAPTURE, ex,
                     CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_OBSERVE(implicit,     CXA_AK_IMPLICIT,     pf,
                     CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_OBSERVE(implicit,     CXA_AK_IMPLICIT,     ex,
                     CXA_DM_EVALUATION_EXCEPTION)

#undef CXA_NOEXCEPT_OBSERVE

// Specialized entry points: noexcept_enforce ([[noreturn]], D4298).

#define CXA_NOEXCEPT_ENFORCE(kind_name, kind_val, mode_name, mode_val)	   \
extern "C" [[noreturn]] void						   \
__cxa_contract_violation_##kind_name##_noexcept_enforce_##mode_name##_noexcept \
    (void* __data) noexcept						   \
{									   \
  try { __dispatch_with_override_core(					   \
	    __data, kind_val, CXA_ES_NOEXCEPT_ENFORCE, mode_val); }	   \
  catch (...) { std::terminate(); }					   \
  __builtin_unreachable();						   \
}

CXA_NOEXCEPT_ENFORCE(pre,    CXA_AK_PRE,    pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_ENFORCE(pre,    CXA_AK_PRE,    ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_ENFORCE(post,   CXA_AK_POST,   pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_ENFORCE(post,   CXA_AK_POST,   ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_ENFORCE(assert, CXA_AK_ASSERT, pf, CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_ENFORCE(assert, CXA_AK_ASSERT, ex, CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_ENFORCE(post_capture, CXA_AK_POST_CAPTURE, pf,
                     CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_ENFORCE(post_capture, CXA_AK_POST_CAPTURE, ex,
                     CXA_DM_EVALUATION_EXCEPTION)
CXA_NOEXCEPT_ENFORCE(implicit,     CXA_AK_IMPLICIT,     pf,
                     CXA_DM_PREDICATE_FALSE)
CXA_NOEXCEPT_ENFORCE(implicit,     CXA_AK_IMPLICIT,     ex,
                     CXA_DM_EVALUATION_EXCEPTION)

#undef CXA_NOEXCEPT_ENFORCE

// P3100: pure-virtual-call terminus variants (ub:class.abstract.pure.virtual).
//
// The vtable slot for a pure virtual is a plain void() function pointer.  When
// the class's implicit contract configuration (resolved where the vtable is
// emitted) selects a checking semantic, the compiler points that slot at one of
// these instead of the legacy __cxa_pure_virtual.  Each builds a generic
// implicit contract-violation data block ("pure virtual function called", with
// no per-site source location -- the terminus is shared across every pure
// virtual configured to this semantic) and dispatches it through the
// contract-violation handler.
//
// A pure-virtual call has no valid continuation (no function to run, no value
// to return), so every reacting variant ends by terminating; the one escape is
// a throwing handler on a non-noexcept pure virtual, which unwinds out through
// the caller.  The compiler selects the _noexcept variant when the pure
// virtual is declared noexcept, so such a throwing handler terminates at the
// noexcept boundary rather than escaping into a caller that assumed the call
// could not throw.  These termini are grouped here alongside the other
// __cxa_pure_virtual ABI symbols; the _noexcept ones need this file's
// terminate-on-throw barrier, and the throwing ones delegate to the pure-C
// libcontracts helpers.

// Each terminus builds the generic implicit violation ("pure virtual function
// called") and dispatches it through a libcontracts helper, which owns the sole
// data-block/descriptor construction shared with the P3290 C API -- no data
// block is hand-rolled here.  The throwing enforce/observe termini need no C++
// exception handling and use the pure-C helpers directly; the two _noexcept
// variants go through __c_contract_check_noexcept, whose dispatch is wrapped in
// this file's __contract_dispatch_core_noexcept terminate-on-throw barrier.

// __cxa_pure_virtual_quick: fast, silent termination -- no handler, no report.
extern "C" [[noreturn]] void
__cxa_pure_virtual_quick (void) noexcept
{
  __builtin_trap ();
}

// enforce (throwing): the handler runs; a throwing handler unwinds out through
// this frame to the caller, otherwise the enforcing core terminates via abort()
// after the handler returns (__c_contract_check_enforce is [[noreturn]]).
extern "C" [[noreturn]] void
__cxa_pure_virtual_enforce (void)
{
  __c_contract_check_enforce ("pure virtual function called", "", "", 0,
			      CXA_AK_IMPLICIT);
  __builtin_unreachable ();
}

// noexcept_enforce: as enforce, but a throwing handler terminates at the
// noexcept barrier (the pure virtual is noexcept, so it must not escape).
extern "C" [[noreturn]] void
__cxa_pure_virtual_noexcept_enforce (void) noexcept
{
  __c_contract_check_noexcept ("pure virtual function called", "", "", 0,
			       CXA_AK_IMPLICIT, CXA_ES_NOEXCEPT_ENFORCE);
  __builtin_unreachable ();
}

// observe (throwing): the handler runs and returns; a throwing handler unwinds
// out through this frame to the caller.  A pure-virtual call has no valid
// continuation, so if the handler returns normally we terminate via abort() --
// not std::terminate() (which would print a misleading "terminate called
// without an active exception").
extern "C" [[noreturn]] void
__cxa_pure_virtual_observe (void)
{
  __c_contract_check_observe ("pure virtual function called", "", "", 0,
			      CXA_AK_IMPLICIT);
  __builtin_abort ();
}

// noexcept_observe: as observe, but a throwing handler terminates at the
// noexcept barrier.
extern "C" [[noreturn]] void
__cxa_pure_virtual_noexcept_observe (void) noexcept
{
  __c_contract_check_noexcept ("pure virtual function called", "", "", 0,
			       CXA_AK_IMPLICIT, CXA_ES_NOEXCEPT_OBSERVE);
  // No valid continuation (see __cxa_pure_virtual_observe).
  __builtin_abort ();
}
