---
id: 6360-p3100-check-placement-new-null
subject: 'c++: contracts: P3100: implicit placement-new null check'
depends:
  - 6350-p3100-check-null-member-pointer
regenerates: []
fixes: []
---

## Rationale

The implicit contract assertion `ub:expr.new.non.allocating.null`: a
new-expression whose allocation function is a non-allocating form of
`operator new` (`std_placement_new_fn_p`, the `void *` placement forms) and
whose pointer argument is null, which [expr.new] makes undefined.  GCC does
not test that result for null, since DR 1748, so a null argument goes on to
initialize an object at address zero.

The check is in `build_new_1`, on the result of the allocation call -- the
argument itself, returned unchanged -- before the new-expression does
anything with it, so it covers the forms that store nothing as well.  A
class's own placement `operator new` and the nothrow form are not
non-allocating forms and are not checked.

The null test is a constant expression, so a null pointer is diagnosed in
constant evaluation as an operation with undefined behavior, as for the
other front-end guards; a placement new on a valid pointer stays a constant
expression.  Nothing defined replaces the pointer: execution continuing past
a violation goes on with it, and "ignore" emits nothing.

## Compile gap

None: `build_implicit_unchanged_guard` is in
`6350-p3100-check-null-member-pointer`.

The guard wraps `alloc_expr` while `alloc_call` keeps pointing at the
`CALL_EXPR` inside it, which `stabilize_call` rewrites in place: the
placement arguments' initializers are prepended to the guarded expression,
so they are still evaluated first.

## Contents

- gcc/cp/init.cc : /#include "contracts.h"/, /ub:expr.new.non.allocating.null/
- gcc/cp/contracts.cc : @build_implicit_placement_new_check
- gcc/cp/contracts.h : /build_implicit_placement_new_check/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-placement-null-* : *
