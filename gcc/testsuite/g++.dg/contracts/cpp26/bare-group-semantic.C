// -fcontract-group-evaluation-semantic= takes group:semantic; a bare semantic
// is rejected.
//
// Mirror: clang/test/Contracts/bare-group-semantic.cpp in the
// llvm_llvm-project fork.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce -fcontract-group-evaluation-semantic=observe" }
// { dg-error "invalid .-fcontract-group-evaluation-semantic=. argument .observe.; expected .group:semantic." "" { target *-*-* } 0 }

void f () { contract_assert (false); }
