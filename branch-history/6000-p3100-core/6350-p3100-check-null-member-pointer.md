---
id: 6350-p3100-check-null-member-pointer
subject: 'c++: contracts: P3100: implicit null member pointer checks'
depends:
  - 6000-p3100-core
regenerates: []
fixes: []
---

## Rationale

The implicit contract assertion `ub:expr.mptr.oper.member.func.null`:
applying a null pointer to member with `.*` or `->*`, which [expr.mptr.oper]
makes undefined.  The group has two leaves, one per kind of member pointer.

`.data` is checked where `build_m_component_ref` forms the address
`object + offset`: the null pointer to data member is the offset -1, and
the guard is on the address, so execution continuing past a violation sees
the operation unchecked.  `.function` is checked where
`get_member_function_from_ptrfunc` resolves a pointer to member function to
the function pointer a call uses: a null one has a zero `pfn`, and where the
virtual bit lives in `delta` (`ptrmemfunc_vbit_in_delta`) a zero `delta` low
bit too, since there a virtual function's `pfn` is a vtable offset that may
be zero.  A constant member pointer is not checked, nor one applied to a
dummy object (a conversion, not a call).

Both conditions test the representation of a member pointer, which is not a
constant expression, so both guards carry `COND_EXPR_IMPLICIT_UB_RUNTIME_P`
and constant evaluation is unchanged.  Nothing defined can replace the
member pointer, so the guarded value is the original one on every path, and
"ignore" emits nothing; `build_implicit_unchanged_guard` builds that shape
for this and later checks.

## Compile gap

None: `build_implicit_op_guard`, `resolve_implicit_contract_semantic` and
`COND_EXPR_IMPLICIT_UB_RUNTIME_P` are in `6000-p3100-core`.

The trap is evaluation order.  The guarded value is evaluated first, as the
guard's forced operand, and the condition reads `pfn`, `delta` or the offset
only afterwards: the member pointer may be a `TARGET_EXPR`, whose first
occurrence must come before the others.

## Contents

- gcc/cp/typeck2.cc : /#include "contracts.h"/, /ub:expr.mptr.oper.member.func.null.data/, /tree offset = NULL_TREE/, /offset = convert_to_ptrofftype/, /build_implicit_null_data_member_check/
- gcc/cp/typeck.cc : /ub:expr.mptr.oper.member.func.null.function/, /build_implicit_null_member_function_check/
- gcc/cp/contracts.cc : @build_implicit_unchanged_guard, @build_implicit_null_data_member_check, @build_implicit_null_member_function_check
- gcc/cp/contracts.h : /build_implicit_null_data_member_check/, /build_implicit_null_member_function_check/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-mptr-null-* : *
