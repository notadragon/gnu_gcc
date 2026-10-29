---
id: 6380-p3100-check-noreturn
subject: 'c++: contracts: P3100: implicit [[noreturn]] return check'
depends:
  - 6000-p3100-core
regenerates: []
fixes: []
---

## Rationale

The implicit contract assertion `ub:dcl.attr.noreturn.eventually.returns`:
a function declared `[[noreturn]]` (or noreturn by any other means) that
returns to its caller, which [dcl.attr.noreturn] makes undefined.

The check is in the callee, so it covers a call through a function pointer
as well as a direct call -- UBSan's call-site `unreachable` check sees only
the latter.  `apply_implicit_noreturn_check`, called from `cp_genericize`
after the flow-off check, wraps the body of a noreturn function in a
`TRY_FINALLY_EXPR` whose finally clause is the normal-path half of an
`EH_ELSE_EXPR`, as for postconditions: every return statement and the
fall-off reach it, after the function's locals are destroyed, and an
exception leaving the function does not.  Coroutines are not instrumented.

`build_implicit_noreturn_reaction` builds the reaction.  The caller was
compiled assuming the call does not return, so there is no valid
continuation: "observe" and "noexcept_observe" call the handler and then
abort, as the pure-virtual termini do, and "ignore" keeps today's behaviour.
A throwing handler unwinds into the caller.  The check reports the group
itself: D4277R1 splits it by how the function was called, which the callee
cannot tell.  Constant evaluation uses the body saved before
genericization, so it is unchanged.

## Compile gap

None: `resolve_implicit_contract_semantic`, `build_implicit_violation_call`
and `build_quick_enforce_reaction` are in `6000-p3100-core`.

## Contents

- gcc/cp/cp-gimplify.cc : /apply_implicit_noreturn_check/
- gcc/cp/contracts.cc : @build_implicit_noreturn_reaction, @apply_implicit_noreturn_check
- gcc/cp/contracts.h : /apply_implicit_noreturn_check/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-noreturn-* : *
