---
id: 3610-constexpr-contract-evaluation
subject: 'c++: contracts: P3099: report every constant-evaluation violation, not just the first'
depends:
  - 1020-constexpr-side-effect
  - 2400-p3595
  - 3600-p3099
regenerates: []
fixes: []
---

## Rationale

The base implementation recorded exactly one failed contract per outermost
constant evaluation -- `constexpr_global_ctx::contract_statement' -- and
short-circuited every contract after it.  That is wrong twice over once a
constexpr-specific evaluation semantic exists: an `observe' violation is not
an error and must not suppress a later `enforce' one, and several `observe'
violations in one constant expression should all be reported.

So the evaluator now accumulates.  `record_contract_violation' keeps a
representative statement, preferring a terminating one so evaluation order
cannot mask an `enforce', and pushes every non-terminating one onto a vector
capped at eight with a "and N more" summary.  `check_for_failed_contracts'
reports the terminating one alone as an error, or walks the vector as
warnings.  The per-violation diagnostic is factored out as
`emit_contract_violation_diagnostic' so the two paths cannot drift.

Three commits meet here, in hunks with no unchanged line between them:

* `1020-constexpr-side-effect' -- the accumulation itself, which is
  base-facility behaviour; the fields it needs, and the modification-tracking
  it shares with the side-effect warning, are in that commit already.

* `2400-p3595' -- the contract's constexpr semantic is a separate, lazily
  populated slot, so the skip test becomes
  `contract_constexpr_terminating_p' / `contract_constexpr_ignored_p' over
  that slot, and `ensure_evaluation_semantic (..., in_ce=true)' has to
  populate it first.  `contract_terminating_p', which reads the run-time
  semantic, is what the base code tested.

* `3600-p3099' -- the diagnostic prints the user-supplied message when the
  contract carries one.

Placed after P3099, the last of them.

## Compile gap

None: the constexpr semantic accessors and `CONTRACT_MESSAGE' are all
introduced by the dependencies.  Before this commit the base one-violation
behaviour is still in place and self-consistent.

## Contents

- gcc/cp/constexpr.cc : @check_for_failed_contracts, @mark_non_constant, /A non-terminating \(observe\) contract violation recorded/, /representative violation for reporting/, /auto_vec<constexpr_contract_violation> contract_violations;/, /void record_contract_violation \(tree t, bool non_const\)/, /Populate the constexpr semantic slot lazily/, /record_contract_violation \(t, /
