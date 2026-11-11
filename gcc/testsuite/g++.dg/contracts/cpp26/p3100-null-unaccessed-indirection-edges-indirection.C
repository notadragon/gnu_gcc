// P3100: the forms of p3100-null-unaccessed-indirection-edges.h under the
// .indirection leaf alone, at -O2 (GCC-804): each is reported once, as
// under the whole group (p3100-null-unaccessed-indirection-edges.C), except
// the conversions of `*p' to a virtual base, which read through p (the
// .access leaf), and the access of `(&*p)->m'.  At -O2 std::addressof is
// not folded away while the leaf applies.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-unaccessed-indirection-edges.cpp.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-O2 -Wno-unused-value" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-unaccessed-indirection-edges.h"
// { dg-output "addr2 1(\n|\r\n|\r)disc_sub 1(\n|\r\n|\r)disc_cond 1(\n|\r\n|\r)" }
// { dg-output "disc_comma 1(\n|\r\n|\r)disc_comma_lhs 1(\n|\r\n|\r)addr_cond 1(\n|\r\n|\r)" }
// { dg-output "addr_comma 1(\n|\r\n|\r)addr_cast 1(\n|\r\n|\r)addr_ccast 1(\n|\r\n|\r)" }
// { dg-output "addr_rcast 1(\n|\r\n|\r)addr_base 1(\n|\r\n|\r)addr_base_off 1(\n|\r\n|\r)" }
// { dg-output "disc_cast 1(\n|\r\n|\r)disc_base_off 1(\n|\r\n|\r)std_addressof 1(\n|\r\n|\r)" }
// { dg-output "builtin_addressof 1(\n|\r\n|\r)se 1(\n|\r\n|\r)sck 1(\n|\r\n|\r)" }
// { dg-output "pdtor 1(\n|\r\n|\r)amem 1(\n|\r\n|\r)addr_vbase 0(\n|\r\n|\r)" }
// { dg-output "disc_base 1(\n|\r\n|\r)disc_ccast 1(\n|\r\n|\r)disc_rcast 1(\n|\r\n|\r)" }
// { dg-output "disc_cbase 1(\n|\r\n|\r)disc_void_cast 1(\n|\r\n|\r)disc_vbase 0(\n|\r\n|\r)" }
// { dg-output "se_star 1(\n|\r\n|\r)sck_cast 1(\n|\r\n|\r)sf_cast 1(\n|\r\n|\r)" }
// { dg-output "se_cond 1(\n|\r\n|\r)pdtor_star 1(\n|\r\n|\r)pdtor_cast 1(\n|\r\n|\r)" }
// { dg-output "sdtor 1(\n|\r\n|\r)sdtor_star 1(\n|\r\n|\r)builtin_addressof_cast 1(\n|\r\n|\r)" }
// { dg-output "std_addressof_cond 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)controls 0(\n|\r\n|\r)" }
