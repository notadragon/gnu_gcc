// A class copied from, or stored to, the element one past the end of an
// array of known bound, for the p3100-array-bounds-copy*.C tests: each run
// prints the number of implicit-assertion reports of one form.  The forms
// of Clang's Runnable/p3100-subscript-copy-bounds.cpp, in its order; then
// GCC's extra empty forms.

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation (const cs::contract_violation &v)
{
  if (v.kind () == cs::assertion_kind::implicit)
    ++reports;
}

struct E {};
struct S { int a, b; };
struct EB {};
struct D : EB { int x; };

#define NI __attribute__((noinline))
S sa[2];
E ea[2];
D da[2];
int ia[2];
NI void sbyval (S) {}
NI void ebyval (E) {}
NI void scopy (int i) { S s2 = sa[i]; (void) s2; }
NI void sfrom (int i, S &s) { s = sa[i]; }
NI void sarg (int i) { sbyval (sa[i]); }
NI void sstore (int i, S &s) { sa[i] = s; }
NI int iread (int i) { return ia[i]; }
NI void ecopy (int i) { E e2 = ea[i]; (void) e2; }
NI void estore (int i, E &e) { ea[i] = e; }
NI void eparenstore (int i, E &e) { (ea[i]) = e; }
NI void bcopy (int i) { EB b = da[i]; (void) b; }
NI void bstore (int i, EB &b) { static_cast<EB &> (da[i]) = b; }
NI void earithcopy (int i) { E e2 = (ea + 1)[i]; (void) e2; }
NI void earithstore (int i, E &e) { (ea + 1)[i] = e; }
// GCC's: an empty element copied by value, returned, moved, and stored to
// by a move or an explicit call.
NI void earg (int i) { ebyval (ea[i]); }
NI E eret (int i) { return ea[i]; }
NI void emove (int i) { E e2 = static_cast<E &&> (ea[i]); (void) e2; }
NI void emovestore (int i, E &e) { ea[i] = static_cast<E &&> (e); }
NI void ecall (int i, E &e) { ea[i].operator= (e); }

#define COUNT(name, ...)					\
  do								\
    {								\
      int before = reports;					\
      __VA_ARGS__;						\
      std::printf (name " %d\n", reports - before);		\
    }								\
  while (0)

int main ()
{
  volatile int one = 1, two = 2;
  volatile int three = 3;
  S s{};
  E e;
  EB b;
  COUNT ("scopy", scopy (two));
  COUNT ("sfrom", sfrom (two, s));
  COUNT ("sarg", sarg (two));
  COUNT ("sstore", sstore (two, s));
  COUNT ("iread", iread (two));
  COUNT ("ecopy", ecopy (two));
  COUNT ("estore", estore (two, e));
  COUNT ("eparenstore", eparenstore (two, e));
  COUNT ("bcopy", bcopy (two));
  COUNT ("bstore", bstore (two, b));
  COUNT ("earithcopy", earithcopy (one));
  COUNT ("earithstore", earithstore (one, e));
  COUNT ("inrange", scopy (one); sfrom (one, s); sarg (one); sstore (one, s);
	 iread (one); ecopy (one); estore (one, e); eparenstore (one, e);
	 bcopy (one); bstore (one, b); earithcopy (0); earithstore (0, e));
  COUNT ("earg", earg (two));
  COUNT ("eret", eret (two));
  COUNT ("emove", emove (two));
  COUNT ("emovestore", emovestore (two, e));
  COUNT ("ecall", ecall (two, e));
  COUNT ("ecopy3", ecopy (three));
  COUNT ("estore3", estore (three, e));
  COUNT ("inrange_extra", earg (one); eret (one); emove (one);
	 emovestore (one, e); ecall (one, e));
  return 0;
}
