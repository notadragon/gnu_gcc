// D4405 ("The Analysis Evaluation"): the analysis evaluation is the first
// evaluation of the assertion, so its proof never relies on the
// assertion's own runtime check.  Each assertion below is checked with
// enforce, whose violation does not return, and analyzed with diagnose: had
// the analysis come after the check, the check would prove it.  A later
// assertion of the same predicate is proven by the earlier ones.  At a
// call, the analysis comes before the caller-side check
// (-fcontracts-client-check=all) as well.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3850 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=diagnose -fcontracts-client-check=all" }

void a (int x)
{
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  contract_assert (x > 0);	// { dg-bogus "cannot be proven" }
}

int b (int x)
  post (r: r > 0)		// { dg-warning "contract assertion cannot be proven" }
{
  return x;
}

int c (int x) pre (x > 0);	// { dg-message "the predicate of the contract assertion" }

int d (int y)
{
  int r = c (y);		// { dg-warning "contract assertion cannot be proven" }
  return r + c (y);		// { dg-bogus "cannot be proven" }
}
