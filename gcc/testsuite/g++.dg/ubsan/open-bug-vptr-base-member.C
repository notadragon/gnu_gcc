// Watch test for GCC-773 (stock; Clang the same): -fsanitize=vptr does not
// check a member of a polymorphic base class reached through a glvalue of
// the wrong derived type ([expr.ref], ub:expr.ref.member.not.similar).  The
// member access is instrumented against the base subobject, whose dynamic
// type is right.  A member of the derived class itself is reported (the
// control).  XFAIL until fixed; see bug-reports/gcc-773/.
// { dg-do run { target { c++11 && { ilp32 || lp64 } } } }
// { dg-options "-fsanitize=vptr" }

#include <cstdio>

struct Base { virtual ~Base () {} int x = 1; };
struct A : Base { int a = 2; };
struct B : Base { int b = 3; };

__attribute__((noipa)) int own (A &r) { return reinterpret_cast<B &> (r).b; }
__attribute__((noipa)) int inherited (A &r)
{ return reinterpret_cast<B &> (r).x; }

int
main ()
{
  A a;
  own (a);
  std::fprintf (stderr, "control done\n");
  inherited (a);
  return 0;
}

// { dg-output "open-bug-vptr-base-member.C:16:\[0-9\]*: runtime error: member access within address 0x\[0-9a-fA-F\]* which does not point to an object of type 'B'.*control done\[^\n\r\]*(\n|\r\n|\r)\[^\n\r\]*open-bug-vptr-base-member.C:18:\[0-9\]*: runtime error: member access within address 0x\[0-9a-fA-F\]* which does not point to an object of type 'B'" { xfail *-*-* } }
