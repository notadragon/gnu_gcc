// gcc-648-contract-only-capture-pack.cpp                          -*-C++-*-
//
// GCC-648: a function parameter pack named only in a contract assertion of
// a lambda with a capture-default is implicitly captured, silently.  An
// entity used only in contract assertions is not implicitly captured; it
// must be captured explicitly (P2900 [expr.prim.lambda.capture]).  A single
// parameter is diagnosed (line B).
//
// Expected: errors on lines A and B ("not implicitly captured by a contract
// assertion").  Actual: only line B.
//
// g++ -std=c++26 -fcontracts -fsyntax-only gcc-648-contract-only-capture-pack.cpp

template <class... Ts> void p (Ts... ts)
{
  auto l = [=] { contract_assert (((ts > 0) && ...)); };  // A
  l ();
}
template void p (int, int);

void q (int t)
{
  auto l = [=] { contract_assert (t > 0); };              // B
  l ();
}
