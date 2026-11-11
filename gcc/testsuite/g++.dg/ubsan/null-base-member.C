// -fsanitize=null checks a member of a base class used through a null
// pointer in every form: the object pointer is checked before its
// conversion to the base, so a base at a nonzero offset (whose converted
// address is not null) is reported, and a virtual base is reported before
// its offset is loaded through the object's vptr, which then faults (the
// fault is caught and ends the run).  A base at offset 0 is the control.
// Mirror of llvm_llvm-project
// clang/test/Contracts/OpenBugs/ubsan-null-base-member.cpp, an open-bug
// watch test there: stock Clang misses every form but the control.
// { dg-do run { target c++11 } }
// { dg-options "-fsanitize=null" }

#include <csignal>
#include <cstdio>
#include <unistd.h>

struct B0 { int z; };
struct B { int b; __attribute__((noinline)) int f () { return 7; } };
struct D1 : B { int x; };		// B at offset 0
struct D : B0, B { int x; };		// B at offset 4
struct VB : virtual B { int x; };

__attribute__((noinline)) int *first_addr (D1 *p) { return &p->b; }
__attribute__((noinline)) int *n_addr (D *p) { return &p->b; }
__attribute__((noinline)) int n_call (D *p) { return p->f (); }
__attribute__((noinline)) B &n_bind (D *p) { return *p; }
__attribute__((noinline)) int v_rd (VB *p) { return p->b; }

static void
on_segv (int)
{
  static const char msg[] = "faulted\n";
  write (2, msg, sizeof msg - 1);
  _exit (0);
}

int
main ()
{
  std::signal (SIGSEGV, on_segv);
  first_addr (nullptr);
  std::fprintf (stderr, "control done\n");
  n_addr (nullptr);
  n_call (nullptr);
  (void) &n_bind (nullptr);
  std::fprintf (stderr, "nonvirtual done\n");
  return v_rd (nullptr);
}

// { dg-output "member access within null pointer of type 'struct D1'(\n|\r\n|\r)" }
// { dg-output "control done(\n|\r\n|\r)" }
// { dg-output "\[^\n\r]*member access within null pointer of type 'struct D'(\n|\r\n|\r)" }
// { dg-output "\[^\n\r]*member access within null pointer of type 'struct D'(\n|\r\n|\r)" }
// { dg-output "\[^\n\r]*member access within null pointer of type 'struct D'(\n|\r\n|\r)" }
// { dg-output "nonvirtual done(\n|\r\n|\r)" }
// { dg-output "\[^\n\r]*member access within null pointer of type 'struct VB'(\n|\r\n|\r)" }
// { dg-output "faulted" }
