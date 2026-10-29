// P3100: a read through a null pointer of a member of a virtual base of an
// element at a constant subscript, `vbp[1].v', converts `vbp + 1' to the
// virtual base, reading the base's offset through it: reported once
// (.access, DECISIONS.md E94 (b); the check tests vbp, E97 (c)), with
// -fsanitize=vptr routed to the contract-violation handler too and without
// it.  (GCC-861: with -fsanitize=vptr, the vptr check of the conversion
// made the offset read through `vbp + 1' as a temporary of its own, which
// the ubsan pass checked instead of vbp: not reported.)  The form is in
// p3100-null-vptr-vbase-subscript.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-vptr-vbase-subscript.cpp.
// With -fsanitize=vptr routed to the contract-violation handler, at -O2.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-vptr-vbase-subscript.h"

// { dg-output "vbsubrd 1(\n|\r\n|\r)" }
