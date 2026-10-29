// An empty -fcontract-group-evaluation-semantic= is rejected.
//
// Mirror: clang/test/Contracts/group-semantic-empty.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-group-evaluation-semantic=" }
// { dg-error "missing argument to .-fcontract-group-evaluation-semantic=." "" { target *-*-* } 0 }

int f (int x) pre (x > 0) { return x; }
