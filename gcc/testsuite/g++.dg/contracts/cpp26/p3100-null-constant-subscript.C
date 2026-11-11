// P3100: the element at a constant subscript of a null pointer, `sp[1]'
// (also negative, `sp[-1]'), is checked on the pointer itself, as `sp[0]'
// is (DECISIONS.md E97 (c)): a member read or stored to (`sp[1].a', an
// element of an array member), a scalar element read or stored to, a class
// copied from or stored to, the object of `sp[1].*pm' and a virtual call or
// member read through a vtable pointer are reported once under .access;
// the address of the element or of its member, a reference bound to
// either, a non-virtual member function called on it and an empty class
// copied from or stored to it access nothing and are reported once under
// .indirection (DECISIONS.md E86 (a)).  A non-null pointer is not reported.
// A non-constant subscript is checked on the element's address (GCC-794).
// Then Clang's extra forms, each .indirection: a member function called
// through a pointer to member function, a reference bound to the element
// as an argument, returned, const, captured by a lambda, a non-trivial
// destructor called, a reference bound to a base of the element and a
// member function of a base called on it.
// The forms are in p3100-null-constant-subscript.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-constant-subscript.cpp.
// With the whole group configured, at -O0.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-constant-subscript.h"
// { dg-output "mread 1(\n|\r\n|\r)mstore 1(\n|\r\n|\r)maddr 1(\n|\r\n|\r)" }
// { dg-output "mbind 1(\n|\r\n|\r)sbind 1(\n|\r\n|\r)saddr 1(\n|\r\n|\r)" }
// { dg-output "scopy 1(\n|\r\n|\r)sfrom 1(\n|\r\n|\r)sstore 1(\n|\r\n|\r)" }
// { dg-output "sarg 1(\n|\r\n|\r)scall 1(\n|\r\n|\r)arrread 1(\n|\r\n|\r)" }
// { dg-output "arrstore 1(\n|\r\n|\r)iread 1(\n|\r\n|\r)istore 1(\n|\r\n|\r)" }
// { dg-output "pm 1(\n|\r\n|\r)efrom 1(\n|\r\n|\r)ecopy 1(\n|\r\n|\r)" }
// { dg-output "earg 1(\n|\r\n|\r)estore 1(\n|\r\n|\r)ebind 1(\n|\r\n|\r)" }
// { dg-output "vcall 1(\n|\r\n|\r)vread 1(\n|\r\n|\r)mread_neg 1(\n|\r\n|\r)" }
// { dg-output "maddr_neg 1(\n|\r\n|\r)sbind_neg 1(\n|\r\n|\r)ecopy_neg 1(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
// { dg-output "pmf 1(\n|\r\n|\r)sarg_ref 1(\n|\r\n|\r)sret_ref 1(\n|\r\n|\r)" }
// { dg-output "scbind 1(\n|\r\n|\r)slambda 1(\n|\r\n|\r)tdtor 1(\n|\r\n|\r)" }
// { dg-output "bbind 1(\n|\r\n|\r)bcall 1(\n|\r\n|\r)nonnull_extra 0(\n|\r\n|\r)" }
