---
id: 1240-retval-cleanup-not-respliced
subject: "c++: contracts: don't re-splice the return-value cleanup around the contract block"
depends: []
regenerates: []
fixes: [gcc-05]
---

## Rationale

GCC-5 (PR c++/127281).  `maybe_apply_function_contracts` runs from
`finish_function` with the `sk_function_parms` level current, and wraps the
already-finished body in an artificial block.  `do_poplevel` pops that block's
own level before calling `maybe_splice_retval_cleanup`, so the latter sees
`sk_function_parms` a second time -- which is exactly the test it uses to
recognise a function body, and one the real body's closing brace has already
passed.  A contract therefore made the compiler treat one function body as two:
`current_retval_sentinel` got a second `DECL_EXPR`, and, where
`cp_function_chain->throwing_cleanup` was set, a second retval `CLEANUP_STMT`
carrying the same destructor.

Two symptoms, depending on which the function reaches first.  With a named
return value the duplicated declaration trips `gimple_add_tmp_var`'s assertion
and the compiler ICEs.  Without one there is no assertion to trip and the
duplicated cleanup simply runs, destroying the returned object twice when
something on the way out throws -- a program that compiles without a
diagnostic and is wrong.  The predicate's content is irrelevant in both; it is
never false and never fails, and `post` behaves as `pre` does.  Both need
codegen, so `-fsyntax-only` alone compiles clean, and both reproduce with plain
`-fcontracts` at `-std=c++23` -- no extension paper is involved.

The fix hides `current_retval_sentinel` across the artificial block with a
`make_temp_override`, so `maybe_splice_retval_cleanup` takes its
already-spliced early exit for it.  A contract check runs either before the
return object exists (a precondition) or after the body's own cleanup has
already dealt with it (a postcondition), so the block wants neither the
declaration nor the cleanup.  Two things a reviewer should check.  First, that
this is done in `contracts.cc` rather than by guarding a second `function_body`
splice inside `maybe_splice_retval_cleanup`: that keeps function-try-blocks and
the c++/112301 rethrow path, which have no bug, untouched.  Second, how it
reads against `1140-retval-destroyed-on-unwind`, which deliberately gives the
postcondition checks a retval cleanup of their own with no sentinel guard.  The
two are consistent only because of this override: without it the artificial
block would splice a covering cleanup as well, and the two would overlap and
destroy the returned object twice on a throwing violation handler.  They are
independent commits in either order, but the non-overlap argument in 1140
assumes this one.

## Compile gap

None.  `current_retval_sentinel` and `make_temp_override` are both upstream,
and this is a single insertion into a function that already exists at the base.

## Contents

- gcc/cp/contracts.cc : #75
- gcc/testsuite/g++.dg/contracts/cpp26/contract-retval-sentinel.C : *
