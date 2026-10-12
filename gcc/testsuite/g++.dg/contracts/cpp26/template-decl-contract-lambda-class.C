// As template-decl-contract-lambda.C, for a member of a class template.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

template <class U> struct SC
{
  void fc (U t) pre ([t] { return t > 0; } ());
};
