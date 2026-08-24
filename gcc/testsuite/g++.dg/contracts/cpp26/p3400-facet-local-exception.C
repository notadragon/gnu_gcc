// P3400: Local handler throws exception — propagates out, skips global handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::violation_handled;

static int global_calls = 0;

struct throwing_handler_t {
  using assertion_control_object = throwing_handler_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    throw 42;
  }
};
constexpr throwing_handler_t throwing_handler{};

void handle_contract_violation(const contract_violation&) {
  ++global_calls;
}

void f(int x) pre<throwing_handler>(x > 0) { }

int main() {
  bool caught = false;
  try {
    f(-1);
  } catch (int e) {
    caught = true;
    if (e != 42) __builtin_abort();
  }
  if (!caught) __builtin_abort();
  if (global_calls != 0) __builtin_abort();
}
