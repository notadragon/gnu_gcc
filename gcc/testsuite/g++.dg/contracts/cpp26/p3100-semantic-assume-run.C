// P3100: the predicate of an evaluation with the assume semantic is never
// evaluated, though it is an optimizer assumption: one with side effects
// (here a call counting its evaluations) is outlined into a function that
// is never called, as for [[assume]].  Preconditions, postconditions and
// contract_assert, at -O2 (p3100-assume-allowed.C is the unoptimized case;
// p3100-semantic-assume-outlined.C has the checks outlined,
// p3100-semantic-assume-caller.C a caller-side evaluation).  The predicates
// hold, so the assumptions are true.
//
// Clang: CLANG-111 in the llvm_llvm-project fork (its assume semantic
// lowers to ignore).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int evaluations = 0;

[[gnu::noinline]] bool check (int i) { ++evaluations; return i > 0; }

[[gnu::noinline]] int f (int i)
  pre (check (i))
  post (r: check (r))
{
  contract_assert (check (i));
  return i;
}

struct S
{
  int v = 1;
  [[gnu::noinline]] int m (int i) pre (check (i + v)) { return i + v; }
};

int main ()
{
  int r = f (5);
  S s;
  r += s.m (2);
  if (r != 8 || evaluations != 0)
    __builtin_abort ();
  return 0;
}
