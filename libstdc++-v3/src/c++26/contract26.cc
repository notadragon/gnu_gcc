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

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION
namespace contracts
{

namespace {

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std

