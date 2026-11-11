// P3100: every leaf of DECISIONS.md E125 (a), configured by its own name,
// under "quick_enforce": the program terminates, with no handler.
// The operations are in p3100-ub-leaves.h.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.div:quick_enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.mod:quick_enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.div:quick_enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.mod:quick_enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shl:quick_enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shr:quick_enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.add:quick_enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.sub:quick_enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.mul:quick_enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.negate:quick_enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.bool:quick_enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.enum:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-ub-leaves.h"

// { dg-output "div_zero: trap(\n|\r\n|\r)div_zero_assign: trap(\n|\r\n|\r)mod_zero: trap(\n|\r\n|\r)" }
// { dg-output "div_repr: trap(\n|\r\n|\r)mod_repr: trap(\n|\r\n|\r)shl: trap(\n|\r\n|\r)" }
// { dg-output "shl_neg: trap(\n|\r\n|\r)shr: trap(\n|\r\n|\r)add: trap(\n|\r\n|\r)" }
// { dg-output "inc: trap(\n|\r\n|\r)sub: trap(\n|\r\n|\r)sub_const: trap(\n|\r\n|\r)" }
// { dg-output "dec: trap(\n|\r\n|\r)mul: trap(\n|\r\n|\r)mul_assign: trap(\n|\r\n|\r)" }
// { dg-output "negate: trap(\n|\r\n|\r)load_bool: trap(\n|\r\n|\r)load_enum: trap(\n|\r\n|\r)" }
