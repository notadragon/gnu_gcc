// Code generation with debug information for contract checks: a free
// function's postcondition, and a constructor's and a destructor's
// preconditions and postconditions.  It must compile.
// codegen-debug-info-throwing.C has the checks whose predicate or capture
// may throw.
//
// Mirror: clang/test/Contracts/codegen-debug-info.cpp in the
// llvm_llvm-project fork (CLANG-129; GCC-668).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce -g" }

int foo (const int x) post (x != 0) { return x; }

struct S {
  S (const int x) pre (x != 0) post (x != 1) : x (x) {}
  ~S () pre (x != 0) post (true) {}
  int x;
};

S s (2);
