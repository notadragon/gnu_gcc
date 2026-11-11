// P3100: by DECISIONS.md E94 (b) every conversion of a glvalue to a virtual
// base through a null pointer is the .access leaf, reported once, whatever
// its result is used for: a reference bound to it, its address taken,
// discarded -- of a member object (`E &r = hp->ve', `&static_cast<E &>
// (hp->ve)', `static_cast<E &> (hp->ve);', `(*hp).ve', `hp->*pm'), of an
// element of a member array (`hp->va[1]'), of a non-empty virtual base (`B
// &r = hp->vb'), of an element `vp[i]' of the null pointer itself (checked
// on `vp').  A member of such a base read, called or its address taken is
// .access too.  A non-null pointer is not reported.  The forms are in
// p3100-null-vbase-member-bind.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-vbase-member-bind.cpp.
// With only .access configured (-O2).
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-vbase-member-bind.h"
// { dg-output "hvmember_bind 1(\n|\r\n|\r)hvmember_addr 1(\n|\r\n|\r)hvmember_disc 1(\n|\r\n|\r)" }
// { dg-output "hvstar_bind 1(\n|\r\n|\r)hvmptr_bind 1(\n|\r\n|\r)hvarray_bind 1(\n|\r\n|\r)" }
// { dg-output "hvarray_disc 1(\n|\r\n|\r)hvbase_bind 1(\n|\r\n|\r)hvbase_read 1(\n|\r\n|\r)" }
// { dg-output "hvbase_call 1(\n|\r\n|\r)hvbase_addr 1(\n|\r\n|\r)vsub_bind 1(\n|\r\n|\r)" }
// { dg-output "vsub0_bind 1(\n|\r\n|\r)vsub_disc 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
