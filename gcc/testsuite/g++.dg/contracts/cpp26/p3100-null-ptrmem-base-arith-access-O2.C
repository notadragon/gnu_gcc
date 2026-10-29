// P3100: `(a + i)->*pm' is an indirection through a + i, checked for null
// on a itself (DECISIONS.md E99 (a)) -- also where a + i is converted to
// the base class whose member the pointer to member is: a pointer to data
// member, read (.access) or its address taken (.indirection), of a
// polymorphic base at offset zero or not and of one that is not
// polymorphic; a call through a pointer to member function of a base at
// a nonzero offset, virtual (.access) or not (.indirection; DECISIONS.md
// E102 (a)).  Each once, under its leaf; a non-null pointer is not
// reported.  The forms are in p3100-null-ptrmem-base-arith.h, a mirror of
// Clang's Runnable/p3100-null-ptrmem-base-arith.cpp (CLANG-801, where the
// conversion hid the arithmetic).
// With only .access configured, at -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-ptrmem-base-arith.h"

// { dg-output "mwrd 1(\n|\r\n|\r)mvrd 1(\n|\r\n|\r)dsrd 1(\n|\r\n|\r)" }
// { dg-output "dsaddr 0(\n|\r\n|\r)mvaddr 0(\n|\r\n|\r)mvpmfv 1(\n|\r\n|\r)" }
// { dg-output "mvpmfnv 0(\n|\r\n|\r)mwrd0 1(\n|\r\n|\r)mvrd0 1(\n|\r\n|\r)" }
// { dg-output "srd 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
