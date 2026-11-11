---
id: 6100-p3100-check-vptr
subject: 'c++: contracts: P3100: route the vptr check (RUC_VPTR)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 0: `-fsanitize=vptr`, the check that a polymorphic object's
vptr matches the type it is being used as.

The check itself is stock UBSan; what this commit adds is its wire row --
the first row of `gcc/contracts-routed-checks.def`, and therefore the row
that fixes the base of the id space shared with Clang -- and the tests that
prove the three moving parts line up: the compiler emits a wire byte for
`vptr`, the runtime maps its `ErrorType` back to id 0, and the violation is
delivered to the handler with `assertion_kind::implicit`.

The cpp26 tests are the codegen side (`assume` suppresses the
instrumentation entirely; `enforce` and `recover` select the entry-point
variants); the ubsan tests are the run side, including the LTO variant --
the routing decision has to survive streaming -- and the lazily-populated
`report()`.  `vptr-reinterpret-reference.C` pins the stock check of a
member read through a `reinterpret_cast` glvalue of an unrelated
polymorphic type, a reference variable bound to it included (a form stock
Clang misses).

`cp_ubsan_instrument_vptr_p` also declines to instrument when the check
resolves to `assume`.  The `no_sanitize` attribute that realizes `assume`
for the other routed checks is added before genericization, which is too
late for the casts instrumented while parsing: a `static_cast` downcast
and a conversion to a virtual base.

There is no front-end implementation for this check.  A vptr contract
assertion is not synthesised by the C++ front end the way a null-dereference
or divide-by-zero assertion is; the whole feature is "take the report UBSan
was going to print and hand it to the violation handler instead".

## Compile gap

None: the `.def` machinery, the `-fcontract-sanitize-semantic=vptr` parsing
and the wire descriptor are all in `6000-p3100-core`, and the runtime side
is `6030-p3100-ubsan-runtime`.

The semantic trap is the one this commit exists to close.  With the routing
framework present but this row absent, nothing fails to build: `RUC_VPTR`
simply does not exist, so `routed_checks[]` has no entry for it,
`routed_ubsan_bits[]` has no `SANITIZE_VPTR`, and `ROUTED_SANITIZER_BITS`
does not include it.  `-fcontract-sanitize-semantic=vptr=observe` is then
rejected as an unknown check name -- the one loud failure -- but a
configuration that names a *group* containing vptr silently leaves vptr
unrouted, and `assume` silently fails to suppress the instrumentation.  The
`.def` file exists precisely so those two cannot be separate hand-written
lists that disagree without a diagnostic.

## Contents

- gcc/contracts-routed-checks.def : /RUC_VPTR/
- gcc/cp/cp-ubsan.cc : /^#include "opts.h"$/, /resolved_sanitizer_semantic \(SANITIZE_VPTR\)/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-vptr-* : *
- gcc/testsuite/g++.dg/ubsan/p3100-vptr-* : *
- gcc/testsuite/g++.dg/ubsan/open-bug-vptr-base-member.C : *
- gcc/testsuite/g++.dg/ubsan/vptr-reinterpret-reference.C : *
