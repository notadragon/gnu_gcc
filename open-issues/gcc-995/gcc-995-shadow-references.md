# GCC-995: a proof shadow's references still count for the real code

**Kind:** deferred (a documented limitation) -- deferred by the user
2026-11-07 (DECISIONS.md E204 (a))
**Status:** Open, this branch only -- recorded 2026-11-07 (X9a-GCC, the
GCC-907 fix); documented in `gcc/doc/invoke.texi` under
`-fcontracts-prove-shadow`
**Affects:** the P4405 proof shadow (`gcc/contracts-shadow.cc`) and the IPA
analyses that derive a symbol's properties from its references:
`ipa_discover_variable_flags` (`gcc/ipa.cc`: read-only, write-only,
address taken), `cgraph_node::address_taken` (and so `local`, which gates
IPA-SRA and the local calling conventions), ipa-reference's module
statics
**Clang:** not affected -- its shadows are emitted into a separate proof
module

## Symptom

[`open-bug-p4405-shadow-references.C`](../../gcc/testsuite/g++.dg/contracts/cpp26/open-bug-p4405-shadow-references.C)
(`-O2 -fno-contracts-prove-assume`, `diagnose`): the assembly differs from
`-fno-contracts-prove-shadow` and from `ignore` in two places.

* `bump`, which only the predicate of `f`'s assertion calls, is the only
  code that stores to the static `counter`.  Without the shadow `counter`
  is found read-only and the real code's load of it folds to 0; with it,
  the shadow's store (once `bump` is inlined into it) keeps `counter`
  writable and the load stays.
* Only the predicate of `h`'s assertion takes the address of the static
  `g`.  Without the shadow `g`'s address is not taken, `g` is local, and
  IPA-SRA drops its unused parameter; with it, `g` is address-taken.

## Cause

GCC-907's fix makes IPA blind to the *calls* analysis-only code makes
(`contract_analysis_only_call_p`) and to its existence and size
(`contract_analysis_only_p`), but not to its *references* (`ipa_ref`s
whose referring symbol is a shadow or a body inlined into one).  A
predicate's references are in the shadow only: the real function's code
is `ignore`'s.

## Options

* (a) Leave it, documented: the real code can differ when a predicate is
  the only code that stores to a static variable (through a call) or takes
  the address of a static variable or function.  (A variable the real code
  only writes and a predicate reads measured unaffected: the write-only
  shape keeps the same code.)
* (b) Ignore analysis-only stores (and loads) in
  `ipa_discover_variable_flags`, and keep the shadow's own view by exempting
  analysis-only functions from the removal of stores to write-only
  variables (`tree-cfg.cc`) and from folding loads of a variable found
  read-only (`ctor_for_folding`); proofs then stay as they are.  The
  address-taken half stays: `TREE_ADDRESSABLE` cannot be cleared while the
  shadow's GIMPLE takes the address (verify_gimple), and a function made
  local while the shadow holds its address could have its signature
  changed under it.
* (c) (b), and for the address: give the shadow a stand-in for each
  symbol whose address only analysis-only code takes (a declaration-only
  alias, never emitted), so that the real symbol's address is not taken.
  Larger; touches every reference rewrite.

Recommendation: (b) if the identity matters for the read-only shape, with
the address half staying documented; otherwise (a).

## Decision

(a), DECISIONS.md E204, 2026-11-07: documented and left.  `invoke.texi`
says, under `-fcontracts-prove-shadow`, that the shadows' references still
count, with both shapes.  The watch test stays, xfailed, as the record of
the current behaviour (it XPASSes if the shadows' references stop
counting).  Tests that compare the real code across shadows or proof
optimization levels keep these shapes out.
