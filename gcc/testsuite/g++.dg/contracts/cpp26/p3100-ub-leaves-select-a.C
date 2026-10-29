// P3100: each implicit assertion of DECISIONS.md E125 (a) reports its
// leaf, which is configured on its own.  Here one leaf of each group is
// observed (noexcept_observe) -- ub:expr.mul.div.by.zero.int.div,
// ub:expr.mul.representable.type.result.int.mod,
// ub:expr.shift.neg.and.width.shl, ub:expr.expr.eval.signed.integer.add
// and .mul, ub:conv.lval.valid.representation.bool -- and the others keep
// the default "assume" (p3100-ub-leaves-select-b*.C swaps them).  The
// operations are in p3100-ub-leaves.h.  At -O0.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.div:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.mod:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shl:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.add:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.mul:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.bool:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-ub-leaves.h"

// { dg-output "div_zero: R6 =0(\n|\r\n|\r)div_zero_assign: R6 =0(\n|\r\n|\r)mod_zero: fpe(\n|\r\n|\r)" }
// { dg-output "div_repr: fpe(\n|\r\n|\r)mod_repr: R6 =0(\n|\r\n|\r)shl: R6 =0(\n|\r\n|\r)" }
// { dg-output "shl_neg: R6 =0(\n|\r\n|\r)shr: =-?\[0-9\]+(\n|\r\n|\r)add: R6 =-2147483648(\n|\r\n|\r)" }
// { dg-output "inc: R6 =-2147483648(\n|\r\n|\r)sub: =-?\[0-9\]+(\n|\r\n|\r)sub_const: =-?\[0-9\]+(\n|\r\n|\r)" }
// { dg-output "dec: =-?\[0-9\]+(\n|\r\n|\r)mul: R6 =-2(\n|\r\n|\r)mul_assign: R6 =-2(\n|\r\n|\r)" }
// { dg-output "negate: =-?\[0-9\]+(\n|\r\n|\r)load_bool: R6 =0(\n|\r\n|\r)load_enum: =-?\[0-9\]+(\n|\r\n|\r)" }
