// Non-virtual member calls through a null pointer to an object of a
// polymorphic class, or to an object holding one, with -fsanitize=null and
// -fsanitize=vptr routed to the contract-violation handler as well as
// ub:expr.unary.dereference.nullptr configured, for the
// p3100-null-vptr-sanitize-null*.C tests: each run prints the number of
// reports of one form, and ` fault' if the program faulted.  None of the
// called functions reads through its object pointer, so a fault is the
// vptr check's, reading the vtable pointer through the null pointer; it is
// caught as SIGSEGV and resumed with siglongjmp.  The forms: a qualified
// virtual and a non-virtual call of a polymorphic base at a nonzero
// offset, `MV''s `W2' after the polymorphic `W1'; a non-virtual call on a
// member at a nonzero offset; a non-virtual call on `vp[1]' and on `*vp'.

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
struct H { int x; V m; V arr[2]; };
struct W1 { virtual void w1 () {} int q; };
struct W2 { virtual void w2 () {} void nw () {} int r; };
struct MV : W1, W2 { int z; };
NI void mvq (MV *mp) { mp->W2::w2 (); }
NI void mvnw (MV *mp) { mp->nw (); }
NI void hnv (H *hp) { hp->m.nv (); }
NI void sub (V *vp) { vp[1].nv (); }
NI void vnv (V *vp) { vp->nv (); }

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
  V *vp = reinterpret_cast<V *> (nv);
  H *hp = reinterpret_cast<H *> (nv);
  MV *mvp = reinterpret_cast<MV *> (nv);
  COUNT ("mvq", mvq (mvp));
  COUNT ("mvnw", mvnw (mvp));
  COUNT ("hnv", hnv (hp));
  COUNT ("sub", sub (vp));
  COUNT ("vnv", vnv (vp));
  return 0;
}
