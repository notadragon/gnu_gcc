/* P3100 in C: a contract assertion, precondition or postcondition whose
   semantic is assume is an optimizer assumption, as [[assume]] is: the
   test each one rules out is removed (the postcondition's where its
   function is inlined), and the predicate is never evaluated
   (c-allow-assume-semantic.c).  Pinned to -O2 (the torture options are
   skipped).

   Clang: CLANG-111 in the llvm_llvm-project fork (its assume semantic
   lowers to ignore).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume -O2 -fdump-tree-optimized" } */
/* { dg-skip-if "pinned to -O2" { *-*-* } { "-O0" "-O1" "-O3" "-Os" "-Og" "-flto" } { "" } } */

extern void dead_pre (void);
extern void dead_post (void);
extern void dead_assert (void);
extern int opaque (int);

int
pre_assumed (int x) _Pre (x > 0)
{
  if (x <= 0)
    dead_pre ();
  return x;
}

static int
post_assumed (int x) _Post (r: r != 7)
{
  return opaque (x);
}

int
use_post (int x)
{
  int r = post_assumed (x);
  if (r == 7)
    dead_post ();
  return r;
}

int
assert_assumed (int x)
{
  _ContractAssert (x != 3);
  if (x == 3)
    dead_assert ();
  return x;
}

/* { dg-final { scan-tree-dump-not "dead_pre" "optimized" } } */
/* { dg-final { scan-tree-dump-not "dead_post" "optimized" } } */
/* { dg-final { scan-tree-dump-not "dead_assert" "optimized" } } */
