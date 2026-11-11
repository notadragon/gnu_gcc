---
id: 0370-noexcept-array-filler
subject: 'c++: noexcept of an initializer list with a throwing array filler'
depends: []
regenerates: []
fixes: [gcc-682]
---

## Rationale

GCC-682.  The elements of an array that an initializer list does not
initialize explicitly are initialized by its array filler, so a
potentially-throwing default constructor there makes the list
potentially-throwing ([except.spec]/6).  `noexcept (A{})` was nevertheless
true for `struct A { S s[2]; }` with a throwing `S::S ()`, and so were
`noexcept (A{{}})` and `noexcept (S[2]{})`.

When the trailing elements' initialization is not constant,
`process_init_constructor_array` leaves it to a `VEC_INIT_EXPR`, whose
element initialization is built only when it is expanded at
genericization; `check_noexcept_r` looks for calls and found none.  It now
walks, for a `VEC_INIT_EXPR`, the element initialization
`build_vec_init_expr` computed for it, which the new
`vec_init_expr_elt_init` rebuilds.

## Compile gap

None.  Everything named is upstream's; the test uses no contracts.

## Contents

- gcc/cp/except.cc : /The element initialization, which may call a potentially-throwing/
- gcc/cp/tree.cc : /^vec_init_expr_elt_init \(tree vec_init, tsubst_flags_t complain\)$/
- gcc/cp/cp-tree.h : /^extern tree vec_init_expr_elt_init/
- gcc/testsuite/g++.dg/cpp0x/noexcept-array-filler.C : *
