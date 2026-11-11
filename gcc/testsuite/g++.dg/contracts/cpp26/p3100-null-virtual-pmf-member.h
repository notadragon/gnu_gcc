// A call through a pointer to member function, virtual or not, on a
// member subobject of the object a null pointer points to, for the
// p3100-null-virtual-pmf-member*.C tests: each run prints the number of
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

#define NI __attribute__((noinline))
struct V
{
  virtual void h () {}
  void nv () {}
  int v;
};
typedef void (V::*VPM) ();
struct H
{
  int x;
  V m;
  V arr[2];
  NI void self (VPM p) { (m.*p) (); }
};
NI void mem (H *hp, VPM p) { (hp->m.*p) (); }
NI void memarr (H *hp, VPM p) { (hp->arr[1].*p) (); }
NI void memptr (H *hp, V H::*pm, VPM p) { ((hp->*pm).*p) (); }
NI void dmem (H *hp, VPM p) { ((*hp).m.*p) (); }
// GCC's: through `this' (DECISIONS.md E106 (a)) and a reference, on an
// element at a constant subscript, and through a pointer to data member
// of a member.
NI void selfm (H *hp, VPM p) { hp->self (p); }
NI void refbody (H &r, VPM p) { (r.m.*p) (); }
NI void refm (H *hp, VPM p) { refbody (*hp, p); }
NI void submem (H *hp, VPM p) { (hp[1].m.*p) (); }
NI void ptrmemarr (H *hp, V (H::*pa)[2], VPM p) { ((hp->*pa)[1].*p) (); }
// Clang's: a member of a base class at a nonzero offset, of a virtual
// base.
struct X { long pad; };
struct B { int y; V m; };
struct D : X, B {};
struct VD : virtual B { int z; };
NI void basemem (D *dp, VPM p) { (dp->m.*p) (); }
NI void dbasemem (D *dp, VPM p) { ((*dp).m.*p) (); }
NI void vbasemem (VD *vp, VPM p) { (vp->m.*p) (); }

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
  H *hp = reinterpret_cast<H *> (nv);
  D *dp = reinterpret_cast<D *> (nv);
  VD *vp = reinterpret_cast<VD *> (nv);
  H h;
  H ha[2];
  D d;
  VD vd;
  COUNT ("memv", mem (hp, &V::h));
  COUNT ("memnv", mem (hp, &V::nv));
  COUNT ("memarrv", memarr (hp, &V::h));
  COUNT ("memarrnv", memarr (hp, &V::nv));
  COUNT ("memptrv", memptr (hp, &H::m, &V::h));
  COUNT ("memptrnv", memptr (hp, &H::m, &V::nv));
  COUNT ("dmemv", dmem (hp, &V::h));
  COUNT ("dmemnv", dmem (hp, &V::nv));
  COUNT ("nonnull", mem (&h, &V::h); mem (&h, &V::nv); memarr (&h, &V::h);
	 memarr (&h, &V::nv); memptr (&h, &H::m, &V::h);
	 memptr (&h, &H::m, &V::nv); dmem (&h, &V::h); dmem (&h, &V::nv));
  COUNT ("selfv", selfm (hp, &V::h));
  COUNT ("selfnv", selfm (hp, &V::nv));
  COUNT ("refv", refm (hp, &V::h));
  COUNT ("refnv", refm (hp, &V::nv));
  COUNT ("submemv", submem (hp, &V::h));
  COUNT ("submemnv", submem (hp, &V::nv));
  COUNT ("ptrmemarrv", ptrmemarr (hp, &H::arr, &V::h));
  COUNT ("ptrmemarrnv", ptrmemarr (hp, &H::arr, &V::nv));
  COUNT ("nonnull_gcc", selfm (ha, &V::h); selfm (ha, &V::nv);
	 refm (ha, &V::h); refm (ha, &V::nv); submem (ha, &V::h);
	 submem (ha, &V::nv); ptrmemarr (ha, &H::arr, &V::h);
	 ptrmemarr (ha, &H::arr, &V::nv));
  COUNT ("basememv", basemem (dp, &V::h));
  COUNT ("basememnv", basemem (dp, &V::nv));
  COUNT ("dbasememv", dbasemem (dp, &V::h));
  COUNT ("dbasememnv", dbasemem (dp, &V::nv));
  COUNT ("vbasememv", vbasemem (vp, &V::h));
  COUNT ("vbasememnv", vbasemem (vp, &V::nv));
  COUNT ("nonnull_clang", basemem (&d, &V::h); basemem (&d, &V::nv);
	 dbasemem (&d, &V::h); dbasemem (&d, &V::nv); vbasemem (&vd, &V::h);
	 vbasemem (&vd, &V::nv));
  return 0;
}
