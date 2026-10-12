---
id: 0190-ubsan-null-ptrmem-object
subject: 'c++: check the object pointer of ->* with a pointer to data member'
depends: []
regenerates: []
fixes: [gcc-569]
---

## Rationale

GCC-569.  Not a contracts bug.  `-fsanitize=null` never tested the object
pointer of `np->*pm` when `pm` is a pointer to data member, so a null `np`
faulted with no report.  `build_m_component_ref` lowers the access to an
indirection through `np + pm`, with the offset already added, and the null
check at gimplification tests that address, which is not null.

`build_m_component_ref` now checks the object pointer while it is still an
operand, before the offset is added, through a new
`ubsan_maybe_instrument_ptrmem_object` in `c-ubsan.cc` that reuses the
member-call path with `UBSAN_MEMBER_ACCESS` ("member access within null
pointer of type 'S'").  An object named directly (`obj.*pm`) has a non-null
address and is not instrumented.

## Compile gap

None.  The helper uses only `ubsan_maybe_instrument_reference_or_call`, which
upstream has with the same signature; the call site is upstream's
`build_m_component_ref`.

## Contents

- gcc/c-family/c-ubsan.cc : /ubsan_maybe_instrument_ptrmem_object/
- gcc/c-family/c-ubsan.h : /ubsan_maybe_instrument_ptrmem_object/
- gcc/cp/typeck2.cc : /c-family\/c-ubsan.h/, /Check the object pointer for null/
- gcc/testsuite/g++.dg/ubsan/null-ptrmem-object.C : *
