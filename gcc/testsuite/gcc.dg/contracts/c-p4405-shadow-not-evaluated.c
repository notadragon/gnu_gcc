/* P4405 stage 3 in C: with the proof shadow at -O2 the predicate is never
   evaluated at run time, proven or not.  Pinned to -O2.

   Mirror: clang/test/Contracts/C/c-p4405-shadow-not-evaluated.c in the
   llvm_llvm-project fork.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -O2" } */
/* { dg-skip-if "pinned to -O2" { *-*-* } { "-O0" "-O1" "-O3" "-Os" "-Og" "-flto" } { "" } } */

static int evaluated;
__attribute__ ((noinline)) static int touch (int v) { ++evaluated; return v; }

__attribute__ ((noinline)) int
f (int x)
  _Pre (touch (x > 0))
{
  if (x > 0)
    _ContractAssert (touch (x > 0) && x > 0);
  _ContractAssert (touch (0));
  _ContractAssert (++evaluated > 0);
  return x;
}

int
main (void)
{
  if (f (1) + f (-1) != 0 || evaluated != 0)
    __builtin_abort ();
  return 0;
}
