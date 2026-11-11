// D4405's proof optimization level (DECISIONS.md E162-E168, E205): the
// default at -O0 is level 2, so the proof shadow is on, and the shadows
// are optimized at level 2 although the code is not optimized: f and g are
// proven as at -O2.  f's own optimize attribute does not alter the proofs
// (E168 (7)).  With -fcontracts-prove-optimize=follow, level 0, only
// folding proves (p4405-prove-optimize-follow-O0.C).
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
