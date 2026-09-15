# GCC-43: An `[[assume]]` whose operand contains a side-effecting contract assertion is rejected instead of declined

**Status:** Open (upstream); fixed here
**Resolved by:** [69bc4b2defd8](https://github.com/notadragon/gnu_gcc/commit/69bc4b2defd8e4c943f0caaa0f71c956d1c2eccb)
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** UNKNOWN -- Bugzilla has not been searched for this one.
The three PRs carrying the same diagnostic text
([125459](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125459),
[125587](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125587),
[124100](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124100)) are GCC-3 and
a resolved bug; none is this.
**Affects:** re-measured 2026-09-14 -- still rejected on stock trunk
17.0.0 20260914 (built at `b0730e1d39f`), and, until
[69bc4b2defd8](https://github.com/notadragon/gnu_gcc/commit/69bc4b2defd8e4c943f0caaa0f71c956d1c2eccb), on this branch too.  Unaffected by
GCC-9's fix landing the same day, which is the point of the Notes below:
this is a third, independent defect in the same machinery.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `17.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] an [[assume]] operand containing a side-effecting contract assertion is rejected rather than ignored` |

Attachments:

| File | Description |
|---|---|
| [`assume-operand-contract-rejected.cpp`](assume-operand-contract-rejected.cpp) | Reproducer, its own preprocessed source |

````
[dcl.attr.assume] says the operand of [[assume]] is not evaluated.  An
implementation that cannot evaluate it without side effects therefore has
nothing to do: it does not get to assume anything, and the program is
unaffected.  g++ rejects it instead.

  constexpr bool modifying (int *p) { contract_assert ((++*p, true)); return true; }

  constexpr int f ()
  {
    int i = 0;
    [[assume (modifying (&i))]];   // operand is NOT evaluated
    return i;                      // therefore: 0
  }

  constexpr int v = f ();

  error: contract condition is not constant

Expected: the assumption is discarded and v == 0.  Clang accepts the program
and says so:

  warning: assumption is ignored because it contains (potential)
           side-effects [-Wassume]

The suppression is already correct -- only the reporting is wrong.  Compile
the same translation unit under a non-terminating semantic and it builds and
gives the right answer:

  $ g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe ...
  warning: contract condition is not constant
  $ ./a.out; echo $?
  0

So the modification is correctly discarded; what escapes is the DIAGNOSTIC.

Root cause: the contract case in cxx_eval_constant_expression records its
outcome on the GLOBAL context --

    ctx->global->contract_statement = t;
    ctx->global->contract_condition_non_const = true;

-- and check_for_failed_contracts reports that afterwards.  The [[assume]]
evaluation that contained it is speculative and discardable: it runs with
quiet = true, and its failure means only "do not use this assumption".  But
the contract state it wrote is not part of what gets discarded, so a failure
that should have been swallowed is reported as a hard error under a
terminating semantic.

Note this is NOT a modifiable_tracker defect and would survive removing the
tracker from the contract site entirely: the tracker's refusal is correct
here, because the enclosing assumption genuinely must not have side effects.
The bug is that a correct refusal inside a discardable evaluation escapes as
a diagnostic.

Suggested fix: save and restore the contract reporting state
(contract_statement, contract_condition_non_const) across the speculative
evaluation of an [[assume]] operand, the same way the tracker saves and
restores the modifiable set -- or, equivalently, do not let a contract
recorded under a quiet, discardable evaluation reach
check_for_failed_contracts.
````

## Notes

**Distinct from GCC-9 and GCC-42**, which are the other two defects around
this machinery.  All three no longer reproduce on this branch, but by
different routes:

| | what it is | fixed here |
|---|---|---|
| GCC-9 | `~modifiable_tracker` cleared the enclosing tracker's set instead of restoring it, so nesting broke suppression | fixed **upstream**, [`7b60a368fb1`](https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=7b60a368fb19b33f8676482a233d389cbc47b85e) (PR127282), so this branch no longer carries it |
| GCC-42 | a correctly refused store was reported as non-constancy, rejecting a well-formed contract predicate | yes, [d81fd685f92e](https://github.com/notadragon/gnu_gcc/commit/d81fd685f92e368a66d65e74c896553311c9d537) |
| GCC-43 | a correct refusal inside a *discardable* evaluation escapes as a diagnostic | yes, [69bc4b2defd8](https://github.com/notadragon/gnu_gcc/commit/69bc4b2defd8e4c943f0caaa0f71c956d1c2eccb) |

The composition of `[[assume]]` and contract assertions is otherwise correct
on this branch, which is worth stating because the asymmetry invites doubt: a
contract predicate under a checking semantic is evaluated and its side
effects are real, an `[[assume]]` operand is not evaluated and its side
effects must never be observable, and an inner construct never overrides the
outer one's rule.  Measured over every nesting permutation --

| outer | inner | delivery | modifications visible |
|---|---|---|---|
| `contract_assert` | `contract_assert` | function / lambda | 3 |
| `contract_assert` | `[[assume]]` | function / lambda | 2 |
| `[[assume]]` | `contract_assert` | function / lambda | **0** |
| `[[assume]]` | `[[assume]]` | function / lambda | **0** |

-- so a contract assertion nested inside an assumption discards everything it
does, which is required.  Coverage is
[`g++.dg/contracts/cpp26/contract-assume-nesting-permutations.C`](../../gcc/testsuite/g++.dg/contracts/cpp26/contract-assume-nesting-permutations.C);
this row's own coverage is
[`g++.dg/contracts/cpp26/contract-discardable-evaluation.C`](../../gcc/testsuite/g++.dg/contracts/cpp26/contract-discardable-evaluation.C),
which also pins the second symptom the same escape produced: a violation
recorded by a nested contract during the discarded trial of a modifying
predicate used to be reported once per pass.

The `Suggested fix` above is what was implemented, in
[69bc4b2defd8](https://github.com/notadragon/gnu_gcc/commit/69bc4b2defd8e4c943f0caaa0f71c956d1c2eccb): a `contract_report_tracker`
alongside `modifiable_tracker`, saving and restoring the four reporting
fields, used at both discardable evaluations.  The row stays here because it
is still open upstream.

Found 2026-09-12 while checking whether GCC-42's fix could have been a
deletion of the tracker from the contract site rather than the two-pass
re-evaluation it is.  It could have been -- and this row is what that
question turned up on the way.
