// D4405, step 2: a configured runtime prove beats a configured analysis
// diagnose -- the analysis evaluation takes the stronger, prove.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -fcontract-analysis-semantic=diagnose" }

int f (int x)
  pre (x > 0)			// { dg-message "the predicate of the contract assertion" }
  post (r: r > 0)		// { dg-error "contract assertion cannot be proven" }
{
  contract_assert (x != 3);	// { dg-error "contract assertion cannot be proven" }
  return x;
}

int use (int y)
{
  return f (y);			// { dg-error "contract assertion cannot be proven" }
}
