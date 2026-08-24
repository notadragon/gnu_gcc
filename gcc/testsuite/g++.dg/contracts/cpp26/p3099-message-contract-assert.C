// P3099: a user-defined diagnostic-message on a *contract_assert* statement is
// delivered to the handler when the assertion fails at run time.  p3099-message-
// basic.C exercises runtime delivery for pre and post (its contract_assert holds);
// this covers the third assertion form P3099 applies to, failing at run time.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static const char* last_message = nullptr;
static int violations = 0;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  last_message = v.message();
  ++violations;
}

void f(int x) {
  contract_assert(x > 0, "assert message");
}

int main() {
  f(-1);
  if (violations != 1)
    __builtin_abort();
  if (!last_message || std::strcmp(last_message, "assert message") != 0)
    __builtin_abort();
}
