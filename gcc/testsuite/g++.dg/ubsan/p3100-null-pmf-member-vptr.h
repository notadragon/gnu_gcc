// Calls through a pointer to member function of a polymorphic class on a
// member subobject of the object a null pointer points to, with
// -fsanitize=vptr (and -fsanitize=null) routed to the contract-violation
// handler as well as ub:expr.unary.dereference.nullptr configured, for the
// p3100-null-pmf-member-vptr*.C tests: each run prints the number of
// reports of one form.  A fault after the report is caught as SIGSEGV and
// resumed with siglongjmp.  The forms: on `*vp' itself, the control; on a
// member, an element of a member array, the object of `->*', a member of
// `*hp', a member of a base at a nonzero offset, a member of an element at
// a constant subscript; each with a virtual and a non-virtual pointer to
// member function.

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
struct X { long pad; };
struct B { int y; V m; };
struct D : X, B {};
NI void ptr (V *vp, VPM p) { (vp->*p) (); }
NI void mem (H *hp, VPM p) { (hp->m.*p) (); }
NI void memarr (H *hp, VPM p) { (hp->arr[1].*p) (); }
NI void memptr (H *hp, V H::*pm, VPM p) { ((hp->*pm).*p) (); }
NI void dmem (H *hp, VPM p) { ((*hp).m.*p) (); }
NI void basemem (D *dp, VPM p) { (dp->m.*p) (); }
NI void submem (H *hp, VPM p) { (hp[1].m.*p) (); }

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

int
main ()
{
  std::signal (SIGSEGV, on_segv);
  int *volatile nv = nullptr;
  V *vp = reinterpret_cast<V *> (nv);
  H *hp = reinterpret_cast<H *> (nv);
  D *dp = reinterpret_cast<D *> (nv);
  COUNT ("ptrv", ptr (vp, &V::h));
  COUNT ("ptrnv", ptr (vp, &V::nv));
  COUNT ("memv", mem (hp, &V::h));
  COUNT ("memnv", mem (hp, &V::nv));
  COUNT ("memarrv", memarr (hp, &V::h));
  COUNT ("memarrnv", memarr (hp, &V::nv));
  COUNT ("memptrv", memptr (hp, &H::m, &V::h));
  COUNT ("memptrnv", memptr (hp, &H::m, &V::nv));
  COUNT ("dmemv", dmem (hp, &V::h));
  COUNT ("dmemnv", dmem (hp, &V::nv));
  COUNT ("basememv", basemem (dp, &V::h));
  COUNT ("basememnv", basemem (dp, &V::nv));
  COUNT ("submemv", submem (hp, &V::h));
  COUNT ("submemnv", submem (hp, &V::nv));
  return 0;
}
