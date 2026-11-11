---
id: 6320-p3100-check-coroutine-flow-off
subject: 'c++: contracts: P3100: implicit coroutine flow-off-the-end check'
depends:
  - 6000-p3100-core
  - 6230-p3100-check-flow-off
regenerates: []
fixes: []
---

## Rationale

The implicit contract assertion `ub:stmt.return.coroutine.flow.off`: control
flowing off the end of a coroutine whose promise type has no usable
`return_void`.

Separate from `6230-p3100-check-flow-off` even though the two are the same
sentence of [stmt.return] applied to two kinds of function, because the
implementations share nothing.  A coroutine's fall-off point is not in the
function the user wrote: it is in the ramp/actor split the coroutine
transform builds, so the guard is added by
`coro_maybe_add_flow_off_check` from `wrap_original_function_body`, at the
point where the transform has already decided there is no `return_void` to
call.  It has its own group id, so a configuration can check ordinary
functions and not coroutines, or the reverse.

The dependency on `6230-p3100-check-flow-off` is ordering, not linkage: the
two guards are built by different functions, but reviewing this one without
having read the ordinary flow-off check first is pointless, and the tests
here are written as the coroutine variants of the tests there.

## Compile gap

None: `build_implicit_coroutine_flow_off_check` is here with its
declaration and its only caller, and `build_implicit_op_guard` and
`resolve_implicit_contract_semantic` are in `6000-p3100-core`.

The trap is where the reaction runs.  The guard is emitted inside the actor
function, after the coroutine body and before final suspend, so a throwing
reaction propagates into the promise's `unhandled_exception` rather than out
to the coroutine's caller -- the caller has long since resumed.  That is
correct and unavoidable, and it is why `p3100-coro-flow-off-throw.C` exists;
it is also why `p3100-noexcept-throw-terminate-coro.C`, which is about the
interaction with P4298 rather than about this check, stays in
`6000-p3100-core`.

## Contents

- gcc/cp/contracts.cc : @build_implicit_coroutine_flow_off_check
- gcc/cp/contracts.h : @build_implicit_coroutine_flow_off_check
- gcc/cp/coroutines.cc : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-coro-flow-off* : *
