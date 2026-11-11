// P3100: as p3100-semantic-assume-run.C, with the checks outlined into the
// pre and post functions (-fcontract-checks-outlined, GCC only): the
// predicates are still never evaluated.
//
// Mirror: none (outlined checks are GCC-only, DECISIONS B6).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume -fcontract-checks-outlined -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int evaluations = 0;

[[gnu::noinline]] bool check (int i) { ++evaluations; return i > 0; }

[[gnu::noinline]] int f (int i)
  pre (check (i))
  post (r: check (r))
{
  return i;
}

int main ()
{
  if (f (5) != 5 || evaluations != 0)
    __builtin_abort ();
  return 0;
}
