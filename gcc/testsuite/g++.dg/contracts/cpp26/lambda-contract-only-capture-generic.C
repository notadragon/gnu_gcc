// A generic lambda whose precondition, or whose only statement, a
// contract_assert, names a variable captured only for it: diagnosed, and the
// instantiation that follows does not crash on the capture the pattern had
// and the instantiation does not.
//
// Mirror: clang/test/Contracts/lambda-contract-only-capture.cpp (gen) in
// the llvm_llvm-project fork (CLANG-597).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void h (int t)
{
  auto t1 = [=] (auto u) pre (u > t) {};	// { dg-error "not implicitly captured" }
  t1 (1);
  auto t2 = [&] (auto u) { contract_assert (u > t); };	// { dg-error "not implicitly captured" }
  t2 (1);
}
