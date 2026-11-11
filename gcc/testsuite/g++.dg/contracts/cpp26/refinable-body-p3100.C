// P3100 implicit contract assertions never make a function a refinable body
// (refinable-body design, decision 4): going past one is undefined behavior
// in every translation unit, whatever semantic each chooses for it, so the
// facts a definition gives are sound for every other.  The inline function
// p_div holds an implicit division check, trapping (quick_enforce) in this
// translation unit, so it cannot throw: its caller's landing pad goes, and
// no function here has a refinable body.
//
// Mirror: clang/test/Contracts/refinable-body-p3100.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O2 -fno-ipa-sra -fdump-tree-gimple -fdump-tree-optimized -fdump-ipa-inline" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int g (int);

[[gnu::noinline]] inline int p_div (int a, int b) { return a / b; }

int use_p (int a, int b) { try { return p_div (a, b); } catch (...) { return g (1); } }

// { dg-final { scan-tree-dump "__builtin_trap \\(\\);" "gimple" } }
// { dg-final { scan-tree-dump-not "g \\(1\\)" "optimized" } }
// { dg-final { scan-ipa-dump-not "refinable body" "inline" } }
