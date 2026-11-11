// A constant that a lambda's contract assertion names without odr-using it
// is not captured at all ([basic.def.odr], [expr.prim.lambda.capture]), so
// the rule against implicit captures made only for contract assertions
// ([expr.prim.lambda.closure]/10) does not apply.  The provisional capture
// made when the name was seen is pruned once the lambda is complete; the
// check ran before that (GCC-649; stock GCC too).
//
// Mirror: clang/test/Contracts/lambda-contract-only-capture-constant.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

void f ()
{
  const int n = 5;
  constexpr int c = 6;
  auto l1 = [=] { contract_assert (n > 0); };		// OK
  auto l2 = [&] { contract_assert (n > 0); };		// OK
  auto l3 = [=] { contract_assert (c > 0); };		// OK
  auto l4 = [] { contract_assert (n > 0 && c > 0); };	// OK, no capture-default
  auto l5 = [=] pre (n > 0) {};				// OK
  // An odr-use is a capture, and is diagnosed.
  auto l6 = [=] { contract_assert (&n != nullptr); };	// { dg-error "not implicitly captured by a contract assertion" }
  l1 (); l2 (); l3 (); l4 (); l5 (); l6 ();
}

template <class T>
void g ()
{
  const int n = 5;
  auto l1 = [=] { contract_assert (n > 0); };		// OK
  auto l2 = [=] pre (n > 0) {};				// OK
  l1 (); l2 ();
}
template void g<int> ();
