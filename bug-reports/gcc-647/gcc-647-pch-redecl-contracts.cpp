// gcc-647-pch-redecl-contracts.cpp                               -*-C++-*-
//
// GCC-647: after a precompiled header, a redeclaration of a function the
// header declared with contract assertions is checked as though the
// header's declarations had none.
//
//   g++ -std=c++26 -fcontracts -x c++-header gcc-647-pch-redecl-contracts.h \
//       -o gcc-647-pch-redecl-contracts.h.gch
//   g++ -std=c++26 -fcontracts -fsyntax-only gcc-647-pch-redecl-contracts.cpp
//
// Expected: line A accepted; line B "mismatched contract condition".
// Actual:   "declaration adds contracts" on both, for most builds of the .gch.

#include "gcc-647-pch-redecl-contracts.h"

int nf (int x) pre (x > 1);   // A
int nf2 (int x) pre (x > 2);  // B
