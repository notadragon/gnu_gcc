// D4405 ("The Analysis Evaluation"): a precondition's analysis evaluation
// takes place at each call, in the caller, with the arguments substituted
// for the parameters -- never in the function itself, whose definition is
// not diagnosed, and not at all for a function nobody calls.  A
// postcondition's and a contract_assert's take place in the function body.
// Each is diagnosed whatever the semantic of the assertion's runtime
// evaluations, here enforce (GCC-915).  Optimized, so the proof shadow
// proves the guarded call.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=diagnose" }

int f (int x)
  pre (x > 0)			// { dg-message "the predicate of the contract assertion" }
{
  return x;
}

int unused (int x)
  pre (x > 0)			// { dg-bogus "cannot be proven" }
{
  return x;
}

int g (int y)
{
  int r = f (1);		// { dg-bogus "cannot be proven" }
  r += f (y);			// { dg-warning "contract assertion cannot be proven" }
  if (y > 5)
    r += f (y);			// { dg-bogus "cannot be proven" }
  return r;
}

int h (int x)
  post (r: r > 0)		// { dg-warning "contract assertion cannot be proven" }
{
  contract_assert (x != 3);	// { dg-warning "contract assertion cannot be proven" }
  return x;
}

struct S {
  int v;
  void m (int x) pre (x > v);	// { dg-message "the predicate of the contract assertion" }
};

void k (S &s, int x)
{
  s.m (x);			// { dg-warning "contract assertion cannot be proven" }
  if (x > s.v)
    s.m (x);			// { dg-bogus "cannot be proven" }
}
