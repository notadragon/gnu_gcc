// P3100: the effects of the contract-violation handler of a check built as
// IFN_UBSAN_NULL -- the null-dereference and alignment implicit assertions
// -- are seen by the code after the check when optimizing (GCC-805): a
// check whose reaction calls the handler is not a leaf call, so `reports',
// a static only the handler changes, is read again after it.  Each form is
// counted once at -O2, as the division by zero, a check that calls the
// handler directly, always was.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe,ub:basic.align.object.alignment:noexcept_observe,ub:expr.mul.div.by.zero:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

static int reports;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++reports;
}

struct S { int m; };
inline int *maddr (S *sp) { return &sp->m; }
static volatile int vz = 0;

int
main ()
{
  S *volatile vs = nullptr;
  S *s = vs;
  int before = reports;
  int *volatile r = maddr (s);
  std::printf ("null %d\n", reports - before);

  alignas (int) char buf[8] = {};
  int *pi = (int *) (buf + 1 + vz);
  before = reports;
  volatile int y = *pi;
  std::printf ("align %d\n", reports - before);

  int z = vz;
  before = reports;
  volatile int q = 7 / z;
  std::printf ("div %d\n", reports - before);
  (void) r; (void) y; (void) q;
  return 0;
}
// { dg-output "null 1(\n|\r\n|\r)align 1(\n|\r\n|\r)" }
// { dg-output "div 1(\n|\r\n|\r)" }
