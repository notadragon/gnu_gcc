// Virtual and non-virtual calls through a null pointer to a class with a
// virtual base, of functions of the virtual base, for the
// p3100-null-virtual-call-vbase*.C tests: each run prints the number of
// reports of one form.  A fault after the report is caught as SIGSEGV and
// resumed with siglongjmp, so one run counts the reports of every form.
// The forms: a virtual call through `->', through `*', on an element at a
// constant subscript, a qualified call, a non-virtual call, and a virtual
// call on a member of a class with a virtual base.

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
struct VB : virtual V { int w; };
struct HV { int x; VB vb; };
NI void vbh (VB *p) { p->h (); }
NI void vbstar (VB *p) { (*p).h (); }
NI void vbsub (VB *p) { p[1].h (); }
NI void vbq (VB *p) { p->V::h (); }
NI void vbnv (VB *p) { p->nv (); }
NI void vbmem (HV *hp) { hp->vb.h (); }

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
  VB *vbp = reinterpret_cast<VB *> (nv);
  HV *hvp = reinterpret_cast<HV *> (nv);
  COUNT ("vbh", vbh (vbp));
  COUNT ("vbstar", vbstar (vbp));
  COUNT ("vbsub", vbsub (vbp));
  COUNT ("vbq", vbq (vbp));
  COUNT ("vbnv", vbnv (vbp));
  COUNT ("vbmem", vbmem (hvp));
  return 0;
}
