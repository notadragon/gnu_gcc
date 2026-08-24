// P3595: output.dynamic parses cleanly (no diagnostics).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-parse-good.json" }
void f(int x) pre(x > 0) { }
