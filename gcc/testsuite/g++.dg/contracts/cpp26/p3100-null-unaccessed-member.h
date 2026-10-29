// A member of an object reached through a null pointer, evaluated and not
// accessed, for the p3100-null-unaccessed-member*.C tests: each run prints
// the number of implicit-assertion reports of one form.  A fault after the
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

struct E {};
struct S
{
  int m;
  enum { K = 5 };
  static constexpr int ck = 6;
  static int sf () { return 3; }
};
struct H
{
  int x;
  S s;
  int a[2];
  E e;
  int g ();
};
struct A { int a; };
struct B { int b; };
struct D : A, B {};
struct V : virtual B { int v; };
struct VH { volatile int vx; };

#define NI __attribute__((noinline))
// Discarded.
NI void disc_member (H *hp) { hp->x; }
NI void disc_member_void (H *hp) { (void) hp->x; }
NI void disc_member_star (H *hp) { (*hp).x; }
NI void disc_member_nested (H *hp) { hp->s.m; }
NI void disc_member_array (H *hp) { hp->a[1]; }
NI void disc_member_comma (H *hp) { (hp->x, 0); }
NI void disc_member_empty (H *hp) { hp->e; }
NI void disc_member_cast (S *sp) { static_cast<S &> (*sp).m; }
NI void disc_member_base (D *dp) { dp->b; }
NI void disc_member_vbase (V *vp) { vp->b; }
// The object expression of a static member, an enumerator, a static
// member function; a trivial destructor called on a member.
NI int smember_static (H *hp) { return hp->s.ck; }
NI int smember_enum (H *hp) { return hp->s.K; }
NI int smember_sf (H *hp) { return hp->s.sf (); }
NI void member_dtor (H *hp) { hp->s.~S (); }
// The subscript is evaluated.
NI int disc_member_side (H *hp) { int i = 0; hp->a[i++]; return i; }
// Read: the .access leaf.
NI void disc_volatile (VH *p) { p->vx; }
// Controls: a reference, `this'.
NI void disc_ref_member (H &r) { r.x; }
int H::g () { x; this->x; (*this).x; return 0; }

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
  H *hp = reinterpret_cast<H *> (vp);
  S *sp = reinterpret_cast<S *> (vp);
  D *dp = reinterpret_cast<D *> (vp);
  V *vbp = reinterpret_cast<V *> (vp);
  VH *vhp = reinterpret_cast<VH *> (vp);
  H h{};
  S s{};
  D d{};
  V v;
  VH vh{};
  COUNT ("disc_member", disc_member (hp));
  COUNT ("disc_member_void", disc_member_void (hp));
  COUNT ("disc_member_star", disc_member_star (hp));
  COUNT ("disc_member_nested", disc_member_nested (hp));
  COUNT ("disc_member_array", disc_member_array (hp));
  COUNT ("disc_member_comma", disc_member_comma (hp));
  COUNT ("disc_member_empty", disc_member_empty (hp));
  COUNT ("disc_member_cast", disc_member_cast (sp));
  COUNT ("disc_member_base", disc_member_base (dp));
  COUNT ("disc_member_vbase", disc_member_vbase (vbp));
  COUNT ("smember_static", smember_static (hp));
  COUNT ("smember_enum", smember_enum (hp));
  COUNT ("smember_sf", smember_sf (hp));
  COUNT ("member_dtor", member_dtor (hp));
  COUNT ("disc_member_side", if (disc_member_side (hp) != 1) reports += 100);
  COUNT ("disc_volatile", disc_volatile (vhp));
  COUNT ("nonnull", disc_member (&h); disc_member_nested (&h);
	 disc_member_array (&h); disc_member_cast (&s); disc_member_base (&d);
	 disc_member_vbase (&v); smember_static (&h); member_dtor (&h);
	 disc_volatile (&vh));
  COUNT ("controls", disc_ref_member (h); h.g ());
  return 0;
}
