// A trivial copy or move of a class through a null pointer, for the
// p3100-null-empty-class-copy*.C tests: each run prints the number of
// implicit-assertion reports of one form.  A fault after the report is
// caught as SIGSEGV and resumed with siglongjmp, so one run counts the
// reports of every form.  The forms of Clang's
// Runnable/p3100-null-empty-class-copy.cpp, in its order, then GCC's extra
// ones.

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
#include <utility>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation (const cs::contract_violation &v)
{
  if (v.kind () == cs::assertion_kind::implicit)
    ++reports;
}

static sigjmp_buf env;
static void on_segv (int) { siglongjmp (env, 1); }

struct E {};
struct S { int m; };
struct A { int a; };
struct DE : A, E {};
struct VE : virtual E { int v; };

#define NI __attribute__((noinline))
NI void eassign (E *ep, E &e) { *ep = e; }
NI void eassign_tmp (E *ep) { *ep = E (); }
NI void eassign_call (E *ep, E &e) { ep->operator= (e); }
NI void eassign_from (E *ep, E &e) { e = *ep; }
NI void ecopy (E *ep) { E e2 = *ep; (void) e2; }
NI void emove (E *ep) { E e2 = std::move (*ep); (void) e2; }
NI void sassign (S *sp, S &s) { *sp = s; }
NI void sassign_call (S *sp, S &s) { sp->operator= (s); }
NI void sassign_from (S *sp, S &s) { s = *sp; }
NI void scopy (S *sp) { S s2 = *sp; (void) s2; }
// GCC's extra forms.
NI void ebyval (E) {}
NI void earg (E *ep) { ebyval (*ep); }
NI E eret (E *ep) { return *ep; }
NI void ebrace (E *ep) { E e2{*ep}; (void) e2; }
NI void ecast (E *ep) { E e2 = static_cast<E &> (*ep); (void) e2; }
NI void emove_from (E *ep, E &e) { e = std::move (*ep); }
NI void ebase_from (DE *dp, E &e) { e = *dp; }
NI void evbase_from (VE *vp, E &e) { e = *vp; }
template <class T> NI void tcopy (T *p) { T t = *p; (void) t; }

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
  E *ep = reinterpret_cast<E *> (vp);
  S *sp = reinterpret_cast<S *> (vp);
  DE *dep = reinterpret_cast<DE *> (vp);
  VE *vep = reinterpret_cast<VE *> (vp);
  E e;
  S s{};
  DE de{};
  VE ve;
  COUNT ("eassign", eassign (ep, e));
  COUNT ("eassign_tmp", eassign_tmp (ep));
  COUNT ("eassign_call", eassign_call (ep, e));
  COUNT ("eassign_from", eassign_from (ep, e));
  COUNT ("ecopy", ecopy (ep));
  COUNT ("emove", emove (ep));
  COUNT ("sassign", sassign (sp, s));
  COUNT ("sassign_call", sassign_call (sp, s));
  COUNT ("sassign_from", sassign_from (sp, s));
  COUNT ("scopy", scopy (sp));
  COUNT ("earg", earg (ep));
  COUNT ("eret", eret (ep));
  COUNT ("ebrace", ebrace (ep));
  COUNT ("ecast", ecast (ep));
  COUNT ("emove_from", emove_from (ep, e));
  COUNT ("ebase_from", ebase_from (dep, e));
  COUNT ("evbase_from", evbase_from (vep, e));
  COUNT ("tcopy", tcopy (ep));
  COUNT ("nonnull", eassign (&e, e); eassign_call (&e, e);
	 eassign_from (&e, e); ecopy (&e); emove (&e);
	 sassign_call (&s, s); scopy (&s); earg (&e); eret (&e);
	 emove_from (&e, e); ebase_from (&de, e); evbase_from (&ve, e);
	 tcopy (&e));
  return 0;
}
