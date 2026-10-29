// A trivial special member function called explicitly through a null
// pointer, for the p3100-null-trivial-member-call*.C tests: each run
// prints the number of implicit-assertion reports of one form.  A fault
// after the report is caught as SIGSEGV and resumed with siglongjmp, so one
// run counts the reports of every form.  The forms of Clang's
// Runnable/p3100-null-trivial-member-call.cpp, in its order.

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

struct S
{
  int m;
  void self ();
};
struct A { int a; };
struct D : A, S {};
struct V : virtual S {};

#define NI __attribute__((noinline))
NI void sdtor (S *sp) { sp->~S (); }
NI void sdtor_star (S *sp) { (*sp).~S (); }
NI void sdtor_qual (S *sp) { sp->S::~S (); }
NI void bdtor (D *dp) { dp->S::~S (); }
NI void bdtor_star (D *dp) { (*dp).S::~S (); }
template <class T> NI void tdtor (T *p) { p->~T (); }
NI void vdtor (V *vp) { vp->S::~S (); }
NI void assign (S *sp, S &s) { sp->operator= (s); }
NI void assign_tmp (S *sp) { sp->operator= (S ()); }
NI void assign_base (D *dp, S &s) { dp->S::operator= (s); }
NI void assign_star (S *sp, S &s) { (*sp).operator= (s); }
NI void assign_op (S *sp, S &s) { *sp = s; }
// Controls.
void S::self ()
{
  this->~S ();
  S::~S ();
  this->operator= (*this);
}
NI void local ()
{
  S s{};
  s.~S ();
  (&s)->~S ();
  (&s)->operator= (s);
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
  int *volatile vp = nullptr;
  S *sp = reinterpret_cast<S *> (vp);
  D *dp = reinterpret_cast<D *> (vp);
  V *vbp = reinterpret_cast<V *> (vp);
  S s{};
  D d{};
  V v;
  COUNT ("sdtor", sdtor (sp));
  COUNT ("sdtor_star", sdtor_star (sp));
  COUNT ("sdtor_qual", sdtor_qual (sp));
  COUNT ("bdtor", bdtor (dp));
  COUNT ("bdtor_star", bdtor_star (dp));
  COUNT ("tdtor", tdtor (sp));
  COUNT ("vdtor", vdtor (vbp));
  COUNT ("assign", assign (sp, s));
  COUNT ("assign_tmp", assign_tmp (sp));
  COUNT ("assign_base", assign_base (dp, s));
  COUNT ("assign_star", assign_star (sp, s));
  COUNT ("assign_op", assign_op (sp, s));
  COUNT ("nonnull", sdtor (&s); sdtor_star (&s); bdtor (&d); bdtor_star (&d);
	 tdtor (&s); vdtor (&v); assign (&s, s); assign_base (&d, s));
  COUNT ("controls", s.self (); local ());
  return 0;
}
