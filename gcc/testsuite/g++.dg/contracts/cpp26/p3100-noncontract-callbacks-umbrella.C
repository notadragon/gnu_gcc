// P3100 (DECISIONS.md E178 (3)): -fsanitize-noncontract-callbacks only
// means anything when sanitizer checks are routed to the contract-violation
// handler, like -fsanitize-semantic= (p3100-sanitize-semantic-no-p3100.C).
// No warning when -fcontracts-p3850 turns -fcontracts-p3100 on.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fsanitize=null -fsanitize-noncontract-callbacks -fcontracts-p3850" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-bogus "has no effect" "" { target *-*-* } 0 }

int
f (int *p)
{
  return *p;
}
