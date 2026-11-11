// g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe \
//     block-scope-first-decl-contract-dropped.cpp && ./a.out
//
// A block-scope function declaration that is the first declaration of lx
// has a precondition; the namespace-scope definition omits it, which is
// allowed ([dcl.contract.func]: a redeclaration may omit the
// function-contract-specifier-seq).  lx (-1) must report a violation.
// GCC 16.2 and trunk print only "after".

#include <cstdio>

int main ()
{
  void lx (int x) pre (x > 0);
  lx (-1);
  std::puts ("after");
}

void lx (int) {}
