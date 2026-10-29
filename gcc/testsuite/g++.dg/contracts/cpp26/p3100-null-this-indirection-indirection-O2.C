// P3100: a member function called through a null pointer is reported once,
// under .indirection, where it is called, at every optimization level; an
// indirection through `this' inside it that reads or writes nothing is not
// checked (DECISIONS.md E106 (a), extending E100 (a) to every use of
// `this'): a reference bound to `*this' (also const, to a base, a
// reference argument), to a member through `this->' or `(*this).', the
// address of a member through either, of `this->*pm' and `(*this).*pm',
// and of `this[0].a', a member function called through `this->', by name
// or through `(*this).', and in a lambda capturing `this' a member
// function called, a member's address and a reference bound to `*this'.
// Nor through a non-zero subscript of `this' (DECISIONS.md E108 (a)): the
// address of `this[1].a', `this[i].a' and `this[1]', a reference bound to
// `this[1]', a member function called on it, also in a lambda.  Nor is the
// parameter of a non-trivial copy constructor bound to `*this' (a copy of
// `*this', the `[*this]' capture).  `&*this', a discarded `*this' and a
// static member named through it (also in a lambda) indirect through
// nothing checked.  A non-null pointer is not reported.
// (GCC-836: at -O0 each was reported a second time, under .indirection,
// inside the member function.)  The forms are in
// p3100-null-this-indirection.h; the reads and writes through `this' are
// p3100-null-this-read*.C.
// With .indirection configured, at -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-this-indirection.h"
// { dg-output "g 1(\n|\r\n|\r)bind 1(\n|\r\n|\r)cbind 1(\n|\r\n|\r)" }
// { dg-output "bbind 1(\n|\r\n|\r)targ 1(\n|\r\n|\r)mbind 1(\n|\r\n|\r)" }
// { dg-output "maddr 1(\n|\r\n|\r)daddr 1(\n|\r\n|\r)dmbind 1(\n|\r\n|\r)" }
// { dg-output "pmaddr 1(\n|\r\n|\r)pmdaddr 1(\n|\r\n|\r)sub0addr 1(\n|\r\n|\r)" }
// { dg-output "tcall 1(\n|\r\n|\r)icall 1(\n|\r\n|\r)dcall 1(\n|\r\n|\r)" }
// { dg-output "lamcall 1(\n|\r\n|\r)lamaddr 1(\n|\r\n|\r)lambind 1(\n|\r\n|\r)" }
// { dg-output "sub1addr 1(\n|\r\n|\r)subiaddr 1(\n|\r\n|\r)sub1p 1(\n|\r\n|\r)" }
// { dg-output "sub1bind 1(\n|\r\n|\r)sub1call 1(\n|\r\n|\r)lamsub1call 1(\n|\r\n|\r)" }
// { dg-output "addr 1(\n|\r\n|\r)disc 1(\n|\r\n|\r)sstat 1(\n|\r\n|\r)" }
// { dg-output "lamthisaddr 1(\n|\r\n|\r)lamdisc 1(\n|\r\n|\r)ncopy 1(\n|\r\n|\r)" }
// { dg-output "nlamcopy 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
