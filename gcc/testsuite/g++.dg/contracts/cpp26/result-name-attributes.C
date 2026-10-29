// A result-name-introducer is an attributed-identifier followed by `:', so
// the result name may carry an attribute-specifier-seq.
//
// Mirror: clang/test/Contracts/result-name-attributes.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

int f (int x) post (r [[maybe_unused]] : r > 0) { return x; }
