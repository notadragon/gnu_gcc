// P3100: with -fsanitize=null routed to the contract-violation handler as
// well as ub:expr.unary.dereference.nullptr configured, a downcast of `*p'
// to a reference, p a null pointer, with the base at a nonzero offset in
// the derived class, used -- read, bound, called through -- is reported
// once (DECISIONS.md E120; GCC-870: the reads three times, the binding
// and the calls twice).  Under -fcontracts-p3100 the conversion makes no
// null test where a leaf of the group is checked, and the null check of
// the use, P3100's or the sanitizer's, tests p (P3100_DOWNCAST_ADDRESS_P).
// With the group ignored the conversion keeps its null test and the checks
// are stock -fsanitize=null's: the binding twice (GCC-871, stock), each
// use once more by its own check (E120: dropping that needs the binding
// tracked for the sanitizer, as at offset zero), a virtual call's object
// evaluated twice.  The forms are in
// ../contracts/cpp26/p3100-null-downcast-ref-offset.h.
// With the group ignored, at -O0.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fsanitize=null -fsanitize-semantic=null:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "../contracts/cpp26/p3100-null-downcast-ref-offset.h"

// { dg-output "drd 3(\n|\r\n|\r)dbind 2(\n|\r\n|\r)dcall 2(\n|\r\n|\r)" }
// { dg-output "srd 3(\n|\r\n|\r)sbind 2(\n|\r\n|\r)scall 2(\n|\r\n|\r)" }
// { dg-output "drd0 2(\n|\r\n|\r)dbind0 1(\n|\r\n|\r)daddr 3(\n|\r\n|\r)" }
// { dg-output "dst 3(\n|\r\n|\r)drv 2(\n|\r\n|\r)dvcall 3(\n|\r\n|\r)" }
// { dg-output "dhcall 4(\n|\r\n|\r)dxcall 4(\n|\r\n|\r)dthis 1(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
