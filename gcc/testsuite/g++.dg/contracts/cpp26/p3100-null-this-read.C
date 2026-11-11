// P3100: a member function called through a null pointer is reported once,
// under .indirection, where it is called; a read or write of a member
// through `this' inside it is not checked (DECISIONS.md E100 (a)), at any
// optimization level: by name, through `this->' or `(*this).', an element
// of an array member, a member of a member, in a lambda capturing `this'
// (by `[this]', `[&]', or the copy `[*this]'), and a copy of `*this', which
// reads through `this' too.  A read through a pointer parameter of a static
// member function is checked as any other (.access).  A non-null pointer is
// not reported.  Then Clang's forms: a store through `(*this).', in a lambda
// capturing `this' or by reference, `(*this).a' stored and a copy of
// `*this' in a lambda capturing `this', an assignment to and from `*this',
// an element of an array member and a member of a member through
// `(*this).', `this->*pm' and `(*this).*pm', `*this' moved (by a cast and
// by `std::move') and passed by value, `this[0].a', and, in a class with a
// base and a virtual base, a member of each read by name, through
// `(*this).' and through a cast of `*this' to the base, and the base
// copied.  Then GCC's: a store through `this->*pm' and `(*this).*pm', and
// a read through each in a lambda capturing `this'; and a read or write
// through a non-zero subscript of `this' (DECISIONS.md E108 (a)):
// `this[1].a' read and stored to, `this[i].a' read and stored to, an
// element of an array member, in a lambda capturing `this', and a copy of
// `this[1]'.  The indirections through `this' that read or write nothing
// (DECISIONS.md E106 (a)) are p3100-null-this-indirection*.C.  The forms
// are in p3100-null-this-read.h.
// With the whole group configured, at -O0.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-this-read.h"
// { dg-output "g 1(\n|\r\n|\r)st 1(\n|\r\n|\r)ex 1(\n|\r\n|\r)" }
// { dg-output "dr 1(\n|\r\n|\r)ar 1(\n|\r\n|\r)sx 1(\n|\r\n|\r)" }
// { dg-output "lam 1(\n|\r\n|\r)lamcopy 1(\n|\r\n|\r)lamref 1(\n|\r\n|\r)" }
// { dg-output "copy 1(\n|\r\n|\r)stat 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
// { dg-output "dst 1(\n|\r\n|\r)lamst 1(\n|\r\n|\r)lamrefst 1(\n|\r\n|\r)" }
// { dg-output "lamdr 1(\n|\r\n|\r)lamthiscopy 1(\n|\r\n|\r)assignto 1(\n|\r\n|\r)" }
// { dg-output "assignfrom 1(\n|\r\n|\r)arrdr 1(\n|\r\n|\r)sdr 1(\n|\r\n|\r)" }
// { dg-output "pmarrow 1(\n|\r\n|\r)pmdot 1(\n|\r\n|\r)movecopy 1(\n|\r\n|\r)" }
// { dg-output "stdmove 1(\n|\r\n|\r)byval 1(\n|\r\n|\r)sub0 1(\n|\r\n|\r)" }
// { dg-output "base 1(\n|\r\n|\r)dbase 1(\n|\r\n|\r)cbase 1(\n|\r\n|\r)" }
// { dg-output "bcopy 1(\n|\r\n|\r)vbase 1(\n|\r\n|\r)dvbase 1(\n|\r\n|\r)" }
// { dg-output "cvbase 1(\n|\r\n|\r)nonnull_extra 0(\n|\r\n|\r)" }
// { dg-output "pmstore 1(\n|\r\n|\r)pmstored 1(\n|\r\n|\r)lampm 1(\n|\r\n|\r)" }
// { dg-output "lampmd 1(\n|\r\n|\r)nonnull_gcc 0(\n|\r\n|\r)" }
// { dg-output "sub1 1(\n|\r\n|\r)sub1st 1(\n|\r\n|\r)subi 1(\n|\r\n|\r)" }
// { dg-output "subist 1(\n|\r\n|\r)sub1arr 1(\n|\r\n|\r)lamsub1 1(\n|\r\n|\r)" }
// { dg-output "sub1copy 1(\n|\r\n|\r)nonnull_sub 0(\n|\r\n|\r)" }
