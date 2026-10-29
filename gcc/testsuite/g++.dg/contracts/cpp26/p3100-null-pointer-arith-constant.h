// An indirection through pointer arithmetic with a null pointer and a
// constant, `*(p + c)', for the p3100-null-pointer-arith-constant*.C tests:
// each run prints the number of implicit-assertion reports of one form.  A
// fault after the report is caught as SIGSEGV and resumed with siglongjmp,
// so one run counts the reports of every form.  The forms of
// llvm_llvm-project's Runnable/p3100-null-pointer-arith-constant.cpp, in its
// order; then GCC's extra forms: up to `nonnull_gcc' with `p + 1', then the
// other spellings of a pointer plus or minus an integral constant
// expression (DECISIONS.md E105 (a)): `c + p', `p - c', a constexpr and a
// const variable as `c'.

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
struct S { int a, b; int arr[2]; void f () {} static void sf () {} };
struct B { int x; void g () {} };
struct D : B { int y; };
struct V { virtual void h () {} int v; };

#define NI __attribute__((noinline))
constexpr int k = 1;
const int kc = 1;
NI void sbyval (S) {}
NI void ebyval (E) {}
NI int iread (int *ip) { return *(ip + 1); }
NI void istore (int *ip) { *(ip + 1) = 2; }
NI int iread_rev (int *ip) { return *(1 + ip); }
NI int iread_neg (int *ip) { return *(ip - 1); }
NI int iread_k (int *ip) { return *(ip + k); }
NI int mread (S *sp) { return (sp + 1)->a; }
NI void mstore (S *sp) { (sp + 1)->a = 1; }
NI int mread_dot (S *sp) { return (*(sp + 1)).a; }
NI int *maddr (S *sp) { return &(sp + 1)->a; }
NI void mbind (S *sp) { int &r = (sp + 1)->a; (void) r; }
NI void sbind (S *sp) { S &r = *(sp + 1); (void) r; }
NI S *saddr (S *sp) { return &*(sp + 1); }
NI void scopy (S *sp) { S s2 = *(sp + 1); (void) s2; }
NI void sfrom (S *sp, S &s) { s = *(sp + 1); }
NI void sstore (S *sp, S &s) { *(sp + 1) = s; }
NI void sarg (S *sp) { sbyval (*(sp + 1)); }
NI void scall (S *sp) { (sp + 1)->f (); }
NI int arrread (S *sp) { return (sp + 1)->arr[1]; }
NI int pm (S *sp, int S::*p) { return (sp + 1)->*p; }
NI void ecopy (E *ep) { E e2 = *(ep + 1); (void) e2; }
NI void estore (E *ep, E &e) { *(ep + 1) = e; }
NI void ebind (E *ep) { E &r = *(ep + 1); (void) r; }
NI void vcall (V *vp) { (vp + 1)->h (); }
NI int vread (V *vp) { return (vp + 1)->v; }
NI void disc (S *sp) { *(sp + 1); }
// Calls through the arithmetic.
NI void sassign (S *sp, S &s) { (sp + 1)->operator= (s); }
NI void sdtor (S *sp) { (sp + 1)->~S (); }
NI void sstatic (S *sp) { (sp + 1)->sf (); }
NI void pmf (S *sp, void (S::*p) ()) { ((sp + 1)->*p) (); }
NI void bcall (D *dp) { (dp + 1)->g (); }
// GCC's: an element of an array member stored to, an empty class assigned
// from it and copied by value.
NI void arrstore (S *sp) { (sp + 1)->arr[1] = 1; }
NI void efrom (E *ep, E &e) { e = *(ep + 1); }
NI void earg (E *ep) { ebyval (*(ep + 1)); }
// GCC's: the other spellings (DECISIONS.md E105 (a)), stored to, a member
// read, stored to and its address, a const variable, a reference bound,
// a class copied, a member function called and a pointer to member.
NI void istore_rev (int *ip) { *(1 + ip) = 2; }
NI void istore_neg (int *ip) { *(ip - 1) = 2; }
NI void istore_k (int *ip) { *(ip + k) = 2; }
NI int mread_rev (S *sp) { return (1 + sp)->a; }
NI int mread_neg (S *sp) { return (sp - 1)->a; }
NI int mread_k (S *sp) { return (sp + k)->a; }
NI void mstore_rev (S *sp) { (1 + sp)->a = 1; }
NI void mstore_neg (S *sp) { (sp - 1)->a = 1; }
NI void mstore_k (S *sp) { (sp + k)->a = 1; }
NI int *maddr_rev (S *sp) { return &(1 + sp)->a; }
NI int *maddr_neg (S *sp) { return &(sp - 1)->a; }
NI int *maddr_k (S *sp) { return &(sp + k)->a; }
NI int iread_kc (int *ip) { return *(ip + kc); }
NI int mread_kc (S *sp) { return (sp + kc)->a; }
NI int mread_negk (S *sp) { return (sp - k)->a; }
NI int mread_revk (S *sp) { return (kc + sp)->a; }
NI void sbind_rev (S *sp) { S &r = *(1 + sp); (void) r; }
NI void scopy_neg (S *sp) { S s2 = *(sp - 1); (void) s2; }
NI void scall_k (S *sp) { (sp + k)->f (); }
NI int pm_kc (S *sp, int S::*p) { return (sp + kc)->*p; }

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
  D *dp = reinterpret_cast<D *> (nv);
  int *ip = nv;
  S s{}, sa[3]{};
  D da[3]{};
  E e, ea[3];
  V va[3];
  int ia[3]{};
  COUNT ("iread", iread (ip));
  COUNT ("istore", istore (ip));
  COUNT ("iread_rev", iread_rev (ip));
  COUNT ("iread_neg", iread_neg (ip));
  COUNT ("iread_k", iread_k (ip));
  COUNT ("mread", mread (sp));
  COUNT ("mstore", mstore (sp));
  COUNT ("mread_dot", mread_dot (sp));
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
  COUNT ("pm", pm (sp, &S::b));
  COUNT ("ecopy", ecopy (ep));
  COUNT ("estore", estore (ep, e));
  COUNT ("ebind", ebind (ep));
  COUNT ("vcall", vcall (vp));
  COUNT ("vread", vread (vp));
  COUNT ("disc", disc (sp));
  COUNT ("nonnull", iread (ia); istore (ia); iread_rev (ia);
	 iread_neg (ia + 1); iread_k (ia); mread (sa); mstore (sa);
	 mread_dot (sa); maddr (sa); mbind (sa); sbind (sa); saddr (sa);
	 scopy (sa); sfrom (sa, s); sstore (sa, s); sarg (sa); scall (sa);
	 arrread (sa); pm (sa, &S::b); ecopy (ea); estore (ea, e);
	 ebind (ea); vcall (va); vread (va); disc (sa));
  COUNT ("sassign", sassign (sp, s));
  COUNT ("sdtor", sdtor (sp));
  COUNT ("sstatic", sstatic (sp));
  COUNT ("pmf", pmf (sp, &S::f));
  COUNT ("bcall", bcall (dp));
  COUNT ("nonnull_extra", sassign (sa, s); sdtor (sa); sstatic (sa);
	 pmf (sa, &S::f); bcall (da));
  COUNT ("arrstore", arrstore (sp));
  COUNT ("efrom", efrom (ep, e));
  COUNT ("earg", earg (ep));
  COUNT ("nonnull_gcc", arrstore (sa); efrom (ea, e); earg (ea));
  COUNT ("istore_rev", istore_rev (ip));
  COUNT ("istore_neg", istore_neg (ip));
  COUNT ("istore_k", istore_k (ip));
  COUNT ("mread_rev", mread_rev (sp));
  COUNT ("mread_neg", mread_neg (sp));
  COUNT ("mread_k", mread_k (sp));
  COUNT ("mstore_rev", mstore_rev (sp));
  COUNT ("mstore_neg", mstore_neg (sp));
  COUNT ("mstore_k", mstore_k (sp));
  COUNT ("maddr_rev", maddr_rev (sp));
  COUNT ("maddr_neg", maddr_neg (sp));
  COUNT ("maddr_k", maddr_k (sp));
  COUNT ("iread_kc", iread_kc (ip));
  COUNT ("mread_kc", mread_kc (sp));
  COUNT ("mread_negk", mread_negk (sp));
  COUNT ("mread_revk", mread_revk (sp));
  COUNT ("sbind_rev", sbind_rev (sp));
  COUNT ("scopy_neg", scopy_neg (sp));
  COUNT ("scall_k", scall_k (sp));
  COUNT ("pm_kc", pm_kc (sp, &S::b));
  COUNT ("nonnull_e105", istore_rev (ia); istore_neg (ia + 1);
	 istore_k (ia); mread_rev (sa); mread_neg (sa + 1); mread_k (sa);
	 mstore_rev (sa); mstore_neg (sa + 1); mstore_k (sa);
	 maddr_rev (sa); maddr_neg (sa + 1); maddr_k (sa); iread_kc (ia);
	 mread_kc (sa); mread_negk (sa + 1); mread_revk (sa);
	 sbind_rev (sa); scopy_neg (sa + 1); scall_k (sa);
	 pm_kc (sa, &S::b));
  return 0;
}
