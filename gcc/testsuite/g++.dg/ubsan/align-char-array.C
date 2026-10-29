// -fsanitize=alignment reports an access through a pointer an explicit
// cast formed from a character array, misaligned by the offset --
// `*(int *) (buf + o)' read and stored to, `*(int *) &buf[o]' -- as
// through a `char *' parameter, the control.  The mirror of
// llvm_llvm-project's watch test OpenBugs/ubsan-alignment-char-array.cpp
// for stock Clang's miss of these forms (CLANG-809); GCC reports them.
// Its placement-new forms, which GCC's -fsanitize=alignment does not
// check either, are GCC-877's (open-bug-placement-new-align-nostore.C).
// { dg-do run { target { c++11 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-options "-fsanitize=alignment -fsanitize-recover=alignment" }

#include <cstdio>

static volatile int reports;
extern "C" void __ubsan_on_report () { reports = reports + 1; }

#define NI __attribute__((noinline))
alignas (8) char buf[16];
NI int load (int o) { return *(int *) (buf + o); }
NI void store (int o) { *(int *) (buf + o) = 1; }
NI int load_elem (int o) { return *(int *) &buf[o]; }
NI int load_ptr (char *c, int o) { return *(int *) (c + o); }

int
main ()
{
  int volatile one = 1;
  int before = reports;
  load (one);
  std::printf ("load %d\n", reports - before);
  before = reports;
  store (one);
  std::printf ("store %d\n", reports - before);
  before = reports;
  load_elem (one);
  std::printf ("load_elem %d\n", reports - before);
  before = reports;
  load_ptr (buf, one);
  std::printf ("load_ptr %d\n", reports - before);
  return 0;
}

// { dg-output "load 1(\n|\r\n|\r).*store 1(\n|\r\n|\r).*load_elem 1(\n|\r\n|\r).*load_ptr 1(\n|\r\n|\r)" }
