// A constexpr ::handle_contract_violation is implicitly inline
// ([dcl.constexpr]), so it cannot replace the violation handler
// ([dcl.fct.def.replace]) and is diagnosed at its declaration.  The
// consteval spelling is pr128202.C's.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>

constexpr void handle_contract_violation (const std::contracts::contract_violation &) {} // { dg-error "'::handle_contract_violation' must not be declared 'inline'" }

int main ()
{
  contract_assert (false);
}
