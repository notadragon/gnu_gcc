// A member function called through a null pointer that reads or writes a
// member through `this', for the p3100-null-this-read*.C tests: each run
// prints the number of implicit-assertion reports of one form.  A fault
// after the report is caught as SIGSEGV and resumed with siglongjmp, so one
// run counts the reports of every form.  The forms up to `nonnull' are
// mirrored, in this order, by llvm_llvm-project's
// Runnable/p3100-null-this-read.cpp; the forms after it up to
// `nonnull_extra' are Clang's, and the forms after that GCC's: up to
// `nonnull_gcc' through a pointer to member, then through a non-zero
// subscript of `this' (DECISIONS.md E108 (a)).  The indirections through
// `this' that read or write nothing are in p3100-null-this-indirection.h.

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
#include <utility>
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
struct T;
NI int byvalue (T t);
struct T
{
  int a;
  int arr[2];
  struct { int x; } s;
  NI int g ();
  NI void st ();
  NI int ex () { return this->a; }
  NI int dr () { return (*this).a; }
  NI int ar () { return arr[1]; }
  NI int sx () { return s.x; }
  NI int lam () { return [this] { return a; } (); }
  NI int lamcopy () { return [*this] { return a; } (); }
  NI int lamref () { return [&] { return a; } (); }
  NI int copy () { T t2 = *this; return t2.a; }
  NI static int stat (T *tp) { return tp->a; }
  // Clang's.
  NI void dst () { (*this).a = 1; }
  NI void lamst () { [this] { a = 1; } (); }
  NI void lamrefst () { [&] { a = 1; } (); }
  NI void lamdr () { [this] { (*this).a = 1; } (); }
  NI int lamthiscopy () { return [this] { T t2 = *this; return t2.a; } (); }
  NI void assignto (T &o) { o = *this; }
  NI void assignfrom (const T &o) { *this = o; }
  NI int arrdr () { return (*this).arr[1]; }
  NI int sdr () { return (*this).s.x; }
  NI int pmarrow (int T::*p) { return this->*p; }
  NI int pmdot (int T::*p) { return (*this).*p; }
  NI int movecopy () { T t2 = static_cast<T &&> (*this); return t2.a; }
  NI int stdmove () { T t2 = std::move (*this); return t2.a; }
  NI int byval () { return byvalue (*this); }
  NI int sub0 () { return this[0].a; }
  // GCC's: a store through `this->*pm' and `(*this).*pm', and a read
  // through each in a lambda capturing `this'.
  NI void pmstore (int T::*p) { this->*p = 1; }
  NI void pmstored (int T::*p) { (*this).*p = 1; }
  NI int lampm (int T::*p) { return [this, p] { return this->*p; } (); }
  NI int lampmd (int T::*p) { return [this, p] { return (*this).*p; } (); }
  // GCC's: a read or write through a non-zero subscript of `this'.
  NI int sub1 () { return this[1].a; }
  NI void sub1st () { this[1].a = 1; }
  NI int subi (int i) { return this[i].a; }
  NI void subist (int i) { this[i].a = 1; }
  NI int sub1arr () { return this[1].arr[1]; }
  NI int lamsub1 () { return [this] { return this[1].a; } (); }
  NI int sub1copy () { T t2 = this[1]; return t2.a; }
};
int T::g () { return a; }
void T::st () { a = 1; }
int byvalue (T t) { return t.a; }

struct B { int b; };
struct VB { int vb; };
struct U : B, virtual VB
{
  int u;
  NI int base () { return b; }
  NI int dbase () { return (*this).b; }
  NI int cbase () { return static_cast<B &> (*this).b; }
  NI int bcopy () { B b2 = *this; return b2.b; }
  NI int vbase () { return vb; }
  NI int dvbase () { return (*this).vb; }
  NI int cvbase () { return static_cast<VB &> (*this).vb; }
};

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
  T *tp = reinterpret_cast<T *> (nv);
  U *up = reinterpret_cast<U *> (nv);
  T t{}, t2{}, ta[2]{};
  U u;
  COUNT ("g", tp->g ());
  COUNT ("st", tp->st ());
  COUNT ("ex", tp->ex ());
  COUNT ("dr", tp->dr ());
  COUNT ("ar", tp->ar ());
  COUNT ("sx", tp->sx ());
  COUNT ("lam", tp->lam ());
  COUNT ("lamcopy", tp->lamcopy ());
  COUNT ("lamref", tp->lamref ());
  COUNT ("copy", tp->copy ());
  COUNT ("stat", T::stat (tp));
  COUNT ("nonnull", t.g (); t.st (); t.ex (); t.dr (); t.ar (); t.sx ();
	 t.lam (); t.lamcopy (); t.lamref (); t.copy (); T::stat (&t));
  COUNT ("dst", tp->dst ());
  COUNT ("lamst", tp->lamst ());
  COUNT ("lamrefst", tp->lamrefst ());
  COUNT ("lamdr", tp->lamdr ());
  COUNT ("lamthiscopy", tp->lamthiscopy ());
  COUNT ("assignto", tp->assignto (t2));
  COUNT ("assignfrom", tp->assignfrom (t2));
  COUNT ("arrdr", tp->arrdr ());
  COUNT ("sdr", tp->sdr ());
  COUNT ("pmarrow", tp->pmarrow (&T::a));
  COUNT ("pmdot", tp->pmdot (&T::a));
  COUNT ("movecopy", tp->movecopy ());
  COUNT ("stdmove", tp->stdmove ());
  COUNT ("byval", tp->byval ());
  COUNT ("sub0", tp->sub0 ());
  COUNT ("base", up->base ());
  COUNT ("dbase", up->dbase ());
  COUNT ("cbase", up->cbase ());
  COUNT ("bcopy", up->bcopy ());
  COUNT ("vbase", up->vbase ());
  COUNT ("dvbase", up->dvbase ());
  COUNT ("cvbase", up->cvbase ());
  COUNT ("nonnull_extra", t.dst (); t.lamst (); t.lamrefst (); t.lamdr ();
	 t.lamthiscopy (); t.assignto (t2); t.assignfrom (t2); t.arrdr ();
	 t.sdr (); t.pmarrow (&T::a); t.pmdot (&T::a); t.movecopy ();
	 t.stdmove (); t.byval (); t.sub0 (); u.base (); u.dbase ();
	 u.cbase (); u.bcopy (); u.vbase (); u.dvbase (); u.cvbase ());
  COUNT ("pmstore", tp->pmstore (&T::a));
  COUNT ("pmstored", tp->pmstored (&T::a));
  COUNT ("lampm", tp->lampm (&T::a));
  COUNT ("lampmd", tp->lampmd (&T::a));
  COUNT ("nonnull_gcc", t.pmstore (&T::a); t.pmstored (&T::a);
	 t.lampm (&T::a); t.lampmd (&T::a));
  COUNT ("sub1", tp->sub1 ());
  COUNT ("sub1st", tp->sub1st ());
  COUNT ("subi", tp->subi (1));
  COUNT ("subist", tp->subist (1));
  COUNT ("sub1arr", tp->sub1arr ());
  COUNT ("lamsub1", tp->lamsub1 ());
  COUNT ("sub1copy", tp->sub1copy ());
  COUNT ("nonnull_sub", ta[0].sub1 (); ta[0].sub1st (); ta[0].subi (1);
	 ta[0].subist (1); ta[0].sub1arr (); ta[0].lamsub1 ();
	 ta[0].sub1copy ());
  return 0;
}
