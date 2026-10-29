// P3100: each implicit assertion of DECISIONS.md E125 (a) reports its
// leaf, which is configured on its own.  Here one leaf of each group is
// observed (noexcept_observe) -- ub:expr.mul.div.by.zero.int.mod,
// ub:expr.mul.representable.type.result.int.div,
// ub:expr.shift.neg.and.width.shr, ub:expr.expr.eval.signed.integer.sub
// and .negate, ub:conv.lval.valid.representation.enum -- and the others
// keep the default "assume" (p3100-ub-leaves-select-a*.C swaps them).
// The operations are in p3100-ub-leaves.h.  At -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.mod:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.div:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shr:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.sub:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.negate:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.enum:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-ub-leaves.h"

// { dg-output "div_zero: fpe(\n|\r\n|\r)div_zero_assign: fpe(\n|\r\n|\r)mod_zero: R6 =0(\n|\r\n|\r)" }
// { dg-output "div_repr: R6 =0(\n|\r\n|\r)mod_repr: fpe(\n|\r\n|\r)shl: =-?\[0-9\]+(\n|\r\n|\r)" }
// { dg-output "shl_neg: =-?\[0-9\]+(\n|\r\n|\r)shr: R6 =0(\n|\r\n|\r)add: =-?\[0-9\]+(\n|\r\n|\r)" }
// { dg-output "inc: =-?\[0-9\]+(\n|\r\n|\r)sub: R6 =2147483647(\n|\r\n|\r)sub_const: R6 =2147483647(\n|\r\n|\r)" }
// { dg-output "dec: R6 =2147483647(\n|\r\n|\r)mul: =-?\[0-9\]+(\n|\r\n|\r)mul_assign: =-?\[0-9\]+(\n|\r\n|\r)" }
// { dg-output "negate: R6 =-2147483648(\n|\r\n|\r)load_bool: =-?\[0-9\]+(\n|\r\n|\r)load_enum: R6 =0(\n|\r\n|\r)" }
