// P3100: with the overlap check (ub:expr.assign.overlap) configured, an
// assignment whose operands have side effects is still a constant
// expression: the saved addresses are evaluated, and the comparison of
// addresses as integers is not.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.assign.overlap:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

constexpr int *at (int *p, int i, int &calls) { ++calls; return p + i; }
constexpr int f () {
  int a[3] {1, 2, 3};
  int k = 0;
  a[k++] = a[k + 1];
  int calls = 0;
  *at (a, 2, calls) = *at (a, 1, calls);
  return a[0] * 1000 + a[2] * 100 + k * 10 + calls;
}
static_assert (f () == 2000 + 200 + 10 + 2);
