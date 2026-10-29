// GCC-938: a postcondition that odr-uses a value parameter that is not
// const is ill-formed ([dcl.contract.func]); once that is diagnosed, the
// postcondition is erroneous, and its D4405 analysis evaluation is not made
// -- no second error for the same assertion -- in a function or in a
// function template's specialization.  (Any error stops the compilation
// before the proof shadows run, so a TU with one diagnoses every other
// obligation as unproven: there is none here.)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2" }

int f (int x)			// { dg-message "parameter declared here" }
  post (r: r > x)		// { dg-error "a value parameter used in a postcondition must be const" }
{				// { dg-bogus "cannot be proven" "" { target *-*-* } .-1 }
  return x + 1;
}

template <typename T>
T h (T x)			// { dg-error "value parameter 'x' used in a postcondition must be const" }
				// { dg-message "parameter declared here" "" { target *-*-* } .-1 }
				// { dg-message "previous declaration here" "" { target *-*-* } .-2 }
  post (r: r > x)		// { dg-error "a value parameter used in a postcondition must be const" }
				// { dg-bogus "cannot be proven" "" { target *-*-* } .-1 }
{
  return x + 1;
}

int call () { return h (1); }
