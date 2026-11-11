// A member function of a base class called through a pointer to member
// function of the base on the object a null pointer points to, the object
// converted to the base first, for the p3100-null-pmf-base*.C tests: each
// run prints the number of implicit-assertion reports of one form, the
// pointer to member function virtual (`v') or not.  A fault after the
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
// W2, polymorphic, is MW's primary base, at offset zero.
struct MW : P, W2 { int z; };
struct W1 { virtual void w1 () {} int q; };
// W2 is at a nonzero offset, after the polymorphic W1.
struct MV : W1, W2 { int z; };
struct DW : W2 { int z; };
struct HW { int x; MW mw; };
typedef void (W2::*WPM) ();
NI void mwpmf (MW *mp, WPM p) { (mp->*p) (); }
NI void mwdpmf (MW *mp, WPM p) { ((*mp).*p) (); }
NI void mvpmf (MV *mp, WPM p) { (mp->*p) (); }
NI void mvdpmf (MV *mp, WPM p) { ((*mp).*p) (); }
NI void dwpmf (DW *dp, WPM p) { (dp->*p) (); }
NI void mwsub (MW *mp, WPM p) { (mp[1].*p) (); }
NI void mvsub (MV *mp, WPM p) { (mp[1].*p) (); }
NI void mwarith (MW *mp, WPM p) { ((mp + 1)->*p) (); }
NI void mwdarith (MW *mp, WPM p) { ((*(mp + 1)).*p) (); }
NI void hwmem (HW *hp, WPM p) { (hp->mw.*p) (); }
NI void hwmemptr (HW *hp, MW HW::*pm, WPM p) { ((hp->*pm).*p) (); }

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
  DW *dp = reinterpret_cast<DW *> (nv);
  HW *hp = reinterpret_cast<HW *> (nv);
  MW mw[2]{};
  MV mv[2]{};
  HW hw{};
  WPM v = &W2::w2, n = &W2::nw;
  COUNT ("mwpmf", mwpmf (mp, n));
  COUNT ("mwpmfv", mwpmf (mp, v));
  COUNT ("mwdpmf", mwdpmf (mp, n));
  COUNT ("mwdpmfv", mwdpmf (mp, v));
  COUNT ("mvpmf", mvpmf (mvp, n));
  COUNT ("mvpmfv", mvpmf (mvp, v));
  COUNT ("mvdpmf", mvdpmf (mvp, n));
  COUNT ("mvdpmfv", mvdpmf (mvp, v));
  COUNT ("dwpmf", dwpmf (dp, n));
  COUNT ("dwpmfv", dwpmf (dp, v));
  COUNT ("mwsub", mwsub (mp, n));
  COUNT ("mwsubv", mwsub (mp, v));
  COUNT ("mvsub", mvsub (mvp, n));
  COUNT ("mvsubv", mvsub (mvp, v));
  COUNT ("mwarith", mwarith (mp, n));
  COUNT ("mwarithv", mwarith (mp, v));
  COUNT ("mwdarith", mwdarith (mp, n));
  COUNT ("mwdarithv", mwdarith (mp, v));
  COUNT ("hwmem", hwmem (hp, n));
  COUNT ("hwmemv", hwmem (hp, v));
  COUNT ("hwmemptr", hwmemptr (hp, &HW::mw, n));
  COUNT ("hwmemptrv", hwmemptr (hp, &HW::mw, v));
  COUNT ("nonnull", mwpmf (mw, v); mwdpmf (mw, n); mvpmf (mv, v);
	 mvdpmf (mv, n); mwsub (mw, v); mvsub (mv, n); hwmem (&hw, v));
  return 0;
}
