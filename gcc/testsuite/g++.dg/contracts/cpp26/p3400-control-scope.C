// P3400: contract_control names are NOT visible in regular expressions.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400" }

namespace my_labels {
  struct my_label_t {
    using assertion_control_object = my_label_t;
  };
  constexpr my_label_t my_label{};
  constexpr int value = 42;
}

using contract_control namespace my_labels;

// Visible in assertion-control expression:
void f(int x) pre<my_label>(x > 0) { }

// NOT visible in regular code:
int y = value;  // { dg-error "not declared" }
auto z = my_label;  // { dg-error "not declared" }
