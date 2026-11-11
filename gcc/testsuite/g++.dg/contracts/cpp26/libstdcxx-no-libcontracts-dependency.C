// A plain program without contracts links and runs: libstdc++.so.6 has no
// dependency on libcontracts that the loader would have to find.
//
// Mirror: clang/test/Contracts/libcxx-linker-script-libcontracts.cpp in the
// llvm_llvm-project fork, which checks that libc++'s linker script makes
// libcontracts a direct dependency.

// { dg-do run }
// { dg-additional-options "-std=c++17" }

#include <cstdio>

int main () { std::puts ("hello"); }
