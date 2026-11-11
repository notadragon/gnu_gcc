// P4405 stage 3, D4405's proof optimization level (DECISIONS.md E168 (7)):
// a function's own optimize attribute does not alter the proofs.  In a -O0
// compilation the proof shadows are optimized at the proof level, 2 by
// default, whatever their functions' own levels, so f, optimized at -O2 by
// its attribute, and g, unoptimized, are proven alike.  At proof level 0
// neither is (p4405-prove-optimize-follow-O0.C).
//
// Mirror: none (GCC's optimize attribute).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove" }

[[gnu::optimize ("O2")]] int f (int i, int n)
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
