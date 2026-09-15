// P3400: compute_semantic facet — review label transforms enforce to observe.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int handler_count = 0;
static bool terminated = false;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  ++handler_count;
}

// With review label: enforce is transformed to observe.
// Handler is called but execution CONTINUES (not terminate).
void f(int x)
  pre<review>(x > 0)
{
}

// Without label: enforce means terminate after handler.
// (We don't test this here since it would abort.)

int main() {
  f(-1);  // violates — handler called, continues (review made it observe)
  if (handler_count != 1) __builtin_abort();

  f(-2);  // violates again — still continues
  if (handler_count != 2) __builtin_abort();

  f(1);   // passes
  if (handler_count != 2) __builtin_abort();
}
