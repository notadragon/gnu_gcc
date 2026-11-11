// P3100: a call through a pointer to member function on a member
// subobject of the object a null pointer points to -- `(hp->m.*p) ()',
// `(hp->arr[1].*p) ()', `((hp->*pm).*p) ()', `((*hp).m.*p) ()' -- is
// reported once, under the leaf the pointer to member function selects:
// through a virtual one the call reads the subobject's vtable pointer
// through hp, .access, as one on `*vp' does (DECISIONS.md E102 (a),
// p3100-null-virtual-pmf.h); through a non-virtual one, .indirection.  A
// non-null pointer is not reported.  (GCC-846: the virtual forms were
// .indirection.)  Then GCC's: on the member of an element at a constant
// subscript, `(hp[1].m.*p) ()', and on an element of a member array
// through a pointer to data member, `((hp->*pa)[1].*p) ()', the same; on
// a member through `this' in a member function called through a null
// pointer (DECISIONS.md E106 (a)) or through a reference bound to `*hp',
// only the call or the binding is reported, once under .indirection.
// Then Clang's: on a member of a base class at a nonzero offset,
// `(dp->m.*p) ()' and `((*dp).m.*p) ()', the same as on a member; on a
// member of a virtual base, `(vp->m.*p) ()', once under .access whatever
// the pointer to member function is, the base's offset being read through
// vp first (DECISIONS.md E94 (b); GCC-850: not reported).  The forms are in
// p3100-null-virtual-pmf-member.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-virtual-pmf-member.cpp.
// With .indirection configured, at -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-virtual-pmf-member.h"
// { dg-output "memv 0(\n|\r\n|\r)memnv 1(\n|\r\n|\r)memarrv 0(\n|\r\n|\r)" }
// { dg-output "memarrnv 1(\n|\r\n|\r)memptrv 0(\n|\r\n|\r)memptrnv 1(\n|\r\n|\r)" }
// { dg-output "dmemv 0(\n|\r\n|\r)dmemnv 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
// { dg-output "selfv 1(\n|\r\n|\r)selfnv 1(\n|\r\n|\r)refv 1(\n|\r\n|\r)" }
// { dg-output "refnv 1(\n|\r\n|\r)submemv 0(\n|\r\n|\r)submemnv 1(\n|\r\n|\r)" }
// { dg-output "ptrmemarrv 0(\n|\r\n|\r)ptrmemarrnv 1(\n|\r\n|\r)nonnull_gcc 0(\n|\r\n|\r)" }
// { dg-output "basememv 0(\n|\r\n|\r)basememnv 1(\n|\r\n|\r)dbasememv 0(\n|\r\n|\r)" }
// { dg-output "dbasememnv 1(\n|\r\n|\r)vbasememv 0(\n|\r\n|\r)vbasememnv 0(\n|\r\n|\r)" }
// { dg-output "nonnull_clang 0(\n|\r\n|\r)" }
