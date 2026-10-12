// The outlined checks (-fcontract-checks-outlined) of a lambda in a function
// template are internal functions in the comdat group of the lambda's
// operator().  start_preparsed_function also gave each a comdat group of its
// own, as a function of a local class in a template, so it was in two and a
// checking build's verify_cgraph_node rejected it (GCC-657).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

template <class T>
int
t (T v)
{
  auto l = [] (T x) pre (x > 0) post (r: r == 1) { return 1; };
  auto g = [] (auto x) pre (x > 0) { return 1; };
  return l (v) + g (v);
}

int
main ()
{
  t (1);
  if (violations != 0)
    __builtin_abort ();
  t (0);
  if (violations != 2)
    __builtin_abort ();
}
