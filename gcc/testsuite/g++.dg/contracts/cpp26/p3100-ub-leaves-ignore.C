// P3100: every leaf of DECISIONS.md E125 (a), configured by its own name,
// under "ignore": no report, and the operation yields its defined
// substitute (0, the wrapped result, a valid value).
// The operations are in p3100-ub-leaves.h.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.div:ignore" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.mod:ignore" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.div:ignore" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.mod:ignore" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shl:ignore" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shr:ignore" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.add:ignore" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.sub:ignore" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.mul:ignore" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.negate:ignore" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.bool:ignore" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.enum:ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-ub-leaves.h"

// { dg-output "div_zero: =0(\n|\r\n|\r)div_zero_assign: =0(\n|\r\n|\r)mod_zero: =0(\n|\r\n|\r)" }
// { dg-output "div_repr: =0(\n|\r\n|\r)mod_repr: =0(\n|\r\n|\r)shl: =0(\n|\r\n|\r)" }
// { dg-output "shl_neg: =0(\n|\r\n|\r)shr: =0(\n|\r\n|\r)add: =-2147483648(\n|\r\n|\r)" }
// { dg-output "inc: =-2147483648(\n|\r\n|\r)sub: =2147483647(\n|\r\n|\r)sub_const: =2147483647(\n|\r\n|\r)" }
// { dg-output "dec: =2147483647(\n|\r\n|\r)mul: =-2(\n|\r\n|\r)mul_assign: =-2(\n|\r\n|\r)" }
// { dg-output "negate: =-2147483648(\n|\r\n|\r)load_bool: =0(\n|\r\n|\r)load_enum: =0(\n|\r\n|\r)" }
