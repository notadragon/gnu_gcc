// P3100 (DECISIONS.md E178 (3)): -fsanitize-noncontract-callbacks only
// means anything when sanitizer checks are routed to the contract-violation
// handler, like -fsanitize-semantic= (p3100-sanitize-semantic-no-p3100.C).
// Without -fcontracts-p3100 it warns that it has no effect.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fsanitize=null -fsanitize-noncontract-callbacks" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-warning "'-fsanitize-noncontract-callbacks' has no effect without '-fcontracts-p3100'" "" { target *-*-* } 0 }

int
f (int *p)
{
  return *p;
}
