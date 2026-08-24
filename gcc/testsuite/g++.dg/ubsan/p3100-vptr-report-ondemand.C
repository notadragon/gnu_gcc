// P3100 RF5 (UBSan runtime routing): on the routed vptr path the sanitizer emits
// NOTHING itself.  The report leg captures the rendered UBSan text live and
// registers a lazy populator (CXA_FIELD_REPORT); the handler renders it ON
// DEMAND via contract_violation::report().  This proves:
//   (a) nothing (no "runtime error:" line) appears BEFORE the handler,
//   (b) the full vptr diagnostic appears only BETWEEN REPORT-BEGIN / REPORT-END.
// Default -fsanitize=vptr is recoverable, so with -fcontracts-p4298 the routed
// semantic is noexcept_observe and the program continues after the handler.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fcontracts-p4301 -fsanitize=vptr" }
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

struct S { S () : a (0) {} virtual int v () { return 0; } int a; };
struct T : S { T () : b (0) {} int b; };

int __attribute__((noinline))
access_b (T *p)
{
  return p->b;  // vptr: member access; *p is really an S, not a T.
}

int main ()
{
  S s;
  T *p = reinterpret_cast<T *> (&s);
  volatile int sink = access_b (p);
  (void) sink;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// Anchored at ^: the FIRST output must be REPORT-BEGIN, so any pre-handler UBSan
// text shifts it off the start and fails.  The vptr diagnostic ("does not point
// to an object of type") must appear between the markers, then execution
// continues to "survived".
// { dg-output "^REPORT-BEGIN\[\r\n\]+(?:.*)does not point to an object of type(?:.*)REPORT-END(?:.*)survived" }
