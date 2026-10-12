// As hidden-friend-template-pre.C, with a precondition naming a parameter of
// the class template's parameter type.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

template <class T> struct S {
  friend void fr2 (S, T t) pre (t > 0) {}
};

int main () { fr2 (S<int>{}, 1); }
