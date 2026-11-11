// g++ -std=c++26 -fcontracts -fsyntax-only template-member-decl-contract-lambda-ice.cpp
//
// A lambda in the contract of a member function template of an ordinary
// class.  Valid code; GCC 16.1, 16.2 and trunk crash (Segmentation fault, in
// add_stmt from pushtag) with no instantiation.  The lambda need not capture
// anything, and defining f here instead of only declaring it crashes the
// same way.

struct S
{
  template <class T> void f () pre ([] { return true; } ());
};
