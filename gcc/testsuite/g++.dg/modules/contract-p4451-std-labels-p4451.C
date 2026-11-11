// "import std;" built with -fcontracts-p4451 alone exports
// std::contracts::labels::terminating and none of P3400's library: no other
// label, no assertion_control_object concept, and no contract_control
// directive, so `terminating' is not found unqualified.
//
// Mirror: clang/test/Contracts/libcxx-labels-gating.cpp and
// std-module-contracts-labels.cpp in the llvm_llvm-project fork.

// { dg-additional-options "-fmodules --compile-std-module -fcontracts-p4451" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi std }
// { dg-module-cmi std.compat }
// { dg-module-cmi <bits/stdc++.h> }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

import std;

namespace L = std::contracts::labels;
constexpr auto &t = L::terminating;
auto e = L::empty_label;	// { dg-error "empty_label" }
auto r = L::review;	// { dg-error "review" }
using L::assertion_control_object;	// { dg-error "assertion_control_object" }
auto u = terminating;	// { dg-error "not declared" }

