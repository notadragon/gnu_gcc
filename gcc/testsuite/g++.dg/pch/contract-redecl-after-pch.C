// After a precompiled header, a redeclaration of a function the header
// declared with contract assertions is checked against them (GCC-647):
// repeating the same contracts is valid, and different ones are
// "mismatched contract condition".  The table of contract specifiers used
// to be hashed on addresses, which writing the PCH changes, so the header's
// were found or lost depending on the build of the .gch; there are twelve
// of each here so that a regression cannot pass by chance.
//
// Mirror: clang/test/Contracts/pch-contracts-constexpr.cpp in the
// llvm_llvm-project fork.
// { dg-options "-std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce" }
// { dg-prune-output "previous contract here" }

#include "contract-redecl-after-pch.H"

int nf0 (int x) pre (x > 0);	// { dg-bogus "adds contracts" }
int mf0 (int x) pre (x > 1);	// { dg-error "mismatched contract condition" }
int nf1 (int x) pre (x > 1);	// { dg-bogus "adds contracts" }
int mf1 (int x) pre (x > 2);	// { dg-error "mismatched contract condition" }
int nf2 (int x) pre (x > 2);	// { dg-bogus "adds contracts" }
int mf2 (int x) pre (x > 3);	// { dg-error "mismatched contract condition" }
int nf3 (int x) pre (x > 3);	// { dg-bogus "adds contracts" }
int mf3 (int x) pre (x > 4);	// { dg-error "mismatched contract condition" }
int nf4 (int x) pre (x > 4);	// { dg-bogus "adds contracts" }
int mf4 (int x) pre (x > 5);	// { dg-error "mismatched contract condition" }
int nf5 (int x) pre (x > 5);	// { dg-bogus "adds contracts" }
int mf5 (int x) pre (x > 6);	// { dg-error "mismatched contract condition" }
int nf6 (int x) pre (x > 6);	// { dg-bogus "adds contracts" }
int mf6 (int x) pre (x > 7);	// { dg-error "mismatched contract condition" }
int nf7 (int x) pre (x > 7);	// { dg-bogus "adds contracts" }
int mf7 (int x) pre (x > 8);	// { dg-error "mismatched contract condition" }
int nf8 (int x) pre (x > 8);	// { dg-bogus "adds contracts" }
int mf8 (int x) pre (x > 9);	// { dg-error "mismatched contract condition" }
int nf9 (int x) pre (x > 9);	// { dg-bogus "adds contracts" }
int mf9 (int x) pre (x > 10);	// { dg-error "mismatched contract condition" }
int nf10 (int x) pre (x > 10);	// { dg-bogus "adds contracts" }
int mf10 (int x) pre (x > 11);	// { dg-error "mismatched contract condition" }
int nf11 (int x) pre (x > 11);	// { dg-bogus "adds contracts" }
int mf11 (int x) pre (x > 12);	// { dg-error "mismatched contract condition" }
