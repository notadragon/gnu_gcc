// A variable's constant destruction evaluates the destructor's contract
// assertions as any other constant evaluation does.  A constexpr variable,
// which must have constant destruction ([dcl.constexpr]), and a class-type
// template argument are ill-formed when a precondition is violated or is
// not a core constant expression under a terminating semantic; under
// observe they get a warning (constexpr-dtor-contracts-constant-destruction-observe.C,
// which also checks that any other variable whose destructor's precondition
// fails is destroyed, and the precondition evaluated, at run time).  GCC
// reports the problem at the destructor's assertion, without naming the
// variable.  The same destructor run inside a constant evaluation is
// constexpr-cdtor-contracts.C.
//
// Mirror: clang/test/Contracts/constexpr-dtor-contracts-constant-destruction.cpp
// in the llvm_llvm-project fork (its enforce run; CLANG-862 there).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

struct DF {
  constexpr DF () {}
  constexpr ~DF () pre (false) {}	// { dg-error "contract predicate is false in constant expression" }
};

int g;
struct DE {
  constexpr ~DE () pre (g > 0) {}	// { dg-error "contract condition is not constant" }
};

struct DT {
  constexpr ~DT () pre (true) {}
};

constexpr DF f2;

template <DF x> struct X {};
X<DF{}> xv;

constexpr DE e2;

// Not constant destruction: destroyed at exit, through the destructor.
DE e1;
void f () { static DE el; }
DF ff;

// A destructor whose contract assertions hold: constant destruction.
DT t1;
constexpr DT t2;
