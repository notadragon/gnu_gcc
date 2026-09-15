// D4298: noexcept_enforce behaves just like enforce when the handler
// returns normally: the ABI must abort() promptly, not fall through to
// __builtin_unreachable() (undefined behavior) and not std::terminate().
//
// Discrimination: a SIGABRT handler prints a marker (checked via dg-output)
// then resets to the default disposition and re-raises, so the process
// still dies abnormally (dg-shouldfail) once the marker has been observed.
// A SIGALRM fired after a few seconds forces a fast, distinctly-marked exit
// if the process is still alive by then, instead of relying on the (very
// slow) DejaGnu execution timeout to reveal a hang -- the undefined-behavior
// fallthrough this test guards against was observed to manifest as a hang,
// not an immediate crash, when this fix is reverted.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce aborts rather than hanging or falling through" }
// { dg-output "GOT_SIGABRT(\n|\r\n|\r)" }

#include <contracts>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

void handle_contract_violation(const std::contracts::contract_violation&)
{
  // Returns normally; the ABI must still abort() on completion.
}

extern "C" void on_sigabrt(int)
{
  std::fputs("GOT_SIGABRT\n", stderr);
  std::fflush(stderr);
  std::signal(SIGABRT, SIG_DFL);
  std::raise(SIGABRT);
}

extern "C" void on_alarm(int)
{
  // Still alive after the alarm: the ABI hung (or otherwise failed to
  // abort promptly) instead of aborting.  Exit fast with a distinct,
  // non-abort status rather than waiting on the DejaGnu execution timeout;
  // the missing "GOT_SIGABRT" marker in dg-output will fail the test.
  _exit(3);
}

int f(int x) pre(x > 0) { return x; }

int main()
{
  std::signal(SIGABRT, on_sigabrt);
  std::signal(SIGALRM, on_alarm);
  alarm(5);
  f(-1);
  return 0;
}
