---
id: 0250-constexpr-unreported-incomplete-contract
subject: 'c++/contracts: do not report contracts from a constant evaluation that fails'
depends: []
regenerates: []
fixes: [gcc-618]
---

## Rationale

GCC-618.  Whether a variable is constant-initialized is decided by a
notional evaluation of its initializer ([expr.const]); an array bound where
VLAs are available is decided the same way.  A contract assertion met during
that evaluation records its problem -- a false predicate, or one that is not
a core constant expression -- and evaluation continues.  If the rest of the
evaluation is not constant either, the variable is initialized at run time
and its predicates are evaluated there, so nothing recorded may be reported:
it is not a contract violation in a manifestly constant-evaluated context,
because the expression is not constant-evaluated at all.

`cxx_eval_outermost_constant_expr` called `check_for_failed_contracts`
whenever the evaluation was allowed to be non-constant, including when it
was, and the initializer of a potentially-constant variable is manifestly
constant-evaluated, so a precondition on a function taking its argument by
reference rejected `const int step = get_one (divisor);` in a function never
constant-evaluated.  Where a constant is required, the recorded contract was
reported in place of the reason the expression is not constant.  Report
only when the evaluation completed.

## Compile gap

None.

## Contents

- gcc/cp/constexpr.cc : /else if \(!non_constant_p && check_for_failed_contracts \(&ctx\)\)/, /A contract problem recorded during an evaluation that is not constant/
- gcc/testsuite/g++.dg/contracts/cpp26/precondition-const-local-init.C : *
