// P3100: with the pointer-wrap check configured, pointer arithmetic is still a
// constant expression: the check compares addresses as integers, which is not
// a constant expression, so constant evaluation evaluates the arithmetic
// alone.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

constexpr int f (int i) { int a[4] {1, 2, 3, 4}; int *p = a; p = p + i; return *p; }
static_assert (f (2) == 3);
constexpr int g (int i) { int a[4] {}; int *p = a + 4; p -= i; return p - a; }
static_assert (g (4) == 0);
