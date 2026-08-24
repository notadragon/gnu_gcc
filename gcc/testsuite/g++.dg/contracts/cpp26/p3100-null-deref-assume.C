// P3100: null-pointer dereference under the default "assume" semantic must be
// byte-identical to no P3100 -- no null instrumentation is emitted at all.
// (No configuration file: the builtin default resolves implicit assertions to
// "assume".)  Compiled at -O0 so the optimizer's own erroneous-path isolation
// cannot introduce an unrelated trap and confound the check.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O0 -fdump-tree-optimized -Wno-return-type" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int load_it (int *p) { return *p; }
void store_it (int *p, int v) { *p = v; }

// No trap and no UBSAN internal call: assume leaves the dereference untouched.
// { dg-final { scan-tree-dump-not "__builtin_trap" "optimized" } }
// { dg-final { scan-tree-dump-not "UBSAN" "optimized" } }
