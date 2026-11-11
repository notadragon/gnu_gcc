// A "group" in a P3595 configuration entry holding a NUL character is an
// error, not cut at the NUL (notadragon_wg21 DECISIONS.md E192 (1)(a)).
// p3595-config-group-nul-inline.C is the inline form.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-group-nul.json" }
// { dg-error ".group. must not contain a null character" "" { target *-*-* } 0 }
// { dg-prune-output "In JSON value" }

int f (int x) pre (x > 0) { return x; }
