// g++ -std=c++26 -fcontracts -fsyntax-only block-scope-decl-then-def-rejected.cpp
//
// The same shape with the contract repeated on the definition, which is
// also valid.  GCC 16.2 and trunk reject it: "declaration adds contracts to
// 'void loc(int)'".

void o (int t)
{
  void loc (int x) pre (x > 0);
  loc (t);
}

void loc (int x) pre (x > 0) {}
