// P3100: every implicit check resolved to assume emits no check at all.
// The builtin default resolves implicit assertions to assume, so with no
// configuration file each site below -- division and remainder by zero and
// overflow, shift, a known-bound subscript, an enum conversion out of
// range, a float-to-int conversion, flowing off the end of a non-void
// function, signed overflow, a null dereference and a bool load -- must
// compile to no violation call, no trap and no sanitizer call (the
// per-check *-assume tests cover a few of these one at a time; under
// observe this file has fourteen violation calls).  Compiled at -O0, so
// nothing here is the optimizer's doing.
//
// Mirror: clang/test/Contracts/p3100-assume-codegen-all.cpp in the
// llvm_llvm-project fork; owed to EDG, recorded in its open-issues/README.md.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O0 -w -fdump-tree-optimized" }

enum Pl { P0, P1 };
int g[4];
int divz (int a, int b) { return a / b; }
int modz (int a, int b) { return a % b; }
int shl (int a, int b) { return a << b; }
int bnd (int i) { return g[i]; }
Pl ecast (int i) { return static_cast<Pl> (i); }
int fpint (double d) { return (int) d; }
int flow (int x) { if (x) return 1; }
int ovf (int a, int b) { return a + b; }
int nul (int *p) { return *p; }
bool ldb (bool *p) { return *p; }

// { dg-final { scan-tree-dump-not "contract_violation" "optimized" } }
// { dg-final { scan-tree-dump-not "__builtin_trap" "optimized" } }
// { dg-final { scan-tree-dump-not "__ubsan" "optimized" } }
