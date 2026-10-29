// The element at a constant subscript of a null pointer, for the
// p3100-null-constant-subscript*.C tests: each run prints the number of
// implicit-assertion reports of one form.  A fault after the report is
// caught as SIGSEGV and resumed with siglongjmp, so one run counts the
// reports of every form.  The forms up to `nonnull' are mirrored, in this
// order, by llvm_llvm-project's Runnable/p3100-null-constant-subscript.cpp;
// the forms after it are Clang's.

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
struct S { int a, b; int arr[2]; void f () {} };
struct V { virtual void h () {} int v; };
struct T { int a; ~T () {} };
struct B { int x; void g () {} };
struct D : B { int y; };

#define NI __attribute__((noinline))
NI void sbyval (S) {}
NI void ebyval (E) {}
NI void sbyref (S &) {}
NI int mread (S *sp) { return sp[1].a; }
NI void mstore (S *sp) { sp[1].a = 1; }
NI int *maddr (S *sp) { return &sp[1].a; }
NI void mbind (S *sp) { int &r = sp[1].a; (void) r; }
NI void sbind (S *sp) { S &r = sp[1]; (void) r; }
NI S *saddr (S *sp) { return &sp[1]; }
NI void scopy (S *sp) { S s2 = sp[1]; (void) s2; }
NI void sfrom (S *sp, S &s) { s = sp[1]; }
NI void sstore (S *sp, S &s) { sp[1] = s; }
NI void sarg (S *sp) { sbyval (sp[1]); }
NI void scall (S *sp) { sp[1].f (); }
NI int arrread (S *sp) { return sp[1].arr[1]; }
NI void arrstore (S *sp) { sp[1].arr[1] = 1; }
NI int iread (int *ip) { return ip[1]; }
NI void istore (int *ip) { ip[1] = 2; }
NI int pm (S *sp, int S::*p) { return sp[1].*p; }
NI void efrom (E *ep, E &e) { e = ep[1]; }
NI void ecopy (E *ep) { E e2 = ep[1]; (void) e2; }
NI void earg (E *ep) { ebyval (ep[1]); }
NI void estore (E *ep, E &e) { ep[1] = e; }
NI void ebind (E *ep) { E &r = ep[1]; (void) r; }
NI void vcall (V *vp) { vp[1].h (); }
NI int vread (V *vp) { return vp[1].v; }
// A negative constant subscript.
NI int mread_neg (S *sp) { return sp[-1].a; }
NI int *maddr_neg (S *sp) { return &sp[-1].a; }
NI void sbind_neg (S *sp) { S &r = sp[-1]; (void) r; }
NI void ecopy_neg (E *ep) { E e2 = ep[-1]; (void) e2; }
// Clang's: other uses that bind or call through the element.
NI void pmf (S *sp, void (S::*p) ()) { (sp[1].*p) (); }
NI void sarg_ref (S *sp) { sbyref (sp[1]); }
NI S &sret_ref (S *sp) { return sp[1]; }
NI void scbind (S *sp) { const S &r = sp[1]; (void) r; }
NI void slambda (S *sp) { auto l = [&r = sp[1]] { (void) r; }; l (); }
NI void tdtor (T *tp) { tp[1].~T (); }
NI void bbind (D *dp) { B &r = dp[1]; (void) r; }
NI void bcall (D *dp) { dp[1].g (); }

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
  S *sp = reinterpret_cast<S *> (nv);
  E *ep = reinterpret_cast<E *> (nv);
  V *vp = reinterpret_cast<V *> (nv);
  T *tp = reinterpret_cast<T *> (nv);
  D *dp = reinterpret_cast<D *> (nv);
  int *ip = nv;
  S s{}, sa[3]{};
  E e, ea[3];
  V va[3];
  T ta[3]{};
  D da[3]{};
  int ia[3]{};
  COUNT ("mread", mread (sp));
  COUNT ("mstore", mstore (sp));
  COUNT ("maddr", maddr (sp));
  COUNT ("mbind", mbind (sp));
  COUNT ("sbind", sbind (sp));
  COUNT ("saddr", saddr (sp));
  COUNT ("scopy", scopy (sp));
  COUNT ("sfrom", sfrom (sp, s));
  COUNT ("sstore", sstore (sp, s));
  COUNT ("sarg", sarg (sp));
  COUNT ("scall", scall (sp));
  COUNT ("arrread", arrread (sp));
  COUNT ("arrstore", arrstore (sp));
  COUNT ("iread", iread (ip));
  COUNT ("istore", istore (ip));
  COUNT ("pm", pm (sp, &S::b));
  COUNT ("efrom", efrom (ep, e));
  COUNT ("ecopy", ecopy (ep));
  COUNT ("earg", earg (ep));
  COUNT ("estore", estore (ep, e));
  COUNT ("ebind", ebind (ep));
  COUNT ("vcall", vcall (vp));
  COUNT ("vread", vread (vp));
  COUNT ("mread_neg", mread_neg (sp));
  COUNT ("maddr_neg", maddr_neg (sp));
  COUNT ("sbind_neg", sbind_neg (sp));
  COUNT ("ecopy_neg", ecopy_neg (ep));
  COUNT ("nonnull", mread (sa); mstore (sa); maddr (sa); mbind (sa);
	 sbind (sa); saddr (sa); scopy (sa); sfrom (sa, s); sstore (sa, s);
	 sarg (sa); scall (sa); arrread (sa); arrstore (sa); iread (ia);
	 istore (ia); pm (sa, &S::b); efrom (ea, e); ecopy (ea); earg (ea);
	 estore (ea, e); ebind (ea); vcall (va); vread (va);
	 mread_neg (sa + 1); maddr_neg (sa + 1); sbind_neg (sa + 1);
	 ecopy_neg (ea + 1));
  COUNT ("pmf", pmf (sp, &S::f));
  COUNT ("sarg_ref", sarg_ref (sp));
  COUNT ("sret_ref", sret_ref (sp));
  COUNT ("scbind", scbind (sp));
  COUNT ("slambda", slambda (sp));
  COUNT ("tdtor", tdtor (tp));
  COUNT ("bbind", bbind (dp));
  COUNT ("bcall", bcall (dp));
  COUNT ("nonnull_extra", pmf (sa, &S::f); sarg_ref (sa); sret_ref (sa);
	 scbind (sa); slambda (sa); tdtor (ta); bbind (da); bcall (da));
  return 0;
}
