// D4405, step 2 of "Adjusting to the Allowed Semantics": a prove or diagnose
// configured for a runtime evaluation moves to the analysis evaluation,
// which takes the stronger of it and its own configured semantic (ignore <
// diagnose < prove); the runtime evaluation becomes ignore.  Here the
// runtime evaluations are configured diagnose and the analysis evaluation
// prove, so each unproven assertion is an error, not a warning.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -fcontract-analysis-semantic=prove" }

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
