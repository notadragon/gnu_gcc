// P3100: an evaluation with the assume semantic is an optimizer assumption,
// lowered as [[assume]] lowers its own: the optimizers may take the
// predicate to hold where the evaluation happens.  Here a test that a
// precondition, a postcondition (seen by the caller its function is inlined
// into) or a contract_assert rules out is removed as dead code, also for a
// predicate calling a function, which is outlined, never evaluated.
// p3100-semantic-assume-run.C shows the predicate is never evaluated.
//
// Clang: CLANG-111 in the llvm_llvm-project fork (its assume semantic
// lowers to ignore).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume -O2 -fdump-tree-optimized" }

extern void dead_pre ();
extern void dead_post ();
extern void dead_assert ();
extern void dead_outlined ();
extern void live ();
extern int opaque (int);

int pre_assumed (int x)
  pre (x > 0)
{
  if (x <= 0)
    dead_pre ();
  return x;
}

static int post_assumed (int x)
  post (r: r != 7)
{
  return opaque (x);
}

int use_post (int x)
{
  int r = post_assumed (x);
  if (r == 7)
    dead_post ();
  return r;
}

int assert_assumed (int x)
{
  contract_assert (x != 3);
  if (x == 3)
    dead_assert ();
  return x;
}

int outlined_assumed (int x)
  pre (x > 10 && opaque (x) > 0)
{
  if (x <= 10)
    dead_outlined ();
  if (x == 11)
    live ();
  return x;
}

// { dg-final { scan-tree-dump-not "dead_pre" "optimized" } }
// { dg-final { scan-tree-dump-not "dead_post" "optimized" } }
// { dg-final { scan-tree-dump-not "dead_assert" "optimized" } }
// { dg-final { scan-tree-dump-not "dead_outlined" "optimized" } }
// { dg-final { scan-tree-dump "live" "optimized" } }
