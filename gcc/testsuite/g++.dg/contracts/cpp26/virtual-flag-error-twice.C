// A contract on a member of a class template that is virtual at its
// definition is diagnosed (without the virtual-function contracts extension)
// once, not once per instantiation.  (A dg-error consumes every matching
// message on its line, so it cannot see a duplicate; this pins the
// diagnostic.)
//
// Mirror: clang/test/Contracts/virtual-flag-error-twice.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct B { virtual void f (int) = 0; };
template <class T> struct S : B {
  T m = 0;
  void f (int x) override pre (x > m) {}	// { dg-error "contracts cannot be added to virtual functions" }
};
int main () { S<int> s; s.f (1); }
