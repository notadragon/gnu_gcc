// P3098 x P3400 single-unit rule: under the observe semantic the capture IS
// constructed (initializer side effect runs), the predicate is evaluated, the
// handler is called on failure, and execution continues.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int init_count = 0;
static int made(int v) { ++init_count; return v; }

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

int f() post [old = made(5)] (r: r == old) { return 7; }  // 7 == 5 -> fails

int main() {
  init_count = 0;
  violation_count = 0;
  if (f() != 7) __builtin_abort();
  if (init_count != 1) __builtin_abort();      // capture constructed once
  if (violation_count != 1) __builtin_abort(); // predicate evaluated, reported
}
