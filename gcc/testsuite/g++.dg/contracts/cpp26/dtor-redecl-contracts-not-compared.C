// A non-first declaration of a destructor may not add or change contracts
// ([dcl.contract.func]).
//
// Mirror: clang/test/Contracts/dtor-redecl-contracts-not-compared.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct B { ~B (); };
B::~B () pre (true) {}		// { dg-error "adds contracts" }

struct C { ~C () pre (true); };
C::~C () pre (false) {}		// { dg-error "mismatched contract condition" }
