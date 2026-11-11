// Under P3595 caller-side checking, a call to a C-variadic function goes
// through a caller-side wrapper, which passes the variadic arguments on with
// __builtin_va_arg_pack (DECISIONS.md K12).
//
// No Clang mirror: Clang has no caller-side checking.  The P3097 shape is
// p3097-variadic-wrapper.C.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/open-bug-caller-side.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cstdarg>

int sum (int n, ...) pre (n >= 0)
{
  va_list ap;
  va_start (ap, n);
  int s = 0;
  for (int i = 0; i < n; ++i)
    s += va_arg (ap, int);
  va_end (ap);
  return s;
}

int main ()
{
  if (sum (8, 1, 2, 3, 4, 5, 6, 7, 8) != 36)
    __builtin_abort ();
}
