// A read through a null pointer of a member of a virtual base of an element
// at a constant subscript, `vbp[1].v', for the
// p3100-null-vptr-vbase-subscript*.C tests, which print its number of
// reports.  A fault after the report is caught as SIGSEGV and resumed with
// siglongjmp.

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>

static int reports;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++reports;
}

static sigjmp_buf env;
static void on_segv (int) { siglongjmp (env, 1); }

struct V { virtual void h () {} int v; };
struct VB : virtual V { int w; };
__attribute__((noinline)) int vbsubrd (VB *p) { return p[1].v; }

int
main ()
{
  std::signal (SIGSEGV, on_segv);
  VB *volatile vbp = nullptr;
  int before = reports;
  if (!sigsetjmp (env, 1))
    vbsubrd (vbp);
  std::printf ("vbsubrd %d\n", reports - before);
  return 0;
}
