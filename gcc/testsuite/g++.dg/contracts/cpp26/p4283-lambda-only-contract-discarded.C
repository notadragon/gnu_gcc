// A lambda in a function template whose only contract specifier is
// discarded in an instance (its requires-clause is not satisfied) has no
// contract assertion there, so l1(-1.5) is a constant 1; in the int
// instance the precondition is checked.
//
// Mirror: clang/test/Contracts/p4283-lambda-only-contract-discarded.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283 -fcontract-evaluation-semantic=enforce" }

template <class T> concept Int = __is_same (T, int);

template <class T> constexpr int l1 (T t)
{
  auto a = [] (T x) pre requires Int<T> (x > 0) { return 1; };  // { dg-error "contract predicate is false" }
  return a (t);
}
static_assert (l1 (-1.5) == 1);
static_assert (l1 (1) == 1);
static_assert (l1 (-1) == 1);  // error at the predicate

// Both a precondition and a postcondition, both discarded for double.
template <class T> constexpr int l2 (T t)
{
  auto a = [] (T x) pre requires Int<T> (x > 0)
    post requires Int<T> (r: r > 0) { return 1; };
  return a (t);
}
static_assert (l2 (-1.5) == 1);
static_assert (l2 (1) == 1);
