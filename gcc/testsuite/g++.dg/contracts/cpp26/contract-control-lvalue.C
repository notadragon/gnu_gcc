// An assertion-control-expression "is an lvalue denoting a constexpr object
// with a deduced type initialized by its constant-expression" (P3400,
// [expr.contract.control]).  Its postfix placement is pinned by
// contract-control-postfix.C, templates by contract-control-template.C.
//
// Mirror: clang/test/Contracts/contract-control-lvalue.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }

#include <type_traits>
struct L { using assertion_control_object = L; int v; };
constexpr L make () { return L{3}; }

static_assert (std::is_same_v<decltype ((contract_control (42))), const int &>);
static_assert (std::is_same_v<decltype ((contract_control (make ()))), const L &>);
constexpr const L *p = &contract_control (make ());
