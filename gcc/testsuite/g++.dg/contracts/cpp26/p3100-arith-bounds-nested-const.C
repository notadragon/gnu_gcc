// P3100: with the known-bound checks (ub:expr.add.out.of.bounds.known) and
// the past-the-end indirection check (ub:expr.unary.dereference.invalid.known)
// configured, nested arithmetic, an indirection through it and a subscript of
// it at a constant position within the array emit no check.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-gimple" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known:quick_enforce,ub:expr.unary.dereference.invalid.known:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int a[4];
int *nested () { return (a + 1) + 3; }
int deref () { return *(a + 1 + 2); }
int direct () { return *(a + 3); }
int sub () { return (a + 1)[2]; }
int *subaddr () { return &(a + 1)[2]; }
// { dg-final { scan-tree-dump-not "__builtin_trap \\(\\);" "gimple" } }
