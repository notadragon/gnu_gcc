// Routed TSan calls the contract-violation handler after releasing its report,
// slot and thread-registry locks, so a handler may start a thread.  A watchdog
// thread, running before the report, turns a regression's hang into exit
// status 1.
//
// Mirror: clang/test/Contracts/Runnable/p3100-tsan-handler-creates-thread.cpp
// in the llvm_llvm-project fork.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize-semantic=thread:noexcept_observe -ldl" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <atomic>
#include <contracts>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "tsan_barrier.h"

static std::atomic<bool> watching{false};
static void *watchdog (void *)
{
  watching = true;
  sleep (10);
  syscall (SYS_exit_group, 1);  // no sanitizer locks on the way out
  return nullptr;
}

static int joined = 0;
static void *helper (void *p) { return p; }
void handle_contract_violation (const std::contracts::contract_violation &)
{
  pthread_t t;
  pthread_create (&t, nullptr, helper, nullptr);
  pthread_join (t, nullptr);
  joined = 1;
}

static pthread_barrier_t barrier;
int Global;

static void *T1 (void *)
{
  barrier_wait (&barrier);
  usleep (1000);
  Global = 42;
  return nullptr;
}

static void *T2 (void *)
{
  Global = 43;
  barrier_wait (&barrier);
  return nullptr;
}

int main ()
{
  pthread_t w;
  pthread_create (&w, nullptr, watchdog, nullptr);
  while (!watching)
    ;
  barrier_init (&barrier, 2);
  pthread_t t[2];
  pthread_create (&t[0], nullptr, T1, nullptr);
  pthread_create (&t[1], nullptr, T2, nullptr);
  pthread_join (t[0], nullptr);
  pthread_join (t[1], nullptr);
  return joined ? 0 : 1;
}
