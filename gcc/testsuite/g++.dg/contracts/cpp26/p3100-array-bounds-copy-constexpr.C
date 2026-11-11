// P3100: with the bounds checks configured, an empty class copied from the
// element one past the end of an array of known bound (`E e2 = a[2]', also
// moved, and an empty base of a non-empty element, `EB b = da[2]',
// DECISIONS.md E98 (a)) is diagnosed in constant evaluation as it is
// without them: the check that keeps the element's address for the elided
// copy (DECISIONS.md E95 (a)) is a run-time check, and constant evaluation
// still evaluates the source.  An element in range is a constant
// expression.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.subscript:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct E {};
constexpr bool copy (int i) { E a[2]; E e2 = a[i]; (void) e2; return true; }	// { dg-error "outside the bounds" }
static_assert (copy (1));
static_assert (copy (2));	// { dg-error "non-constant condition" }
constexpr bool move (int i) { E a[2]; E e2 = static_cast<E &&> (a[i]); (void) e2; return true; }	// { dg-error "outside the bounds" }
static_assert (move (1));
static_assert (move (2));	// { dg-error "non-constant condition" }
// { dg-message "in .constexpr. expansion" "" { target *-*-* } .-4 }
// { dg-message "in .constexpr. expansion" "" { target *-*-* } .-2 }
struct EB {};
struct D : EB { int x; };
constexpr bool bcopy (int i) { D a[2]{}; EB b = a[i]; (void) b; return true; }	// { dg-error "outside the bounds" }
static_assert (bcopy (1));
static_assert (bcopy (2));	// { dg-error "non-constant condition" }
// { dg-message "in .constexpr. expansion" "" { target *-*-* } .-1 }
