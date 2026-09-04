// gcc-13-precondition-in-lambda-nested-in-generic-lambda.cpp         -*-C++-*-
//
// GCC-13.  **THE FILENAME UNDERSTATES THE BUG AND IS KEPT ONLY SO THE LINKS
// IN THE LEDGERS STILL RESOLVE** -- the same wrinkle already tolerated for
// gcc-03/gcc-04.  Nested lambdas are not required, generic lambdas are not
// required, and preconditions are not required.  What is actually required is:
//
//   A LAMBDA CARRYING A CONTRACT SPECIFIER WHOSE PREDICATE NAMES THE LAMBDA'S
//   OWN PARAMETER (OR ITS POSTCONDITION RESULT), APPEARING ANYWHERE INSIDE A
//   TEMPLATE THAT IS INSTANTIATED.
//
// MEASURED 2026-09-02 (see the matrix below).  The original write-up said
// "preconditions only, and only when the enclosing lambda is a template", and
// listed a postcondition in the same position as a working control.  Both were
// wrong, for the same reason: the whole matrix had been classified on RELEASE
// builds, where the postcondition face is a silent
// `gcc_checking_assert` that is compiled out.
//
// ROOT CAUSE -- one defect, four faces.  tsubst_function_decl copies a
// function's contract specifiers onto the instantiation WITHOUT substituting
// them ("NOTE these are not substituted at this point"), leaving that to
// regenerate_decl_from_template.  A lambda's operator() never goes through
// regenerate_decl_from_template: tsubst_lambda_expr builds it and substitutes
// its body directly.  So the instantiated operator() keeps the PATTERN's
// contract trees, whose predicate names the PATTERN's PARM_DECLs and whose
// result binding belongs to the PATTERN.  Nothing checks this, so it surfaces
// wherever the stale tree is first used:
//
//   | face | shape                                   | where it lands |
//   |------|-----------------------------------------|----------------|
//   | 1    | `pre`, non-generic lambda               | `expand_expr_real_1` -- the predicate's `b` has no RTL in this function |
//   | 2    | `pre`, generic lambda                   | `tsubst_expr` `gcc_assert (cp_unevaluated_operand)` -- `retrieve_local_specialization` cannot find the pattern's `b` |
//   | 3    | `post` with a result name, ONE instantiation  | `rebuild_postconditions` `gcc_checking_assert (!DECL_CONTEXT (oldvar) \|\| DECL_CONTEXT (oldvar) == fndecl)`.  **CHECKING BUILDS ONLY** |
//   | 4    | `post` with a result name, TWO instantiations | `tsubst_expr`.  **RELEASE BUILDS TOO** -- face 3 silently reparents the pattern's result variable onto the first instantiation, and the second one then trips over it |
//
// Face 4 is the one that makes the postcondition half reportable rather than a
// checking-build curiosity: `g++ 16.2.0` compiles a single instantiation
// cleanly AND RUNS IT CORRECTLY (verified: the postcondition sees the right
// value), then ICEs on the second instantiation of the very same template.
//
// PROVENANCE: UPSTREAM'S, measured 2026-09-02 -- identical on stock
// `g++ 16.2.0`, on `g++-trunk` (17.0.0 20260901) and on our branch, at the
// corresponding line numbers.  Clang accepts every row.  Nothing to do with
// the p3850 branch.
//
// FOUND 2026-09-02 by probing the area PR126041 describes after that report
// could not be fetched.  **GCC-13 is NOT PR126041** -- 126041 is GCC-14, whose
// cached report title ("Source-location line table underflow and diagnostic
// metadata corruption in nested generic lambda contract assertions") matches
// that bug and not this one.  Bugzilla has NOT been searched for this one; the
// Anubis proof-of-work interstitial blocks non-browser clients, so the search
// is owed to a browser session.  Do that before filing.
//
// THE MEASURED MATRIX.  Each row is its own translation unit; `clean` means
// compiled with no diagnostic, distinguished from `ill-formed` (the GCC-7
// lesson: a probe that only looks for an ICE will call a rejected program
// "clean").
//
//   | # | shape                                                      | 16.2.0 | trunk |
//   |---|------------------------------------------------------------|--------|-------|
//   | a | `pre` naming a parm, plain lambda in a generic lambda       | ICE 1  | ICE 1 |
//   | b | `pre` naming a parm, generic lambda in a generic lambda     | ICE 2  | ICE 2 |
//   | c | `post (r : ...)`, plain lambda in a generic lambda          | clean  | ICE 3 |
//   | d | `post (r : ...)`, generic lambda in a generic lambda        | clean  | clean |
//   | e | `contract_assert` in the inner body                         | clean  | clean |
//   | f | `pre` naming a parm, generic lambda in a PLAIN lambda        | clean  | clean |
//   | g | contract on the OUTER generic lambda only                   | clean  | clean |
//   | h | `pre` on a generic lambda, no nesting                       | clean  | clean |
//   | i | nested generic lambdas, no contract                         | clean  | clean |
//   | j | `post` with NO result name, naming a by-value parm          | ill-formed (dcl.contract.func) -- not a control |
//   | k | `pre` naming a parm, plain lambda in a FUNCTION TEMPLATE    | ICE 1  | ICE 1 |
//   | l | as k, but `pre (true)` -- predicate names nothing local      | clean  | clean |
//   | m | as a, but `pre (true)`                                      | clean  | clean |
//   | n | as a, but the predicate names a GLOBAL                      | clean  | clean |
//   | o | `pre` naming a parm, lambda in a CLASS TEMPLATE member       | ICE 1  | ICE 1 |
//   | p | `post (r : ...)`, lambda in a function template             | clean  | ICE 3 |
//   | q | `contract_assert` in a lambda in a function template         | clean  | clean |
//   | r | `pre` naming a parm, GENERIC lambda in a function template   | ICE 2  | ICE 2 |
//   | s | as k, but the lambda is never called                        | clean  | clean |
//   | t | as k, but in a NON-template function                        | clean  | clean |
//
// So the necessary ingredients are exactly three, and each is confirmed by
// dropping it on its own: the lambda carries a contract SPECIFIER (q drops it
// for a `contract_assert`, which is part of the body and substituted
// normally); the predicate names the lambda's own parameter or result (l, m, n
// drop it); and the lambda is instantiated as part of a template (t drops the
// template, s drops the instantiation).  Whether anything is nested, and
// whether anything is generic, is irrelevant -- k and o carry no nesting and
// no generic lambda at all.
//
// RUNNING THIS FILE.  Compilation stops at the first ICE, so compile one case
// at a time:
//
//   g++ -std=c++26 -fcontracts -c -DONLY=1 ...   (face 1, expand_expr_real_1)
//   g++ -std=c++26 -fcontracts -c -DONLY=2 ...   (face 2, tsubst_expr)
//   g++ -std=c++26 -fcontracts -c -DONLY=3 ...   (face 3, rebuild_postconditions)
//   g++ -std=c++26 -fcontracts -c -DONLY=4 ...   (face 4, two instantiations)
//   g++ -std=c++26 -fcontracts -c -DONLY=5 ...   (the controls, all clean)

#ifndef ONLY
#define ONLY 1
#endif

#if ONLY == 1
// Face 1 -- `pre` naming a parameter, in a plain lambda inside a function
// template.  No nesting and no generic lambda: this is the smallest shape.
template <class T>
int
ft (T a)
{
  auto inner = [] (int b) pre (b > 0) { return b; };
  return inner (a);
}

int f () { return ft (1); }
#endif

#if ONLY == 2
// Face 2 -- the same, with a GENERIC lambda.  Substitution of the inner
// operator() is deferred to its own instantiation, which regenerates the
// contracts from a pattern that no longer matches.
template <class T>
int
ft (T a)
{
  auto inner = [] (auto b) pre (b > 0) { return b; };
  return inner (a);
}

int f () { return ft (1); }
#endif

#if ONLY == 3
// Face 3 -- a postcondition with a result name.  CHECKING BUILDS ONLY: on a
// release build this compiles and runs correctly, having quietly reparented
// the pattern's result variable onto this instantiation.
template <class T>
int
ft (T a)
{
  auto inner = [] (int b) post (r : r > 0) { return b; };
  return inner (a);
}

int f () { return ft (1); }
#endif

#if ONLY == 4
// Face 4 -- the same postcondition, instantiated TWICE.  This is the one that
// reaches a release build: the first instantiation left the pattern's result
// variable pointing at itself, and the second cannot recover from it.
template <class T>
int
ft (T a)
{
  auto inner = [] (int b) post (r : r > 0) { return b; };
  return inner ((int) a);
}

int f () { return ft (1) + ft (2.0); }
#endif

#if ONLY == 5
// The controls.  Every one of these compiles clean; each drops exactly one of
// the three necessary ingredients.

int g_v = 1;

// Drops "the predicate names something local" -- three ways.
template <class T> int c_true   (T a)
{ auto l = [] (int b) pre (true)    { return b; }; return l (a); }
template <class T> int c_global (T a)
{ auto l = [] (int b) pre (g_v > 0) { return b; }; return l (a); }

// Drops "carries a contract specifier": a contract_assert is part of the body
// and is substituted with it.
template <class T> int c_assert (T a)
{ auto l = [] (int b) { contract_assert (b > 0); return b; }; return l (a); }

// Drops "instantiated as part of a template" -- two ways.
int c_nontemplate ()
{ auto l = [] (int b) pre (b > 0) { return b; }; return l (1); }
template <class T> int c_uncalled (T a)
{ auto l = [] (int b) pre (b > 0) { return b; }; return (int) a; }

// A contract on the template's own function, rather than on a lambda inside
// it, goes through regenerate_decl_from_template and is fine.
template <class T> int c_on_the_template (T a) pre (a > 0) { return (int) a; }

int
f ()
{
  return c_true (1) + c_global (1) + c_assert (1) + c_nontemplate ()
	 + c_uncalled (1) + c_on_the_template (1);
}
#endif

int
main ()
{
  return 0;
}
