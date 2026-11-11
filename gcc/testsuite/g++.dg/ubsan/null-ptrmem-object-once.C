// With -fsanitize=null alone (no contract options), a pointer to data
// member applied through a null pointer, `sp->*pm' with pm the member at
// offset zero, is reported once, by the check of the object pointer of
// `->*' (null-ptrmem-object.C): the read through the member's address,
// null too, is not checked again (GCC-853: twice).
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/ubsan-ptrmem-null-once.cpp (Clang reports
// the read, "load of null pointer": it makes no check of the object
// pointer of `->*' without -fcontracts-p3100).
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-options "-fsanitize=null -fsanitize-recover=null" }

#include <csetjmp>
#include <csignal>
#include <cstdio>

static sigjmp_buf env;
static void on_segv (int) { siglongjmp (env, 1); }

struct S { int a; int b; };

__attribute__((noinline)) int pmd (S *sp, int S::*pm) { return sp->*pm; }

int
main ()
{
  std::signal (SIGSEGV, on_segv);
  S *volatile sp = 0;
  if (!sigsetjmp (env, 1))
    pmd (sp, &S::a);
  std::fprintf (stderr, "done\n");
  return 0;
}

// { dg-output "^\[^\n\r\]*runtime error: member access within null pointer of type 'struct S'\[^\n\r\]*(\n|\r\n|\r)done" }
