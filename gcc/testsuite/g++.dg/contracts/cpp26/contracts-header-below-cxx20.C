// <contracts> is empty below C++20 (its interface names std::source_location),
// and including it there is not an error, even with -fcontracts, which reaches
// any dialect.  The library feature-test macros are not defined there either.
//
// Mirror: clang/test/Contracts/libcxx-contracts-below-cxx20.cpp in the
// llvm_llvm-project fork.
// { dg-do compile }
// { dg-additional-options "-std=c++17 -fcontracts" }

#include <contracts>

#ifdef __cpp_lib_contracts
#error "__cpp_lib_contracts defined below C++20"
#endif

#ifdef __cpp_contracts
int f (int x) pre (x > 0) { return x; }
#endif
