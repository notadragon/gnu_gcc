// A variable declared inside a GNU statement expression in a contract
// predicate is declared inside the contract, so it is not const
// ([expr.prim.id.unqual]).
//
// Mirror: clang/test/Contracts/stmt-expr-local-constified.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void f () { contract_assert (__extension__ ({ int i = 0; ++i; }) > 0); }
