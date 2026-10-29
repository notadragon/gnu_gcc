// [dcl.fct.def.replace]: ::handle_contract_violation may not be deleted.
// Used to crash at the end of the translation unit, emitting the
// __handle_contract_violation alias for a function with no body.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>

void handle_contract_violation (const std::contracts::contract_violation&) = delete; // { dg-error "'::handle_contract_violation' cannot be deleted" }

int main () {}
