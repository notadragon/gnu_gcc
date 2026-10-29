// Accesses through a pointer an explicit cast formed from a character
// array, misaligned by the offset -- a load, a store, a member read, a
// class copy, a subscript of the cast pointer, `*(int *) &buf[o]' -- for
// the p3100-align-char-array-cast*.C tests: each run prints the number of
// implicit-assertion reports of one form.  The mirror of
// llvm_llvm-project's Runnable/p3100-align-char-array-cast.cpp
// (CLANG-810).  Controls: the same through a `char *' parameter, a
// reference binding, the aligned offset, and packed members, whose
// reduced alignment is not a cast's (0).

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation (const cs::contract_violation &v)
{
  if (v.kind () == cs::assertion_kind::implicit)
    ++reports;
}

#define NI __attribute__((noinline))
alignas (8) char buf[32];
struct P { int a, b; };
struct __attribute__((packed)) Q { char c; int i; };
Q q;
NI int gload (int o) { return *(int *) (buf + o); }
NI int lload (int o)
{
  alignas (8) char lb[16] = {};
  return *(int *) (lb + o);
}
NI void gstore (int o) { *(int *) (buf + o) = 1; }
NI int gmem (int o) { return ((P *) (buf + o))->a; }
NI int gmemb (int o) { return ((P *) (buf + o))->b; }
NI int gmemd (int o) { return (*(P *) (buf + o)).a; }
NI P gcopy (int o) { return *(P *) (buf + o); }
NI int gsub (int o) { return *(int *) &buf[o]; }
NI int gidx0 (int o) { return ((int *) (buf + o))[0]; }
NI int gidx (int o, int i) { return ((int *) (buf + o))[i]; }
NI int gsum2 (int o) { return *(int *) (buf + o + 4); }
NI long iarr (int o)
{
  static int ia[8];
  return *(long *) (ia + o);
}
// Controls.
NI int pload (char *c, int o) { return *(int *) (c + o); }
NI int pmem (char *c, int o) { return ((P *) (c + o))->a; }
NI bool gref (int o)
{
  int &r = *(int *) (buf + o);
  return &r == nullptr;
}
NI int qpacked () { return q.i; }
NI int qpackedp (Q *p) { return p->i; }
NI int qcast () { return *(int *) &q.i; }

#define COUNT(name, ...)						\
  do									\
    {									\
      int before = reports;						\
      __VA_ARGS__;							\
      std::printf (name " %d\n", reports - before);			\
    }									\
  while (0)

int
main ()
{
  int volatile one = 1, zero = 0;
  COUNT ("gload", gload (one));
  COUNT ("lload", lload (one));
  COUNT ("gstore", gstore (one));
  COUNT ("gmem", gmem (one));
  COUNT ("gmemb", gmemb (one));
  COUNT ("gmemd", gmemd (one));
  COUNT ("gcopy", gcopy (one));
  COUNT ("gsub", gsub (one));
  COUNT ("gidx0", gidx0 (one));
  COUNT ("gidx", gidx (one, zero));
  COUNT ("gsum2", gsum2 (one));
  COUNT ("iarr", iarr (one));
  COUNT ("pload", pload (buf, one));
  COUNT ("pmem", pmem (buf, one));
  COUNT ("gref", gref (one));
  COUNT ("packed", qpacked (); qpackedp (&q); qcast ());
  COUNT ("aligned", gload (zero); lload (zero); gstore (zero); gmem (zero);
	 gmemb (zero); gmemd (zero); gcopy (zero); gsub (zero); gidx0 (zero);
	 gidx (zero, zero); gsum2 (zero); iarr (zero));
  return 0;
}
