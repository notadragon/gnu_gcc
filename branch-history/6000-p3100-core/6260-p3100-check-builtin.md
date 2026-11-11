---
id: 6260-p3100-check-builtin
subject: 'c++: contracts: P3100: route the builtin check (RUC_BUILTIN)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 19: `-fsanitize=builtin`, a builtin called with an argument
its definition makes undefined -- `__builtin_clz (0)` and its relatives.

Pure routing: a wire row and a run test, its own commit for its own wire id.

No implicit contract assertion.  These are library-level preconditions on
GCC builtins, not core-language UB with a [basic]/[expr] citation, so they
have no group id in the implicit registry; a future paper that gives the
standard library contract assertions would express them there rather than
here.

## Compile gap

None.

The trap is a naming one: `-fsanitize=builtin` covers a *set* of builtins
that grows between releases, so the routed id 19 does not name a fixed
predicate the way the other twenty do.  A program's routed-violation output
can therefore change across compiler versions without any configuration
change.

## Contents

- gcc/contracts-routed-checks.def : /RUC_BUILTIN/
- gcc/testsuite/g++.dg/ubsan/p3100-builtin-* : *
