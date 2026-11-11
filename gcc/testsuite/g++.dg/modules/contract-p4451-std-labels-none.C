// "import std;" exports P3400's std::contracts::labels only when the std
// module is built with -fcontracts-p3400 (std-module-contract-labels.C),
// and nothing of it with contracts alone: no labels namespace.  As
// <contracts> does (p4451-diag-no-flag.C).
//
// Mirror: clang/test/Contracts/libcxx-labels-gating.cpp in the
// llvm_llvm-project fork.

// { dg-additional-options "-fmodules --compile-std-module -fcontracts" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi std }
// { dg-module-cmi std.compat }
// { dg-module-cmi <bits/stdc++.h> }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

import std;

auto e = std::contracts::labels::empty_label;	// { dg-error "labels" }
void f (int x) pre<terminating> (x > 0);	// { dg-error "requires .-fcontracts-p4451." }
