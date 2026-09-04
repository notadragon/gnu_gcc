// SUPERSEDED 2026-09-02 -- this became the in-tree regression test
// gcc/testsuite/g++.dg/contracts/cpp26/lambda-in-postcondition-result-name.C
// when GCC-11 was fixed (gnu_gcc bb488b220c0).  The installed copy is the
// one to edit; it adds a lambda nested inside a lambda and instantiates the
// templated case three times.  Kept here only as the attachment for the
// upstream report, which is not yet sent.
//
// A lambda-expression in the predicate of a postcondition that has a
// result-name-introducer failed to parse on a non-member function:
//
//   error: expected ')' before '{' token
//
// The non-deferred contract path raises processing_template_decl whenever a
// result name is present, because the result variable's type is not known
// while the predicate is parsed off the declarator.  Lambda parsing does not
// cope with that flag being raised outside a real template -- the same
// problem a requires-expression has, and which cp_parser_lambda_expression
// already works around; the workaround simply did not cover the contract
// case.
//
// A member function's contract is deferred and late-parsed, and the late path
// raises the flag only for an undeduced return type, which is why members
// were unaffected.  Both are covered below.
//
// The lambda's position in the predicate is irrelevant -- the flag is in
// effect for the whole parse -- so first, later and parenthesized positions
// are all pinned.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }

int g_seen = -1;

bool
record (int v)
{
  g_seen = v;
  return true;
}

/* The failing shape: non-member, postcondition, result-name-introducer.  */
int
first (int x) post (r : [] { return record (1); } ())
{
  return x;
}

int
later (int x) post (r : r >= 0 && [] { return record (2); } ())
{
  return x;
}

int
parenthesized (int x) post (r : ([] { return record (3); } ()))
{
  return x;
}

/* The result name actually used in the predicate alongside a lambda.  */
int
uses_result (int x) post (r : r == 7 && [] { return record (4); } ())
{
  return x;
}

/* A capture, which reaches the capture machinery as well as the parser.  */
int
with_capture (const int x) post (r : [x] { return record (x); } ())
{
  return x;
}

/* Controls: no result name, and a precondition.  Both always worked.  */
int
no_result_name (int x) post ([] { return record (5); } ())
{
  return x;
}

int
precondition (int x) pre ([] { return record (6); } ())
{
  return x;
}

/* Member function with a result name: the path that already worked.  */
struct S
{
  int mem (int x) post (r : [] { return record (7); } ()) { return x; }
};

/* A real template, where processing_template_decl is genuinely raised and
   must stay raised.  */
template <class T>
T
templated (T x) post (r : [] { return record (8); } ())
{
  return x;
}

int
main ()
{
  g_seen = -1; first (0);          if (g_seen != 1) __builtin_abort ();
  g_seen = -1; later (0);          if (g_seen != 2) __builtin_abort ();
  g_seen = -1; parenthesized (0);  if (g_seen != 3) __builtin_abort ();
  g_seen = -1; uses_result (7);    if (g_seen != 4) __builtin_abort ();
  g_seen = -1; with_capture (9);   if (g_seen != 9) __builtin_abort ();
  g_seen = -1; no_result_name (0); if (g_seen != 5) __builtin_abort ();
  g_seen = -1; precondition (0);   if (g_seen != 6) __builtin_abort ();

  S s;
  g_seen = -1; s.mem (0);          if (g_seen != 7) __builtin_abort ();

  g_seen = -1; templated (0);      if (g_seen != 8) __builtin_abort ();

  return 0;
}
