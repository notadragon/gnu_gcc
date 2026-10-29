// A function that calls a contract wrapper -- here one checking f's
// postcondition on the caller's side, -fcontracts-client-check=all -- holds
// that evaluation, which another translation unit's definition of the
// function makes differently or not at all.  So it has a refinable body
// even when the wrapper is not inlined into it (-fno-inline): the other unit
// compiles g with an enforced client check, so the wrapper's and g's return
// value is positive, but the linker keeps this unit's g, which checks
// nothing on the caller's side, and this unit's f ignores its
// postcondition.  use_g (-5) must be 0.
// { dg-lto-do run }
// { dg-lto-options { { -O2 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -fno-inline -fno-ipa-sra } { -O2 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -fno-inline -fno-ipa-sra -flto } } }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int f (int x) post (r: r > 0);

int f (int x) { return x; }

[[gnu::noinline]] inline int
g (int x)
{
  return f (x);
}

int (*volatile gp) (int) = g;

int use_g (int);

int
main ()
{
  if (use_g (-5) != 0)
    __builtin_abort ();
}
