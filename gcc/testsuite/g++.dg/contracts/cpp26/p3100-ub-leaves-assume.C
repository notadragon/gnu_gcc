// P3100: every leaf of DECISIONS.md E125 (a), configured by its own name,
// under "assume": nothing is checked, and the operation is performed (a
// division traps).
// The operations are in p3100-ub-leaves.h.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.div:assume" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.mod:assume" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.div:assume" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.mod:assume" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shl:assume" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shr:assume" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.add:assume" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.sub:assume" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.mul:assume" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.negate:assume" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.bool:assume" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.enum:assume" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-ub-leaves.h"

// { dg-output "div_zero: fpe(\n|\r\n|\r)div_zero_assign: fpe(\n|\r\n|\r)mod_zero: fpe(\n|\r\n|\r)" }
// { dg-output "div_repr: fpe(\n|\r\n|\r)mod_repr: fpe(\n|\r\n|\r)shl: =-?\[0-9\]+(\n|\r\n|\r)" }
// { dg-output "shl_neg: =-?\[0-9\]+(\n|\r\n|\r)shr: =-?\[0-9\]+(\n|\r\n|\r)add: =-?\[0-9\]+(\n|\r\n|\r)" }
// { dg-output "inc: =-?\[0-9\]+(\n|\r\n|\r)sub: =-?\[0-9\]+(\n|\r\n|\r)sub_const: =-?\[0-9\]+(\n|\r\n|\r)" }
// { dg-output "dec: =-?\[0-9\]+(\n|\r\n|\r)mul: =-?\[0-9\]+(\n|\r\n|\r)mul_assign: =-?\[0-9\]+(\n|\r\n|\r)" }
// { dg-output "negate: =-?\[0-9\]+(\n|\r\n|\r)load_bool: =-?\[0-9\]+(\n|\r\n|\r)load_enum: =-?\[0-9\]+(\n|\r\n|\r)" }
