// The operand of an assertion-control-expression is a constant-expression
// ([expr.contract.control], P3400): non-constant operands are rejected.
//
// Mirror: clang/test/Contracts/contract-control-nonconstant.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }

struct L { using assertion_control_object = L; int v; };

int g (int x)
{
  L l{x};
  auto a = contract_control (l);        // { dg-error "not usable in a constant expression" }
  auto b = contract_control (x + 1);    // { dg-error "not a constant expression" }
  return a.v + b;
}
