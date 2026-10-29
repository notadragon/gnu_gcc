// P3100: a constexpr call that is not manifestly constant-evaluated, and
// that GCC folds when optimizing (an `if' condition), is not folded when it
// performs an indirection through a null pointer whose result is not
// accessed, so the run-time check stays and reports once (GCC-804): the
// forms of p3100-null-unaccessed-indirection-fold.h, among them `sp->E'
// (an enumerator) and `&pp[0][1]' (whose check the ubsan pass builds after
// the fold).  Manifestly constant evaluation is unchanged: the
// static_asserts with a pointer to an object hold, and a constexpr
// variable is constant-initialized with no report.  `__builtin_constant_p'
// of such a call is 0.  This file is -O0;
// p3100-null-unaccessed-indirection-fold-O2.C is -O2.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-unaccessed-indirection-fold.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-Wno-unused-value" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-unaccessed-indirection-fold.h"
// { dg-output "sfc 1(\n|\r\n|\r)sfc_star 1(\n|\r\n|\r)sck 1(\n|\r\n|\r)" }
// { dg-output "se 1(\n|\r\n|\r)skaddr 1(\n|\r\n|\r)addr 1(\n|\r\n|\r)" }
// { dg-output "addr0 1(\n|\r\n|\r)disc 1(\n|\r\n|\r)faddr 1(\n|\r\n|\r)" }
// { dg-output "fdisc 1(\n|\r\n|\r)maddr 1(\n|\r\n|\r)aaddr 1(\n|\r\n|\r)" }
// { dg-output "paddr 1(\n|\r\n|\r)obind 1(\n|\r\n|\r)fbind 1(\n|\r\n|\r)" }
// { dg-output "bcp 0(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
