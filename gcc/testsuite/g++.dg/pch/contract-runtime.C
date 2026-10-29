// Contract assertions declared in a precompiled header are checked as if the
// header were read as source: pch.exp compares the assembly of this TU built
// with the PCH against the same TU built from the header source, here for a
// function's precondition and postcondition, an inline function's
// contract_assert, a member function's and a lambda's precondition, and
// function templates instantiated after the PCH (GCC-646; eight, so that a
// regression cannot pass by chance), all under quick_enforce.  (EDG shape:
// pch/runtime.)
//
// Mirror: clang/test/Contracts/Runnable/pch-contracts-runtime.cpp in the
// llvm_llvm-project fork.
// { dg-options "-std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce" }

#include "contract-runtime.H"

int main () { f (2); g (0); M ().m (1); lam (1); return 0; }
int v1 () { return f (0); }		// precondition
int v2 () { return f (1); }		// postcondition
void v3 () { g (3); }			// contract_assert
int v5 () { return M ().m (0); }	// member's precondition
int v6 () { return lam (0); }		// lambda's precondition
int v7 () { return t0 (1) + t1 (2) + t2 (3) + t3 (4) + t4 (5) + t5 (6)
		  + t6 (7) + t7 (8); }	// templates
