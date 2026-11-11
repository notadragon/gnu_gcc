// OPEN BUG (GCC-1001 in gnu_gcc open-issues/; deferred, DECISIONS.md E212
// (c): a documented limitation).
//
// D4405 under -fcontracts-prove-assume (on with -fcontracts-p4405): a
// function that only an unproven obligation's predicate calls, and that
// is not inlined, is emitted.  The real function holds an assumption
// region for each obligation, `if (.CONTRACT_PROVEN (id)) <assume pred>',
// which keeps what the predicate calls reachable through the IPA passes;
// the region goes only when the function is expanded, after the shadows
// have decided the obligation, and by then the set of functions to emit
// is fixed.  So h (inline, noinline) and s (static, noinline) are emitted
// out of line though no code calls them; with every evaluation ignore, or
// with -fno-contracts-prove-assume, they are not.  They should not be;
// the XFAILs record that.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

extern int g (int);
[[gnu::noinline]] inline bool h (int n) { return g (n) > 0; }
[[gnu::noinline]] static bool s (int n) { return g (n + 1) > 0; }

int f (int n)
{
  contract_assert (h (n));
  contract_assert (s (n));
  return n;
}

// { dg-final { scan-assembler-not "_Z1hi" { xfail *-*-* } } }
// { dg-final { scan-assembler-not "_ZL1si" { xfail *-*-* } } }
