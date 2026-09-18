// P3099: Verify that message syntax is rejected without -fcontracts-p3099.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void f(int x) pre(x > 0, "message") { }  // { dg-error "expected" }
