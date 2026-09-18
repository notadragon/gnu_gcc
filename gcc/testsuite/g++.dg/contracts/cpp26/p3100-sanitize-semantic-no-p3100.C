// P3100: -fsanitize-semantic= only takes effect when routing is on.
// Without -fcontracts-p3100 it was accepted silently and did nothing,
// which reads as an ordinary sanitizer option that quietly failed.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fsanitize=null -fsanitize-semantic=null:assume" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-warning "'-fsanitize-semantic=' has no effect without '-fcontracts-p3100'" "" { target *-*-* } 0 }

int
f (int *p)
{
  return *p;
}
