// P3595 dynamic: a deeply nested-namespace C++ selector name (a::b::c::sel)
// mangles to the correct Itanium symbol.  Extends p3595-dynamic-cxxname.C, which
// used only a single-level namespace, to the multi-component mangler branch.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -O0" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-mangle-nested.json" }

#include <contracts>

namespace a { namespace b { namespace c {
  std::contracts::evaluation_semantic sel();
}}}

void f(int x) pre(x > 0) { }

// The generated call must reference the mangled nested symbol at a symbol
// boundary (not as a substring of some other re-mangled name).
// { dg-final { scan-assembler "\[ \t\]_ZN1a1b1c3selEv\[^A-Za-z0-9_\]" } }
