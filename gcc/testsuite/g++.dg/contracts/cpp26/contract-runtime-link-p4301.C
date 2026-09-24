// A per-paper contract flag must link the contracts runtime on its own.
//
// -fcontracts-p4301 turns contracts on in the front end (c.opt's LangEnabledBy
// list), but it was missing from the case list in g++spec.cc that sets
// need_experimental, so the driver linked neither libstdc++exp nor
// libcontracts and the program failed with an undefined reference to
// __cxa_contract_violation_pre_enforce_pf.
//
// -std=c++23 is essential: at C++26 and later the -std= case in that same list
// sets need_experimental anyway, which hides the defect entirely.  The flag has
// to be the only thing enabling contracts.

// { dg-do run }
// { dg-additional-options "-std=c++23 -fcontracts-p4301" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int f (int x) pre (x > 0) { return x; }

int
main ()
{
  return f (1) - 1;
}
