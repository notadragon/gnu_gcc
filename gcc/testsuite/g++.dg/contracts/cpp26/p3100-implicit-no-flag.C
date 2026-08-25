// P3100: library FTM is not defined without -fcontracts-p3100.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

#ifdef __cpp_lib_contracts_implicit
#error "__cpp_lib_contracts_implicit should not be defined without -fcontracts-p3100"
#endif
