// P3100: an object reached through a null pointer and bound to a
// reference that the same expression then reads or writes through -- the
// reference parameter of an elided trivial copy (`S s2 = *sp', `s = *sp',
// direct and list initialization, a return, a by-value argument, a copy
// without the tail padding, from a base, a member passed by value), a
// reference cast of `*p' read, added to, stored to, copied, its member
// read, `std::move (*p)' and `std::as_const (*p)' read or copied -- is one
// null dereference, reported once, under .access (the binding's check
// becomes the access's).  A reference bound to a reference, `return r'
// from a function taking and returning one, indirects through nothing;
// the binding of `*p' to its parameter is .indirection, and the read
// through the reference the call returns is not checked again
// (DECISIONS.md E92 (a)), at any level.  The address of a member of a
// reference cast of `*sp' is .indirection.  A non-null pointer is not
// reported.  The forms are in p3100-null-binding-read.h, then Clang's
// extra forms, each once under .access: a class member copied
// (initialized from, assigned from, returned, in parentheses, moved,
// through `(*hp).s'), also the first member, at the object's address, and
// stored to.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-binding-read.cpp.
// With only .indirection configured (-O0), only the bindings to a reference
// parameter and the address are reported.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-binding-read.h"
// { dg-output "scopy 0(\n|\r\n|\r)sassign_from 0(\n|\r\n|\r)sparen 0(\n|\r\n|\r)" }
// { dg-output "sbrace 0(\n|\r\n|\r)sret 0(\n|\r\n|\r)sarg 0(\n|\r\n|\r)" }
// { dg-output "ucopy 0(\n|\r\n|\r)uassign_from 0(\n|\r\n|\r)tassign_base 0(\n|\r\n|\r)" }
// { dg-output "hmember_arg 0(\n|\r\n|\r)tcopy 0(\n|\r\n|\r)iscast 0(\n|\r\n|\r)" }
// { dg-output "iscast_add 0(\n|\r\n|\r)iscast_store 0(\n|\r\n|\r)scast 0(\n|\r\n|\r)" }
// { dg-output "scast_member 0(\n|\r\n|\r)imove 0(\n|\r\n|\r)smove 0(\n|\r\n|\r)" }
// { dg-output "smove_from 0(\n|\r\n|\r)iconst 0(\n|\r\n|\r)rebind_id 1(\n|\r\n|\r)" }
// { dg-output "rebind_const 1(\n|\r\n|\r)scast_addr 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
// { dg-output "hmember_copy 0(\n|\r\n|\r)hmember_assign 0(\n|\r\n|\r)hmember_ret 0(\n|\r\n|\r)" }
// { dg-output "hmember_paren 0(\n|\r\n|\r)hmember_move 0(\n|\r\n|\r)hstar_member_copy 0(\n|\r\n|\r)" }
// { dg-output "hmember_store 0(\n|\r\n|\r)h0member_arg 0(\n|\r\n|\r)h0member_copy 0(\n|\r\n|\r)" }
// { dg-output "h0member_assign 0(\n|\r\n|\r)h0member_move 0(\n|\r\n|\r)h0member_store 0(\n|\r\n|\r)" }
// { dg-output "nonnull_extra 0(\n|\r\n|\r)" }
