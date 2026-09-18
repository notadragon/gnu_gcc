// P3400: Test multiple -fcontract-group-evaluation-semantic flags and
// prefix matching on '.' delimiter.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-group-evaluation-semantic=safety:observe -fcontract-group-evaluation-semantic=perf:ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// "safety" group — configured to observe
void f_safety(int x) pre<"safety"group>(x > 0) { }

// "safety.memory" — prefix-matches "safety" config → observe
void f_safety_mem(int x) pre<"safety.memory"group>(x > 0) { }

// "safety.memory.bounds" — also prefix-matches "safety" → observe
void f_safety_deep(int x)
  pre<"safety.memory.bounds"group>(x > 0)
{ }

// "perf" group — configured to ignore
void f_perf(int x) pre<"perf"group>(x > 0) { }

// "perf.cache" — prefix-matches "perf" → ignore
void f_perf_cache(int x) pre<"perf.cache"group>(x > 0) { }

// "safetyx" — does NOT prefix-match "safety" (no '.' after "safety")
// falls through to default (enforce)
void f_safetyx(int x) pre<"safetyx"group>(x > 0) { }

int main() {
  // safety → observe, handler called
  f_safety(-1);
  if (violations != 1) __builtin_abort();

  // safety.memory → observe via prefix match
  f_safety_mem(-1);
  if (violations != 2) __builtin_abort();

  // safety.memory.bounds → observe via prefix match
  f_safety_deep(-1);
  if (violations != 3) __builtin_abort();

  // perf → ignore, no handler, no abort
  f_perf(-1);
  if (violations != 3) __builtin_abort();

  // perf.cache → ignore via prefix match
  f_perf_cache(-1);
  if (violations != 3) __builtin_abort();

  // safetyx — NOT a prefix match for "safety" (missing '.'),
  // falls through to default. Don't violate since default is enforce.
  f_safetyx(1);
  if (violations != 3) __builtin_abort();
}
