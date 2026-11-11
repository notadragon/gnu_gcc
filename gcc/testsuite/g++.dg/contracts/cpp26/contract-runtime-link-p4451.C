// A per-paper contract flag must link the contracts runtime on its own:
// -fcontracts-p4451 is in g++spec.cc's list as well as in c.opt's
// LangEnabledBy list (see contract-runtime-link-p4301.C).  -std=c++23 so
// that the flag is the only thing enabling contracts.

// { dg-do run }
// { dg-additional-options "-std=c++23 -fcontracts-p4451" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int f (int x) pre (x > 0) { return x; }

int
main ()
{
  return f (1) - 1;
}
