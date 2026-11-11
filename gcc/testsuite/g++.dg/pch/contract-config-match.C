// The header is precompiled with the configuration file this TU uses, which
// makes every contract ignore.  The PCH matches and is used, and the
// precondition below is not evaluated.
//
// No Clang mirror: Clang's contracts take the TU's configuration.
// { dg-options "-std=c++26 -fcontracts -fcontract-configuration-file=${srcdir}/g++.dg/pch/contract-config-mismatch.json" }

#include "contract-config-match.H"

constexpr int v = g (-1);
