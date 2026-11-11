// A member function called through a pointer to member function whose
// `this' adjustment is not zero, on the object a null pointer points to,
// for the p3100-null-pmf-adjust*.C tests: each run prints the number of
// implicit-assertion reports of one form.  A fault after the report is
// caught as SIGSEGV and resumed with siglongjmp, so one run counts the
// reports of every form.

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation (const cs::contract_violation &v)
{
  if (v.kind () == cs::assertion_kind::implicit)
    ++reports;
}

static sigjmp_buf env;
static void on_segv (int) { siglongjmp (env, 1); }

struct B1 { int q; void g1 () {} };
struct B2 { int r; void g2 () {} };
struct M : B1, B2 { int z; };

#define NI __attribute__((noinline))
NI void arrow (M *mp, void (M::*p) ()) { (mp->*p) (); }
NI void star (M *mp, void (M::*p) ()) { ((*mp).*p) (); }
NI void sub (M *mp, void (M::*p) ()) { (mp[1].*p) (); }
NI void arith (M *mp, void (M::*p) ()) { ((mp + 1)->*p) (); }

#define COUNT(name, ...)					\
  do								\
    {								\
      int before = reports;					\
      if (!sigsetjmp (env, 1))					\
	{							\
	  __VA_ARGS__;						\
	}							\
      std::printf (name " %d\n", reports - before);		\
    }								\
  while (0)

int main ()
{
  std::signal (SIGSEGV, on_segv);
  int *volatile nv = nullptr;
  M *mp = reinterpret_cast<M *> (nv);
  M ma[3]{};
  void (M::*adj) () = &M::g2;
  void (M::*noadj) () = &M::g1;
  COUNT ("arrow", arrow (mp, adj));
  COUNT ("star", star (mp, adj));
  COUNT ("sub", sub (mp, adj));
  COUNT ("arith", arith (mp, adj));
  COUNT ("arrow0", arrow (mp, noadj));
  COUNT ("star0", star (mp, noadj));
  COUNT ("nonnull", arrow (ma, adj); star (ma, adj); sub (ma, adj);
	 arith (ma, adj); arrow (ma, noadj); star (ma, noadj));
  return 0;
}
