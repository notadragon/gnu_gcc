/* P4405 in C: -fsyntax-only emits no checks, so prove and diagnose
   diagnose nothing.

   Mirror: clang/test/Contracts/C/c-p4405-syntax-only.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -fsyntax-only" } */

int
f (int x)
  _Pre (x > 0)			/* { dg-bogus "cannot be proven" } */
{
  _ContractAssert (x != 3);	/* { dg-bogus "cannot be proven" } */
  return x;
}
