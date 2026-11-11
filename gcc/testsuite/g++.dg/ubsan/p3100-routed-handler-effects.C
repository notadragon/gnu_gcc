// P3100: the effects of the contract-violation handler reached from a
// routed sanitizer check are seen by the code after the check when
// optimizing (GCC-805): a check of a sanitizer routed to the handler with a
// semantic that calls it is not a leaf call (IFN_UBSAN_BOUNDS,
// IFN_UBSAN_NULL), so `reports', a static only the handler changes, is read
// again after it.  Each form is counted once at -O2.
// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -O2 -fsanitize=bounds,alignment -fsanitize-semantic=bounds:noexcept_observe,alignment:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

static int reports;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++reports;
}

static volatile int vz = 0;

int
main ()
{
  int a[4] = {};
  int i = vz + 4;
  int before = reports;
  volatile int x = a[i];
  std::printf ("bounds %d\n", reports - before);

  alignas (int) char buf[8] = {};
  int *pi = (int *) (buf + 1 + vz);
  before = reports;
  volatile int y = *pi;
  std::printf ("align %d\n", reports - before);
  (void) x; (void) y;
  return 0;
}
// { dg-output "bounds 1(\n|\r\n|\r)align 1(\n|\r\n|\r)" }
