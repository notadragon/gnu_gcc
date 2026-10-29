// P4405 stage 3: with -ffat-lto-objects the proof shadow is still
// diagnosed once, at the compile step, after the early optimizations, and
// neither the LTO stream nor the object code holds it.  diagnose, so that
// there is object code to scan.
//
// Mirror: none (GCC's fat LTO objects).
// { dg-do compile { target c++26 } }
// { dg-require-effective-target lto }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2 -flto -ffat-lto-objects" }
// { dg-additional-options "-fcontract-evaluation-semantic=diagnose" }

int f (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-bogus "cannot be proven|always false" }
  contract_assert (i < n);	// { dg-warning "contract assertion cannot be proven" }
  return i;
}

// { dg-final { scan-assembler-not "contract_shadow" } }
