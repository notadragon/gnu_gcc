// P3098 x P3400 x P3595 single-unit rule via group configuration: a capturing
// postcondition tagged with an identification-label group that the
// -fcontract-group-evaluation-semantic flag sets to ignore must NOT construct
// its capture nor evaluate its predicate, even though the base semantic is
// enforce.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p3098 -fcontract-group-evaluation-semantic=g:ignore -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int init_count = 0;
static int made(int v) { ++init_count; return v; }

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

// Group "g" configured to ignore; base semantic enforce.
int f() post<"g"group> [old = made(5)] (r: r == old) { return 7; }

int main() {
  init_count = 0;
  violation_count = 0;
  if (f() != 7) __builtin_abort();
  if (init_count != 0) __builtin_abort();      // capture not constructed
  if (violation_count != 0) __builtin_abort(); // predicate not evaluated
}
