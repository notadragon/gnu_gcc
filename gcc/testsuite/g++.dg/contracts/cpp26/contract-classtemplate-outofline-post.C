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

// A postcondition naming a parameter (which must be const) and the result.
template <int N> struct PostParm {
  int f(const int rhs) post(r : r >= rhs);
};
template <int N> int PostParm<N>::f(const int rhs) { return rhs - N; }

// Both a pre and a post on one out-of-line member.
template <int N> struct Both {
  int f(const int rhs) pre(rhs >= 0) post(r : r == rhs);
};
template <int N> int Both<N>::f(const int rhs) { return rhs; }

int main() {
  PostParm<0> pp0;
  pp0.f(5);
  expect(0, "PostParm good");
  PostParm<1> pp1;
  pp1.f(5); // returns 4, which is < rhs
  expect(1, "PostParm bad");

  Both<1> bo;
  bo.f(3);
  expect(0, "Both good");
  bo.f(-3); // pre fires; post then compares -3 == -3 and holds
  expect(1, "Both bad");

  if (failures == 0)
    std::puts("PASS");
  return failures;
}
