// P3100: the alignment check of a reference binding
// (ub:basic.align.object.alignment.store) reports a reference bound to
// `*(int *) (a + o)', a an array of char and o an offset that misaligns
// it -- a global or local array, a variable or a call's argument -- as
// -fsanitize=alignment does.  GCC-876: the front end folded the binding
// into `(int &) &a + o' and checked the address of the array alone;
// under -fcontracts-p3100 fold_unary_loc keeps the conversion of the sum
// in GENERIC, as for -fsanitize=alignment.  The address of an element and
// a pointer plus an offset are the controls.  The forms are in
// p3100-align-binding-arith.h.
// With the group configured, at -O0.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-align-binding-arith.h"

// { dg-output "garr 1(\n|\r\n|\r)larr 1(\n|\r\n|\r)garg 1(\n|\r\n|\r)" }
// { dg-output "gsub 1(\n|\r\n|\r)gptr 1(\n|\r\n|\r)aligned 0(\n|\r\n|\r)" }
