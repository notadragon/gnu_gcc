// A call through a pointer to member function on the object a null pointer
// points to, virtual or not, for the p3100-null-virtual-pmf*.C tests: each
// run prints the number of implicit-assertion reports of one form.  A
// fault after the report is caught as SIGSEGV and resumed with siglongjmp,
// so one run counts the reports of every form.

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

#define NI __attribute__((noinline))
struct V
{
  virtual void h () {}
  void nv () {}
  int v;
  NI void self (void (V::*p) ()) { (this->*p) (); }
};
struct W1 { virtual void w1 () {} int q; };
struct W2 { virtual void w2 () {} void nw2 () {} int r; };
struct MW : W1, W2 { int z; };

typedef void (V::*VPM) ();
typedef void (MW::*MPM) ();
NI void call (V *vp, VPM p) { (vp->*p) (); }
NI void callsub (V *vp, VPM p) { (vp[1].*p) (); }
NI void callarith (V *vp, VPM p) { ((vp + 1)->*p) (); }
NI void calladj (MW *mp, MPM p) { (mp->*p) (); }
NI void calladjsub (MW *mp, MPM p) { (mp[1].*p) (); }

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
  V *vp = reinterpret_cast<V *> (nv);
  MW *mp = reinterpret_cast<MW *> (nv);
  V va[2];
  MW ma[2];
  COUNT ("virt", call (vp, &V::h));
  COUNT ("virtsub", callsub (vp, &V::h));
  COUNT ("virtarith", callarith (vp, &V::h));
  COUNT ("virtadj", calladj (mp, &MW::w2));
  COUNT ("virtadjsub", calladjsub (mp, &MW::w2));
  COUNT ("nonvirt", call (vp, &V::nv));
  COUNT ("nonvirtsub", callsub (vp, &V::nv));
  COUNT ("nonvirtarith", callarith (vp, &V::nv));
  COUNT ("nonvirtadj", calladj (mp, &MW::nw2));
  COUNT ("nonvirtadjsub", calladjsub (mp, &MW::nw2));
  COUNT ("self", vp->self (&V::h));
  COUNT ("nonnull", call (va, &V::h); callsub (va, &V::h);
	 callarith (va, &V::h); calladj (ma, &MW::w2);
	 calladjsub (ma, &MW::w2); call (va, &V::nv); callsub (va, &V::nv);
	 callarith (va, &V::nv); calladj (ma, &MW::nw2);
	 calladjsub (ma, &MW::nw2); va[0].self (&V::h));
  return 0;
}
