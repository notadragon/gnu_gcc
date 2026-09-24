// P3100: codegen for a configurable [[assume]] across semantics.  The optimizer
// "assume" hint (lowered to `if (!cond) __builtin_unreachable ()`) is emitted
// only where cond is guaranteed to hold: the default assume, and after a passing
// enforcing check.  ignore emits nothing; observe emits the check but no hint.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -fdump-tree-gimple -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-assume-attr-codegen.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int def_f (int x) { [[assume (x > 0)]]; return x; }                   // assume: hint only
namespace ign_ns { int f (int x) { [[assume (x > 0)]]; return x; } }  // ignore: nothing
namespace obs_ns { int f (int x) { [[assume (x > 0)]]; return x; } }  // observe: check, no hint
namespace enf_ns { int f (int x) { [[assume (x > 0)]]; return x; } }  // enforce: check + hint

// Hint present for assume (def_f) and enforce (enf_ns) only -> 2 occurrences;
// ignore and observe add none.
// { dg-final { scan-tree-dump-times "__builtin_unreachable" 2 "gimple" } }
// observe and enforce each call their CAK_IMPLICIT entry point once.
// { dg-final { scan-tree-dump-times "implicit_observe" 1 "gimple" } }
// { dg-final { scan-tree-dump-times "implicit_enforce" 1 "gimple" } }
