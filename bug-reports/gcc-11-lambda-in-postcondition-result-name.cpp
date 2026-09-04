// gcc-11-lambda-in-postcondition-result-name.cpp                     -*-C++-*-
//
// GCC-11: a lambda-expression in the predicate of a FREE function's
// postcondition fails to parse when the postcondition uses a
// result-name-introducer.  The same lambda parses fine in a precondition, in
// a postcondition WITHOUT a result name, and in a member function's
// postcondition WITH one -- so the trigger is the specific combination.
//
//   g++ -std=c++26 -fcontracts -c gcc-11-....cpp
//   -> error: expected ')' before '{' token
//      error: expected unqualified-id before ')' token
//
// NO CAPTURE IS INVOLVED.  The lambda below captures nothing; this is a pure
// parse failure, not a contracts capture problem.
//
// PROVENANCE: UPSTREAM'S, measured 2026-09-02.  Reproduces on stock
// g++ 16.2.0 and g++-trunk (17.0.0 20260901) as well as our branch.
// g++ 15.3.0 rejects the `post' syntax outright ("expected initializer before
// 'post'"), i.e. it predates the feature, so the bug spans every GCC release
// that has ever accepted this syntax.  Nothing to do with the p3850 branch.
//
// Clang accepts all four rows below, so it is GCC-specific.
//
// FOUND 2026-09-02 while implementing [expr.prim.lambda.capture]/3.3 capture
// support for lambdas in contract predicates (PR117435); it surfaced as a
// spurious failure in that work's test and was isolated out of it, tracked
// internally as GCC-11.

bool ok () { return true; }

// (1) THE BUG: free function, postcondition, result-name-introducer.
int broken (int x) post (r : [] { return ok (); } ()) { return x; }

// (2) Control: free function, postcondition, NO result-name.  Accepted.
int no_result_name (int x) post ([] { return ok (); } ()) { return x; }

// (3) Control: free function, precondition.  Accepted.
int precondition (int x) pre ([] { return ok (); } ()) { return x; }

// (4) Control: MEMBER function, postcondition, result-name-introducer.
//     Accepted -- which is what makes this a parse-path difference rather
//     than anything about postconditions or result names as such.  A member
//     function's contract is deferred and late-parsed after the class is
//     complete; a free function's is parsed straight off the declarator.
struct S
{
  int mem (int x) post (r : [] { return ok (); } ()) { return x; }
};

int
main ()
{
  return 0;
}
