// P3098 x constant evaluation: a postcondition capture in a constexpr function
// evaluated at compile time.  The capture snapshots a constant (the parameter),
// so a predicate over it is constant-evaluable.
//
// It reaches this answer only because the postcondition result variable is
// bound during constant evaluation: leave `r` unbound and the predicate
// `r == old` cannot evaluate, so the contract comes out "not constant".  See
// contract-postcondition-constexpr.C.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }

// Capture snapshot compared against the result.
constexpr int good (int x) post [old = x] (r: r == old) { return x; }
static_assert (good (5) == 5);

// Capture referenced without the result.
constexpr int keep (int x) post [old = x] (old > 0) { return x; }
static_assert (keep (5) == 5);

// A false capturing postcondition is a proper constant-evaluation violation.
constexpr int bad (int x)
  post [old = x] (r: r > old)  // { dg-error "contract predicate is false in constant expression" }
{ return x; }
constexpr int b = bad (5);
