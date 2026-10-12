// [basic.contract.eval] Example 1: the array bound depends on the
// evaluation semantic of the contract_assert.  Here the assertion is
// evaluated during constant evaluation, but GCC then undoes what the
// predicate modified (the evaluation without side effects the wording
// permits), so i is not incremented: the bound is 1, as under ignore.
// Either way the bound is a constant expression, so -Wvla stays quiet.
// (Clang mirror: clang/test/Contracts/Sema/basic.contract.eval.cpp)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -Wvla -fcontract-evaluation-semantic=enforce" }

constexpr int f (int i) {
  contract_assert ((++const_cast<int &> (i), true));
  return i;
}
inline void g () {
  int a[f (1)];
  static_assert (sizeof (a) == 1 * sizeof (int));
}
static_assert (f (1) == 1);
