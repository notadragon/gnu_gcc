// D4405: -fcontract-analysis-semantic=prove gives the analysis evaluation of
// every contract assertion the prove semantic: each unproven one is an
// error, a precondition at each call.  The runtime evaluations are ignore (-fcontract-evaluation-semantic).
// A predicate that folds to true is proven.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove" }

int f (int x)
  pre (x > 0)			// { dg-message "the predicate of the contract assertion" }
  post (r: r > 0)		// { dg-error "contract assertion cannot be proven" }
{
  contract_assert (x != 3);	// { dg-error "contract assertion cannot be proven" }
  contract_assert (sizeof (int) > 0);	// { dg-bogus "cannot be proven" }
  return x;
}

int g (int y)
{
  return f (y)			// { dg-error "contract assertion cannot be proven" }
	 + f (1);		// { dg-bogus "cannot be proven" }
}
