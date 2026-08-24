// P3100 RF5 (UBSan runtime routing): on the routed alignment path the sanitizer
// emits NOTHING itself.  The report leg captures the rendered UBSan text live
// and registers a lazy populator (CXA_FIELD_REPORT); the handler renders it ON
// DEMAND via contract_violation::report().  This proves (a) nothing appears
// before the handler and (b) the full diagnostic appears only between the
// markers.  Default -fsanitize=alignment is recoverable, so with
// -fcontracts-p4298 the routed semantic is noexcept_observe and execution
// continues after the handler.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fcontracts-p4301 -fsanitize=alignment" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("REPORT-BEGIN\n");
  std::fflush (stdout);
  const char *r = v.report ();
  std::printf ("%s\n", r ? r : "(null)");
  std::fflush (stdout);
  std::printf ("REPORT-END\n");
  std::fflush (stdout);
}

int __attribute__((noinline))
load (int *p)
{
  return *p;
}

int main ()
{
  alignas (int) char buf[8];
  int *p = reinterpret_cast<int *> (buf + 1);
  volatile int sink = load (p);
  (void) sink;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// Anchored at ^: the FIRST output must be REPORT-BEGIN.  The alignment
// diagnostic ("misaligned address") appears between the markers, then execution
// continues to "survived".
// { dg-output "^REPORT-BEGIN\[\r\n\]+(?:.*)misaligned address(?:.*)REPORT-END(?:.*)survived" }
