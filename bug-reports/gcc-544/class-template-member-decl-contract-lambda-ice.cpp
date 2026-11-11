// g++ -std=c++26 -fcontracts -fsyntax-only class-template-member-decl-contract-lambda-ice.cpp
//
// A lambda in the contract of a member function of a class template.  Valid
// code; GCC 16.1, 16.2 and trunk crash (Segmentation fault, in add_stmt from
// pushtag) while parsing the class, with no instantiation.  The lambda need
// not capture anything, and defining f here instead of only declaring it
// crashes the same way.

template <class T> struct S
{
  void f () pre ([] { return true; } ());
};
