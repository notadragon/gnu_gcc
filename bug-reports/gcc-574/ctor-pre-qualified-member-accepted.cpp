// In a constructor precondition or destructor postcondition, a potentially
// evaluated id-expression naming a non-static member is transformed to
// this->m and is ill-formed ([expr.prim.id.general]); that covers a
// qualified-id (C::b) as well as an unqualified one.  GCC checks only the
// unqualified form.
//
// g++ -std=c++26 -fcontracts -fsyntax-only ctor-pre-qualified-member-accepted.cpp
//   -> no diagnostics.  Expected: an error on q1 and q2, as for pre(b).
//
struct C {
  bool b;
  C(char) pre(C::b);                  // q1: error expected, accepted
};
struct E {
  bool b;
  ~E() post(E::b);                    // q2: error expected, accepted
};
