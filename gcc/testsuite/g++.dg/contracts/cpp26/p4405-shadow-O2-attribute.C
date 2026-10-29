// P4405 stage 3: a function optimized at -O2 by its own attribute gets a
// proof shadow in a -O0 compilation.  It comes first: an error issued
// before any evaluation is deferred stops the compilation before shadows
// are optimized, and in a -O0 compilation nothing is deferred until then.
//
// Mirror: none (GCC's optimize attribute).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove -fmax-errors=1" }

[[gnu::optimize ("O2")]] int f (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-bogus "cannot be proven|always false" }
  return i;
}

int g (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-error "contract assertion cannot be proven" }
  return i;
}
