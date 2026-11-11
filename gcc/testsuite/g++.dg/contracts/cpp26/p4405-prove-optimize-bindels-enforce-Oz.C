// D4405's proof optimization level (DECISIONS.md E162-E168, E206): D4405R0's
// Peter Bindels `expected` example (p4405-shadow-callee-premises-bindels.C)
// with enforce in a -Oz build, at the default proof level, 2: both calls
// are proven, through level copies of the -Oz callees.
//
// Mirror: clang/test/Contracts/p4405-prove-optimize-bindels.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -Oz" }
// { dg-additional-options "-fcontract-evaluation-semantic=enforce" }

#include <contracts>
using namespace std::contracts::labels;

struct Expected {
  int d_value;
  int d_error;
  int d_index;

  int index () const post (r : r == 0 || r == 1) { return d_index; }
  bool is_error () const { return index () == 1; }
  int error () const pre<prove> (index () == 1) { return d_error; }
  int value () const pre<prove> (index () == 0) { return d_value; }
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
      use (p.value ());	// { dg-bogus "cannot be proven" }
    }
}
