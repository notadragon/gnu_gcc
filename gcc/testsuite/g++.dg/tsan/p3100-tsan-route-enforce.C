// P3100: with -fcontracts-p4298 and -fsanitize-semantic=thread:noexcept_enforce
// the routed thread check resolves to noexcept_enforce: the handler runs, then
// the program terminates.  The handler owns all output (the sanitizer prints
// nothing on the routed path).

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize-semantic=thread:noexcept_enforce -ldl" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce terminates after the handler" }

#include <contracts>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>
#include "tsan_barrier.h"

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
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

// Handler runs (kind 7, semantic noexcept_enforce = 7), then terminate.
// { dg-output "^handler kind=7 semantic=7" }
