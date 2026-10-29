// P3100: an indirection through pointer arithmetic with a null pointer and
// a constant, `*(p + c)', is checked on the pointer itself, as `p[c]' is
// (DECISIONS.md E99 (a)): a scalar read or stored to, a member read or
// stored to (`(p + c)->a', `(*(p + c)).a', an element of an array member),
// a class copied from or stored to, the object of `(p + c)->*pm', a virtual
// call and a member read through a vtable pointer are reported once under
// .access; the address of the element or of its member, a reference bound
// to either, a non-virtual member function called on it, an empty class
// copied from or stored to it and a discarded `*(p + c)' access nothing and
// are reported once under .indirection.  A non-null pointer is not
// reported.  Then a trivial assignment called through it,
// `(p + c)->operator= (s)' (.access, a store), and, each .indirection, a
// trivial destructor, a static member function named through it, a call
// through a pointer to member function and a base's member function.  The
// virtual call is .access, as for `vp[1].h ()': it reads the vtable
// pointer through the pointer (DECISIONS.md E102 (a)).  The same for the other spellings of a
// pointer plus or minus an integral constant expression (DECISIONS.md
// E105 (a)): `*(1 + ip)', `*(ip - 1)' and `*(ip + k)', `k' a constexpr
// variable, read, then (GCC's) the same stored to, a member read, stored
// to and its address through `1 + sp', `sp - 1' and `sp + k', a const
// variable as `c' (a scalar and a member read), `(sp - k)->a',
// `(kc + sp)->a', a reference bound to `*(1 + sp)', a class copied from
// `*(sp - 1)', a member function called through `sp + k' and the object of
// `(sp + kc)->*pm'.  The forms are in
// p3100-null-pointer-arith-constant.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-pointer-arith-constant.cpp.
// With .indirection configured, at -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-pointer-arith-constant.h"
// { dg-output "iread 0(\n|\r\n|\r)istore 0(\n|\r\n|\r)iread_rev 0(\n|\r\n|\r)" }
// { dg-output "iread_neg 0(\n|\r\n|\r)iread_k 0(\n|\r\n|\r)mread 0(\n|\r\n|\r)" }
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
// { dg-output "istore_rev 0(\n|\r\n|\r)istore_neg 0(\n|\r\n|\r)istore_k 0(\n|\r\n|\r)" }
// { dg-output "mread_rev 0(\n|\r\n|\r)mread_neg 0(\n|\r\n|\r)mread_k 0(\n|\r\n|\r)" }
// { dg-output "mstore_rev 0(\n|\r\n|\r)mstore_neg 0(\n|\r\n|\r)mstore_k 0(\n|\r\n|\r)" }
// { dg-output "maddr_rev 1(\n|\r\n|\r)maddr_neg 1(\n|\r\n|\r)maddr_k 1(\n|\r\n|\r)" }
// { dg-output "iread_kc 0(\n|\r\n|\r)mread_kc 0(\n|\r\n|\r)mread_negk 0(\n|\r\n|\r)" }
// { dg-output "mread_revk 0(\n|\r\n|\r)sbind_rev 1(\n|\r\n|\r)scopy_neg 0(\n|\r\n|\r)" }
// { dg-output "scall_k 1(\n|\r\n|\r)pm_kc 0(\n|\r\n|\r)nonnull_e105 0(\n|\r\n|\r)" }
