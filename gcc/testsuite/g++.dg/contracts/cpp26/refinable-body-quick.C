// A quick_enforce check traps inline: __builtin_trap, which is noreturn and
// cold, with no wrapper function around it (GCC-103).  So the code after the
// check relies on its predicate -- in the function itself (hi), and in a
// caller that inlines it, also an always_inline function (use_a) -- and the
// trap goes to the function's cold part (fs.cold).  That another
// definition of an inline function may evaluate the assertion differently
// is handled by the refinable-body mark instead: the caller of the inline
// function fi keeps its comparison, the callers of the static and the
// external function, the only definitions of their symbols, fold it.
//
// Mirror: clang/test/Contracts/refinable-body-enforce.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=quick_enforce -O2 -fno-ipa-sra -fdump-tree-gimple -fdump-tree-optimized" }

[[gnu::noinline]] inline int fi (int x) { contract_assert (x > 0); return x; }
[[gnu::noinline]] static int fs (int x) { contract_assert (x > 0); return x; }
[[gnu::noinline]] int fe (int x) { contract_assert (x > 0); return x; }

[[gnu::noinline]] inline int hi (int x)
{
  contract_assert (x > 0);
  return x > 0 ? 1234 : 5678;
}

[[gnu::always_inline]] inline int ai (int x) { contract_assert (x > 0); return x; }

int use_i (int y) { return fi (y) > 0; }
int use_s (int y) { return fs (y) > 0; }
int use_e (int y) { return fe (y) > 0; }
int use_h (int y) { return hi (y); }
int use_a (int y) { return ai (y) > 0 ? 4321 : 8765; }

// { dg-final { scan-tree-dump-times "__builtin_trap \\(\\);" 5 "gimple" } }
// { dg-final { scan-tree-dump-not "quick_enforce_wrapper" "gimple" } }
// { dg-final { scan-assembler-not "quick_enforce_wrapper" } }
// { dg-final { scan-tree-dump-times "= _\[0-9\]+ > 0;" 1 "optimized" } }
// { dg-final { scan-tree-dump "return 1234;" "optimized" } }
// { dg-final { scan-tree-dump-not "5678" "optimized" } }
// { dg-final { scan-tree-dump "return 4321;" "optimized" } }
// { dg-final { scan-tree-dump-not "8765" "optimized" } }
// { dg-final { scan-assembler "_ZL2fsi\\.cold" { target { x86_64-*-* i?86-*-* } } } }
