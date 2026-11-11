// D4405 proof shadows and -g (GCC-997): the premise shadows and premise
// copies (-fcontracts-prove-callee-premises, implied by -fcontracts-p4405)
// leave the debug information as it is without them, as the ordinary
// shadows do (p4405-shadow-debug-info.C).  p4405-shadow-callee-premises.C's
// shapes, under observe: each h gets a premise shadow, and idx, idx2, k and
// take premise copies, which the premise shadows inline.  The object code,
// the debug sections included, is the same with them, without the copies,
// without any shadow and at proof level 0, at -O0, -O1/-Og (shadows and
// copies at proof level 2, with level copies) and -O2/-O3.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -g -gno-record-gcc-switches" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=observe -Wno-contract-unproven" }

#include <contracts>
using namespace std::contracts::labels;

int idx (int x) post (r : r == 0) { return x; }
void h1 (int x)
{
  int i = idx (x);
  contract_assert<diagnose> (i == 0);
}

inline int idx2 (int x)
{
  contract_assert (x == 0);
  return x;
}
void h3 (int x)
{
  int i = idx2 (x);
  contract_assert<diagnose> (i == 0);
}

int k (int x) { return idx (x) + 1; }
void h4 (int x)
{
  int i = k (x);
  contract_assert<diagnose> (i == 1);
}

void take (int x) pre (x == 0) {}
void h5 (int x)
{
  take (x);
  contract_assert<diagnose> (x == 0);
}

// { dg-final { contracts-same-object-code "-O0 -fno-contracts-prove-shadow" "-O0" "-O0 -fno-contracts-prove-callee-premises" "-O0 -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-O1 -fno-contracts-prove-shadow" "-O1" "-O1 -fno-contracts-prove-callee-premises" "-O1 -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-Og -fno-contracts-prove-shadow" "-Og" "-Og -fno-contracts-prove-callee-premises" "-Og -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-O2 -fno-contracts-prove-shadow" "-O2" "-O2 -fno-contracts-prove-callee-premises" "-O2 -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-O3 -fno-contracts-prove-shadow" "-O3" "-O3 -fno-contracts-prove-callee-premises" "-O3 -fcontracts-prove-optimize=0" } }
