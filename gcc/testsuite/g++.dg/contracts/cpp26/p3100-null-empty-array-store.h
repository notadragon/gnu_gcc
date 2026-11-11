// A store to an element of an array of empty class objects through a null
// pointer, for the p3100-null-empty-array-store*.C tests: each run prints
// the number of implicit-assertion reports of one form.  A fault after the
// report is caught as SIGSEGV and resumed with siglongjmp, so one run
// counts the reports of every form.  The forms of Clang's
// Runnable/p3100-null-empty-array-store.cpp, in its order.

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

struct E {};
struct HA { int x; E ea[2]; E eaa[2][2]; };
struct H0 { E ea[2]; int x; };

#define NI __attribute__((noinline))
NI void earray_store (HA *hp, E &e) { hp->ea[1] = e; }
NI void earray_store_star (HA *hp, E &e) { (*hp).ea[1] = e; }
NI void earray2_store (HA *hp, E &e) { hp->eaa[1][0] = e; }
NI void earray_move_store (HA *hp, E &e) { hp->ea[1] = static_cast<E &&> (e); }
NI void earray_call (HA *hp, E &e) { hp->ea[1].operator= (e); }
NI void earray0_store (H0 *hp, E &e) { hp->ea[0] = e; }
NI void eptr_store (E *ep, E &e) { ep[1] = e; }
NI void eptr0_store (E *ep, E &e) { ep[0] = e; }

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
  HA *hap = reinterpret_cast<HA *> (vp);
  H0 *h0p = reinterpret_cast<H0 *> (vp);
  E *ep = reinterpret_cast<E *> (vp);
  E e, ea[2];
  HA ha{};
  H0 h0{};
  COUNT ("earray_store", earray_store (hap, e));
  COUNT ("earray_store_star", earray_store_star (hap, e));
  COUNT ("earray2_store", earray2_store (hap, e));
  COUNT ("earray_move_store", earray_move_store (hap, e));
  COUNT ("earray_call", earray_call (hap, e));
  COUNT ("earray0_store", earray0_store (h0p, e));
  COUNT ("eptr_store", eptr_store (ep, e));
  COUNT ("eptr0_store", eptr0_store (ep, e));
  COUNT ("nonnull", earray_store (&ha, e); earray_store_star (&ha, e);
	 earray2_store (&ha, e); earray_move_store (&ha, e);
	 earray_call (&ha, e); earray0_store (&h0, e); eptr_store (ea, e);
	 eptr0_store (ea, e));
  return 0;
}
