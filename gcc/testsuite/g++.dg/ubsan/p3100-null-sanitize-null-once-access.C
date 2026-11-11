// P3100: with -fsanitize=null routed to the contract-violation handler as
// well as ub:expr.unary.dereference.nullptr configured, a member call
// through a null pointer on a subobject at offset zero -- a function of a
// primary base, `mp->W2::w2 ()', `mp->nw ()', a member, `hp->m.nv ()' --
// and a read through a downcast of `*vp' to a reference,
// `static_cast<D &> (*vp).d', are reported once (DECISIONS.md E120;
// GCC-860: twice, the object pointer's check and the sanitizer's check of
// the subobject's address, or of the read through the reference, as stock
// -fsanitize=null alone does).  The front end makes no -fsanitize=null
// check of a member call's object pointer that is a subobject's address
// the ubsan pass checks the object pointer of, and the ubsan pass none of
// a read through a reference just bound, whose binding is checked.  With
// the group ignored, the reference binding gets no P3100 check and the
// sanitizer's two checks of the downcast forms stay stock: twice (E120:
// suppressing them would need the binding tracked for the sanitizer as
// well).  The downcast alone, bound to a reference, and a non-virtual call
// through it, are reported once in every run, the group ignored too: the
// binding's check or the call's alone checks the null dereference.
// (Clang reports them twice, the call three times at -O0: CLANG-796 and
// CLANG-792, accepted under E120 and E114 (b).)  The forms are in
// p3100-null-sanitize-null-once.h.
// With only .access configured, at -O0.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fsanitize=null -fsanitize-semantic=null:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-sanitize-null-once.h"

// { dg-output "mwq 1(\n|\r\n|\r)mwnw 1(\n|\r\n|\r)h0nv 1(\n|\r\n|\r)" }
// { dg-output "drdc 1(\n|\r\n|\r)dsrd 1(\n|\r\n|\r)" }
// { dg-output "dref 1(\n|\r\n|\r)dsref 1(\n|\r\n|\r)dcall 1(\n|\r\n|\r)" }
