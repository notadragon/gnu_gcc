// P3100: a call through a pointer to member function of a polymorphic
// class on a member subobject of the object a null pointer points to,
// `(hp->m.*p) ()', checks the pointer once (DECISIONS.md E102 (a)); with
// -fsanitize=vptr, the call's vptr check of the subobject follows that
// check and skips a null pointer, so the null dereference is reported once
// before the program faults.  A call on `*vp' itself is the control.  With
// -fsanitize=null too, each is still reported once (DECISIONS.md
// E111 (a)), the call through a pointer to a non-virtual member function
// on `*vp' at -O0 as well: GCC makes no check of `this' in the member
// function's prologue (Clang reports that one twice at -O0, CLANG-792,
// DECISIONS.md E114, open).  The forms are in p3100-null-pmf-member-vptr.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-pmf-member-vptr.cpp.
// With -fsanitize=vptr, at -O2.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-pmf-member-vptr.h"

// { dg-output "ptrv 1(\n|\r\n|\r)ptrnv 1(\n|\r\n|\r)memv 1(\n|\r\n|\r)" }
// { dg-output "memnv 1(\n|\r\n|\r)memarrv 1(\n|\r\n|\r)memarrnv 1(\n|\r\n|\r)" }
// { dg-output "memptrv 1(\n|\r\n|\r)memptrnv 1(\n|\r\n|\r)dmemv 1(\n|\r\n|\r)" }
// { dg-output "dmemnv 1(\n|\r\n|\r)basememv 1(\n|\r\n|\r)basememnv 1(\n|\r\n|\r)" }
// { dg-output "submemv 1(\n|\r\n|\r)submemnv 1(\n|\r\n|\r)" }
