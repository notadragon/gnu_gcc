/* Test a P3400 assertion-control specifier whose expression contains
   tokens that the specifier's own delimiters also use.

   Regression test: cp_maybe_function_contract_specifier is a cheap
   token-arithmetic recognizer used to decide whether a trailing return
   type is followed by a contract specifier.  Its "< ... >" skip counted
   angle brackets with no awareness of (), [] or {} nesting, and bailed
   outright on a semicolon.  So a label expression containing a relational
   operator inside parentheses, or a lambda body containing a statement,
   made the skip stop in the wrong place; the recognizer then declined the
   contract, the tentative abstract-declarator parse swallowed "pre<...>",
   and the whole declaration failed with "expected initializer".

   Only the trailing-return-type form goes through that recognizer, so
   each case is paired with the identical contract written with a leading
   return type.  The two must agree: valid code cannot depend on where the
   return type is written.  This is the same family as
   p3400-label-trailing-return.C, which fixed one instance of it.

   The requires-clause shapes are here for the same reason -- the
   recognizer assumed a requires-clause was always "requires ( ... )",
   while the grammar allows an unparenthesized
   constraint-logical-or-expression.  Those happened to recover through
   the tentative-parse fallback, so they are a lock against regressing
   what already worked rather than a fix.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4283 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations = 0;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

struct L
{
  using assertion_control_object = L;
};
constexpr L lbl{};
constexpr int N = 2;

template <class T> concept Pos = true;

/* '>' inside parentheses in the label expression.  */
template <class T> auto t_gt (T a) -> int pre<(N > 1 ? lbl : lbl)> (a > 0)
{ return 1; }
template <class T> int l_gt (T a) pre<(N > 1 ? lbl : lbl)> (a > 0)
{ return 1; }

/* '<' inside parentheses.  */
template <class T> auto t_lt (T a) -> int pre<(N < 1 ? lbl : lbl)> (a > 0)
{ return 1; }
template <class T> int l_lt (T a) pre<(N < 1 ? lbl : lbl)> (a > 0)
{ return 1; }

/* ';' inside a lambda body in the label expression.  */
template <class T> auto t_semi (T a) -> int pre<([]{ return lbl; }())> (a > 0)
{ return 1; }
template <class T> int l_semi (T a) pre<([]{ return lbl; }())> (a > 0)
{ return 1; }

/* Control: a parenthesized label expression with none of the three.  */
template <class T> auto t_plain (T a) -> int pre<(true ? lbl : lbl)> (a > 0)
{ return 1; }

/* Unparenthesized requires-clauses, trailing return.  */
template <class T> auto r_bare (T a) -> int pre requires Pos<T> (a > 0)
{ return 1; }
template <class T> auto r_and (T a) -> int
  pre requires Pos<T> && Pos<T> (a > 0)
{ return 1; }
template <class T> auto r_mixed (T a) -> int
  pre requires (Pos<T>) && Pos<T> (a > 0)
{ return 1; }
template <class T> auto r_expr (T a) -> int
  pre requires requires { a > 0; } (a > 0)
{ return 1; }

/* Declaration first, definition later.  */
template <class T> auto r_decl (T a) -> int pre requires Pos<T> (a > 0);
template <class T> auto r_decl (T a) -> int { return 1; }

#define CHECK(CALL)				\
  do {						\
    violations = 0;				\
    CALL;					\
    if (violations != 1)			\
      __builtin_abort ();			\
  } while (0)

int
main ()
{
  /* Each pair must behave identically.  */
  CHECK (t_gt (-1));
  CHECK (l_gt (-1));
  CHECK (t_lt (-1));
  CHECK (l_lt (-1));
  CHECK (t_semi (-1));
  CHECK (l_semi (-1));
  CHECK (t_plain (-1));

  CHECK (r_bare (-1));
  CHECK (r_and (-1));
  CHECK (r_mixed (-1));
  CHECK (r_expr (-1));
  CHECK (r_decl (-1));

  return 0;
}
