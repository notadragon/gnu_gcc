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
// With .access configured, at -O0.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-this-indirection.h"
// { dg-output "g 0(\n|\r\n|\r)bind 0(\n|\r\n|\r)cbind 0(\n|\r\n|\r)" }
// { dg-output "bbind 0(\n|\r\n|\r)targ 0(\n|\r\n|\r)mbind 0(\n|\r\n|\r)" }
// { dg-output "maddr 0(\n|\r\n|\r)daddr 0(\n|\r\n|\r)dmbind 0(\n|\r\n|\r)" }
// { dg-output "pmaddr 0(\n|\r\n|\r)pmdaddr 0(\n|\r\n|\r)sub0addr 0(\n|\r\n|\r)" }
// { dg-output "tcall 0(\n|\r\n|\r)icall 0(\n|\r\n|\r)dcall 0(\n|\r\n|\r)" }
// { dg-output "lamcall 0(\n|\r\n|\r)lamaddr 0(\n|\r\n|\r)lambind 0(\n|\r\n|\r)" }
// { dg-output "sub1addr 0(\n|\r\n|\r)subiaddr 0(\n|\r\n|\r)sub1p 0(\n|\r\n|\r)" }
// { dg-output "sub1bind 0(\n|\r\n|\r)sub1call 0(\n|\r\n|\r)lamsub1call 0(\n|\r\n|\r)" }
// { dg-output "addr 0(\n|\r\n|\r)disc 0(\n|\r\n|\r)sstat 0(\n|\r\n|\r)" }
// { dg-output "lamthisaddr 0(\n|\r\n|\r)lamdisc 0(\n|\r\n|\r)ncopy 0(\n|\r\n|\r)" }
// { dg-output "nlamcopy 0(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
