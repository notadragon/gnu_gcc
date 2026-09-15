---
id: 1140-retval-destroyed-on-unwind
subject: 'c++: contracts: destroy the returned object if a postcondition check throws'
depends: [1130-result-binding-one-object]
regenerates: []
fixes: []
---

## Rationale

A violation handler may throw, and a postcondition check runs after the
returned object has been initialised: [stmt.return]/5 sequences postcondition
evaluation after the copy-initialisation of the result and after the
destruction of the return statement's locals.  Unwinding past that point
without running the returned object's destructor leaks an object the program
can no longer reach.

`wrap_postconditions_in_retval_cleanup` wraps the postcondition checks -- and
only them -- in a cleanup that destroys the returned object on the exceptional
path.  It deliberately carries no sentinel guard, unlike
`maybe_splice_retval_cleanup`: this region is reached only on the
normal-completion path of the body, where the returned object necessarily
exists.  That is also what keeps the two cleanups from overlapping, the body's
covering the body and stopping there and this one covering only the checks, so
the object is destroyed exactly once however the function unwinds.  A reviewer
should check that non-overlap first; it is the whole correctness argument.

This is deliberately more than the standard currently requires, and the code
says so.  [except.ctor]/2 destroys the returned object only for an exception
thrown "during the destruction of temporaries or local variables for a return
statement" and does not mention contract assertions; [basic.contract.eval]
says a throwing handler behaves "as if the function body exits via that same
exception", which describes a state where the result object was never
initialised -- not the state we are actually in.  Leaking is not a defensible
answer; the gap is worth a core issue, and this behaviour should not be
reverted to a leak on the strength of the wording alone.

## Compile gap

None.  The helper and its documentation arrive together, and its caller is in
`apply_postconditions`, introduced by the dependency.

## Contents

- gcc/cp/contracts.cc : @apply_postconditions:1, @wrap_postconditions_in_retval_cleanup
- gcc/testsuite/g++.dg/contracts/cpp26/contract-retval-destroyed-on-unwind.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-retval-throwing-cleanup.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-throw-escapes-try.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-throw-noexcept-terminate.C : *
