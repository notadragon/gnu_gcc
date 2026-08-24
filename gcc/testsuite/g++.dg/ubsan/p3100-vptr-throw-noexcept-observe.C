// P3100 x P4298 sanitizer-routed path (regression): a THROWING contract-
// violation handler under noexcept_observe must TERMINATE the program, not escape into the
// sanitizer runtime's non-unwindable C frames.  Routes the UBSan vptr check
// to the handler and has the handler throw; the routed-path terminate barrier
// (__contract_dispatch_core_noexcept, reached via __cxa_contract_violation_-
// sanitizer) must catch the throw and call std::terminate.
//
// main wraps the trigger in try/catch and RETURNS SUCCESS if an exception ever
// escapes: combined with dg-shouldfail this makes an escaped-and-caught
// exception (the bug) a test FAILURE, while a correct terminate (abort) is the
// expected failure.  dg-output confirms the handler ran (kind 7, semantic
// 6) and that no "ESCAPED"/"survived" text is produced.
//
// LINKAGE: the barrier (__contract_dispatch_core_noexcept) lives in the C++
// runtime object contracts_abi.o (libstdc++exp); the g++ driver force-links it
// (-u, see g++spec.cc) for every -fcontracts program, so a routed-only TU like
// this one gets it too and terminates rather than escaping -- this test also
// exercises that driver force-link.
// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "throwing handler under noexcept_observe terminates" }

#include <contracts>
#include <cstdio>

struct E {};
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
  std::fflush (stdout);
  throw E{};                     // must be caught by the barrier -> terminate
}

struct S { S () : a (0) {} virtual int v () { return 0; } int a; };
struct T : S { T () : b (0) {} int b; };
__attribute__((noinline)) static int access_b (T *p) { return p->b; }

int main ()
{
  try
    {
      S s; T *p = reinterpret_cast<T *> (&s); volatile int sink = access_b (p); (void) sink;
      std::printf ("survived\n");           // noexcept_observe non-throw path
    }
  catch (...)
    {
      std::printf ("ESCAPED\n");            // bug: exception escaped the barrier
    }
  std::fflush (stdout);
  return 0;                                  // success => dg-shouldfail FAILS
}

// { dg-output "^handler kind=7 semantic=6" }
