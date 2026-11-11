// Without -fcontracts-p4283 a requires clause on a contract assertion is
// rejected once per clause, and the parser recovers at the predicate: the
// next specifier is still diagnosed and a function body is still parsed
// (GCC-625).  Mirrors Clang's p4283-no-flag.cpp.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

template<typename T> concept C = true;

template<typename T>
void f(T x)
  pre requires(true) (x > 0)  // { dg-error "requires clause on contract assertions requires '-fcontracts-p4283'" }
  post requires(true) (x > 0);  // { dg-error "requires clause on contract assertions requires '-fcontracts-p4283'" }

// An unparenthesized constraint, twice in a row.
template<typename T>
void f2(T x)
  pre requires C<T> (x > 0)  // { dg-error "requires clause on contract assertions requires '-fcontracts-p4283'" }
  pre requires C<T> (x > 1);  // { dg-error "requires clause on contract assertions requires '-fcontracts-p4283'" }

// A result name, then a function body that must still be parsed.
template<typename T>
int f3(T x)
  post requires(true) (r: r > 0)  // { dg-error "requires clause on contract assertions requires '-fcontracts-p4283'" }
{
  return x;
}

template<typename T>
void g(T x)
{
  contract_assert requires(true) (x > 0);  // { dg-error "requires clause on contract assertions requires '-fcontracts-p4283'" }
  contract_assert requires C<T> (x > 0);  // { dg-error "requires clause on contract assertions requires '-fcontracts-p4283'" }
}

// Control: the declarations after the rejected clauses are still parsed.
template<typename T>
void h(T x) pre (x > 0);
int after_f3 = 0;
static_assert (sizeof (after_f3) == sizeof (int));
