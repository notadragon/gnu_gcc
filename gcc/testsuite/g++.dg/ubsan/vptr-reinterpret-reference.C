// { dg-do run { target { c++11 && { ilp32 || lp64 } } } }
// { dg-options "-fsanitize=vptr" }

// -fsanitize=vptr reports a member read through a glvalue of polymorphic
// type B that refers to an object of the unrelated polymorphic type A, in
// every form: the reinterpret_cast reference used directly, a reference
// variable, a pointer, a member call, a B && variable, a B & parameter.
// Mirror of llvm_llvm-project CLANG-752 (see the end of the file).

#include <cstdio>

struct A { virtual ~A () {} int i, j; };
struct B { virtual ~B () {} int i, j; __attribute__((noipa)) int f () { return 3; } };

__attribute__((noipa)) int direct (A &a) { return reinterpret_cast<B &> (a).j; }
__attribute__((noipa)) int viaref (A &a)
{ B &b = reinterpret_cast<B &> (a); return b.j; }
__attribute__((noipa)) int viaptr (A &a)
{ B *b = reinterpret_cast<B *> (&a); return b->j; }
__attribute__((noipa)) int viacall (A &a)
{ B &b = reinterpret_cast<B &> (a); return b.f (); }
__attribute__((noipa)) int viarvref (A &a)
{ B &&b = reinterpret_cast<B &&> (a); return b.j; }
__attribute__((noipa)) int param (B &b) { return b.j; }

int
main ()
{
  A a;
  a.i = 1;
  a.j = 2;
  __builtin_fprintf (stderr, "start\n");
  direct (a);
  viaref (a);
  viaptr (a);
  viacall (a);
  viarvref (a);
  param (reinterpret_cast<B &> (a));
  __builtin_fprintf (stderr, "end\n");
  return 0;
}

// { dg-output "^start\[^\n\r]*(\n|\r\n|\r)" }
// { dg-output "\[^\n\r]*vptr-reinterpret-reference.C:15:\[0-9]*: runtime error: member access within address 0x\[0-9a-fA-F]* which does not point to an object of type 'B'(\n|\r\n|\r)" }
// { dg-output "0x\[0-9a-fA-F]*: note: object is of type 'A'.*" }
// { dg-output "\[^\n\r]*vptr-reinterpret-reference.C:17:\[0-9]*: runtime error: member access within address 0x\[0-9a-fA-F]* which does not point to an object of type 'B'(\n|\r\n|\r)" }
// { dg-output "0x\[0-9a-fA-F]*: note: object is of type 'A'.*" }
// { dg-output "\[^\n\r]*vptr-reinterpret-reference.C:19:\[0-9]*: runtime error: member access within address 0x\[0-9a-fA-F]* which does not point to an object of type 'B'(\n|\r\n|\r)" }
// { dg-output "0x\[0-9a-fA-F]*: note: object is of type 'A'.*" }
// { dg-output "\[^\n\r]*vptr-reinterpret-reference.C:21:\[0-9]*: runtime error: member call on address 0x\[0-9a-fA-F]* which does not point to an object of type 'B'(\n|\r\n|\r)" }
// { dg-output "0x\[0-9a-fA-F]*: note: object is of type 'A'.*" }
// { dg-output "\[^\n\r]*vptr-reinterpret-reference.C:23:\[0-9]*: runtime error: member access within address 0x\[0-9a-fA-F]* which does not point to an object of type 'B'(\n|\r\n|\r)" }
// { dg-output "0x\[0-9a-fA-F]*: note: object is of type 'A'.*" }
// { dg-output "\[^\n\r]*vptr-reinterpret-reference.C:24:\[0-9]*: runtime error: member access within address 0x\[0-9a-fA-F]* which does not point to an object of type 'B'(\n|\r\n|\r)" }
// { dg-output "0x\[0-9a-fA-F]*: note: object is of type 'A'.*" }
// { dg-output "end" }

// The mirror is llvm_llvm-project
// clang/test/Contracts/OpenBugs/vptr-reinterpret-reference.cpp, where stock
// Clang misses the read through the reference variable, the B && variable
// and the B & parameter (CLANG-752); it reports the member call.
