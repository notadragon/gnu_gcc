// `(a + i)->*pm' through a conversion to the base class whose member the
// pointer to member is, a a null pointer, for the
// p3100-null-ptrmem-base-arith*.C tests: each run prints the number of
// implicit-assertion reports of one form.  A pointer to data member, read
// or its address taken, of a polymorphic base at offset zero (`MW''s
// primary base `W2') and at a nonzero offset (`MV''s, after the
// polymorphic `W1'), and of a base that is not polymorphic at a nonzero
// offset; a call through a pointer to member function of a base at a
// nonzero offset, virtual or not.  The controls: the same through a itself
// and without a conversion, and non-null pointers.  A fault after the
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
struct P { int p; };
struct W2 { virtual void w2 () {} void nw () {} int r; };
struct MW : P, W2 { int z; };
struct W1 { virtual void w1 () {} int q; };
struct MV : W1, W2 { int z; };
struct S { int a; int b; };
struct Y { long y; };
struct DS : Y, S { int t; };
typedef int W2::*WPD;
typedef void (W2::*WPM) ();
NI int mwrd (MW *mp, WPD m) { return (mp + 1)->*m; }
NI int mvrd (MV *mp, WPD m) { return (mp + 1)->*m; }
NI int dsrd (DS *dp, int S::*m) { return (dp + 1)->*m; }
NI int *dsaddr (DS *dp, int S::*m) { return &((dp + 1)->*m); }
NI int *mvaddr (MV *mp, WPD m) { return &((mp + 1)->*m); }
NI void mvpmf (MV *mp, WPM p) { ((mp + 1)->*p) (); }
// The controls.
NI int mwrd0 (MW *mp, WPD m) { return mp->*m; }
NI int mvrd0 (MV *mp, WPD m) { return mp->*m; }
NI int srd (S *sp, int S::*m) { return (sp + 1)->*m; }

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
  MW *mp = reinterpret_cast<MW *> (nv);
  MV *mvp = reinterpret_cast<MV *> (nv);
  DS *dp = reinterpret_cast<DS *> (nv);
  S *sp = reinterpret_cast<S *> (nv);
  MV mv[2]{};
  DS ds[2]{};
  COUNT ("mwrd", mwrd (mp, &W2::r));
  COUNT ("mvrd", mvrd (mvp, &W2::r));
  COUNT ("dsrd", dsrd (dp, &S::b));
  COUNT ("dsaddr", dsaddr (dp, &S::b));
  COUNT ("mvaddr", mvaddr (mvp, &W2::r));
  COUNT ("mvpmfv", mvpmf (mvp, &W2::w2));
  COUNT ("mvpmfnv", mvpmf (mvp, &W2::nw));
  COUNT ("mwrd0", mwrd0 (mp, &W2::r));
  COUNT ("mvrd0", mvrd0 (mvp, &W2::r));
  COUNT ("srd", srd (sp, &S::b));
  COUNT ("nonnull", mvrd (mv, &W2::r); dsrd (ds, &S::b); dsaddr (ds, &S::b);
	 mvpmf (mv, &W2::w2); mvpmf (mv, &W2::nw));
  return 0;
}
