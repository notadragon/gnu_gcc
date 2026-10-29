// The comment of a contract whose predicate involves macros (GCC-707).
// The predicate is lifted out of macro expansions to the context where the
// contract's own parentheses were written.  There, outside any expansion,
// the comment is the source text from the predicate's first token to its
// last, including the whole of a macro invocation at either end.  Inside a
// macro, it is the text of the replacement list if every token of the
// predicate is from it, and the text of an argument as written if every
// token is from that one use of the argument.  Otherwise it is the
// predicate's tokens, separated by a space where whitespace preceded one.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "contract-comment-macro.h"

#define X x
#define ZERO 0
#define Y X
#define NEG(a) ((a) < 0)
#define ID(e) e
#define LHS x <
#define RHS < 0
#define FIRST(a, b) (a)
#define CAT(a, b) a ## b
#define PRE_NEG pre (x < 0)
#define MY_PRE(c) pre (c)
#define MY_POST(c) post (r: c)
#define MY_ASSERT(c) contract_assert (c)
#define WRAP(c) MY_PRE (c)
#define CHK(c) contract_assert ((c) > 0)
#define BOTH(a, b) pre (a && b)
#define TWICE(a) pre (a == -a)

const char *expected;
int violations, mismatches;

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  ++violations;
  if (std::strcmp (v.comment (), expected) != 0)
    {
      ++mismatches;
      std::fprintf (stderr, "expected \"%s\", got \"%s\"\n", expected,
		    v.comment ());
    }
}

// Outside any expansion: the source text.
int begins_in (int x) pre (X < 0) { return x; }
int begins_in_fn (int x) pre (NEG (x) && x > 9) { return x; }
int ends_in (int x) pre (x < ZERO) { return x; }
int ends_in_fn (int x) pre (x > 9 || NEG (x)) { return x; }
int two (int x) pre (X < ZERO) { return x; }
int two_fn (int x) pre (NEG (x) || NEG (x)) { return x; }
int one_fn (int x) pre (NEG (x)) { return x; }
int first_half (int x) pre (LHS 0) { return x; }
int second_half (int x) pre (x RHS) { return x; }
int header (int x) pre (HX < 0) { return x; }
int nested (int x) pre (Y < 0) { return x; }
int in_argument (int x) pre (ID (x) < 0) { return x; }
int whole_argument (int x) pre (ID (x < 0)) { return x; }
int multi_line (int x) pre (NEG (
			      x)) { return x; }
int with_comment (int x) pre (NEG (x /* why */)) { return x; }
int string_argument (int x) pre (FIRST (x < 0, ")")) { return x; }
int pasted (int xy) pre (CAT (x, y) < 0) { return xy; }

// The whole contract from a macro.
int body (int x) PRE_NEG { return x; }
int argument (int x) MY_PRE (x < 0) { return x; }
int argument_two (int x) MY_PRE (X < ZERO) { return x; }
int argument_fn (int x) MY_PRE (NEG (x)) { return x; }
int argument_wrapped (int x) WRAP (x < 0) { return x; }
int argument_post (int x) MY_POST (r < 0) { return x; }

// Tokens from more than one place: the tokens.
int two_arguments (int x) BOTH (x > 0, x < 5) { return x; }
int argument_twice (int x) TWICE (x) { return x; }

struct S
{
  int member (int x) pre (NEG (x)) { return x; }
  int member_argument (int x) MY_PRE (X < ZERO) { return x; }
};

void
check (const char *e, int (*f) (int))
{
  expected = e;
  f (5);
}

int
main ()
{
  check ("X < 0", begins_in);
  check ("NEG (x) && x > 9", begins_in_fn);
  check ("x < ZERO", ends_in);
  check ("x > 9 || NEG (x)", ends_in_fn);
  check ("X < ZERO", two);
  check ("NEG (x) || NEG (x)", two_fn);
  check ("NEG (x)", one_fn);
  check ("LHS 0", first_half);
  check ("x RHS", second_half);
  check ("HX < 0", header);
  check ("Y < 0", nested);
  check ("ID (x) < 0", in_argument);
  check ("ID (x < 0)", whole_argument);
  check ("NEG ( x)", multi_line);
  check ("NEG (x /* why */)", with_comment);
  check ("FIRST (x < 0, \")\")", string_argument);
  check ("CAT (x, y) < 0", pasted);

  check ("x < 0", body);
  check ("x < 0", argument);
  check ("X < ZERO", argument_two);
  check ("NEG (x)", argument_fn);
  check ("x < 0", argument_wrapped);
  check ("r < 0", argument_post);

  check ("x > 0 && x < 5", two_arguments);
  check ("x == -x", argument_twice);

  S s;
  expected = "NEG (x)";
  s.member (5);
  expected = "X < ZERO";
  s.member_argument (5);

  int x = 5;
  expected = "X < ZERO";
  contract_assert (X < ZERO);
  expected = "x < 0";
  MY_ASSERT (x < 0);
  expected = "(-x) > 0";
  CHK (-x);

  if (violations != 30 || mismatches != 0)
    std::abort ();
}
