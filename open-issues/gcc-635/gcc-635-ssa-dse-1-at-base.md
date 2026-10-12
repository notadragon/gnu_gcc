# GCC-635: `g++.dg/tree-ssa/ssa-dse-1.C` fails at base `8802cd9fdbe`

**Kind:** defect
**Status:** Open -- upstream's; stock trunk fails it identically
**Affects:** nothing in contracts.  One `dg.exp` row, a stale
`scan-tree-dump-times` expectation.  No contracts feature depends on it.
**Upstream Link:** None found (searched 2026-10-06: Bugzilla REST quicksearch
on `ssa-dse-1.C`)

## Symptom

A whole-`dg.exp` run on this branch at base `8802cd9fdbe` (2026-10-06) shows
**1 unexpected failure** and no others (176,527 g++ + 42,777 gcc passes):

    FAIL: g++.dg/tree-ssa/ssa-dse-1.C   scan-tree-dump-times dse1
      "MEM <char\\[[0-9]+]> \\[\\(struct FixBuf \\*\\)&<retval> \\+ [0-9]+B\\] = {}" 1

The previous base, `7aa4b1c5054`, passed it (217,204 pass / 0 unexpected).

## Why it is not ours

The stock Compiler Explorer nightly `gcc-trunk-20261005`, built at exactly
`8802cd9fdbe`, gives the same result: its `dse1` dump has no trimmed
`<retval> = {}` store at all, because the front end now emits the
constant-evaluated buffer as a single string store,

    <retval>.buf = "0 1 2 3 4 5 6 7 8 9 10 11 12 13";

where 16.2.0 (and this branch at the previous base) emitted
`<retval> = {}` followed by per-element stores, which DSE trimmed to the
`MEM <char[169]> ... + 31B] = {}` the test counts.  So the code got better
and the test's expectation went stale.

The likely cause is `a382bed036a` ("c++: Normalize character array
initializers after folding [PR127416]", `gcc/cp/typeck2.cc`), the only
commit in the drift that changes how a folded character-array initializer
is represented.  Not bisected.

## To retire

Re-run `dg.exp` after each rebase.  The row goes when upstream updates or
removes the test's expectation.  Reporting it upstream is the user's call.
