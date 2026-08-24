// P3097+P3098: Pack captures on a virtual member function.  A parameter pack on
// a virtual function is only expressible when the pack comes from the enclosing
// class template (a virtual function cannot itself be a function template).
//
// Previously BUG-1 rejected the capture at instantiation, and then BUG-20
// evaluated the interface *pack* capture incorrectly across virtual dispatch
// (the wrapper never emitted the capture initializer, so the captured pack held
// wrong values and even a holding predicate reported a violation).  Both are now
// fixed -- the interface postcondition's pack capture is captured and checked
// correctly across virtual dispatch.  See testing-gap-catalogue.md section 10.
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
