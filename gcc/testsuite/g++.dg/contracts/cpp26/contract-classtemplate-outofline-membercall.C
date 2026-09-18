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

// Predicate calls another member -- the "Call must have function pointer
// type" face of the same bug.
template <class T> struct MemberCall {
  bool ok(T v) const;
  int f(T v) pre(ok(v));
};
template <class T> bool MemberCall<T>::ok(T v) const { return v > T(); }
template <class T> int MemberCall<T>::f(T v) { return 1; }

int main() {
  MemberCall<int> mc;
  mc.f(1);
  expect(0, "MemberCall good");
  mc.f(-1);
  expect(1, "MemberCall bad");

  if (failures == 0)
    std::puts("PASS");
  return failures;
}
