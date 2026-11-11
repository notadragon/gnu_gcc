/* P4405 stage 3 in C, D4405's proof optimization level: at -O0 the
   default proof level is 2, and the proof shadow, optimized at that level,
   proves what c-p4405-shadow-O0.c's folding cannot.  Pinned to -O0 (the
   torture options are skipped).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -O0" } */
/* { dg-skip-if "pinned to -O0" { *-*-* } { "-O1" "-O2" "-O3" "-Os" "-Og" "-flto" } { "" } } */

int
f (int i, int n)
{
  if (i < n)
    _ContractAssert (i < n);	/* { dg-bogus "(cannot be proven|always false)" } */
  _ContractAssert (2 > 1);	/* { dg-bogus "(cannot be proven|always false)" } */
  return i;
}
