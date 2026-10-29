// P3100: a [[noreturn]] function that returns (ub:dcl.attr.noreturn.eventually.returns)
// left at the default "assume": no check is emitted (the operation itself is not run here).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

// { dg-prune-output "does return" }
[[noreturn]] __attribute__((noinline)) void nr (int x) { if (x < 0) throw x; }
void (*volatile fp) (int) = nr;	// the caller does not know nr is noreturn
// { dg-final { scan-tree-dump-not "contract_violation" "optimized" } }
// { dg-final { scan-tree-dump-not "__builtin_trap" "optimized" } }
