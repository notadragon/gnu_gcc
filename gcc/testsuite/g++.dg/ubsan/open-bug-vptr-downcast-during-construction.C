// Watch test for an open bug (GCC-717, stock GCC too; not a contracts bug):
// -fsanitize=vptr reports a static_cast downcast to F made in E's
// constructor while an F is being constructed.  The downcast is defined
// ([expr.static.cast]/2: the B is a base subobject of an F), but the vptr
// check compares against the dynamic type during construction, E.  The
// xfailed dg-output expects no report before "done", so a fix shows as an
// XPASS.
//
// Also the mirror of CLANG-688 in the llvm_llvm-project fork (the same
// program; stock Clang reports it too, llvm#59060).
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/vptr-downcast-during-construction.cpp.
// { dg-do run { target { c++11 && { ilp32 || lp64 } } } }
// { dg-options "-fsanitize=vptr" }

#include <cstdio>

struct B { virtual ~B () {} };
struct E : B { E (); };
struct F : E { int x = 0; };
E::E () { (void) static_cast<F *> (static_cast<B *> (this)); }

int
main ()
{
  F f;
  std::fprintf (stderr, "done\n");
  return 0;
}

// { dg-output "^done" { xfail *-*-* } }
