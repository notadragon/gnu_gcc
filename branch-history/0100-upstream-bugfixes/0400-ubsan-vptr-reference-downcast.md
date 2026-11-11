---
id: 0400-ubsan-vptr-reference-downcast
subject: 'c++: check the reference form of a static_cast downcast with -fsanitize=vptr'
depends: []
regenerates: []
fixes: [gcc-734]
---

## Rationale

GCC-734 (upstream: PR116102).  Not a contracts bug.  `-fsanitize=vptr`
checked `static_cast<D *> (pb)` but never `static_cast<D &> (b)` or
`static_cast<D &&> (b)`, so a reference downcast to a class the object does
not have went unreported.  The reference path of `build_static_cast_1`
called `cp_ubsan_maybe_instrument_downcast` with the class type `B` as the
source type, which that function rejects (it wants a pointer or
reference), and with the operand still a `B *`, before the conversion to
`D *` -- too early for the check, which reads the vptr as a `D`'s.

The check is now made after `build_base_path`, on the `D *`, with `B *`
as the source type, as the pointer form does.  If `build_base_path` tested
for null (under `-fno-delete-null-pointer-checks`), only its adjusted
pointer is instrumented, so the null-reference check of
`-fsanitize=null` still finds the `COND_EXPR` it looks for.

## Compile gap

None.  Everything named is upstream's; the test uses no contracts.

## Contents

- gcc/cp/typeck.cc : /Check the dynamic type of the "D\*"/, /^      if \(sanitize_flags_p \(SANITIZE_VPTR\)\)$/
- gcc/testsuite/g++.dg/ubsan/vptr-reference-downcast.C : *
