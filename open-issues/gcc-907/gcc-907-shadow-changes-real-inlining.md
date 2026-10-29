# GCC-907: a proof shadow changes the real function's inlining

**Kind:** defect
**Status:** Open -- recorded 2026-10-24 (found while updating D4405R0's
"Proving Strategies"); to be fixed later, per the user
**Affects:** the P4405 proof shadow (`gcc/contracts-shadow.cc`,
`contract_shadow_clone`) and the IPA inliner's view of callers
**Clang:** not affected -- its shadows are emitted into a separate proof
module, so the real module's inlining never sees them (measured: identical
code with and without shadows); mirror:
`clang/test/Contracts/p4405-shadow-real-inlining.cpp` in llvm_llvm-project

## Symptom

[`open-bug-p4405-shadow-real-inlining.C`](../../gcc/testsuite/g++.dg/contracts/cpp26/open-bug-p4405-shadow-real-inlining.C):
the large `static` function `big` is called once, from `f`, which holds an
unproven `diagnose` evaluation.  At `-O2`, measured with the installed g++
(the install W1-GCC pulled on 2026-10-24; gnu_gcc head 014bc40dfba changed
only test comments since):

| Configuration | `big` in `f` |
|---|---|
| `-fcontract-evaluation-semantic=ignore` | inlined; `big` not emitted |
| `diagnose -fno-contracts-prove-shadow` | inlined; `big` not emitted |
| `diagnose` (shadow on) | called out of line; `_ZL3bigi` emitted |
| `diagnose -fno-contracts-prove-assume` | called out of line; `_ZL3bigi` emitted |

The design promises that the real function's code is what it would be with
*ignore* unless a sound proof is assumed (`p4405-shadow-same-code.C` checks
a case where nothing else differs, but not this one).

## Cause

`contract_shadow_clone` creates the shadow before `pass_build_ssa_passes`
as an ordinary clone of the function, so for the IPA inliner the shadow is
one more caller of every function the original calls.  `big` goes from one
caller to two, so the "called once" and growth heuristics decide against
inlining it into `f` (the IPA inline dump shows two callers).

## Fix direction

Callers that are shadows must not count in the inliner's decisions for the
real code: exclude shadow call edges when the inliner counts callers and
estimates growth for a callee (and anywhere else IPA sizes a callee by its
callers -- e.g. ipa-cp/ipa-sra caller statistics), while still letting the
shadow inline what it needs.  Removing a callee whose only remaining
callers are shadows is already handled (shadow-only callees are kept as
declarations).  Then un-xfail the watch test and add the ignore-identity
check for this shape to `p4405-shadow-same-code.C`.
