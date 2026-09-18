// P3400: Test identification_label facet — basic group labels and
// the -fcontract-group-evaluation-semantic flag.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-group-evaluation-semantic=safety:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// "safety" group — configured to observe
void f_safety(int x) pre<"safety"group>(x > 0) { }

// "other" group — no config, falls through to default (enforce)
void f_other(int x) pre<"other"group>(x > 0) { }

// No group — falls through to default (enforce)
void f_plain(int x) pre(x > 0) { }

int main() {
  // safety:observe — handler called, continues
  f_safety(-1);
  if (violations != 1) __builtin_abort();

  // Non-violating calls should work fine
  f_safety(1);
  f_other(1);
  f_plain(1);
  if (violations != 1) __builtin_abort();
}
