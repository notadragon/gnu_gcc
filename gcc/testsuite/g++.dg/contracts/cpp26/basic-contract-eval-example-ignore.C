// [basic.contract.eval] Example 1: the array bound depends on the
// evaluation semantic of the contract_assert.  Here the assertion is
// ignored, so it does not increment i: the bound is 1.  Either way the bound is a
// constant expression, so -Wvla stays quiet.
// (Clang mirror: clang/test/Contracts/Sema/basic.contract.eval.cpp)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -Wvla -fcontract-evaluation-semantic=ignore" }

constexpr int f (int i) {
  contract_assert ((++const_cast<int &> (i), true));
  return i;
}
inline void g () {
  int a[f (1)];
  static_assert (sizeof (a) == 1 * sizeof (int));
}
static_assert (f (1) == 1);
