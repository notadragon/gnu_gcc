// P3100: the [[noreturn]] check is made once per function, at its normal
// exits only: "quick_enforce" adds one terminating call to each noreturn function
// definition, whether it returns by flowing off its end or by a return
// statement, and none to other functions.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-gimple" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:dcl.attr.noreturn.eventually.returns:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-prune-output "does return" }

[[noreturn]] void a (int x) { if (x) throw x; }
[[noreturn]] void b (int x) { if (x) throw x; throw 0; }
void c (int x) { if (x) throw x; }
[[noreturn]] void d (int);
void e () { d (1); }
[[noreturn]] void g (int x) { if (x) return; throw x; }	// { dg-warning "has a .return. statement" }
// { dg-final { scan-tree-dump-times "__builtin_trap \\(\\);" 3 "gimple" } }
