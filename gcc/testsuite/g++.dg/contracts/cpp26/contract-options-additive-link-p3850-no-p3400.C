// DECISIONS.md E175 (GCC-961): the per-paper contract flags are additive,
// in the driver as in the front end, so a later -fno- of one per-paper flag
// leaves contracts on, and the contracts runtime linked, while another is
// on.  -std=c++23 so that the flags are the only thing enabling contracts
// (see contract-runtime-link-p4301.C).

// { dg-do run }
// { dg-additional-options "-std=c++23 -fcontracts-p3850 -fno-contracts-p3400" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int f (int x) pre (x > 0) { return x; }

int
main ()
{
  return f (1) - 1;
}
