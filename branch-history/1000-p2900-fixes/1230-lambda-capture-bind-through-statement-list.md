---
id: 1230-lambda-capture-bind-through-statement-list
subject: "c++: contracts: P2900: find a lambda's capture bind through a statement list"
depends: []
regenerates: []
fixes: [gcc-16]
---

## Rationale

GCC-16 (PR c++/126038), the half of it that is a defect in the base facility.
A precondition on a lambda that named one of the lambda's own captures ICEd:

    internal compiler error: in gimplify_var_or_parm_decl, at gimplify.cc:3298

That assertion catches a `VAR_DECL` the gimplifier has not seen declared in any
`BIND_EXPR`.  The decl is the capture proxy, whose `DECL_VALUE_EXPR` is
`__closure->__a`; it would have resolved a few lines further on had it been in
scope.  `maybe_apply_function_contracts` already hoists a lambda's body
`BIND_EXPR` out and re-opens it around the contracts, precisely so the capture
proxies stay in scope for the predicates -- but it matched only a body that was
itself a `BIND_EXPR`.  The proxy-declaring `BIND_EXPR` also arrives wrapped in a
`STATEMENT_LIST` holding nothing else, and in that case the hoist was skipped
and the precondition was emitted as a *preceding sibling* of the bind that
declares the proxies.  Only preconditions ever crashed: postconditions are
emitted after the body, by which point the gimplifier has already walked that
bind.  All three explicit capture forms reached it -- by reference, by copy,
and an init-capture.

The fix looks through a single-statement `STATEMENT_LIST` when locating the
bind.  Two details are what a reviewer should check.  The look-through is
deliberately narrow: it unwraps only a list whose first statement is also its
last, so a lambda body that genuinely is a sequence is left alone.  And the
`BIND_EXPR` test moved out of the `if` condition and into the block, so a
lambda whose body is neither shape now falls through with `fnbody` untouched
instead of being skipped by the outer test -- same behaviour, but the two
questions ("is this a lambda" and "did we find a bind") are now asked
separately, which is what makes the wrapper case expressible at all.

This does not close GCC-16 by itself, and is not meant to.  The report also
covers the same predicate written inside an instantiated template, which needs
the capture proxy of the *pattern* lambda bridged to the instantiation's.
Stock GCC never substitutes a lambda's contract specifiers at all (that is
GCC-13), so there is nothing to hang that bridge on until
`4640-contract-substitution-plumbing` builds the substitution entry point for
lambdas; the bridge and its `process_outer_var_ref` carve-out therefore stay
where they are.  What is fixed here is the shape PR126038 itself reports, which
needs no template.

## Compile gap

None.  `tsi_start`, `tsi_stmt`, `tsi_next`, `tsi_end_p` and
`LAMBDA_FUNCTION_P` are all upstream, and the two hunks rewrite one block
inside a function that already exists at the base.  `pr126038.C` passes on this
commit alone: it is the non-template shape, and nothing outside
`contracts.cc` is involved in it.

## Contents

- gcc/cp/contracts.cc : #78, #79
- gcc/testsuite/g++.dg/contracts/cpp26/pr126038.C : *
