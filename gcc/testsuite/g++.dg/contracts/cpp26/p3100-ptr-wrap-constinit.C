// P3100: with the pointer-wrap check configured, an address constant plus a
// constant offset is not guarded, so it stays a constant initializer.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int g[4];
constinit int *gp = g + 2;
constinit int *gq = &g[3] - 1;
