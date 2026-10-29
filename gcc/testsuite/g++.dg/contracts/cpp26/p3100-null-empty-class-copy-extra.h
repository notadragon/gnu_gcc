// More trivial copies of an empty class through a null pointer, for the
// p3100-null-empty-class-copy-extra*.C tests: each run prints the number
// of implicit-assertion reports of one form.  A fault after the report is
// caught as SIGSEGV and resumed with siglongjmp, so one run counts the
// reports of every form.  The forms of Clang's
// Runnable/p3100-null-empty-class-copy-extra.cpp, in its order, then GCC's
// extra ones.

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
struct A { int a; };
struct DE : A, E {};
struct DE0 : E { int b; };
struct VE : virtual E { int v; };
struct HE { int x; E e; };
struct HD { int x; DE d; };
// GCC's: an empty base at a nonzero offset (a member of type E is at
// offset 0), an array of empty members, a member with an empty virtual
// base.
struct F0 { E e; int i; };
struct DEn : F0, E {};
struct HA { int x; E ea[2]; };
struct HV { int x; VE ve; };

#define NI __attribute__((noinline))
NI void ebyval (E) {}
// GCC's extra forms of p3100-null-empty-class-copy.h.
NI void earg (E *ep) { ebyval (*ep); }
NI E eret (E *ep) { return *ep; }
NI void ebrace (E *ep) { E e2{*ep}; (void) e2; }
NI void ecast (E *ep) { E e2 = static_cast<E &> (*ep); (void) e2; }
NI void emove_from (E *ep, E &e) { e = std::move (*ep); }
NI void ebase_from (DE *dp, E &e) { e = *dp; }
NI void evbase_from (VE *vp, E &e) { e = *vp; }
template <class T> NI void tcopy (T *p) { T t = *p; (void) t; }
// Clang's extra forms.
NI void ebase_copy (DE *dp) { E e2 = *dp; (void) e2; }
NI void ebase_arg (DE *dp) { ebyval (*dp); }
NI E ebase_ret (DE *dp) { return *dp; }
NI void ebase_store (DE *dp, E &e) { static_cast<E &> (*dp) = e; }
NI void ebase_call (DE *dp, E &e) { dp->E::operator= (e); }
NI void ebase_move (DE *dp, E &e) { e = std::move (*dp); }
NI void ebase0_from (DE0 *dp, E &e) { e = *dp; }
NI void evbase_copy (VE *vp) { E e2 = *vp; (void) e2; }
NI void emember_copy (HE *hp) { E e2 = hp->e; (void) e2; }
NI void emember_arg (HE *hp) { ebyval (hp->e); }
NI void emember_store (HE *hp, E &e) { hp->e = e; }
NI void hdmember_base (HD *hp, E &e) { e = hp->d; }
// GCC's extra forms.
NI void ebasen_from (DEn *dp, E &e) { e = *dp; }
NI void ebasen_copy (DEn *dp) { E e2 = *dp; (void) e2; }
NI void ebasen_move (DEn *dp, E &e) { e = std::move (*dp); }
NI void ebasen_store (DEn *dp, E &e) { static_cast<E &> (*dp) = e; }
NI void ebasen_call (DEn *dp, E &e) { dp->E::operator= (e); }
NI E *ebasen_addr (DEn *dp) { return &static_cast<E &> (*dp); }
NI E *ebasen_bind (DEn *dp) { E &r = *dp; return &r; }
NI void ebasen_disc (DEn *dp) { static_cast<E &> (*dp); }
NI void emember_move (HE *hp) { E e2 = std::move (hp->e); (void) e2; }
NI void emember_move_from (HE *hp, E &e) { e = std::move (hp->e); }
NI void earray_copy (HA *hp) { E e2 = hp->ea[1]; (void) e2; }
NI void earray_from (HA *hp, E &e) { e = hp->ea[1]; }
NI void earray_store (HA *hp, E &e) { hp->ea[1] = e; }
NI int earray_side (HA *hp, E &e) { int i = 0; e = hp->ea[i++]; return i; }
NI void hvmember_from (HV *hp, E &e) { e = hp->ve; }
NI void hvmember_copy (HV *hp) { E e2 = hp->ve; (void) e2; }
NI void hvmember_disc (HV *hp) { static_cast<E &> (hp->ve); }

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
  DE *dep = reinterpret_cast<DE *> (vp);
  DE0 *de0p = reinterpret_cast<DE0 *> (vp);
  VE *vep = reinterpret_cast<VE *> (vp);
  HE *hep = reinterpret_cast<HE *> (vp);
  HD *hdp = reinterpret_cast<HD *> (vp);
  DEn *denp = reinterpret_cast<DEn *> (vp);
  HA *hap = reinterpret_cast<HA *> (vp);
  HV *hvp = reinterpret_cast<HV *> (vp);
  E e;
  DE de{};
  DE0 de0{};
  VE ve;
  HE he{};
  HD hd{};
  DEn den{};
  HA ha{};
  HV hv{};
  COUNT ("earg", earg (ep));
  COUNT ("eret", eret (ep));
  COUNT ("ebrace", ebrace (ep));
  COUNT ("ecast", ecast (ep));
  COUNT ("emove_from", emove_from (ep, e));
  COUNT ("ebase_from", ebase_from (dep, e));
  COUNT ("evbase_from", evbase_from (vep, e));
  COUNT ("tcopy", tcopy (ep));
  COUNT ("ebase_copy", ebase_copy (dep));
  COUNT ("ebase_arg", ebase_arg (dep));
  COUNT ("ebase_ret", ebase_ret (dep));
  COUNT ("ebase_store", ebase_store (dep, e));
  COUNT ("ebase_call", ebase_call (dep, e));
  COUNT ("ebase_move", ebase_move (dep, e));
  COUNT ("ebase0_from", ebase0_from (de0p, e));
  COUNT ("evbase_copy", evbase_copy (vep));
  COUNT ("emember_copy", emember_copy (hep));
  COUNT ("emember_arg", emember_arg (hep));
  COUNT ("emember_store", emember_store (hep, e));
  COUNT ("hdmember_base", hdmember_base (hdp, e));
  COUNT ("nonnull", earg (&e); eret (&e); emove_from (&e, e);
	 ebase_from (&de, e); evbase_from (&ve, e); tcopy (&e);
	 ebase_copy (&de); ebase_store (&de, e); ebase_call (&de, e);
	 ebase_move (&de, e); ebase0_from (&de0, e); evbase_copy (&ve);
	 emember_copy (&he); emember_store (&he, e); hdmember_base (&hd, e));
  COUNT ("ebasen_from", ebasen_from (denp, e));
  COUNT ("ebasen_copy", ebasen_copy (denp));
  COUNT ("ebasen_move", ebasen_move (denp, e));
  COUNT ("ebasen_store", ebasen_store (denp, e));
  COUNT ("ebasen_call", ebasen_call (denp, e));
  COUNT ("ebasen_addr", ebasen_addr (denp));
  COUNT ("ebasen_bind", ebasen_bind (denp));
  COUNT ("ebasen_disc", ebasen_disc (denp));
  COUNT ("emember_move", emember_move (hep));
  COUNT ("emember_move_from", emember_move_from (hep, e));
  COUNT ("earray_copy", earray_copy (hap));
  COUNT ("earray_from", earray_from (hap, e));
  COUNT ("earray_store", earray_store (hap, e));
  COUNT ("earray_side", if (earray_side (hap, e) != 1) reports += 100);
  COUNT ("hvmember_from", hvmember_from (hvp, e));
  COUNT ("hvmember_copy", hvmember_copy (hvp));
  COUNT ("hvmember_disc", hvmember_disc (hvp));
  COUNT ("nonnull_extra", ebasen_from (&den, e); ebasen_copy (&den);
	 ebasen_move (&den, e); ebasen_store (&den, e); ebasen_call (&den, e);
	 ebasen_addr (&den); ebasen_bind (&den); ebasen_disc (&den);
	 emember_move (&he); emember_move_from (&he, e); earray_copy (&ha);
	 earray_from (&ha, e); earray_store (&ha, e); earray_side (&ha, e);
	 hvmember_from (&hv, e); hvmember_copy (&hv); hvmember_disc (&hv));
  return 0;
}
