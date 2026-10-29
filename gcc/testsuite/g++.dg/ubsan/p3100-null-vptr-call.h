// Member calls and accesses through a null pointer to an object of a
// polymorphic class, with -fsanitize=vptr routed to the contract-violation
// handler as well as ub:expr.unary.dereference.nullptr configured, for the
// p3100-null-vptr-call*.C tests: each run prints the number of reports of
// one form.  A fault after the report is caught as SIGSEGV and resumed with
// siglongjmp, so one run counts the reports of every form.  The forms: a
// virtual and a non-virtual call, a read and a write; a read of and a call
// on a member; a qualified call; a virtual call and a read through a base
// at offset zero; a call devirtualized through a `final' class; a virtual
// call on an element of a member array; a virtual call, a read and a
// qualified call of a polymorphic base's function or member -- `MW''s
// `W2', which, `P' not being polymorphic, is its primary base, at offset
// zero; a read, a virtual and a non-virtual call through a virtual base; a
// call on `*vp', on `vp[1]', and a reference bound to `*vp'; a downcast of
// `*vp' to a reference and a read through it; a reference to the virtual
// base bound to `*vbp' and a virtual call on `vbp[1]'.  Then Clang's: the
// same three forms of a base at a nonzero offset, `MV''s `W2' after the
// polymorphic `W1'; and a read of `vbp[1]''s member of the virtual base
// (GCC-861).

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
struct V
{
  virtual void h () {}
  void nv () {}
  int v;
};
struct H { int x; V m; V arr[2]; };
struct D : V { int d; };
struct F final { virtual void h () {} int f; };
struct P { int p; };
struct W2 { virtual void w2 () {} int r; };
struct MW : P, W2 { int z; };
struct VB : virtual V { int w; };
struct W1 { virtual void w1 () {} int q; };
struct MV : W1, W2 { int z; };
NI void vh (V *vp) { vp->h (); }
NI void vnv (V *vp) { vp->nv (); }
NI int vrd (V *vp) { return vp->v; }
NI void vwr (V *vp) { vp->v = 1; }
NI int hrd (H *hp) { return hp->m.v; }
NI void hnv (H *hp) { hp->m.nv (); }
NI void qh (V *vp) { vp->V::h (); }
NI void dh (D *dp) { dp->h (); }
NI int drd (D *dp) { return dp->v; }
NI void fin (F *fp) { fp->h (); }
NI void memarr (H *hp) { hp->arr[1].h (); }
NI void base (MW *mp) { mp->w2 (); }
NI int mwrd (MW *mp) { return mp->r; }
NI void mwq (MW *mp) { mp->W2::w2 (); }
NI int vbrd (VB *p) { return p->v; }
NI void vbh (VB *p) { p->h (); }
NI void vbnv (VB *p) { p->nv (); }
NI void star (V *vp) { (*vp).nv (); }
NI void sub (V *vp) { vp[1].nv (); }
NI V &bind (V *vp) { return *vp; }
NI D &dref (V *vp) { return static_cast<D &> (*vp); }
NI int drdc (V *vp) { return static_cast<D &> (*vp).d; }
NI V &vbref (VB *p) { return *p; }
NI void vbsub (VB *p) { p[1].h (); }
NI void mvh (MV *mp) { mp->w2 (); }
NI int mvrd (MV *mp) { return mp->r; }
NI void mvq (MV *mp) { mp->W2::w2 (); }
NI int vbsubrd (VB *p) { return p[1].v; }

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
  H *hp = reinterpret_cast<H *> (nv);
  D *dp = reinterpret_cast<D *> (nv);
  F *fp = reinterpret_cast<F *> (nv);
  MW *mp = reinterpret_cast<MW *> (nv);
  VB *vbp = reinterpret_cast<VB *> (nv);
  MV *mvp = reinterpret_cast<MV *> (nv);
  COUNT ("vh", vh (vp));
  COUNT ("vnv", vnv (vp));
  COUNT ("vrd", vrd (vp));
  COUNT ("vwr", vwr (vp));
  COUNT ("hrd", hrd (hp));
  COUNT ("hnv", hnv (hp));
  COUNT ("qh", qh (vp));
  COUNT ("dh", dh (dp));
  COUNT ("drd", drd (dp));
  COUNT ("fin", fin (fp));
  COUNT ("memarr", memarr (hp));
  COUNT ("base", base (mp));
  COUNT ("mwrd", mwrd (mp));
  COUNT ("mwq", mwq (mp));
  COUNT ("vbrd", vbrd (vbp));
  COUNT ("vbh", vbh (vbp));
  COUNT ("vbnv", vbnv (vbp));
  COUNT ("star", star (vp));
  COUNT ("sub", sub (vp));
  COUNT ("bind", bind (vp));
  COUNT ("dref", dref (vp));
  COUNT ("drdc", drdc (vp));
  COUNT ("vbref", vbref (vbp));
  COUNT ("vbsub", vbsub (vbp));
  COUNT ("mvh", mvh (mvp));
  COUNT ("mvrd", mvrd (mvp));
  COUNT ("mvq", mvq (mvp));
  COUNT ("vbsubrd", vbsubrd (vbp));
  return 0;
}
