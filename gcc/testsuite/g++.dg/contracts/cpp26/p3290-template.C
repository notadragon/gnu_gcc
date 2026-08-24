// P3290 x templates: the library violation-triggering API invoked from within a
// function template works for each instantiation.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static int handler_count = 0;
static const char* last_comment = nullptr;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  ++handler_count;
  last_comment = v.comment();
  if (v.kind() != std::contracts::assertion_kind::manual) __builtin_abort();
}

template <typename T>
void trigger(T) {
  std::contracts::handle_observed_contract_violation("from template");
}

int main() {
  handler_count = 0;
  trigger(1);      // int instantiation
  trigger(1.0);    // double instantiation
  if (handler_count != 2) __builtin_abort();
  if (!last_comment || std::strcmp(last_comment, "from template") != 0)
    __builtin_abort();
}
