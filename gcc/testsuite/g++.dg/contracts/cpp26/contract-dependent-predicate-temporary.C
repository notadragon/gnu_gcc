// A contract whose predicate is TYPE-DEPENDENT and nonetheless holds a
// temporary with a NON-TRIVIAL DESTRUCTOR.  GCC has always got this right;
// this is a guard mirrored from Clang (CLANG-6), where the combination
// corrupted the contract specifier and later segfaulted the front end.
//
// On Clang the dependent predicate meant the contract's condition was never
// finalized as a full-expression at parse time, so the "expression needs
// cleanups" flag the temporary had set was still live when the contract
// statement itself was finished.  The generic full-statement finalization
// then wrapped the contract in a statement-expression, and the wrapper was
// stored into the contract specifier through an unchecked cast.  Everything
// that later walked the contract list read garbage; the crash that surfaced
// it was rebuilding the contract for an out-of-line definition.
//
// Found by a contracts-enabled build of BDE, where the shape is an ordinary
// one: a member function template declared in class with a precondition over
// a chrono duration and defined out of line.
//
// Run test, not compile test, on purpose: on Clang a fix that merely stopped
// the crash but left a mis-typed node in the specifier would still compile,
// so what is pinned is that every predicate is really evaluated, against
// this call's arguments.  10 <= 5 is false and 10 <= 20 is true, so a
// dropped or misdirected predicate gives a different count.

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <cstdlib>

static int violations = 0;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

// The non-trivial destructor is the point; do not make it trivial.
struct Guard {
  long v;
  Guard (long x) : v (x) {}
  ~Guard () {}
};
bool operator<= (const Guard &a, long b) { return a.v <= b; }

// A trivially destructible counterpart, for the control below.
struct Plain {
  long v;
  Plain (long x) : v (x) {}
};
bool operator<= (const Plain &a, long b) { return a.v <= b; }

// --- the shape that broke Clang: in-class declaration, out-of-line
// --- definition without the contract

struct MemberOutOfLine {
  template <class T>
  void pre_only (const T &v) pre (Guard (10) <= v);

  // Return type T, so the result name is dependent too.
  template <class T>
  T post_only (const T &v) post (r : Guard (10) <= r);

  template <class T>
  T both (const T &v) pre (Guard (10) <= v) post (r : Guard (10) <= r);

  template <class T>
  void two_pre (const T &v) pre (Guard (10) <= v) pre (Guard (1) <= v);
};

template <class T>
void MemberOutOfLine::pre_only (const T &v) { (void) v; }

template <class T>
T MemberOutOfLine::post_only (const T &v) { return v; }

template <class T>
T MemberOutOfLine::both (const T &v) { return v; }

template <class T>
void MemberOutOfLine::two_pre (const T &v) { (void) v; }

// --- free function template: declaration then definition

template <class T>
void free_decl_then_def (const T &v) pre (Guard (10) <= v);

template <class T>
void free_decl_then_def (const T &v) { (void) v; }

// --- contract written on the definition itself

template <class T>
void on_definition (const T &v) pre (Guard (10) <= v) { (void) v; }

// --- contract_assert in a template body

template <class T>
void in_body (const T &v) { contract_assert (Guard (10) <= v); }

// --- controls that must keep working

struct TrivialTemp {
  template <class T>
  void f (const T &v) pre (Plain (10) <= v);
};
template <class T>
void TrivialTemp::f (const T &v) { (void) v; }

struct NonDependent {
  template <class T>
  void f (const T &, long w) pre (Guard (10) <= w);
};
template <class T>
void NonDependent::f (const T &, long w) { (void) w; }

struct NotATemplate {
  void f (long w) pre (Guard (10) <= w);
};
void NotATemplate::f (long w) { (void) w; }

static void
expect (int want, const char *what)
{
  if (violations != want)
    {
      std::printf ("FAIL: %s: expected %d violations, got %d\n", what, want,
		   violations);
      std::exit (1);
    }
}

int
main ()
{
  MemberOutOfLine m;

  violations = 0;
  m.pre_only (20);
  expect (0, "pre_only satisfied");
  m.pre_only (5);
  expect (1, "pre_only violated");

  violations = 0;
  m.post_only (20);
  expect (0, "post_only satisfied");
  m.post_only (5);
  expect (1, "post_only violated");

  violations = 0;
  m.both (20);
  expect (0, "both satisfied");
  m.both (5);
  expect (2, "both violated");

  violations = 0;
  m.two_pre (20);
  expect (0, "two_pre satisfied");
  m.two_pre (5);
  expect (1, "two_pre: only the 10 <= v precondition fails");
  m.two_pre (0);
  expect (3, "two_pre: both preconditions fail");

  violations = 0;
  free_decl_then_def (20);
  expect (0, "free_decl_then_def satisfied");
  free_decl_then_def (5);
  expect (1, "free_decl_then_def violated");

  violations = 0;
  on_definition (20);
  expect (0, "on_definition satisfied");
  on_definition (5);
  expect (1, "on_definition violated");

  violations = 0;
  in_body (20);
  expect (0, "in_body satisfied");
  in_body (5);
  expect (1, "in_body violated");

  violations = 0;
  TrivialTemp t;
  t.f (20);
  expect (0, "TrivialTemp satisfied");
  t.f (5);
  expect (1, "TrivialTemp violated");

  violations = 0;
  NonDependent n;
  n.f (0, 20);
  expect (0, "NonDependent satisfied");
  n.f (0, 5);
  expect (1, "NonDependent violated");

  violations = 0;
  NotATemplate p;
  p.f (20);
  expect (0, "NotATemplate satisfied");
  p.f (5);
  expect (1, "NotATemplate violated");

  return 0;
}
