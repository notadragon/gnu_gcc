// D4405 ("The Analysis Evaluation"): a sound proof of an analysis
// evaluation may drop the runtime evaluations it covers in the same code,
// and with -fcontracts-prove-assume (implied by -fcontracts-p4405) the
// proof is an assumption, so the check that follows it goes away.  In
// sound (), as in p4405-prove-assume.C, the shadow inlines the static
// ns::h, whose enforced precondition proves y > 0 soundly, while the real
// function calls h out of line: the assumption made of that proof removes
// the enforced check of the same contract_assert, which comes after its
// analysis evaluation.  A caller's proof never removes a callee-side
// check: kk::k (y), proven at its call, keeps its own check, as does h.
// p4405-analysis-drop-off.C has -fno-contracts-prove-assume.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2" }
// { dg-additional-options "--param large-function-insns=40 --param large-function-growth=10" }
// { dg-additional-options "--param max-inline-insns-single=200 --param max-inline-insns-auto=200 -fno-partial-inlining" }
// { dg-additional-options "-fdump-tree-optimized" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-analysis-drop.json" }

int sink (int);
int big (int);

namespace ns {
static int h (int x) pre (x > 0)	// enforce
{
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}
}

namespace kk {
[[gnu::noipa]] int k (int x) pre (x > 0)	// enforce, analysis prove
{
  return sink (x);
}
}

int sound (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// line 41: assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  int r = ns::h (y);
  contract_assert (y > 0);	// line 49: enforce, analysis prove
  return r + sink (y / 4);
}

int call_k (int y)
{
  if (y > 0)
    return kk::k (y);
  return 0;
}

// { dg-final { scan-tree-dump {ns::h[.a-z0-9]* \(y_[0-9]+\(D\)\)} "optimized" } }
// { dg-final { scan-tree-dump-not "__cxa_contract_violation_assert_enforce_pf" "optimized" } }
// { dg-final { scan-tree-dump-times "__cxa_contract_violation_pre_enforce_pf \\(" 2 "optimized" } }
