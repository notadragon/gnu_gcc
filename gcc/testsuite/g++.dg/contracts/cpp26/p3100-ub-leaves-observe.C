// P3100: every leaf of DECISIONS.md E125 (a), configured by its own name,
// under "observe": the handler runs and the operation yields its
// substitute; the checks of signed overflow and invalid values, made by
// the middle end, do not support a throwing handler and report under
// noexcept_observe, the best fit (-fcontracts-p4298).
// The operations are in p3100-ub-leaves.h.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.div:observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.mod:observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.div:observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.mod:observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shl:observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shr:observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.add:observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.sub:observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.mul:observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.negate:observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.bool:observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.enum:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-ub-leaves.h"

// { dg-output "div_zero: R2 =0(\n|\r\n|\r)div_zero_assign: R2 =0(\n|\r\n|\r)mod_zero: R2 =0(\n|\r\n|\r)" }
// { dg-output "div_repr: R2 =0(\n|\r\n|\r)mod_repr: R2 =0(\n|\r\n|\r)shl: R2 =0(\n|\r\n|\r)" }
// { dg-output "shl_neg: R2 =0(\n|\r\n|\r)shr: R2 =0(\n|\r\n|\r)add: R6 =-2147483648(\n|\r\n|\r)" }
// { dg-output "inc: R6 =-2147483648(\n|\r\n|\r)sub: R6 =2147483647(\n|\r\n|\r)sub_const: R6 =2147483647(\n|\r\n|\r)" }
// { dg-output "dec: R6 =2147483647(\n|\r\n|\r)mul: R6 =-2(\n|\r\n|\r)mul_assign: R6 =-2(\n|\r\n|\r)" }
// { dg-output "negate: R6 =-2147483648(\n|\r\n|\r)load_bool: R6 =0(\n|\r\n|\r)load_enum: R6 =0(\n|\r\n|\r)" }
