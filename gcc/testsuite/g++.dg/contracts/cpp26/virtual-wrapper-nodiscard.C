// Discarding the result of a [[nodiscard]] virtual function with contracts
// is diagnosed through the P3097 interface wrapper, naming the callee.
//
// Mirror: clang/test/Contracts/virtual-wrapper-drops-nodiscard.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -Wall" }

struct B { [[nodiscard]] virtual int v (int x) pre (x > 0); };
void u (B &b) { b.v (1); }	// { dg-warning "ignoring return value of .virtual int B::v\\(int\\)." }
