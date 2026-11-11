// P3100: a member call through a null pointer is checked once, under
// .indirection for a non-virtual function and under .access for a
// virtual one, devirtualized through a `final' class or not
// (DECISIONS.md E102 (a), E110 (a)) -- also one returning a class with a
// non-trivial copy constructor or destructor by value, an AGGR_INIT_EXPR
// until gimplification (GCC-854: not reported unless it read a vtable
// pointer; fixed for the P3100 check, DECISIONS.md E113 (b), while stock
// -fsanitize=null's miss stays open upstream).  As Clang.  The forms are
// in p3100-null-member-call-class-return.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-member-call-class-return.cpp
// (its -fsanitize=null runs: g++.dg/ubsan/open-bug-null-member-call-class-return.C).
// With only .indirection configured, at -O2.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-member-call-class-return.h"
// { dg-output "scls 1(\n|\r\n|\r)sint 1(\n|\r\n|\r)vcls 0(\n|\r\n|\r)" }
// { dg-output "vint 0(\n|\r\n|\r)fcls 0(\n|\r\n|\r)fint 0(\n|\r\n|\r)" }
// { dg-output "subcls 1(\n|\r\n|\r)memcls 1(\n|\r\n|\r)fmemcls 0(\n|\r\n|\r)" }
// { dg-output "qcls 1(\n|\r\n|\r)starcls 1(\n|\r\n|\r)refc 1(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
