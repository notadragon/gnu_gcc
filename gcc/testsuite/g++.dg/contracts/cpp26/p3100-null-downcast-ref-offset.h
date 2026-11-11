// A downcast of `*p' to a reference, p a null pointer, used -- a member
// read, a reference bound to it, a member function called on it -- for
// the p3100-null-downcast-ref-offset*.C tests: each run prints the number
// of implicit-assertion reports of one form.  The base is at a nonzero
// offset in the derived class, so the derived object's address, p minus
// the offset, is not null: polymorphic (`V' after the polymorphic `X' in
// `D2') and not (`S' after `Y' in `DS2').  Further forms: the address of a
// member, a store, a reference bound by an rvalue downcast, virtual calls
// (of the derived class's own function, of the base's, of the other
// base's).  The controls: the base at offset zero, a downcast of `*this'
// in a member function called through the null pointer, and non-null
// pointers.
// A fault after the report is caught as SIGSEGV and resumed with
// siglongjmp, so one run counts the reports of every form.

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

#define NI __attribute__((noinline))
struct V { virtual void h () {} void tc (); int v; };
struct X { virtual void x () {} long pad; };
// V is at offset 16 in D2, after the polymorphic X.
struct D2 : X, V { int d; void dn () {} virtual void dv () {} };
struct S { int s; };
struct Y { long y; };
// S is at offset 8 in DS2.
struct DS2 : Y, S { int t; void tn () {} };
// V is at offset zero in D.
struct D : V { int d; };
NI int drd (V *vp) { return static_cast<D2 &> (*vp).d; }
NI D2 &dbind (V *vp) { return static_cast<D2 &> (*vp); }
NI void dcall (V *vp) { static_cast<D2 &> (*vp).dn (); }
NI int srd (S *sp) { return static_cast<DS2 &> (*sp).t; }
NI DS2 &sbind (S *sp) { return static_cast<DS2 &> (*sp); }
NI void scall (S *sp) { static_cast<DS2 &> (*sp).tn (); }
NI int drd0 (V *vp) { return static_cast<D &> (*vp).d; }
NI D &dbind0 (V *vp) { return static_cast<D &> (*vp); }
NI int *daddr (V *vp) { return &static_cast<D2 &> (*vp).d; }
NI void dst (V *vp) { static_cast<D2 &> (*vp).d = 1; }
NI D2 &&drv (V *vp) { return static_cast<D2 &&> (*vp); }
NI void dvcall (V *vp) { static_cast<D2 &> (*vp).dv (); }
NI void dhcall (V *vp) { static_cast<D2 &> (*vp).h (); }
NI void dxcall (V *vp) { static_cast<D2 &> (*vp).x (); }
// A downcast of `*this' adds no check: a call through a null pointer is
// reported where it is made (DECISIONS.md E100 (a)).
NI void V::tc () { static_cast<D2 &> (*this).dn (); }
NI void dthis (V *vp) { vp->tc (); }

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
  int *volatile nv = nullptr;
  V *vp = reinterpret_cast<V *> (nv);
  S *sp = reinterpret_cast<S *> (nv);
  D2 d2;
  DS2 ds2{};
  COUNT ("drd", drd (vp));
  COUNT ("dbind", dbind (vp));
  COUNT ("dcall", dcall (vp));
  COUNT ("srd", srd (sp));
  COUNT ("sbind", sbind (sp));
  COUNT ("scall", scall (sp));
  COUNT ("drd0", drd0 (vp));
  COUNT ("dbind0", dbind0 (vp));
  COUNT ("daddr", daddr (vp));
  COUNT ("dst", dst (vp));
  COUNT ("drv", drv (vp));
  COUNT ("dvcall", dvcall (vp));
  COUNT ("dhcall", dhcall (vp));
  COUNT ("dxcall", dxcall (vp));
  COUNT ("dthis", dthis (vp));
  COUNT ("nonnull", drd (&d2); dbind (&d2); dcall (&d2); srd (&ds2);
	 scall (&ds2); daddr (&d2); dst (&d2); dvcall (&d2); dhcall (&d2);
	 dxcall (&d2); dthis (&d2));
  return 0;
}
