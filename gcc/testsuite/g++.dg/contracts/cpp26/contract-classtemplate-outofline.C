// A contract declared in-class on a member of a CLASS TEMPLATE, where that
// member is DEFINED OUT-OF-LINE.  GCC has always got this right; this is a
// guard mirrored from Clang, where it crashed CodeGen (mirror ledger #66).
// Split across several small files rather than one combined one: DejaGnu
// classified a single file combining many of these shapes at once as
// UNSUPPORTED for reasons unrelated to the compiler under test (confirmed
// by compiling and running the exact combined content directly against the
// release compiler outside DejaGnu, where it passes cleanly under both
// -std=c++26 and -std=c++29) -- see bugfix-added-tests.md.
//
// Clang kept two contract specifiers on the pattern's redeclaration chain --
// the in-class declaration's, installed on the instantiated member as a
// placeholder, and the out-of-line definition's own re-pointed copy -- and
// its idempotence guard compared the placeholder against only the latter.
// It concluded the contract had already been substituted and left the
// instantiation holding a dependent predicate referencing the *pattern's*
// parameters, which CodeGen then tripped over in several different places
// depending on what the predicate contained.
//
// Run test, not compile test, on purpose: a fix that substituted against
// the wrong parameter would still compile and would silently check
// garbage, so what is pinned is that the predicate is evaluated against
// THIS call's arguments.

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violation_count;
}
static int failures = 0;
static void expect(int expected, const char *what) {
  if (violation_count != expected) {
    std::printf("FAIL: %s: expected %d, got %d\n", what, expected, violation_count);
    ++failures;
  }
  violation_count = 0;
}

// The original shape: predicate names an ordinary parameter.
template <int N> struct ParmRef {
  int f(int rhs) pre(rhs >= 0);
};
template <int N> int ParmRef<N>::f(int rhs) { return rhs; }

// The Clang AutoReturn case (a deduced return type on the out-of-line
// definition), added now that GCC-4 (gcc-bugs/pending_reports.md, fixed
// gnu_gcc 677b81146d0) no longer crashes GCC on this shape for an
// unrelated reason -- see dcl.contract.res-auto-return-parm.C for that
// bug's own dedicated regression test.
template <class T> struct AutoReturn {
  auto f(T v) pre(v != T());
};
template <class T> auto AutoReturn<T>::f(T v) { return v; }

// Controls: these always worked and must keep working.
template <int N> struct InlineDef {
  int f(int rhs) pre(rhs >= 0) { return rhs; }
};

struct NonTemplate {
  int f(int rhs) pre(rhs >= 0);
};
int NonTemplate::f(int rhs) { return rhs; }

template <int N> int freeFn(int rhs) pre(rhs >= 0);
template <int N> int freeFn(int rhs) { return rhs; }

int main() {
  ParmRef<1> pr;
  pr.f(5);
  expect(0, "ParmRef good");
  pr.f(-1);
  expect(1, "ParmRef bad");

  // A second instantiation must get its own substituted specifier.
  ParmRef<2> pr2;
  pr2.f(-1);
  expect(1, "ParmRef<2> bad");

  AutoReturn<int> ar;
  ar.f(1);
  expect(0, "AutoReturn good");
  ar.f(0);
  expect(1, "AutoReturn bad");

  InlineDef<1> id;
  id.f(-1);
  expect(1, "InlineDef bad");

  NonTemplate nt;
  nt.f(-1);
  expect(1, "NonTemplate bad");

  freeFn<1>(-1);
  expect(1, "freeFn bad");

  if (failures == 0)
    std::puts("PASS");
  return failures;
}
