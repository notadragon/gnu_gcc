// During constant evaluation a false or not-constant predicate is recorded
// and evaluation continues.  If the evaluation completes, every recorded
// problem is reported at its own assertion (GCC-619).  If it does not
// complete, nothing recorded is reported and the only error is that the
// expression is not constant (GCC-618).
//
// Mirror: clang/test/Contracts/constexpr-contract-every-problem.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fsyntax-only" }
// Where the non-constant expression is reported is not what this tests.
// { dg-prune-output "error: (?!contract)" }

// Both preconditions are false and the evaluation completes.
constexpr int
f1 (const int &v)
  pre (false) // { dg-error "contract predicate is false" }
  pre (false) // { dg-error "contract predicate is false" }
{
  return 1;
}

int
use1 (int p)
{
  const int k = f1 (p);
  return k;
}

// The first is not constant, the second false; the evaluation completes.
constexpr int
f2 (const int &v)
  pre (v > 0) // { dg-error "contract condition is not constant" }
  pre (false) // { dg-error "contract predicate is false" }
{
  return 1;
}

int
use2 (int p)
{
  const int k = f2 (p);
  return k;
}

// The body reads a non-constant object, so the constexpr variable is simply
// not constant; the false precondition is not reported.
int g = 1;

constexpr int
f3 (const int &v)
  pre (false) // { dg-bogus "contract" }
{
  return v;
}

constexpr int k3 = f3 (g);
