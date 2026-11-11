// D4405: -fcontract-analysis-semantic=ignore is the default: no analysis.
// Given after =prove, the last one wins.  A prove selected for the runtime
// evaluations by -fcontract-evaluation-semantic still moves to the analysis
// evaluation (the stronger of the two), as p4405-prove.C shows; here the
// runtime evaluations are ignore, so nothing is diagnosed.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -fcontract-analysis-semantic=ignore" }

int f (int x)
  pre (x > 0)			// { dg-bogus "cannot be proven" }
  post (r: r > 0)		// { dg-bogus "cannot be proven" }
{
  contract_assert (x != 3);	// { dg-bogus "cannot be proven" }
  return x;
}
