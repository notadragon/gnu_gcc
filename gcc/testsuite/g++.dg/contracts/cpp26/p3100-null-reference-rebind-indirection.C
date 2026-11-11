// P3100: a glvalue that is already a reference -- a reference variable,
// parameter or member, a call returning a reference -- indirects through
// nothing.  One null dereference, `*p' bound to a reference, is reported
// once, under .indirection, when the reference is then bound to another
// reference (`const int &r2 = r', `int &&r2 = std::move (r)',
// `static_cast<int &&> (r)', `return r' from a function returning a
// reference, passed to a reference parameter), converted to a base (also a
// virtual one, or an empty one at a nonzero offset) or to a base pointer,
// has a member function called on it or a member's address taken, or a
// member discarded, and when the reference a call returns is read, stored
// to, has its member read or is copied, and when a reference variable is
// read or copied or its virtual base's offset read (DECISIONS.md E92 (a);
// p3100-null-reference-read*.C has more).  A non-null pointer is not
// reported.  The forms are in p3100-null-reference-rebind.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-reference-rebind.cpp.
// With only .indirection configured (-O0), every form once.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-reference-rebind.h"
// { dg-output "rret 1(\n|\r\n|\r)rebind_call_arg 1(\n|\r\n|\r)rebind_local 1(\n|\r\n|\r)" }
// { dg-output "rebind_noread 1(\n|\r\n|\r)rebind_move 1(\n|\r\n|\r)rebind_cast 1(\n|\r\n|\r)" }
// { dg-output "rebind_base 1(\n|\r\n|\r)rebind_vbase 1(\n|\r\n|\r)rcall_base 1(\n|\r\n|\r)" }
// { dg-output "iread_ref 1(\n|\r\n|\r)scopy_ref 1(\n|\r\n|\r)rcall_member 1(\n|\r\n|\r)" }
// { dg-output "rcall_call 1(\n|\r\n|\r)rcall_store 1(\n|\r\n|\r)rcall_copy 1(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)rvar_call 1(\n|\r\n|\r)rvar_addr 1(\n|\r\n|\r)" }
// { dg-output "rvar_ptr 1(\n|\r\n|\r)rparam_call 1(\n|\r\n|\r)rmember_call 1(\n|\r\n|\r)" }
// { dg-output "rvar_disc 1(\n|\r\n|\r)rebind_ebase 1(\n|\r\n|\r)rvar_ebase_call 1(\n|\r\n|\r)" }
// { dg-output "nonnull_extra 0(\n|\r\n|\r)" }
