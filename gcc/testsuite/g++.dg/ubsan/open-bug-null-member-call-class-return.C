// WATCH TEST for GCC-854 (stock; bug-reports/gcc-854/): the output test
// XFAILs until it is fixed.  -fsanitize=null does not check the object
// pointer of a member function call returning a class with a non-trivial
// copy constructor or destructor by value (an AGGR_INIT_EXPR, which
// cp_genericize_r does not instrument); the same call returning int, the
// control, is reported.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-options "-fsanitize=null -fsanitize-recover=null" }

#include <cstdio>

struct R { R () {} R (const R &) {} ~R () {} int r; };
struct S { R f () { return R (); } int g () { return 0; } };

__attribute__((noipa)) void cls (S *sp) { sp->f (); }
__attribute__((noipa)) void scalar (S *sp) { sp->g (); }

int
main ()
{
  S *volatile sp = 0;
  std::fprintf (stderr, "cls\n");
  cls (sp);
  std::fprintf (stderr, "scalar\n");
  scalar (sp);
  return 0;
}

// { dg-output "cls\[^\n\r\]*(\n|\r\n|\r)\[^\n\r\]*runtime error: member call on null pointer of type 'struct S'\[^\n\r\]*(\n|\r\n|\r)scalar\[^\n\r\]*(\n|\r\n|\r)\[^\n\r\]*runtime error: member call on null pointer of type 'struct S'" { xfail *-*-* } }
