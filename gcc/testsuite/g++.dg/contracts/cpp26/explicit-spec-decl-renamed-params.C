// An explicit specialization's first declaration may name its parameters
// differently from the template's, and its contracts refer to them by the
// names it wrote -- also when that declaration is not a definition, where
// the specialization keeps the parameters of the implicit instantiation it
// replaces (GCC-626: "'a' was not declared in this scope").  Each call
// below violates the specialization's own contract and satisfies the
// template's, so the count shows which one was checked.  An explicit
// specialization of a member template still has this bug (GCC-627,
// open-bug-member-template-spec-contracts.C).
// (Clang mirror: clang/test/Contracts/Runnable/explicit-spec-decl-renamed-params.cpp)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations = 0;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

// A function template.
template<typename T> int g (T t) pre (t > 0) { return 0; }
template<> int g<double> (double a) pre (a > 1);
template<> int g<double> (double a) pre (a > 1) { return 1; }

// A member of a class template.
template<typename T> struct B { int m (T t) pre (t > 0) { return 0; } };
template<> int B<double>::m (double a) pre (a > 1);
template<> int B<double>::m (double a) pre (a > 1) { return 1; }

// Declared without the definition in sight: the call is checked against
// the declaration's contract alone.
template<> int g<long> (long a) pre (a > 1);

int
main ()
{
  if (g (0.5) != 1 || violations != 1)
    __builtin_abort ();
  if (B<double> ().m (0.5) != 1 || violations != 2)
    __builtin_abort ();
  if (g (1L) != 2 || violations != 3)
    __builtin_abort ();
  return 0;
}

template<> int g<long> (long a) pre (a > 1) { return 2; }
