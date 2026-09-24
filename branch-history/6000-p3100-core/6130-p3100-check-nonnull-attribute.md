---
id: 6130-p3100-check-nonnull-attribute
subject: 'c++: contracts: P3100: route the nonnull-attribute check (RUC_NONNULL_ATTRIBUTE)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 4: `-fsanitize=nonnull-attribute`, the check that an
argument passed for a parameter marked `__attribute__((nonnull))` is not
null.

Pure routing, and deliberately distinct from `RUC_NULL`: a null argument to
a `nonnull` parameter is a violated *declared* precondition, not a
dereference, and it is checked at the call boundary rather than at an
access.  `pass_ubsan` reflects that separation directly -- call arguments
are instrumented only when the sanitizer is on, never for a P3100
dereference assertion -- so this check has no implicit-assertion side and no
front-end code of its own.

Its `.def` row stays in `6000-p3100-core`; see
`6120-p3100-check-object-size` for why those four rows cannot be split.

## Compile gap

None.  As with object-size, the wire row is already in the core, so this
commit adds tests over live function rather than turning the check on.

The semantic trap is the boundary with `RUC_NULL`: a configuration that
routes `null` but not `nonnull-attribute` still reports a null argument
through the stock UBSan path, with no diagnostic saying the two are separate
checks.  `p3100-nonnull-attribute-enforce-codegen.C` pins the codegen so
that stays visible.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/p3100-nonnull-attribute-* : *
- gcc/testsuite/g++.dg/ubsan/p3100-nonnull-attribute-* : *
