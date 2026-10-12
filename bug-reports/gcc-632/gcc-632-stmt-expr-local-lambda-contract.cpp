// GCC-632: a local declared in a statement expression in a lambda's
// precondition or postcondition ICEs in gimplify_var_or_parm_decl.
// g++ -std=c++26 -fcontracts -c gcc-632-stmt-expr-local-lambda-contract.cpp
void f ()
{
  // CONTROL: a statement expression without a declaration compiles.
  auto ok = [] (const int x) pre (({ x > 0; })) {};
  ok (1);
  // THE BUG: the declaration of y.
  auto l = [] (const int x) pre (({ int y = x; y > 0; })) {};
  l (1);
}
