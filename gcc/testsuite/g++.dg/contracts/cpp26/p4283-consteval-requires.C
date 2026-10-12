// P4283 x consteval: a constrained contract assertion on a consteval
// function template (and on a contract_assert in one) is kept or discarded
// per instantiation, and a kept one is checked when the immediate
// invocation is evaluated.  int satisfies the constraint, so its false
// precondition makes the invocation ill-formed; double does not, so the
// same call is a constant.  A satisfied constraint with a true predicate is
// fine.
//
// Mirror: clang/test/Contracts/p4283-consteval-requires.cpp in the
// llvm_llvm-project fork; owed to EDG, recorded in its open-issues/README.md.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283 -fcontract-evaluation-semantic=enforce" }

#include <concepts>

template <typename T>
consteval T
f (T x)
  pre requires (std::integral<T>) (x > 0) // { dg-error "contract predicate is false in constant expression" }
{
  return x;
}

template <typename T>
consteval T
g (T x)
{
  contract_assert requires (std::integral<T>) (x > 0); // { dg-error "contract predicate is false in constant expression" }
  return x;
}

static_assert (f (1) == 1);
static_assert (f (-1.5) == -1.5);	// discarded for double
static_assert (g (2) == 2);
static_assert (g (-2.5) == -2.5);	// discarded for double
// Diagnosed at the assertions above.
constexpr int bad_f = f (-1);
constexpr int bad_g = g (-1);
