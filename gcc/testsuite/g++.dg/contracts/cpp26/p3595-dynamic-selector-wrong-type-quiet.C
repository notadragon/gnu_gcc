// GCC-672: p3595-dynamic-selector-wrong-type.C under
// -Wno-contract-configuration, which silences every warning there.  (The
// included file's dg- directives are not read.)
//
// Mirror: the -Wno-contract-configuration RUN line of
// clang/test/Contracts/p3595-dynamic-selector-wrong-type.cpp in the
// llvm_llvm-project fork (CLANG-655).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -Wno-contract-configuration" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-selector-wrong-type.json" }

#include "p3595-dynamic-selector-wrong-type.C"
