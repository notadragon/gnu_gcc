// A VLA named only in a contract_assert inside a [&] lambda is captured only
// by the contract assertion, which is ill-formed.  Its capture proxy rebuilds
// the array from the {pointer, bound} capture struct, which the check has to
// look through to find the capture (it used to crash), and the diagnostic
// names the array.  A VLA is otherwise handled like any other entity when a
// nested lambda captures it: outside the contract assertion it is used
// there too and is accepted, inside it is still used only in the contract
// assertion, and an explicit capture is accepted.
//
// A use outside the contract assertion that comes AFTER the one inside it
// finds the VLA's capture proxy by name rather than the VLA, so it has to
// clear the mark the contract assertion's capture left (GCC-670: these were
// diagnosed).  A lambda in the predicate that names the proxy does not.
//
// Mirror: clang/test/Contracts/contract-assert-vla-implicit-capture.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -Wno-vla" }

void v (int n)
{
  int a[n];
  auto l = [&] { contract_assert (a[0] == 0); };	// { dg-error "'a' is not implicitly captured by a contract assertion" }
  l ();
}

void nested_outside (int n)
{
  int a[n];
  auto l = [&] {
    contract_assert (a[0] == 0);
    [&] { (void) a[0]; } ();
  };
  l ();
}

void nested_inside (int n)
{
  int a[n];
  auto l = [&] {
    contract_assert ([&] { return a[0] == 0; } ());	// { dg-error "'a' is not implicitly captured by a contract assertion" }
  };
  l ();
}

void explicit_capture (int n)
{
  int a[n];
  auto l = [&a] { contract_assert (a[0] == 0); };
  l ();
}

void after (int n)
{
  int a[n];
  auto l = [&] {
    contract_assert (a[0] == 0);
    (void) a[0];
  };
  l ();
}

void nested_after (int n)
{
  int a[n];
  auto l = [&] {
    contract_assert (a[0] == 0);
    [&] { (void) a[0]; } ();
    [&] { (void) a[0]; } ();
  };
  l ();
}

void nested_inside_after (int n)
{
  int a[n];
  auto l = [&] {
    contract_assert (a[0] == 0				// { dg-error "'a' is not implicitly captured by a contract assertion" }
		     && [&] { return a[0] == 0; } ());
  };
  l ();
}
