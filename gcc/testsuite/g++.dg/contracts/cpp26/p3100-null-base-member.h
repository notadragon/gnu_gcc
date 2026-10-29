// A member of a base class used through a null pointer, in every form, for
// the p3100-null-base-member*.C tests: each run prints the number of
// implicit-assertion reports of one form.  A null dereference that is read
// faults after the handler returns; the fault is caught as SIGSEGV and
// resumed with siglongjmp, so one run counts the reports of every form.

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

struct B0 { int z; };
struct B { int b; __attribute__((noinline)) int f () { return 7; } };
struct D : B0, B { int x; };
struct D1 : B { int x; };
struct VB : virtual B { int x; };
struct H { D1 d; D dd; VB v; };

#define NI __attribute__((noinline))
// A virtual base: every use reads the vptr, so every form is .access.
NI int v_rd (VB *p) { return p->b; }
NI void v_wr (VB *p) { p->b = 1; }
NI int *v_addr (VB *p) { return &p->b; }
NI int &v_ref (VB *p) { return p->b; }
NI int v_call (VB *p) { return p->f (); }
NI int v_rd_star (VB *p) { return (*p).b; }
NI int v_call_star (VB *p) { return (*p).f (); }
NI B &v_bind (VB *p) { return *p; }
NI int v_cast (VB *p) { return static_cast<B &> (*p).b; }
NI int v_member (H *p) { return p->v.b; }
NI int v_sub (VB *q, int i) { return q[i].b; }
// A base at a nonzero offset: the leaf of the use.
NI int n_rd (D *p) { return p->b; }
NI int *n_addr (D *p) { return &p->b; }
NI int &n_ref (D *p) { return p->b; }
NI int n_call (D *p) { return p->f (); }
NI int n_rd_star (D *p) { return (*p).b; }
NI int n_call_star (D *p) { return (*p).f (); }
NI B &n_bind (D *p) { return *p; }
NI int n_loaded (D **pp) { return (*pp)->b; }
NI int n_sub (D *q, int i) { return q[i].b; }
// A member subobject whose base's member is used.
NI int m_rd (H *p) { return p->d.b; }
NI int m_call (H *p) { return p->d.f (); }
NI int &m_ref (H *p) { return p->d.b; }
NI int m_rd_off (H *p) { return p->dd.b; }
// Pointers to members of a base.
NI int n_pmf_arrow (D *p, int (B::*pmf) ()) { return (p->*pmf) (); }
NI int n_pmf_dot (D *p, int (B::*pmf) ()) { return ((*p).*pmf) (); }
NI int n_pm_dot (D *p, int B::*pm) { return (*p).*pm; }
NI int v_pmf_arrow (VB *p, int (B::*pmf) ()) { return (p->*pmf) (); }
NI int v_pmf_dot (VB *p, int (B::*pmf) ()) { return ((*p).*pmf) (); }
NI int v_pm_dot (VB *p, int B::*pm) { return (*p).*pm; }
NI int *v_pm_addr (VB *p, int B::*pm) { return &(p->*pm); }
// The control: a base at offset 0.
NI int rd_first (D1 *p) { return p->b; }

#define RUN(name, ...)						\
  do								\
    {								\
      reports = 0;						\
      if (!sigsetjmp (env, 1))					\
	(void) (__VA_ARGS__);					\
      std::printf ("%s %d\n", #name, reports);			\
    }								\
  while (0)

int main ()
{
  std::signal (SIGSEGV, on_segv);
  D *n = nullptr;
  RUN (v_rd, v_rd (nullptr));
  RUN (v_wr, v_wr (nullptr));
  RUN (v_addr, v_addr (nullptr));
  RUN (v_ref, &v_ref (nullptr));
  RUN (v_call, v_call (nullptr));
  RUN (v_rd_star, v_rd_star (nullptr));
  RUN (v_call_star, v_call_star (nullptr));
  RUN (v_bind, &v_bind (nullptr));
  RUN (v_cast, v_cast (nullptr));
  RUN (v_member, v_member (nullptr));
  RUN (v_sub, v_sub (nullptr, 0));
  RUN (n_rd, n_rd (nullptr));
  RUN (n_addr, n_addr (nullptr));
  RUN (n_ref, &n_ref (nullptr));
  RUN (n_call, n_call (nullptr));
  RUN (n_rd_star, n_rd_star (nullptr));
  RUN (n_call_star, n_call_star (nullptr));
  RUN (n_bind, &n_bind (nullptr));
  RUN (n_loaded, n_loaded (&n));
  RUN (n_sub, n_sub (nullptr, 0));
  RUN (m_rd, m_rd (nullptr));
  RUN (m_call, m_call (nullptr));
  RUN (m_ref, &m_ref (nullptr));
  RUN (m_rd_off, m_rd_off (nullptr));
  RUN (n_pmf_arrow, n_pmf_arrow (nullptr, &B::f));
  RUN (n_pmf_dot, n_pmf_dot (nullptr, &B::f));
  RUN (n_pm_dot, n_pm_dot (nullptr, &B::b));
  RUN (v_pmf_arrow, v_pmf_arrow (nullptr, &B::f));
  RUN (v_pmf_dot, v_pmf_dot (nullptr, &B::f));
  RUN (v_pm_dot, v_pm_dot (nullptr, &B::b));
  RUN (v_pm_addr, v_pm_addr (nullptr, &B::b));
  RUN (rd_first, rd_first (nullptr));
  return 0;
}
