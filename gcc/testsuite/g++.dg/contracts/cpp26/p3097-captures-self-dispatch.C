// P3097+P3098: Virtual function with captures called on own type.
// Regression test: previously ICE'd in expand_expr_real_1 because the
// wrapper and original function shared capture VAR_DECLs, and the
// destructive DECL_INITIAL clearing in the first-processed function
// left the second with uninitialized captures.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int counter = 0;

struct Base {
  virtual int next()
    post [old_c = counter] (r: r == old_c + 1)
  { return ++counter; }
};

struct Derived : Base {
  int next() override { return counter += 2; }
};

int main() {
  // Polymorphic dispatch (wrapper + callee-side both have captures).
  Derived d;
  Base& b = d;
  counter = 0;
  violation_count = 0;
  b.next();
  // Interface post: 2 == 0 + 1 -> false.
  if (violation_count != 1) __builtin_abort();

  // Self-dispatch: Base called on its own type.
  Base obj;
  Base& b2 = obj;
  counter = 0;
  violation_count = 0;
  b2.next();
  // Interface post: 1 == 0 + 1 -> true. Callee post: same -> true.
  if (violation_count != 0) __builtin_abort();
}
