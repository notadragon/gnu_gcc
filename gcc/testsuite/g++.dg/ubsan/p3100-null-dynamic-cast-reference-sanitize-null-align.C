// P3100: with -fsanitize=null routed to the contract-violation handler as
// well as ub:expr.unary.dereference.nullptr configured, a dynamic_cast to
// a reference type of the object a null pointer points to,
// `dynamic_cast<W &> (*vp)', is reported once (DECISIONS.md E120; GCC-875:
// twice, the P3100 .access check of the vtable pointer's read and the
// sanitizer's check of the operand's binding to the cast's reference).
// The ubsan pass drops the binding's check, made on a pointer formed from
// the checked object pointer, when it checks no alignment: the object
// pointer's check makes the sanitizer's null check where its P3100 leaf
// does not apply (.indirection alone: it was twice at -O0).  With the
// group ignored the checks are stock -fsanitize=null's, once; `vp[1]' and
// `*(vp + 1)' are then not reported, the sanitizer testing `vp + 1'
// (DECISIONS.md E121 (a), E115 (a)).  The forms are in
// ../contracts/cpp26/p3100-null-dynamic-cast-reference.h.
// With -fsanitize=alignment routed too, the binding's check, which then
// tests the alignment as well, is kept whole, and its null edge reports
// again: twice.  So does the alignment check the ubsan pass makes of the
// object pointer of `hp->m' and of the conversion of `*xp' to its base,
// whose null edge tests the same pointer (three times for the base, which
// stock -fsanitize=null reports twice).  Accepted under DECISIONS.md E120:
// dropping only a check's null edge would need it carried from the ubsan
// pass to the check's expansion; the front end keeps a checked
// subobject's binding check with an alignment edge the same way.  The
// whole group configured, at -O0.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fsanitize=null,alignment -fsanitize-semantic=null:noexcept_observe,alignment:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "../contracts/cpp26/p3100-null-dynamic-cast-reference.h"

// { dg-output "star 2(\n|\r\n|\r)cstar 2(\n|\r\n|\r)sub 2(\n|\r\n|\r)" }
// { dg-output "member 2(\n|\r\n|\r)refparam 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
// { dg-output "arith 2(\n|\r\n|\r)base 3(\n|\r\n|\r)nonnull_clang 0(\n|\r\n|\r)" }
