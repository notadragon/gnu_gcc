---
id: 6030-p3100-ubsan-runtime
subject: 'sanitizer: contracts: route UBSan reports to the violation handler'
depends:
  - 6000-p3100-core
regenerates: []
fixes: []
---

## Rationale

The UndefinedBehaviorSanitizer half of the routing wire.  `ubsan_diag.cpp`
gains the runtime's own copy of the routed-check id space -- the `RUC_*`
enumeration and `ErrorTypeToRoutedId`, which maps UBSan's internal
`ErrorType` onto the id that `gcc/contracts-routed-checks.def` assigns --
plus the per-check wire byte reader (`__ubsan_contract_semantic`), the
`__cxa_contract_violation_sanitizer` entry point, the lazily-populated
`report()` producer (`ContractCaptureBegin` / `ContractCaptureEnd` /
`ubsan_contract_report_populate`), and the `ScopedReport` hook that diverts
a report into the contract-violation handler instead of printing it.

This is the piece every routed UBSan check funnels through, and it is not
divisible per check: `ErrorTypeToRoutedId` is one switch over the whole
`ErrorType` enumeration, emitted by the net diff as a single hunk.  It is
therefore a sanitizer-level commit, and each check's commit below carries
only its wire row and its tests.

`ubsan_diag.h` adds the one field `ScopedReport` needs to carry the check
identity to the hook.

## Compile gap

None on the compiler side: the wire descriptor the runtime reads is emitted
by `6000-p3100-core` (`cp/decl2.cc`), and this commit only reads it.  The
semantic trap is the other direction: with the runtime present but a check's
`.def` row absent, the compiler emits no wire byte for that check, the
runtime reads zero, and the check reports through the stock UBSan path.
That is a SILENT non-routing -- no diagnostic on either side -- which is
exactly why the `.def` file is the single source the three compiler-side
tables are generated from.

No tests land here.  Every `g++.dg/ubsan/p3100-*` test names one routed
check and travels with that check's commit, so this commit is exercised
first by `6100-p3100-check-vptr`.

## Contents

- libsanitizer/ubsan/ubsan_diag.cpp : *
- libsanitizer/ubsan/ubsan_diag.h : *
