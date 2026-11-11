// D4405's proof optimization level (DECISIONS.md E162-E168, E166): in a
// -O0 build IPA pure-const, which is off at -O0, still runs for the
// functions optimized at the proof level -- here f's shadow -- and only for
// them: the real sq and f are not analyzed.  sq is never inlined; the
// level copy of it that the shadow calls is found const, so the two calls
// in the predicate are one, the assertion is proven, as at -O2, and the
// copy is gone before IPA.  The object code is the same as without the
// shadows.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0" }
// { dg-additional-options "-fcontract-evaluation-semantic=diagnose -fdump-ipa-pure-const" }

[[gnu::noinline]] int sq (int x) { return x * x; }

void f (int n)
{
  contract_assert (sq (n) == sq (n));	// { dg-bogus "cannot be proven" }
}

// { dg-final { scan-ipa-dump "local analysis of void _Z1fi\\.contract_shadow_0" "pure-const" } }
// { dg-final { scan-ipa-dump-not "local analysis of int sq" "pure-const" } }
// { dg-final { scan-ipa-dump-not "local analysis of void f" "pure-const" } }
// { dg-final { contracts-same-diagnostics "-O2" "" } }
// { dg-final { contracts-same-object-code "-fcontract-evaluation-semantic=ignore" "" "-fcontracts-prove-optimize=0 -Wno-contract-unproven" } }
