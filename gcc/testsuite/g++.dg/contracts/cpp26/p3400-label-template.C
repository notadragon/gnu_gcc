// P3400: Variable template as label with >> (rshift split in nested context).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct empty_label_t {
  using assertion_control_object = empty_label_t;
};

template<typename T>
constexpr empty_label_t typed_label{};

// Function template with a labeled contract using a variable template.
template<typename T>
void check(T x)
  pre<typed_label<T>>(x > T{})
{
}

// Nested template in postcondition.
template<typename T>
T positive(T x)
  post<typed_label<T>>(r: r > T{})
{
  return x;
}

static int handler_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++handler_count;
}

int main() {
  check(1);   // passes
  check(-1);  // violates
  if (handler_count != 1) __builtin_abort();

  positive(5);   // passes
  positive(-1);  // violates post
  if (handler_count != 2) __builtin_abort();
}
