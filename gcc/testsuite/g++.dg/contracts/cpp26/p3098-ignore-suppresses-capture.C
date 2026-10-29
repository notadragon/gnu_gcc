// P3098 x P3400 single-unit rule: when a postcondition resolves to the ignore
// semantic, the capture is NOT constructed (its initializer side effect does not
// run) and the predicate is not evaluated -- construction, predicate and
// destruction are gated as a unit.  Here ignore is the base semantic.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int init_count = 0;
static int made(int v) { ++init_count; return v; }

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

// Capture initializer has an observable side effect (init_count) and the
// captured value is used in the predicate (so it cannot be elided as dead).
int f() post [old = made(5)] (r: r == old) { return 7; }  // 7 == 5 would fail

int main() {
  init_count = 0;
  violation_count = 0;
  if (f() != 7) __builtin_abort();
  if (init_count != 0) __builtin_abort();      // capture not constructed
  if (violation_count != 0) __builtin_abort(); // predicate not evaluated
}
