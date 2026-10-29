// Inside a requires-expression in a contract predicate, a variable declared
// outside the contract is const ([expr.prim.id.unqual]), so
// `requires { ++x; }` is false, in templates too.
//
// Mirror: clang/test/Contracts/Runnable/constify-requires-expr.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int handled = 0;

void handle_contract_violation (const std::contracts::contract_violation&)
{
  ++handled;
}

template <class T> void free_pre (T x) pre (!requires { ++x; }) {}
template <class T> void body_assert (T x) { contract_assert (!requires { ++x; }); }
template <class T> struct S {
  void mem_pre (T x) pre (!requires { ++x; }) {}
  T m;
  void mem_this () pre (!requires { ++m; }) {}
};

int main ()
{
  free_pre (1);
  body_assert (1);
  S<int> s{};
  s.mem_pre (1);
  s.mem_this ();
  if (handled != 0)
    __builtin_abort ();
}
