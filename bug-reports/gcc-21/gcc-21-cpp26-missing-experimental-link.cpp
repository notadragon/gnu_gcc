// GCC-21 / PR126158: `-std=c++26` alone does not link the contract
// violation handler.
//
// Freshly constructed (not extracted from a test -- there is no test:
// this is a driver/link-model gap, not a compiler-proper defect). Compile
// and link with just:
//
//   g++ -std=c++26 gcc-21-cpp26-missing-experimental-link.cpp -o out
//
// On stock g++ (both 16.2.0 and trunk 17.0.0 20260901), this fails to
// link:
//
//   undefined reference to `handle_contract_violation
//     (std::contracts::contract_violation const&)'
//
// Adding -lstdc++exp explicitly makes it link. This branch's own
// gcc/cp/g++spec.cc auto-adds that library (and -lcontracts) whenever
// -std=c++26 (or gnu++26, c++29, gnu++29) is selected, precisely to avoid
// needing this -- see the parent .md's Notes for how that was confirmed.

int
half (int x) pre (x >= 0) post (r : r >= 0)
{
  return x / 2;
}

int
main ()
{
  return half (4) == 2 ? 0 : 1;
}
