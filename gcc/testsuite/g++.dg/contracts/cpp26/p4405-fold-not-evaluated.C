// P4405 stage 2: folding never evaluates the predicate at run time and
// never proves one whose side effects it would have to drop: a predicate
// that is true whatever its operand's side effects is still not proven,
// and none of them run.  A proven predicate emits nothing either.  diagnose
// is used so that the program links and runs.
//
// Mirror: clang/test/Contracts/Runnable/p4405-fold-not-evaluated.cpp in the
// llvm_llvm-project fork (Clang does not fold x == x; its mirror proves x > 0
// || true instead).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int evaluated, violations;
bool touch () { ++evaluated; return false; }

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

int f (int x)
  pre (touch () || true)	// { dg-message "the predicate of the contract assertion" }
  pre (x == x)			// proven
  post (r: (touch (), true))	// { dg-warning "contract assertion cannot be proven" }
{
  contract_assert (touch () && false);	// { dg-warning "contract assertion cannot be proven" }
  contract_assert (false);	// { dg-warning "contract predicate is always false" }
  contract_assert (true);	// proven
  return x;
}

int main ()
{
  f (1);			// { dg-warning "contract assertion cannot be proven" }
  if (evaluated != 0 || violations != 0)
    __builtin_abort ();
}
