// Watch test for GCC-904 (deferred, DECISIONS.md E136): whether a call is
// checked on the caller's side (P3595 caller-side configuration) is decided
// per translation unit, and the refinable-body mark covers a function only
// where it calls a contract wrapper.  An inline function compiled where its
// calls are not checked has no wrapper and is not marked, so its callers
// there use facts from that body, although another unit's definition, the
// one the linker keeps, checks on the caller's side with a violation
// handler that throws.
//
// open-bug-caller-side-unwrapped-u.cc: f ignores its precondition, so it
// cannot throw; g calls f with no caller-side check, so g cannot throw
// either, and use_g, compiled there, drops its handler.
// open-bug-caller-side-unwrapped-k.cc, linked first: the g the linker
// keeps, which checks f's precondition caller-side with enforce.  The
// handler throws, so use_g (-5) must catch the exception and return -1;
// GCC calls std::terminate instead.  -fno-ipa-sra: otherwise use_g could
// call a local clone of its own unit's g, which would hide the problem.
//
// Mirror: none (Clang has no caller-side checking, so E136 has no Clang
// form).  The P3097 shape, which passes in both compilers, is
// refinable-body-p3097-unit-config.C here and, in the llvm_llvm-project
// fork, clang/test/Contracts/Runnable/refinable-body-p3097-unit-config.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -O2 -fno-ipa-sra" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/open-bug-caller-side-unwrapped.json" }
// { dg-additional-sources "open-bug-caller-side-unwrapped-k.cc open-bug-caller-side-unwrapped-u.cc" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-xfail-run-if "GCC-904" { *-*-* } }

#include <contracts>

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  throw 42;
}

int use_g (int);

int
main ()
{
  if (use_g (-5) != -1)
    __builtin_abort ();
}
