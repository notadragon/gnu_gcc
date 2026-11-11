// PR c++/116102
// { dg-do run { target { c++11 && { ilp32 || lp64 } } } }
// { dg-options "-fsanitize=vptr" }

// -fsanitize=vptr checks the reference form of a static_cast downcast, as it
// does the pointer form: an lvalue or xvalue of type B cast to D& or D&& when
// the object is not a D.

#include <cstdio>

struct S { virtual ~S () {} int s = 1; };
struct T : S { int t = 2; };
struct U : S { int u = 3; };
struct V { virtual ~V () {} long v = 4; };
struct W : V, S { int w = 5; };	// S at a nonzero offset
struct X : V, S { int x = 6; };	// the same layout, not a W

__attribute__((noipa)) int lref (S &r) { return static_cast<T &> (r).s; }
__attribute__((noipa)) int rref (S &r)
{ return static_cast<T &&> (static_cast<S &&> (r)).s; }
__attribute__((noipa)) int cref (const S &r)
{ return static_cast<const T &> (r).s; }
__attribute__((noipa)) int offset (S &r) { return static_cast<W &> (r).s; }

int
main ()
{
  T t; U u; S s; W w; X x;
  // These do not report: the object is of the target type.
  lref (t);
  rref (t);
  cref (t);
  offset (w);
  __builtin_fprintf (stderr, "clean\n");
  // These do.
  lref (u);
  rref (s);
  cref (u);
  offset (x);
  return 0;
}

// { dg-output "^clean\[^\n\r]*(\n|\r\n|\r)" }
// { dg-output "\[^\n\r]*vptr-reference-downcast.C:18:\[0-9]*: runtime error: downcast of address 0x\[0-9a-fA-F]* which does not point to an object of type 'T'(\n|\r\n|\r)" }
// { dg-output "0x\[0-9a-fA-F]*: note: object is of type 'U'.*" }
// { dg-output "\[^\n\r]*vptr-reference-downcast.C:20:\[0-9]*: runtime error: downcast of address 0x\[0-9a-fA-F]* which does not point to an object of type 'T'(\n|\r\n|\r)" }
// { dg-output "0x\[0-9a-fA-F]*: note: object is of type 'S'.*" }
// { dg-output "\[^\n\r]*vptr-reference-downcast.C:22:\[0-9]*: runtime error: downcast of address 0x\[0-9a-fA-F]* which does not point to an object of type 'T'(\n|\r\n|\r)" }
// { dg-output "0x\[0-9a-fA-F]*: note: object is of type 'U'.*" }
// { dg-output "\[^\n\r]*vptr-reference-downcast.C:23:\[0-9]*: runtime error: downcast of address 0x\[0-9a-fA-F]* which does not point to an object of type 'W'(\n|\r\n|\r)" }
// { dg-output "0x\[0-9a-fA-F]*: note: object is of type 'X'" }
