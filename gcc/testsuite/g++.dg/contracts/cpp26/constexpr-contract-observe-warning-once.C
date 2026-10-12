// Under observe, a contract problem is warned about once, however many
// times the implementation evaluates the expression.
//
// Mirror: clang/test/Contracts/constexpr-contract-observe-warning-once.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe -fsyntax-only" }

constexpr int
f (const int &v)
  pre (false) // { dg-warning "contract predicate is false" }
{
  return 1;
}

static_assert (f (1) == 1);

// Here the first precondition is not constant (it reads g) and the second is
// false; each is still warned about once.
constexpr int
h (const int &v)
  pre (v > 0) // { dg-warning "contract condition is not constant" }
  pre (false) // { dg-warning "contract predicate is false" }
{
  return 1;
}

int g = 1;
static_assert (h (g) == 1);
