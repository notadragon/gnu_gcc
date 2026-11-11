// P3100: an indirection through a null pointer to function whose result is
// called, `(*fp) ()' (also `(*(fp)) ()', `(**fpp) ()', in a template, and
// through `static_cast<F &>', `(F &)', `static_cast<F &&>', a conditional
// or a comma expression), or bound to a reference, `F &r = *fp' (a local,
// a returned reference, an argument, through `static_cast<F &>', an rvalue
// reference, a conditional, a comma expression), is reported once under
// ub:expr.unary.dereference.nullptr (GCC-800 for the calls), as a reference
// bound to `*p' for an object is; so is `reinterpret_cast<F &> (*gp)',
// bound or called (Clang checks neither, nor the conditional and comma
// forms).  A plain call `fp ()', which names no indirection, is not
// checked.  A non-null pointer, a function named directly (also chosen by
// a conditional) and a reference to a function report nothing.  The forms
// are in p3100-null-function-indirection.h; -indirection.C runs them under
// the leaf at -O2, -access.C under .access, which reports none.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-function-indirection.cpp.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-function-indirection.h"
// { dg-output "call 1(\n|\r\n|\r)call_parens 1(\n|\r\n|\r)call_twice 1(\n|\r\n|\r)" }
// { dg-output "tcall 1(\n|\r\n|\r)call_cast 1(\n|\r\n|\r)call_ccast 1(\n|\r\n|\r)" }
// { dg-output "call_rvcast 1(\n|\r\n|\r)call_cond 1(\n|\r\n|\r)call_comma 1(\n|\r\n|\r)" }
// { dg-output "bind 1(\n|\r\n|\r)bind_ret 1(\n|\r\n|\r)bind_arg 1(\n|\r\n|\r)" }
// { dg-output "bind_cast 1(\n|\r\n|\r)bind_rv 1(\n|\r\n|\r)bind_cond 1(\n|\r\n|\r)" }
// { dg-output "bind_comma 1(\n|\r\n|\r)bind_rcast 1(\n|\r\n|\r)call_rcast 1(\n|\r\n|\r)" }
// { dg-output "call_plain 0(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
