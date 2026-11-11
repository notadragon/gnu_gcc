// P3100: -fsanitize-semantic=thread:quick_enforce terminates silently on the
// first detected data race WITHOUT ever entering the contract-violation handler
// and WITHOUT printing anything (the routed sanitizer emits nothing).  Accepted
// without -fcontracts-p4298.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fsanitize-semantic=thread:quick_enforce -ldl" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "quick_enforce terminates silently" }

#include <contracts>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>
#include "tsan_barrier.h"

// This handler must NOT run under quick_enforce.  If it did it would print
// "handler ..." and the dg-output check below would fail.
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d\n", (int) v.kind ());
  std::fflush (stdout);
}

static pthread_barrier_t barrier;
int Global;

static void *Thread1 (void *)
{
  barrier_wait (&barrier);
  usleep (1000);
  Global = 42;
  return nullptr;
}

static void *Thread2 (void *)
{
  Global = 43;
  barrier_wait (&barrier);
  return nullptr;
}

int main ()
{
  barrier_init (&barrier, 2);
  pthread_t t[2];
  pthread_create (&t[0], nullptr, Thread1, nullptr);
  pthread_create (&t[1], nullptr, Thread2, nullptr);
  pthread_join (t[0], nullptr);
  pthread_join (t[1], nullptr);
  std::printf ("NOTREACHED\n");
  std::fflush (stdout);
  return 0;
}

// quick_enforce = silent terminate, no handler, no output.  The program
// produces NO output before dying (dg-shouldfail asserts the non-zero exit).
// { dg-output "^\[ \t\r\n\]*$" }
