// -fsanitize=null checks a downcast of `*p' to a reference, p a null
// pointer, with the base at a nonzero offset in the derived class --
// bound, its member's address taken, a member function called on it --
// as with the base at offset zero: the conversion tests p for null and
// yields a null reference, whose binding is reported.  (Stock Clang does
// not report them, CLANG-803; this is the mirror of Clang's
// OpenBugs/ubsan-null-downcast-ref-offset.cpp.)  Only the first report of
// each form is matched: GCC-871 (open-bug-null-downcast-ref-twice.C)
// reports the binding twice.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-options "-fsanitize=null -fsanitize-recover=null" }

#include <cstdio>

struct V { virtual void h () {} int v; };
struct X { virtual void x () {} long pad; };
struct D0 : V { int d; };		// V at offset 0
struct D2 : X, V			// V at offset 16
{
  int d;
  __attribute__((noinline)) void dn () {}
};
__attribute__((noipa)) D0 &bind0 (V *p) { return static_cast<D0 &> (*p); }
__attribute__((noipa)) D2 &bind (V *p) { return static_cast<D2 &> (*p); }
__attribute__((noipa)) int *addr (V *p) { return &static_cast<D2 &> (*p).d; }
__attribute__((noipa)) void call (V *p) { static_cast<D2 &> (*p).dn (); }

int
main ()
{
  V *volatile p = 0;
  std::fprintf (stderr, "bind0\n");
  bind0 (p);
  std::fprintf (stderr, "bind\n");
  bind (p);
  std::fprintf (stderr, "addr\n");
  addr (p);
  std::fprintf (stderr, "call\n");
  call (p);
  std::fprintf (stderr, "done\n");
  return 0;
}

// { dg-output "bind0\[^\n\r\]*(\n|\r\n|\r)\[^\n\r\]*runtime error: reference binding to null pointer of type 'struct D0'\[^\n\r\]*(\n|\r\n|\r)" }
// { dg-output "bind\[^\n\r\]*(\n|\r\n|\r)\[^\n\r\]*runtime error: reference binding to null pointer of type 'struct D2'.*" }
// { dg-output "addr\[^\n\r\]*(\n|\r\n|\r)\[^\n\r\]*runtime error: reference binding to null pointer of type 'struct D2'.*" }
// { dg-output "call\[^\n\r\]*(\n|\r\n|\r)\[^\n\r\]*runtime error: reference binding to null pointer of type 'struct D2'.*" }
// { dg-output "done" }
