// P3098 x P3400 single-unit rule with compute_semantic: the 'review' label
// (enforce -> observe) applied to a capturing postcondition keeps the capture
// live -- it is constructed once, the predicate is evaluated, and because the
// resolved semantic is observe the handler runs and execution continues.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int init_count = 0;
static int made(int v) { ++init_count; return v; }

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

int f() post<review> [old = made(5)] (r: r == old) { return 7; }  // 7 == 5 -> fails

int main() {
  init_count = 0;
  violation_count = 0;
  if (f() != 7) __builtin_abort();
  if (init_count != 1) __builtin_abort();      // capture constructed once
  if (violation_count != 1) __builtin_abort(); // observe: reported, continued
}
