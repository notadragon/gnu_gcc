// P3100: a class copied from the element one past the end of an array of
// known bound (`S s2 = sa[N]', `s = sa[N]', by value) reads it, so it is
// ub:expr.add.out.of.bounds.known.subscript, as a read of a scalar element
// and a store to the element are, reported once, and not
// ub:expr.unary.dereference.invalid.known.  An empty class copied from or
// stored to the element (`E e2 = ea[N]', `ea[N] = e', in parentheses, a
// subscript of known-bound pointer arithmetic `(ea + 1)[i]', by value,
// returned, moved, move-stored, `ea[N].operator= (e)') reads or writes no
// object, so it is the indirection past the end, .invalid.known, as
// `&ea[N]' is (DECISIONS.md E95 (a)), reported once; one further past
// (`ea[N + 1]') it is checked as `&ea[N + 1]' is, reported once under
// either leaf.  So is an empty base of a non-empty element copied from or
// stored to (`EB b = da[N]', `static_cast<EB &> (da[N]) = b'; DECISIONS.md
// E98 (a)).  An element in range is not reported.
// The forms are in p3100-array-bounds-copy.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-subscript-copy-bounds.cpp.
// With ub:expr.unary.dereference.invalid.known configured (-O0).
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-array-bounds-copy.h"
// { dg-output "scopy 0(\n|\r\n|\r)sfrom 0(\n|\r\n|\r)sarg 0(\n|\r\n|\r)" }
// { dg-output "sstore 0(\n|\r\n|\r)iread 0(\n|\r\n|\r)ecopy 1(\n|\r\n|\r)" }
// { dg-output "estore 1(\n|\r\n|\r)eparenstore 1(\n|\r\n|\r)bcopy 1(\n|\r\n|\r)" }
// { dg-output "bstore 1(\n|\r\n|\r)earithcopy 1(\n|\r\n|\r)earithstore 1(\n|\r\n|\r)" }
// { dg-output "inrange 0(\n|\r\n|\r)earg 1(\n|\r\n|\r)eret 1(\n|\r\n|\r)" }
// { dg-output "emove 1(\n|\r\n|\r)emovestore 1(\n|\r\n|\r)ecall 1(\n|\r\n|\r)" }
// { dg-output "ecopy3 1(\n|\r\n|\r)estore3 1(\n|\r\n|\r)inrange_extra 0(\n|\r\n|\r)" }
