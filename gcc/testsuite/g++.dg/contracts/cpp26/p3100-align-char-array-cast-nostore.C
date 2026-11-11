// P3100: the alignment check of an access
// (ub:basic.align.object.alignment.store) reports a misaligned load,
// store, member read, class copy or subscript through a pointer an
// explicit cast formed from a character array -- `*(int *) (buf + o)',
// `*(int *) &buf[o]', a local array, a member -- once each.  The mirror of
// llvm_llvm-project's Runnable/p3100-align-char-array-cast.cpp
// (CLANG-810, where the check missed them); passing on GCC before any
// change except the class copy, reported twice before GCC-879.  The forms
// are in p3100-align-char-array-cast.h.
// With only .nostore configured, nothing is reported.  At -O0.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-align-char-array-cast.h"

// { dg-output "gload 0(\n|\r\n|\r)lload 0(\n|\r\n|\r)gstore 0(\n|\r\n|\r)" }
// { dg-output "gmem 0(\n|\r\n|\r)gmemb 0(\n|\r\n|\r)gmemd 0(\n|\r\n|\r)" }
// { dg-output "gcopy 0(\n|\r\n|\r)gsub 0(\n|\r\n|\r)gidx0 0(\n|\r\n|\r)" }
// { dg-output "gidx 0(\n|\r\n|\r)gsum2 0(\n|\r\n|\r)iarr 0(\n|\r\n|\r)" }
// { dg-output "pload 0(\n|\r\n|\r)pmem 0(\n|\r\n|\r)gref 0(\n|\r\n|\r)" }
// { dg-output "packed 0(\n|\r\n|\r)aligned 0(\n|\r\n|\r)" }
