// The GNU extension spelling __contract_assert ICEs.
//
//   g++ -std=c++26 -fsyntax-only contract-assert-alt-spelling-ice.cpp
//
//     -> internal compiler error: in grok_contract, at cp/contracts.cc:2102
//
// Both spellings tokenize to RID_CONTASSERT in c-common.cc, so the extension
// spelling reaches grok_contract exactly as the standard one does.

void f(int x)
{
    __contract_assert(x > 0);   // ICE
}

// CONTROL: the standard spelling is fine, which is what places the defect in
// recognising the token rather than in assertion-statements generally.
void g(int x)
{
    contract_assert(x > 0);
}
