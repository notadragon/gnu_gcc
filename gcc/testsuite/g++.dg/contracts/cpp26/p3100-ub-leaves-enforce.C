// P3100: every leaf of DECISIONS.md E125 (a), configured by its own name,
// under "enforce": the handler runs, then the program terminates;
// the middle-end checks report under noexcept_enforce, the best fit.
// The operations are in p3100-ub-leaves.h.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.div:enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.mod:enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.div:enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.mod:enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shl:enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shr:enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.add:enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.sub:enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.mul:enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.negate:enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.bool:enforce" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.enum:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-ub-leaves.h"

// { dg-output "div_zero: R3 abort(\n|\r\n|\r)div_zero_assign: R3 abort(\n|\r\n|\r)mod_zero: R3 abort(\n|\r\n|\r)" }
// { dg-output "div_repr: R3 abort(\n|\r\n|\r)mod_repr: R3 abort(\n|\r\n|\r)shl: R3 abort(\n|\r\n|\r)" }
// { dg-output "shl_neg: R3 abort(\n|\r\n|\r)shr: R3 abort(\n|\r\n|\r)add: R7 abort(\n|\r\n|\r)" }
// { dg-output "inc: R7 abort(\n|\r\n|\r)sub: R7 abort(\n|\r\n|\r)sub_const: R7 abort(\n|\r\n|\r)" }
// { dg-output "dec: R7 abort(\n|\r\n|\r)mul: R7 abort(\n|\r\n|\r)mul_assign: R7 abort(\n|\r\n|\r)" }
// { dg-output "negate: R7 abort(\n|\r\n|\r)load_bool: R7 abort(\n|\r\n|\r)load_enum: R7 abort(\n|\r\n|\r)" }
