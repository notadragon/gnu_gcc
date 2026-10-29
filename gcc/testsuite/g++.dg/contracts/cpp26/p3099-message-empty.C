// P3099: Verify empty string message is distinct from no message.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

void f(int x) pre(x > 0, "") { }

static bool handler_called = false;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  handler_called = true;
  // An empty string message should NOT be nullptr.
  if (v.message() == nullptr)
    __builtin_abort();
  // It should be an empty string.
  if (std::strcmp(v.message(), "") != 0)
    __builtin_abort();
}

int main() {
  f(-1);
  if (!handler_called)
    __builtin_abort();
}
