---
id: 1025-contract-discardable-evaluation
subject: 'c++: contracts: P2900: do not let contract reports escape a discarded evaluation'
depends: [1020-constexpr-side-effect]
regenerates: []
fixes: [gcc-43]
---

## Rationale

Two evaluations during constant evaluation are speculative.  The operand of
an `[[assume]]` is not evaluated at all ([dcl.attr.assume]), and the
side-effect-free trial of a contract predicate added by
`1020-constexpr-side-effect` is thrown away and redone when the predicate
turns out to modify the enclosing evaluation.  Both already discard their
*evaluation* failure -- each passes a local `non_constant_p` -- but a
contract assertion reached along the way records itself on the shared
`constexpr_global_ctx`, and that escaped.

The consequences differed.  For the assumption it was a rejected program
(GCC-43): a refusal that was entirely correct, since the operand genuinely
must not have side effects, surfaced through
`check_for_failed_contracts` as `contract condition is not constant`.  For
the predicate trial it was a duplicated diagnostic, because a nested
contract's violation was recorded once per pass.

`contract_report_tracker` saves the four reporting fields and restores them,
exactly as `modifiable_tracker` does for the modifiable set.  It carries a
`dismiss ()` for the case where the trial turns out to be the real
evaluation, and a `restore ()` for rolling back mid-scope so that the re-run
records for real.

Note this is not a `modifiable_tracker` defect and would survive removing
that class from the contract site: what leaks is the reporting, not the
suppression.  The suppression was already correct, which
`-fcontract-evaluation-semantic=observe` showed before the fix -- the
program compiled and the modification was correctly absent.

## Compile gap

None.  The tracker is defined next to `modifiable_tracker` and used at both
sites in the same commit.

## Contents

- gcc/cp/constexpr.cc : @contract_report_tracker, /Some evaluations are speculative/, /void dismiss \(\) { dismissed = true; }/, /~contract_report_tracker \(\) { restore \(\); }/, /previous_extra_violations/, /nothing about this evaluation may reach/, /Spans the re-run decision below/, /No re-run: this trial IS the evaluation/, /cs\.restore \(\);/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-discardable-evaluation.C : *
