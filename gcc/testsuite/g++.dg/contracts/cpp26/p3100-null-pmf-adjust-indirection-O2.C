// P3100: a member function called through a pointer to member function on
// the object a null pointer points to, `(mp->*p) ()', `((*mp).*p) ()', is
// an indirection through the pointer, reported once under .indirection
// whatever the pointer to member function's `this' adjustment (here the
// offset of a second base, `&M::g2'), as with none (`&M::g1'); so is one
// on the element at a constant subscript of a null pointer, `(mp[1].*p) ()'
// (DECISIONS.md E97 (c)), or at a constant offset from it, `((mp + 1)->*p)
// ()' (DECISIONS.md E99 (a)).  A non-null pointer is not reported.  The
// forms are in p3100-null-pmf-adjust.h.
// With .indirection configured, at -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-pmf-adjust.h"
// { dg-output "arrow 1(\n|\r\n|\r)star 1(\n|\r\n|\r)sub 1(\n|\r\n|\r)" }
// { dg-output "arith 1(\n|\r\n|\r)arrow0 1(\n|\r\n|\r)star0 1(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
