// -fsanitize=null checks a trivial assignment operator called through a
// null pointer, `sp->operator= (s)', as the store it is, like
// `(*sp).operator= (s)' and `*sp = s'; the store then faults (the fault is
// caught and ends the run).
// Mirror of llvm_llvm-project
// clang/test/Contracts/OpenBugs/ubsan-null-trivial-assign-call.cpp, an
// open-bug watch test there: stock Clang does not check it.
// { dg-do run { target c++11 } }
// { dg-options "-fsanitize=null" }

#include <csignal>
#include <unistd.h>

struct S { int m; };
__attribute__((noinline)) void assign (S *sp, S &s) { sp->operator= (s); }

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
  S *volatile vs = nullptr;
  S s{};
  assign (vs, s);
  return 0;
}

// { dg-output "store to null pointer of type 'struct S'(\n|\r\n|\r)" }
// { dg-output "faulted" }
