---
id: 6180-p3100-check-integer-divide
subject: 'c++: contracts: implicit integer divide-by-zero and divide-overflow checks (RUC_INTEGER_DIVIDE_BY_ZERO)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
  - 6170-p3100-check-shift
regenerates: []
fixes: []
---

## Rationale

Routed-check id 10, `-fsanitize=integer-divide-by-zero`, and the two
implicit contract assertions on integer division:
`ub:expr.mul.div.by.zero.int` for a zero divisor and
`ub:expr.mul.representable.type.result` for the one signed quotient that is
not representable, `INT_MIN / -1`.

The two assertions are one commit because they guard one operator, at one
site, in one order: `build_implicit_divide_overflow_check` is applied first
(inner) and `build_implicit_divide_check` second (outer), so the
divide-by-zero condition is tested first at run time and the overflow guard
only sees a non-zero divisor.  They are separately configurable -- two group
ids, two semantics, and `p3100-div-overflow-independent.C` proves they
resolve independently -- but they are not separately implementable.

This commit also carries `cp_build_binary_op`, which is where all three of
the binary-operator guards are wired in.  Its three hunks resolve the
divide, divide-overflow and shift semantics up front (so that the status-quo
`assume` leaves the operands untouched), widen the existing
`sanitize_flags_p` gate to include them, and apply the guards; each hunk
names all three checks, so none of them can be split off for
`6170-p3100-check-shift`.  Placing them here, in the later of the two
commits, keeps the caller behind the callee.

## Compile gap

None: `build_implicit_shift_check` arrives in `6170-p3100-check-shift`,
`build_implicit_op_guard` and `resolve_implicit_contract_semantic` in
`6000-p3100-core`.  This commit is where the shift guard acquires its
caller, which is why it depends on `6170` rather than merely following it.

Two semantic traps a reviewer should look for and which no identifier
shows.  First, the guards are skipped inside an unevaluated operand
(`cp_unevaluated_operand`): a P3100 implicit assertion must not change the
result of the `noexcept` operator, whatever semantic is configured for it,
and there is no code generated for the operand anyway.  Nothing enforces
that gate but its own condition, and getting it wrong changes overload
resolution rather than producing a diagnostic.  Second, `ignore` for
divide-by-zero is a codegen change, not a suppressed report: the expression
must yield a defined value without executing the trapping division, so the
guard remains and only the reaction is dropped.

## Contents

- gcc/contracts-routed-checks.def : /RUC_INTEGER_DIVIDE_BY_ZERO/
- gcc/cp/contracts.cc : @build_implicit_divide_check, @build_implicit_divide_overflow_check
- gcc/cp/contracts.h : @build_implicit_divide_check, @build_implicit_divide_overflow_check
- gcc/cp/typeck.cc : @along, @cp_build_binary_op
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-div-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-mod-by-zero-* : *
- gcc/testsuite/g++.dg/ubsan/p3100-integer-divide-by-zero-* : *
