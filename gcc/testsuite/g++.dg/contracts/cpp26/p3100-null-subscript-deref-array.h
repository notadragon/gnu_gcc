// A subscript of the array a pointer to an array points to, `(*pa)[i]'
// with `pa' null -- bound to a reference, read, stored to, its address
// taken; through `**ppa' and a two-dimensional array -- for the
// p3100-null-subscript-deref-array*.C tests: each run prints the number of
// implicit-assertion reports of one form.  The mirror of
// llvm_llvm-project's Runnable/p3100-null-subscript-deref-array.cpp
// (CLANG-813).  Controls: `pa[0][1]', `&pa[0][1]' (a subscript of the
// pointer) and a non-null `pa'.  A read faults after its report (caught).

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
NI int *bind1 (int (*pa)[4]) { int &r = (*pa)[1]; return &r; }
NI int *bindi (int (*pa)[4], int i) { int &r = (*pa)[i]; return &r; }
NI int *bindp (int (*pa)[4]) { int &r = (*(pa))[2]; return &r; }
NI int read1 (int (*pa)[4]) { return (*pa)[1]; }
NI int readi (int (*pa)[4], int i) { return (*pa)[i]; }
NI int read0 (int (*pa)[4]) { return (*pa)[0]; }
NI void store1 (int (*pa)[4]) { (*pa)[1] = 2; }
NI int *addr1 (int (*pa)[4]) { return &(*pa)[1]; }
NI int dd (int (**ppa)[4]) { return (**ppa)[1]; }
NI int md (int (*pm)[2][2]) { return (*pm)[1][1]; }
NI int sub01 (int (*pa)[4]) { return pa[0][1]; }
NI int *sub01a (int (*pa)[4]) { return &pa[0][1]; }

#define COUNT(name, ...)						\
  do									\
    {									\
      int before = reports;						\
      if (!sigsetjmp (env, 1))						\
	{								\
	  __VA_ARGS__;							\
	}								\
      std::printf (name " %d\n", reports - before);			\
    }									\
  while (0)

int
main ()
{
  std::signal (SIGSEGV, on_segv);
  int (*volatile nv)[4] = nullptr;
  int (*volatile nm)[2][2] = nullptr;
  int (*pa)[4] = nv;
  int a[4] = {}, m[2][2] = {};
  int (*ap)[4] = &a;
  COUNT ("bind1", bind1 (nv));
  COUNT ("bindi", bindi (nv, 1));
  COUNT ("bindp", bindp (nv));
  COUNT ("read1", read1 (nv));
  COUNT ("readi", readi (nv, 1));
  COUNT ("read0", read0 (nv));
  COUNT ("store1", store1 (nv));
  COUNT ("addr1", addr1 (nv));
  COUNT ("dd", dd (&pa));
  COUNT ("md", md (nm));
  COUNT ("sub01", sub01 (nv));
  COUNT ("sub01a", sub01a (nv));
  COUNT ("nonnull", bind1 (&a); bindi (&a, 1); bindp (&a); read1 (&a);
	 readi (&a, 1); read0 (&a); store1 (&a); addr1 (&a); dd (&ap);
	 md (&m); sub01 (&a); sub01a (&a));
  return 0;
}
