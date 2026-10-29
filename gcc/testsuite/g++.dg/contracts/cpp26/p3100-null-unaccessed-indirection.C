// P3100: an indirection through a null pointer whose result is not
// accessed is reported by ub:expr.unary.dereference.nullptr.indirection
// (GCC-799) -- `&*p' (and `&p[0]', in a template too), a discarded `*p;'
// or `(void) *p', a static member named through a null pointer
// (`sp->sf ()', `(*sp).sf ()', `sp->k', `&sp->k'), and `&*fp' and a
// discarded `*fp;' for a pointer to function, each of which evaluates
// `*p' ([expr.unary.op], [expr.ref]) -- once each under noexcept_observe,
// as a reference bound to `*p' is.  A constexpr function's call that GCC
// would fold is not folded when the pointer is null; with a pointer to an
// object it still is, and constant evaluation is unchanged.  A non-null
// pointer reports nothing.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-unaccessed-indirection.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-additional-options "-Wno-unused-value" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation (const cs::contract_violation &)
{
  ++reports;
}

struct S { static int k; static constexpr int sf () { return 3; } int m; };
int S::k = 4;

__attribute__((noipa)) int *addr (int *p) { return &*p; }
__attribute__((noipa)) int *addr0 (int *p) { return &p[0]; }
__attribute__((noipa)) void disc (int *p) { *p; }
__attribute__((noipa)) void disc_void (int *p) { (void) *p; }
__attribute__((noipa)) int sfc (S *sp) { return sp->sf (); }
__attribute__((noipa)) int sfc_star (S *sp) { return (*sp).sf (); }
__attribute__((noipa)) int sk (S *sp) { return sp->k; }
__attribute__((noipa)) int *sk_addr (S *sp) { return &sp->k; }
__attribute__((noipa)) int &bind (int *p) { return *p; }
using F = int ();
__attribute__((noipa)) F *faddr (F *fp) { return &*fp; }
__attribute__((noipa)) void fdisc (F *fp) { *fp; }
constexpr int sfc_constexpr (S *sp) { return sp->sf (); }
template <class T> __attribute__((noipa)) T *taddr (T *p) { return &*p; }
constexpr int *id (int *p) { return &*p; }
constexpr int g = 0;
constexpr S gs {};
static_assert (id (const_cast<int *> (&g)) == &g);
static_assert (sfc_constexpr (const_cast<S *> (&gs)) == 3);

#define COUNT(name, ...)					\
  do {								\
    int before = reports;					\
    __VA_ARGS__;						\
    std::printf (name " %d\n", reports - before);		\
  } while (0)

int
main ()
{
  int *volatile vp = nullptr;
  S *volatile vsp = nullptr;
  int *p = vp;
  S *sp = vsp;
  COUNT ("bind", bind (p));
  COUNT ("addr", addr (p));
  COUNT ("addr0", addr0 (p));
  COUNT ("disc", disc (p));
  COUNT ("disc_void", disc_void (p));
  COUNT ("sfc", sfc (sp));
  COUNT ("sfc_star", sfc_star (sp));
  COUNT ("sk", sk (sp));
  COUNT ("sk_addr", sk_addr (sp));
  COUNT ("sfc_constexpr", int r = sfc_constexpr (nullptr); (void) r);
  COUNT ("taddr", taddr (p));
  F *volatile vfp = nullptr;
  F *fp = vfp;
  COUNT ("faddr", faddr (fp));
  COUNT ("fdisc", fdisc (fp));
  int x = 0;
  S s {};
  COUNT ("nonnull", addr (&x); disc (&x); sfc (&s); sk (&s); taddr (&x);
	 int r2 = sfc_constexpr (&s); (void) r2);
  return 0;
}
// { dg-output "bind 1(\n|\r\n|\r)" }
// { dg-output "addr 1(\n|\r\n|\r)" }
// { dg-output "addr0 1(\n|\r\n|\r)" }
// { dg-output "disc 1(\n|\r\n|\r)" }
// { dg-output "disc_void 1(\n|\r\n|\r)" }
// { dg-output "sfc 1(\n|\r\n|\r)" }
// { dg-output "sfc_star 1(\n|\r\n|\r)" }
// { dg-output "sk 1(\n|\r\n|\r)" }
// { dg-output "sk_addr 1(\n|\r\n|\r)" }
// { dg-output "sfc_constexpr 1(\n|\r\n|\r)" }
// { dg-output "taddr 1(\n|\r\n|\r)" }
// { dg-output "faddr 1(\n|\r\n|\r)" }
// { dg-output "fdisc 1(\n|\r\n|\r)" }
// { dg-output "nonnull 0(\n|\r\n|\r)" }
