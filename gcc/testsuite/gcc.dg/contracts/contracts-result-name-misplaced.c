/* A result-name introducer ("identifier :") is only meaningful on a
   postcondition.  On _Pre and _ContractAssert it must be diagnosed
   directly.

   Regression test: recognizing it only for _Post lets it fall
   through into the predicate parse elsewhere, where the identifier is
   undeclared and the colon is a syntax error, producing an unrelated cascade
   of up to three diagnostics instead of saying what is actually wrong.  This
   is the C-front-end half of the rule; cp_parser_contract_result_name is
   the C++ half.  */

/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>

/* Misplaced on a precondition.  */
int bad_pre (int x) _Pre (r : x > 0) /* { dg-error "result name 'r' not allowed outside of post condition specifier" } */
{
  return x;
}

/* Misplaced on a contract assertion.  */
void
bad_assert (int x)
{
  _ContractAssert (r : x > 0); /* { dg-error "result name 'r' not allowed outside of post condition specifier" } */
}

/* Positive control: on a postcondition it is exactly right.  */
int
good_post (int x) _Post (r : r > 0)
{
  return x + 1;
}

/* Positive control: a predicate that merely starts with an identifier is
   untouched.  */
int
plain_pre (int x) _Pre (x > 0)
{
  return x;
}
