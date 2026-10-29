// P3100: under quick_enforce of the .access leaf alone, neither a call
// through `*fp' nor a reference bound to `*fp' is guarded: nothing is
// accessed (p3100-null-function-indirection-codegen.C has the
// .indirection guards).
// Mirror: llvm_llvm-project
// clang/test/Contracts/p3100-null-function-indirection-codegen.cpp (ACC).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

using F = int ();
int fn ();

int call (F *fp) { return (*fp) (); }
int call_cast (F *fp) { return static_cast<F &> (*fp) (); }
F &bind (F *fp) { return *fp; }

int call_named () { return (*fn) (); }
F &bind_named () { return fn; }
F &bind_ref (F &r) { return r; }
int call_plain (F *fp) { return fp (); }

// { dg-final { scan-tree-dump-not "__builtin_trap" "optimized" } }
