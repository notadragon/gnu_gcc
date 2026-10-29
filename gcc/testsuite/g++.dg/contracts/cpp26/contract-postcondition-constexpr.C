// Postcondition result binding during constant evaluation.  A named-result
// postcondition (post(r: ... r ...)) evaluated at compile time must bind the
// result variable to the function's return value.  Previously the result
// variable was left unbound during constant evaluation, so ANY
// result-referencing postcondition made the call non-constant ("contract
// condition is not constant").  This is the general (non-virtual) case;
// p3097-virtual-constexpr.C covers the same fix through virtual dispatch.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

constexpr int inc (const int x) post (r: r == x + 1) { return x + 1; }
static_assert (inc (4) == 5);

constexpr int identity (int x) post (r: r >= 0) { return x; }
static_assert (identity (7) == 7);

// With the result now bound, a false postcondition is a proper constant-
// evaluation violation (predicate false) -- not "condition is not constant".
// The diagnostic is reported at the contract, not the call site.
constexpr int bad (int x)
  post (r: r > 100)  // { dg-error "contract predicate is false in constant expression" }
{ return x; }
constexpr int z = bad (5);
