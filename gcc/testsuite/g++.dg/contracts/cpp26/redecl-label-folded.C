// The assertion-control specifiers of two declarations must satisfy the ODR
// as contract-control expressions (P3400), so a redeclaration naming a
// different constexpr object, or a temporary, is ill-formed even when the
// values are equal.
//
// Mirror: clang/test/Contracts/redecl-label-folded.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
struct L { using assertion_control_object = L; };
constexpr L a{}, b{};

void f (int x) pre<a> (x > 0);
void g (int x) pre<a> (x > 0);

void f (int x) pre<b> (x > 0) {}	// { dg-error "mismatched assertion-control label" }
void g (int x) pre<L{}> (x > 0) {}	// { dg-error "mismatched assertion-control label" }
