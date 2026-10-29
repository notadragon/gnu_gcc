// P3100: a call through a null pointer to a member function that is not
// inlined, with -fsanitize=null routed to the contract-violation handler
// as well as the P3100 null check, is reported once: the P3100 check at
// the call replaces the sanitizer's (DECISIONS.md E111 (a)), and GCC makes
// no check of `this' in the member function's prologue.  Clang's stock
// sanitizer does, and reports it again at -O0 (CLANG-792, DECISIONS.md
// E114, open).  At -O2 the handler's effects are seen by the caller
// (GCC-857).  The forms are in p3100-null-member-call-prologue.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/p3100-null-member-call-prologue.cpp.
// At -O0.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fsanitize=null -fsanitize-semantic=null:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-member-call-prologue.h"

// { dg-output "plain 1(\n|\r\n|\r)poly 1(\n|\r\n|\r)ptrnv 1(\n|\r\n|\r)" }
// { dg-output "fin 1(\n|\r\n|\r)" }
