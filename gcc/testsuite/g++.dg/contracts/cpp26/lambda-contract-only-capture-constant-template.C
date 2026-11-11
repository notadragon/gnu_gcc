// A constant odr-used only in a contract assertion of a lambda in a
// template is captured, which [expr.prim.lambda.closure]/10 forbids, and is
// diagnosed when the lambda is instantiated (GCC-653).  The instantiation
// keeps its captures of constants unpruned, but the uses that are not
// odr-uses have been folded away.  The non-template case is
// lambda-contract-only-capture-constant.C.
//
// Mirror: clang/test/Contracts/lambda-contract-only-capture-constant-template.cpp
// in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

template <class T>
void g ()
{
  const int n = 5;
  auto l1 = [=] { contract_assert (n > 0); };		// OK
  auto l2 = [=] { contract_assert (&n != nullptr); };	// { dg-error "not implicitly captured by a contract assertion" }
  auto l3 = [=] { const int *p = &n; contract_assert (&n == p); };	// OK
  l1 (); l2 (); l3 ();
}
template void g<int> ();
