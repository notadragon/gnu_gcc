# GCC-27: A contract mismatch between two friend declarations is accepted silently

**Status:** Open -- **deliberately**, on this branch; see "Our Fix"
**Component:** c++ / contracts
**Upstream Link:** None found. Searched 2026-09-05, including resolved bugs:
comment text `friend contract mismatch`, comment text
`mismatched contract condition` (GCC's own message for the diagnosed case),
and summary `friend contract`. No hits on any of them
**Affects:** measured 2026-09-05 -- reproduces on stock g++ trunk
(17.0.0 20260901) with plain `-fcontracts`

## Bug Report

Two friend declarations of the same function may specify contradictory
contracts, and nothing is said:

```cpp
struct C {
  friend int f (int x) pre (x > 0);
  friend int f (int x) pre (x < 0);   // accepted; should be a mismatch
};
int f (int x) { return x; }
```

Only one of them takes effect, so a reader who trusts the other is misled
about what the function checks.

The control is what makes this a bug rather than a missing feature: **the
identical mismatch at namespace scope IS diagnosed**, with "mismatched
contract condition in declaration". So contract matching works; the friend
path skips it.

```cpp
int g (int x) pre (x > 0);
int g (int x) pre (x < 0);   // error: mismatched contract condition in declaration
```

## Root cause

A friend declaration's contract is `DEFERRED_PARSE` at the point
`duplicate_decls` merges the two declarations, and `check_redecl_contract`
skips matching when either side is still deferred.

Deferring the comparison instead is not the small change it looks like.
`duplicate_decls` discards the second declaration and calls
`remove_decl_with_fn_contracts_specifiers` on it -- dropping its
`contract_decl_map` entry -- **before** end-of-class late parsing runs. The
second contract's tokens are gone by the time anything could compare them. A
fix has to either

* compare the raw deferred token streams at the skip point, which is a
  textual match that diverges from the semantic matcher (it would report
  `pre ((x > 0))` against `pre (x > 0)`), or
* keep the discarded declaration's tokens and late-parse them in its own
  parameter scope, since the two friend declarations may name their
  parameters differently and the surviving declaration's scope cannot be
  reused.

## Reproducer

See [`gcc-27-deferred-friend-contract-mismatch.C`](gcc-27-deferred-friend-contract-mismatch.C)
in this directory, which carries the namespace-scope control alongside.

## Our Fix

**None, by decision.** Both routes above are invasive for a degenerate
construct, so the limitation is retained rather than overlooked;
investigated 2026-08-07. Tracked internally as CC-1 / F31.

It is pinned in the testsuite by
`gcc/testsuite/g++.dg/contracts/cpp26/contract-friend-deferred-mismatch.C`,
whose `xfail` accounts for both of the contracts suite's expected failures
(one per `-std` variant). A fix -- ours or upstream's -- turns that into an
XPASS, which is the signal to delete this entry and the `xfail` together.

Because it is unfixed here as well as upstream, it also has a row in
[`../open-issues/README.md`](../open-issues/README.md), which links back to
this writeup rather than repeating it.

## Notes

Clang has the same defect, tracked there as CLANG-12. That one is
branch-only and unfilable -- contracts are not upstream in Clang -- so this
report stands alone, but the two should be fixed with an eye on each other.

Originally recorded as branch-only in `open-issues/` and moved here
2026-09-05 on measuring that it reproduces on stock. The entry criterion is
reproduction on stock, and it had never been tested.
