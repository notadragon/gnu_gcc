// For -fcontract-group-evaluation-semantic=foo:bogus the diagnostic names
// the invalid semantic, not the group name.
//
// Mirror: clang/test/Contracts/group-semantic-invalid-semantic.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-group-evaluation-semantic=foo:bogus" }
// { dg-error "invalid semantic .bogus. in .-fcontract-group-evaluation-semantic=. argument" "" { target *-*-* } 0 }

int f (int x) pre (x > 0) { return x; }
