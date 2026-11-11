// A lambda with a precondition naming a member (through a captured `this`)
// in a default member initializer of a class template.  Valid code.
//
// g++ -std=c++26 -fcontracts -fsyntax-only nsdmi-lambda-contract-this.cpp
//   -> error: could not convert '(((S<T>*)__closure)->S<T>::m < ((const int)x))'
//      from 'dependent_operator_type' to 'bool'
//   The lambda's contract is never substituted when it is instantiated;
//   the default member initializer is not needed (a lambda in a member
//   function of the class template does the same).
//   Without the member (`pre(x > 0)`) the same defect ICEs instead.
//   Expected: accepted, and the precondition checked.
template <class T> struct S {
  T m = 5;
  bool r = [this](int x) pre(x > m) { return true; }(1);
};
int main() { S<int> s; }
