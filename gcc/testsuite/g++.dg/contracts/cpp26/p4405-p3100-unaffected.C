// P4405: P3100 implicit contract assertions are unaffected by prove and
// diagnose: neither is among the semantics an implicit assertion allows, so
// the best fit takes a configured prove or diagnose to ignore -- no
// diagnostic, no report, and each operation yields its defined substitute,
// exactly as p3100-ub-leaves-ignore.C.  The operations are in
// p3100-ub-leaves.h.
//
// Mirror: clang/test/Contracts/p4405-p3100-unaffected.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4405 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.div:prove" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.mod:diagnose" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.div:prove" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.mod:diagnose" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shl:prove" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shr:diagnose" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.add:prove" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.sub:diagnose" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.mul:prove" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.negate:diagnose" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.bool:prove" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.enum:diagnose" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-ub-leaves.h"

// { dg-output "div_zero: =0(\n|\r\n|\r)div_zero_assign: =0(\n|\r\n|\r)mod_zero: =0(\n|\r\n|\r)" }
// { dg-output "div_repr: =0(\n|\r\n|\r)mod_repr: =0(\n|\r\n|\r)shl: =0(\n|\r\n|\r)" }
// { dg-output "shl_neg: =0(\n|\r\n|\r)shr: =0(\n|\r\n|\r)add: =-2147483648(\n|\r\n|\r)" }
// { dg-output "inc: =-2147483648(\n|\r\n|\r)sub: =2147483647(\n|\r\n|\r)sub_const: =2147483647(\n|\r\n|\r)" }
// { dg-output "dec: =2147483647(\n|\r\n|\r)mul: =-2(\n|\r\n|\r)mul_assign: =-2(\n|\r\n|\r)" }
// { dg-output "negate: =-2147483648(\n|\r\n|\r)load_bool: =0(\n|\r\n|\r)load_enum: =0(\n|\r\n|\r)" }
