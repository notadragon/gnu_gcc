// P3100: with -fsanitize=vptr routed to the contract-violation handler as
// well as ub:expr.unary.dereference.nullptr configured, a member call or
// access through a null pointer to an object of a polymorphic class is
// reported as without -fsanitize=vptr: once, under the leaf of the form
// (DECISIONS.md E94 (b), E102 (a), E110 (a)).  The vptr check of a member
// call or access, of a downcast to a reference or of a conversion to a
// virtual base skips a null object pointer, as Clang's does, and the
// member is accessed as without the check (GCC-856: the check's read of
// the vtable pointer was reported again, or read through the null
// pointer before the null check of a subobject's pointer).  The forms are
// in p3100-null-vptr-call.h.
// With only .indirection configured, at -O0.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-vptr-call.h"

// { dg-output "vh 0(\n|\r\n|\r)vnv 1(\n|\r\n|\r)vrd 0(\n|\r\n|\r)" }
// { dg-output "vwr 0(\n|\r\n|\r)hrd 0(\n|\r\n|\r)hnv 1(\n|\r\n|\r)" }
// { dg-output "qh 1(\n|\r\n|\r)dh 0(\n|\r\n|\r)drd 0(\n|\r\n|\r)" }
// { dg-output "fin 0(\n|\r\n|\r)memarr 0(\n|\r\n|\r)base 0(\n|\r\n|\r)" }
// { dg-output "mwrd 0(\n|\r\n|\r)mwq 1(\n|\r\n|\r)vbrd 0(\n|\r\n|\r)" }
// { dg-output "vbh 0(\n|\r\n|\r)vbnv 0(\n|\r\n|\r)star 1(\n|\r\n|\r)" }
// { dg-output "sub 1(\n|\r\n|\r)bind 1(\n|\r\n|\r)dref 1(\n|\r\n|\r)" }
// { dg-output "drdc 0(\n|\r\n|\r)vbref 0(\n|\r\n|\r)vbsub 0(\n|\r\n|\r)" }
// { dg-output "mvh 0(\n|\r\n|\r)mvrd 0(\n|\r\n|\r)mvq 1(\n|\r\n|\r)" }
// { dg-output "vbsubrd 0(\n|\r\n|\r)" }
