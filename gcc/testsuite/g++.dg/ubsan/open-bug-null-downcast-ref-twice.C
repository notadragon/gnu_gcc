// WATCH TEST for GCC-871 (stock; bug-reports/gcc-871/): the output test
// XFAILs until it is fixed.  -fsanitize=null reports a downcast of `*p' to
// a reference, p a null pointer, twice when the base is at a nonzero
// offset in the derived class: the conversion's null test yields a null
// reference, whose binding build_static_cast_1 instruments, and
// cp_genericize_r instruments the same null reference again.  The base at
// offset zero, the control, is reported once.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-options "-fsanitize=null -fsanitize-recover=null" }

#include <cstdio>

struct V { virtual void h () {} int v; };
struct X { virtual void x () {} long pad; };
struct D0 : V { int d; };		// V at offset 0
struct D2 : X, V { int d; };		// V at offset 16
__attribute__((noipa)) D0 &bind0 (V *p) { return static_cast<D0 &> (*p); }
__attribute__((noipa)) D2 &bind (V *p) { return static_cast<D2 &> (*p); }

int
main ()
{
  V *volatile p = 0;
  std::fprintf (stderr, "bind0\n");
  bind0 (p);
  std::fprintf (stderr, "bind\n");
  bind (p);
  std::fprintf (stderr, "done\n");
  return 0;
}

// { dg-output "bind0\[^\n\r\]*(\n|\r\n|\r)\[^\n\r\]*runtime error: reference binding to null pointer of type 'struct D0'\[^\n\r\]*(\n|\r\n|\r)bind\[^\n\r\]*(\n|\r\n|\r)\[^\n\r\]*runtime error: reference binding to null pointer of type 'struct D2'\[^\n\r\]*(\n|\r\n|\r)done" { xfail *-*-* } }
