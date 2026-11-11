// The diagnostics for a declaration of ::handle_contract_violation that
// cannot replace the violation handler, worded with "must" like GCC's other
// signature diagnostics ("'::main' must return 'int'"; notadragon_wg21
// DECISIONS.md A10).  Overloads, so that each declaration is checked on its
// own.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;

inline void handle_contract_violation (const contract_violation &) {} // { dg-error "'::handle_contract_violation' must not be declared 'inline'" }
void handle_contract_violation (); // { dg-error "'::handle_contract_violation' must have a single parameter of type 'const std::contracts::contract_violation&'" }
void handle_contract_violation (const contract_violation &, int); // { dg-error "'::handle_contract_violation' must have a single parameter" }
void handle_contract_violation (contract_violation &&); // { dg-error "parameter of '::handle_contract_violation' must be an lvalue reference to 'const std::contracts::contract_violation'" }
void handle_contract_violation (contract_violation &); // { dg-error "parameter of '::handle_contract_violation' must be a reference to 'const std::contracts::contract_violation'" }
void handle_contract_violation (const int &); // { dg-error "parameter of '::handle_contract_violation' must be of type 'const std::contracts::contract_violation&'" }
int handle_contract_violation (const contract_violation *); // { dg-error "'::handle_contract_violation' must return 'void'" }
// { dg-error "must be an lvalue reference" "" { target *-*-* } .-1 }
extern "C" void handle_contract_violation (const volatile contract_violation &); // { dg-error "'::handle_contract_violation' must have C\\+\\+ language linkage" }
// { dg-error "must be a reference to 'const" "" { target *-*-* } .-1 }
