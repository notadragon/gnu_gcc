// D4301: contract_violation::report() is noexcept.  Rendering the
// diagnostic on demand may allocate/throw, but the accessor guards the
// populator call and returns a fixed diagnostic string on failure, so it
// never propagates an exception -- like the other contract_violation
// accessors.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4301" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <utility>

static_assert(
    noexcept(
	std::declval<const std::contracts::contract_violation&>().report()));

// The other accessors are also noexcept.
static_assert(
    noexcept(
	std::declval<const std::contracts::contract_violation&>().comment()));
static_assert(
    noexcept(
	std::declval<const std::contracts::contract_violation&>().message()));
