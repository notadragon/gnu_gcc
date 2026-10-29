// As decl-contract-lambda-default-arg.C, at namespace scope.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void f0 (int x) pre ([x] { return x > 0; } ());

void f4 (int y = 1);
