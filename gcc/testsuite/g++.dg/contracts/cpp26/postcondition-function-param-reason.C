// A non-const function pointer named in a postcondition breaks the rule
// because it is not const.
//
// Mirror: clang/test/Contracts/postcondition-function-param-reason.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void f2 (int (*g) ()) post (g != nullptr) {} // { dg-error "must be const" }
// { dg-message "parameter declared here" "" { target *-*-* } .-1 }
