// P3100: the forms of p3100-null-function-indirection.h under the .access
// leaf alone: an indirection through a null pointer to function accesses
// nothing, so no form is reported (the calls fault, caught as SIGSEGV).
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-function-indirection.cpp.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-function-indirection.h"
// { dg-output "call 0(\n|\r\n|\r)call_parens 0(\n|\r\n|\r)call_twice 0(\n|\r\n|\r)" }
// { dg-output "tcall 0(\n|\r\n|\r)call_cast 0(\n|\r\n|\r)call_ccast 0(\n|\r\n|\r)" }
// { dg-output "call_rvcast 0(\n|\r\n|\r)call_cond 0(\n|\r\n|\r)call_comma 0(\n|\r\n|\r)" }
// { dg-output "bind 0(\n|\r\n|\r)bind_ret 0(\n|\r\n|\r)bind_arg 0(\n|\r\n|\r)" }
// { dg-output "bind_cast 0(\n|\r\n|\r)bind_rv 0(\n|\r\n|\r)bind_cond 0(\n|\r\n|\r)" }
// { dg-output "bind_comma 0(\n|\r\n|\r)bind_rcast 0(\n|\r\n|\r)call_rcast 0(\n|\r\n|\r)" }
// { dg-output "call_plain 0(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
