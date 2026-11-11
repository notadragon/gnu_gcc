// The element at a subscript of a null pointer that is a constant
// variable, `p[k]', for the p3100-null-constant-subscript-variable*.C
// tests: each run prints the number of implicit-assertion reports of one
// form.  A fault after the report is caught as SIGSEGV and resumed with
// siglongjmp, so one run counts the reports of every form.  The literal
// subscripts are in p3100-null-constant-subscript.h.

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

struct S { int a, b; void f () {} };

#define NI __attribute__((noinline))
constexpr int k = 1;
const int kc = 1;
NI int iread (int *ip) { return ip[k]; }
NI void istore (int *ip) { ip[k] = 2; }
NI int mread (S *sp) { return sp[k].a; }
NI void mstore (S *sp) { sp[k].a = 1; }
NI int *maddr (S *sp) { return &sp[k].a; }
NI S *saddr (S *sp) { return &sp[k]; }
NI void mbind (S *sp) { int &r = sp[k].a; (void) r; }
NI void sbind (S *sp) { S &r = sp[k]; (void) r; }
NI void scopy (S *sp) { S s2 = sp[k]; (void) s2; }
NI void scall (S *sp) { sp[k].f (); }
NI int pm (S *sp, int S::*p) { return sp[k].*p; }
NI int iread_kc (int *ip) { return ip[kc]; }
NI int mread_kc (S *sp) { return sp[kc].a; }
NI int mread_rev (S *sp) { return k[sp].a; }
NI int mread_expr (S *sp) { return sp[k + 1].a; }

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
  int *ip = nv;
  S sa[3]{};
  int ia[3]{};
  COUNT ("iread", iread (ip));
  COUNT ("istore", istore (ip));
  COUNT ("mread", mread (sp));
  COUNT ("mstore", mstore (sp));
  COUNT ("maddr", maddr (sp));
  COUNT ("saddr", saddr (sp));
  COUNT ("mbind", mbind (sp));
  COUNT ("sbind", sbind (sp));
  COUNT ("scopy", scopy (sp));
  COUNT ("scall", scall (sp));
  COUNT ("pm", pm (sp, &S::b));
  COUNT ("iread_kc", iread_kc (ip));
  COUNT ("mread_kc", mread_kc (sp));
  COUNT ("mread_rev", mread_rev (sp));
  COUNT ("mread_expr", mread_expr (sp));
  COUNT ("nonnull", iread (ia); istore (ia); mread (sa); mstore (sa);
	 maddr (sa); saddr (sa); mbind (sa); sbind (sa); scopy (sa);
	 scall (sa); pm (sa, &S::b); iread_kc (ia); mread_kc (sa);
	 mread_rev (sa); mread_expr (sa));
  return 0;
}
