// P3100: a subscript of the array a pointer to an array points to,
// `(*pa)[i]' with `pa' null, is an indirection through `pa', checked as
// `p[i]' checks `p' (DECISIONS.md E83 (c), E97 (c)): once, whatever `i'
// is -- under .access where the element is read or stored, under
// .indirection where a reference is bound to it or its address alone is
// formed.  The mirror of llvm_llvm-project's
// Runnable/p3100-null-subscript-deref-array.cpp (CLANG-813, where a
// nonzero `i' was not checked); passing on GCC before any change.  The
// forms are in p3100-null-subscript-deref-array.h.
// With .access configured, at -O0.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-subscript-deref-array.h"

// { dg-output "bind1 0(\n|\r\n|\r)bindi 0(\n|\r\n|\r)bindp 0(\n|\r\n|\r)" }
// { dg-output "read1 1(\n|\r\n|\r)readi 1(\n|\r\n|\r)read0 1(\n|\r\n|\r)" }
// { dg-output "store1 1(\n|\r\n|\r)addr1 0(\n|\r\n|\r)dd 1(\n|\r\n|\r)" }
// { dg-output "md 1(\n|\r\n|\r)sub01 1(\n|\r\n|\r)sub01a 0(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
