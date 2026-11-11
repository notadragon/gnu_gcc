// P3100: a configuration file naming a leaf of DECISIONS.md E125 (a) and,
// after it, the group that contains it: the first entry that matches
// decides.  ub:conv.lval.valid.representation.enum is ignored before the
// combined group ub:conv.lval.valid.representation.bool.enum observes
// .bool; ub:expr.mul.div.by.zero.int.mod is ignored before
// ub:expr.mul.div.by.zero.int observes .int.div;
// ub:expr.expr.eval.signed.integer.negate is ignored before
// ub:expr.expr.eval observes .add, .sub and .mul.  The other leaves keep
// the default "assume".  The operations are in p3100-ub-leaves.h.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-ub-leaves-groups-config.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-ub-leaves.h"

// { dg-output "div_zero: R6 =0(\n|\r\n|\r)div_zero_assign: R6 =0(\n|\r\n|\r)mod_zero: =0(\n|\r\n|\r)" }
// { dg-output "div_repr: fpe(\n|\r\n|\r)mod_repr: fpe(\n|\r\n|\r)shl: =-?\[0-9\]+(\n|\r\n|\r)" }
// { dg-output "shl_neg: =-?\[0-9\]+(\n|\r\n|\r)shr: =-?\[0-9\]+(\n|\r\n|\r)add: R6 =-2147483648(\n|\r\n|\r)" }
// { dg-output "inc: R6 =-2147483648(\n|\r\n|\r)sub: R6 =2147483647(\n|\r\n|\r)sub_const: R6 =2147483647(\n|\r\n|\r)" }
// { dg-output "dec: R6 =2147483647(\n|\r\n|\r)mul: R6 =-2(\n|\r\n|\r)mul_assign: R6 =-2(\n|\r\n|\r)" }
// { dg-output "negate: =-2147483648(\n|\r\n|\r)load_bool: R6 =0(\n|\r\n|\r)load_enum: =0(\n|\r\n|\r)" }
