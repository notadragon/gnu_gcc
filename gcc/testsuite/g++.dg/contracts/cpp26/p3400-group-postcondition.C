// P3400: Test identification_label facet with postconditions.
// Verifies group_names extraction works when a postcondition result
// name is in scope (Bug 1: ICE at constexpr.cc in cxx_eval_component_reference).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-group-evaluation-semantic=safety:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// Postcondition with group label and result name binding.
int f_post(int x)
  post<"safety"group>(r: r > 0)
{
  return x;
}

// Precondition with group label (baseline — always worked).
void f_pre(int x) pre<"safety"group>(x > 0) { }

// Postcondition with group label, no result name.
void f_post_no_result()
  post<"safety"group>(true)
{ }

int main() {
  // safety:observe — handler called, continues.
  f_pre(-1);
  if (violations != 1) __builtin_abort();

  // Postcondition with result name: r = -1, r > 0 is false.
  f_post(-1);
  if (violations != 2) __builtin_abort();

  // Non-violating postcondition.
  f_post(1);
  if (violations != 2) __builtin_abort();

  // Postcondition without result name.
  f_post_no_result();
  if (violations != 2) __builtin_abort();
}
