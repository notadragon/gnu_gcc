// P3400: <contracts> header provides using contract_control namespace.
// After including <contracts>, labels are available unqualified in assertions.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

// empty_label should be found unqualified via the header's directive.
void f(int x)
  pre<empty_label>(x > 0)
{
}

int g(int x)
  post<empty_label>(r: r >= 0)
{
  return x;
}

void h() {
  contract_assert<empty_label>(true);
}

// contract_control(expr) also works with the header's directive.
constexpr auto my = contract_control(empty_label);

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

int main() {
  f(-1);
  if (violations != 1) __builtin_abort();
  g(-1);
  if (violations != 2) __builtin_abort();
  h();  // passes
  f(1); // passes
}
