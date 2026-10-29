// A call through a pointer to member function, a virtual call and a
// pointer-to-data-member access through a null pointer, with
// -fsanitize=null routed to the contract-violation handler as well as
// ub:expr.unary.dereference.nullptr configured, for the
// p3100-null-pmf-sanitize-null*.C tests: each run prints the number of
// reports of one form.  A fault after the report is caught as SIGSEGV and
// resumed with siglongjmp, so one run counts the reports of every form.

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
typedef void (V::*VPM) ();
struct H { int x; V m; V arr[2]; };
NI void ptr (V *vp, VPM p) { (vp->*p) (); }
NI void mem (H *hp, VPM p) { (hp->m.*p) (); }
NI void call (H *hp) { hp->m.h (); }
NI int rd (H *hp) { return hp->m.v; }
// GCC's.
struct F final { virtual void h () {} int f; };
struct P { int p; };
struct W2 { virtual void w2 () {} void nw () {} int r; };
struct MW : P, W2 { int z; };
struct B { int y; V m; };
struct VD : virtual B { int z; };
struct S { int a; int b; };
struct HS { int x; S s; };
NI void ptrnv (V *vp, VPM p) { (vp->*p) (); }
NI void memptr (H *hp, V H::*pm, VPM p) { ((hp->*pm).*p) (); }
NI void submem (H *hp, VPM p) { (hp[1].m.*p) (); }
NI void vbasemem (VD *vp, VPM p) { (vp->m.*p) (); }
NI void fin (F *fp) { fp->h (); }
NI void decay (H *hp) { hp->arr->h (); }
NI void memarr (H *hp) { hp->arr[1].h (); }
NI void base (MW *mp) { mp->w2 (); }
NI int pmd (S *sp, int S::*pm) { return sp->*pm; }
NI int pmdmem (HS *hp, int S::*pm) { return hp->s.*pm; }
// Clang's.
struct X { long pad; };
struct D : X, B {};
NI void memarrp (H *hp, VPM p) { (hp->arr[1].*p) (); }
NI void dmem (H *hp, VPM p) { ((*hp).m.*p) (); }
NI void basemem (D *dp, VPM p) { (dp->m.*p) (); }
NI void dbasemem (D *dp, VPM p) { ((*dp).m.*p) (); }
// M88's: a pointer to member function of a base class, the object
// converted to the base first: `MW''s `W2', its primary base, at offset
// zero, `MV''s at a nonzero offset, on an element at a constant subscript
// (GCC-864).
struct W1 { virtual void w1 () {} int q; };
struct MV : W1, W2 { int z; };
typedef void (W2::*WPM) ();
NI void mwpmf (MW *mp, WPM p) { (mp->*p) (); }
NI void mwdpmf (MW *mp, WPM p) { ((*mp).*p) (); }
NI void mvpmf (MV *mp, WPM p) { (mp->*p) (); }
NI void mwsub (MW *mp, WPM p) { (mp[1].*p) (); }

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
  F *fp = reinterpret_cast<F *> (nv);
  MW *mp = reinterpret_cast<MW *> (nv);
  VD *vdp = reinterpret_cast<VD *> (nv);
  S *sp = reinterpret_cast<S *> (nv);
  HS *hsp = reinterpret_cast<HS *> (nv);
  D *dp = reinterpret_cast<D *> (nv);
  MV *mvp = reinterpret_cast<MV *> (nv);
  COUNT ("ptrv", ptr (vp, &V::h));
  COUNT ("memv", mem (hp, &V::h));
  COUNT ("memnv", mem (hp, &V::nv));
  COUNT ("call", call (hp));
  COUNT ("rd", rd (hp));
  COUNT ("ptrnv", ptrnv (vp, &V::nv));
  COUNT ("memptrv", memptr (hp, &H::m, &V::h));
  COUNT ("memptrnv", memptr (hp, &H::m, &V::nv));
  COUNT ("submemv", submem (hp, &V::h));
  COUNT ("submemnv", submem (hp, &V::nv));
  COUNT ("vbasememv", vbasemem (vdp, &V::h));
  COUNT ("vbasememnv", vbasemem (vdp, &V::nv));
  COUNT ("fin", fin (fp));
  COUNT ("decay", decay (hp));
  COUNT ("memarr", memarr (hp));
  COUNT ("base", base (mp));
  COUNT ("pmd", pmd (sp, &S::b));
  COUNT ("pmd0", pmd (sp, &S::a));
  COUNT ("pmdmem", pmdmem (hsp, &S::b));
  COUNT ("memarrpv", memarrp (hp, &V::h));
  COUNT ("memarrpnv", memarrp (hp, &V::nv));
  COUNT ("dmemv", dmem (hp, &V::h));
  COUNT ("dmemnv", dmem (hp, &V::nv));
  COUNT ("basememv", basemem (dp, &V::h));
  COUNT ("basememnv", basemem (dp, &V::nv));
  COUNT ("dbasememv", dbasemem (dp, &V::h));
  COUNT ("dbasememnv", dbasemem (dp, &V::nv));
  COUNT ("mwpmfv", mwpmf (mp, &W2::w2));
  COUNT ("mwpmfnv", mwpmf (mp, &W2::nw));
  COUNT ("mwdpmfv", mwdpmf (mp, &W2::w2));
  COUNT ("mwdpmfnv", mwdpmf (mp, &W2::nw));
  COUNT ("mvpmfv", mvpmf (mvp, &W2::w2));
  COUNT ("mvpmfnv", mvpmf (mvp, &W2::nw));
  COUNT ("mwsubv", mwsub (mp, &W2::w2));
  COUNT ("mwsubnv", mwsub (mp, &W2::nw));
  return 0;
}
