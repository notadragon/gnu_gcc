---
id: 0170-postcondition-undeduced-result-ice
subject: 'c++: contracts: P2900: do not emit a check whose predicate was never substituted'
depends: []
regenerates: []
fixes: [gcc-49]
---

## Rationale

GCC-49 (PR c++/127450).  A postcondition with a result name, written on a
function with a deduced return type whose deduction never completes, aborted
the compiler:

    internal compiler error: in check_noexcept_r, at cp/except.cc:1063

Such a predicate is parsed with `processing_template_decl` raised, because the
result variable's type is `auto` while it is being parsed, so a call inside it
is built in template form with an unresolved callee.  `rebuild_postconditions`
is what makes it concrete once the return type is known, and it declines while
the type is undeduced -- so when the body never produces a type, nothing comes
back to finish the job.  Genericization then asked `expr_noexcept_p` whether
the predicate could throw, and `check_noexcept_r` asserted that the callee had
a pointer type.  A call in the predicate was part of the trigger, because
`check_noexcept_r` asserts on nothing else: without one the same unsubstituted
state passed through silently.

`cp_genericize_r` now asks `contract_predicate_unsubstituted_p` before
building a check, and falls through to the `void_node` replacement it already
performs for a contract that produced none.  The test is the result variable's
own type: `rebuild_postconditions` replaces `POSTCONDITION_IDENTIFIER` with a
copy carrying the deduced type in the same breath as it substitutes the
condition, so a result variable whose type still uses `auto` is exactly one
whose predicate was never substituted.  The late-parse path used for member
functions builds that variable with the real type and never answers true.

The guard sits in the caller rather than in `build_contract_check` so that it
reads against upstream's code and nothing else: the `case POSTCONDITION_STMT`
arm of `cp_genericize_r` is upstream's byte for byte, while this branch has
rewritten `build_contract_check` entirely for P3595.  A guard written inside
that function would arrive with the P3595 rewrite, several commits after the
helper it calls, and would read as a fragment of that rewrite rather than as
a fix for this defect.

Reaching the guard at all requires an ill-formed function, and the diagnostic
that says so has already been emitted by the time genericization runs, so
declining costs nothing; `gcc_checking_assert (seen_error ())` pins that,
because the one outcome worse than the abort would be silently dropping a
check in a program that is otherwise accepted.

**Invalidating the contract instead would have been too late, and one of the
rows in the test proves it.**  For a function with no return statement at all,
`finish_function`'s auto-to-void fallback runs `rebuild_postconditions`, which
diagnoses the result name against `void` and invalidates the contract -- and
the compiler still aborted, after saying the correct thing, because
`maybe_apply_function_contracts` had already spliced the checks into the body
several steps earlier.  Declining at the point of emission has no such
ordering dependency, and it covers the outlined-check and P3595
dynamic-dispatch modes this branch adds as well as the default one.

The test carries eight shapes that each failed deduction for a different
reason -- an ill-formed returned expression, use before deduction, no return
statement, `auto *`, `decltype (auto)`, a template instantiation, a lambda and
two postconditions on one function -- and three controls.  The middle control
is load-bearing: a postcondition with no result name on a function whose
return type is *also* still undeduced when the checks are spliced is well
formed and must keep its check, which is why the guard asks about the result
variable rather than about the function's return type.

## Compile gap

None.  One file-local predicate and one branch in the genericization walk,
both in `cp-gimplify.cc`, against code that is upstream's at this point in the
history and using only `type_uses_auto` and `seen_error`.  Nothing here
depends on anything this branch adds later.

The predicate is file-local rather than exported from `contracts.cc` for the
same reason the guard is in the caller: `cp_genericize_r` is its only user,
and keeping the change to one file that upstream also has keeps it readable as
a fix rather than as a slice of this branch's contract machinery.

## Contents

- gcc/cp/cp-gimplify.cc : /^contract_predicate_unsubstituted_p/, /A predicate that was never substituted/
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-undeduced-result.C : *
