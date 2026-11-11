// DECISIONS.md E175 (GCC-961): -fcontracts-p3850 with an explicit
// -fno-contracts-p3400 still implies P4451, whatever the order: the macros
// are raised, and <contracts> declares P4451's labels::terminating alone.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3850 -fno-contracts-p3400" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

#if !(__cpp_contracts > 202502L)
#error "__cpp_contracts"
#endif
#if !(__cpp_lib_contracts > 202502L)
#error "__cpp_lib_contracts"
#endif
#if !(__gcc_contracts_p4451 > 0)
#error "__gcc_contracts_p4451"
#endif
#ifdef __cpp_lib_contracts_labels
#error "__cpp_lib_contracts_labels"
#endif

constexpr auto &t = std::contracts::labels::terminating;
auto e = std::contracts::labels::empty_label;	// { dg-error "empty_label" }
void f (int x) pre<terminating> (x > 0);
