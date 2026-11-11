// GCC-503 in this repository's bug-reports/ (stock GCC has it too): a capture
// an enclosing lambda makes only on behalf of a lambda inside its contract
// assertion is a capture by that contract assertion, which is ill-formed --
// the standard's example f7 in [expr.prim.lambda.closure] ("outer capture of
// y is invalid").
//
// Mirror: clang/test/Contracts/lambda-contract-only-capture.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void h ()
{
  bool y = true;
  auto n1 = [=] pre ([=] { return y; } ()) {};	// { dg-error "captured" }
  auto n2 = [=] pre ([=] { return y; } ()) { return y; };	// OK, used outside the predicate too
}
