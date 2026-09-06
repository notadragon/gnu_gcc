# GCC-36: a non-template's postcondition const diagnostic is emitted twice

**Status:** Open on stock upstream; **not present on this branch**
**Component:** c++ / contracts
**Upstream Link:** None found (searched 2026-09-06)
**Reproduces on stock:** GCC **16.2.0** and **trunk** (Compiler Explorer build
`3653b8dd68b0`, 17.0.0 20260905). GCC 15.3.0 predates contracts.
**Found:** 2026-09-06, while fixing GCC-35.

## Symptom

For a **non-template** function, the [dcl.contract.func] const-parameter
diagnostic is emitted twice -- byte-identical message, byte-identical location,
identical note:

```c++
int f (int v) post (r: v == r) { return v; }
```

```
:1:24: error: a value parameter used in a postcondition must be const
:1:12: note: parameter declared here
:1:24: error: a value parameter used in a postcondition must be const
:1:12: note: parameter declared here
```

Reproducer:
[`gcc-36b-postcondition-const-nontemplate.C`](gcc-36b-postcondition-const-nontemplate.C).

**This branch reports it once**, and at the contract rather than at the
odr-use, having diverged in `check_postcondition_odr_use_r` (it honours a
contract location when one is available). So this is upstream's alone. It is
recorded here because the entry criterion for this directory is reproduction on
stock, whether or not the branch still has it.

## What this issue is NOT: the two-diagnostic template case

It was originally filed as "the rule is reported twice for a function
template", on the strength of this:

```c++
template <class T> T f (T v) post (r: v == r) { return v; }
int use () { return f (1); }
```

```
:1:27: error: value parameter 'v' used in a postcondition must be const
:1:39: error: a value parameter used in a postcondition must be const
```

**That is not a defect.** Two mechanisms implement the rule and they anchor at
different places on purpose:

* `check_postcondition_parm_in_redecl` reports **at the parameter**, naming it
  -- "which parameter is wrong";
* `check_postcondition_odr_use_r` reports **at the contract** -- "which
  postcondition uses it".

With one parameter and one postcondition those collapse onto adjacent columns
and look redundant, which is what the minimal reproducer above suggests. With
several they are complementary, and the testsuite already depends on it:
`dcl.contract.func.p7-t1.C` has a five-parameter function with four
postconditions and expects **both** families -- three "value parameter 'i'/'k'/'l'"
at the parameter list, and three "a value parameter ..." at the individual
offending `post` clauses. Our branch and stock trunk both emit 28 errors for
that file; they agree exactly.

A fix was attempted and reverted. Suppressing the walk when the carry-over had
already reported -- derivable without new state, since the carry-over's
condition is the walk's plus `!TREE_READONLY` -- works on the minimal
reproducer and destroys the per-contract diagnostics, taking 65 test results
with it. The lesson is that the minimal reproducer was not representative:
**check a multi-contract, multi-parameter function before concluding that two
diagnostics for one rule are redundant.**

An earlier attempt failed differently and is worth not repeating either:
skipping on `parm_used_in_post_p` alone conflates "the carry-over ran" with
"the carry-over reported", and contract constification sets `TREE_READONLY`, so
for a constified parameter the carry-over marks without diagnosing and the walk
is the only reporter.

## Impact

Diagnostic quality only, on stock, for non-template functions. Both messages
are true and the program is correctly rejected.

## Bugzilla

Searched 2026-09-06 by summary substring `postcondition`: 18 PRs, none about a
duplicated diagnostic. Nearest neighbours are different symptoms in the same
code: **PR124395**, **PR127196**, **PR126897**. Quicksearch on the diagnostic
text returns nothing.
