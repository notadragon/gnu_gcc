// D4405's proof optimization level (DECISIONS.md E162-E168, E205):
// D4405R0's Peter Bindels `expected` example
// (p4405-shadow-callee-premises-bindels.C) with
// -fcontracts-prove-optimize=0 and the proof shadow and the callee premise
// copies given explicitly at -O2.  The explicit flags win over the level
// (E164), and the shadows run at level 0, optimized as at -O0 (E208 (a)):
// nothing is inlined into them, so neither call is proven, though both are
// at the default level, 2.  A -O0 build at proof level 0 -- given, or by
// follow -- gives the same diagnostics.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-callee-premises" }
// { dg-additional-options "-fdump-tree-contract_shadow" }

#include <contracts>
using namespace std::contracts::labels;

struct Expected {
  int d_value;
  int d_error;
  int d_index;

  int index () const post (r : r == 0 || r == 1) { return d_index; }
  bool is_error () const { return index () == 1; }
  int error () const pre<prove> (index () == 1) { return d_error; }	// { dg-message "the predicate of the contract assertion" }
  int value () const pre<prove> (index () == 0) { return d_value; }	// { dg-message "the predicate of the contract assertion" }
};

Expected get12thPrime ();
void use (int);

void g ()
{
  Expected p = get12thPrime ();
  if (p.is_error ())
    {
      use (p.error ());	// { dg-error "contract assertion cannot be proven" }
    }
  else
    {
      use (p.value ());	// { dg-error "contract assertion cannot be proven" }
    }
}

// Decided in g's proof shadow, at level 0.
// { dg-final { scan-tree-dump {\(line 36\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 40\): unproven} "contract_shadow" } }

// { dg-final { contracts-same-diagnostics "" "-O0" "-O0 -fcontracts-prove-optimize=follow" } }
