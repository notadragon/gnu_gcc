// gcc-01a-contract-pack-empty-ice.cpp                               -*-C++-*-
//
// GCC-1 (a): a contract specifier on a function template whose parameter
// pack expands to NOTHING segfaults the compiler.
//
//   g++ -std=c++26 -fcontracts -c gcc-01a-contract-pack-empty-ice.cpp
//
//   internal compiler error: Segmentation fault
//     contains_struct_check(...)          gcc/tree.h
//     check_postconditions_in_redecl(...) gcc/cp/contracts.cc
//     tsubst_function_decl                gcc/cp/pt.cc
//
// Note there is no postcondition anywhere: check_postconditions_in_redecl
// returns early only when the function has no contract specifiers at all, so
// a bare `pre` is enough to start the walk that crashes.
//
// A non-empty pack -- f (1, 2) -- compiles.  Removing the `pre` compiles.

template <class... A>
void f (int x, A&&... a)
  pre (x > 0)
{ }

void g () { f (1); }   // empty pack: ICE
