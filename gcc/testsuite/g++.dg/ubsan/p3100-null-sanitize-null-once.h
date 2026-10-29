// Member calls and reads through a null pointer, with -fsanitize=null
// routed to the contract-violation handler, for the
// p3100-null-sanitize-null-once*.C tests: each run prints the number of
// reports of one form.  A fault after the report is caught as SIGSEGV and
// resumed with siglongjmp.  The forms: a qualified virtual and a
// non-virtual call of a function of `MW''s primary base `W2' (offset
// zero, `P' not being polymorphic); a non-virtual call on a member
// subobject at offset zero; a read through a downcast of `*vp' to a
// reference, of a polymorphic class and of one that is not; the downcast
// alone, bound to the returned reference, of both; a non-virtual call
// through it.

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation (const cs::contract_violation &) { ++reports; }

static sigjmp_buf env;
static void on_segv (int) { siglongjmp (env, 1); }

#define NI __attribute__((noinline))
struct P { int p; };
struct W2 { virtual void w2 () {} void nw () {} int r; };
struct MW : P, W2 { int z; };
struct V { virtual void h () {} void nv () {} int v; };
struct D : V { int d; void dn () {} };
struct H0 { V m; int x; };
struct S { int s; };
struct DS : S { int t; };
NI void mwq (MW *mp) { mp->W2::w2 (); }
NI void mwnw (MW *mp) { mp->nw (); }
NI void h0nv (H0 *hp) { hp->m.nv (); }
NI int drdc (V *vp) { return static_cast<D &> (*vp).d; }
NI int dsrd (S *sp) { return static_cast<DS &> (*sp).t; }
NI D &dref (V *vp) { return static_cast<D &> (*vp); }
NI DS &dsref (S *sp) { return static_cast<DS &> (*sp); }
NI void dcall (V *vp) { static_cast<D &> (*vp).dn (); }

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
  COUNT ("mwq", mwq (reinterpret_cast<MW *> (nv)));
  COUNT ("mwnw", mwnw (reinterpret_cast<MW *> (nv)));
  COUNT ("h0nv", h0nv (reinterpret_cast<H0 *> (nv)));
  COUNT ("drdc", drdc (reinterpret_cast<V *> (nv)));
  COUNT ("dsrd", dsrd (reinterpret_cast<S *> (nv)));
  COUNT ("dref", dref (reinterpret_cast<V *> (nv)));
  COUNT ("dsref", dsref (reinterpret_cast<S *> (nv)));
  COUNT ("dcall", dcall (reinterpret_cast<V *> (nv)));
  return 0;
}
