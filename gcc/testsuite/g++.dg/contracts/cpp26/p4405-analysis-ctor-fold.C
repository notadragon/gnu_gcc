// D4405 ("The Analysis Evaluation"): a constexpr constructor call that
// initializes a variable and is folded to a constant -- possible when its
// precondition's constant evaluation is ignore -- still has its
// precondition's analysis evaluation where the call was: unguarded it
// cannot be proven, guarded it is (GCC-920: the evaluation was dropped with
// the call, so neither was diagnosed).  The constant initialization of a
// variable of static storage duration is constant evaluation, which has no
// analysis evaluation.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=ignore -O2" }

extern int g;

struct T {
  constexpr T (int y) pre (g > 0) : v (y) {}	// { dg-message "the predicate of the contract assertion" }
  int v;
};

struct U {
  constexpr U () pre (g > 0) : v (1) {}	// { dg-message "the predicate of the contract assertion" }
  int v;
};

int h () { T t (1); return t.v; }		// { dg-error "contract assertion cannot be proven" }
int hu () { U u; return u.v; }			// { dg-error "contract assertion cannot be proven" }

int
k ()
{
  if (g > 0)
    {
      T t (1);					// { dg-bogus "cannot be proven" }
      U u;					// { dg-bogus "cannot be proven" }
      return t.v + u.v;
    }
  return 0;
}

int
s ()
{
  static T t (1);				// { dg-bogus "cannot be proven" }
  static U u;					// { dg-bogus "cannot be proven" }
  return t.v + u.v;
}
