// A not-constant precondition met while deciding whether a const local is
// constant-initialized, under observe: the evaluation completes, so the
// predicate is a contract violation reported as a warning, and the variable
// is still constant-initialized (usable in a constant expression).
//
// Mirror: clang/test/Contracts/constexpr-contract-observe-not-constant-init.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe -fsyntax-only" }

constexpr int
f (const int &v)
  pre (v > 0) // { dg-warning "contract condition is not constant" }
{
  return 1;
}

int
use (int p)
{
  const int k = f (p);
  static_assert (k == 1);
  return k;
}
