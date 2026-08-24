// P3098 x constant evaluation: a postcondition capture in a constexpr function
// evaluated at compile time.  The capture snapshots a constant (the parameter),
// so a predicate over it is constant-evaluable.
//
// Previously BUG-8: such a contract was wrongly "not constant" during constant
// evaluation.  The root cause was the unbound postcondition result variable
// (the predicate `r == old` could not evaluate with `r` unbound); binding the
// result during constant evaluation fixes it -- see
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
