// Member calls and accesses on an object at offset zero from a null
// pointer, with -fsanitize=vptr routed to the contract-violation handler,
// for the p3100-null-vptr-offset-zero*.C tests: each run prints the number
// of reports of one form, and ` fault' if the program faulted.  None of the
// called functions reads through its object pointer, so a fault is the
// vptr check's, reading the vtable pointer through the null pointer; it is
// caught as SIGSEGV and resumed with siglongjmp.  The forms: a qualified
// virtual and a non-virtual call of, the address of a member of and a call
// through `.*' of `MW''s primary base `W2' (`P' not being polymorphic); a
// non-virtual call on and the address of a member of a member subobject at
// offset zero; and a call through `->*' on the base, the control.

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
namespace cs = std::contracts;

static int reports, faults;
void handle_contract_violation (const cs::contract_violation &) { ++reports; }

static sigjmp_buf env;
static void on_segv (int) { ++faults; siglongjmp (env, 1); }

#define NI __attribute__((noinline))
struct V
{
  virtual void h () {}
  void nv () {}
  int v;
};
struct P { int p; };
struct W2 { virtual void w2 () {} void nw () {} int r; };
struct MW : P, W2 { int z; };
struct H0 { V m; int x; };
typedef void (W2::*WPM) ();
NI void mwq (MW *mp) { mp->W2::w2 (); }
NI void mwnw (MW *mp) { mp->nw (); }
NI int *mwaddr (MW *mp) { return &mp->r; }
NI void mwdpmf (MW *mp, WPM p) { ((*mp).*p) (); }
NI void h0nv (H0 *hp) { hp->m.nv (); }
NI int *h0addr (H0 *hp) { return &hp->m.v; }
NI void mwpmf (MW *mp, WPM p) { (mp->*p) (); }

#define COUNT(name, ...)					\
  do								\
    {								\
      int before = reports, fbefore = faults;			\
      if (!sigsetjmp (env, 1))					\
	{							\
	  __VA_ARGS__;						\
	}							\
      std::printf (name " %d%s\n", reports - before,		\
		   faults != fbefore ? " fault" : "");		\
    }								\
  while (0)

int main ()
{
  std::signal (SIGSEGV, on_segv);
  int *volatile nv = nullptr;
  MW *mp = reinterpret_cast<MW *> (nv);
  H0 *hp = reinterpret_cast<H0 *> (nv);
  COUNT ("mwq", mwq (mp));
  COUNT ("mwnw", mwnw (mp));
  COUNT ("mwaddr", mwaddr (mp));
  COUNT ("mwdpmf", mwdpmf (mp, &W2::nw));
  COUNT ("h0nv", h0nv (hp));
  COUNT ("h0addr", h0addr (hp));
  COUNT ("mwpmf", mwpmf (mp, &W2::nw));
  return 0;
}
