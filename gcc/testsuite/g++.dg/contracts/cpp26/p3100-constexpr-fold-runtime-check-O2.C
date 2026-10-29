// P3100: p3100-constexpr-fold-runtime-check.C at -O2, where genericization
// folds constexpr calls in initializers too (GCC-797).
// { dg-do run { target c++26 } }
// { dg-additional-options "-O2 -fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun:observe,ub:expr.assign.overlap:observe,ub:class.abstract.pure.virtual.direct:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-prune-output "called from constructor" }

#include "p3100-constexpr-fold-runtime-check.C"

// { dg-output "base-init 1(\n|\r\n|\r)" }
// { dg-output "base-init-control 1(\n|\r\n|\r)" }
// { dg-output "pure-virtual 1(\n|\r\n|\r)" }
// { dg-output "pure-virtual-control 1(\n|\r\n|\r)" }
// { dg-output "overlap-call 1(\n|\r\n|\r)" }
// { dg-output "overlap-ctor 1(\n|\r\n|\r)" }
// { dg-output "overlap-control 1(\n|\r\n|\r)" }
// { dg-output "constexpr-var 0(\n|\r\n|\r)" }
// { dg-output "static-var 0(\n|\r\n|\r)" }
