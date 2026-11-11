// P3100: a trivial copy or move of an empty class through a null pointer
// accesses no object, so the indirection is reported once under
// .indirection (DECISIONS.md E86 (a); p3100-null-empty-class-copy*.C have
// the first forms): passing or returning `*ep' by value, list-initializing
// from it, copying from a reference cast of it, move-assigning from it, in
// a template; an empty non-virtual base of `*dp' -- at offset 0 or not, or
// at a nonzero offset where its address is `dp' plus the offset -- copied,
// assigned from, moved from, passed or returned by value, stored to through
// a reference cast or `dp->E::operator= (e)', its address taken, a
// reference bound to it, discarded; an empty member of `*hp', or an element
// of an array of them, copied, moved from, passed by value or stored to (a
// subscript with side effects still evaluated); an empty base of a member
// of `*hp' copied.  Copying an empty virtual base, or discarding a
// conversion to one, is the .access leaf: one of `*vp' reads the base's
// offset through the pointer, and one of a member object, whose offset is
// known, is a conversion to a virtual base all the same (DECISIONS.md E94
// (b); p3100-null-vbase-member-bind.C has more).  A non-null pointer is not
// reported.  The forms are in p3100-null-empty-class-copy-extra.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-empty-class-copy-extra.cpp.
// With the whole group configured, at -O0, every form is reported once.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-empty-class-copy-extra.h"
// { dg-output "earg 1(\n|\r\n|\r)eret 1(\n|\r\n|\r)ebrace 1(\n|\r\n|\r)" }
// { dg-output "ecast 1(\n|\r\n|\r)emove_from 1(\n|\r\n|\r)ebase_from 1(\n|\r\n|\r)" }
// { dg-output "evbase_from 1(\n|\r\n|\r)tcopy 1(\n|\r\n|\r)ebase_copy 1(\n|\r\n|\r)" }
// { dg-output "ebase_arg 1(\n|\r\n|\r)ebase_ret 1(\n|\r\n|\r)ebase_store 1(\n|\r\n|\r)" }
// { dg-output "ebase_call 1(\n|\r\n|\r)ebase_move 1(\n|\r\n|\r)ebase0_from 1(\n|\r\n|\r)" }
// { dg-output "evbase_copy 1(\n|\r\n|\r)emember_copy 1(\n|\r\n|\r)emember_arg 1(\n|\r\n|\r)" }
// { dg-output "emember_store 1(\n|\r\n|\r)hdmember_base 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
// { dg-output "ebasen_from 1(\n|\r\n|\r)ebasen_copy 1(\n|\r\n|\r)ebasen_move 1(\n|\r\n|\r)" }
// { dg-output "ebasen_store 1(\n|\r\n|\r)ebasen_call 1(\n|\r\n|\r)ebasen_addr 1(\n|\r\n|\r)" }
// { dg-output "ebasen_bind 1(\n|\r\n|\r)ebasen_disc 1(\n|\r\n|\r)emember_move 1(\n|\r\n|\r)" }
// { dg-output "emember_move_from 1(\n|\r\n|\r)earray_copy 1(\n|\r\n|\r)earray_from 1(\n|\r\n|\r)" }
// { dg-output "earray_store 1(\n|\r\n|\r)earray_side 1(\n|\r\n|\r)hvmember_from 1(\n|\r\n|\r)" }
// { dg-output "hvmember_copy 1(\n|\r\n|\r)hvmember_disc 1(\n|\r\n|\r)nonnull_extra 0(\n|\r\n|\r)" }
