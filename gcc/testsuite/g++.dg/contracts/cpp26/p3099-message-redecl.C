// P3099: Verify redeclaration sameness checking for diagnostic messages.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3099" }

void f(int x) pre(x > 0, "must be positive");
void f(int x) pre(x > 0, "different message");  // { dg-error "mismatched contract diagnostic message" }

void g(int x) pre(x > 0, "same message");
void g(int x) pre(x > 0, "same message");  // OK

void h(int x) pre(x > 0, "has message");
void h(int x) pre(x > 0);  // { dg-error "mismatched contract diagnostic message" }

void i(int x) pre(x > 0);
void i(int x) pre(x > 0);  // OK, neither has a message
