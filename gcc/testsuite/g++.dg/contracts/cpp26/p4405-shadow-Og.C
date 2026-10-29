// P4405 stage 3: at -Og the proof shadow runs, through -Og's own
// optimizations: what they show is proven (a constant propagated from a
// local variable), what needs more is not (a dominating condition needs
// the value-range passes -Og does not run).
//
// Mirror: none (GCC's -Og).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -Og" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove" }

int f (int x)
{
  int k = 4;
  contract_assert (k == 4);	// { dg-bogus "cannot be proven|always false" }
  contract_assert (x > 0);	// { dg-error "contract assertion cannot be proven" }
  return x + k;
}
