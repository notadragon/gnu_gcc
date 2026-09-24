// A type-dependent init-capture in a LATE-parsed (in-class member of a class
// template) postcondition must not segfault.  This is the sibling of the F21
// fix: F21 covered the immediate parsing path
// (cp_parser_function_contract_specifier); this exercises the deferred
// late-parse path (cp_parser_late_contract_condition), which had the same
// unlowered_expr_type-returns-null hazard.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3850" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>

struct Has { int x; };

template<class T>
struct S
{
  T m;
  // `m.x` is a member access on a dependent object -> unlowered_expr_type is
  // null at parse time; the capture type is deferred via decltype.
  int g () post [c = m.x] (r : r > c) { return m.x + 1; }
};

int
main ()
{
  S<Has> s{ Has{ 5 } };
  if (s.g () != 6)
    __builtin_abort ();
  return 0;
}
