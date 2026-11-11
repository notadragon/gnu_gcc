// A call to a member function that is not inlined, through a null
// pointer, for the p3100-null-member-call-prologue*.C tests: each run
// prints the number of reports of one form.  The forms: a non-virtual
// member function of a class that is not polymorphic, and of one that is,
// a call through a pointer to a non-virtual member function, and a call
// devirtualized through a `final' class.

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation (const cs::contract_violation &) { ++reports; }

#define NI __attribute__((noinline))
struct S { NI int g () { return 0; } };
struct V
{
  virtual void h () {}
  NI void nv () {}
};
typedef void (V::*VPM) ();
struct F final { virtual void h () {} };
NI void plain (S *sp) { sp->g (); }
NI void poly (V *vp) { vp->nv (); }
NI void ptrnv (V *vp, VPM p) { (vp->*p) (); }
NI void fin (F *fp) { fp->h (); }

#define COUNT(name, ...)					\
  do								\
    {								\
      int before = reports;					\
      __VA_ARGS__;						\
      std::printf (name " %d\n", reports - before);		\
    }								\
  while (0)

int
main ()
{
  int *volatile nv = nullptr;
  COUNT ("plain", plain (reinterpret_cast<S *> (nv)));
  COUNT ("poly", poly (reinterpret_cast<V *> (nv)));
  COUNT ("ptrnv", ptrnv (reinterpret_cast<V *> (nv), &V::nv));
  COUNT ("fin", fin (reinterpret_cast<F *> (nv)));
  return 0;
}
