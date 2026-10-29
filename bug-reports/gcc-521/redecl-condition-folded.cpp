// The predicates of two declarations' contracts must satisfy the
// one-definition rule ([dcl.contract.func]).  `x > 0', `x > 1 - 1' and
// `x > z' are different token sequences, so each redeclaration is ill-formed.
//
//   g++ -std=c++26 -fcontracts -fsyntax-only redecl-condition-folded.cpp
//
// Expected: an error on lines 13 and 14.  GCC accepts both (it does diagnose
// `x > 1' against `x > 0').
constexpr int z = 0;
void f (int x) pre (x > 0);
void g (int x) pre (x > 0);

void f (int x) pre (x > 1 - 1) {}
void g (int x) pre (x > z) {}
