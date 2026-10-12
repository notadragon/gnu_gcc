// A contract assertion in a generic lambda follows the capture rules a
// non-generic lambda's does: an entity odr-used only in contract assertions
// is not implicitly captured, one used outside them as well is, and an
// explicit capture is always fine.
//
// Mirror: clang/test/Contracts/generic-lambda-contract-capture.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

int only (int x)	// { dg-message "declared here" }
{
  auto l = [&] (auto a) { contract_assert (x > a); return a; };	// { dg-error "not implicitly captured by a contract assertion" }
  return l (1);
}

int also_outside (int x)
{
  auto l = [&] (auto a) { contract_assert (x > a); return a + x; };
  return l (1);
}

int explicit_capture (int x)
{
  auto l = [&x] (auto a) { contract_assert (x > a); return a; };
  return l (1);
}

int in_pre (int x) pre ([&] (auto k) { return x + k; } (1) > 0) { return x; }
int in_post () post (r: [&] (auto k) { return r + k; } (1) > 0) { return 5; }
