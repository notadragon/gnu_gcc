// [expr.prim.id.general] turns an id-expression naming a non-static member
// in a constructor precondition or destructor postcondition into this->m,
// which is ill-formed there -- but its own example marks
// `C() pre(sizeof(b) > 0);` "OK, b is not potentially evaluated".  GCC
// rejects the example and the other unevaluated operands.
//
// g++ -std=c++26 -fcontracts -fsyntax-only ctor-pre-unevaluated-member-rejected.cpp
//   -> "'C::b' 'this' required when accessing a member within a constructor
//      precondition or destructor postcondition contract check" for u1, u2,
//      u3 and u4.  Expected: clean.  (requires { b; } and &this->b are
//      accepted.)
struct C {
  bool b;
  int f() const;
  C(int) pre(sizeof(b) > 0);          // u1: OK, the standard's example
  C(float) pre(sizeof(f()) > 0);      // u2: OK, unevaluated
  C(double) pre(noexcept(f()));       // u3: OK, unevaluated
  C(unsigned) pre(requires { b; });   // OK, accepted
  C(long) pre(&this->b);              // OK (example), accepted
};
struct D {
  bool b;
  ~D() post(sizeof(b) > 0);           // u4: OK, unevaluated
};
