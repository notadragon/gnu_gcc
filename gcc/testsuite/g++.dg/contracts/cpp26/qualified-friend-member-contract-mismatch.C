// A friend declaration whose declarator-id names a member function of another
// class redeclares that member ([dcl.contract.func]): its contract
// specifiers, if any, must be the same as the member's, compared once the
// befriending class is complete.
//
// Mirror: clang/test/Contracts/OpenBugs/qualified-friend-member-contract-mismatch.cpp
// in the llvm_llvm-project fork, the watch test for Clang's CLANG-639 (Clang
// accepts the mismatches).  Found by the EDG fork's
// sema/friend_member_redeclaration test.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

struct Inner {
  void fn (int n) pre (n > 0);
  void fn3 (int n) pre (n > 0);
  void fn4 (int n) pre (n > 0);
  void fn5 (int n);
};
struct Outer {
  friend void Inner::fn3 (int m) pre (m > 0);	// OK, parameter renamed
  friend void Inner::fn4 (int n);	// OK, omitted
  friend void Inner::fn (int n) pre (n < 0);	// { dg-error "mismatched contract condition" }
  friend void Inner::fn5 (int n) pre (n > 0);	// { dg-error "declaration adds contracts" }
};

// The befriending class is complete in the contract assertions.
struct O2 {
  struct In {
    void fn (int n) pre (n > 0 && bob > 1);
    void fn2 (int n) pre (n > 0);
  };
  friend void In::fn (int n) pre (n > 0 && bob > 1);	// OK
  friend void In::fn2 (int n) pre (n > 1);	// { dg-error "mismatched contract condition" }
  static int bob;
};
