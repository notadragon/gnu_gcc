// D4405 ("The Analysis Evaluation"): a temporary's destructor call, whose
// analysis evaluations wait in its object argument until it is known
// whether the temporary is elided (GCC-944), still destroys the right
// object, once, and only when the temporary is materialized -- with a
// caller-side wrapper (-fcontracts-client-check=all) called in place of the
// destructor, and at namespace scope.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -fcontracts-client-check=all -fcontract-evaluation-semantic=enforce" }

int g = 1;
static int ctors, dtors;
static void *last_destroyed;

struct T {
  T *self;
  T () : self (this) { ++ctors; }
  T (const T &) = delete;
  ~T () pre (g > 0) pre (self == this)
  {
    ++dtors;
    last_destroyed = this;
  }
};

T make () { return T (); }
void take (T t) { if (t.self != &t) __builtin_abort (); }

T nx = T ();
T ny = make ();
T *np = new T (T ());
int nz = (T (), 1);
static int ns_ctors = ctors, ns_dtors = dtors;

T ret () { return g ? T () : make (); }

int
main ()
{
  if (ns_ctors != 4 || ns_dtors != 1
      || nx.self != &nx || ny.self != &ny || np->self != np)
    __builtin_abort ();
  ctors = dtors = 0;
  {
    T a = T ();
    T b = make ();
    T c = ret ();
    if (a.self != &a || b.self != &b || c.self != &c || dtors != 0)
      __builtin_abort ();
  }
  if (ctors != 3 || dtors != 3)
    __builtin_abort ();
  ctors = dtors = 0;
  int i = (T (), 1);
  if (i != 1 || ctors != 1 || dtors != 1)
    __builtin_abort ();
  take (T ());
  take (make ());
  if (ctors != 3 || dtors != 3)
    __builtin_abort ();
  T *p = new T (make ());
  if (p->self != p || dtors != 3)
    __builtin_abort ();
  delete p;
  if (dtors != 4 || last_destroyed != p)
    __builtin_abort ();
}
