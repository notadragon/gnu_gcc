// P4283 x attributes: confirm a contract specifier can carry both a
// requires-clause and an attribute together (grammar: `pre requires(C)
// [[attr]] (predicate)`).  Clang once had a TrailingObjects layout bug for
// exactly this combination (BUG-1, a requires-clause slot shifting the
// attribute array by one).  GCC has no equivalent representation --
// attributes on a contract specifier are parsed and discarded (with a
// warning) rather than stored -- so there is no analogous defect; this just
// confirms the combination is accepted at all, which was previously
// unverified on either compiler beyond this test.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283" }

#include <concepts>

// pre: attribute after the requires clause.
template<typename T>
T pre_attr_after (T x)
  pre requires (std::integral<T>) [[unlikely]] (x > 0) // { dg-warning "attributes are ignored" }
{ return x; }

// post: same, with a result name.
template<typename T>
T post_attr_after (const T x)
  post requires (std::integral<T>) [[unlikely]] (r: r == x) // { dg-warning "attributes are ignored" }
{ return x; }

// contract_assert inside a template.
template<typename T>
void assert_attr (T x)
{
  contract_assert requires (std::integral<T>) [[unlikely]] (x != T{}); // { dg-warning "attributes are ignored" }
}

// More than one attribute, to catch an off-by-one that only shows past index 0.
template<typename T>
T multi_attr (T x)
  pre requires (std::integral<T>) [[unlikely]] [[maybe_unused]] (x >= 0) // { dg-warning "attributes are ignored" }
{ return x; }

template int pre_attr_after<int> (int);
template int post_attr_after<int> (const int);
template void assert_attr<int> (int);
template int multi_attr<int> (int);
