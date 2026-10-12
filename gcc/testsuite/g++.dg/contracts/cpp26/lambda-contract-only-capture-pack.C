// A parameter pack odr-used only in a lambda's contract assertions is not
// implicitly captured ([expr.prim.lambda.closure]/10), as a single parameter
// is not.  A pack is captured only when its elements are known, at
// instantiation, where nothing checked for it (GCC-648; stock GCC too).
//
// Mirror: clang/test/Contracts/lambda-contract-only-capture-pack.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

template <class... Ts>
void p (Ts... ts)
{
  auto l = [=] { contract_assert (((ts > 0) && ...)); };	// { dg-error "not implicitly captured by a contract assertion" }
  auto m = [&] { contract_assert (((ts > 0) && ...)); };	// { dg-error "not implicitly captured by a contract assertion" }
  auto n = [=] { contract_assert (((ts > 0) && ...)); return (ts + ...); };	// OK, also used outside
  auto o = [ts...] { contract_assert (((ts > 0) && ...)); };	// OK, explicit
  l (); m (); n (); o ();
}
template void p (int, int);

void q (int t)
{
  auto l = [=] { contract_assert (t > 0); };	// { dg-error "not implicitly captured by a contract assertion" }
  l ();
}

// Diagnosed once, in the template, for a capture that is not a pack.
template <class T>
void r (T t)
{
  auto l = [=] { contract_assert (t > 0); };	// { dg-error "not implicitly captured by a contract assertion" }
  l ();
}
template void r (int);
