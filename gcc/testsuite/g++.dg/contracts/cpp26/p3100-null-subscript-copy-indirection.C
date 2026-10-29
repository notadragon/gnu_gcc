// P3100: an array element copied through a null pointer -- an element of
// the pointer (`s = sp[1]', `S s2 = sp[1]', by value, returned, moved) or
// of a member array (`s = hp->sa[1]', the first element at the object's
// address).  A copy of a class reads the element, so the null pointer is
// reported once under .access; a copy of an empty class reads no object,
// .indirection (DECISIONS.md E86 (a)); a copy of an empty virtual base of
// the element reads the base's offset through it, .access (DECISIONS.md E94
// (b)).  An element of a null pointer at a constant subscript is checked
// on the pointer itself (DECISIONS.md E97 (c)): copied, an empty class
// copied from `ep[1]', or read or stored to through a reference cast
// (`static_cast<int &> (p[1])').  A non-null pointer is not
// reported.  The forms are in p3100-null-subscript-copy.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-subscript-copy.cpp.
// With only .indirection configured (-O0).
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-subscript-copy.h"
// { dg-output "ssub_from 0(\n|\r\n|\r)ssub_copy 0(\n|\r\n|\r)ssub_arg 0(\n|\r\n|\r)" }
// { dg-output "ssub_ret 0(\n|\r\n|\r)ssub_move 0(\n|\r\n|\r)ssub0_from 0(\n|\r\n|\r)" }
// { dg-output "sarray_from 0(\n|\r\n|\r)sarray_copy 0(\n|\r\n|\r)sarray0_from 0(\n|\r\n|\r)" }
// { dg-output "sarray0_copy 0(\n|\r\n|\r)esub_from 1(\n|\r\n|\r)esub_copy 1(\n|\r\n|\r)" }
// { dg-output "esub_arg 1(\n|\r\n|\r)earray0_from 1(\n|\r\n|\r)earray0_copy 1(\n|\r\n|\r)" }
// { dg-output "vsub_from 0(\n|\r\n|\r)hvarray_from 0(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
// { dg-output "isub_cast 0(\n|\r\n|\r)isub_cast_store 0(\n|\r\n|\r)nonnull_extra 0(\n|\r\n|\r)" }
