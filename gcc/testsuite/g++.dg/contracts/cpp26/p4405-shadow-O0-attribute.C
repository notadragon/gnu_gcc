// P4405 stage 3, D4405's proof optimization level (DECISIONS.md E168 (7)):
// a function's own optimize attribute does not alter the proofs.  f,
// optimized at -O0 by its attribute in a -O2 compilation, gets a proof
// shadow optimized at the proof level, as g does, and its evaluation is
// proven.  The shadow does not keep the attribute.
//
// Mirror: none (GCC's optimize attribute).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove -fdump-ipa-contract_shadow_clone" }

[[gnu::optimize ("O0")]] int f (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-bogus "(cannot be proven|always false)" }
  return i;
}

int g (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-bogus "(cannot be proven|always false)" }
  return i;
}

// { dg-final { scan-ipa-dump "Shadow \[^\n\]*_Z1fii\\.contract_shadow_\[0-9\]" "contract_shadow_clone" } }
// { dg-final { scan-ipa-dump-not "contract shadow \[^\n\]*optimize" "contract_shadow_clone" } }
