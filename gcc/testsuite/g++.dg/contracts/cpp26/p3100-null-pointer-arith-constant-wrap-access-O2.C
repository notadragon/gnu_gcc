// P3100: with ub:expr.add.out.of.bounds.pointer.arithmetic configured too,
// an indirection through a null pointer plus a constant, `*(p + c)'
// (DECISIONS.md E99 (a)), is checked for null on the pointer as without
// it, once, under the leaf it is without it: .access where the element is
// read or written, .indirection where it is not (a class copied from it
// reads it through the reference the copy binds, .access).  The forms are in
// p3100-null-pointer-arith-constant.h; the expected counts are those of
// p3100-null-pointer-arith-constant-access-O2.C, but a negative offset,
// `*(p - c)' (DECISIONS.md E105 (a)), also wraps around the address space
// and is reported once more, by that check.
// With .access and the pointer-arithmetic check configured, at -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-pointer-arith-constant.h"
// { dg-output "iread 1(\n|\r\n|\r)istore 1(\n|\r\n|\r)iread_rev 1(\n|\r\n|\r)" }
// { dg-output "iread_neg 2(\n|\r\n|\r)iread_k 1(\n|\r\n|\r)mread 1(\n|\r\n|\r)" }
// { dg-output "mstore 1(\n|\r\n|\r)mread_dot 1(\n|\r\n|\r)maddr 0(\n|\r\n|\r)" }
// { dg-output "mbind 0(\n|\r\n|\r)sbind 0(\n|\r\n|\r)saddr 0(\n|\r\n|\r)" }
// { dg-output "scopy 1(\n|\r\n|\r)sfrom 1(\n|\r\n|\r)sstore 1(\n|\r\n|\r)" }
// { dg-output "sarg 1(\n|\r\n|\r)scall 0(\n|\r\n|\r)arrread 1(\n|\r\n|\r)" }
// { dg-output "pm 1(\n|\r\n|\r)ecopy 0(\n|\r\n|\r)estore 0(\n|\r\n|\r)" }
// { dg-output "ebind 0(\n|\r\n|\r)vcall 1(\n|\r\n|\r)vread 1(\n|\r\n|\r)" }
// { dg-output "disc 0(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)sassign 1(\n|\r\n|\r)" }
// { dg-output "sdtor 0(\n|\r\n|\r)sstatic 0(\n|\r\n|\r)pmf 0(\n|\r\n|\r)" }
// { dg-output "bcall 0(\n|\r\n|\r)nonnull_extra 0(\n|\r\n|\r)arrstore 1(\n|\r\n|\r)" }
// { dg-output "efrom 0(\n|\r\n|\r)earg 0(\n|\r\n|\r)nonnull_gcc 0(\n|\r\n|\r)" }
// { dg-output "istore_rev 1(\n|\r\n|\r)istore_neg 2(\n|\r\n|\r)istore_k 1(\n|\r\n|\r)" }
// { dg-output "mread_rev 1(\n|\r\n|\r)mread_neg 2(\n|\r\n|\r)mread_k 1(\n|\r\n|\r)" }
// { dg-output "mstore_rev 1(\n|\r\n|\r)mstore_neg 2(\n|\r\n|\r)mstore_k 1(\n|\r\n|\r)" }
// { dg-output "maddr_rev 0(\n|\r\n|\r)maddr_neg 1(\n|\r\n|\r)maddr_k 0(\n|\r\n|\r)" }
// { dg-output "iread_kc 1(\n|\r\n|\r)mread_kc 1(\n|\r\n|\r)mread_negk 2(\n|\r\n|\r)" }
// { dg-output "mread_revk 1(\n|\r\n|\r)sbind_rev 0(\n|\r\n|\r)scopy_neg 2(\n|\r\n|\r)" }
// { dg-output "scall_k 0(\n|\r\n|\r)pm_kc 1(\n|\r\n|\r)nonnull_e105 0(\n|\r\n|\r)" }
