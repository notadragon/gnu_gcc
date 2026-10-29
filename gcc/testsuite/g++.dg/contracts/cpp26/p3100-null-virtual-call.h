// A virtual call through a null pointer, and calls that read no vtable
// pointer, for the p3100-null-virtual-call*.C tests: each run prints the
// number of implicit-assertion reports of one form.  A fault after the
// report is caught as SIGSEGV and resumed with siglongjmp, so one run
// counts the reports of every form.

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
};
struct P { int p; };
struct W2 { virtual void w2 () {} int r; };
struct MW : P, W2
{
  int z;
  NI void g () { w2 (); }
  NI void gl () { [this] { w2 (); } (); }
};
struct H { int x; V m; V arr[2]; };
struct HH { int y; H h; };
struct VD { virtual ~VD () {} int d; };
struct F final { virtual void h () {} int f; };
struct D : V { void h () final {} };
NI void vcall (V *vp) { vp->h (); }
NI void vdcall (V *vp) { (*vp).h (); }
NI void vsub (V *vp) { vp[1].h (); }
NI void vsubi (V *vp, int i) { vp[i].h (); }
NI void varith (V *vp) { (vp + 1)->h (); }
NI void varithi (V *vp, int i) { (vp + i)->h (); }
NI void vbase (MW *mp) { mp->w2 (); }
NI void vmemarr (H *hp) { hp->arr[1].h (); }
NI void vmemdecay (H *hp) { hp->arr->h (); }
NI void vdtor (VD *dp) { dp->~VD (); }
NI void vmember (H *hp) { hp->m.h (); }
NI void final (F *fp) { fp->h (); }
NI void qual (V *vp) { vp->V::h (); }
NI void dfinal (D *dp) { dp->h (); }
NI void nonvirt (V *vp) { vp->nv (); }
// GCC's: a primary base's virtual function (offset zero) through `*mp', an
// element at a constant subscript or offset, `this' (DECISIONS.md E106
// (a)) or a reference, and one on an element of a member array of a
// member.
NI void vdbase (MW *mp) { (*mp).w2 (); }
NI void vsubbase (MW *mp) { mp[1].w2 (); }
NI void varithbase (MW *mp) { (mp + 1)->w2 (); }
NI void vthis (MW *mp) { mp->g (); }
NI void vlambda (MW *mp) { mp->gl (); }
NI void vrefbody (MW &r) { r.w2 (); }
NI void vref (MW *mp) { vrefbody (*mp); }
NI void vnested (HH *hp) { hp->h.arr[1].h (); }

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
  H *hp = reinterpret_cast<H *> (nv);
  VD *vdp = reinterpret_cast<VD *> (nv);
  F *fp = reinterpret_cast<F *> (nv);
  D *dp = reinterpret_cast<D *> (nv);
  HH *hhp = reinterpret_cast<HH *> (nv);
  volatile int one = 1;
  V va[2];
  MW m;
  H h;
  F f;
  D d;
  MW ma[2];
  HH hh;
  COUNT ("vcall", vcall (vp));
  COUNT ("vdcall", vdcall (vp));
  COUNT ("vsub", vsub (vp));
  COUNT ("vsubi", vsubi (vp, one));
  COUNT ("varith", varith (vp));
  COUNT ("varithi", varithi (vp, one));
  COUNT ("vbase", vbase (mp));
  COUNT ("vmemarr", vmemarr (hp));
  COUNT ("vmemdecay", vmemdecay (hp));
  COUNT ("vdtor", vdtor (vdp));
  COUNT ("vmember", vmember (hp));
  COUNT ("final", final (fp));
  COUNT ("qual", qual (vp));
  COUNT ("dfinal", dfinal (dp));
  COUNT ("nonvirt", nonvirt (vp));
  COUNT ("nonnull", vcall (va); vdcall (va); vsub (va); vsubi (va, one);
	 varith (va); varithi (va, one); vbase (&m); vmemarr (&h);
	 vmemdecay (&h); vmember (&h); final (&f); qual (va); dfinal (&d);
	 nonvirt (va));
  COUNT ("vdbase", vdbase (mp));
  COUNT ("vsubbase", vsubbase (mp));
  COUNT ("varithbase", varithbase (mp));
  COUNT ("vthis", vthis (mp));
  COUNT ("vlambda", vlambda (mp));
  COUNT ("vref", vref (mp));
  COUNT ("vnested", vnested (hhp));
  COUNT ("nonnull_gcc", vdbase (ma); vsubbase (ma); varithbase (ma);
	 vthis (ma); vlambda (ma); vref (ma); vnested (&hh));
  return 0;
}
