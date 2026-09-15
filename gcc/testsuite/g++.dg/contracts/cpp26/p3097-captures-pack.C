// P3097+P3098: Pack captures on a virtual member function.  A parameter pack on
// a virtual function is only expressible when the pack comes from the enclosing
// class template (a virtual function cannot itself be a function template).
//
// Two hazards cross here: rejecting the capture at instantiation, and
// evaluating the interface *pack* capture incorrectly across virtual dispatch
// -- a wrapper that emits no capture initializer leaves the captured pack
// holding wrong values, so even a holding predicate reports a violation.  The
// interface postcondition's pack capture must be captured and checked
// correctly across virtual dispatch.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int viol = 0;
void handle_contract_violation (const std::contracts::contract_violation&) { ++viol; }

template <typename... Ts>
bool allpos (Ts... x) { return (... && (x > 0)); }

template <typename... Ts>
struct Base {
  virtual bool f (Ts... ts)
    post [ts...] (r: r == allpos (ts...))
  { return allpos (ts...); }
  virtual ~Base () = default;
};

struct Derived : Base<int, int, int> {
  bool f (int a, int b, int) override { return a > 0 && b > 0; }
};

int main () {
  Derived d;
  Base<int, int, int>& b = d;
  (void) b.f (1, 2, 3);    // Derived -> true; allpos(1,2,3)=true -> holds
  if (viol != 0) __builtin_abort ();
  (void) b.f (1, 2, -3);   // Derived -> true; allpos(1,2,-3)=false -> violation
  if (viol != 1) __builtin_abort ();
}
