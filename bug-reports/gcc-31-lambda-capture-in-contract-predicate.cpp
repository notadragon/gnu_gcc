// gcc-31-lambda-capture-in-contract-predicate.cpp                   -*-C++-*-
//
// GCC-31: a lambda written in a contract predicate cannot capture the
// enclosing function's parameters -- valid code is rejected.
//
//   g++ -std=c++26 -fcontracts -fsyntax-only \
//       gcc-31-lambda-capture-in-contract-predicate.cpp
//
// Stock g++ trunk, on each of the three capture forms:
//   error: use of parameter outside function body before '>' token
//
// [expr.prim.lambda.capture]/3.3 allows a capture-default or simple-capture
// in a lambda-introducer when the lambda appears within a contract assertion
// and its innermost enclosing scope is the corresponding contract-assertion
// scope.  So these are well-formed.
//
// PLAIN -fcontracts.

// THE BUG: all three rejected; all three are valid.
void simple  (int x) pre ([x] { return x > 0; } ()) { }
void by_copy (int x) pre ([=] { return x > 0; } ()) { }
void by_ref  (int x) pre ([&] { return x > 0; } ()) { }

// The same in a postcondition, and in one with a result-name-introducer.
// (const, because a postcondition that odr-uses a by-value parameter needs
// it const -- an unrelated rule that would otherwise fire here too.)
int in_post     (const int x) post ([x] { return x > 0; } ()) { return x; }
int in_post_res (const int x) post (r : [x] { return x > 0; } () && r == x) { return x; }

// And in an assertion-statement, which reaches the capture machinery by a
// third path.
void
in_assert (int x)
{
  contract_assert ([x] { return x > 0; } ());
}

// CONTROL: a lambda in a predicate that captures NOTHING was always accepted,
// which is what says the defect is in the capture and not in allowing a
// lambda in a predicate at all.
int g_n = 0;
void no_capture () pre ([] { return g_n >= 0; } ()) { }

// CONTROL: the same capturing lambda in the function BODY is accepted, so it
// is the contract-assertion scope that is mishandled, not the lambda.
void
in_body (int x)
{
  auto l = [x] { return x > 0; };
  (void) l ();
}
