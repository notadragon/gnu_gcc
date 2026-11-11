// A postcondition capture (P3098) on a declaration that is not the
// definition reads the value passed to the definition's parameter (GCC-658):
// on a declaration before the definition, on a member function declared in
// its class, and on a function template.
//
// Mirror: clang/test/Contracts/Runnable/p3098-capture-on-declaration.cpp in
// the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;
void handle_contract_violation (const std::contracts::contract_violation &)
{ ++violations; }

int g (int a) post [old = a] (old > 0);
int g (int y) { return y; }

struct S
{
  int m (int a) post [old = a] (old > 0);
};
int S::m (int y) { return y; }

template <class T> T t (T a) post [old = a] (old > 0);
template <class T> T t (T y) { return y; }

int main ()
{
  g (1);
  S{}.m (1);
  t (1);
  if (violations != 0)
    __builtin_abort ();
  g (0);
  if (violations != 1)
    __builtin_abort ();
  S{}.m (0);
  if (violations != 2)
    __builtin_abort ();
  t (0);
  if (violations != 3)
    __builtin_abort ();
}
