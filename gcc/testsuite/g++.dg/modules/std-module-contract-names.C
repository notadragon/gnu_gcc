// "import std;" can name every public std::contracts name: libstdc++'s std.cc
// exports evaluation_semantic_set, violation_handled, is_nonthrowing and the
// handle_*_contract_violation functions.  It lives here rather than in
// g++.dg/contracts/cpp26 because --compile-std-module needs this directory's
// dg-module-cmi harness.
//
// Mirror: clang/test/Contracts/std-module-contracts-names.cpp in the
// llvm_llvm-project fork.

// { dg-additional-options "-fmodules --compile-std-module -fcontracts-p3850" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi std }
// { dg-module-cmi std.compat }
// { dg-module-cmi <bits/stdc++.h> }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

import std;

std::contracts::evaluation_semantic e = std::contracts::evaluation_semantic::observe;
std::contracts::evaluation_semantic_set s;
std::contracts::violation_handled h = std::contracts::violation_handled::handled;
bool b = std::contracts::is_nonthrowing (e);

void
f ()
{
  std::contracts::handle_observed_contract_violation ("x");
  std::contracts::handle_enforced_contract_violation ("x");
  std::contracts::handle_quick_enforced_contract_violation ("x");
}
