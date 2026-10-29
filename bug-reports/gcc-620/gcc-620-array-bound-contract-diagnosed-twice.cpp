// GCC-620: a contract violation found while constant-evaluating an array
// bound is diagnosed twice, byte-identical at the same location.
//
//   g++ -std=c++26 -fcontracts -fsyntax-only gcc-620-array-bound-contract-diagnosed-twice.cpp
//
//   error: contract predicate is false in constant expression   (twice)
//
// Under -fcontract-evaluation-semantic=observe the warning is doubled the
// same way.  A const int initialized from the same call reports it once.

constexpr int f(const int& v) pre(false) { return 1; }

int use(int p)
{
  char a[f(p)];
  return sizeof a;
}
