// P3100: with the placement-new alignment check configured, constant
// evaluation is unchanged: the address test is not a constant expression,
// and a placement new that stores nothing stays a constant expression.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <new>
struct S { int a, b; };
constexpr int f ()
{
  S s { 1, 2 };
  S *p = new (&s) S;
  p->a = 3;
  p->b = 4;
  return p->a + p->b;
}
static_assert (f () == 7);
