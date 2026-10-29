// D4405 ("The Analysis Evaluation"): a precondition's analysis evaluation
// at a call with no argument but the object -- a default constructor, a
// destructor, a function returning a class with no parameters -- is
// deferred to the proof shadow like any other, so a guarded call is proven
// when optimizing (GCC-919).  A constructor or destructor call whose object
// exists carries the evaluation in its object argument; a call initializing
// a temporary (a prvalue of class type, `T ()', `mk ()') carries it in the
// temporary's initializer, ahead of the call.  The same precondition on a
// function returning int is proven too, and unguarded calls still cannot
// be.  A [[nodiscard]] call carrying the evaluation is still found to be
// discarded.  An unproven evaluation is a premise for those after it, so the
// unguarded calls are in functions of their own.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

extern int g;
// A call (operator new, another element's constructor) may change g, so
// the calls that follow one in a new-expression, an array initialization
// or a delete-expression have preconditions on k, which no call changes.
extern const int k;

struct T {
  T () pre<always_prove> (g > 0);		// { dg-message "the predicate of the contract assertion" }
  int v = 0;
};
struct D {
  ~D () pre<always_prove> (g > 0);		// { dg-message "the predicate of the contract assertion" }
};
struct R { int v; };
R mk () pre<always_prove> (g > 0);		// { dg-message "the predicate of the contract assertion" }
int mi () pre<always_prove> (g > 0);		// { dg-message "the predicate of the contract assertion" }
struct M {
  R get () const pre<always_prove> (g > 0);	// { dg-message "the predicate of the contract assertion" }
};
R operator+ (R, R) pre<always_prove> (g > 0);	// { dg-message "the predicate of the contract assertion" }
[[nodiscard]] R nd () pre<always_prove> (g > 0);
struct TK {
  TK () pre<always_prove> (k > 0);		// { dg-message "the predicate of the contract assertion" }
  int v = 0;
};
struct DK {
  ~DK () pre<always_prove> (k > 0);		// { dg-message "the predicate of the contract assertion" }
};
struct H {
  T t;
  H (int) : t () {}				// { dg-error "contract assertion cannot be proven" }
};

T
ret ()
{
  if (g > 0)
    return T ();				// { dg-bogus "cannot be proven" }
  __builtin_unreachable ();
}

int
use (M &m, R a, R b)
{
  int r = 0;
  if (g > 0)
    T t;					// { dg-bogus "cannot be proven" }
  if (g > 0)
    {
      D d;					// { dg-bogus "cannot be proven" }
    }
  if (g > 0)
    r += mk ().v;				// { dg-bogus "cannot be proven" }
  if (g > 0)
    r += mi ();					// { dg-bogus "cannot be proven" }
  if (g > 0)
    r += T ().v;				// { dg-bogus "cannot be proven" }
  if (g > 0)
    r += T {}.v;				// { dg-bogus "cannot be proven" }
  if (k > 0)
    r += (new TK)->v;				// { dg-bogus "cannot be proven" }
  if (g > 0)
    r += m.get ().v;				// { dg-bogus "cannot be proven" }
  if (g > 0)
    r += (a + b).v;				// { dg-bogus "cannot be proven" }
  if (k > 0)
    {
      TK arr[2];				// { dg-bogus "cannot be proven" }
      r += arr[1].v;
    }
  if (g > 0)
    {
      R x = mk ();				// { dg-bogus "cannot be proven" }
      r += x.v;
    }
  if (g > 0)
    {
      T y = T ();				// { dg-bogus "cannot be proven" }
      r += y.v;
    }
  if (k > 0)
    delete new DK;				// { dg-bogus "cannot be proven" }
  if (g > 0)
    nd ();					// { dg-warning "ignoring return value" }
  return r;
}

void u1 () { T t; }				// { dg-error "contract assertion cannot be proven" }
void u2 () { D d; }				// { dg-error "contract assertion cannot be proven" }
int u3 () { return mk ().v; }			// { dg-error "contract assertion cannot be proven" }
int u4 () { return T ().v; }			// { dg-error "contract assertion cannot be proven" }
int u5 (M &m) { return m.get ().v; }		// { dg-error "contract assertion cannot be proven" }
int u6 (R a, R b) { return (a + b).v; }		// { dg-error "contract assertion cannot be proven" }
int u7 () { return mi (); }			// { dg-error "contract assertion cannot be proven" }
int u8 () { return (new TK)->v; }		// { dg-error "contract assertion cannot be proven" }
void u9 () { delete new DK; }			// { dg-error "contract assertion cannot be proven" }
