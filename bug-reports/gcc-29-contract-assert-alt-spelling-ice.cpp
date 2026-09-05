// gcc-29-contract-assert-alt-spelling-ice.cpp                       -*-C++-*-
//
// GCC-29: the GNU extension spelling `__contract_assert` ICEs.
//
//   g++ -std=c++26 -fcontracts -fsyntax-only \
//       gcc-29-contract-assert-alt-spelling-ice.cpp
//
// Stock g++ trunk:
//   internal compiler error: in grok_contract, at cp/contracts.cc:2102
//
// Both spellings tokenize to RID_CONTASSERT in c-common.cc, so the extension
// spelling reaches grok_contract exactly as the standard one does -- but
// grok_contract recognised only the standard spelling and fell off the end.
//
// PLAIN -fcontracts.  Nothing of ours is involved.

void
f (int x)
{
  __contract_assert (x > 0);   // ICE
}

// CONTROL: the standard spelling is fine, which is what says the defect is in
// recognising the token rather than in assertion-statements generally.
void
g (int x)
{
  contract_assert (x > 0);
}
