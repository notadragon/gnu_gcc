// P3100: a call of a virtual function by its unqualified name through a
// null pointer is reported once under .access, whether it reads the
// vtable pointer through it (DECISIONS.md E102 (a)) or is devirtualized
// (DECISIONS.md E110 (a)): `vp->h ()', `(*vp).h ()', on an element `vp[1]'
// and at an offset `vp + 1' (which check vp), a virtual function of a
// polymorphic primary base (offset zero), on an element of a member array
// `hp->arr[1]', a virtual destructor called explicitly, and the calls GCC
// devirtualizes -- on a member subobject `hp->m' or the first element of a
// member array `hp->arr->' (whose dynamic types are known), through a
// `final' class or a `final' overrider.  A qualified or non-virtual call,
// which reads no vtable pointer, is reported once under .indirection.  A
// non-null pointer is not reported.  On an element `vp[i]' or at an offset
// `vp + i' that is not a constant, the null check is made on `vp + i', not
// vp, and nothing is reported (the GCC-794 divergence, DECISIONS.md E103
// (a)); Clang checks vp.  (GCC-845: `mp->w2 ()' and `hp->arr[1].h ()'
// were reported under .indirection too; GCC-849: the devirtualized calls
// were .indirection.)  Then GCC's: a primary base's virtual function at
// offset zero through `*mp', on `mp[1]' and `mp + 1', and one on an element of
// a member array of a member, each once under .access (GCC-845's shape,
// reported twice too); one called through `this' in a member function or
// a lambda called through a null pointer (DECISIONS.md E106 (a)) or
// through a reference bound to `*mp': only the call or the binding is
// reported, once under .indirection.  The forms are in
// p3100-null-virtual-call.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-virtual-call.cpp.
// With .indirection configured, at -O0.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-virtual-call.h"
// { dg-output "vcall 0(\n|\r\n|\r)vdcall 0(\n|\r\n|\r)vsub 0(\n|\r\n|\r)" }
// { dg-output "vsubi 0(\n|\r\n|\r)varith 0(\n|\r\n|\r)varithi 0(\n|\r\n|\r)" }
// { dg-output "vbase 0(\n|\r\n|\r)vmemarr 0(\n|\r\n|\r)vmemdecay 0(\n|\r\n|\r)" }
// { dg-output "vdtor 0(\n|\r\n|\r)vmember 0(\n|\r\n|\r)final 0(\n|\r\n|\r)" }
// { dg-output "qual 1(\n|\r\n|\r)dfinal 0(\n|\r\n|\r)nonvirt 1(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)vdbase 0(\n|\r\n|\r)vsubbase 0(\n|\r\n|\r)" }
// { dg-output "varithbase 0(\n|\r\n|\r)vthis 1(\n|\r\n|\r)vlambda 1(\n|\r\n|\r)" }
// { dg-output "vref 1(\n|\r\n|\r)vnested 0(\n|\r\n|\r)nonnull_gcc 0(\n|\r\n|\r)" }
