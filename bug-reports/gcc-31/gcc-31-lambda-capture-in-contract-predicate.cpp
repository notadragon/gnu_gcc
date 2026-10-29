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

int g_n = 0;

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

// THE SECOND LAYER.  These are what a compiler hits once the capture above is
// accepted, so a fix that stops at the first layer does not finish the job.
// A contract_assert nested inside the predicate lambda, NAMING one of its
// captures: the nested assert re-enters the contract-condition state, which
// exempts a parameter reference from the capture machinery, so a bare
// reference to the ENCLOSING function's parameter survives into the lambda
// body.  It reaches expansion and trips
//
//   internal compiler error: in expand_expr_real_1, at expr.cc:11792
//   "Variables inherited from containing functions should have been lowered
//    by this point"
//
// Stock g++ cannot show this today because it rejects the capture first.
void nested_assert (int x)
  pre ([x] { contract_assert (x >= 0); return x > 0; } ()) { }

int nested_assert_post (const int x)
  post ([x] { contract_assert (x >= 0); return x > 0; } ()) { return x; }

// CONTROL for the second layer: a nested assert naming a GLOBAL rather than a
// capture is fine, which places the defect on the captured reference.
void nested_assert_on_global (int x)
  pre ([x] { contract_assert (g_n >= 0); return x > 0; } ()) { }

// CONTROL for the second layer: the same nesting in an ASSERTION-STATEMENT
// rather than a function contract is fine, so it is the function-contract
// path specifically.
void nested_in_assertion_statement (int x)
{
  contract_assert ([x] { contract_assert (x >= 0); return x > 0; } ());
}

// CONTROL: a lambda in a predicate that captures NOTHING was always accepted,
// which is what says the defect is in the capture and not in allowing a
// lambda in a predicate at all.
void no_capture () pre ([] { return g_n >= 0; } ()) { }

// CONTROL: the same capturing lambda in the function BODY is accepted, so it
// is the contract-assertion scope that is mishandled, not the lambda.
void
in_body (int x)
{
  auto l = [x] { return x > 0; };
  (void) l ();
}
