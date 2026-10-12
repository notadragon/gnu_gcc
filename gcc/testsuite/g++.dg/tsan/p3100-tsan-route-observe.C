// P3100: under -fcontracts-p3100, a ThreadSanitizer-detected data race is routed
// to the contract-violation handler.  With -fcontracts-p4298 and
// -fsanitize-semantic=thread:noexcept_observe the thread check resolves to
// noexcept_observe: the handler runs (reporting the race as an implicit contract
// assertion) and the program CONTINUES, exiting normally (the routed observe
// report is not counted, so TSan does not force a nonzero exit).

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize-semantic=thread:noexcept_observe -ldl" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>
#include "tsan_barrier.h"

// Non-throwing handler: a throwing handler would std::terminate() from inside
// libtsan's noexcept report path, which is why the routed semantic is
// noexcept_observe.  Print kind AND semantic to assert kind()==implicit (7) and
// semantic()==noexcept_observe (6).
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
  Global = 42;			// races with Thread2's write below
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
  // noexcept_observe = report + continue: we reach here after the race.
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// The handler ran, reporting an implicit contract assertion (kind 7) with the
// NOEXCEPT_OBSERVE evaluation semantic (6).  The sanitizer emits NOTHING on the
// routed path (this handler does not call report()), so the handler line is the
// FIRST output -- anchor at ^ so any leaked pre-handler TSan text fails.
// { dg-output "^handler kind=7 semantic=6" }
// { dg-output ".*survived" }
