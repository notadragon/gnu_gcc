// A member function call through a null pointer returning a class with a
// non-trivial copy constructor and destructor by value, each paired with
// the same call returning int, for the
// p3100-null-member-call-class-return*.C tests: each run prints the number
// of reports of one form.  A fault after the report is caught as SIGSEGV
// and resumed with siglongjmp.  The forms: a non-virtual function, a
// virtual one, and one devirtualized through a `final' class; then, the
// class returned, a call on an element at a constant subscript, on a
// member, on a member of a `final' class (devirtualized), a qualified
// virtual call, a call on `*p' and through a reference bound to `*p'.

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
struct R
{
  R () {}
  R (const R &) {}
  ~R () {}
  int r;
};
struct S
{
  R f () { return R (); }
  int g () { return 0; }
};
struct V
{
  virtual R f () { return R (); }
  virtual int g () { return 0; }
};
struct F final
{
  virtual R f () { return R (); }
  virtual int g () { return 0; }
};
NI void scls (S *p) { p->f (); }
NI void sint (S *p) { p->g (); }
NI void vcls (V *p) { p->f (); }
NI void vint (V *p) { p->g (); }
NI void fcls (F *p) { p->f (); }
NI void fint (F *p) { p->g (); }
struct H { int x; S m; F fm; };
NI void subcls (S *p) { p[1].f (); }
NI void memcls (H *p) { p->m.f (); }
NI void fmemcls (H *p) { p->fm.f (); }
NI void qcls (V *p) { p->V::f (); }
NI void starcls (S *p) { (*p).f (); }
NI void refcls (S &r) { r.f (); }
NI void refc (S *p) { refcls (*p); }

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
  COUNT ("scls", scls (reinterpret_cast<S *> (nv)));
  COUNT ("sint", sint (reinterpret_cast<S *> (nv)));
  COUNT ("vcls", vcls (reinterpret_cast<V *> (nv)));
  COUNT ("vint", vint (reinterpret_cast<V *> (nv)));
  COUNT ("fcls", fcls (reinterpret_cast<F *> (nv)));
  COUNT ("fint", fint (reinterpret_cast<F *> (nv)));
  COUNT ("subcls", subcls (reinterpret_cast<S *> (nv)));
  COUNT ("memcls", memcls (reinterpret_cast<H *> (nv)));
  COUNT ("fmemcls", fmemcls (reinterpret_cast<H *> (nv)));
  COUNT ("qcls", qcls (reinterpret_cast<V *> (nv)));
  COUNT ("starcls", starcls (reinterpret_cast<S *> (nv)));
  COUNT ("refc", refc (reinterpret_cast<S *> (nv)));
  S s;
  H h;
  COUNT ("nonnull", scls (&s); memcls (&h); fmemcls (&h); starcls (&s));
  return 0;
}
