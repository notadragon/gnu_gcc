// P4451 [dcl.contract.func]: two function contract assertions are the same
// only if both have an assertion-control-specifier or neither does.  Under
// -fcontracts-p3400 and -fcontracts-p4451 (the P3400 label).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p4451" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

void f (int x) pre<terminating> (x > 0);
void f (int x) pre (x > 0) { }	// { dg-error "mismatched assertion-control label in declaration" }

void g (int x) pre (x > 0);
void g (int x) pre<terminating> (x > 0) { }	// { dg-error "mismatched assertion-control label in declaration" }

void h (const int x) pre<terminating> (x > 0) post<terminating> (x > 0);
void h (const int x) pre<terminating> (x > 0) post<terminating> (x > 0) { }

struct S
{
  void m (int x) pre<terminating> (x > 0);
  void n (int x) pre<terminating> (x > 0);
};
void S::m (int x) pre (x > 0) { }	// { dg-error "mismatched assertion-control label in declaration" }
void S::n (int x) pre<terminating> (x > 0) { }
