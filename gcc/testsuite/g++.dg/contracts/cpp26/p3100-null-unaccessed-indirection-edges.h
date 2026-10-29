// More indirections through a null pointer whose result is not accessed,
// for the p3100-null-unaccessed-indirection-edges*.C tests: each run prints
// the number of implicit-assertion reports of one form.  A fault after the
// report is caught as SIGSEGV and resumed with siglongjmp, so one run
// counts the reports of every form.  The forms of Clang's
// Runnable/p3100-null-unaccessed-indirection-edges.cpp, in its order, then
// GCC's extra ones.

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
#include <memory>
#include <typeinfo>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation (const cs::contract_violation &v)
{
  if (v.kind () == cs::assertion_kind::implicit)
    ++reports;
}

static sigjmp_buf env;
static void on_segv (int) { siglongjmp (env, 1); }

struct S
{
  int m;
  enum { E = 5 };
  static constexpr int ck = 6;
  static int sf () { return 3; }
  int g ();
};
struct A { int a; };
struct D : A, S {};
struct V : virtual S {};
struct P { virtual ~P () {} };
using I = int;

#define NI __attribute__((noipa))
NI int *addr2 (int *p) { return &p[2]; }
NI void disc_sub (int *p) { p[0]; }
NI void disc_cond (int *p, int *q, bool b) { b ? *p : *q; }
NI void disc_comma (int *p) { (0, *p); }
NI void disc_comma_lhs (int *p) { (*p, 0); }
NI int *addr_cond (int *p, int *q, bool b) { return &(b ? *p : *q); }
NI int *addr_comma (int *p) { return &(0, *p); }
NI S *addr_cast (S *sp) { return &static_cast<S &> (*sp); }
NI const S *addr_ccast (S *sp) { return &static_cast<const S &> (*sp); }
NI int *addr_rcast (long *lp) { return &reinterpret_cast<int &> (*lp); }
NI A *addr_base (D *dp) { return &static_cast<A &> (*dp); }
NI S *addr_base_off (D *dp) { return &static_cast<S &> (*dp); }
NI void disc_cast (S *sp) { static_cast<S &> (*sp); }
NI void disc_base_off (D *dp) { static_cast<S &> (*dp); }
NI int *std_addressof (int *p) { return std::addressof (*p); }
NI int *builtin_addressof (int *p) { return __builtin_addressof (*p); }
NI int se (S *sp) { return sp->E; }
NI int sck (S *sp) { return sp->ck; }
NI void pdtor (int *p) { p->~I (); }
NI int amem (S *sp) { return (&*sp)->m; }
NI S *addr_vbase (V *vp) { return &static_cast<S &> (*vp); }
// GCC's extra forms.
NI void disc_base (D *dp) { static_cast<A &> (*dp); }
NI void disc_ccast (S *sp) { static_cast<const S &> (*sp); }
NI void disc_rcast (long *lp) { reinterpret_cast<int &> (*lp); }
NI void disc_cbase (D *dp) { (S &) *dp; }
NI void disc_void_cast (S *sp) { (void) static_cast<S &> (*sp); }
NI void disc_vbase (V *vp) { static_cast<S &> (*vp); }
NI int se_star (S *sp) { return (*sp).E; }
NI int sck_cast (S *sp) { return static_cast<S &> (*sp).ck; }
NI int sf_cast (D *dp) { return static_cast<S &> (*dp).sf (); }
NI int se_cond (S *sp, S *sq, bool b) { return (b ? *sp : *sq).E; }
NI void pdtor_star (int *p) { (*p).~I (); }
NI void pdtor_cast (long *lp) { reinterpret_cast<int &> (*lp).~I (); }
NI void sdtor (S *sp) { sp->~S (); }
NI void sdtor_star (S *sp) { (*sp).~S (); }
NI S *builtin_addressof_cast (S *sp)
{
  return __builtin_addressof (static_cast<S &> (*sp));
}
NI int *std_addressof_cond (int *p, int *q, bool b)
{
  return std::addressof (b ? *p : *q);
}
// Controls.
int S::g ()
{
  (void) *this;
  S *q = &*this;
  static_cast<S &> (*this);
  return this->ck + (*this).E + (q == this);
}
int gx;
int garr[2];
NI int *addr_obj () { return &*&gx; }
NI int *addr_arr () { return &*garr; }
NI int *addr_ref (int &r) { return &r; }
NI void disc_ref (int &r) { r; }
NI void disc_ref_cast (int &r) { static_cast<int &> (r); }
NI bool tid (P *pp)
{
  try
    {
      return typeid (*pp) == typeid (P);
    }
  catch (std::bad_typeid &)
    {
      return false;
    }
}
NI void del (S *sp) { delete sp; }
NI int unevaluated (int *p) { return sizeof (*p) + sizeof (decltype (&*p)); }

#define COUNT(name, ...)					\
  do {								\
    int before = reports;					\
    if (!sigsetjmp (env, 1))					\
      {								\
	__VA_ARGS__;						\
      }								\
    std::printf (name " %d\n", reports - before);		\
  } while (0)

int
main ()
{
  std::signal (SIGSEGV, on_segv);
  int *volatile vp = nullptr;
  int *p = vp;
  S *sp = reinterpret_cast<S *> (p);
  D *dp = reinterpret_cast<D *> (p);
  V *vbp = reinterpret_cast<V *> (p);
  P *pp = reinterpret_cast<P *> (p);
  long *lp = reinterpret_cast<long *> (p);
  int x = 0;
  S s {};
  D d {};
  V v;
  COUNT ("addr2", addr2 (p));
  COUNT ("disc_sub", disc_sub (p));
  COUNT ("disc_cond", disc_cond (&x, p, false));
  COUNT ("disc_comma", disc_comma (p));
  COUNT ("disc_comma_lhs", disc_comma_lhs (p));
  COUNT ("addr_cond", addr_cond (&x, p, false));
  COUNT ("addr_comma", addr_comma (p));
  COUNT ("addr_cast", addr_cast (sp));
  COUNT ("addr_ccast", addr_ccast (sp));
  COUNT ("addr_rcast", addr_rcast (lp));
  COUNT ("addr_base", addr_base (dp));
  COUNT ("addr_base_off", addr_base_off (dp));
  COUNT ("disc_cast", disc_cast (sp));
  COUNT ("disc_base_off", disc_base_off (dp));
  COUNT ("std_addressof", std_addressof (p));
  COUNT ("builtin_addressof", builtin_addressof (p));
  COUNT ("se", se (sp));
  COUNT ("sck", sck (sp));
  COUNT ("pdtor", pdtor (p));
  COUNT ("amem", amem (sp));
  COUNT ("addr_vbase", addr_vbase (vbp));
  COUNT ("disc_base", disc_base (dp));
  COUNT ("disc_ccast", disc_ccast (sp));
  COUNT ("disc_rcast", disc_rcast (lp));
  COUNT ("disc_cbase", disc_cbase (dp));
  COUNT ("disc_void_cast", disc_void_cast (sp));
  COUNT ("disc_vbase", disc_vbase (vbp));
  COUNT ("se_star", se_star (sp));
  COUNT ("sck_cast", sck_cast (sp));
  COUNT ("sf_cast", sf_cast (dp));
  COUNT ("se_cond", se_cond (&s, sp, false));
  COUNT ("pdtor_star", pdtor_star (p));
  COUNT ("pdtor_cast", pdtor_cast (lp));
  COUNT ("sdtor", sdtor (sp));
  COUNT ("sdtor_star", sdtor_star (sp));
  COUNT ("builtin_addressof_cast", builtin_addressof_cast (sp));
  COUNT ("std_addressof_cond", std_addressof_cond (&x, p, false));
  COUNT ("nonnull", addr2 (&x); disc_cond (&x, p, true);
	 addr_cond (&x, p, true); addr_base_off (&d); disc_base_off (&d);
	 se (&s); pdtor (&x); amem (&s); disc_vbase (&v); disc_cast (&s);
	 sck_cast (&s); std_addressof_cond (&x, p, true));
  COUNT ("controls", s.g (); addr_obj (); addr_arr (); addr_ref (x);
	 disc_ref (x); disc_ref_cast (x); tid (pp); del (sp);
	 unevaluated (p));
  return 0;
}
