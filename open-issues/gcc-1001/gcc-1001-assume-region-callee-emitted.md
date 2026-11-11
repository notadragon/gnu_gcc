# GCC-1001: under -fcontracts-prove-assume, what only an unproven predicate calls is emitted

**Kind:** deferred (a documented limitation) -- deferred by the user
2026-11-09 (DECISIONS.md E212 (c))
**Status:** Open, this branch only -- recorded 2026-11-09 (X16-GCC, found
probing CLANG-914's shape); documented in `gcc/doc/invoke.texi` under
`-fcontracts-prove-shadow`
**Affects:** the P4405 assumption regions the front ends place under
`-fcontracts-prove-assume` (`c-family/contracts-prove-shadow.cc`,
`cp/contracts.cc`), removed in the real function by the `contract_proven`
pass when it is expanded; `expand_all_functions` (`cgraphunit.cc`), which
expands every function the IPA passes left reachable
**Clang:** not affected at any level (measured in X17-Clang, which adds a
passing mirror test): its regions are deleted by the back end, after
which `GlobalDCE` drops an unreferenced `linkonce_odr` or internal
function at -O1 and up

## Symptom

```c++
extern int g (int);
[[gnu::noinline]] inline bool h (int n) { return g (n) > 0; }
[[gnu::noinline]] static bool s (int n) { return g (n + 1) > 0; }
int f (int n) { contract_assert (h (n)); contract_assert (s (n)); return n; }
```

`-std=c++26 -fcontracts -fcontracts-p4405 -O2
-fcontract-evaluation-semantic=diagnose` (so `-fcontracts-prove-assume`):
neither obligation is proven, so no assumption is kept and `f` is the same
as with `ignore`, but `h` and `s` are emitted out of line, called by
nothing.  With `ignore`, `-fno-contracts-prove-assume` or
`-fno-contracts-prove-shadow` they are not.  Likewise at -O1 and -O2 with
`-fno-inline` for a plain `inline` or `static` callee, and at -O1 for an
inline virtual function called by qualified name (GCC-1000's shape,
`-fno-inline`).  -O0 does not differ.  Watch test
[`open-bug-p4405-prove-assume-region-callee.C`](../../gcc/testsuite/g++.dg/contracts/cpp26/open-bug-p4405-prove-assume-region-callee.C).

## Cause

Each obligation leaves the region `if (.CONTRACT_PROVEN (id)) <assume
pred>` in the real function, so the predicate's callees have real callers
through every IPA pass; the inliner inlines them into the region when it
can, and then nothing is left out of line.  The region is folded away when
the real function is expanded (`contract_proven`), after the shadows have
decided the obligation (`contract_shadow_expand_first`), but by then
`expand_all_functions` has its list: every function reachable at the end
of IPA is expanded and emitted, whether or not anything still calls it.
(Upstream does the same for a call a late pass deletes; what is particular
here is that the call was never in the real code.)

## Fix options

* (a) Once the shadows are decided, before any real function is expanded,
  drop the call edges and references of every unproven obligation's region
  (the blocks the region's guard alone reaches, as for the discriminators),
  and skip expanding the functions nothing real reaches any longer
  (`process` cleared; a COMDAT group as a whole).  Inline clones in a
  region count as part of it.  Moderate: `contracts-shadow.cc` and the
  expansion loop in `cgraphunit.cc`.
* (b) Decide the obligations before the IPA passes end, so the regions are
  gone before the last removal of unreachable functions: the shadows would
  have to be optimized before the real code's IPA decisions are final,
  against the current design (the shadows are expanded first, at
  expansion).  Larger.
* (c) Document it as a limitation (as GCC-995): under
  `-fcontracts-prove-assume`, a function that only an unproven predicate
  calls and that is not inlined can be emitted; the code of the functions
  holding the evaluations is unchanged.

Recommendation: (a) if the extra functions matter (each is dead code, a
size cost only, and a COMDAT one is merged by the linker); otherwise (c),
as E204 (a) did for GCC-995.

## Decision

(c), DECISIONS.md E212, 2026-11-09: documented and left, as GCC-995
(E204 (a)) and GCC-998 (E209 (c)) were.  `invoke.texi` says, under
`-fcontracts-prove-shadow`, after those two paragraphs, that under
`-fcontracts-prove-assume` a function only an unproven evaluation's
predicate calls, and that is not inlined, can still be emitted out of
line, called by nothing, while the code of the function holding the
evaluation is the same.  The watch test stays, xfailed, as the record of
the current behaviour (it XPASSes if the callees stop being emitted).
Revisit with (a) if the dead out-of-line copies turn out to matter.
