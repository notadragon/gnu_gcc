---
id: 6370-p3100-check-placement-new-alignment
subject: 'c++: contracts: P3100: implicit placement-new alignment check'
depends:
  - 6110-p3100-check-null-and-alignment
  - 6360-p3100-check-placement-new-null
regenerates: []
fixes: []
---

## Rationale

The `.nostore` leaf of `ub:basic.align.object.alignment`: creating an object
in storage not suitably aligned for it, where the creation stores nothing.
The routed alignment check of `6110-p3100-check-null-and-alignment` is at a
typed access (the `.store` leaf), so a placement new that performs no
initialization -- `new (p) int`, `new (p) S` for a trivially
default-constructible `S` -- or creates an empty class is never checked
there.

This checks the placement target pointer itself, in `build_new_1`, beside
the null check of `6360-p3100-check-placement-new-null` and after it, for a
non-allocating placement new whose new-expression stores nothing: no
initialization is performed, or the type is an empty class (an empty class
has nothing a constructor could store to).  Every other initialization
stores to the object and is left to the `.store` check, so a misaligned
placement new is reported once.  An array new with a cookie is not checked.

The test compares the address as an integer, which is not a constant
expression, so the guard carries `COND_EXPR_IMPLICIT_UB_RUNTIME_P` and
constant evaluation is unchanged.  Nothing defined replaces the pointer, and
"ignore" emits nothing.

## Compile gap

None: `build_implicit_unchanged_guard` is in
`6350-p3100-check-null-member-pointer`, and the null check this one follows
in `6360-p3100-check-placement-new-null`.

## Contents

- gcc/cp/init.cc : /ub:basic.align.object.alignment.nostore/
- gcc/cp/contracts.cc : @build_implicit_placement_new_align_check
- gcc/cp/contracts.h : /build_implicit_placement_new_align_check/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-placement-align-* : *
- gcc/testsuite/g++.dg/ubsan/open-bug-placement-new-align-nostore.C : *
