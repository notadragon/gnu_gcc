// gcc-01c-contract-pack-tail-misattributed.cpp                      -*-C++-*-
//
// GCC-1 (c): the non-crashing sibling, and the one that matters most -- a
// WELL-FORMED PROGRAM IS REJECTED, naming the wrong parameter.
//
//   g++ -std=c++26 -fcontracts -c gcc-01c-contract-pack-tail-misattributed.cpp
//
//   error: value parameter 'a#1' used in a postcondition must be const
//
// The postcondition uses only `y`, which IS const, so the program is valid.
// A parameter written after a pack that expands to N sits N-1 slots further
// along in the instantiation than in the pattern, so the lockstep walk in
// check_postconditions_in_redecl pairs `y` with a pack element and blames
// that element for `y`'s use.
//
// The tail parameter's type must be dependent for this to be reached: a
// non-dependently-typed parameter is checked while the contract is parsed,
// before instantiation.

template <class... A, class T>
int f (A... a, const T y)
  post (r : r > y)
{ return y; }

void g () { f<int, int> (1, 2, 3); }   // rejected; should compile
