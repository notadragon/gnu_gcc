// P3100: a member operator function of *this called in a constructor
// function-try-block handler (ub:except.handle.handler.ctor.dtor) under
// "observe": `(*this)[0]`, `*this == *this` and `this->operator[](0)` are
// each caught, as a named member function call is; an operator applied to
// another object is not.
// Mirror: llvm_llvm-project clang/test/Contracts/Runnable/p3100-handler-member-operator.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:except.handle.handler.ctor.dtor:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation (const cs::contract_violation& v) {
  if (v.kind () == cs::assertion_kind::implicit)
    ++viols;
}
struct A {
  int x;
  int operator[] (int) const { return 0; }
  bool operator== (const A &) const { return true; }
  A (int v, const A *other) try : x (v == 1 ? throw 1 : 1) {
  } catch (int) {
    (void) (*other)[0];
    (void) (*other == *other);
    std::printf ("clean=%d\n", viols);
    (void) (*this)[0];
    (void) (*this == *this);
    (void) this->operator[] (0);
    std::printf ("ctor=%d\n", viols);
  }
};
int main () {
  A other (0, nullptr);
  try {
    A a (1, &other);
  } catch (int) {
  }
  return 0;
}
// { dg-output "clean=0(\n|\r\n|\r)ctor=3" }
