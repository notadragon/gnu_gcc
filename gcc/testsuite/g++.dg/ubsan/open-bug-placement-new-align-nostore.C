// WATCH TEST for GCC-877 (stock; bug-reports/gcc-877/): the output test
// XFAILs until it is fixed.  -fsanitize=alignment does not check the
// pointer of a placement new that stores nothing -- an empty class, `new
// (b) S ()', and default-initialization, `new (b) T' -- since it checks
// only the stores of the initialization; creating an object at a
// misaligned address is undefined all the same.  Clang's
// -fsanitize=alignment checks the pointer ("constructor call on misaligned
// address").  A placement new that stores, the control, is reported.
// Under -fcontracts-p3100 the native .nostore check reports both
// (p3100-placement-align-char-array*.C).
// { dg-do run { target { c++11 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-options "-fsanitize=alignment -fsanitize-recover=alignment" }

#include <cstdio>
#include <new>

struct alignas (4) S {};
struct T { int a, b; };
__attribute__((noipa)) void empty (char *b) { new (b) S (); }
__attribute__((noipa)) void dflt (char *b) { new (b) T; }
__attribute__((noipa)) void store (char *b) { new (b) int (1); }
alignas (8) char buf[16];

int
main ()
{
  std::fputs ("empty\n", stderr);
  empty (buf + 1);
  std::fputs ("dflt\n", stderr);
  dflt (buf + 1);
  std::fputs ("store\n", stderr);
  store (buf + 1);
  std::fputs ("done\n", stderr);
  return 0;
}

// { dg-output "empty(\n|\r\n|\r)\[^\n\r\]*runtime error: \[^\n\r\]*misaligned address\[^\n\r\]*'S'.*dflt(\n|\r\n|\r)\[^\n\r\]*runtime error: \[^\n\r\]*misaligned address\[^\n\r\]*'T'.*store(\n|\r\n|\r)\[^\n\r\]*runtime error: store to misaligned address.*done" { xfail *-*-* } }
