// E8: contract_violation::report() is idempotent -- calling it more than once
// returns the same rendered diagnostic (the lazy populator result is cached and
// reused, not regenerated).  White-boxed on the ASan-routed P3100 path, which
// installs a real lazy report populator.
// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fcontracts-p4301 -fsanitize=address" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce terminates after the handler" }

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  const char *r1 = v.report ();
  const char *r2 = v.report ();
  // Content must be identical across calls; the cache also returns one pointer.
  bool same_content = r1 && r2 && std::strcmp (r1, r2) == 0;
  std::printf (same_content ? "REPORT-IDEMPOTENT\n" : "REPORT-DIFFERS\n");
  std::printf (r1 == r2 ? "SAME-PTR\n" : "DIFF-PTR\n");
  std::fflush (stdout);
}

volatile int sink;

int __attribute__((noinline))
oob (int *p, int i)
{
  return p[i];
}

int main ()
{
  int *p = (int *) std::malloc (4 * sizeof (int));
  sink = oob (p, 100);   // heap out-of-bounds read -> routed to the handler
  std::free (p);
  return 0;
}

// The handler must observe an idempotent, cached report.
// { dg-output "REPORT-IDEMPOTENT\[\r\n\]+SAME-PTR" }
