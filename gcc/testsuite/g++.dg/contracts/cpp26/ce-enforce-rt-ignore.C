// CE semantic: enforce.  Runtime semantic: ignore.
// Violated precondition in manifestly-CE context must be an error.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/ce-enforce-rt-ignore.json" }

constexpr int f(int x) pre(x >= 0) { return x; } // { dg-error "contract predicate is false in constant expression" }
constexpr int y = f(-1);
