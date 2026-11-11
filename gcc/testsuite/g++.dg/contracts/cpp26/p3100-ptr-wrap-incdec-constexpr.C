// P3100: with the pointer-wrap check configured, pointer increments and
// decrements are still constant expressions: the check compares addresses
// as integers, which is not a constant expression, so constant evaluation
// evaluates the increment alone, as without the check.  (An increment past
// one past the end is accepted either way: GCC-739.)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

constexpr int f () {
  int a[4] {1, 2, 3, 4};
  int *p = a;
  ++p;
  p++;
  --p;
  int *q = p--;
  return *q * 10 + *p + (p - a);
}
static_assert (f () == 21);
