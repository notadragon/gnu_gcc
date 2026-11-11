---
id: 3610-constexpr-contract-evaluation
subject: 'c++: contracts: P3099: report every constant-evaluation violation, not just the first'
depends:
  - 1020-constexpr-side-effect
  - 2400-p3595
  - 3600-p3099
regenerates: []
fixes: [gcc-1006]
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

A predicate that exits via an exception is a contract violation too
([basic.contract.eval]).  The evaluation leaves no value and the exception
in its jump target; neither was looked at, so the null value reached
`integer_zerop' and the compiler crashed, and a throwing call left its
exception object allocated, to be reported as storage never deallocated.
The case now ends the exception while the rollback still holds its value --
`discard_contract_predicate_exception' frees every heap object the
predicate left live and drops the exception from the uncaught count -- and
records the violation with the exception's type and, for a class derived
from `std::exception', its `what ()' string, which the diagnostic reports
in the form an uncaught exception's is.  A predicate that yields no value
otherwise is not a constant expression.

Three commits meet here, in hunks with no unchanged line between them:

* `1020-constexpr-side-effect' -- the accumulation itself, which is
  base-facility behaviour; the fields it needs, and the modification-tracking
  it shares with the side-effect warning, are in that commit already.

* `2400-p3595' -- the contract's constexpr semantic is a separate, lazily
  populated slot, so the skip test becomes
  `contract_constexpr_terminating_p' / `contract_constexpr_ignored_p' over
  that slot, and `ensure_evaluation_semantic (..., in_ce=true)' has to
  populate it first.  `contract_terminating_p', which reads the run-time
  semantic, is what the base code tested; with that test gone it and its
  siblings `contract_ignored_p' and `contract_evaluated_p' have no caller
  left, so this commit deletes them from `contracts.h'.

* `3600-p3099' -- the diagnostic prints the user-supplied message when the
  contract carries one.

Placed after P3099, the last of them.

## Compile gap

None: the constexpr semantic accessors and `CONTRACT_MESSAGE' are all
introduced by the dependencies.  Before this commit the base one-violation
behaviour is still in place and self-consistent.

## Contents

- gcc/cp/constexpr.cc : @check_for_failed_contracts, @mark_non_constant, /A non-terminating \(observe\) contract violation recorded/, /representative violation: a terminating \(enforce\) one/, @contract_report_key, @reported_contract_problems, @emit_contract_violation_diagnostic, /global_ctx\.contract_outer_loc = cp_expr_loc_or_input_loc \(t\);/, /auto_vec<constexpr_contract_violation> contract_violations;/, /void record_contract_violation \(tree t, bool non_const,$/, /Populate the constexpr semantic slot lazily/, /record_contract_violation \(t, /, /^discard_contract_predicate_exception \(const constexpr_ctx \*ctx, tree exc,$/, /^\ttree exc_report = NULL_TREE;$/, /^\t  \/\* Before the rollback, while the exception object still has its$/
- gcc/cp/contracts.h : /^contract_terminating_p \(const_tree contract\)$/
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-contract-every-problem.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/matrix-notional-*.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-throwing-predicate*.C : *
