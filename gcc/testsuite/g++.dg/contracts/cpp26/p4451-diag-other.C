// P4451: without -fcontracts-p3400 the only assertion-control-specifier is
// `<terminating>', spelled so: `terminating' is not looked up, so neither
// another name nor a qualified one is accepted.  <contracts> declares
// labels::terminating alone, with none of P3400's library.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p4451" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

void f (int x) pre<other> (x > 0);	// { dg-error "only .<terminating>. is allowed as an assertion-control-specifier without .-fcontracts-p3400." }
void g (int x) pre<std::contracts::labels::terminating> (x > 0);	// { dg-error "only .<terminating>. is allowed" }
void h (int x) { contract_assert<(terminating)> (x > 0); }	// { dg-error "only .<terminating>. is allowed" }
void k (int x) pre<terminating> (x > 0);

auto e = std::contracts::labels::empty_label;	// { dg-error "empty_label" }
auto a = std::contracts::labels::always_enforce;	// { dg-error "always_enforce" }

#ifdef __cpp_lib_contracts_labels
#error "no P3400 library"
#endif
#ifdef __cpp_contracts_labels
#error "no P3400"
#endif
