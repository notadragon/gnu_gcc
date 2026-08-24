// P3099 x templates: a user-defined message on a contract inside a function
// template is carried to the handler for each instantiation.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static const char* last_message = nullptr;
static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  ++violation_count;
  last_message = v.message();
}

template <typename T>
T f(T x) pre(x > T{}, "must be positive") { return x; }

int main() {
  last_message = nullptr;
  violation_count = 0;
  (void) f(-1);   // int instantiation, pre fails
  if (violation_count != 1) __builtin_abort();
  if (!last_message || std::strcmp(last_message, "must be positive") != 0)
    __builtin_abort();

  last_message = nullptr;
  (void) f(-1.0); // double instantiation, same message
  if (violation_count != 2) __builtin_abort();
  if (!last_message || std::strcmp(last_message, "must be positive") != 0)
    __builtin_abort();
}
