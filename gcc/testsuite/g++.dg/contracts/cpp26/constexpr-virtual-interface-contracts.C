// Under P3097 a virtual call checks the contracts of the statically named
// function (the interface) as well as the final overrider's, in constant
// evaluation as at run time: B::f's violated precondition and B::g's violated
// postcondition are diagnosed though D's overriders have no contracts.
//
// Mirror: clang/test/Contracts/constexpr-virtual-interface-contracts.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097" }

struct B {
  constexpr virtual int f (int x) pre (x > 0) { return x; } // { dg-error "contract predicate is false in constant expression" }
  constexpr virtual int g (int x) post (r: r > 0) { return x; } // { dg-error "contract predicate is false in constant expression" }
};
struct D : B {
  constexpr int f (int x) override { return x; }
  constexpr int g (int x) override { return x; }
};
constexpr int t () { D d; B &b = d; return b.f (-1); }
constexpr int v = t ();
constexpr int u () { D d; B &b = d; return b.g (-1); }
constexpr int w = u ();
constexpr int ok () { D d; B &b = d; return b.f (1) + b.g (1); }
static_assert (ok () == 2);
