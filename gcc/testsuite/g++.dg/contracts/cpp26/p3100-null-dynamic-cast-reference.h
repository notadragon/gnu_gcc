// A dynamic_cast to a reference type of the object a null pointer points
// to, for the p3100-null-dynamic-cast-reference*.C tests: each run prints
// the number of implicit-assertion reports of one form.  A fault after the
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

struct V { virtual void h () {} int v; };
struct W : V {};
struct H { int x; V m; };
struct P { int p; };
struct X : P, V {};

#define NI __attribute__((noinline))
NI void star (V *vp)
{
  try { (void) dynamic_cast<W &> (*vp); } catch (...) {}
}
NI void cstar (V *vp)
{
  try { (void) dynamic_cast<const W &> (*vp); } catch (...) {}
}
NI void sub (V *vp)
{
  try { (void) dynamic_cast<W &> (vp[1]); } catch (...) {}
}
NI void member (H *hp)
{
  try { (void) dynamic_cast<W &> (hp->m); } catch (...) {}
}
NI void refparam (V &r)
{
  try { (void) dynamic_cast<W &> (r); } catch (...) {}
}
// Clang's.
NI void arith (V *vp)
{
  try { (void) dynamic_cast<W &> (*(vp + 1)); } catch (...) {}
}
NI void base (X *xp)
{
  try { (void) dynamic_cast<W &> (static_cast<V &> (*xp)); } catch (...) {}
}

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
  W w;
  V va[2];
  H h{};
  X x;
  COUNT ("star", star (vp));
  COUNT ("cstar", cstar (vp));
  COUNT ("sub", sub (vp));
  COUNT ("member", member (hp));
  COUNT ("refparam", refparam (*vp));
  COUNT ("nonnull", star (&w); cstar (&w); sub (va); member (&h);
	 refparam (w));
  COUNT ("arith", arith (vp));
  COUNT ("base", base (reinterpret_cast<X *> (nv)));
  COUNT ("nonnull_clang", arith (va); base (&x));
  return 0;
}
