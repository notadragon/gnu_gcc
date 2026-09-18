// P3400: operator| found through contract_control lookup without explicit using.
// The <contracts> header's "using contract_control namespace std::contracts::labels;"
// makes operator| visible inside <...> and contract_control(...).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

// User-defined labels — no using declaration for operator| here.
struct alpha_t { using assertion_control_object = alpha_t; };
struct beta_t { using assertion_control_object = beta_t; };
constexpr alpha_t alpha{};
constexpr beta_t beta{};

// operator| found inside assertion-control expression via header's directive.
void f(int x)
  pre<(alpha | beta)>(x > 0)
{
}

// operator| found inside contract_control(expr) via header's directive.
constexpr auto combined = contract_control(alpha | beta | empty_label);
void g(int x)
  pre<combined>(x > 0)
{
}

// Chaining: result of | is itself an assertion_control_object, can be |'d again.
void h(int x)
  pre<(alpha | beta | empty_label)>(x > 0)
{
}

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

int main() {
  f(-1);
  if (violations != 1) __builtin_abort();
  g(-1);
  if (violations != 2) __builtin_abort();
  h(-1);
  if (violations != 3) __builtin_abort();
}
