// gcc-646-pch-twelve-templates.cpp                              -*-C++-*-
//
// GCC-646, case b (see gcc-646-pch-twelve-templates.h).
//
//   g++ -std=c++26 -fcontracts -x c++-header gcc-646-pch-twelve-templates.h \
//       -o gcc-646-pch-twelve-templates.h.gch
//   g++ -std=c++26 -fcontracts -fsyntax-only gcc-646-pch-twelve-templates.cpp
//
// Expected: twelve "contract predicate is false in constant expression"
// errors (as without the .gch).  Actual on stock: fewer (1 of 12 on
// 17.0.0 20261110, with address-space randomization off).

#include "gcc-646-pch-twelve-templates.h"

static_assert (ct0 (0) == 0);
static_assert (ct1 (0) == 0);
static_assert (ct2 (0) == 0);
static_assert (ct3 (0) == 0);
static_assert (ct4 (0) == 0);
static_assert (ct5 (0) == 0);
static_assert (ct6 (0) == 0);
static_assert (ct7 (0) == 0);
static_assert (ct8 (0) == 0);
static_assert (ct9 (0) == 0);
static_assert (ct10 (0) == 0);
static_assert (ct11 (0) == 0);
