// D4405's proof optimization level (DECISIONS.md E162-E168, E205): the
// callee premise copies (-fcontracts-prove-callee-premises) are on from
// level 2.  In D4405R0's Peter Bindels `expected` example under observe
// (p4405-shadow-callee-premises-bindels-observe.C), the call of value ()
// in the else branch is proven only through index ()'s premise copy.
// Here -fcontracts-prove-optimize=follow at -O1, level 1: no premise
// copies.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O1 -fcontracts-prove-optimize=follow" }
// { dg-additional-options "-fcontract-evaluation-semantic=observe" }

#include <contracts>
using namespace std::contracts::labels;

struct Expected {
  int d_value;
  int d_error;
  int d_index;

  int index () const post (r : r == 0 || r == 1) { return d_index; }
  bool is_error () const { return index () == 1; }
  int error () const pre<prove> (index () == 1) { return d_error; }
  int value () const pre<prove> (index () == 0) { return d_value; }	// { dg-message "the predicate of the contract assertion" }
};

Expected get12thPrime ();
void use (int);

void g ()
{
  Expected p = get12thPrime ();
  if (p.is_error ())
    {
      use (p.error ());	// { dg-bogus "cannot be proven" }
    }
  else
    {
      use (p.value ());	// { dg-error "contract assertion cannot be proven" }
    }
}
