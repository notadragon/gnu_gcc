// The extension spellings __pre and __post are accepted wherever pre and
// post are, as Clang accepts them (notadragon_wg21 DECISIONS.md C7).
// Without -fcontracts they are ordinary identifiers
// (extension-spelling-without-contracts.C).
//
// Mirror: clang/test/Contracts/alternative-keywords.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int
f (const int x) __pre (x) __post (x > 0)
{
  __contract_assert (x);
  return x;
}

// In a template, where the contracts are handled once for the pattern and
// again for each instantiation.
template <typename T>
T
tmpl (const T x) __pre (x > 0) __post (r : r > 0)
{
  __contract_assert (x > 0);
  return x;
}

template int tmpl<int> (const int);

// On a member function, whose contracts are late-parsed.
struct S
{
  int m = 1;
  int mem (const int x) __pre (x > 0) __post (x > 0)
  {
    __contract_assert (m > 0);
    return x;
  }
};

// On a lambda's own declarator.
int
in_lambda (const int x)
{
  auto l = [] (const int y) __pre (y > 0) __post (y > 0) { return y; };
  return l (x);
}

// Inside a lambda in a contract predicate.
void
in_predicate_lambda (const int x)
  __pre ([x] { __contract_assert (x >= 0); return x > 0; } ())
{
}

// Mixed spellings on one declaration.
int
mixed (const int x) pre (x > 0) __post (r : r > 0)
{
  contract_assert (x > 0);
  return x;
}

// The spellings make real checks: count the violations.
int violations;
void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

int
main ()
{
  S s;
  f (1);
  tmpl (1);
  s.mem (1);
  in_lambda (1);
  in_predicate_lambda (1);
  mixed (1);
  if (violations != 0)
    __builtin_abort ();
  tmpl (-1);	// __pre, __contract_assert and __post all fail
  if (violations != 3)
    __builtin_abort ();
  s.mem (-1);	// __pre and __post
  if (violations != 5)
    __builtin_abort ();
}
