// P3100: more indirections through a null pointer whose result is not
// accessed (GCC-804), each reported once under the whole
// ub:expr.unary.dereference.nullptr group at -O0: an enumerator, a static
// member or a pseudo-destructor named through `*p' (also cast to a
// reference, or a conditional), a trivial destructor called through `p',
// the address of a conditional or comma expression yielding `*p' or of
// `*p' cast to a reference, the same discarded, the conversion of `*p' to a base class (a virtual base's reads
// its offset through p: the .access leaf), std::addressof and
// __builtin_addressof of such a glvalue.  `(&*p)->m' reports twice, the
// indirection of `&*p' and the access through it.  `&p[2]' is checked on p
// (DECISIONS.md E97 (c)).  `this', `*&x', an array, a
// reference, `typeid' of a polymorphic `*p', `delete p' and an unevaluated
// operand are not checked; neither is a non-null pointer.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-unaccessed-indirection-edges.cpp.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-Wno-unused-value" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-unaccessed-indirection-edges.h"
// { dg-output "addr2 1(\n|\r\n|\r)disc_sub 1(\n|\r\n|\r)disc_cond 1(\n|\r\n|\r)" }
// { dg-output "disc_comma 1(\n|\r\n|\r)disc_comma_lhs 1(\n|\r\n|\r)addr_cond 1(\n|\r\n|\r)" }
// { dg-output "addr_comma 1(\n|\r\n|\r)addr_cast 1(\n|\r\n|\r)addr_ccast 1(\n|\r\n|\r)" }
// { dg-output "addr_rcast 1(\n|\r\n|\r)addr_base 1(\n|\r\n|\r)addr_base_off 1(\n|\r\n|\r)" }
// { dg-output "disc_cast 1(\n|\r\n|\r)disc_base_off 1(\n|\r\n|\r)std_addressof 1(\n|\r\n|\r)" }
// { dg-output "builtin_addressof 1(\n|\r\n|\r)se 1(\n|\r\n|\r)sck 1(\n|\r\n|\r)" }
// { dg-output "pdtor 1(\n|\r\n|\r)amem 2(\n|\r\n|\r)addr_vbase 1(\n|\r\n|\r)" }
// { dg-output "disc_base 1(\n|\r\n|\r)disc_ccast 1(\n|\r\n|\r)disc_rcast 1(\n|\r\n|\r)" }
// { dg-output "disc_cbase 1(\n|\r\n|\r)disc_void_cast 1(\n|\r\n|\r)disc_vbase 1(\n|\r\n|\r)" }
// { dg-output "se_star 1(\n|\r\n|\r)sck_cast 1(\n|\r\n|\r)sf_cast 1(\n|\r\n|\r)" }
// { dg-output "se_cond 1(\n|\r\n|\r)pdtor_star 1(\n|\r\n|\r)pdtor_cast 1(\n|\r\n|\r)" }
// { dg-output "sdtor 1(\n|\r\n|\r)sdtor_star 1(\n|\r\n|\r)builtin_addressof_cast 1(\n|\r\n|\r)" }
// { dg-output "std_addressof_cond 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)controls 0(\n|\r\n|\r)" }
