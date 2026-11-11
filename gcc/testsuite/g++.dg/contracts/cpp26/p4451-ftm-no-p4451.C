// P4451: a later -fno-contracts-p4451 turns it off again: P2900's
// feature-test macro values, and `<terminating>' is rejected.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4451 -fno-contracts-p4451" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

#if __cpp_contracts != 202502L || __cpp_lib_contracts != 202502L
#error "P2900's values"
#endif
#ifdef __gcc_contracts_p4451
#error "__gcc_contracts_p4451"
#endif
void f (int x) pre<terminating> (x > 0);	// { dg-error "requires .-fcontracts-p4451." }
