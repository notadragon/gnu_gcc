// P3100: a store to an element of an array of empty class objects through
// a null pointer -- of a member array (`hp->ea[1] = e', `(*hp).ea[1] = e',
// a two-dimensional one, a move, `hp->ea[1].operator= (e)', the first
// element at the object's address) or of the pointer itself (`ep[0] = e')
// -- writes no object, so the indirection is reported once under
// .indirection (DECISIONS.md E86 (a)), as `hp->e = e' is.  `ep[1] = e', at
// a constant subscript, is checked on ep (DECISIONS.md E97 (c)).  A
// non-null pointer is not reported.  The forms are in
// p3100-null-empty-array-store.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-empty-array-store.cpp.
// With only .access configured (-O0).
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-empty-array-store.h"
// { dg-output "earray_store 0(\n|\r\n|\r)earray_store_star 0(\n|\r\n|\r)earray2_store 0(\n|\r\n|\r)" }
// { dg-output "earray_move_store 0(\n|\r\n|\r)earray_call 0(\n|\r\n|\r)earray0_store 0(\n|\r\n|\r)" }
// { dg-output "eptr_store 0(\n|\r\n|\r)eptr0_store 0(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
