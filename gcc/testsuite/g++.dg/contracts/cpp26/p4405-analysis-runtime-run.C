// D4405 ("The Analysis Evaluation"): the analysis evaluation is separate
// from the runtime evaluations, which keep the configured semantic -- here
// observe, under -fcontract-analysis-semantic=diagnose.  Each assertion is
// diagnosed (its analysis evaluation never evaluates the predicate) and
// still checked at run time once: the precondition in f, the postcondition
// and the contract_assert in g.  A call through a pointer, which has no
// analysis evaluation, still reaches f's check.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=diagnose" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int evaluated, violations;
[[gnu::noipa]] bool chk (int x) { ++evaluated; return x > 0; }

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

[[gnu::noinline]] int f (int x) pre (chk (x)) { return x; }	// { dg-message "the predicate of the contract assertion" }

[[gnu::noinline]] int g (int x)
  post (r: chk (r))		// { dg-warning "contract assertion cannot be proven" }
{
  contract_assert (chk (x));	// { dg-warning "contract assertion cannot be proven" }
  return x;
}

int (*volatile fptr) (int) = f;

int main ()
{
  f (-1);			// { dg-warning "contract assertion cannot be proven" }
  if (evaluated != 1 || violations != 1)
    __builtin_abort ();
  g (-1);
  if (evaluated != 3 || violations != 3)
    __builtin_abort ();
  fptr (-1);
  if (evaluated != 4 || violations != 4)
    __builtin_abort ();
}
