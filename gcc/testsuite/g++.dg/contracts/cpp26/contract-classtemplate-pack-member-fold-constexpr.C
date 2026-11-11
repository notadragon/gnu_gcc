// A fold over a class template's pack in a precondition or postcondition of
// a constexpr member function declared in the class and defined outside it
// is checked in constant evaluation: the holding calls are constant, and
// each violation is diagnosed at the violated predicate (for g, the one
// repeated on the definition).  contract-classtemplate-pack-member-fold.C has the
// run-time checks.  Mirrored from the EDG front end (EDG-124 in the
// edgcpp_compiler fork).
//
// Mirror: tests/tests/contracts/constexpr/class_pack_member_fold.sft.cpp in
// the edgcpp_compiler fork;
// clang/test/Contracts/contract-classtemplate-pack-member-fold-constexpr.cpp
// in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

template <class... T> struct Z
{
  constexpr int f (const T... t) const pre (((t > 0) && ...));	// { dg-error "contract predicate is false in constant expression" }
  constexpr auto g (const T... t) const post (r : ((t > r) && ...));
};
// Other parameter names on the definition; the contract not repeated.
template <class... T>
constexpr int Z<T...>::f (const T... b) const { return 0; }
template <class... T>
constexpr auto Z<T...>::g (const T... t) const post (r : ((t > r) && ...))	// { dg-error "contract predicate is false in constant expression" }
{
  return 0;
}

constexpr Z<int, int> z;
static_assert (z.f (1, 2) == 0);	// OK
static_assert (z.g (1, 2) == 0);	// OK
constexpr int bad_f = z.f (1, -2);	// Error, at f's precondition
constexpr int bad_g = z.g (1, 0);	// Error, at g's postcondition
