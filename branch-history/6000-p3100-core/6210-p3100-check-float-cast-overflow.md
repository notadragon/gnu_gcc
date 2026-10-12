---
id: 6210-p3100-check-float-cast-overflow
subject: 'c++: contracts: P3100: implicit float-to-integer conversion check (RUC_FLOAT_CAST_OVERFLOW)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 14, `-fsanitize=float-cast-overflow`, and the implicit
contract assertion `ub:conv.fpint.float.not.represented`: converting a
floating-point value to an integer type whose truncated value it cannot
represent.

The middle-end change is a refactoring in service of the front end.
`ubsan_instrument_float_cast` built the out-of-range predicate and the
sanitizer call together; the predicate is split out as
`ubsan_float_cast_overflow_predicate`, which the C++ front end can then use
to build the same condition for a contract guard.  The sanitizer's own path
is unchanged and still calls it.

`build_implicit_float_cast_check` is the front-end guard: it yields the
converted value when the conversion is in range and the configured
reaction's defined value when it is not.

## Compile gap

Its call site is a forward reference: the conversion is guarded in
`ocp_convert`, whose one hunk in `cp/cvt.cc` also contains the guard for the
enum-cast check.  That hunk travels with `6310-p3100-check-enum-cast`, the
later of the two, so that neither guard is called before it is defined.

Until then `build_implicit_float_cast_check` is defined, declared, and
uncalled.  It builds; a float-to-integer conversion is simply never guarded,
and `-fcontract-implicit-semantic=ub:conv.fpint.float.not.represented=observe`
is accepted and does nothing.  The routing half is live from this commit, so
`p3100-float-cast-overflow-route-observe.C` passes here and the
`p3100-fpint-*` tests need `6310`.

The semantic trap is in the refactoring: `ubsan_float_cast_overflow_predicate`
returns `NULL_TREE` both when the conversion can never be out of range and
when the floating mode is unsupported.  Both mean "emit no check", but only
the first is a correctness-preserving answer for a contract assertion, and
nothing distinguishes them at the call site.

## Contents

- gcc/contracts-routed-checks.def : /RUC_FLOAT_CAST_OVERFLOW/
- gcc/cp/contracts.cc : @build_implicit_float_cast_check
- gcc/cp/contracts.h : @build_implicit_float_cast_check
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-float-cast-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-fpint* : *
- gcc/testsuite/g++.dg/ubsan/p3100-float-cast-overflow-* : *
- gcc/ubsan.cc : @ubsan_use_new_style_p, @tree, @ubsan_instrument_float_cast
