/* P4405 stage 3 in C: a postcondition is one evaluation, however many
   returns its function has, and the proof shadow proves it at every return
   (a marker after each return value is bound to the result name) or
   diagnoses it once, at its declaration.  Falling off the end of a void
   function is a return too.  Pinned to -O2 (the torture options are
   skipped).

   Mirror: clang/test/Contracts/C/c-p4405-shadow-post.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -O2 -fdump-tree-contract_shadow" } */
/* { dg-skip-if "pinned to -O2" { *-*-* } { "-O0" "-O1" "-O3" "-Os" "-Og" "-flto" } { "" } } */

int sink (int);
int g;

int
clamped (int v)
  _Post (r: r >= 0 && r <= 9)	/* { dg-bogus "cannot be proven|always false" } */
{
  if (v < 0)
    return 0;
  if (v > 9)
    return 9;
  return v;
}

int
partly (int v)
  _Post (r: r >= 0)		/* { dg-error "contract assertion cannot be proven" } */
{
  if (v < 0)
    return v;
  return sink (v) & 7;
}

void
store (int v)
  _Post (g >= 0)		/* { dg-bogus "cannot be proven|always false" } */
{
  if (v < 0)
    {
      g = 0;
      return;
    }
  g = v;
}

/* { dg-final { scan-tree-dump-times "contract obligation 0 \\(line 19\\): sound" 1 "contract_shadow" } } */
