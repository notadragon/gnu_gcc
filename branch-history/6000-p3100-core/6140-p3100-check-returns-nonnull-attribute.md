---
id: 6140-p3100-check-returns-nonnull-attribute
subject: 'c++: contracts: P3100: route the returns-nonnull-attribute check (RUC_RETURNS_NONNULL_ATTRIBUTE)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 5: `-fsanitize=returns-nonnull-attribute`, the check that a
function marked `__attribute__((returns_nonnull))` does not return null.

Pure routing, and the postcondition counterpart of
`6130-p3100-check-nonnull-attribute`: same shape, opposite direction, its
own wire slot and its own `-fsanitize=` bit, so its own commit.  No implicit
assertion and no front-end code.

Its `.def` row stays in `6000-p3100-core`; see
`6120-p3100-check-object-size`.

## Compile gap

None.  The wire row and the option parsing are in the core and the runtime
mapping is in `6030-p3100-ubsan-runtime`; this commit is the tests.

Note that the routed violation is reported at the `return`, inside the
callee, and is therefore attributed to the callee's contract configuration
-- not the caller's, which is where a reader expecting postcondition
semantics would look.  Nothing diagnoses a configuration that assumed
otherwise; `p3100-returns-nonnull-attribute-enforce-codegen.C` pins where
the check lands.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/p3100-returns-nonnull-attribute-* : *
- gcc/testsuite/g++.dg/ubsan/p3100-returns-nonnull-attribute-* : *
