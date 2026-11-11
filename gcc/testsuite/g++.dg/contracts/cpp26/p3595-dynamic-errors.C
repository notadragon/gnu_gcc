// P3595: output.dynamic validation errors.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-errors.json" }
//
// Entry 0: "dynamic" with no "name" is rejected.
// Entry 1: "dynamic" with no "semantic" but explicit "provideweak": true is
//   rejected (nothing for a weak definition to return).
// Entry 2: an "output" with neither "semantic" nor "dynamic" is rejected.
// Each config-file diagnostic is preceded by a JSON-pointer path line.
// { dg-error "requires a string .name." "" { target *-*-* } 0 }
// { dg-regexp {[^\n]*p3595-dynamic-errors\.json: In JSON object '/0/output/dynamic'} }
// { dg-error ".provideweak. requires an output .semantic." "" { target *-*-* } 0 }
// { dg-regexp {[^\n]*p3595-dynamic-errors\.json: In JSON object '/1/output/dynamic'} }
// { dg-error ".output. requires a .semantic. or a .dynamic. field" "" { target *-*-* } 0 }
// { dg-regexp {[^\n]*p3595-dynamic-errors\.json: In JSON object '/2/output'} }
void f(int x) pre(x > 0) { }
