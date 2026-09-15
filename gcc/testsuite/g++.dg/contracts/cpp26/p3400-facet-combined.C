// P3400: compute_semantic with combined labels, including empty | empty.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::labels::operator|;

// empty | empty — no compute_semantic, enforce stays enforce.
// (But since enforce terminates, we test with observe below for this case.)

// review | empty_label — review's compute_semantic applies.
void f(int x)
  pre<(review | empty_label)>(x > 0)
{
}

// empty_label | review — same result (chaining: empty has no compute_semantic,
// review does, so only review's applies).
void g(int x)
  pre<(empty_label | review)>(x > 0)
{
}

// review | review — chained: enforce → observe → observe (idempotent).
void h(int x)
  pre<(review | review)>(x > 0)
{
}

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

int main() {
  // All three functions have review in the chain, so enforce → observe.
  // Handler is called, execution continues.
  f(-1);
  if (violations != 1) __builtin_abort();
  g(-1);
  if (violations != 2) __builtin_abort();
  h(-1);
  if (violations != 3) __builtin_abort();
}
