// P3100: with ub:expr.add.out.of.bounds.pointer.arithmetic configured too,
// the element at a constant subscript of a null pointer, `sp[1]'
// (DECISIONS.md E97 (c)), is checked for null on the pointer as without
// it, once, under the leaf it is without it: .access where it is read or
// written, .indirection where it is not (a class copied from it reads it
// through the reference the copy binds, .access).  A negative offset from
// a null pointer also wraps around the address space, reported by that
// check, so `sp[-1]' reports once more than without it.  The forms are in
// p3100-null-constant-subscript.h; the expected counts are those of
// p3100-null-constant-subscript-indirection-O2.C but the negative offsets.
// With .indirection and the pointer-arithmetic check configured, at -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-constant-subscript.h"
// { dg-output "mread 0(\n|\r\n|\r)mstore 0(\n|\r\n|\r)maddr 1(\n|\r\n|\r)" }
// { dg-output "mbind 1(\n|\r\n|\r)sbind 1(\n|\r\n|\r)saddr 1(\n|\r\n|\r)" }
// { dg-output "scopy 0(\n|\r\n|\r)sfrom 0(\n|\r\n|\r)sstore 0(\n|\r\n|\r)" }
// { dg-output "sarg 0(\n|\r\n|\r)scall 1(\n|\r\n|\r)arrread 0(\n|\r\n|\r)" }
// { dg-output "arrstore 0(\n|\r\n|\r)iread 0(\n|\r\n|\r)istore 0(\n|\r\n|\r)" }
// { dg-output "pm 0(\n|\r\n|\r)efrom 1(\n|\r\n|\r)ecopy 1(\n|\r\n|\r)" }
// { dg-output "earg 1(\n|\r\n|\r)estore 1(\n|\r\n|\r)ebind 1(\n|\r\n|\r)" }
// { dg-output "vcall 0(\n|\r\n|\r)vread 0(\n|\r\n|\r)mread_neg 1(\n|\r\n|\r)" }
// { dg-output "maddr_neg 2(\n|\r\n|\r)sbind_neg 2(\n|\r\n|\r)ecopy_neg 2(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
// { dg-output "pmf 1(\n|\r\n|\r)sarg_ref 1(\n|\r\n|\r)sret_ref 1(\n|\r\n|\r)" }
// { dg-output "scbind 1(\n|\r\n|\r)slambda 1(\n|\r\n|\r)tdtor 1(\n|\r\n|\r)" }
// { dg-output "bbind 1(\n|\r\n|\r)bcall 1(\n|\r\n|\r)nonnull_extra 0(\n|\r\n|\r)" }
