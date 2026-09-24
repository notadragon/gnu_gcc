---
id: 6230-p3100-check-flow-off
subject: 'c++: contracts: P3100: implicit flow-off-the-end check (RUC_RETURN)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 16, `-fsanitize=return`, and the implicit contract assertion
`ub:stmt.return.flow.off`: control reaching the closing brace of a
value-returning function.

This is the largest of the front-end implicit checks, and the only one that
had to restructure existing code rather than add beside it.
`cp_maybe_instrument_return` used to fold "find the last statement" into
"decide whether to instrument"; the search is split out as
`cp_last_body_stmt` and run first, unconditionally, because the P3100 path
must act on a fall-off point even at `-O0` where the legacy gate returns
early.

`find_function_try_block` is the new part with no counterpart in the
sanitizer.  For a function-try-block, control reaches the end of the
function in two places -- off the end of the try-block body, where the
handlers can still catch, and off the end of the whole construct -- so the
guard is emitted twice, inside and after.  The sanitizer never needed the
distinction because its reaction cannot throw; a contract reaction can, and
which handler catches it is observable.  That is what the
`p3100-flow-off-fntryblock-*` tests pin.

The semantic is resolved once and reused for both guards, deliberately: it
is resolved by a call that can emit a configuration-error diagnostic, and
resolving twice would emit it twice.

## Compile gap

None: `build_implicit_flow_off_check` is here together with its declaration
and its only callers; `resolve_implicit_contract_semantic` and
`build_implicit_op_guard` are in `6000-p3100-core`.

The trap is the ordering against the legacy path.  `assume` must fall
through to the pre-existing behaviour and leave codegen byte-for-byte
identical, which means the P3100 block has to sit after the fall-off
detection but before the `-fsanitize=return` /
`__builtin_unreachable` gate -- and the fall-off detection had to move
above that gate to make it possible.  Get the order wrong and the failure is
not a build error but a `-O0` build in which the check silently never fires,
or an `assume` build whose codegen has changed.

## Contents

- gcc/contracts-routed-checks.def : /RUC_RETURN, SANITIZE_RETURN/
- gcc/cp/contracts.cc : @build_implicit_flow_off_check
- gcc/cp/contracts.h : @build_implicit_flow_off_check
- gcc/cp/cp-gimplify.cc : @cp_genericize_tree, @cp_maybe_instrument_return, @find_function_try_block
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-flow-off-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-implicit-flow-off-* : *
- gcc/testsuite/g++.dg/ubsan/p3100-return-* : *
