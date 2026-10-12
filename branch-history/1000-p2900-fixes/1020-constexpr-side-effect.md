---
id: 1020-constexpr-side-effect
subject: 'c++: contracts: P2900: undo what a predicate modifies during constant evaluation'
depends: []
regenerates: []
fixes: [gcc-42]
---

## Rationale

A contract predicate evaluated during constant evaluation runs under a
`modifiable_tracker`, which refuses any store to an object that belongs to the
enclosing evaluation.  Upstream treated that refusal as "not a constant
expression" and rejected the program.  That is wrong: a predicate that
modifies the enclosing evaluation is still a core constant expression, so
rejecting it rejects a well-formed program.  [basic.contract.eval] leaves it
unspecified whether the predicate is evaluated at all, and permits an
evaluation that produces the predicate's value without its side effects.

So evaluate the predicate for real, and then undo whatever it modified: the
enclosing evaluation proceeds as if the predicate had not been evaluated, and
means the same under every evaluation semantic (under `ignore` the predicate
is not evaluated at all).  This is the side-effect-free evaluation the wording
permits, obtained by rolling back rather than by refusing.

`predicate_rollback` does it.  `constexpr_global_ctx` gains a pointer to the
innermost one, and the four members that change an entry of the value map
(`get_value_ptr` when it hands out a slot to store through, `put_value`,
`destroy_value` and `clear_value`) report the entry first through
`note_modification`.  Each active rollback saves the entry's old value, or its
absence, the first time it is reported, together with a copy of every
CONSTRUCTOR reachable from it; the destructor puts the entries back and
restores those CONSTRUCTORs *in place*, because the enclosing evaluation may
be building one of them (`constexpr_ctx::ctor`) while the predicate runs --
a constructor's own `contract_assert`, for instance.  An entry that did not
exist is removed again, which also disposes of the predicate's own locals and
temporaries.  Heap objects that existed when the predicate started and that it
deallocated are revived, and `heap_dealloc_count` follows.  Rollbacks nest:
each saves independently, so an inner one restores the state at its start and
an outer one the state at its own.

The `ASSERTION_STMT` / `PRECONDITION_STMT` / `POSTCONDITION_STMT` case of
`cxx_eval_constant_expression` then evaluates the predicate once, under a
`predicate_rollback`, where upstream used a `modifiable_tracker`.

## Compile gap

None.

## Contents

- gcc/cp/constexpr.cc : /class predicate_rollback \*rollback;/, /^      rollback \(nullptr\),$/, /^    if \(rollback\)$/, /Tell every active predicate_rollback/, /^\/\* Helper class for constexpr_global_ctx\.  \[basic\.contract\.eval\] leaves it$/, @predicate_rollback, /^  explicit predicate_rollback \(constexpr_global_ctx \*g\)$/, /^constexpr_global_ctx::note_modification \(tree t\)$/, /^\t  predicate_rollback rb \(new_ctx\.global\);$/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-side-effect.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-side-effect-nested.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-side-effect-rollback.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-side-effect-warn.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-nested-modifiable-tracker.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-assume-nesting-permutations.C : *
