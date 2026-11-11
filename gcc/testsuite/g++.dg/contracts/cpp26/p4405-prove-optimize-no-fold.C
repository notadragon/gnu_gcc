// D4405's proof optimization level (DECISIONS.md E162-E168, E205):
// -fno-contracts-prove-fold wins over -fcontracts-prove-optimize=0, where
// folding is the only strategy on, so nothing is proven.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-prove-optimize=0 -fno-contracts-prove-fold -fcontract-evaluation-semantic=prove" }

void f ()
{
  contract_assert (true);	// { dg-error "contract assertion cannot be proven" }
  contract_assert (false);	// { dg-error "contract assertion cannot be proven" }
}
