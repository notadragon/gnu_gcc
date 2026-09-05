# GCC-27: A contract mismatch between two friend declarations is accepted silently

**Kind:** deferred
**Status:** Open
**Affects:** `-fcontracts`, C++26 and later; two friend declarations of the
same function, in the same class, carrying different contract predicates
**Workaround:** declare the function at namespace scope and befriend that
declaration; the mismatch is then diagnosed normally

## Symptom

Two friend declarations of one function may specify contradictory
preconditions and the compiler says nothing. Only one of them takes effect,
so a reader who trusts the other one is misled about what the function checks.

## Trigger

See `gcc-27-deferred-friend-contract-mismatch.C` in this directory. It also
carries the control that bounds the problem: the same mismatch written at
namespace scope IS diagnosed, with "mismatched contract condition in
declaration". So the gap is specific to friend declarations, not to contract
matching.

## Why it is open

A friend declaration's contract is `DEFERRED_PARSE` at the point
`duplicate_decls` merges the two declarations, and `check_redecl_contract`
skips matching when either side is still deferred.

Deferring the comparison instead is not the small change it appears to be.
`duplicate_decls` discards the second declaration and calls
`remove_decl_with_fn_contracts_specifiers` on it -- dropping its
`contract_decl_map` entry -- *before* end-of-class late parsing runs. The
second contract's tokens are gone by the time anything could compare them.
A fix has to either

* compare the raw deferred token streams at the skip point, which is a textual
  match that diverges from the semantic matcher (it would report
  `pre ((x > 0))` against `pre (x > 0)`), or
* keep the discarded declaration's tokens and late-parse them in its own
  parameter scope, since the two friend declarations may name their parameters
  differently and the surviving declaration's scope cannot be reused.

Both are invasive for a degenerate construct, so the limitation is retained by
decision rather than by oversight. Investigated 2026-08-07.

## Notes

Tracked internally as CC-1 / F31. Pinned in the testsuite by
`gcc/testsuite/g++.dg/contracts/cpp26/contract-friend-deferred-mismatch.C`,
whose `xfail` accounts for both of the contracts suite's expected failures
(one per `-std` variant). A fix will turn that into an XPASS, which is the
signal to delete this entry and the `xfail` together.
