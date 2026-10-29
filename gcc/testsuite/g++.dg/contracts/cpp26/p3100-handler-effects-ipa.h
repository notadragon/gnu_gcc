// A call through a null pointer of a member function that does nothing,
// in a function that is not inlined and does nothing else, for the
// *p3100-handler-effects-ipa*.C tests: each run prints the number of
// reports of one form, `reports' read before and after the call of the
// function holding the check, which the contract-violation handler alone
// changes.  The forms: a non-virtual member function returning a constant,
// a non-virtual one of a polymorphic class, a call devirtualized through a
// `final' class, and, the control, a call through a pointer to a
// non-virtual member function.

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
NI void fin (F *fp) { fp->h (); }
NI void ptrnv (V *vp, VPM p) { (vp->*p) (); }

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
  COUNT ("fin", fin (reinterpret_cast<F *> (nv)));
  COUNT ("ptrnv", ptrnv (reinterpret_cast<V *> (nv), &V::nv));
  return 0;
}
