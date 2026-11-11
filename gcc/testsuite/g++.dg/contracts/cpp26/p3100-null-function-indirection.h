// An indirection through a null pointer to function, `*fp', called or bound
// to a reference, for the p3100-null-function-indirection*.C tests: each run
// prints the number of implicit-assertion reports of one form.  A call
// through the null pointer faults after the handler returns; the fault is
// caught as SIGSEGV and resumed with siglongjmp, so one run counts the
// reports of every form.

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

using F = int ();
using G = long ();
int fn () { return 7; }

#define NI __attribute__((noipa))
// A call through `*fp'.
NI int call (F *fp) { return (*fp) (); }
NI int call_parens (F *fp) { return (*(fp)) (); }
NI int call_twice (F **fpp) { return (**fpp) (); }
template <class T> NI int tcall (T *fp) { return (*fp) (); }
NI int call_cast (F *fp) { return static_cast<F &> (*fp) (); }
NI int call_ccast (F *fp) { return ((F &) *fp) (); }
NI int call_rvcast (F *fp) { return static_cast<F &&> (*fp) (); }
NI int call_cond (F *fp, F *gp, bool b) { return (b ? *fp : *gp) (); }
NI int call_comma (F *fp) { return ((void) 0, *fp) (); }
// A reference bound to `*fp'.
NI int bind (F *fp)
{
  F &r = *fp;
  return &r != nullptr;
}
NI F &bind_ret (F *fp) { return *fp; }
NI int take (F &) { return 1; }
NI int bind_arg (F *fp) { return take (*fp); }
NI int bind_cast (F *fp)
{
  F &r = static_cast<F &> (*fp);
  return &r != nullptr;
}
NI int bind_rv (F *fp)
{
  F &&r = static_cast<F &&> (*fp);
  return &r != nullptr;
}
NI int bind_cond (F *fp, F *gp, bool b)
{
  F &r = b ? *fp : *gp;
  return &r != nullptr;
}
NI int bind_comma (F *fp)
{
  F &r = ((void) 0, *fp);
  return &r != nullptr;
}
// Through `reinterpret_cast<F &> (*gp)', bound or called: checked here
// (the reference conversion of `gp' is checked like any other), not by
// Clang; nor are the conditional and comma forms above.
NI int bind_rcast (G *gp)
{
  F &r = reinterpret_cast<F &> (*gp);
  return &r != nullptr;
}
NI int call_rcast (G *gp) { return reinterpret_cast<F &> (*gp) (); }
// Not checked: a plain call `fp ()', which names no indirection.
NI int call_plain (F *fp) { return fp (); }
// Controls: a function named directly, a reference to a function (also
// cast, or returned by a call), a conditional choosing a named function.
NI int call_named () { return (*fn) (); }
NI int bind_named ()
{
  F &r = fn;
  return r ();
}
NI int bind_ref (F &r)
{
  F &r2 = r;
  return r2 ();
}
NI int call_cond_named (F *fp, bool b) { return (b ? fn : *fp) (); }
NI int call_ref_cast (F &r) { return static_cast<F &> (r) (); }
NI F &id (F &r) { return r; }
NI int call_returned (F &r) { return id (r) (); }

#define COUNT(name, ...)					\
  do {								\
    int before = reports;					\
    if (!sigsetjmp (env, 1))					\
      {								\
	__VA_ARGS__;						\
      }								\
    std::printf (name " %d\n", reports - before);		\
  } while (0)

int
main ()
{
  std::signal (SIGSEGV, on_segv);
  F *volatile vfp = nullptr;
  F *fp = vfp;
  G *gp = reinterpret_cast<G *> (fp);
  F *nfp = fn;
  G *ngp = reinterpret_cast<G *> (nfp);
  COUNT ("call", call (fp));
  COUNT ("call_parens", call_parens (fp));
  COUNT ("call_twice", call_twice (&fp));
  COUNT ("tcall", tcall (fp));
  COUNT ("call_cast", call_cast (fp));
  COUNT ("call_ccast", call_ccast (fp));
  COUNT ("call_rvcast", call_rvcast (fp));
  COUNT ("call_cond", call_cond (nfp, fp, false));
  COUNT ("call_comma", call_comma (fp));
  COUNT ("bind", bind (fp));
  COUNT ("bind_ret", (void) &bind_ret (fp));
  COUNT ("bind_arg", bind_arg (fp));
  COUNT ("bind_cast", bind_cast (fp));
  COUNT ("bind_rv", bind_rv (fp));
  COUNT ("bind_cond", bind_cond (nfp, fp, false));
  COUNT ("bind_comma", bind_comma (fp));
  COUNT ("bind_rcast", bind_rcast (gp));
  COUNT ("call_rcast", call_rcast (gp));
  COUNT ("call_plain", call_plain (fp));
  COUNT ("nonnull", call (nfp); call_parens (nfp); call_twice (&nfp);
	 tcall (nfp); call_cast (nfp); call_ccast (nfp); call_rvcast (nfp);
	 bind (nfp); (void) &bind_ret (nfp); bind_arg (nfp); bind_cast (nfp);
	 bind_rv (nfp); bind_rcast (ngp); call_rcast (ngp); call_plain (nfp);
	 call_cond (nfp, fp, true); call_comma (nfp); bind_cond (nfp, fp, true);
	 bind_comma (nfp); call_named (); bind_named (); bind_ref (fn);
	 call_cond_named (fp, true); call_ref_cast (fn); call_returned (fn));
  return 0;
}
