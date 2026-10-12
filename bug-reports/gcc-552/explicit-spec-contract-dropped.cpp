// g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe \
//     explicit-spec-contract-dropped.cpp && ./a.out
//
// An explicit specialization first declared with a precondition and then
// defined without one (a redeclaration may omit the
// function-contract-specifier-seq, [dcl.contract.func]).  ft (1) must
// report a violation of x > 5.  GCC 16.2 and trunk print only "after".
// (A specialization whose definition repeats the contract is checked.)

#include <cstdio>

template <class T> void ft (T x);
template <> void ft<int> (int x) pre (x > 5);
template <> void ft<int> (int x) {}

int main () { ft (1); std::puts ("after"); }
