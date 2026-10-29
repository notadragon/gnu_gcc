// P4405 -fcontracts-prove-assume: the assumption made of a soundly proven
// evaluation is lowered as [[assume]]'s, so its predicate is never
// evaluated.  positive is declared pure but counts its calls, so that a
// call from the outlined assumption would show: the shadow proves the
// contract_assert from the test before it (the two calls of a pure function
// are one), soundly, since positive is pure, terminating and does not
// throw.  Only the test's own call happens.
//
// Mirror: clang/test/Contracts/Runnable/p4405-prove-assume-run.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove" }
// { dg-additional-options "-fdump-tree-contract_proven" }

int calls;

[[gnu::pure, gnu::noinline]] bool positive (int x) { ++calls; return x > 0; }

[[gnu::noinline]] int f (int x)
{
  if (positive (x))
    {
      contract_assert (positive (x));	// { dg-bogus "cannot be proven|always false" }
      return x;
    }
  return 0;
}

int main ()
{
  if (f (5) != 5 || calls != 1)
    __builtin_abort ();
  return 0;
}

// { dg-final { scan-tree-dump "contract obligation \[0-9\]+: assumed" "contract_proven" } }
