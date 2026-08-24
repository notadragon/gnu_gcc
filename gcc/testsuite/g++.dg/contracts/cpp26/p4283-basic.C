// P4283: Basic requires clause on contract assertions.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283" }

#include <concepts>

template <typename T>
void f(T x)
  pre requires(std::integral<T>) (x > 0);

template <typename T>
T g(T x)
  post requires(std::integral<T>) (r: r >= 0)
{
  return x;
}

template <typename T>
void h(T x) {
  contract_assert requires(std::integral<T>) (x > 0);
}

void test() {
  f(42);
  f(3.14);
  g(10);
  g(1.5);
  h(5);
  h(2.0);
}
