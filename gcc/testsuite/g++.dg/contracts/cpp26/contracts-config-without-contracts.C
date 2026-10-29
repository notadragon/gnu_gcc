// With contracts off (C++23, no -fcontracts) the contract configuration
// options are not used, so they are not parsed either: say so rather than
// accept them silently.  Clang warns that the argument is unused.
//
// Mirror (C, no -fcontracts-p4299): gcc.dg/contracts/contracts-config-without-p4299.c
// { dg-do compile }
// { dg-options "-std=c++23 -fcontract-group-evaluation-semantic=a:bogus" }
// { dg-warning "contract configuration options have no effect without .-fcontracts." "" { target *-*-* } 0 }

int x;
