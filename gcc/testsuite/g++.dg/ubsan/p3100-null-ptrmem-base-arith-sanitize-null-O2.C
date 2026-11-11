// P3100: with -fsanitize=null routed to the contract-violation handler as
// well as ub:expr.unary.dereference.nullptr configured, `(a + i)->*pm'
// through a conversion to the base class whose member the pointer to
// member is, a a null pointer, is reported once: a pointer to data member
// read or its address taken, a call through a pointer to member function.
// The forms are in ../contracts/cpp26/p3100-null-ptrmem-base-arith.h, a
// mirror of Clang's Runnable/p3100-null-ptrmem-base-arith.cpp (CLANG-801).
// With the whole group configured, at -O2.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fsanitize=null -fsanitize-semantic=null:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "../contracts/cpp26/p3100-null-ptrmem-base-arith.h"

// { dg-output "mwrd 1(\n|\r\n|\r)mvrd 1(\n|\r\n|\r)dsrd 1(\n|\r\n|\r)" }
// { dg-output "dsaddr 1(\n|\r\n|\r)mvaddr 1(\n|\r\n|\r)mvpmfv 1(\n|\r\n|\r)" }
// { dg-output "mvpmfnv 1(\n|\r\n|\r)mwrd0 1(\n|\r\n|\r)mvrd0 1(\n|\r\n|\r)" }
// { dg-output "srd 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
