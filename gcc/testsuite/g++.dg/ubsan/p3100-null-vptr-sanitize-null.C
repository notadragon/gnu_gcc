// P3100: with -fsanitize=null and -fsanitize=vptr routed to the
// contract-violation handler as well as ub:expr.unary.dereference.nullptr
// configured, a member call through a null pointer is reported as with
// -fsanitize=null alone, and the vptr check, which skips a null object
// pointer where a null leaf applies (GCC-856), does not fault: it tests
// the pointer the object is reached through, not the address the
// -fsanitize=null check of the call saved (GCC-865: `&mp->W2', `&hp->m'
// and `vp + 1' were tested, not null, and the vtable pointer read through
// them faulted).  With only .access configured, `vp[1].nv ()' is not
// reported: the sanitizer, which makes the null check of the .indirection
// leaf there, tests `vp + 1' (GCC-860, pending DECISIONS.md E115).  The
// forms are in p3100-null-vptr-sanitize-null.h.
// With the whole group configured, at -O0.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fsanitize=null,vptr -fsanitize-semantic=null:noexcept_observe,vptr:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-vptr-sanitize-null.h"

// { dg-output "mvq 1(\n|\r\n|\r)mvnw 1(\n|\r\n|\r)hnv 1(\n|\r\n|\r)" }
// { dg-output "sub 1(\n|\r\n|\r)vnv 1(\n|\r\n|\r)" }
