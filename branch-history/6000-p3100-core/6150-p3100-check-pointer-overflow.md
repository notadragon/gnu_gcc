---
id: 6150-p3100-check-pointer-overflow
subject: 'c++: contracts: P3100: route the pointer-overflow check (RUC_POINTER_OVERFLOW)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 6: `-fsanitize=pointer-overflow`, the check that pointer
arithmetic does not wrap.

Pure routing: no implicit contract assertion, no front-end code, and unlike
its four predecessors it has a `.def` row of its own, so the row travels
with the check here.

The cpp26 tests pin the two codegen shapes the routing has to produce --
`enforce` (noreturn entry point) and `recover` (returning entry point, the
access proceeds) -- and the ubsan tests run all three semantics.

## Compile gap

None: the framework, the option parsing and the wire descriptor are in
`6000-p3100-core` and the runtime mapping is in
`6030-p3100-ubsan-runtime`.

The trap is the silent one the `.def` file exists to prevent, and this
commit is the first per-check commit where it is fully in view: adding
`RUC_POINTER_OVERFLOW` to only two of the three generated tables would leave
either a check that never routes or an `assume` that never suppresses the
instrumentation, with no diagnostic either way.  Generating all three from
this one row is what makes adding the row sufficient.

## Contents

- gcc/contracts-routed-checks.def : /RUC_POINTER_OVERFLOW/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-pointer-overflow-* : *
- gcc/testsuite/g++.dg/ubsan/p3100-pointer-overflow-* : *
