// D4405 ("The Analysis Evaluation"): a constructor precondition that names
// `this', analyzed at a call that makes a prvalue (`T (y)'), is given the
// slot of the temporary the call initializes (GCC-926) -- and the object an
// initialization that elides the temporary initializes instead: a
// variable, a returned object, an aggregate's member, an array element,
// a new-expression's object, a by-value parameter, a base subobject, either
// arm of a conditional, in a lambda.  Each guarded call is proven, and
// compiles; an unguarded one is not proven.  The constructor of a named
// return value, constructing the returned object, is proven too.
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

struct T {
  T (int y) pre<always_prove> (this != nullptr && y > 0);	// { dg-message "the predicate of the contract assertion" }
  T (const T &);
  ~T ();
  int v = 0;
};
struct A { T t; int k; };
struct D : T { D (int y) : T (T (y)) {} };	// { dg-error "contract assertion cannot be proven" }
void take (T);

T
ret (int y)
{
  if (y > 0)
    return T (y);				// { dg-bogus "cannot be proven" }
  return T (1);
}

T
nrvo ()
{
  T t (1);					// { dg-bogus "cannot be proven" }
  return t;
}

int
var (int y)
{
  if (y > 0)
    {
      T x = T (y);				// { dg-bogus "cannot be proven" }
      return x.v;
    }
  return 0;
}

int
agg (int y)
{
  if (y > 0)
    {
      A a{T (y), 1};				// { dg-bogus "cannot be proven" }
      return a.k;
    }
  return 0;
}

int
arr (int y)
{
  if (y > 0)
    {
      T a[] = {T (y), T (y)};			// { dg-bogus "cannot be proven" }
      return a[0].v;
    }
  return 0;
}

T *
nw (int y)
{
  if (y > 0)
    return new T (T (y));			// { dg-bogus "cannot be proven" }
  return nullptr;
}

void
arg (int y)
{
  if (y > 0)
    take (T (y));				// { dg-bogus "cannot be proven" }
}

int
base (int y)
{
  if (y > 0)
    {
      D d (y);
      return d.v;
    }
  return 0;
}

int
cond (int y, bool b)
{
  if (y > 0)
    {
      T x = b ? T (y) : T (y);			// { dg-bogus "cannot be proven" }
      return x.v;
    }
  return 0;
}

int
lam (int y)
{
  if (y > 0)
    return [&] { return T (y).v; } ();		// { dg-error "contract assertion cannot be proven" }
  return 0;
}

int
unguarded (int y)
{
  T x = T (y);					// { dg-error "contract assertion cannot be proven" }
  return x.v;
}
