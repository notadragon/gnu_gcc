// The contract configuration file changes the generated code, so it is
// recorded in the -MD dependency file.
//
// Mirror: clang/test/Contracts/config-file-in-depfile.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -MD -fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/config-file-in-depfile.json" }

int f (int x) pre (x > 0) { return x; }

// { dg-final { scan-file config-file-in-depfile.d "config-file-in-depfile\\.json" } }
