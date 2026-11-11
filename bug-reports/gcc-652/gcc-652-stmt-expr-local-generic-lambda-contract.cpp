// gcc-652-stmt-expr-local-generic-lambda-contract.cpp             -*-C++-*-
//
// GCC-652: a variable declared in a GNU statement expression in a generic
// lambda's precondition is rejected when the operator() is instantiated.
// Expected: compiles.  Actual: "use of parameter from containing function"
// and "use of local variable with automatic storage from containing
// function".
//
// g++ -std=c++26 -fcontracts -c gcc-652-stmt-expr-local-generic-lambda-contract.cpp

int k ()
{
  auto m = [] (auto x) pre (({ int y = x; y > 0; })) { return x; };
  return m (1);
}
