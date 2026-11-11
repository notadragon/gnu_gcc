// g++ -std=c++26 -fcontracts -fsyntax-only contract-lambda-then-default-arg-ice.cpp
//
// A lambda in the precondition of a member function declaration, followed
// by any later declaration with a default argument.  Valid code; GCC 16.2
// and trunk crash at the `= 1' (Segmentation fault).

struct S
{
  void f0 (int x) pre ([] { return true; } ());
};

void f4 (int y = 1);
