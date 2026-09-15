---
id: 6220-p3100-check-bounds
subject: 'c++: contracts: implicit array-bounds check (RUC_BOUNDS)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 15, `-fsanitize=bounds`, and the implicit contract assertion
`ub:expr.add.out.of.bounds.known`: a subscript outside an array whose size
the compiler knows statically.

Unlike the other implicit checks this one is reached through a langhook that
returns a *replacement expression* rather than a guard around a value.
`ubsan_maybe_instrument_array_ref` calls
`lang_hooks.build_implicit_bounds_check` with the subscript and the first
out-of-range index; `cp_build_implicit_bounds_check` returns an index that
yields a defined valid index (0) when the raw one is out of range, running
the reaction as a side effect.  The subscript is rewritten in place, so
`ignore` again changes codegen rather than suppressing a report.

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
- gcc/c-family/c-ubsan.cc : @ubsan_maybe_instrument_array_ref
- gcc/contracts-routed-checks.def : /RUC_BOUNDS/
- gcc/cp/contracts.cc : @cp_build_implicit_bounds_check
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-array-bounds* : *
- gcc/testsuite/g++.dg/ubsan/p3100-bounds-* : *
