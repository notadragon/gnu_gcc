// P3100: p3100-null-unaccessed-indirection.C with only the .access leaf of
// ub:expr.unary.dereference.nullptr configured: an indirection whose result
// is not accessed is the .indirection leaf's (GCC-799), so nothing reports.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-additional-options "-Wno-unused-value" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-unaccessed-indirection.C"

// { dg-output "bind 0(\n|\r\n|\r)" }
// { dg-output "addr 0(\n|\r\n|\r)" }
// { dg-output "addr0 0(\n|\r\n|\r)" }
// { dg-output "disc 0(\n|\r\n|\r)" }
// { dg-output "disc_void 0(\n|\r\n|\r)" }
// { dg-output "sfc 0(\n|\r\n|\r)" }
// { dg-output "sfc_star 0(\n|\r\n|\r)" }
// { dg-output "sk 0(\n|\r\n|\r)" }
// { dg-output "sk_addr 0(\n|\r\n|\r)" }
// { dg-output "sfc_constexpr 0(\n|\r\n|\r)" }
// { dg-output "taddr 0(\n|\r\n|\r)" }
// { dg-output "faddr 0(\n|\r\n|\r)" }
// { dg-output "fdisc 0(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
