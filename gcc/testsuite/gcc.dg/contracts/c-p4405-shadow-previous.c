/* P4405 stage 3 in C: the proof shadow assumes an earlier observed
   contract assertion (a conditional proof) and an earlier enforced
   precondition (a sound one), as the dump shows.  Pinned to -O2.

   Mirror: clang/test/Contracts/C/c-p4405-shadow-previous.c in the
   llvm_llvm-project fork, with its own way of showing the result.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -O2 -fdump-tree-contract_shadow" } */
/* { dg-additional-options "-fcontract-configuration-file=${srcdir}/gcc.dg/contracts/c-p4405-shadow-previous.json" } */
/* { dg-skip-if "pinned to -O2" { *-*-* } { "-O0" "-O1" "-O3" "-Os" "-Og" "-flto" } { "" } } */

int sink (int);

int
f (int x)
  _Pre (x > 0)			/* line 16: enforce */
{
  _ContractAssert (x > 0);	/* line 18 */
  return sink (x);
}

int
g (int x)
{
  _ContractAssert (x > 0);	/* line 25: observe */
  _ContractAssert (x > 0);	/* line 26 */
  return sink (x);
}

/* { dg-final { scan-tree-dump {\(line 18\): sound} "contract_shadow" } } */
/* { dg-final { scan-tree-dump {\(line 26\): conditional} "contract_shadow" } } */
