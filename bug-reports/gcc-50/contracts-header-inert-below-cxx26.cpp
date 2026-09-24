// <contracts> wraps its whole body in #ifdef __cpp_lib_contracts, and the
// guard generated into bits/version.h requires __cplusplus > 202302L.  So at
// -std=c++23 -fcontracts the language half of contracts is on and the library
// half is not: the header expands to nothing but its include guard, and
// std::contracts::contract_violation does not exist.  A violation handler is
// declared by naming that type, so below C++26 there is no way to write one.
//
//   g++ -std=c++23 -fcontracts -fsyntax-only \
//       contracts-header-inert-below-cxx26.cpp   -> error
//   g++ -std=c++26 -fcontracts -fsyntax-only \
//       contracts-header-inert-below-cxx26.cpp   -> clean
//
// The C++26 invocation is the control: it shows the file is otherwise valid,
// so the C++23 error is the guard and not a mistake in the reproducer.

#include <contracts>

// The language feature is on at C++23: this half compiles either way.
int f (int x) pre (x > 0) { return x; }

// The library type it needs is absent at C++23: this half does not.
void
handle_contract_violation (const std::contracts::contract_violation &)
{
}
