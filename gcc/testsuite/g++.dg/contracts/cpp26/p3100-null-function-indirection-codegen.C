// P3100: under quick_enforce, a call through `*fp' (GCC-800), also through
// `static_cast<F &>', and a reference bound to `*fp' get one null guard
// each, under the .indirection leaf; a function named directly
// (`(*fn) ()', `F &r = fn'), a reference to a function and a plain call
// through the pointer, `fp ()', which names no indirection, get none.
// -codegen-access.C: .access alone guards none.
// Mirror: llvm_llvm-project
// clang/test/Contracts/p3100-null-function-indirection-codegen.cpp.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:quick_enforce" }
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

// { dg-final { scan-tree-dump-times "__builtin_trap" 3 "optimized" } }
