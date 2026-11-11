// P3100: with the known-bound pointer-arithmetic check configured, a constant
// offset that is in range is not guarded, so an address constant stays a
// constant initializer.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.arithmetic:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int g[4];
constinit int *gp = g + 2;
constinit int *ge = g + 4;
