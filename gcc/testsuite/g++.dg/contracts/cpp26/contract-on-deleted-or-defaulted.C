// [dcl.contract.func]: a deleted function, and a function defaulted on its
// first declaration, shall not have a function-contract-specifier-seq; both
// are diagnosed.
//
// Mirror: clang/test/Contracts/contract-on-deleted-or-defaulted.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void del (int x) pre (x > 0) = delete; // { dg-error "deleted function" }

struct S {
  S () pre (true) = default;           // { dg-error "defaulted on its first declaration" }
};

// Control: defaulted on a later declaration; the contract is on the first.
struct T {
  T () pre (true);
};
T::T () = default;
