/* The violation arm of a C contract check is marked unlikely, under every
   semantic that checks (notadragon_wg21 DECISIONS.md E63 (a)): one
   cold-label prediction per check -- here a precondition, a postcondition
   and a contract assertion under observe.

   Mirror: clang/test/Contracts/C/c-contract-check-unlikely.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe -fdump-tree-original" } */

#include <contracts.h>

int f (int x) _Pre (x > 0) _Post (r : r > 0) { _ContractAssert (x != 3); return x; }

/* { dg-final { scan-tree-dump-times "predicted unlikely by cold label predictor" 3 "original" } } */
