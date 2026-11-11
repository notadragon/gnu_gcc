// P3097 x P3595: callee-side configuration selects different semantics for the
// interface (base) and implementation (override) postconditions of a virtual
// function, matched by source location.  The override's post (in the configured
// line range) is set to ignore; everything else observes.  A polymorphic call
// whose result violates both posts reports only the interface (observed) one.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3097-config-semantic.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

struct Base {
  virtual int f()
    post(r: r > 100)   // interface post -> observe (outside ignore range)
  { return 1; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f() override
    post(r: r > 100)   // implementation post -> ignore (in configured range)
  { return 5; }
};

int main() {
  Derived d;
  Base& b = d;
  violations = 0;
  int r = b.f();
  if (r != 5) __builtin_abort();
  // Implementation post ignored; interface post observed -> exactly one report.
  if (violations != 1) __builtin_abort();
}
