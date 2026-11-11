// P4451: feature-test macros under -fcontracts-p4451 alone.  __cpp_contracts and
// __cpp_lib_contracts are raised from P2900's 202502L (to D4451R0's 202611L,
// not pinned), and __gcc_contracts_p4451 is defined.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p4451" }
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
// Without P3097, nothing else raises __cpp_contracts: both macros carry
// P4451's value.
#if __cpp_contracts != __cpp_lib_contracts
#error "__cpp_contracts and __cpp_lib_contracts differ"
#endif
#ifdef __cpp_lib_contracts_labels
#error "__cpp_lib_contracts_labels"
#endif

constexpr auto &t = std::contracts::labels::terminating;
