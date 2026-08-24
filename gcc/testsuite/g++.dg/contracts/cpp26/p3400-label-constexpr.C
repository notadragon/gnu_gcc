// P3400: Various constexpr forms for assertion-control labels.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct my_label_t {
  using assertion_control_object = my_label_t;
};

// Constexpr variable.
constexpr my_label_t my_label{};

// Constexpr function returning a label.
constexpr my_label_t get_label() { return my_label_t{}; }

// Direct constructor invocation as the label expression.
void f(int x)
  pre<my_label_t{}>(x > 0)
{
}

// Constexpr variable.
void g(int x)
  pre<my_label>(x > 0)
{
}

// Constexpr function call.
void h(int x)
  pre<get_label()>(x > 0)
{
}

// Library-provided empty_label.
void k(int x)
  pre<std::contracts::labels::empty_label>(x > 0)
{
}

// contract_assert with constexpr function.
void m() {
  contract_assert<get_label()>(true);
}

// postcondition with constexpr variable.
int n(int x)
  post<my_label>(r: r >= 0)
{
  return x;
}

static int handler_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++handler_count;
}

int main() {
  f(-1);
  if (handler_count != 1) __builtin_abort();
  g(-1);
  if (handler_count != 2) __builtin_abort();
  h(-1);
  if (handler_count != 3) __builtin_abort();
  k(-1);
  if (handler_count != 4) __builtin_abort();
  m();  // passes
  n(-1);
  if (handler_count != 5) __builtin_abort();
}
