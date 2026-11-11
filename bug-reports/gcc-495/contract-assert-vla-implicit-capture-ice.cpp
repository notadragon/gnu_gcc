// A VLA named only in a contract_assert inside a [&] lambda crashes GCC.
// g++ -std=c++26 -fcontracts -fsyntax-only contract-assert-vla-implicit-capture-ice.cpp
// Expected: error: 'a' is not implicitly captured by a contract assertion
// (as for a fixed-size array).  Actual: internal compiler error (in
// set_contract_capture_flag).
void v (int n)
{
  int a[n];
  auto l = [&] { contract_assert (a[0] == 0); };
  l ();
}
