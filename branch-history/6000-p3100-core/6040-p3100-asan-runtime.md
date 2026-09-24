---
id: 6040-p3100-asan-runtime
subject: 'sanitizer: contracts: P3100: route ASan reports to the violation handler'
depends:
  - 6000-p3100-core
regenerates: []
fixes: []
---

## Rationale

The AddressSanitizer half of the routing wire, and the only sanitizer whose
routing is visible outside its own runtime.

`asan_report.cpp` gains the wire byte (`__asan_contract_semantic`), the
per-check descriptor and check enumeration, the
`__cxa_contract_violation_sanitizer` entry point, the report populator, and
the `ScopedInErrorReport` / `SuppressErrorReport` changes that divert a
report into the contract-violation handler.  It also gains
`__asan_set_error_report_callback` handling, because a program that installs
its own report callback would otherwise interpose on the path contract
enforcement drives.

`sanitizer_common/sanitizer_termination.cpp` is placed here, not in the
shared core, and not shared between the three sanitizers, because despite
its directory it is ASan-specific: it reads `__asan_contract_semantic`
through a weak reference and its diagnostic says "AddressSanitizer".  It is
the death-callback guardrail for the same interposition problem
`__asan_set_error_report_callback` solves, its opt-out is the same
`-fsanitize-noncontract-callbacks`, and its two tests
(`p3100-guardrail-optout.C`, `p3100-guardrail-report-callback.C`) are in the
ASan test directory.  Putting it in the core would make the core reference a
symbol no commit has yet defined; putting it in UBSan or TSan would be
simply wrong.

ASan has no rows in `gcc/contracts-routed-checks.def` -- its checks are
addressed by `-fsanitize=` bit, not by a routed-check id -- so there are no
per-check commits below it and its whole test directory travels here.

## Compile gap

None: `__asan_contract_semantic` is emitted by `cp/decl2.cc` in
`6000-p3100-core`, and the `-fcontract-sanitize-semantic=` parsing that sets
it is there too.

The semantic trap is the weak reference in `sanitizer_termination.cpp`:
`&__asan_contract_semantic == nullptr` is the deliberate test for "the ASan
runtime is not linked".  Compiled with a compiler that resolves the address
of a weak symbol at compile time it would fold to false; it is written this
way because the sanitizer runtimes are built with the flags that keep it a
run-time test.  A build of this commit alone links and runs stock: with no
routing configured the wire byte is 0 and every path here is the byte-for-
byte stock behaviour.

## Contents

- gcc/testsuite/g++.dg/asan/p3100-* : *
- libsanitizer/asan/asan_report.cpp : *
- libsanitizer/sanitizer_common/sanitizer_termination.cpp : *
