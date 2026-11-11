// <version> defines __cpp_lib_replaceable_contract_violation_handler
// (202603L: the violation handler is replaceable).
//
// Mirror: clang/test/Contracts/ftm-replaceable-handler.cpp in the
// llvm_llvm-project fork.
// { dg-do preprocess { target c++26 } }
// { dg-additional-options "-fcontracts" }

#include <version>

#if !defined(__cpp_lib_replaceable_contract_violation_handler) \
    || __cpp_lib_replaceable_contract_violation_handler != 202603L
#error "__cpp_lib_replaceable_contract_violation_handler"
#endif
