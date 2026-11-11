// A local entity implicitly captured by a lambda only because it is named in
// contract assertions makes the program ill-formed
// ([expr.prim.lambda.closure]): `this' is such an entity, whether named in a
// precondition or in a contract_assert in the body.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct S {
  int m;
  void g ()
  {
    auto s1 = [&] pre (m > 0) {};		// { dg-error "'this' is not implicitly captured" }
    auto s2 = [&] { contract_assert (m > 0); };	// { dg-error "'this' is not implicitly captured" }
    auto s3 = [&] pre (m > 0) { return m; };	// OK: also used in the body
    auto s4 = [this] pre (m > 0) {};		// OK: explicit
  }
  void k () pre ([&] { return m > 0; } ()) {}	// OK: a lambda in a predicate may capture
};

void k (int x)
{
  auto c = [=] pre (x > 0) {};			// { dg-error "not implicitly captured" }
}
