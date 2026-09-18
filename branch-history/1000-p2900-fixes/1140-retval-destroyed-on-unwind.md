---
id: 1140-retval-destroyed-on-unwind
subject: 'c++: contracts: one return-value cleanup, covering the postcondition checks'
depends: [1130-result-binding-one-object]
regenerates: []
fixes: [gcc-44, gcc-05]
---

## Rationale

Two defects with one cause, and so one fix.

`maybe_apply_function_contracts` wraps the finished function body in an
artificial block.  `do_poplevel` pops that block's own level before calling
`maybe_splice_retval_cleanup`, so the latter sees `sk_function_parms` a second
time -- exactly the test it uses to recognise a function body, and one the
real body's closing brace has already passed.  A contract therefore made the
compiler treat one function body as two: `current_retval_sentinel` got a
second `DECL_EXPR`, which trips `gimple_add_tmp_var` and ICEs, and a second
retval `CLEANUP_STMT`, which destroys the returned object twice on the way out
(GCC-5, PR c++/127281).

Separately, a violation handler may throw, and a postcondition check runs
after the returned object has been initialised: [stmt.return]/5 sequences
postcondition evaluation after the copy-initialisation of the result and after
the destruction of the return statement's locals.  Unwinding past that point
without running the returned object's destructor leaks an object the program
can no longer reach (GCC-44, PR c++/127414).

The second visit is not a problem to be suppressed; it is the one we want.
`start_function_contracts` records that a function whose postconditions can
throw has a cleanup that might throw -- which is simply true, the checks are
emitted into the finally arm of the contracts `TRY_FINALLY` -- and asks for the
body's splice to be deferred.  The single cleanup is then spliced around the
contracts block, covering the body and the checks together.  The body throwing
before a return finds the sentinel clear and destroys nothing; a postcondition
throwing finds it set and destroys the returned object exactly once.  One
mechanism answers both defects, and there is no second cleanup to prove
non-overlapping against.

Two bits keep the two visits straight, because both see `sk_function_parms`:
`retval_sentinel_declared` holds the `DECL_EXPR` to one, and
`retval_cleanup_spliced` holds the function-body cleanup to one.  The second
is not redundant with the deferral -- a function with only preconditions, or
with postconditions that cannot throw, does not defer, so its body splices
normally and the contracts block must not splice again.  That case is the
non-ICE half of PR c++/127281 and it regressed in testing when only the
`DECL_EXPR` was guarded.  Neither bit affects a function try block, which
legitimately gets a cleanup per try.

Whether a postcondition can throw is a per-assertion question here, not a
translation-unit one: `postconditions_may_throw_p` reuses
`all_contracts_statically_nonthrowing`, so `ignore` and `assume` (never
evaluated), `quick_enforce` (terminates without calling the handler) and the
P4298 `noexcept_` variants (handler invoked under `noexcept`) cost nothing,
and a P3595 dynamic selector or an assertion-control label is treated
conservatively.  Upstream, with one global evaluation semantic and no
`noexcept_` variants, could only answer this for the whole translation unit.

Coroutines keep the older, separate wrapper.  The coroutine transform clears
`throwing_cleanup` before contracts are applied, because it manages its own
cleanups, so no sentinel is ever built for a ramp and the unified cleanup has
nothing to hang on.  `unified_retval_cleanup` therefore mirrors every
condition `maybe_splice_retval_cleanup` applies -- including
`throwing_cleanup` -- so the two agree about which of the two cleanups is
emitted.  When they disagreed in testing the ramp got neither and leaked.
Bringing coroutines onto the unified path means teaching the transform to
include the checks in the ramp's own cleanup, and is deliberately left alone.

This destroys the returned object in a case no wording currently requires:
[except.ctor]/2 covers only temporaries and local variables and does not
mention contract assertions, and [basic.contract.eval] says a throwing handler
behaves "as if the function body exits via that same exception", describing a
state where the result object was never initialised.  A core issue is open at
<https://github.com/cplusplus/cwg/issues/988>.  Leaking is not a defensible
answer; this should not be "corrected" back to a leak on the strength of the
wording alone.

## Compile gap

None.  `current_retval_sentinel`, `make_temp_override` and the sentinel
machinery are all upstream, and the two new `language_function` bits arrive
with the code that reads them.

## Contents

- gcc/cp/cp-tree.h : /defer_retval_cleanup/, /retval_sentinel_declared/, /retval_cleanup_spliced/
- gcc/cp/except.cc : @maybe_splice_retval_cleanup
- gcc/cp/contracts.cc : #53:1, #68:1, /^postconditions_may_throw_p \(tree fndecl\)$/, /^  const bool unified_retval_cleanup$/, /Hand the splice back to do_poplevel/, @wrap_postconditions_in_retval_cleanup
- gcc/testsuite/g++.dg/contracts/cpp26/contract-retval-destroyed-on-unwind.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-retval-destroyed-on-unwind-coro.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-retval-sentinel.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-retval-throwing-cleanup.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-throw-escapes-try.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-throw-noexcept-terminate.C : *
