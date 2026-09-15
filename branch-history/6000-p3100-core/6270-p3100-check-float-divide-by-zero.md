---
id: 6270-p3100-check-float-divide-by-zero
subject: 'c++: contracts: route the float-divide-by-zero check (RUC_FLOAT_DIVIDE_BY_ZERO)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 20, the last implemented row: `-fsanitize=float-divide-by-
zero`, a floating-point division by zero.

Pure routing, with three tests rather than one because this is the check the
throwing-reaction-in-a-`noexcept`-function case is pinned on: a routed
violation reported from a middle-end site cannot propagate an exception out
of a `noexcept` function, so `enforce` and `observe` have to degrade to
their non-throwing forms.  Those two tests
(`p3100-float-divide-by-zero-throw-noexcept-*`) belong with the check they
are written against rather than with the P4298 machinery they exercise.

No implicit contract assertion: floating-point division by zero is not
core-language UB (it is well-defined under IEEE 754), so the front end
synthesises nothing and there is no group id.  That asymmetry with
`6180-p3100-check-integer-divide`, where the same syntactic operator *is*
UB, is the point worth noticing here.

## Compile gap

None: the framework is in `6000-p3100-core` and the runtime mapping in
`6030-p3100-ubsan-runtime`.

The trap is the degradation.  With `-fcontracts-p4298` a routed check in a
`noexcept` function selects the `_noexcept` entry-point variants; without
it, a configuration that asks for a throwing `enforce` at such a site is
clamped, and the clamp is silent by design -- the alternative is a
diagnostic on every `noexcept` function in a translation unit that
configured the check globally.

## Contents

- gcc/contracts-routed-checks.def : /RUC_FLOAT_DIVIDE_BY_ZERO/
- gcc/testsuite/g++.dg/ubsan/p3100-float-divide-by-zero-* : *
