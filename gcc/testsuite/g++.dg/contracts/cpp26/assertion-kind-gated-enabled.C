// The assertion_kind enumerators of P3290 (manual, cassert) and P3098
// (post_capture) are declared when the papers are enabled, with their
// values.  assertion-kind-gated.C has neither paper.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3290 -fcontracts-p3098" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>

using std::contracts::assertion_kind;

static_assert (static_cast<int> (assertion_kind::manual) == 4);
static_assert (static_cast<int> (assertion_kind::cassert) == 5);
static_assert (static_cast<int> (assertion_kind::post_capture) == 6);
static_assert (static_cast<int> (assertion_kind::implicit) == 7);
