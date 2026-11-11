// A P4298 noexcept semantic requested without -fcontracts-p4298 is
// downgraded to its plain counterpart, and the compiler says so.
//
// Clang: clang/test/Contracts/p4298-semantic-without-flag.cpp.
//
//
//
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=noexcept_enforce" }
// { dg-warning "requires .-fcontracts-p4298.; .enforce. is used instead" "" { target *-*-* } 0 }

int f (int x) pre (x > 0) { return x; }
