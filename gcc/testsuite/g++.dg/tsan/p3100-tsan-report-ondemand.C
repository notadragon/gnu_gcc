// P3100 RF: on the routed path the sanitizer emits NOTHING; the handler owns all
// output and can retrieve the sanitizer's description on demand via
// contract_violation::report().  This test's handler calls report() and prints
// it, and asserts nothing leaks before it.
// (v1: libtsan returns a concise description; full multi-line capture is a
// documented follow-up.)

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fcontracts-p4301 -fsanitize-semantic=thread:noexcept_observe -ldl" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>
#include "tsan_barrier.h"

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  const char *r = v.report ();
  std::printf ("report=%s\n", r ? r : "(null)");
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
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// The report() text (concise v1 description) is the first output, with nothing
// leaked before it.
// { dg-output "^report=ThreadSanitizer: data race" }
// { dg-output ".*survived" }
