// A4 / F30 (part 2): in constant evaluation, every non-terminating (observe)
// contract violation is reported, not just the first.  Before the fix a single
// slot meant only the first observe violation was warned per top-level constant
// evaluation.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3850 -fcontract-evaluation-semantic=observe" }
#include <contracts>

// Three observe violations in a single constant evaluation; all three must be
// reported (each contract_assert is const-but-false), and evaluation continues.
constexpr int
f (int a, int b, int c)
{
  contract_assert (a > 0); // { dg-warning "contract predicate is false in constant expression" }
  contract_assert (b > 0); // { dg-warning "contract predicate is false in constant expression" }
  contract_assert (c > 0); // { dg-warning "contract predicate is false in constant expression" }
  return a + b + c;
}

constexpr int bad = f (-1, -2, -3);
