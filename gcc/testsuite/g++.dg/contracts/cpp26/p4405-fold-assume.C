// P4405: -fcontracts-prove-assume (implied by -fcontracts-p4405) is
// accepted, as is its -fno- form, and with folding as the only strategy it
// changes nothing: a proven predicate is still not evaluated and adds no
// check, and an unproven one is still diagnosed.
//
// Mirror: clang/test/Contracts/Runnable/p4405-fold-assume.cpp in the
// llvm_llvm-project fork (Clang does not fold x == x, so its mirror expects
// that one unproven).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fno-contracts-prove-assume -fcontracts-prove-assume -fcontract-evaluation-semantic=diagnose" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int evaluated, violations;
constexpr bool always () { return true; }
bool chk (int x) { ++evaluated; return x > 0; }

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

int f (int x)
  pre (always ())		// proven
  pre (chk (x))			// { dg-message "the predicate of the contract assertion" }
{
  contract_assert (x == x);	// proven
  contract_assert (false);	// { dg-warning "contract predicate is always false" }
  return x;
}

int main ()
{
  if (f (-1) != -1 || evaluated != 0 || violations != 0)	// { dg-warning "contract assertion cannot be proven" }
    __builtin_abort ();
}
