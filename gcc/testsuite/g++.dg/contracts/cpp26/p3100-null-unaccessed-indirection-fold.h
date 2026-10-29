// A constexpr call that is not manifestly constant-evaluated, and that GCC
// would fold when optimizing (an `if' condition), with an indirection
// through a null pointer whose result is not accessed, for the
// p3100-null-unaccessed-indirection-fold*.C tests: each run prints the
// number of implicit-assertion reports of one form.  The forms of Clang's
// Runnable/p3100-null-unaccessed-indirection-fold.cpp, in its order.
// `reports' is volatile: the constexpr calls are inlined into main, and
// the count must not depend on how the check's call is modelled (GCC-805,
// p3100-handler-effects-inlined.C).

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static volatile int reports;
void handle_contract_violation (const cs::contract_violation &v)
{
  if (v.kind () == cs::assertion_kind::implicit)
    reports = reports + 1;
}

struct S
{
  static int k;
  static constexpr int sf () { return 3; }
  static constexpr int ck = 6;
  enum { E = 5 };
  int m;
  int arr[2];
};
int S::k = 4;
using F = int ();
constexpr int seven () { return 7; }

constexpr int sfc (S *sp) { return sp->sf (); }
constexpr int sfc_star (S *sp) { return (*sp).sf (); }
constexpr int sck (S *sp) { return sp->ck; }
constexpr int se (S *sp) { return sp->E; }
constexpr bool skaddr (S *sp) { return &sp->k != nullptr; }
constexpr bool addr (int *p) { return &*p == nullptr; }
constexpr bool addr0 (int *p) { return &p[0] == nullptr; }
constexpr int disc (int *p) { *p; return 1; }
constexpr bool faddr (F *fp) { return &*fp == nullptr; }
constexpr int fdisc (F *fp) { *fp; return 1; }
constexpr bool maddr (S *sp) { return &sp->m != nullptr; }
constexpr bool aaddr (S *sp) { return &sp->arr[1] != nullptr; }
constexpr bool paddr (int (*pp)[2]) { return &pp[0][1] != nullptr; }
constexpr int obind (int *p) { int &r = *p; (void) r; return 1; }
constexpr int fbind (F *fp) { F &r = *fp; (void) r; return 1; }

constexpr S gs {};
constexpr int gi[2][2] {};
static_assert (sfc (const_cast<S *> (&gs)) == 3);
static_assert (sfc_star (const_cast<S *> (&gs)) == 3);
static_assert (sck (const_cast<S *> (&gs)) == 6);
static_assert (se (const_cast<S *> (&gs)) == 5);
static_assert (!addr (const_cast<int *> (&gi[0][0])));
static_assert (maddr (const_cast<S *> (&gs)));
static_assert (paddr (const_cast<int (*)[2]> (gi)));
static_assert (fdisc (seven) == 1 && !faddr (seven) && fbind (seven) == 1);
constexpr int cv = sfc (const_cast<S *> (&gs)) + sck (const_cast<S *> (&gs));

static volatile bool sink;
#define COUNT(name, cond)					\
  do {								\
    int before = reports;					\
    if (cond)							\
      sink = true;						\
    std::printf (name " %d\n", reports - before);		\
  } while (0)

int
main ()
{
  COUNT ("sfc", sfc (nullptr));
  COUNT ("sfc_star", sfc_star (nullptr));
  COUNT ("sck", sck (nullptr));
  COUNT ("se", se (nullptr));
  COUNT ("skaddr", skaddr (nullptr));
  COUNT ("addr", addr (nullptr));
  COUNT ("addr0", addr0 (nullptr));
  COUNT ("disc", disc (nullptr));
  COUNT ("faddr", faddr (nullptr));
  COUNT ("fdisc", fdisc (nullptr));
  COUNT ("maddr", maddr (nullptr));
  COUNT ("aaddr", aaddr (nullptr));
  COUNT ("paddr", paddr (nullptr));
  COUNT ("obind", obind (nullptr));
  COUNT ("fbind", fbind (nullptr));
  std::printf ("bcp %d\n", __builtin_constant_p (sfc (nullptr)));
  int before = reports;
  if (cv != 9 || sfc (const_cast<S *> (&gs)) != 3
      || !maddr (const_cast<S *> (&gs)))
    return 1;
  std::printf ("nonnull %d\n", reports - before);
  return 0;
}
