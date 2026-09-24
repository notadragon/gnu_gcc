---
id: 6050-p3100-tsan-runtime
subject: 'sanitizer: contracts: P3100: route TSan reports to the violation handler'
depends:
  - 6000-p3100-core
regenerates: []
fixes: []
---

## Rationale

The ThreadSanitizer half of the routing wire: the wire byte
(`__tsan_contract_semantic`), the semantic descriptor, the
`__cxa_contract_violation_sanitizer` entry point, the report populator, and
the `OutputReport` hook that diverts a data-race report into the
contract-violation handler.

Like ASan, TSan has no rows in `gcc/contracts-routed-checks.def` -- the
whole sanitizer is one addressable check (`SANITIZE_THREAD`) -- so there are
no per-check commits below it and its five tests travel here.

## Compile gap

None: the wire byte and its descriptor are emitted by `cp/decl2.cc` in
`6000-p3100-core`, and the `assume` suppression that stops TSan
instrumenting a function at all is in `cp/cp-gimplify.cc`, also in the core.

The trap is that TSan's report path is reached from a signal-unsafe context
and the contract handler it now calls is ordinary C++; the routed path is
only taken when the wire byte is non-zero, so a program that does not
configure TSan routing is unaffected.

## Contents

- gcc/testsuite/g++.dg/tsan/p3100-* : *
- libsanitizer/tsan/tsan_rtl_report.cpp : *
