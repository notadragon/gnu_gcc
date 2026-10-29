// D4405: -fcontract-analysis-semantic=diagnose gives the analysis evaluation
// of every contract assertion the diagnose semantic, leaving its runtime
// evaluations to -fcontract-evaluation-semantic (here ignore): a
// precondition (at each call), a postcondition and an assertion each get a
// -Wcontract-unproven warning.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=diagnose" }

int f (int x)
  pre (x > 0)			// { dg-message "the predicate of the contract assertion" }
  post (r: r > 0)		// { dg-warning "contract assertion cannot be proven" }
{
  contract_assert (x != 3);	// { dg-warning "contract assertion cannot be proven" }
  return x;
}

int use (int y)
{
  return f (y);			// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
}
