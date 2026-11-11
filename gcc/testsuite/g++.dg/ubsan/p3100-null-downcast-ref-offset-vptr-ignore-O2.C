// P3100: with -fsanitize=vptr routed to the contract-violation handler and
// ub:expr.unary.dereference.nullptr ignored, a downcast of `*p' to a
// reference, p a null pointer, used -- read, bound, called through -- is
// not reported: the vptr checks skip the null pointer only where a leaf
// of the group is checked (DECISIONS.md E116 (a)) and are otherwise stock
// (E119 (a)); a fault is caught and resumed.  A mirror of the ignored-group
// run of Clang's Runnable/p3100-null-downcast-ref-offset-vptr.cpp
// (GCC-873).
// The forms are in ../contracts/cpp26/p3100-null-downcast-ref-offset.h.
// With the group ignored, at -O2.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "../contracts/cpp26/p3100-null-downcast-ref-offset.h"

// { dg-output "drd 0(\n|\r\n|\r)dbind 0(\n|\r\n|\r)dcall 0(\n|\r\n|\r)" }
// { dg-output "srd 0(\n|\r\n|\r)sbind 0(\n|\r\n|\r)scall 0(\n|\r\n|\r)" }
// { dg-output "drd0 0(\n|\r\n|\r)dbind0 0(\n|\r\n|\r)daddr 0(\n|\r\n|\r)" }
// { dg-output "dst 0(\n|\r\n|\r)drv 0(\n|\r\n|\r)dvcall 0(\n|\r\n|\r)" }
// { dg-output "dhcall 0(\n|\r\n|\r)dxcall 0(\n|\r\n|\r)dthis 0(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
