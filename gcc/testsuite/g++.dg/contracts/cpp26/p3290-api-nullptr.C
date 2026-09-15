// P3290: nullptr comment handled gracefully.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static const char* last_comment = "unset";

void handle_contract_violation(const std::contracts::contract_violation& v) {
  last_comment = v.comment();
}

int main() {
  std::contracts::handle_observed_contract_violation(nullptr);
  if (!last_comment || std::strcmp(last_comment, "") != 0)
    __builtin_abort();
}
