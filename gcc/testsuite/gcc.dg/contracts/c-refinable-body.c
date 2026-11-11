/* The refinable-body rule in C (P4299): a C99 inline definition, or a GNU
   extern inline one, is not the definition a call that is not inlined
   reaches -- that is the external definition in another translation unit,
   which may evaluate the function's contract assertions with another
   semantic (C has no labels, so no evaluation is the same everywhere) -- so
   callers here must not take its return value range: the comparisons in
   use_c99, use_gnu, use_pre and use_post stay.  The static and the external
   definitions are the only definitions of their symbols, so their callers
   fold the comparison; inside the function the trap still folds the
   comparison after the check.  Pinned to -O2 (the torture options are
   skipped).

   Mirror: clang/test/Contracts/C/c-refinable-body.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=quick_enforce -std=gnu11 -O2 -fno-inline -fno-ipa-sra -fdump-tree-optimized" } */
/* { dg-skip-if "pinned to -O2" { *-*-* } { "-O0" "-O1" "-O3" "-Os" "-Og" "-flto" } { "" } } */

#include <contracts.h>

inline int f_c99 (int x) { _ContractAssert (x > 0); return x; }
extern inline __attribute__ ((gnu_inline)) int
f_gnu (int x) { _ContractAssert (x > 0); return x; }
inline int f_pre (int x) _Pre (x > 0) { return x; }
inline int f_post (int x) _Post (r: r > 0) { return x; }
static int f_s (int x) { _ContractAssert (x > 0); return x; }
int f_e (int x) { _ContractAssert (x > 0); return x; }
int h_e (int x) { _ContractAssert (x > 0); return x > 0 ? 1234 : 5678; }

int use_c99 (int y) { return f_c99 (y) > 0; }
int use_gnu (int y) { return f_gnu (y) > 0; }
int use_pre (int y) { return f_pre (y) > 0; }
int use_post (int y) { return f_post (y) > 0; }
int use_s (int y) { return f_s (y) > 0; }
int use_e (int y) { return f_e (y) > 0; }

/* { dg-final { scan-tree-dump-times "= _\[0-9\]+ > 0;" 4 "optimized" } } */
/* { dg-final { scan-tree-dump "return 1234;" "optimized" } } */
/* { dg-final { scan-tree-dump-not "5678" "optimized" } } */
