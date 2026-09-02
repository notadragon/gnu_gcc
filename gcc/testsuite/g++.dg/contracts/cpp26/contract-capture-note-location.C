// A contract assertion may not cause a capture that exists only for it
// ([expr.prim.lambda.closure]), and the diagnostic for that carries a note
// saying where the entity was declared.  The note's location was read out of
// the capture's *initializer*, which is a DECL only when the entity is
// captured directly from the enclosing function.  Two lambdas out, the
// initializer reads through the enclosing closure instead, and the location
// came out as garbage -- a negative line number in a release build, and a
// tree-check ICE with checking enabled.
//
// The point of this test is the LOCATION of the note, so every expectation
// below is anchored to the line the entity is really declared on.  Neither
// structured bindings nor generic lambdas are needed to reach the bug, so
// this covers plain locals and non-generic lambdas as well.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

// One level: this always worked; it is the control.
void
one_level ()
{
  int x = 1;                    // { dg-message "declared here" "one level" }
  auto l = [&] () { contract_assert (x > 0); };
  // { dg-error "not implicitly captured by a contract assertion" "" { target *-*-* } .-1 }
  l ();
}

// Two levels, non-generic: the shape that produced the garbage location.
void
two_levels ()
{
  int x = 1;                    // { dg-message "declared here" "two levels" }
  auto outer = [&] () {
    auto inner = [&] () { contract_assert (x > 0); };
    // { dg-error "not implicitly captured by a contract assertion" "" { target *-*-* } .-1 }
    inner ();
  };
  outer ();
}

// Three levels: one more than the reproducer, because a fix that only
// unwraps a single layer would still be wrong here.
void
three_levels ()
{
  int x = 1;                    // { dg-message "declared here" "three levels" }
  auto a = [&] () {
    auto b = [&] () {
      auto c = [&] () { contract_assert (x > 0); };
      // { dg-error "not implicitly captured by a contract assertion" "" { target *-*-* } .-1 }
      c ();
    };
    b ();
  };
  a ();
}

// Two levels, generic: the report blamed generic lambdas; they are neither
// necessary nor sufficient, and this pins that they are also not broken.
void
two_levels_generic ()
{
  int x = 1;                    // { dg-message "declared here" "generic" }
  auto outer = [&] (auto f) {
    auto inner = [&] (auto g) { contract_assert (x + f + g > 0); };
    // { dg-error "not implicitly captured by a contract assertion" "" { target *-*-* } .-1 }
    inner (1);
  };
  outer (1);
}
