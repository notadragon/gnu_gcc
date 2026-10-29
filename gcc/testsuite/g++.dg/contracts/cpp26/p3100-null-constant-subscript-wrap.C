// P3100: with ub:expr.add.out.of.bounds.pointer.arithmetic configured too,
// the element at a constant subscript of a null pointer, `sp[1]'
// (DECISIONS.md E97 (c)), is checked for null on the pointer as without
// it, once.  A negative offset from a null pointer also wraps around the
// address space, reported by that check, so `sp[-1]' reports twice.  The
// forms are in p3100-null-constant-subscript.h; the expected counts are
// those of p3100-null-constant-subscript.C.
// With the null group and the pointer-arithmetic check configured, at -O0.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-constant-subscript.h"
// { dg-output "mread 1(\n|\r\n|\r)mstore 1(\n|\r\n|\r)maddr 1(\n|\r\n|\r)" }
// { dg-output "mbind 1(\n|\r\n|\r)sbind 1(\n|\r\n|\r)saddr 1(\n|\r\n|\r)" }
// { dg-output "scopy 1(\n|\r\n|\r)sfrom 1(\n|\r\n|\r)sstore 1(\n|\r\n|\r)" }
// { dg-output "sarg 1(\n|\r\n|\r)scall 1(\n|\r\n|\r)arrread 1(\n|\r\n|\r)" }
// { dg-output "arrstore 1(\n|\r\n|\r)iread 1(\n|\r\n|\r)istore 1(\n|\r\n|\r)" }
// { dg-output "pm 1(\n|\r\n|\r)efrom 1(\n|\r\n|\r)ecopy 1(\n|\r\n|\r)" }
// { dg-output "earg 1(\n|\r\n|\r)estore 1(\n|\r\n|\r)ebind 1(\n|\r\n|\r)" }
// { dg-output "vcall 1(\n|\r\n|\r)vread 1(\n|\r\n|\r)mread_neg 2(\n|\r\n|\r)" }
// { dg-output "maddr_neg 2(\n|\r\n|\r)sbind_neg 2(\n|\r\n|\r)ecopy_neg 2(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
// { dg-output "pmf 1(\n|\r\n|\r)sarg_ref 1(\n|\r\n|\r)sret_ref 1(\n|\r\n|\r)" }
// { dg-output "scbind 1(\n|\r\n|\r)slambda 1(\n|\r\n|\r)tdtor 1(\n|\r\n|\r)" }
// { dg-output "bbind 1(\n|\r\n|\r)bcall 1(\n|\r\n|\r)nonnull_extra 0(\n|\r\n|\r)" }
