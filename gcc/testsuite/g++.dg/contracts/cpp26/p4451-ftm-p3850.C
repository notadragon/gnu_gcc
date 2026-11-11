// P4451: feature-test macros under -fcontracts-p3850, which implies it.  __cpp_contracts and
// __cpp_lib_contracts are raised from P2900's 202502L (to D4451R0's 202611L,
// not pinned), and __gcc_contracts_p4451 is defined.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3850" }
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
// P3850 implies P3097 too, whose value tracks the month as
// __gcc_contracts_p4451 does; __cpp_contracts is the greater of that and
// P4451's value, which __cpp_lib_contracts carries.
#if !(__cpp_contracts >= __cpp_lib_contracts)
#error "__cpp_contracts below P4451's value"
#endif
#if !(__cpp_contracts >= __gcc_contracts_p4451)
#error "__cpp_contracts below P3097's value"
#endif

constexpr auto &t = std::contracts::labels::terminating;
