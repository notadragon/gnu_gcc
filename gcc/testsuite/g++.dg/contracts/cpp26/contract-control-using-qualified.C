// Inside an assertion-control scope, qualified lookup considers a `using
// contract_control namespace' directive, so pre<lib::l> finds lab::l; in
// ordinary code lib::l stays an error.  The ordinary-lookup half is
// contract-control-qualified-lookup.C.
//
// Mirror: clang/test/Contracts/contract-control-using-qualified-in-label.cpp
// in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }

namespace lab { struct L { using assertion_control_object = L; }; constexpr L l{}; }
namespace lib { using contract_control namespace lab; }

void f1 (int x) pre<lib::l> (x > 0);                   // OK
auto v = [] (int x) pre<lib::l> (x > 0) { return x; }; // OK
void f2 (int x) { contract_assert<lib::l> (x > 0); }   // OK
auto z = lib::l;                                       // { dg-error "not a member" }
