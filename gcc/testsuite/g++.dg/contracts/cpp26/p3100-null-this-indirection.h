// A member function called through a null pointer that indirects through
// `this' without reading or writing the object, for the
// p3100-null-this-indirection*.C tests: each run prints the number of
// implicit-assertion reports of one form.  A fault after the report is
// caught as SIGSEGV and resumed with siglongjmp, so one run counts the
// reports of every form.  The read and write forms are in
// p3100-null-this-read.h.

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
struct B0 { int b; };
struct T;
NI void take (T &) {}
struct T : B0
{
  int a;
  static int s;
  NI int g () { return 0; }
  // References bound to `*this' and to a member, member addresses.
  NI int bind () { T &r = *this; (void) r; return 0; }
  NI int cbind () { const T &r = *this; (void) r; return 0; }
  NI int bbind () { B0 &r = *this; (void) r; return 0; }
  NI int targ () { take (*this); return 0; }
  NI int mbind () { int &r = this->a; (void) r; return 0; }
  NI int *maddr () { return &this->a; }
  NI int *daddr () { return &(*this).a; }
  NI int dmbind () { int &r = (*this).a; (void) r; return 0; }
  NI int *pmaddr (int T::*p) { return &(this->*p); }
  NI int *pmdaddr (int T::*p) { return &((*this).*p); }
  NI int *sub0addr () { return &this[0].a; }
  // Member functions called on `*this'.
  NI int tcall () { return this->g (); }
  NI int icall () { return g (); }
  NI int dcall () { return (*this).g (); }
  // In a lambda capturing `this'.
  NI int lamcall () { return [this] { return g (); } (); }
  NI int *lamaddr () { return [this] { return &a; } (); }
  NI int lambind () { return [this] { T &r = *this; (void) r; return 0; } (); }
  // Through a non-zero subscript of `this' (DECISIONS.md E108 (a)).
  NI int *sub1addr () { return &this[1].a; }
  NI int *subiaddr (int i) { return &this[i].a; }
  NI T *sub1p () { return &this[1]; }
  NI int sub1bind () { T &r = this[1]; (void) r; return 0; }
  NI int sub1call () { return this[1].g (); }
  NI int lamsub1call () { return [this] { return this[1].g (); } (); }
  // Neither the address `&*this', a discarded `*this' nor a static member
  // named through it indirects through anything checked.
  NI T *addr () { return &*this; }
  NI void disc () { *this; }
  NI int sstat () { return (*this).s; }
  NI T *lamthisaddr () { return [this] { return &*this; } (); }
  NI void lamdisc () { [this] { *this; } (); }
};
int T::s;

// A class with a non-trivial copy constructor: its reference parameter
// is bound to `*this'.
struct N
{
  int n;
  N () : n (0) {}
  N (const N &o) : n (o.n) {}
  NI int copy ();
  NI int lamcopy ();
};
int N::copy () { N n2 = *this; return n2.n; }
int N::lamcopy () { return [*this] { return n; } (); }

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
  N *np = reinterpret_cast<N *> (nv);
  T t[2]{};
  N n;
  COUNT ("g", tp->g ());
  COUNT ("bind", tp->bind ());
  COUNT ("cbind", tp->cbind ());
  COUNT ("bbind", tp->bbind ());
  COUNT ("targ", tp->targ ());
  COUNT ("mbind", tp->mbind ());
  COUNT ("maddr", tp->maddr ());
  COUNT ("daddr", tp->daddr ());
  COUNT ("dmbind", tp->dmbind ());
  COUNT ("pmaddr", tp->pmaddr (&T::a));
  COUNT ("pmdaddr", tp->pmdaddr (&T::a));
  COUNT ("sub0addr", tp->sub0addr ());
  COUNT ("tcall", tp->tcall ());
  COUNT ("icall", tp->icall ());
  COUNT ("dcall", tp->dcall ());
  COUNT ("lamcall", tp->lamcall ());
  COUNT ("lamaddr", tp->lamaddr ());
  COUNT ("lambind", tp->lambind ());
  COUNT ("sub1addr", tp->sub1addr ());
  COUNT ("subiaddr", tp->subiaddr (1));
  COUNT ("sub1p", tp->sub1p ());
  COUNT ("sub1bind", tp->sub1bind ());
  COUNT ("sub1call", tp->sub1call ());
  COUNT ("lamsub1call", tp->lamsub1call ());
  COUNT ("addr", tp->addr ());
  COUNT ("disc", tp->disc ());
  COUNT ("sstat", tp->sstat ());
  COUNT ("lamthisaddr", tp->lamthisaddr ());
  COUNT ("lamdisc", tp->lamdisc ());
  COUNT ("ncopy", np->copy ());
  COUNT ("nlamcopy", np->lamcopy ());
  COUNT ("nonnull", t[0].g (); t[0].bind (); t[0].cbind (); t[0].bbind ();
	 t[0].targ (); t[0].mbind (); t[0].maddr (); t[0].daddr ();
	 t[0].dmbind (); t[0].pmaddr (&T::a); t[0].pmdaddr (&T::a);
	 t[0].sub0addr (); t[0].tcall (); t[0].icall (); t[0].dcall ();
	 t[0].lamcall (); t[0].lamaddr (); t[0].lambind (); t[0].sub1addr ();
	 t[0].subiaddr (1); t[0].sub1p (); t[0].sub1bind (); t[0].sub1call ();
	 t[0].lamsub1call (); t[0].addr (); t[0].disc (); t[0].sstat ();
	 t[0].lamthisaddr (); t[0].lamdisc (); n.copy (); n.lamcopy ());
  return 0;
}
