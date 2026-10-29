// An indirection through a chain of pointer arithmetic with constants on a
// null pointer, `*(p + 1 + 2)', and through pointer arithmetic with an
// offset that is not a constant, for the p3100-null-pointer-arith-chain*.C
// tests: each run prints the number of implicit-assertion reports of one
// form.  A fault after the report is caught as SIGSEGV and resumed with
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

struct S { int a, b; void f () {} };

#define NI __attribute__((noinline))
constexpr int k = 1;
NI int iread (int *ip) { return *(ip + 1 + 2); }
NI int isub (int *ip) { return (ip + 1)[0]; }
NI int iread_k (int *ip) { return *(ip + k + 1); }
NI void istore (int *ip) { *(ip + 1 + 2) = 1; }
NI void isubstore (int *ip) { (ip + 1)[1] = 1; }
NI int mread (S *sp) { return (sp + 1 + 2)->a; }
NI void mstore (S *sp) { (sp + 1 + 2)->a = 1; }
NI int msub (S *sp) { return (sp + 1)[1].a; }
NI int *maddr (S *sp) { return &(sp + 1 + 2)->a; }
// Clang's: an offset that is not a constant.
NI int inc (int *ip, int i) { return *(ip + i); }
NI void incst (int *ip, int i) { *(ip + i) = 1; }
NI int mnc (S *sp, int i) { return (sp + i)->a; }
NI void mncst (S *sp, int i) { (sp + i)->a = 1; }
NI int *maddrnc (S *sp, int i) { return &(sp + i)->a; }
NI void bindnc (S *sp, int i) { S &r = *(sp + i); (void) r; }
NI void copync (S *sp, int i) { S s2 = *(sp + i); (void) s2; }
NI void callnc (S *sp, int i) { (sp + i)->f (); }
NI int pmnc (S *sp, int i, int S::*p) { return (sp + i)->*p; }
NI int revnc (int *ip, int i) { return *(i + ip); }
NI int negnc (int *ip, int i) { return *(ip - i); }
NI int subnc (int *ip, int i) { return (ip + 1)[i]; }
NI int chainnc (int *ip, int i) { return *(ip + i + 1); }
NI int chainnc2 (int *ip, int i) { return *(ip + 1 + i); }
NI int chainneg (int *ip, int i) { return *(ip + i - 1); }
NI void chainbind (S *sp, int i) { S &r = *(sp + 1 + i); (void) r; }
NI void chaincall (S *sp, int i) { (sp + 1 + i)->f (); }
// GCC's: more uses of a chain with constants.
NI void cbind (S *sp) { S &r = *(sp + 1 + 2); (void) r; }
NI void ccopy (S *sp) { S s2 = *(sp + 1 + 2); (void) s2; }
NI void ccall (S *sp) { (sp + 1 + 2)->f (); }
NI int cpm (S *sp, int S::*p) { return (sp + 1 + 2)->*p; }
NI void cdisc (S *sp) { (void) *(sp + 1 + 2); }
NI int cneg (int *ip) { return *(ip - 1 - 2); }
NI int csubm (S *sp) { return (sp + 1)[1].b; }
NI int *caddrsub (S *sp) { return &(sp + 1)[1].a; }

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
  S sa[4]{};
  int ia[4]{};
  volatile int one = 1, two = 2;
  COUNT ("iread", iread (ip));
  COUNT ("isub", isub (ip));
  COUNT ("iread_k", iread_k (ip));
  COUNT ("istore", istore (ip));
  COUNT ("isubstore", isubstore (ip));
  COUNT ("mread", mread (sp));
  COUNT ("mstore", mstore (sp));
  COUNT ("msub", msub (sp));
  COUNT ("maddr", maddr (sp));
  COUNT ("nonnull", iread (ia); isub (ia); iread_k (ia); istore (ia);
	 isubstore (ia); mread (sa); mstore (sa); msub (sa); maddr (sa));
  COUNT ("inc", inc (ip, one));
  COUNT ("incst", incst (ip, one));
  COUNT ("mnc", mnc (sp, one));
  COUNT ("mncst", mncst (sp, one));
  COUNT ("maddrnc", maddrnc (sp, one));
  COUNT ("bindnc", bindnc (sp, one));
  COUNT ("copync", copync (sp, one));
  COUNT ("callnc", callnc (sp, one));
  COUNT ("pmnc", pmnc (sp, one, &S::b));
  COUNT ("revnc", revnc (ip, one));
  COUNT ("negnc", negnc (ip, one));
  COUNT ("subnc", subnc (ip, one));
  COUNT ("chainnc", chainnc (ip, one));
  COUNT ("chainnc2", chainnc2 (ip, one));
  COUNT ("chainneg", chainneg (ip, two));
  COUNT ("chainbind", chainbind (sp, one));
  COUNT ("chaincall", chaincall (sp, one));
  COUNT ("nonnull_clang", inc (ia, one); incst (ia, one); mnc (sa, one);
	 mncst (sa, one); maddrnc (sa, one); bindnc (sa, one);
	 copync (sa, one); callnc (sa, one); pmnc (sa, one, &S::b);
	 revnc (ia, one); negnc (ia + 1, one); subnc (ia, one);
	 chainnc (ia, one); chainnc2 (ia, one); chainneg (ia, two);
	 chainbind (sa, one); chaincall (sa, one));
  COUNT ("cbind", cbind (sp));
  COUNT ("ccopy", ccopy (sp));
  COUNT ("ccall", ccall (sp));
  COUNT ("cpm", cpm (sp, &S::b));
  COUNT ("cdisc", cdisc (sp));
  COUNT ("cneg", cneg (ip));
  COUNT ("csubm", csubm (sp));
  COUNT ("caddrsub", caddrsub (sp));
  COUNT ("nonnull_gcc", cbind (sa); ccopy (sa); ccall (sa); cpm (sa, &S::b);
	 cdisc (sa); cneg (ia + 3); csubm (sa); caddrsub (sa));
  return 0;
}
