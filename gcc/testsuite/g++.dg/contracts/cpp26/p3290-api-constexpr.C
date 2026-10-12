// P3290: the manual violation functions are not constexpr (P3290R6 sec.
// "Triggering Enforce and Observe Semantics" declares them without it), so a
// constant evaluation that reaches one is not a constant expression, while a
// constexpr function that calls one only on a path constant evaluation does
// not take (or only outside constant evaluation, under if consteval) stays
// usable in constant expressions.
//
// Mirror: clang/test/Contracts/p3290-api-constexpr.cpp in the
// llvm_llvm-project fork; owed to EDG, recorded in its open-issues/README.md.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3290" }

#include <contracts>

constexpr int
checked (int x)
{
  if (x < 0)
    std::contracts::handle_observed_contract_violation ("negative"); // { dg-error "call to non-.constexpr. function" }
  return x;
}

constexpr int
guarded (int x)
{
  if !consteval
    {
      if (x < 0)
	std::contracts::handle_enforced_contract_violation ("negative");
    }
  return x;
}

static_assert (checked (1) == 1);
static_assert (guarded (-1) == -1);
constexpr int bad = checked (-1);	// diagnosed in checked, above
