// P4405 premises decided by this TU (T1-GCC): a prove or diagnose
// evaluation as a premise.  It is guaranteed only where its proof is sound
// in this compilation, which is known only for this TU's own evaluations:
// a later evaluation in the same function relying on an unproven one is
// conditional, on a sound one sound, on a conditional one conditional.  A
// sound proof in an inlined callee is no premise for the caller's shadow --
// its assumption is decided after the shadows -- but whatever made that
// proof sound is code the inlining brings into the caller too: here hp's
// quick_enforce trap, which proves the caller's evaluation soundly (hp is
// inlined into the real caller as well).
//
// Mirror: clang/test/Contracts/p4405-premise-prove.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-prove.json" }

int sink (int);

int g_unproven (int x)
{
  contract_assert (x > 0);	// { dg-error "contract assertion cannot be proven" }
  contract_assert (x > 0);	// conditional
  return sink (x);
}

int g_sound (int x)
{
  contract_assert (x > 0);	// quick_enforce
  contract_assert (x > 0);	// prove: sound
  contract_assert (x > 0);	// sound
  return sink (x);
}

int g_conditional (int x)
{
  contract_assert (x > 0);	// observe
  contract_assert (x > 0);	// prove: conditional
  contract_assert (x > 0);	// conditional
  return sink (x);
}

[[gnu::always_inline]] inline int hp (int x)
{
  contract_assert (x > 0);	// quick_enforce
  contract_assert (x > 0);	// prove: sound
  return sink (x);
}

int g_callee (int x)
{
  hp (x);
  contract_assert (x > 0);	// sound
  return 0;
}

// { dg-final { scan-tree-dump {\(line 22\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 23\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 30\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 31\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 38\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 39\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 46\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 53\): sound} "contract_shadow" } }
