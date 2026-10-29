// P3100: a call through a pointer to member function of a polymorphic
// class through a null pointer, `(vp->*p) ()', or on a member subobject of
// the object it points to, `(hp->m.*p) ()', takes the leaf of its P3100
// null check from the pointer's virtual bit, so the check is made after
// the pointer to member function is evaluated (DECISIONS.md E102 (a)).
// With -fsanitize=null too, routed to the handler, the contract's check
// replaces the sanitizer's for the pointer it checks, so the one null
// dereference is reported once (DECISIONS.md E111 (a); GCC-852: twice), as
// where a P3100 check and the sanitizer's are at the same place
// (p3100-precedence-null.C).  A virtual call on the member and a read of
// it, the controls, are reported once.  Then GCC's: a non-virtual pointer
// to member function, the object of `->*' and an element at a constant
// subscript as the object, a member of a virtual base (GCC-850); the
// virtual calls whose object pointer the front end checks for P3100 --
// devirtualized through a `final' class or on the first element of a
// member array (DECISIONS.md E110 (a)), on an element of a member array,
// a primary base's virtual function (offset zero); and a pointer to data
// member applied through a null pointer, at offset zero too, and to a
// member; then Clang's: on an element of a member array, of `*hp', and on
// a member of a base at a nonzero offset, through `->' and `*'; then a
// pointer to member function of a base class, at offset zero and not, on
// an element at a constant subscript (GCC-864: twice at offset zero):
// each once.
// The forms are in p3100-null-pmf-sanitize-null.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-pmf-sanitize-null.cpp.
// With only .access configured, the sanitizer making the null checks of
// the other leaf, at -O2.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fsanitize=null -fsanitize-semantic=null:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-pmf-sanitize-null.h"

// { dg-output "ptrv 1(\n|\r\n|\r)memv 1(\n|\r\n|\r)memnv 1(\n|\r\n|\r)" }
// { dg-output "call 1(\n|\r\n|\r)rd 1(\n|\r\n|\r)ptrnv 1(\n|\r\n|\r)" }
// { dg-output "memptrv 1(\n|\r\n|\r)memptrnv 1(\n|\r\n|\r)submemv 1(\n|\r\n|\r)" }
// { dg-output "submemnv 1(\n|\r\n|\r)vbasememv 1(\n|\r\n|\r)vbasememnv 1(\n|\r\n|\r)" }
// { dg-output "fin 1(\n|\r\n|\r)decay 1(\n|\r\n|\r)memarr 1(\n|\r\n|\r)" }
// { dg-output "base 1(\n|\r\n|\r)pmd 1(\n|\r\n|\r)pmd0 1(\n|\r\n|\r)" }
// { dg-output "pmdmem 1(\n|\r\n|\r)memarrpv 1(\n|\r\n|\r)memarrpnv 1(\n|\r\n|\r)" }
// { dg-output "dmemv 1(\n|\r\n|\r)dmemnv 1(\n|\r\n|\r)basememv 1(\n|\r\n|\r)" }
// { dg-output "basememnv 1(\n|\r\n|\r)dbasememv 1(\n|\r\n|\r)dbasememnv 1(\n|\r\n|\r)" }
// { dg-output "mwpmfv 1(\n|\r\n|\r)mwpmfnv 1(\n|\r\n|\r)mwdpmfv 1(\n|\r\n|\r)" }
// { dg-output "mwdpmfnv 1(\n|\r\n|\r)mvpmfv 1(\n|\r\n|\r)mvpmfnv 1(\n|\r\n|\r)" }
// { dg-output "mwsubv 1(\n|\r\n|\r)mwsubnv 1(\n|\r\n|\r)" }
