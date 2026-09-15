---
id: 6250-p3100-check-vla-bound
subject: 'c++: contracts: route the vla-bound check (RUC_VLA_BOUND)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 18: `-fsanitize=vla-bound`, a variable-length array declared
with a non-positive bound.

Pure routing: a wire row and a run test.  Its own commit for the same reason
as `6240-p3100-check-unreachable` -- its own wire id, its own `-fsanitize=`
bit.

No implicit contract assertion: a VLA is a GNU extension in C++, so there is
no [expr] or [dcl] rule for the front end to synthesise an assertion from
and no group id in the implicit registry.

## Compile gap

None.

The trap is that this row occupies a wire id that Clang also fills, and
Clang's VLA support is not an extension; the id must stay at 18 whatever
either compiler does with it, which is the whole reason
`gcc/contracts-routed-checks.def` also carries rows GCC does not implement.

## Contents

- gcc/contracts-routed-checks.def : /RUC_VLA_BOUND/
- gcc/testsuite/g++.dg/ubsan/p3100-vla-bound-* : *
