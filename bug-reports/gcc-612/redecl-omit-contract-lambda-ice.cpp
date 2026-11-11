// A member function of a class template declared with a precondition whose
// lambda captures a parameter, defined out of line without repeating the
// contract (which [dcl.contract.func] allows).  Valid code.  Instantiating
// the definition crashes.
//
// g++ -std=c++26 -fcontracts -fsyntax-only redecl-omit-contract-lambda-ice.cpp
//   -> internal compiler error: in tsubst_expr, at cp/pt.cc:23546, at the
//      lambda on line 13, "In instantiation of 'int S<T>::f(int) [with T =
//      int]'".  Expected: clean.
//   Stock 16.1.0, 16.2.0 and trunk 17.0.0 20260922 segfault earlier, while
//   parsing the class, on the in-class lambda alone (a separate crash).

template <class T> struct S { int f (int x) pre ([&] { return x > 0; } ()); };
template <class T> int S<T>::f (int x) { return x; }
template struct S<int>;
