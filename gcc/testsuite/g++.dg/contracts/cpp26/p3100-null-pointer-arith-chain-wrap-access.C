// P3100: an indirection through a chain of pointer arithmetic with
// constants on a null pointer, `*(p + 1 + 2)', `(p + 1)[1]', `p + k + 1',
// is checked on the innermost pointer (DECISIONS.md E103 (a)), as
// `*(p + c)' is (E99 (a)): a scalar read or stored to and a member read or
// stored to are reported once under .access, a member's address once under
// .indirection; so are GCC's further forms, a reference bound, a class
// copied, a member function called, a pointer to member, a discarded
// indirection, a negative chain `*(p - 1 - 2)', a member of an element of
// a sum and its address.  A non-null pointer is not reported.  With an
// offset that is not a constant, `*(p + i)', `*(p + i + 1)', `(p + 1)[i]'
// and the rest of Clang's forms, the null check is made on the sum, not p,
// and nothing is reported: the GCC-794 divergence, extended by E103 (a)
// (Clang checks p).  With the pointer-arithmetic check configured too, each
// is reported as without it, and a negative offset also wraps around the
// address space and is reported once more by that check, for each sum
// that wraps.  (GCC-844: only the scalar reads were reported, folded into
// an access at a constant offset from p; with the pointer-arithmetic check
// configured too, the `+' chains were not.)  The forms are in
// p3100-null-pointer-arith-chain.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-pointer-arith-chain.cpp.
// With .access and the pointer-arithmetic check configured, at -O0.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-pointer-arith-chain.h"
// { dg-output "iread 1(\n|\r\n|\r)isub 1(\n|\r\n|\r)iread_k 1(\n|\r\n|\r)" }
// { dg-output "istore 1(\n|\r\n|\r)isubstore 1(\n|\r\n|\r)mread 1(\n|\r\n|\r)" }
// { dg-output "mstore 1(\n|\r\n|\r)msub 1(\n|\r\n|\r)maddr 0(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)inc 0(\n|\r\n|\r)incst 0(\n|\r\n|\r)" }
// { dg-output "mnc 0(\n|\r\n|\r)mncst 0(\n|\r\n|\r)maddrnc 0(\n|\r\n|\r)" }
// { dg-output "bindnc 0(\n|\r\n|\r)copync 0(\n|\r\n|\r)callnc 0(\n|\r\n|\r)" }
// { dg-output "pmnc 0(\n|\r\n|\r)revnc 0(\n|\r\n|\r)negnc 2(\n|\r\n|\r)" }
// { dg-output "subnc 0(\n|\r\n|\r)chainnc 0(\n|\r\n|\r)chainnc2 0(\n|\r\n|\r)" }
// { dg-output "chainneg 0(\n|\r\n|\r)chainbind 0(\n|\r\n|\r)chaincall 0(\n|\r\n|\r)" }
// { dg-output "nonnull_clang 0(\n|\r\n|\r)cbind 0(\n|\r\n|\r)ccopy 1(\n|\r\n|\r)" }
// { dg-output "ccall 0(\n|\r\n|\r)cpm 1(\n|\r\n|\r)cdisc 0(\n|\r\n|\r)" }
// { dg-output "cneg 3(\n|\r\n|\r)csubm 1(\n|\r\n|\r)caddrsub 0(\n|\r\n|\r)" }
// { dg-output "nonnull_gcc 0(\n|\r\n|\r)" }
