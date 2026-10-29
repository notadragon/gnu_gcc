// gcc-646-pch-template-contracts.cpp                             -*-C++-*-
//
// GCC-646: a function template declared in a precompiled header and
// instantiated after it loses its contract assertions.
//
//   g++ -std=c++26 -fcontracts -x c++-header gcc-646-pch-template-contracts.h \
//       -o gcc-646-pch-template-contracts.h.gch
//   g++ -std=c++26 -fcontracts -fsyntax-only gcc-646-pch-template-contracts.cpp
//
// Expected: "contract predicate is false in constant expression" (as without
// the .gch).  Actual: accepted, for most builds of the .gch.

#include "gcc-646-pch-template-contracts.h"

static_assert (ct (0) == 0);
