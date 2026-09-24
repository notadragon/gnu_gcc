// P4283: Error cases for requires clauses on contract assertions.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283" }

#include <concepts>

// Error: requires on non-templated function.
void non_template(int x)
  pre requires(std::integral<int>) (x > 0);  // { dg-error "only allowed on templated functions" }

void non_template2(int x) {
  contract_assert requires(std::integral<int>) (x > 0);  // { dg-error "only allowed on templated functions" }
}
