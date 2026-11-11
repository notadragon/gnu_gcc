// P4405 proof shadow: a path through an exception thrown in an inlined
// member function.  In each function the call of at () throws (thr is
// noreturn and the pointer is null), the handler sets c, and the
// assertion after the try block is proven.  From the D4405 follow-up
// program's research on P4021 (R1 finding (A): GCC proves these).
// Mirror: clang/test/Contracts/p4405-shadow-exception-inlined.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }

#include <contracts>
using namespace std::contracts::labels;

[[noreturn]] void thr ();
struct S { int *p = nullptr; int at (int i) { if (!p) thr (); return p[i]; } };
struct P { int *p; int at (int i) { if (!p) thr (); return p[i]; } };

void x1 ()
{
  bool c = false;
  try { S s; s.at (0); } catch (...) { c = true; }
  contract_assert<always_prove> (c);	// { dg-bogus "(cannot be proven|always false)" }
}

void x2 ()
{
  bool c = false;
  try { P s; s.p = nullptr; s.at (0); } catch (...) { c = true; }
  contract_assert<always_prove> (c);	// { dg-bogus "(cannot be proven|always false)" }
}

void x3 ()
{
  bool c = false;
  try { P s{nullptr}; s.at (0); } catch (...) { c = true; }
  contract_assert<always_prove> (c);	// { dg-bogus "(cannot be proven|always false)" }
}

// Control: no throw, so nothing sets c.
void x4 (int *q)
{
  bool c = false;
  try { P s{q}; if (q) s.at (0); } catch (...) { c = true; }
  contract_assert<always_prove> (c);	// { dg-error "always false" }
}
