// DECISIONS.md E175 (GCC-961): -fno-contracts-p3850 does not turn off the
// P4451 that an explicit -fcontracts-p3400 implies, so __cpp_contracts and
// __cpp_lib_contracts are raised whenever P3400 is on, and P3400's library
// is the one declared.  The contract-options-additive-*.C tests cover the other
// combinations in C++23.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fno-contracts-p3850" }
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
#ifndef __cpp_lib_contracts_labels
#error "__cpp_lib_contracts_labels"
#endif

constexpr auto &t = std::contracts::labels::terminating;
constexpr auto &e = std::contracts::labels::empty_label;
void f (int x) pre<terminating> (x > 0);
