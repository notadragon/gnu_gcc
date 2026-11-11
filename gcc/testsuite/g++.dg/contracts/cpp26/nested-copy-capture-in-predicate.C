// In a contract predicate, a by-copy capture made by a mutable lambda that is
// itself inside a lambda in the predicate names the inner closure's member,
// whose type is the captured entity's (int), so it can be modified:
// [expr.prim.id.unqual] applies the capture rule before constification
// (notadragon_wg21 DECISIONS.md E32).  The enclosing lambda's by-reference
// capture is const in the predicate; lambda_capture_field_type took the
// inner member's type from it (GCC-636).  A namespace-scope variable is not
// captured, so it stays const and stays an error.
//
// Mirror: clang/test/Contracts/nested-copy-capture-in-predicate.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

int f (int p)
  pre ([&] { return [=] () mutable { return ++p; } (); } () > 0)	// OK
{
  int q = 1;
  contract_assert ([&] { return [=] () mutable { return ++q; } (); } () > 0); // OK
  // Three levels deep.
  contract_assert ([&] { return [&] { return [=] () mutable { return ++q; } (); } (); } () > 0);	// OK
  // The outer lambda's own by-copy capture, in a lambda outside the
  // predicate.
  [q] { contract_assert ([&] { return [=] () mutable { return ++q; } (); } () > 0); } ();	// OK
  // The by-reference capture itself stays const.
  contract_assert ([&] { return [&] { return ++q; } (); } () > 0);	// { dg-error "read-only" }
  return 0;
}

int g;
void h ()
{
  contract_assert ([&] { return [=] () mutable { return ++g; } (); } () > 0); // { dg-error "read-only" }
}

void k ()
{
  const int lc = 1;
  // A const entity's copy is const.
  contract_assert ([&] { return [=] () mutable { return ++lc; } (); } () > 0); // { dg-error "read-only" }
}
