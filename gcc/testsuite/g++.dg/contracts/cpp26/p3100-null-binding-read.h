// An object reached through a null pointer, bound to a reference and read
// or written through it within the same expression, for the
// p3100-null-binding-read*.C tests: each run prints the number of
// implicit-assertion reports of one form.  A fault after the report is
// caught as SIGSEGV and resumed with siglongjmp, so one run counts the
// reports of every form.  The forms of Clang's
// Runnable/p3100-null-binding-read.cpp, in its order.

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

struct S { int m; };
// Copied without its tail padding.
struct T
{
  int m;
  char c;
  T () = default;
  T (const T &) = default;
  T &operator= (const T &) = default;
};
struct U : T { char d; };
struct H { int x; S s; };
struct H0 { S s; int x; };

#define NI __attribute__((noinline))
// The copy constructor's or assignment operator's reference parameter
// bound to *sp, and the copy reading it.
NI void scopy (S *sp) { S s2 = *sp; (void) s2; }
NI void sassign_from (S *sp, S &s) { s = *sp; }
NI void sparen (S *sp) { S s2 (*sp); (void) s2; }
NI void sbrace (S *sp) { S s2{*sp}; (void) s2; }
NI S sret (S *sp) { return *sp; }
NI void sbyval (S) {}
NI void sarg (S *sp) { sbyval (*sp); }
NI void ucopy (U *up) { U u2 = *up; (void) u2; }
NI void uassign_from (U *up, U &u) { u = *up; }
NI void tassign_base (U *up, T &t) { t = *up; }
NI void hmember_arg (H *hp) { sbyval (hp->s); }
template <class X> NI void tcopy (X *p) { X x = *p; (void) x; }
// A reference cast of *p read or written.
NI int iscast (int *p) { int x = static_cast<int &> (*p); return x; }
NI int iscast_add (int *p) { return static_cast<int &> (*p) + 1; }
NI void iscast_store (int *p) { static_cast<int &> (*p) = 1; }
NI void scast (S *sp) { S s2 = static_cast<S &> (*sp); (void) s2; }
NI int scast_member (S *sp) { return static_cast<S &> (*sp).m; }
// std::move and std::as_const, which return their argument.
NI int imove (int *p) { int x = std::move (*p); return x; }
NI void smove (S *sp) { S s2 = std::move (*sp); (void) s2; }
NI void smove_from (S *sp, S &s) { s = std::move (*sp); }
NI int iconst (int *p) { return std::as_const (*p); }
// A reference bound to a reference: no indirection of its own.
NI int &id (int &r) { return r; }
NI int rebind_id (int *p) { return id (*p); }
NI const int &cid (int &r) { return r; }
NI int rebind_const (int *p) { return cid (*p); }
// Only the address: .indirection.
NI int *scast_addr (S *sp) { return &static_cast<S &> (*sp).m; }
// Clang's extra forms: a class member copied or stored.
NI void hmember_copy (H *hp) { S s2 = hp->s; (void) s2; }
NI void hmember_assign (H *hp, S &s) { s = hp->s; }
NI S hmember_ret (H *hp) { return hp->s; }
NI void hmember_paren (H *hp) { S s2 ((hp->s)); (void) s2; }
NI void hmember_move (H *hp) { S s2 = std::move (hp->s); (void) s2; }
NI void hstar_member_copy (H *hp) { S s2 = (*hp).s; (void) s2; }
NI void hmember_store (H *hp, S &s) { hp->s = s; }
NI void h0member_arg (H0 *hp) { sbyval (hp->s); }
NI void h0member_copy (H0 *hp) { S s2 = hp->s; (void) s2; }
NI void h0member_assign (H0 *hp, S &s) { s = hp->s; }
NI void h0member_move (H0 *hp) { S s2 = std::move (hp->s); (void) s2; }
NI void h0member_store (H0 *hp, S &s) { hp->s = s; }

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
  int *ip = vp;
  S *sp = reinterpret_cast<S *> (vp);
  U *up = reinterpret_cast<U *> (vp);
  H *hp = reinterpret_cast<H *> (vp);
  H0 *h0p = reinterpret_cast<H0 *> (vp);
  int i = 0;
  S s{};
  U u{};
  T t{};
  H h{};
  H0 h0{};
  COUNT ("scopy", scopy (sp));
  COUNT ("sassign_from", sassign_from (sp, s));
  COUNT ("sparen", sparen (sp));
  COUNT ("sbrace", sbrace (sp));
  COUNT ("sret", sret (sp));
  COUNT ("sarg", sarg (sp));
  COUNT ("ucopy", ucopy (up));
  COUNT ("uassign_from", uassign_from (up, u));
  COUNT ("tassign_base", tassign_base (up, t));
  COUNT ("hmember_arg", hmember_arg (hp));
  COUNT ("tcopy", tcopy (sp));
  COUNT ("iscast", iscast (ip));
  COUNT ("iscast_add", iscast_add (ip));
  COUNT ("iscast_store", iscast_store (ip));
  COUNT ("scast", scast (sp));
  COUNT ("scast_member", scast_member (sp));
  COUNT ("imove", imove (ip));
  COUNT ("smove", smove (sp));
  COUNT ("smove_from", smove_from (sp, s));
  COUNT ("iconst", iconst (ip));
  COUNT ("rebind_id", rebind_id (ip));
  COUNT ("rebind_const", rebind_const (ip));
  COUNT ("scast_addr", scast_addr (sp));
  COUNT ("nonnull", scopy (&s); sassign_from (&s, s); sret (&s); sarg (&s);
	 ucopy (&u); uassign_from (&u, u); tassign_base (&u, t);
	 hmember_arg (&h); iscast (&i); iscast_store (&i); smove (&s);
	 imove (&i); iconst (&i); rebind_id (&i); rebind_const (&i));
  COUNT ("hmember_copy", hmember_copy (hp));
  COUNT ("hmember_assign", hmember_assign (hp, s));
  COUNT ("hmember_ret", hmember_ret (hp));
  COUNT ("hmember_paren", hmember_paren (hp));
  COUNT ("hmember_move", hmember_move (hp));
  COUNT ("hstar_member_copy", hstar_member_copy (hp));
  COUNT ("hmember_store", hmember_store (hp, s));
  COUNT ("h0member_arg", h0member_arg (h0p));
  COUNT ("h0member_copy", h0member_copy (h0p));
  COUNT ("h0member_assign", h0member_assign (h0p, s));
  COUNT ("h0member_move", h0member_move (h0p));
  COUNT ("h0member_store", h0member_store (h0p, s));
  COUNT ("nonnull_extra", hmember_copy (&h); hmember_move (&h);
	 hmember_store (&h, s); h0member_arg (&h0); h0member_copy (&h0);
	 h0member_store (&h0, s));
  return 0;
}
