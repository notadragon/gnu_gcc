// gcc-01b-contract-pack-tail-ice.cpp                                -*-C++-*-
//
// GCC-1 (b): the same crash one parameter earlier.  When a parameter is
// written AFTER a pack that expands to nothing, null reaches
// set_parm_used_in_post rather than the loop increment.
//
//   g++ -std=c++26 -fcontracts -c gcc-01b-contract-pack-tail-ice.cpp
//
//   internal compiler error: Segmentation fault
//     set_parm_used_in_post               gcc/cp/contracts.cc
//     check_postconditions_in_redecl(...) gcc/cp/contracts.cc
//     tsubst_function_decl                gcc/cp/pt.cc
//
// A parameter after a function parameter pack cannot be deduced, so it is
// reached by naming the pack's arguments explicitly.

template <class... A>
int f (A... a, const int y)
  post (r : r > y)
{ return y; }

void g () { f<> (3); }   // empty pack, one parameter after it: ICE
