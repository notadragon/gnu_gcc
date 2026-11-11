// A const function-pointer parameter satisfies the postcondition parameter
// rule (declared const, not of array or function type).
//
// Mirror: clang/test/Contracts/postcondition-const-function-pointer-param.cpp
// in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void f (int (*const g) ()) post (g != nullptr) {}
