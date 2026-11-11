// The implicit (*this). transformation of `m` inside a lambda in a
// constructor precondition still "occurs in the predicate of a precondition
// assertion of a constructor" ([expr.prim.id.general]), so s1 is ill-formed.
// s2 (explicit this->) and s3 (a local class's own member) are OK.
//
// g++ -std=c++26 -fcontracts -fsyntax-only ctor-pre-member-in-lambda.cpp
//   -> s1 is accepted; only the control S(int) is diagnosed.  Expected: the
//      same error for s1 as for `S(int) pre(m > 0);`.
//
struct S {
  int m;
  S(unsigned) pre([&]{ return m > 0; }());          // s1: error
  S(long) pre([this]{ return this->m > 0; }());      // s2: OK
  S(char) pre([]{ struct L { int q; int get() { return q; } }; return true; }());  // s3: OK
  S(int) pre(m > 0);                                 // control: error, diagnosed
};
