// P4405 stage 3: neither a proof shadow nor anything only it uses is
// emitted.  An inline function and a static function called only from
// predicates, and not inlined into the shadow, keep no definition in the
// object file; nor does any "contract_shadow" clone; a function the real
// code calls is still emitted.  The evaluations are proven soundly, so
// -fcontracts-prove-assume (implied) makes assumptions of them, through a
// predicate-only inline function too, which is still not emitted.
//
// Mirror: clang/test/Contracts/p4405-shadow-symbols.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-contract_proven" }

[[gnu::noinline]] inline bool only_in_predicate (int x) { return x * 3 != 7; }
[[gnu::noinline]] static bool static_in_predicate (int x) { return x * 5 != 9; }
[[gnu::noinline]] inline int used_by_code (int x) { return x * 7; }
inline bool positive (int x) { return x > 0; }

int f (int x)
{
  contract_assert (only_in_predicate (x));
  contract_assert (static_in_predicate (x));
  return used_by_code (x);
}

int g (int x)
{
  if (x > 0)
    contract_assert (x > 0 || only_in_predicate (x));
  contract_assert (positive (x) || x <= 0);
  return x;
}

// { dg-final { scan-assembler-not "only_in_predicate" } }
// { dg-final { scan-assembler-not "static_in_predicate" } }
// { dg-final { scan-assembler-not "positive" } }
// { dg-final { scan-tree-dump-times "contract obligation \[0-9\]+ \\(line \[0-9\]+\\): sound" 4 "contract_shadow" } }
// { dg-final { scan-tree-dump "\\.ASSUME \\(_Z1gi\\._assume\\.1" "contract_proven" } }
// { dg-final { scan-assembler-not "contract_shadow" } }
// { dg-final { scan-assembler "used_by_code" } }
