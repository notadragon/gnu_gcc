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
// With only .access configured, at -O2.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-vptr-call.h"

// { dg-output "vh 1(\n|\r\n|\r)vnv 0(\n|\r\n|\r)vrd 1(\n|\r\n|\r)" }
// { dg-output "vwr 1(\n|\r\n|\r)hrd 1(\n|\r\n|\r)hnv 0(\n|\r\n|\r)" }
// { dg-output "qh 0(\n|\r\n|\r)dh 1(\n|\r\n|\r)drd 1(\n|\r\n|\r)" }
// { dg-output "fin 1(\n|\r\n|\r)memarr 1(\n|\r\n|\r)base 1(\n|\r\n|\r)" }
// { dg-output "mwrd 1(\n|\r\n|\r)mwq 0(\n|\r\n|\r)vbrd 1(\n|\r\n|\r)" }
// { dg-output "vbh 1(\n|\r\n|\r)vbnv 1(\n|\r\n|\r)star 0(\n|\r\n|\r)" }
// { dg-output "sub 0(\n|\r\n|\r)bind 0(\n|\r\n|\r)dref 0(\n|\r\n|\r)" }
// { dg-output "drdc 1(\n|\r\n|\r)vbref 1(\n|\r\n|\r)vbsub 1(\n|\r\n|\r)" }
// { dg-output "mvh 1(\n|\r\n|\r)mvrd 1(\n|\r\n|\r)mvq 0(\n|\r\n|\r)" }
// { dg-output "vbsubrd 1(\n|\r\n|\r)" }
