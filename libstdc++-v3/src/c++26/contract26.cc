// -*- C++ -*- std::contracts::contract_violation and friends

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

#include <contracts>
#include <bits/contracts_abi.h>

#ifndef __cpp_lib_contracts
# error "This file requires C++26 contracts support to be enabled"
#endif

#if _GLIBCXX_HOSTED && _GLIBCXX_VERBOSE
# include <iostream>
# include <cxxabi.h>
#endif

static void
__handle_contract_violation_default
(const std::contracts::contract_violation &violation) noexcept
{
#if _GLIBCXX_HOSTED && _GLIBCXX_VERBOSE

  if (const char* __r = violation.report(); __r && __r[0] != '\0')
    std::cerr << __r << '\n';

  std::cerr << "contract violation in function "
    << violation.location().function_name()
    << " at " << violation.location().file_name() << ':'
    << violation.location().line()
    << ": " << violation.comment();

  const char* msg = violation.message();
  if (msg && msg[0] != '\0')
    std::cerr << " (" << msg << ")";

  const char* delimiter = "\n[";

  std::cerr << delimiter << "assertion_kind:";
   switch (violation.kind())
   {
     case std::contracts::assertion_kind::pre:
       std::cerr << " pre";
       break;
     case std::contracts::assertion_kind::post:
       std::cerr << " post";
       break;
     case std::contracts::assertion_kind::assert:
       std::cerr << " assert";
       break;
     case std::contracts::assertion_kind::manual:
       std::cerr << " manual";
       break;
     case std::contracts::assertion_kind::cassert:
       std::cerr << " cassert";
       break;
     case std::contracts::assertion_kind::post_capture:
       std::cerr << " post_capture";
       break;
     case std::contracts::assertion_kind::implicit:
       std::cerr << " implicit";
       break;
     default:
       std::cerr << " unknown(" << (int) violation.kind() << ")";
   }
   delimiter = ", ";

  std::cerr << delimiter << "semantic:";
  switch (violation.semantic())
  {
    case std::contracts::evaluation_semantic::observe:
      std::cerr << " observe";
      break;
    case std::contracts::evaluation_semantic::enforce:
      std::cerr << " enforce";
      break;
    case std::contracts::evaluation_semantic::quick_enforce:
      std::cerr << " quick_enforce";
      break;
    case std::contracts::evaluation_semantic::assume:
      std::cerr << " assume";
      break;
    case std::contracts::evaluation_semantic::noexcept_observe:
      std::cerr << " noexcept_observe";
      break;
    case std::contracts::evaluation_semantic::noexcept_enforce:
      std::cerr << " noexcept_enforce";
      break;
    default:
      std::cerr << " unknown(" << (int) violation.semantic() << ")";
  }
  delimiter = ", ";

  std::cerr << delimiter << "mode:";
  switch (violation.detection_mode())
  {
    case std::contracts::detection_mode::predicate_false:
      std::cerr << " predicate_false";
      break;
    case std::contracts::detection_mode::evaluation_exception:
      std::cerr << " evaluation_exception";
      break;
    case std::contracts::detection_mode::unspecified:
      std::cerr << " unspecified";
      break;
    default:
      std::cerr << " unknown(" << (int) violation.detection_mode() << ")";
  }
  delimiter = ", ";

  if (violation.detection_mode()
      == std::contracts::detection_mode::evaluation_exception)
    {
      /* Based on the impl. in vterminate.cc.  */
      std::type_info *t = __cxxabiv1::__cxa_current_exception_type();
      if (t)
	{
	  int status = -1;
	  char *dem = 0;
	  // Note that "name" is the mangled name.
	  char const *name = t->name();
	  dem = __cxxabiv1::__cxa_demangle(name, 0, 0, &status);
	  std::cerr << ": threw an instance of '";
	  std::cerr << ( status == 0 ? dem : name) << "'";
	}
      else
	std::cerr << ": threw an unknown type";
    }

  std::cerr << delimiter << "terminating:"
	    << (violation.is_terminating () ? " yes" : " no");

  if (delimiter[0] == ',')
    std::cerr << ']';

  std::cerr << std::endl;
#endif
}

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

namespace contracts
{

assertion_kind
contract_violation::kind() const noexcept
{
  using namespace __cxxabiv1;
  auto __raw = __cxa_find_field_value<__UINT8_TYPE__>(
      _M_chain, CXA_FIELD_ASSERTION_KIND, CXA_AK_UNSPECIFIED);
  return static_cast<assertion_kind>(__raw);
}

evaluation_semantic
contract_violation::semantic() const noexcept
{
  using namespace __cxxabiv1;
  auto __raw = __cxa_find_field_value<__UINT8_TYPE__>(
      _M_chain, CXA_FIELD_EVALUATION_SEMANTIC, CXA_ES_UNSPECIFIED);
  return static_cast<evaluation_semantic>(__raw);
}

contracts::detection_mode
contract_violation::detection_mode() const noexcept
{
  using namespace __cxxabiv1;
  auto __raw = __cxa_find_field_value<__UINT8_TYPE__>(
      _M_chain, CXA_FIELD_DETECTION_MODE, CXA_DM_UNSPECIFIED);
  return static_cast<contracts::detection_mode>(__raw);
}

const char*
contract_violation::comment() const noexcept
{
  using namespace __cxxabiv1;
  auto __p = __cxa_find_field_ptr<const char*>(
      _M_chain, CXA_FIELD_COMMENT);
  return __p ? *__p : "";
}

const char*
contract_violation::message() const noexcept
{
  using namespace __cxxabiv1;
  auto __p = __cxa_find_field_ptr<const char*>(
      _M_chain, CXA_FIELD_MESSAGE);
  // Unlike comment(), a missing message field yields nullptr, not "": P3099
  // "Option C1" keeps "no message supplied" (nullptr) distinct from an empty
  // message (""), so a violation with no diagnostic-message field at all (a
  // P3290 API/C-assert violation, or a C++26 TU) reports nullptr.
  return __p ? *__p : nullptr;
}

const char*
contract_violation::report() const noexcept
{
  using namespace __cxxabiv1;
  auto __p = __cxa_find_field_ptr<__cxa_contract_report_populator>(
      _M_chain, CXA_FIELD_REPORT);
  if (!__p || !__p->populate)
    return nullptr;
  // D4301: report() is noexcept, but a populator may allocate and could
  // throw.  Guard it so a throwing populator cannot escape.  The literal
  // has static storage duration, so returning it as const char* is safe.
  try
    {
      return __p->populate(__p->ctx);
    }
  catch (...)
    {
      return "Error generating report";
    }
}

std::source_location
contract_violation::location() const noexcept
{
  using namespace __cxxabiv1;
  auto __p = __cxa_find_field_ptr<__cxa_source_location>(
      _M_chain, CXA_FIELD_SOURCE_LOCATION);
  if (__p)
    {
      std::source_location __loc;
      __loc._M_impl
	  = reinterpret_cast<const std::source_location::__impl*>(__p);
      return __loc;
    }
  return std::source_location{};
}

bool
contract_violation::is_terminating() const noexcept
{
  auto __s = semantic();
  return __s == evaluation_semantic::enforce
      || __s == evaluation_semantic::quick_enforce
      || __s == evaluation_semantic::noexcept_enforce;
}

void*
contract_violation::query_control_object(const void* __key,
					 std::size_t __index) const
{
  using namespace __cxxabiv1;
  auto __query_fn = __cxa_find_field_value<__cxa_query_fn_t>(
      _M_chain, CXA_FIELD_QUERY_FUNCTION, nullptr);
  auto __label_ptr = __cxa_find_field_value<const void*>(
      _M_chain, CXA_FIELD_LABEL_PTR, nullptr);
  if (__query_fn && __label_ptr)
    return __query_fn(__label_ptr, __key, __index);
  return nullptr;
}

void
invoke_default_contract_violation_handler
(const std::contracts::contract_violation& violation) noexcept
{
  return __handle_contract_violation_default(violation);
}

}
}

__attribute__ ((weak)) void
handle_contract_violation (const std::contracts::contract_violation &violation)
{
  return __handle_contract_violation_default(violation);
}

extern "C" __attribute__ ((weak)) void
__handle_contract_violation
    (const std::contracts::contract_violation &violation)
{
  return __handle_contract_violation_default(violation);
}

// C-linkage always-default entry point (contracts ABI spec section 8.4).  This
// is the C form of std::contracts::invoke_default_contract_violation_handler:
// it always invokes the implementation default handler, bypassing any user
// replacement.  libcontracts (pure C) calls it for its dispatch fallback, and
// C violation handlers can call it to emit the default diagnostics.  The
// argument is a pointer to a contract_violation object (ABI-identical to a
// const reference).
extern "C" void
__contract_invoke_default_handler
    (const std::contracts::contract_violation &violation)
{
  std::contracts::invoke_default_contract_violation_handler(violation);
}

#if _GLIBCXX_INLINE_VERSION
// The compiler expects the contract_violation class to be in an unversioned
// namespace, so provide a forwarding function with the expected symbol name.
extern "C" void
_Z25handle_contract_violationRKNSt9contracts18contract_violationE
(const std::contracts::contract_violation &violation)
{ handle_contract_violation(violation); }

extern "C" void
_Z27__handle_contract_violationRKNSt9contracts18contract_violationE
(const std::contracts::contract_violation &violation)
{ __handle_contract_violation(violation); }

extern "C" void
_Z41invoke_default_contract_violation_handlerRKNSt9contracts18contract_violationE
(const std::contracts::contract_violation &violation)
{ invoke_default_contract_violation_handler(violation); }

#endif

#ifdef __cpp_lib_contracts_api

#include <new>
#include <exception>
#include <cstdlib>

// Data block type and descriptor for P3290 manual violations and C assert.
// Shared between the std::contracts API functions and
// __cxa_handle_cassert_violation.
namespace {
using namespace __cxxabiv1;

struct __p3290_data_block_t {
  const __cxa_descriptor_table_t* descriptor;
  const __cxa_contract_data_block* next;
  __cxa_source_location location;
  const char* comment;
  __UINT8_TYPE__ kind;
  __UINT8_TYPE__ semantic;
  __UINT8_TYPE__ mode;
};

struct __p3290_desc_t {
  __UINT8_TYPE__ header;
  __UINT8_TYPE__ num_entries;
  __UINT8_TYPE__ fid[5];
  __UINT8_TYPE__ pad[1];
  __cxa_descriptor_data_t data[5];
};

// A P3290 API violation and a C assert carry no diagnostic-message field, so
// contract_violation::message() returns nullptr for them (P3099 "Option C1": no
// message supplied is distinct from an empty message).
static const __p3290_desc_t __p3290_desc = {
  static_cast<__UINT8_TYPE__>((1u << 4) | CXA_VENDOR_GCC),
  5,
  { CXA_FIELD_SOURCE_LOCATION, CXA_FIELD_COMMENT,
    CXA_FIELD_ASSERTION_KIND, CXA_FIELD_EVALUATION_SEMANTIC,
    CXA_FIELD_DETECTION_MODE },
  { 0 },
  {
    { offsetof(__p3290_data_block_t, location) },
    { offsetof(__p3290_data_block_t, comment) },
    { offsetof(__p3290_data_block_t, kind) },
    { offsetof(__p3290_data_block_t, semantic) },
    { offsetof(__p3290_data_block_t, mode) },
  }
};
} // anonymous namespace

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION
namespace contracts
{

namespace {

static void
__do_handle_violation(const char* __comment,
		      const std::source_location& __location,
		      __UINT8_TYPE__ __semantic)
{
  __p3290_data_block_t __data = {
    reinterpret_cast<const __cxa_descriptor_table_t*>(&__p3290_desc),
    nullptr,
    { __location.file_name(), __location.function_name(),
      __location.line(), __location.column() },
    __comment ? __comment : "",
    CXA_AK_MANUAL,
    __semantic,
    CXA_DM_UNSPECIFIED,
  };
  __cxa_contract_violation(
      const_cast<void*>(static_cast<const void*>(&__data)));
}

} // anonymous namespace

[[noreturn]] void
handle_enforced_contract_violation(
    const char* __comment,
    const std::source_location& __location)
{
  __do_handle_violation(__comment, __location, CXA_ES_ENFORCE);
  // Enforced dispatch aborts on normal handler return (and a throwing handler
  // propagates out of this throwing overload); this abort() is an unreachable
  // backstop consistent with the enforce-terminates-via-abort() policy.
  std::abort();
}

void
handle_observed_contract_violation(
    const char* __comment,
    const std::source_location& __location)
{
  __do_handle_violation(__comment, __location, CXA_ES_OBSERVE);
}

[[noreturn]] void
handle_quick_enforced_contract_violation(
    [[maybe_unused]] const char* __comment,
    [[maybe_unused]] const std::source_location& __location) noexcept
{
  // Quick-enforce terminates immediately without invoking the handler, in the
  // most efficient implementation-defined way (__builtin_trap, not abort).
  __builtin_trap();
}

// D4298: the nothrow_t overloads come in two variants, and which one a
// caller binds to is decided in the caller's own translation unit by whether
// -fcontracts-p4298 (i.e. __cpp_contracts_nonthrowing) was set -- see the
// header <contracts>.  The library provides BOTH, unconditionally (this TU is
// built once): the plain std::contracts variants report the classic
// enforce/observe semantics, and the std::contracts::__p4298 variants report
// the noexcept_enforce/noexcept_observe semantics.  Each mangled name has
// exactly one definition, so there is no ODR issue for a mixed-flag program.

// Plain variants (caller compiled WITHOUT -fcontracts-p4298): classic
// enforce/observe.
[[noreturn]] void
handle_enforced_contract_violation(
    std::nothrow_t,
    const char* __comment,
    const std::source_location& __location) noexcept
{
  // Normal handler return aborts inside the ABI (enforce post-action); a
  // throwing handler hits this noexcept boundary and calls std::terminate().
  __do_handle_violation(__comment, __location, CXA_ES_ENFORCE);
  std::abort();
}

void
handle_observed_contract_violation(
    std::nothrow_t,
    const char* __comment,
    const std::source_location& __location) noexcept
{
  __do_handle_violation(__comment, __location, CXA_ES_OBSERVE);
}

// P4298 variants (caller compiled WITH -fcontracts-p4298): report the
// noexcept_enforce/noexcept_observe semantics.  Same runtime behavior
// otherwise -- normal return still aborts for enforce; a throwing handler
// still terminates at the noexcept boundary.
inline namespace __p4298
{
  [[noreturn]] void
  handle_enforced_contract_violation(
      std::nothrow_t,
      const char* __comment,
      const std::source_location& __location) noexcept
  {
    __do_handle_violation(__comment, __location, CXA_ES_NOEXCEPT_ENFORCE);
    std::abort();
  }

  void
  handle_observed_contract_violation(
      std::nothrow_t,
      const char* __comment,
      const std::source_location& __location) noexcept
  {
    __do_handle_violation(__comment, __location, CXA_ES_NOEXCEPT_OBSERVE);
  }
} // inline namespace __p4298

} // namespace contracts
_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std

// Shared assert-integration entry point (P3290).  Both libstdc++ and libc++
// expand the <cassert> `assert` macro (under
// __STDC_WANT_ASSERT_USES_CONTRACTS__) to a call to this single symbol.  It
// reports evaluation_semantic=enforce and
// assertion_kind=cassert to the handler.  Unlike the general contract entry
// points, *any* completion of the handler results in std::abort(): a normal
// return aborts via the enforce post-action inside the ABI, and an escaping
// exception is caught here and turned into std::abort() as well.  This matches
// classic assert() termination semantics regardless of how the handler exits.
extern "C++" [[noreturn]] void
__cxa_handle_cassert_violation(const char* __comment,
                               std::source_location __location) noexcept
{
  using namespace __cxxabiv1;

  __p3290_data_block_t __data = {
    reinterpret_cast<const __cxa_descriptor_table_t*>(&__p3290_desc),
    nullptr,
    { __location.file_name(), __location.function_name(),
      __location.line(), __location.column() },
    __comment ? __comment : "",
    CXA_AK_CASSERT,
    CXA_ES_ENFORCE,
    CXA_DM_PREDICATE_FALSE,
  };
  try
    {
      __cxa_contract_violation(
	  const_cast<void*>(static_cast<const void*>(&__data)));
    }
  catch (...)
    {
      std::abort();
    }
  std::abort();
}

#endif // __cpp_lib_contracts_api
