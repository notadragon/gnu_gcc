// D4405's callee premise copies: g's premise copy, made at -O0 too at the
// default proof level, and the premise shadow calling it compile without
// an internal error though g's postcondition has a capture (P3098), which
// only a checked postcondition evaluates; the postcondition is not proven,
// nor is g's precondition at its call in use, at -O0 as at -O2.
//
// Mirror: clang/test/Contracts/p4405-shadow-callee-premises-post-captures.cpp
// in the llvm_llvm-project fork (CLANG-913, where the copy crashed).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -O0" }
// { dg-additional-options "-fdump-ipa-contract_shadow_clone" }

int cap (int x);
int g (int x) pre (x > 0) post [c = cap (x)] (r : r > c) { return x; }
// { dg-error "contract assertion cannot be proven" "" { target *-*-* } .-1 }
// { dg-message "the predicate of the contract assertion" "" { target *-*-* } .-2 }

int use (int y) { return g (y); }	// { dg-error "contract assertion cannot be proven" }

// { dg-final { scan-ipa-dump "Premise copy" "contract_shadow_clone" } }
// { dg-final { scan-ipa-dump "Premise shadow" "contract_shadow_clone" } }
// { dg-final { contracts-same-diagnostics "" "-O2" } }
