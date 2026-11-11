// P3100: the null check of a reference bound to `*p', p a null pointer,
// where the binding has no location of its own -- an implicit conversion
// to the reference: a variable, a call's argument, the operand of an
// overloaded operator, a mem-initializer, a default member initializer,
// an init-capture, a structured binding, a range-based for (the range and
// each element), a return -- reports the file and line of the binding in
// contract_violation::location(), as do the conversion to a virtual base
// bound so (.access, DECISIONS.md E94 (b)) and the alignment check of a
// misaligned `*p' bound so (.store).  GCC-874: an empty file name and line
// 0; the check is built at the location of the innermost expression around
// the binding that has one.  The forms are in
// p3100-null-binding-location.h; each line is "form reports good".
// With the whole group configured, at -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-binding-location.h"

// { dg-output "var 1 1(\n|\r\n|\r)fin 1 1(\n|\r\n|\r)mem 1 1(\n|\r\n|\r)down 1 1(\n|\r\n|\r)" }
// { dg-output "arg 1 1(\n|\r\n|\r)cmp 1 1(\n|\r\n|\r)minit 1 1(\n|\r\n|\r)nsdmi 1 1(\n|\r\n|\r)" }
// { dg-output "cap 1 1(\n|\r\n|\r)sbind 1 1(\n|\r\n|\r)rfor 2 2(\n|\r\n|\r)cref 1 1(\n|\r\n|\r)" }
// { dg-output "paren 1 1(\n|\r\n|\r)ret 1 1(\n|\r\n|\r)vbase 1 1(\n|\r\n|\r)ebase 1 1(\n|\r\n|\r)" }
// { dg-output "avar 0 0(\n|\r\n|\r)aarg 0 0(\n|\r\n|\r)aparen 0 0(\n|\r\n|\r)nonnull 0 0(\n|\r\n|\r)" }
