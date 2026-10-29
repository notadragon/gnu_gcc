// P3100: a trivial special member function called explicitly through a
// null pointer.  A trivial destructor call evaluates its object expression
// and accesses nothing, like a pseudo-destructor call: `sp->~S ()',
// `(*sp).~S ()', `sp->S::~S ()', the destructor of a base at a nonzero
// offset (`dp->S::~S ()', `(*dp).S::~S ()', p checked before the
// conversion) and one in a template are each reported once under
// .indirection; a virtual base's (`vp->S::~S ()') loads the base's offset
// through vp, the .access leaf (reported once, also at -O2).  A trivial
// assignment called through the pointer, `sp->operator= (s)', from a
// temporary, or a base's `dp->S::operator= (s)', stores through it as
// `*sp = s' and `(*sp).operator= (s)' do: once, under .access.  `this', a
// declared object, the address of an object and a non-null pointer are not
// checked.  The forms are in p3100-null-trivial-member-call.h.
// With the whole group configured, at -O0, every form is reported once.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-trivial-member-call.cpp.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-trivial-member-call.h"
// { dg-output "sdtor 1(\n|\r\n|\r)sdtor_star 1(\n|\r\n|\r)sdtor_qual 1(\n|\r\n|\r)" }
// { dg-output "bdtor 1(\n|\r\n|\r)bdtor_star 1(\n|\r\n|\r)tdtor 1(\n|\r\n|\r)" }
// { dg-output "vdtor 1(\n|\r\n|\r)assign 1(\n|\r\n|\r)assign_tmp 1(\n|\r\n|\r)" }
// { dg-output "assign_base 1(\n|\r\n|\r)assign_star 1(\n|\r\n|\r)assign_op 1(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)controls 0(\n|\r\n|\r)" }
