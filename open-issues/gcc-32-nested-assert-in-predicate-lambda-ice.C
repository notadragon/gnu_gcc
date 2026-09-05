// GCC-32: a contract_assert inside a lambda that is written in a FUNCTION
// contract's predicate ICEs, when the assert names one of the lambda's
// captures.
//
// Compile with: g++ -std=c++26 -fcontracts
//
//   internal compiler error: in expand_expr_real_1, at expr.cc:11792
//
// The assertion there reads "Variables inherited from containing functions
// should have been lowered by this point": the reference inside the nested
// assert is still the ENCLOSING function's parameter rather than the
// closure's captured copy, so the capture never reached it.

int g_n = 0;

// THE ICE: a capture, named inside a nested contract_assert, in a `pre`.
// (The compiler stops at the first ICE, so comment this one out to see that
// the `post` below fails the same way.)
void in_pre (int x) pre ([x] { contract_assert (x >= 0); return x > 0; } ()) { }

// ... and in a `post`.  (const because a postcondition that odr-uses a
// by-value parameter needs it const, an unrelated rule.)
int in_post (const int x)
  post ([x] { contract_assert (x >= 0); return x > 0; } ()) { return x; }

// CONTROL: the same capture with NO nested assert compiles and runs.  This is
// what says the capture machinery itself is fine.
void no_nested_assert (int x) pre ([x] { return x > 0; } ()) { }

// CONTROL: a nested assert that names a GLOBAL rather than a capture is fine,
// which places the defect on the captured reference specifically.
void nested_assert_on_global (int x)
  pre ([x] { contract_assert (g_n >= 0); return x > 0; } ()) { }

// CONTROL: the same shape in an ASSERTION-STATEMENT rather than a function
// contract is fine -- so it is the function-contract path, not lambdas in
// predicates generally.
void in_assertion_statement (int x)
{
  contract_assert ([x] { contract_assert (x >= 0); return x > 0; } ());
}

// CONTROL: the same lambda in the function BODY is fine.
void in_body (int x)
{
  auto l = [x] { contract_assert (x >= 0); return x > 0; };
  (void) l ();
}

int
main ()
{
  no_nested_assert (1);
  nested_assert_on_global (1);
  in_assertion_statement (1);
  in_body (1);
}
