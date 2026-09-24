// { dg-options "-fcontracts-p3850" }
// { dg-do compile { target c++26 } }

// P4298 std::contracts::is_nonthrowing.  A constexpr classification of the
// enumerators, so it can be checked exhaustively at compile time.

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::is_nonthrowing;

static_assert(is_nonthrowing(evaluation_semantic::ignore));
static_assert(is_nonthrowing(evaluation_semantic::quick_enforce));
static_assert(is_nonthrowing(evaluation_semantic::assume));
static_assert(is_nonthrowing(evaluation_semantic::noexcept_enforce));
static_assert(is_nonthrowing(evaluation_semantic::noexcept_observe));

// The two that can propagate out of handling.
static_assert(!is_nonthrowing(evaluation_semantic::observe));
static_assert(!is_nonthrowing(evaluation_semantic::enforce));
