# GCC-43: An `[[assume]]` whose operand contains a side-effecting contract assertion is rejected instead of declined

**Status:** Open (upstream); fixed here
**Resolved by:** [e929756310e4](https://github.com/notadragon/gnu_gcc/commit/e929756310e479b55a9ab5347cc4bcb1e2c7e18b)
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found (searched 2026-09-16).  Queries, all
`component:c++`: `assume contract`, `assume discarded`, `assume side effect`,
`assume operand rejected`, `contract_assert assume`,
`"contract condition is not constant"`, `keywords:rejects-valid assume`, and
the whole `component:c++ assume` list read through.

Two neighbours, neither of them this:

* [PR127282](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127282) --
  "[[assume]]: a nested assumption's side effects are not rolled back during
  constant evaluation".  **Ours**, filed from this work as GCC-9 and since
  landed upstream as `7b60a368fb1`.  Same machinery, different defect: this
  row is about a refusal escaping a discardable evaluation, not about side
  effects surviving one, and it was re-measured as still failing after
  GCC-9's fix landed.
* The three PRs carrying the same diagnostic text
  ([125459](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125459),
  [125587](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125587),
  [124100](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124100)) are GCC-3 and
  a resolved bug.

So this one is genuinely unreported, which makes it filable as it stands.
**Affects:** re-measured 2026-09-17 -- rejected on `16.1.0`, `16.2.0` and
trunk `17.0.0 20260914` (`b76fde4b175`), and, until
[e929756310e4](https://github.com/notadragon/gnu_gcc/commit/e929756310e479b55a9ab5347cc4bcb1e2c7e18b), on this branch too.  The reproducer
cannot be built before 16.1.0, so the defect is as old as the syntax.
Unaffected by GCC-9's fix, which is the point of the Notes below: this is a
third, independent defect in the same machinery.

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
    [[assume (modifying (&i))]];   // operand is not evaluated
    return i;                      // therefore: 0
  }

  constexpr int v = f ();

  error: contract condition is not constant

Expected: the assumption is discarded and v == 0.

The suppression is already correct -- only the reporting is wrong.  Compile
the same translation unit under a non-terminating semantic and it builds and
gives the right answer:

  $ g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe ...
  warning: contract condition is not constant
  $ ./a.out; echo $?
  0

So the modification is correctly discarded; what escapes is the diagnostic.

DISCOVERY

Found while auditing the constant-evaluation machinery around [[assume]] for
two other defects: a nested assumption's side effects surviving a discarded
evaluation (filed as PR127282), and a contract predicate that modifies the
enclosing constant evaluation being rejected outright.  All three live in the
same few functions, which is why one audit produced three, and the third only
became visible once the other two were understood well enough to rule them
out as explanations.


ANALYSIS

The contract case in cxx_eval_constant_expression records its outcome on the
Global evaluation context rather than on the evaluation that produced it:

    ctx->global->contract_statement = t;
    ctx->global->contract_condition_non_const = true;

check_for_failed_contracts reads that afterwards and turns it into the
diagnostic.

The [[assume]] operand is evaluated speculatively.  It runs with quiet = true
precisely because its failure is not supposed to be an error -- the failure
means only "this assumption cannot be used", and [dcl.attr.assume] says the
operand is not evaluated in the first place.  Everything else about that
speculative evaluation is discarded when it fails.  The two fields above are
not: they live one level up, on the global context, so they outlive the
evaluation that set them and are still there when
check_for_failed_contracts runs.

So the escape is a scoping mismatch between where the refusal is decided and
where it is recorded, not a wrong decision.  Two observations pin that down:

* The suppression already works.  Compiled under a non-terminating semantic
  the same translation unit builds and the modification really is discarded --
  the program returns 0, which is the answer [dcl.attr.assume] requires.  Only
  the severity of the report differs between the two semantics, which is what
  a leaked diagnostic looks like and not what a wrong evaluation looks like.

* It is not a modifiable_tracker defect, and would survive removing the
  tracker from the contract site entirely.  The tracker's refusal is correct
  here: the enclosing assumption genuinely must not have side effects.  The
  bug is that a correct refusal inside a discardable evaluation escapes.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       rejects
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260914, b76fde4b175  yes

13.4.0 does not accept -std=c++26; 14.4.0 and 15.3.0 accept the option but
not `contract_assert`, so the reproducer cannot be built before 16.1.0 and
the defect has existed for as long as the syntax has.
````

## Our Fix

Save and restore the contract reporting state -- `contract_statement` and
`contract_condition_non_const` -- across the speculative evaluation of an
`[[assume]]` operand, the same way the tracker saves and restores the
modifiable set.  Equivalently: do not let a contract recorded under a quiet,
discardable evaluation reach `check_for_failed_contracts`.

Kept out of the report block deliberately.  The analysis there says where the
defect is; what to do about it is upstream's call, and a bug report that
prescribes a patch invites an argument about the patch instead of agreement
about the bug.

## Notes

**Distinct from GCC-9 and GCC-42**, which are the other two defects around
this machinery.  All three no longer reproduce on this branch, but by
different routes:

| | what it is | fixed here |
|---|---|---|
| GCC-9 | `~modifiable_tracker` cleared the enclosing tracker's set instead of restoring it, so nesting broke suppression | fixed **upstream**, [`7b60a368fb1`](https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=7b60a368fb19b33f8676482a233d389cbc47b85e) (PR127282), so this branch no longer carries it |
| GCC-42 | a correctly refused store was reported as non-constancy, rejecting a well-formed contract predicate | yes, [53dcb8cbbda1](https://github.com/notadragon/gnu_gcc/commit/53dcb8cbbda155f40d9acc2bc702b0fbaba92015) |
| GCC-43 | a correct refusal inside a *discardable* evaluation escapes as a diagnostic | yes, [e929756310e4](https://github.com/notadragon/gnu_gcc/commit/e929756310e479b55a9ab5347cc4bcb1e2c7e18b) |

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
[e929756310e4](https://github.com/notadragon/gnu_gcc/commit/e929756310e479b55a9ab5347cc4bcb1e2c7e18b): a `contract_report_tracker`
alongside `modifiable_tracker`, saving and restoring the four reporting
fields, used at both discardable evaluations.  The row stays here because it
is still open upstream.

Found 2026-09-12 while checking whether GCC-42's fix could have been a
deletion of the tracker from the contract site rather than the two-pass
re-evaluation it is.  It could have been -- and this row is what that
question turned up on the way.
