// P3100: a downcast of `*p' to a reference, p a null pointer, with the
// base at a nonzero offset in the derived class, is checked for null on p
// itself by the use of the derived object -- a member read or store
// (.access), a reference bound to it, the address of a member, a
// non-virtual member call (.indirection), a virtual call (.access,
// DECISIONS.md E110 (a)) -- as with the base at offset zero: the derived
// object's address, p minus the offset, is not null (GCC-869: the reads
// were reported, the binding, the address and every call not at all).
// Each once, under its leaf; a non-null pointer is not reported.  The
// forms are in p3100-null-downcast-ref-offset.h; Clang's
// Runnable/p3100-null-downcast-ref-offset.cpp has its first eight
// (CLANG-802).
// With only .access configured, at -O0.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-downcast-ref-offset.h"

// { dg-output "drd 1(\n|\r\n|\r)dbind 0(\n|\r\n|\r)dcall 0(\n|\r\n|\r)" }
// { dg-output "srd 1(\n|\r\n|\r)sbind 0(\n|\r\n|\r)scall 0(\n|\r\n|\r)" }
// { dg-output "drd0 1(\n|\r\n|\r)dbind0 0(\n|\r\n|\r)daddr 0(\n|\r\n|\r)" }
// { dg-output "dst 1(\n|\r\n|\r)drv 0(\n|\r\n|\r)dvcall 1(\n|\r\n|\r)" }
// { dg-output "dhcall 1(\n|\r\n|\r)dxcall 1(\n|\r\n|\r)dthis 0(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
