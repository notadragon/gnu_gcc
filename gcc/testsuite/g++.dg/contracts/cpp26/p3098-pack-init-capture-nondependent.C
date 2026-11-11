// GCC-675: an init-capture pack in a postcondition (P3098) whose
// initializer has a type that is not dependent, although the initializer
// names a pack (`...n = sizeof (args)'), was rejected ("expansion pattern
// 'long unsigned int' contains no parameter packs"), and its instantiation
// then crashed the compiler.  Both parse paths: a function template's
// postcondition is parsed with its declaration, a member function
// template's when its class is complete.  A lambda's init-capture pack is
// accepted in the same shape.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3098-pack-init-capture-nondependent.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

template <class... T>
int g (T... args) post [...n = sizeof (args)] ((n + ... + 0) <= 8)
{ return 0; }

struct S
{
  template <class... T>
  int f (T... args) post [...n = sizeof (args)] ((n + ... + 0) <= 8)
  { return 0; }
};

int
main ()
{
  S s;
  g (1, 2);
  s.f (1, 2);
  if (violations != 0)
    __builtin_abort ();
  g (1.0, 2.0);
  s.f (1.0, 2.0);
  if (violations != 2)
    __builtin_abort ();
}
