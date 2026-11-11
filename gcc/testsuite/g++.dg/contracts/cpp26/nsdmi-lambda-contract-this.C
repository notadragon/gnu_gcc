// A lambda in a default member initializer of a class template, whose
// precondition names a member through the captured this: the pattern's this
// proxy is mapped to the instantiation's, as it is for a lambda in a member
// function (where substituting the function's body does it).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

template <class T> struct S {
  T m = 5;
  bool r = [this] (int x) pre (x > m) { return true; } (1);
};

int main () { S<int> s; }
