// A reference bound to `*p', p a null pointer, where the binding has no
// location of its own -- an implicit conversion to the reference: a
// variable, a call's argument, a mem-initializer, a default member
// initializer, an init-capture, a structured binding, a range-based for --
// and to a misaligned `*p', for the p3100-null-binding-location*.C tests:
// each run prints the number of implicit-assertion reports of one form,
// then how many of them name this file and the form's line in location().
// The conversion to a virtual base reads its offset through the pointer:
// that fault, after the report, is caught as SIGSEGV and resumed with
// siglongjmp.

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
#include <cstring>
namespace cs = std::contracts;

static unsigned expected;
static int reports, good;
void handle_contract_violation (const cs::contract_violation &v)
{
  if (v.kind () != cs::assertion_kind::implicit)
    return;
  ++reports;
  const char *f = v.location ().file_name ();
  if (f && std::strstr (f, "p3100-null-binding-location.h")
      && v.location ().line () == expected)
    ++good;
}

static sigjmp_buf env;
static void on_segv (int) { siglongjmp (env, 1); }

#define NI __attribute__((noinline))
struct S { int m; };
struct F final { int m; };
struct B1 { int x; };
struct B2 { int y; virtual ~B2 () {} };
struct D2 : B1, B2 { int z; };
struct VB { int x; };
struct DV : virtual VB { int y; };
struct EB {};
struct DE2 { int a; };
struct DE : DE2, EB { int c; };
struct P2 { int a, b; };
struct C { int m; };
bool operator== (const C &, const C &) { return true; }
NI void take (int &) {}

NI bool var (int *p) { int &r = *p; return &r == nullptr; } enum { L_var = __LINE__ };
NI bool fin (F *fp) { F &r = *fp; return &r == nullptr; } enum { L_fin = __LINE__ };
NI bool mem (S *sp) { S &r = *sp; return &r.m == nullptr; } enum { L_mem = __LINE__ };
NI bool down (B2 *b2p) { D2 &d = static_cast<D2 &> (*b2p); return &d == nullptr; } enum { L_down = __LINE__ };
NI void arg (int *p) { take (*p); } enum { L_arg = __LINE__ };
NI bool cmp (C *p) { C c{}; return c == *p; } enum { L_cmp = __LINE__ };
struct M { int &r; M (int *p) : r (*p) {} }; enum { L_minit = __LINE__ };
NI bool minit (int *p) { M m (p); return &m.r == nullptr; }
struct N { int *p = nullptr; int &r = *p; }; enum { L_nsdmi = __LINE__ };
NI bool nsdmi () { N n; return &n.r == nullptr; }
NI bool cap (int *p) { auto l = [&r = *p] { return &r == nullptr; }; return l (); } enum { L_cap = __LINE__ };
NI bool sbind (P2 *pp) { auto &[a, b] = *pp; return &a == nullptr; } enum { L_sbind = __LINE__ };
NI int rfor (int (*ap)[2]) { int n = 0; for (int &x : *ap) n += &x == nullptr; return n; } enum { L_rfor = __LINE__ };
NI bool cref (S *sp) { const S &r = *sp; return &r == nullptr; } enum { L_cref = __LINE__ };
NI bool paren (int *p) { int &r = (*p); return &r == nullptr; } enum { L_paren = __LINE__ };
NI int &ret (int *p) { return *p; } enum { L_ret = __LINE__ };
NI bool vbase (DV *p) { VB &b = *p; return &b == nullptr; } enum { L_vbase = __LINE__ };
NI bool ebase (DE *p) { EB &e = *p; return &e == nullptr; } enum { L_ebase = __LINE__ };

#define RUN(name, line, ...)					\
  do								\
    {								\
      reports = good = 0;					\
      expected = line;						\
      if (!sigsetjmp (env, 1))					\
	{							\
	  __VA_ARGS__;						\
	}							\
      std::printf (name " %d %d\n", reports, good);		\
    }								\
  while (0)

alignas (8) char buf[16];

int main ()
{
  std::signal (SIGSEGV, on_segv);
  int *volatile nv = nullptr;
  int *p = nv;
  RUN ("var", L_var, var (p));
  RUN ("fin", L_fin, fin (reinterpret_cast<F *> (p)));
  RUN ("mem", L_mem, mem (reinterpret_cast<S *> (p)));
  RUN ("down", L_down, down (reinterpret_cast<B2 *> (p)));
  RUN ("arg", L_arg, arg (p));
  RUN ("cmp", L_cmp, cmp (reinterpret_cast<C *> (p)));
  RUN ("minit", L_minit, minit (p));
  RUN ("nsdmi", L_nsdmi, nsdmi ());
  RUN ("cap", L_cap, cap (p));
  RUN ("sbind", L_sbind, sbind (reinterpret_cast<P2 *> (p)));
  RUN ("rfor", L_rfor, rfor (reinterpret_cast<int (*)[2]> (p)));
  RUN ("cref", L_cref, cref (reinterpret_cast<S *> (p)));
  RUN ("paren", L_paren, paren (p));
  RUN ("ret", L_ret, (void) &ret (p));
  RUN ("vbase", L_vbase, vbase (reinterpret_cast<DV *> (p)));
  RUN ("ebase", L_ebase, ebase (reinterpret_cast<DE *> (p)));
  // Misaligned: the alignment check of the binding (.store).
  int *volatile mv = reinterpret_cast<int *> (buf + 1);
  RUN ("avar", L_var, var (mv));
  RUN ("aarg", L_arg, arg (mv));
  RUN ("aparen", L_paren, paren (mv));
  int x = 0;
  RUN ("nonnull", 0, var (&x); arg (&x); paren (&x); (void) &ret (&x));
  return 0;
}
