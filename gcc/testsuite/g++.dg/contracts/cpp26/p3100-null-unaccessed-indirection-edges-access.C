// P3100: the forms of p3100-null-unaccessed-indirection-edges.h under the
// .access leaf alone (GCC-804): an indirection whose result is not
// accessed is the .indirection leaf's, so only the reads are reported --
// the access through `&*p' of `(&*p)->m', and the conversions of `*p' to a
// virtual base, which read the base's offset through p (also discarded).
// Every other form reports nothing; the faults that follow are caught.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-unaccessed-indirection-edges.cpp
// (its ALL and IND runs; the .access leaf alone has no Clang run).
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-Wno-unused-value" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-unaccessed-indirection-edges.h"
// { dg-output "addr2 0(\n|\r\n|\r)disc_sub 0(\n|\r\n|\r)disc_cond 0(\n|\r\n|\r)" }
// { dg-output "disc_comma 0(\n|\r\n|\r)disc_comma_lhs 0(\n|\r\n|\r)addr_cond 0(\n|\r\n|\r)" }
// { dg-output "addr_comma 0(\n|\r\n|\r)addr_cast 0(\n|\r\n|\r)addr_ccast 0(\n|\r\n|\r)" }
// { dg-output "addr_rcast 0(\n|\r\n|\r)addr_base 0(\n|\r\n|\r)addr_base_off 0(\n|\r\n|\r)" }
// { dg-output "disc_cast 0(\n|\r\n|\r)disc_base_off 0(\n|\r\n|\r)std_addressof 0(\n|\r\n|\r)" }
// { dg-output "builtin_addressof 0(\n|\r\n|\r)se 0(\n|\r\n|\r)sck 0(\n|\r\n|\r)" }
// { dg-output "pdtor 0(\n|\r\n|\r)amem 1(\n|\r\n|\r)addr_vbase 1(\n|\r\n|\r)" }
// { dg-output "disc_base 0(\n|\r\n|\r)disc_ccast 0(\n|\r\n|\r)disc_rcast 0(\n|\r\n|\r)" }
// { dg-output "disc_cbase 0(\n|\r\n|\r)disc_void_cast 0(\n|\r\n|\r)disc_vbase 1(\n|\r\n|\r)" }
// { dg-output "se_star 0(\n|\r\n|\r)sck_cast 0(\n|\r\n|\r)sf_cast 0(\n|\r\n|\r)" }
// { dg-output "se_cond 0(\n|\r\n|\r)pdtor_star 0(\n|\r\n|\r)pdtor_cast 0(\n|\r\n|\r)" }
// { dg-output "sdtor 0(\n|\r\n|\r)sdtor_star 0(\n|\r\n|\r)builtin_addressof_cast 0(\n|\r\n|\r)" }
// { dg-output "std_addressof_cond 0(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)controls 0(\n|\r\n|\r)" }
