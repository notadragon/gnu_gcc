// P4405 -fcontracts-prove-assume under -flto: the proof shadows are decided
// at the compile step, and the guards of the assumptions made of the sound
// proofs are folded there too, before anything is streamed, so the
// link-time optimizers see the assumption, which gives x the range [1, +INF]
// (as quick_enforce's inline trap does), but no guard.
//
// Mirror: none (the compile-step decision is GCC's).
// { dg-do link { target c++26 } }
// { dg-require-effective-target lto }
// { dg-require-effective-target linker_plugin }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2 -flto" }
// { dg-additional-options "-fdump-tree-optimized -fdump-tree-assumptions -fdump-ipa-contract_shadow_early" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-prove-assume-lto.json" }

[[gnu::noinline]] int sink (int x) { return x + 1; }

[[gnu::noinline]] int sound (int x)
{
  contract_assert (x > 0);	// line 19: quick_enforce
  contract_assert (x > 0);	// line 20
  return sink (x / 4);
}

int main (int argc, char **)
{
  return sound (argc) == 1 ? 0 : 1;
}

// { dg-final { scan-ipa-dump "contract obligation \[0-9\]+: assumed" "contract_shadow_early" } }
// { dg-final { scan-ltrans-tree-dump {x_[0-9]+\(D\) = \[irange\] int \[1, \+INF\]} "assumptions" } }
// { dg-final { scan-ltrans-tree-dump-not "CONTRACT_PROVEN" "assumptions" } }
// { dg-final { scan-ltrans-tree-dump " >> 2;" "optimized" } }
// { dg-final { scan-ltrans-tree-dump-not " / 4;|CONTRACT_PROVEN" "optimized" } }
