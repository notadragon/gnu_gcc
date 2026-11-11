// P3100 (DECISIONS.md E178 (3)): -fsanitize-noncontract-callbacks does not
// warn when -fcontracts-p3100 ends up on, here given before it
// (p3100-noncontract-callbacks-p3100-on.C gives it after).

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3100 -fsanitize=null -fsanitize-noncontract-callbacks" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-bogus "has no effect" "" { target *-*-* } 0 }

int
f (int *p)
{
  return *p;
}
