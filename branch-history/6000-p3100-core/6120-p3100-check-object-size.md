---
id: 6120-p3100-check-object-size
subject: 'c++: contracts: route the object-size check (RUC_OBJECT_SIZE)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 3: `-fsanitize=object-size`, the check that an access stays
within the object the compiler can prove the pointer refers to.

Pure routing.  There is no implicit contract assertion for object-size: the
front end synthesises nothing, and the check remains exactly the middle-end
instrumentation `-fsanitize=object-size` has always emitted.  What the tests
cover is that the wire byte reaches the runtime and that the report is
delivered to the violation handler, under `enforce` and under `observe`, and
that `assume` removes the instrumentation.

Its `.def` row is not here.  `RUC_OBJECT_SIZE` sits in the middle of a
four-row run (`RUC_ALIGNMENT`, `RUC_OBJECT_SIZE`, `RUC_NONNULL_ATTRIBUTE`,
`RUC_RETURNS_NONNULL_ATTRIBUTE`) that the net diff emits as one hunk -- the
last of them spans two source lines, which is what stops the histogram
algorithm splitting the run -- so all four rows stay in `6000-p3100-core`
with the rest of the table's indivisible text.  The check is still its own
commit because its wire slot, its `-fsanitize=` bit and its tests are
distinct from its neighbours'.

## Compile gap

None: the row that gives this check its id, the `-fcontract-sanitize-
semantic=object-size` parsing and the wire descriptor are in
`6000-p3100-core`; the runtime mapping is in `6030-p3100-ubsan-runtime`.

Because the `.def` row arrives before this commit rather than with it, the
usual trap is inverted: routing for object-size is already live in the core,
and this commit adds only the tests that prove it.  Dropping this commit
therefore loses coverage, not function.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/p3100-object-size-* : *
- gcc/testsuite/g++.dg/ubsan/p3100-object-size-* : *
