// P4405: with -fcontract-evaluation-semantic=diagnose, the run-time
// evaluations' diagnose goes to the analysis evaluations, which warn: the
// precondition's at each call, and the assertion's in the body (D4405).
// Constant evaluation is never analyzed: the diagnose it reaches is observe
// (DECISIONS.md E142), so each false predicate is a warning there too, and
// the constant expression does not fail; the static_assert's call of f is
// manifestly constant-evaluated, so its precondition is not analyzed there,
// only at the run-time call in use.
//
// Mirror: clang/test/Contracts/p4405-constexpr-diagnose.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose" }

constexpr int f (int x)
  pre (x > 0)		// { dg-warning "contract predicate is false in constant expression" }
// { dg-message "the predicate of the contract assertion" "" { target *-*-* } .-1 }
{
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
// { dg-warning "contract predicate is false in constant expression" "" { target *-*-* } .-1 }
  return x;
}

static_assert (f (-1) == -1);	// { dg-bogus "always false|cannot be proven" }

int use (int x) { return f (x); }	// { dg-warning "contract assertion cannot be proven" }
