// The group name in -fcontract-group-evaluation-semantic=group:semantic is
// validated: a malformed name ("a..b", ".a") is rejected instead of being
// accepted silently and never matching.
//
// Mirror: clang/test/Contracts/group-semantic-group-name-validated.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-group-evaluation-semantic=a..b:observe -fcontract-group-evaluation-semantic=.a:observe" }
// { dg-error "invalid group name .a\\.\\.b." "" { target *-*-* } 0 }
// { dg-error "invalid group name .\\.a." "" { target *-*-* } 0 }

int f (int x) pre (x > 0) { return x; }
