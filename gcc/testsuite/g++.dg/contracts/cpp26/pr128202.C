// PR c++/128202
// A consteval ::handle_contract_violation is implicitly inline
// ([dcl.constexpr]), so it cannot replace the violation handler
// ([dcl.fct.def.replace]) and is diagnosed at its declaration.  Without that
// diagnostic the program reaches the violation-handler wrapper, which calls
// an immediate function from a non-immediate context and ICEs in
// build_call_a.  The observe semantic is pinned because it is one of the two
// that call the handler.  The constexpr spelling is
// violation-handler-constexpr.C's.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>

consteval void handle_contract_violation (const std::contracts::contract_violation &) {} // { dg-error "'::handle_contract_violation' must not be declared 'inline'" }

int main ()
{
  contract_assert (false);
}
