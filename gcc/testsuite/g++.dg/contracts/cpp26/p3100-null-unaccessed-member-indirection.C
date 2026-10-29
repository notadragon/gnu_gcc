// P3100: a member of an object reached through a null pointer, evaluated
// and not accessed, is an indirection through the pointer whose result is
// not accessed, reported once under .indirection (DECISIONS.md E86 (a)): a
// discarded member (`hp->x;', cast to void, through `(*hp)', nested, an
// element of an array member, the left operand of a comma, empty, of a
// reference cast of `*sp', of a base), the object expression of a static
// member, an enumerator or a static member function named through a member,
// a trivial destructor called on a member; a subscript with side effects is
// still evaluated.  A member of a virtual base reads the base's offset
// through the pointer, and a volatile member is read: the .access leaf.  A
// reference and `this' are not checked, nor is a non-null pointer.  The
// forms are in p3100-null-unaccessed-member.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-unaccessed-member.cpp.
// With only .indirection configured (-O0), every form but the virtual
// base's member and the volatile one.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-Wno-unused-value" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-unaccessed-member.h"
// { dg-output "disc_member 1(\n|\r\n|\r)disc_member_void 1(\n|\r\n|\r)disc_member_star 1(\n|\r\n|\r)" }
// { dg-output "disc_member_nested 1(\n|\r\n|\r)disc_member_array 1(\n|\r\n|\r)disc_member_comma 1(\n|\r\n|\r)" }
// { dg-output "disc_member_empty 1(\n|\r\n|\r)disc_member_cast 1(\n|\r\n|\r)disc_member_base 1(\n|\r\n|\r)" }
// { dg-output "disc_member_vbase 0(\n|\r\n|\r)smember_static 1(\n|\r\n|\r)smember_enum 1(\n|\r\n|\r)" }
// { dg-output "smember_sf 1(\n|\r\n|\r)member_dtor 1(\n|\r\n|\r)disc_member_side 1(\n|\r\n|\r)" }
// { dg-output "disc_volatile 0(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)controls 0(\n|\r\n|\r)" }
