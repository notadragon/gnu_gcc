// P3099: User-defined diagnostic messages for contract assertions.
// Basic parsing and runtime behavior.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

void f(int x) pre(x > 0, "x must be positive") { }

int g(int x)
  pre(x >= 0, "non-negative input")
  post(r: r >= 0, "non-negative result")
{
  return x * 2;
}

void h() {
  contract_assert(true, "always passes");
}

// Custom handler to verify message content.
static const char* last_message = nullptr;
static const char* last_comment = nullptr;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  last_comment = v.comment();
  last_message = v.message();
}

void test_pre_message() {
  last_message = nullptr;
  last_comment = nullptr;
  f(-1);
  if (!last_message || std::strcmp(last_message, "x must be positive") != 0)
    __builtin_abort();
  if (!last_comment)
    __builtin_abort();
}

void test_no_message() {
  // A contract without a message should return nullptr from message().
  // We need a separate function without a message for this test.
}

int main() {
  test_pre_message();
  // If we get here, the custom handler was called and message was correct.
  f(1);   // should not trigger
  g(5);   // should not trigger
  h();    // should not trigger
}
