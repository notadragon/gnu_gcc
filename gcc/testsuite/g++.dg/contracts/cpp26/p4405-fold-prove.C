// P4405 stage 2, -fcontracts-prove-fold (implied by -fcontracts-p4405): an
// evaluation with the prove semantic whose predicate folds to true is
// proven and not diagnosed, whether or not the predicate is a constant
// expression; one that folds to false is "always false"; anything else
// still "cannot be proven".  Preconditions at each call, with the
// arguments substituted (D4405), and postconditions and assertions in the
// function body.
//
// Mirror: clang/test/Contracts/p4405-fold-prove.cpp in the
// llvm_llvm-project fork; the folds below the "GCC folds" line are
// GCC-specific (Clang folds with its constant evaluator, which knows
// nothing of a parameter's value, so its mirror expects them unproven).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove" }

constexpr int N = 3;
constexpr bool positive (int v) { return v > 0; }

int f (int x)
  pre (true)				// { dg-bogus "cannot be proven|always false" }
  pre (sizeof (int) >= 1)		// { dg-bogus "cannot be proven|always false" }
  pre (x > 0)				// { dg-message "the predicate of the contract assertion" }
  post (r: N > 0)			// { dg-bogus "cannot be proven|always false" }
  post (r: r > 0)			// { dg-error "contract assertion cannot be proven" }
{
  contract_assert (1 + 1 == 2);		// { dg-bogus "cannot be proven|always false" }
  contract_assert (positive (N));	// { dg-bogus "cannot be proven|always false" }
  contract_assert (false);		// { dg-error "contract predicate is always false" }
  contract_assert (sizeof (int) == 0);	// { dg-error "contract predicate is always false" }
  contract_assert (positive (-N));	// { dg-error "contract predicate is always false" }
  contract_assert (x != 3);		// { dg-error "contract assertion cannot be proven" }
  return x;
}

// GCC folds these, though none is a constant expression.
int g (int x, unsigned u, int *p)
  pre (x == x)				// { dg-bogus "cannot be proven|always false" }
  pre (u >= 0u)				// { dg-bogus "cannot be proven|always false" }
  pre (x != x)				// { dg-message "the predicate of the contract assertion" }
  pre (u < 0u)				// { dg-message "the predicate of the contract assertion" }
  post (r: r == r)			// { dg-bogus "cannot be proven|always false" }
{
  contract_assert (p == p);		// { dg-bogus "cannot be proven|always false" }
  contract_assert (x * 0 == 0);		// { dg-bogus "cannot be proven|always false" }
  return x;
}

int use (int y, unsigned v, int *q)
{
  return f (y)				// { dg-error "contract assertion cannot be proven" }
	 + g (y, v, q);			// { dg-error "contract predicate is always false" }
}
