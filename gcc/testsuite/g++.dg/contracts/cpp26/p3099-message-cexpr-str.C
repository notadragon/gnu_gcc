// P3099: Verify compile-time-generated strings (non-literal diagnostic-message)
// work via cexpr_str (.size() and .data() members).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

struct MyMessage {
    const char* str;
    constexpr int size() const { return __builtin_strlen(str); }
    constexpr const char* data() const { return str; }
};

static const char* last_message = nullptr;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  last_message = v.message();
}

constexpr MyMessage msg{"custom type message"};

void f(int x) pre(x > 0, msg) { }

int main() {
  f(-1);
  if (!last_message || std::strcmp(last_message, "custom type message") != 0)
    __builtin_abort();
}
