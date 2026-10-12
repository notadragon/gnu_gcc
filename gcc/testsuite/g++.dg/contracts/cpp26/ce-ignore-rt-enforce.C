// CE semantic: ignore.  Runtime semantic: enforce.
// Violated precondition at CE time is silently ignored.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/ce-ignore-rt-enforce.json" }

constexpr int f(int x) pre(x >= 0) { return x; }
constexpr int y = f(-1); // no error: CE semantic is ignore
static_assert(y == -1);
