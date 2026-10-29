/* P4405 stage 3 in C, -fcontracts-prove-shadow (implied by
   -fcontracts-p4405): at -O2 a contract assertion or a precondition the
   proof shadow proves is not diagnosed, one it does not is.  A
   postcondition is proven at every return (c-p4405-shadow-post.c).
   Pinned to -O2 (the torture options are skipped); c-p4405-shadow-O0.c has
   -O0.

   Mirror: clang/test/Contracts/C/c-p4405-shadow-prove.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -O2" } */
/* { dg-skip-if "pinned to -O2" { *-*-* } { "-O0" "-O1" "-O3" "-Os" "-Og" "-flto" } { "" } } */

static int clamp (int v) { return v < 0 ? 0 : v > 9 ? 9 : v; }

int
f (int i, int n)
  _Pre (n >= 0)			/* { dg-error "contract assertion cannot be proven" } */
  _Post (r: r >= 0)		/* { dg-bogus "cannot be proven|always false" } */
{
  if (i >= 0 && i < n)
    _ContractAssert (i < n);	/* { dg-bogus "cannot be proven|always false" } */
  int c = clamp (i);
  _ContractAssert (c <= 9);	/* { dg-bogus "cannot be proven|always false" } */
  _ContractAssert (c < 9);	/* { dg-error "contract assertion cannot be proven" } */
  return c;
}
