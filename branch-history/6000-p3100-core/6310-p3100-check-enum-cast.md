---
id: 6310-p3100-check-enum-cast
subject: 'c++: contracts: implicit enum-cast-out-of-range check'
depends:
  - 6000-p3100-core
  - 6210-p3100-check-float-cast-overflow
regenerates: []
fixes: []
---

## Rationale

The implicit contract assertion `ub:expr.static.cast.enum.outside.range`:
converting an integer or enumeration value to an unscoped enumeration type
with no fixed underlying type, when the value is outside that
enumeration's range.  [expr.static.cast] makes it undefined; UBSan has no
check for it, so there is no wire row and nothing to route.

It is distinct from `6200-p3100-check-invalid-value` and the distinction is
easy to lose: that check is about *reading* an object whose stored bits are
not a valid enumerator, this one is about *producing* an out-of-range value
by conversion.  Different clause, different group id, different tests.  An
enumeration with a fixed underlying type is excluded because the conversion
goes through the underlying type and is not undefined.

This commit also carries `ocp_convert`, and with it the call site of
`build_implicit_float_cast_check` from `6210-p3100-check-float-cast-
overflow`.  Both conversion guards are added by one hunk in the net diff --
consecutive `if` blocks in one function with no unchanged line between them
-- so they cannot be separated.  Placing them in the later of the two
commits keeps each guard's caller behind its callee, which is why this
commit depends on `6210`.

## Compile gap

None, by construction: `build_implicit_float_cast_check` arrives in
`6210-p3100-check-float-cast-overflow` and this commit is where it acquires
its caller, so nothing here forward-references anything.  This is also the
commit at which the `p3100-fpint-*` tests, which have been present since
`6210`, start to pass.

The semantic trap is shared by both guards in `ocp_convert` and is invisible
in the identifiers: both are skipped in an unevaluated operand
(`cp_unevaluated_operand`), because an implicit assertion must not change
the result of the `noexcept` operator.  The same gate appears in
`cp_build_binary_op` for the divide and shift guards; the three are
independent copies of one rule, and nothing checks that they agree.

## Contents

- gcc/cp/contracts.cc : @build_implicit_enum_cast_check
- gcc/cp/contracts.h : @build_implicit_enum_cast_check
- gcc/cp/cvt.cc : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-enum-cast-* : *
