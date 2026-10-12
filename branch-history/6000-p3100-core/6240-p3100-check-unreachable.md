---
id: 6240-p3100-check-unreachable
subject: 'c++: contracts: P3100: route the unreachable check (RUC_UNREACHABLE)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 17: `-fsanitize=unreachable`, reaching a
`__builtin_unreachable`.

Pure routing, and one of the three smallest checks on the branch -- a wire
row and a run test.  It gets its own commit anyway, because it has its own
wire id and its own `-fsanitize=` bit and grouping it with its neighbours
would mean a commit whose subject cannot name what it does.

There is deliberately no implicit contract assertion here.  A reached
`__builtin_unreachable` is UB the *programmer* asserted away, not UB the
language rules create at a syntactic site, so it has no group id in the
implicit registry; the only thing P3100 offers for it is to deliver the
sanitizer's report to the violation handler.  `ub:dcl.attr.assume.false`,
the nearest thing to a language-level counterpart, is a separate check with
its own commit.

## Compile gap

None: the row, the tests and the runtime mapping are all that this check
consists of, and the framework beneath them is `6000-p3100-core` and
`6030-p3100-ubsan-runtime`.

The trap is the interaction with `-funreachable-traps` and with
`-fsanitize=return`, which the C++ front end already couples: with
`-fsanitize=unreachable` off, a flowed-off function is instrumented with
`__builtin_unreachable` rather than a return check, so a configuration that
routes `unreachable` but not `return` can find the flow-off site reported
under this check's id instead.  Nothing diagnoses the swap.

## Contents

- gcc/contracts-routed-checks.def : /RUC_UNREACHABLE/
- gcc/testsuite/g++.dg/ubsan/p3100-unreachable-* : *
