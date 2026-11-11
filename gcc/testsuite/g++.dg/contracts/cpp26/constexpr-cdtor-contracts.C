// The contracts of a constexpr constructor or destructor are checked in
// constant evaluation: a violated one is diagnosed in all three shapes.
//
// Mirror: clang/test/Contracts/constexpr-ctor-contracts.cpp and
// constexpr-dtor-contracts.cpp in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct S {
  int v;
  constexpr S (int x) pre (x > 0) : v (x) {} // { dg-error "contract predicate is false in constant expression" }
};
constexpr S s (-1);

struct T {
  int v;
  constexpr T (int x) post (v == 99) : v (x) {} // { dg-error "contract predicate is false in constant expression" }
};
constexpr T t (1);

struct U {
  int v = 1;
  constexpr ~U () pre (v == 99) {} // { dg-error "contract predicate is false in constant expression" }
};
constexpr bool fu () { U u; return true; }
static_assert (fu ());

constexpr S ok (1);
