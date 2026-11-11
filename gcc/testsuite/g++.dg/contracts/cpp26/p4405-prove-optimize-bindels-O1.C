// D4405's proof optimization level (DECISIONS.md E162-E168, E206): D4405R0's
// Peter Bindels `expected` example (p4405-shadow-callee-premises-bindels.C)
// with observe in a -O1 build, at the default proof level, 2.  The shadow
// is optimized at level 2 and the callees at -O1; the inliner does not
// inline across those levels after the early optimizations, so the shadow
// calls level copies of index (), is_error (), error () and value () at
// level 2 instead, and both calls are proven (the call of value ()
// through index ()'s premise copy, conditionally).
//
// Mirror: clang/test/Contracts/p4405-prove-optimize-bindels.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O1" }
// { dg-additional-options "-fcontract-evaluation-semantic=observe -fdump-ipa-contract_shadow_clone" }

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

// { dg-final { scan-ipa-dump "Level copy .*index.*contract_level_" "contract_shadow_clone" } }
// { dg-final { scan-ipa-dump "Level copy .*value.*contract_level_" "contract_shadow_clone" } }
