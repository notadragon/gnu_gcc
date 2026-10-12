// A requires clause on a contract assertion of a non-templated function is
// rejected once per clause, and the parser recovers at the predicate: the
// next specifier is still diagnosed and a function body is still parsed
// (GCC-625).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283" }

template<typename T> concept C = true;

void f(int x)
  pre requires(true) (x > 0)  // { dg-error "requires clause on contract assertion only allowed on templated functions" }
  post requires(true) (x > 0);  // { dg-error "requires clause on contract assertion only allowed on templated functions" }

void f2(int x)
  pre requires C<int> (x > 0)  // { dg-error "requires clause on contract assertion only allowed on templated functions" }
  pre requires C<int> (x > 1);  // { dg-error "requires clause on contract assertion only allowed on templated functions" }

int f3(int x)
  post requires(true) (r: r > 0)  // { dg-error "requires clause on contract assertion only allowed on templated functions" }
{
  return x;
}

void g(int x)
{
  contract_assert requires(true) (x > 0);  // { dg-error "requires clause on contract assertion only allowed on templated functions" }
  contract_assert requires C<int> (x > 0);  // { dg-error "requires clause on contract assertion only allowed on templated functions" }
}

// Control: the declarations after the rejected clauses are still parsed.
void h(int x) pre (x > 0);
int after_f3 = 0;
static_assert (sizeof (after_f3) == sizeof (int));
