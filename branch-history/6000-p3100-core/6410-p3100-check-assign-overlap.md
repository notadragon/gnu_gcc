---
id: 6410-p3100-check-assign-overlap
subject: 'c++: contracts: P3100: implicit overlapping assignment check'
depends:
  - 6000-p3100-core
regenerates: []
fixes: []
---

## Rationale

The implicit contract assertion `ub:expr.assign.overlap`: a built-in
assignment whose stored value is read from an object that overlaps the
destination, unless the overlap is exact and the two objects have the same
type, which [expr.assign] makes undefined -- for instance assigning one
member of a union from a differently typed member.

`cp_build_modify_expr` asks `build_implicit_assign_overlap_check` for a
check of a simple assignment of scalar type, before the right operand's
conversion, and evaluates it before the store.  The check applies where the
right operand is, through conversions of the value read, a glvalue: it
compares the two address ranges as integers, and reports any overlap
between objects of different types and an inexact one between objects of
the same type.  An operand whose address computation has side effects is
evaluated once: its address is saved and evaluated by the check -- the
right operand's first, as it is sequenced before the left, and whenever the
left's is -- and the operand replaced with an indirection through it (the
glvalue read is found in the unconverted right operand,
`implicit_find_slot_r`).  A bit-field is not checked, nor two different
variables, which cannot overlap (`implicit_overlap_base_decl`), nor an
operand without side effects assigned to itself.

Comparing addresses as integers is not a constant expression, so the guard
is marked `COND_EXPR_IMPLICIT_UB_RUNTIME_P` and constant evaluation is
unchanged; saved addresses are evaluated outside it.  The assignment goes ahead after a violation that returns.

`p3100-constexpr-fold-runtime-check*.C` pin, for this check and the
base-init and direct pure-virtual checks before it, that a runtime-only
guard survives a constexpr call GCC would otherwise fold (the rule is in
`6000-p3100-core`), while manifestly constant evaluation is unchanged.

## Compile gap

None: `resolve_implicit_contract_semantic`, `build_implicit_violation_call`,
`build_quick_enforce_reaction` and the guard flags are in
`6000-p3100-core`.

## Contents

- gcc/cp/typeck.cc : /build_implicit_assign_overlap_check/, /overlap_check/
- gcc/cp/contracts.cc : @implicit_overlap_base_decl, @implicit_find_slot_r, @build_implicit_assign_overlap_check
- gcc/cp/contracts.h : /build_implicit_assign_overlap_check/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-assign-overlap-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-constexpr-fold-runtime-check* : *
