// P3100: the forms of p3100-null-function-indirection.h under the
// .indirection leaf alone, at -O2: each call through `*fp' and each
// reference bound to `*fp' with `fp' null is reported once, as under the
// whole group (p3100-null-function-indirection.C).
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-function-indirection.cpp.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-function-indirection.h"
// { dg-output "call 1(\n|\r\n|\r)call_parens 1(\n|\r\n|\r)call_twice 1(\n|\r\n|\r)" }
// { dg-output "tcall 1(\n|\r\n|\r)call_cast 1(\n|\r\n|\r)call_ccast 1(\n|\r\n|\r)" }
// { dg-output "call_rvcast 1(\n|\r\n|\r)call_cond 1(\n|\r\n|\r)call_comma 1(\n|\r\n|\r)" }
// { dg-output "bind 1(\n|\r\n|\r)bind_ret 1(\n|\r\n|\r)bind_arg 1(\n|\r\n|\r)" }
// { dg-output "bind_cast 1(\n|\r\n|\r)bind_rv 1(\n|\r\n|\r)bind_cond 1(\n|\r\n|\r)" }
// { dg-output "bind_comma 1(\n|\r\n|\r)bind_rcast 1(\n|\r\n|\r)call_rcast 1(\n|\r\n|\r)" }
// { dg-output "call_plain 0(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
