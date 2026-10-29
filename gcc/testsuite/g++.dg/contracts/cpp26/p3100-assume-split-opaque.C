// P3100: an [[assume]] predicate that cannot be evaluated freely reports
// ub:dcl.attr.assume.false.nonpure.opaque when it might only have a side
// effect through a call (an opaque call, a call to a noreturn function) or
// might trap, and .nonpure.side.effects when it has one of its own (an
// increment, an assignment, a volatile access).  "ignore" for .opaque drops
// the three opaque assumptions, and the three others keep the status-quo
// IFN_ASSUME.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-original" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:dcl.attr.assume.false.nonpure.opaque:ignore" }
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
