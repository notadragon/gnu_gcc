---
id: 1025-contract-discardable-evaluation
subject: 'c++: contracts: P2900: do not let contract reports escape a discarded evaluation'
depends: []
regenerates: []
fixes: [gcc-43]
---

## Rationale

The operand of an `[[assume]]` is not evaluated ([dcl.attr.assume]), so the
constant evaluator evaluates it speculatively and throws the result away if it
cannot be had cleanly.  It already discards the *evaluation* failure -- it
passes a local `non_constant_p` -- but a contract assertion reached along the
way records itself on the shared `constexpr_global_ctx`, and that escaped.
The result was a rejected program (GCC-43): a refusal that was entirely
correct, since the operand genuinely must not have side effects, surfaced
through `check_for_failed_contracts` as `contract condition is not constant`.

`contract_report_tracker` saves the four reporting fields and restores them,
exactly as `modifiable_tracker` does for the modifiable set, and
`cxx_eval_assert` uses one around the speculative evaluation.

Note this is not a `modifiable_tracker` defect and would survive removing
that class: what leaks is the reporting, not the suppression.  The
suppression was already correct, which
`-fcontract-evaluation-semantic=observe` showed before the fix -- the
program compiled and the modification was correctly absent.

## Compile gap

None.  The tracker is defined next to `modifiable_tracker` and used in the
same commit.

## Contents

- gcc/cp/constexpr.cc : @contract_report_tracker, /^\/\* Helper class for constexpr_global_ctx\.  The operand of an \[\[assume\]\] is$/, /previous_extra_violations/, /nothing about this evaluation may reach/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-discardable-evaluation.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-contract-observe-not-constant-init.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-contract-array-bound.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-contract-observe-warning-once.C : *
