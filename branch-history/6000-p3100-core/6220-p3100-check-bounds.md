---
id: 6220-p3100-check-bounds
subject: 'c++: contracts: P3100: implicit array-bounds check (RUC_BOUNDS)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 15, `-fsanitize=bounds`, and the implicit contract assertion
`ub:expr.add.out.of.bounds.known.subscript`: a subscript outside an array
whose size the compiler knows statically.  `p3100_flexible_array_member_like_p`
is exported for the pointer-arithmetic check of
`6340-p3100-check-pointer-arithmetic`.

Unlike the other implicit checks this one is reached through a langhook that
returns a *replacement expression* rather than a guard around a value.
`ubsan_maybe_instrument_array_ref` calls
`lang_hooks.build_implicit_bounds_check` with the subscript and the first
out-of-range index; `cp_build_implicit_bounds_check` returns an index that
yields a defined valid index (0) when the raw one is out of range, running
the reaction as a side effect.  The subscript is rewritten in place, so
`ignore` again changes codegen rather than suppressing a report.

The same hook builds a second check, `ub:expr.unary.dereference.invalid.known`
(the hook is told which by its group argument): an indirection through the
pointer past the end of such an array, which C++ does not allow even when
only the address is taken.  The subscript check allows the index N in
`&a[N]`, as C does, so `ubsan_maybe_instrument_array_ref` guards that index
against the element count as well -- after the subscript guard, so that an
index it redirected is not reported twice -- and
`ubsan_maybe_instrument_array_indirection` guards the byte offset of
`*(a + i)` against the size of the array.  A trivial copy assignment from
an element, `s = a[N]`, reads it: `build_over_call` copies from the
element itself, as the trivial copy constructor does, rather than through
the address its reference parameter binds, so the subscript is checked as
a subscript and not as an indirection past the end.

`c-gimplify.cc` opens the ARRAY_REF walk under `-fcontracts-p3100` so the
hook is reached when no sanitizer is on; the `do_sanitize` local in
`ubsan_maybe_instrument_array_ref` is what keeps the two paths apart, since
the pointer-array form is still sanitizer-only.

## Compile gap

None: the `build_implicit_bounds_check` langhook, its default
implementation and its slot in the langhook table are in
`6000-p3100-core`, as are `build_implicit_op_guard` and the group registry.

The trap is the split inside `ubsan_maybe_instrument_array_ref`.  The
function now runs under two independent conditions -- `do_sanitize` and
`flag_contracts_p3100` -- and only the true `ARRAY_REF` form is contract-
checkable; the `is_instrumentable_pointer_array_address` form remains
sanitizer-only.  A configuration that turns the bounds assertion on
therefore covers strictly less than `-fsanitize=bounds` does, silently.

## Contents

- gcc/c-family/c-gimplify.cc : *
- gcc/c-family/c-ubsan.cc : @ubsan_maybe_instrument_array_ref, @p3100_flexible_array_member_like_p, /P3100: whether ARRAY. the array operand of an ARRAY_REF/, @ubsan_maybe_instrument_array_indirection
- gcc/c-family/c-ubsan.h : /p3100_flexible_array_member_like_p/
- gcc/contracts-routed-checks.def : /RUC_BOUNDS/
- gcc/cp/contracts.cc : @cp_build_implicit_bounds_check
- gcc/cp/call.cc : /P3100: copy from an array element itself/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-array-bounds* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-invalid-known-* : *
- gcc/testsuite/g++.dg/ubsan/p3100-bounds-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-throw-bounds-divoverflow.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-throw-bounds-divoverflow.json : *
