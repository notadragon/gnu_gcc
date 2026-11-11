// P3100: with -fsanitize=vptr routed to the contract-violation handler and
// ub:expr.unary.dereference.nullptr configured, the vptr check of a member
// call or access on an object at offset zero from a null pointer -- a
// function or member of a primary base (`mp->W2::w2 ()', `mp->nw ()',
// `&mp->r', `((*mp).*p) ()') or of a member subobject at offset zero
// (`hp->m.nv ()', `&hp->m.v') -- skips the null pointer (GCC-856), and
// the P3100 check reports each once: the program does not fault.  A call
// through `->*' on the base is the control.  With the group ignored, the
// vptr check is stock GCC's, made as without -fcontracts-p3100, which tests
// no null pointer and reads the vtable pointer through it (Clang's skips a
// null pointer: the GCC-866 divergence, accepted by the user, DECISIONS.md
// E119 (a)).  The forms are in p3100-null-vptr-offset-zero.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-vptr-offset-zero.cpp (CLANG-797).
// With the group ignored, at -O0.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-vptr-offset-zero.h"

// { dg-output "mwq 0 fault(\n|\r\n|\r)mwnw 0 fault(\n|\r\n|\r)" }
// { dg-output "mwaddr 0(\n|\r\n|\r)mwdpmf 0 fault(\n|\r\n|\r)" }
// { dg-output "h0nv 0 fault(\n|\r\n|\r)h0addr 0(\n|\r\n|\r)" }
// { dg-output "mwpmf 0 fault(\n|\r\n|\r)" }
