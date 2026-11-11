// P4451: without -fcontracts-p4451 (or -fcontracts-p3400, which implies
// it), an assertion-control-specifier is rejected, and `<terminating>' is
// pointed at the option.  Nor does <contracts> declare the labels
// namespace.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

void f (int x) pre<terminating> (x > 0);	// { dg-error "assertion-control-specifier .<terminating>. requires .-fcontracts-p4451." }
void g (int x) pre<other> (x > 0);	// { dg-error "assertion-control labels require .-fcontracts-p3400." }
void h (int x) { contract_assert<terminating> (x > 0); }	// { dg-error "requires .-fcontracts-p4451." }

auto t = std::contracts::labels::terminating;	// { dg-error "labels" }

#if __cpp_contracts != 202502L || __cpp_lib_contracts != 202502L
#error "P2900's values"
#endif
#ifdef __gcc_contracts_p4451
#error "no P4451"
#endif
