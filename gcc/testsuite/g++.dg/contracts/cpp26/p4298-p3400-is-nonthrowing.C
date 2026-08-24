// D4298 + P3400: is_nonthrowing classifies the nonthrowing-capable semantics
// correctly.  is_nonthrowing is constexpr, so also check at compile time.  E1:
// the `assume' semantic was previously untested here even though the
// implementation classifies it as nonthrowing.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4298" }
#include <contracts>
using namespace std::contracts;

// Compile-time classification (is_nonthrowing is constexpr).
static_assert (is_nonthrowing (evaluation_semantic::ignore));
static_assert (is_nonthrowing (evaluation_semantic::quick_enforce));
static_assert (is_nonthrowing (evaluation_semantic::assume));
static_assert (is_nonthrowing (evaluation_semantic::noexcept_enforce));
static_assert (is_nonthrowing (evaluation_semantic::noexcept_observe));
static_assert (!is_nonthrowing (evaluation_semantic::enforce));
static_assert (!is_nonthrowing (evaluation_semantic::observe));

int main()
{
  if (!is_nonthrowing(evaluation_semantic::ignore)) __builtin_abort ();
  if (!is_nonthrowing(evaluation_semantic::quick_enforce)) __builtin_abort ();
  if (!is_nonthrowing(evaluation_semantic::assume)) __builtin_abort ();
  if (!is_nonthrowing(evaluation_semantic::noexcept_enforce)) __builtin_abort ();
  if (!is_nonthrowing(evaluation_semantic::noexcept_observe)) __builtin_abort ();
  if (is_nonthrowing(evaluation_semantic::enforce)) __builtin_abort ();
  if (is_nonthrowing(evaluation_semantic::observe)) __builtin_abort ();
}
