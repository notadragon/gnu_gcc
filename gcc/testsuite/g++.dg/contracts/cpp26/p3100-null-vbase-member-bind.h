// A conversion of a glvalue to a virtual base through a null pointer, for
// the p3100-null-vbase-member-bind*.C tests: each run prints the number of
// implicit-assertion reports of one form.  A fault after the report is
// caught as SIGSEGV and resumed with siglongjmp, so one run counts the
// reports of every form.  The forms of Clang's
// Runnable/p3100-null-vbase-member-bind.cpp, in its order, but its last,
// `E *q = &hp->ve' (the conversion of the pointer `&hp->ve'), whose leaf is
// DECISIONS.md E96 (open).

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
struct B
{
  int b;
  void f () {}
};
struct VE : virtual E { int v; };
struct VB : virtual B { int v; };
struct HV { int x; VE ve; VE va[2]; VB vb; };

#define NI __attribute__((noinline))
NI E *hvmember_bind (HV *hp) { E &r = hp->ve; return &r; }
NI E *hvmember_addr (HV *hp) { return &static_cast<E &> (hp->ve); }
NI void hvmember_disc (HV *hp) { static_cast<E &> (hp->ve); }
NI E *hvstar_bind (HV *hp) { E &r = (*hp).ve; return &r; }
NI E *hvmptr_bind (HV *hp, VE HV::*pm) { E &r = hp->*pm; return &r; }
NI E *hvarray_bind (HV *hp) { E &r = hp->va[1]; return &r; }
NI void hvarray_disc (HV *hp) { static_cast<E &> (hp->va[1]); }
NI B *hvbase_bind (HV *hp) { B &r = hp->vb; return &r; }
NI int hvbase_read (HV *hp) { return hp->vb.b; }
NI void hvbase_call (HV *hp) { hp->vb.f (); }
NI int *hvbase_addr (HV *hp) { return &hp->vb.b; }
NI E *vsub_bind (VE *vp) { E &r = vp[1]; return &r; }
NI E *vsub0_bind (VE *vp) { E &r = vp[0]; return &r; }
NI void vsub_disc (VE *vp) { static_cast<E &> (vp[1]); }

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
  HV *hvp = reinterpret_cast<HV *> (vp);
  VE *vep = reinterpret_cast<VE *> (vp);
  HV hv{};
  VE va[2];
  COUNT ("hvmember_bind", hvmember_bind (hvp));
  COUNT ("hvmember_addr", hvmember_addr (hvp));
  COUNT ("hvmember_disc", hvmember_disc (hvp));
  COUNT ("hvstar_bind", hvstar_bind (hvp));
  COUNT ("hvmptr_bind", hvmptr_bind (hvp, &HV::ve));
  COUNT ("hvarray_bind", hvarray_bind (hvp));
  COUNT ("hvarray_disc", hvarray_disc (hvp));
  COUNT ("hvbase_bind", hvbase_bind (hvp));
  COUNT ("hvbase_read", hvbase_read (hvp));
  COUNT ("hvbase_call", hvbase_call (hvp));
  COUNT ("hvbase_addr", hvbase_addr (hvp));
  COUNT ("vsub_bind", vsub_bind (vep));
  COUNT ("vsub0_bind", vsub0_bind (vep));
  COUNT ("vsub_disc", vsub_disc (vep));
  COUNT ("nonnull", hvmember_bind (&hv); hvmember_addr (&hv);
	 hvmember_disc (&hv); hvstar_bind (&hv); hvmptr_bind (&hv, &HV::ve);
	 hvarray_bind (&hv); hvarray_disc (&hv); hvbase_bind (&hv);
	 hvbase_read (&hv); hvbase_call (&hv); hvbase_addr (&hv);
	 vsub_bind (va); vsub0_bind (va); vsub_disc (va));
  return 0;
}
