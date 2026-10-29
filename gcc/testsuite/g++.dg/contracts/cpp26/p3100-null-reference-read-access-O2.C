// P3100: a glvalue that is already a reference -- a reference variable,
// parameter, member or capture -- indirects through nothing, so a read or
// write through it is not checked again (DECISIONS.md E92 (a)): one null
// dereference, `*p' bound to the reference, is reported once, under
// .indirection, when the reference is then read, stored to, has a member or
// an element read, is copied or assigned from, is converted to a virtual
// base or has a member of one read, or names the object of `.*'.  A pointer
// formed from the reference is a pointer: an indirection through it is
// checked (`S *q = &r; q->m', twice under the group).  A non-null pointer
// is not reported.  The forms are in p3100-null-reference-read.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-reference-read.cpp.
// With only .access configured (-O2).
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-reference-read.h"
// { dg-output "ptrmem_ref 0(\n|\r\n|\r)ptrmem_param 0(\n|\r\n|\r)ptrmem_store 0(\n|\r\n|\r)" }
// { dg-output "store_ref 0(\n|\r\n|\r)store_param 0(\n|\r\n|\r)read_param 0(\n|\r\n|\r)" }
// { dg-output "array_param 0(\n|\r\n|\r)scopy_param 0(\n|\r\n|\r)sassign_param 0(\n|\r\n|\r)" }
// { dg-output "vbase_param 0(\n|\r\n|\r)vbase_read 0(\n|\r\n|\r)member_ref 0(\n|\r\n|\r)" }
// { dg-output "capture_ref 0(\n|\r\n|\r)ptr_from_ref 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
