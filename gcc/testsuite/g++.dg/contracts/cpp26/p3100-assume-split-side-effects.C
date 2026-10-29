// P3100: as p3100-assume-split-opaque.C, with "ignore" for
// ub:dcl.attr.assume.false.nonpure.side.effects: the three assumptions with a
// side effect of their own are dropped, and the three opaque ones keep the
// status-quo IFN_ASSUME.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-original" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:dcl.attr.assume.false.nonpure.side.effects:ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

bool opaque (int);
[[gnu::noreturn]] bool never ();
volatile int vol;
int f_opaque (int x) { [[assume (opaque (x))]]; return x; }
int f_trap (int *p) { [[assume (*p > 0)]]; return *p; }
int f_noreturn (int x) { [[assume (x > 0 || never ())]]; return x; }
int f_incr (int x) { [[assume (++x > 1)]]; return x; }
int f_assign (int x) { [[assume ((x = 2) > 1)]]; return x; }
int f_volatile (int x) { [[assume (vol > x)]]; return x; }
// { dg-final { scan-tree-dump-times "\\.ASSUME" 3 "original" } }
