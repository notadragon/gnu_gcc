// The header is precompiled with -fcontract-evaluation-semantic=ignore and
// this TU is compiled with enforce.  The PCH does not match, so it is not
// used (GCC-510): the header's semantic must not silently override the TU's.
// pch.exp removes the header source for the compile that uses the PCH, so
// the rejection is the error below; reading the header as source instead
// diagnoses the violated precondition in constant evaluation.
//
// This lives in g++.dg/pch because only pch.exp builds a header with its own
// options.
//
// Mirror: clang/test/Contracts/pch-contract-semantic.cpp in the
// llvm_llvm-project fork.
// { dg-options "-std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce" }

#include "contract-semantic-mismatch.H" // { dg-error "PCH files were found, but they were invalid" }
// { dg-prune-output "No such file or directory" }
// { dg-prune-output "compilation terminated" }

constexpr int v = f (-1);
