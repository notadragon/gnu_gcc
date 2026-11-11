// P3100: with -fsanitize=vptr routed to the contract-violation handler as
// well as ub:expr.unary.dereference.nullptr configured (and -fsanitize=null
// too), a downcast of `*p' to a reference, p a null pointer, with the base
// at a nonzero offset in the derived class, used -- read, bound, called
// through -- is reported once, its vptr check skipped on the null pointer
// (DECISIONS.md E116 (a)).  A member call saves the downcast its vptr
// check wraps as its object argument; the null checks of the call's
// object pointer and of a virtual call's vtable pointer read find p in it
// (ubsan_p3100_vptr_checked_downcast; GCC-872: a non-virtual call and a
// virtual one of the derived class or of its primary base were not
// reported).  The forms are in
// ../contracts/cpp26/p3100-null-downcast-ref-offset.h.
// With -fsanitize=null,vptr, at -O2.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fsanitize=null,vptr -fsanitize-semantic=null:noexcept_observe -fsanitize-semantic=vptr:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "../contracts/cpp26/p3100-null-downcast-ref-offset.h"

// { dg-output "drd 1(\n|\r\n|\r)dbind 1(\n|\r\n|\r)dcall 1(\n|\r\n|\r)" }
// { dg-output "srd 1(\n|\r\n|\r)sbind 1(\n|\r\n|\r)scall 1(\n|\r\n|\r)" }
// { dg-output "drd0 1(\n|\r\n|\r)dbind0 1(\n|\r\n|\r)daddr 1(\n|\r\n|\r)" }
// { dg-output "dst 1(\n|\r\n|\r)drv 1(\n|\r\n|\r)dvcall 1(\n|\r\n|\r)" }
// { dg-output "dhcall 1(\n|\r\n|\r)dxcall 1(\n|\r\n|\r)dthis 1(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
