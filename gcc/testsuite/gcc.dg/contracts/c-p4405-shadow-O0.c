/* P4405 stage 3 in C: at -O0 there is no proof shadow, so folding is
   final.  Pinned to -O0 (the torture options are skipped).

   Mirror: clang/test/Contracts/C/c-p4405-shadow-O0.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -O0" } */
/* { dg-skip-if "pinned to -O0" { *-*-* } { "-O1" "-O2" "-O3" "-Os" "-Og" "-flto" } { "" } } */

int
f (int i, int n)
{
  if (i < n)
    _ContractAssert (i < n);	/* { dg-error "contract assertion cannot be proven" } */
  _ContractAssert (2 > 1);	/* { dg-bogus "cannot be proven|always false" } */
  return i;
}
