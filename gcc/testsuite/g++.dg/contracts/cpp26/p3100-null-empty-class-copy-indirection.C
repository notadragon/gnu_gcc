// P3100: a trivial copy or move of an empty class through a null pointer
// accesses no object -- the class has no member to read or write -- so the
// indirection is reported once under .indirection, not .access: assigning
// to `*ep' (`*ep = e', `*ep = E ()', `ep->operator= (e)'), copying or
// moving from it (`e = *ep', `E e2 = *ep', `E e2 = std::move (*ep)'),
// passing or returning it by value, list-initializing from it, copying from
// a reference cast of it, move-assigning from it, assigning from an empty
// base of `*p' (p checked before the conversion), in a template.  Assigning
// from an empty virtual base reads the base's offset through p, the
// .access leaf.  The same copies of a class with a member read or write
// it, the .access leaf.  A non-null pointer is not reported.  The forms
// are in p3100-null-empty-class-copy.h.
// With only .indirection configured (-O0), the empty-class forms but the
// virtual base's are reported.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-empty-class-copy.cpp.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-empty-class-copy.h"
// { dg-output "eassign 1(\n|\r\n|\r)eassign_tmp 1(\n|\r\n|\r)eassign_call 1(\n|\r\n|\r)" }
// { dg-output "eassign_from 1(\n|\r\n|\r)ecopy 1(\n|\r\n|\r)emove 1(\n|\r\n|\r)" }
// { dg-output "sassign 0(\n|\r\n|\r)sassign_call 0(\n|\r\n|\r)sassign_from 0(\n|\r\n|\r)" }
// { dg-output "scopy 0(\n|\r\n|\r)earg 1(\n|\r\n|\r)eret 1(\n|\r\n|\r)" }
// { dg-output "ebrace 1(\n|\r\n|\r)ecast 1(\n|\r\n|\r)emove_from 1(\n|\r\n|\r)" }
// { dg-output "ebase_from 1(\n|\r\n|\r)evbase_from 0(\n|\r\n|\r)tcopy 1(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
