// P3100: with ub:expr.add.out.of.bounds.pointer.arithmetic configured too,
// an indirection through a null pointer plus a constant, `*(p + c)'
// (DECISIONS.md E99 (a)), is checked for null on the pointer as without
// it, once, under the leaf it is without it: .access where the element is
// read or written, .indirection where it is not (a class copied from it
// reads it through the reference the copy binds, .access).  The forms are in
// p3100-null-pointer-arith-constant.h; the expected counts are those of
// p3100-null-pointer-arith-constant-indirection-O2.C, but a negative offset,
// `*(p - c)' (DECISIONS.md E105 (a)), also wraps around the address space
// and is reported once more, by that check.
// With .indirection and the pointer-arithmetic check configured, at -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-pointer-arith-constant.h"
// { dg-output "iread 0(\n|\r\n|\r)istore 0(\n|\r\n|\r)iread_rev 0(\n|\r\n|\r)" }
// { dg-output "iread_neg 1(\n|\r\n|\r)iread_k 0(\n|\r\n|\r)mread 0(\n|\r\n|\r)" }
// { dg-output "mstore 0(\n|\r\n|\r)mread_dot 0(\n|\r\n|\r)maddr 1(\n|\r\n|\r)" }
// { dg-output "mbind 1(\n|\r\n|\r)sbind 1(\n|\r\n|\r)saddr 1(\n|\r\n|\r)" }
// { dg-output "scopy 0(\n|\r\n|\r)sfrom 0(\n|\r\n|\r)sstore 0(\n|\r\n|\r)" }
// { dg-output "sarg 0(\n|\r\n|\r)scall 1(\n|\r\n|\r)arrread 0(\n|\r\n|\r)" }
// { dg-output "pm 0(\n|\r\n|\r)ecopy 1(\n|\r\n|\r)estore 1(\n|\r\n|\r)" }
// { dg-output "ebind 1(\n|\r\n|\r)vcall 0(\n|\r\n|\r)vread 0(\n|\r\n|\r)" }
// { dg-output "disc 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)sassign 0(\n|\r\n|\r)" }
// { dg-output "sdtor 1(\n|\r\n|\r)sstatic 1(\n|\r\n|\r)pmf 1(\n|\r\n|\r)" }
// { dg-output "bcall 1(\n|\r\n|\r)nonnull_extra 0(\n|\r\n|\r)arrstore 0(\n|\r\n|\r)" }
// { dg-output "efrom 1(\n|\r\n|\r)earg 1(\n|\r\n|\r)nonnull_gcc 0(\n|\r\n|\r)" }
// { dg-output "istore_rev 0(\n|\r\n|\r)istore_neg 1(\n|\r\n|\r)istore_k 0(\n|\r\n|\r)" }
// { dg-output "mread_rev 0(\n|\r\n|\r)mread_neg 1(\n|\r\n|\r)mread_k 0(\n|\r\n|\r)" }
// { dg-output "mstore_rev 0(\n|\r\n|\r)mstore_neg 1(\n|\r\n|\r)mstore_k 0(\n|\r\n|\r)" }
// { dg-output "maddr_rev 1(\n|\r\n|\r)maddr_neg 2(\n|\r\n|\r)maddr_k 1(\n|\r\n|\r)" }
// { dg-output "iread_kc 0(\n|\r\n|\r)mread_kc 0(\n|\r\n|\r)mread_negk 1(\n|\r\n|\r)" }
// { dg-output "mread_revk 0(\n|\r\n|\r)sbind_rev 1(\n|\r\n|\r)scopy_neg 1(\n|\r\n|\r)" }
// { dg-output "scall_k 1(\n|\r\n|\r)pm_kc 0(\n|\r\n|\r)nonnull_e105 0(\n|\r\n|\r)" }
