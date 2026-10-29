---
id: 6340-p3100-check-pointer-arithmetic
subject: 'c++: contracts: P3100: implicit pointer-arithmetic checks'
depends:
  - 6000-p3100-core
  - 6220-p3100-check-bounds
regenerates: []
fixes: []
---

## Rationale

The implicit contract assertion
`ub:expr.add.out.of.bounds.known.arithmetic`: pointer arithmetic `a + i`,
`i + a` or `a - i` on an array of statically-known bound N, decayed to a
pointer to its first element, whose result leaves [0, N].  It is the
arithmetic leaf beside the subscript check of
`6220-p3100-check-bounds`, and like it skips a trailing array that may be
used as a flexible array member, with that commit's
`p3100_flexible_array_member_like_p`.

`cp_pointer_int_sum` is where every built-in pointer-plus-integer
expression is formed, `p += i` and `p[i]` on a pointer included, so the
check is built there: `p3100_decayed_array_bound` recognizes the decayed
array, `implicit_array_arith_check_p` resolves the semantic (nothing to
check for a constant offset in range, so an address constant stays a
constant initializer), and `build_implicit_array_arith_check` guards the
*result* of `pointer_int_sum` with the unmoved pointer as the defined
substitute.  Guarding the integer operand instead does not work:
`pointer_int_sum` folds the conversion and the multiplication by the
element size into the guard's arms, and the folded `COND_EXPR` loses
`COND_EXPR_IMPLICIT_UB_GUARD_P`, so constant evaluation would evaluate the
reaction.  `build_implicit_op_guard` gains the optional substitute for
this.

The second implicit assertion built there is
`ub:expr.add.out.of.bounds.pointer.arithmetic`: any pointer arithmetic
whose result wraps around the address space, tested on the byte offset in
`sizetype` as `-fsanitize=pointer-overflow` tests it, but as a front-end
guard with all seven semantics, continuing with the unmoved pointer.  It is
not the routed pointer-overflow check of `6150-p3100-check-pointer-overflow`,
which stays sanitizer instrumentation.  Its condition compares addresses as
integers, which is not a constant expression, so its guard carries
`COND_EXPR_IMPLICIT_UB_RUNTIME_P` and constant evaluation, and
`potential_constant_expression`, take the guarded arithmetic alone.  The
bound check is the outer guard, so it is tested first.

Arithmetic applied to known-bound arithmetic is checked against the same
array.  `cp_pointer_int_sum` records each known-bound result with the
pointer to the array's first element (`record_implicit_known_arith`, a
`GTY((cache))` map keyed by the result, so an entry goes away with its
key); arithmetic on a recorded result, `(a + i) + j`, is checked at the
combined position, the operand's position being its distance from the
first element (`implicit_known_arith_position`), with an unsigned offset at
least as wide as `sizetype` above the bound counted as outside so the sum
cannot wrap back into range, and continues at the first element.  An
indirection through a recorded result is checked by the past-the-end
indirection check of `6220` where `cp_build_indirect_ref_1` builds it,
before `&*` can fold it (`build_implicit_known_indirection_check`), and a
subscript of one, `(a + k)[i]`, is built as the subscript `a[k + i]` of the
array (`p3100_known_arith_subscript`), so the array-bounds walk of `6220`
reports it as it reports a subscript written on the array.  A
base with side effects of its own is saved so that it can be named again.

The wrap check covers pointer increments and decrements too: configured,
`p3100_pointer_incdec` builds `++p`, `p--` and the rest as an assignment of
the checked sum, the old value read once; unconfigured, the increment is
built as before.

## Compile gap

None: `build_implicit_op_guard` and `resolve_implicit_contract_semantic`
are in `6000-p3100-core`, and `p3100_flexible_array_member_like_p` in
`6220-p3100-check-bounds`; `COND_EXPR_IMPLICIT_UB_RUNTIME_P` and its
handling in constant evaluation are in `6000-p3100-core`, beside
`COND_EXPR_IMPLICIT_UB_GUARD_P`.

The trap is single evaluation.  The pointer and the offset are each used on
both arms of either guard, so both are `SAVE_EXPR`s, and both are forced
before the condition, whose two halves are joined without short-circuiting:
a `SAVE_EXPR` first evaluated inside one arm is read uninitialized on the
other.

## Contents

- gcc/cp/typeck.cc : @cp_pointer_int_sum, /P3100: if PTROP is an array of statically-known bound decayed to a pointer/, /build_implicit_known_indirection_check/, /P3100: the subscript AR\[IND\] where the pointer AR is known-bound pointer/, @p3100_known_arith_subscript, /p3100_known_arith_subscript \(loc/, /P3100: the pointer increment or decrement ARG\+\+/, /inc_bytes/
- gcc/cp/contracts.cc : @implicit_array_arith_offset, @implicit_array_arith_cond, @implicit_array_arith_check_p, @build_implicit_array_arith_check, @build_implicit_pointer_wrap_check, /^static GTY\(\(cache\)\) tree_cache_map \*implicit_known_arith_bases;$/, @record_implicit_known_arith, @implicit_known_arith_base, @implicit_known_arith_position, @build_implicit_known_indirection_check
- gcc/cp/contracts.h : /implicit_array_arith_check_p/, /build_implicit_array_arith_check/, /build_implicit_pointer_wrap_check/, /record_implicit_known_arith/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-ptr-wrap-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-arith-bounds-* : *
