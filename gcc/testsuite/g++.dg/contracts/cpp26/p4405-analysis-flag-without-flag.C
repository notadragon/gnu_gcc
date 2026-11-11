// D4405: -fcontract-analysis-semantic=prove without -fcontracts-p4405 is not
// allowed; the compiler says so and the analysis evaluation is ignore, so
// nothing is diagnosed.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove" }
// { dg-warning "'-fcontract-analysis-semantic=prove' requires '-fcontracts-p4405'; 'ignore' is used instead" "" { target *-*-* } 0 }

int f (int x)
  pre (x > 0)			// { dg-bogus "cannot be proven" }
{
  contract_assert (x != 3);	// { dg-bogus "cannot be proven" }
  return x;
}
