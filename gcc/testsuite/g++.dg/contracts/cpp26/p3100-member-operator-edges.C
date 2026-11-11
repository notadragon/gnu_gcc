// P3100: the lexical member-call checks of a constructor function-try-block
// handler (ub:except.handle.handler.ctor.dtor) and of a base-class
// mem-initializer (ub:class.base.init.mem.fun.nonpolymorphic.mem.init), under
// "observe", for the member functions of (*this) not called by name: a
// trivial copy assignment, as an operator and by name, a conversion function
// and a member operator of a base are each caught once, and so is an
// explicit call of a trivial destructor; a non-member operator taking
// (*this) and an operator of another object are not.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-member-operator-edges.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:except.handle.handler.ctor.dtor:observe,ub:class.base.init.mem.fun.nonpolymorphic.mem.init:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation (const cs::contract_violation& v) {
  if (v.kind () == cs::assertion_kind::implicit)
    ++viols;
}
struct B { int b; int operator+ () const { return 1; } };
struct A;
int operator- (const A &) { return 0; }
struct A : B {
  int x;
  operator int () const { return 2; }
  A () : B (), x (0) {}
  A (int v, A *other) try : B (), x (v == 1 ? throw 1 : 1) {
  } catch (int) {
    (void) -*this;
    *other = *other;
    std::printf ("clean=%d\n", viols);
    *this = *other;
    this->operator= (*other);
    (void) (int) *this;
    (void) +*this;
    std::printf ("handler=%d\n", viols);
    this->~A ();
    std::printf ("dtor=%d\n", viols);
  }
};
struct Base { int v; Base (int y) : v (y) {} };
struct C : Base {
  operator int () const { return 1; }
  C () : Base ((int) *this) {}
};
int main () {
  A other;
  try {
    A a (1, &other);
  } catch (int) {
  }
  viols = 0;
  C c;
  std::printf ("meminit=%d\n", viols);
  return 0;
}
// { dg-output "clean=0(\n|\r\n|\r)handler=4(\n|\r\n|\r)dtor=5(\n|\r\n|\r)meminit=1" }
