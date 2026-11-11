/* DECISIONS.md E179 (1) (GCC-970): -fcontracts-p3850 turns C contracts on
   itself, so a C link with it pulls in libcontracts even when
   -fno-contracts-p3100 turns off the P3100 option it implies: no explicit
   -l in the test (contracts-implicit-link.c for -fcontracts-p4299).

   Mirror: clang/test/Contracts/p3100-c-link-driver.c in the
   llvm_llvm-project fork.  */
/* { dg-do run } */
/* { dg-options "-std=c23 -fcontracts-p3850 -fno-contracts-p3100 -fcontract-evaluation-semantic=observe" } */

int f (int x) _Pre (x > 0) { return x; }

int
main (void)
{
  return f (1) == 1 ? 0 : 1;
}
