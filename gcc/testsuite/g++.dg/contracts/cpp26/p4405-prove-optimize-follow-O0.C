// D4405's proof optimization level (DECISIONS.md E162-E168, E205):
// -fcontracts-prove-optimize=follow at -O0 is level 0, folding only, so a
// function optimized at -O2 by its own attribute gets no proof shadow
// either: the attribute does not alter the proofs (E168 (7)).  At the
// default level, 2, it is proven (p4405-prove-optimize-default-O0.C).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0 -fcontracts-prove-optimize=follow" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove" }

[[gnu::optimize ("O2")]] int f (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-error "contract assertion cannot be proven" }
  return i;
}
