// gcc-649-contract-only-capture-constant.cpp                     -*-C++-*-
//
// GCC-649: a constant named in a lambda's contract assertion without being
// odr-used (its value is read: [basic.def.odr], usable in constant
// expressions) is neither captured nor in need of capture, but with a
// capture-default GCC reports it as "not implicitly captured by a contract
// assertion".  Without a capture-default it is accepted (line C).
//
// Expected: accepted.  Actual: errors on lines A and B.
//
// g++ -std=c++26 -fcontracts -fsyntax-only gcc-649-contract-only-capture-constant.cpp

void f ()
{
  const int n = 5;
  auto l1 = [=] { contract_assert (n > 0); };  // A
  auto l2 = [&] { contract_assert (n > 0); };  // B
  auto l3 = [] { contract_assert (n > 0); };   // C
}
