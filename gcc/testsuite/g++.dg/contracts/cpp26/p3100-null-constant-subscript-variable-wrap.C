// P3100: the element at a subscript of a null pointer that is a constant
// variable, `p[k]', is a constant subscript, checked on the pointer itself
// (DECISIONS.md E97 (c)), as `p[1]' is: a scalar read or stored to, a
// member read or stored to, a class copied from it and the object of
// `p[k].*pm' are reported once under .access; the address of the element
// or of its member, a reference bound to either and a member function
// called on it access nothing and are reported once under .indirection.
// The same with a const variable, `k[p]' and `p[k + 1]'.  A non-null
// pointer is not reported.  (GCC-843: the offset was not folded when the
// element's address was marked, and most forms were not reported.)  The
// forms are in p3100-null-constant-subscript-variable.h.
// With the whole group and the pointer-arithmetic check configured, at -O0.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-constant-subscript-variable.h"
// { dg-output "iread 1(\n|\r\n|\r)istore 1(\n|\r\n|\r)mread 1(\n|\r\n|\r)" }
// { dg-output "mstore 1(\n|\r\n|\r)maddr 1(\n|\r\n|\r)saddr 1(\n|\r\n|\r)" }
// { dg-output "mbind 1(\n|\r\n|\r)sbind 1(\n|\r\n|\r)scopy 1(\n|\r\n|\r)" }
// { dg-output "scall 1(\n|\r\n|\r)pm 1(\n|\r\n|\r)iread_kc 1(\n|\r\n|\r)" }
// { dg-output "mread_kc 1(\n|\r\n|\r)mread_rev 1(\n|\r\n|\r)mread_expr 1(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
