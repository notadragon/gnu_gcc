// P4405 stage 3: a function optimized at -O0 by its own attribute gets no
// proof shadow in a -O2 compilation, so folding is final for it.
//
// Mirror: none (GCC's optimize attribute).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove -fmax-errors=1" }

[[gnu::optimize ("O0")]] int f (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-error "contract assertion cannot be proven" }
  return i;
}

int g (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-bogus "cannot be proven|always false" }
  return i;
}
