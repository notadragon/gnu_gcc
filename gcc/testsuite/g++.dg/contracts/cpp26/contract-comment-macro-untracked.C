// With -ftrack-macro-expansion=0 the location of a token from a macro
// expansion is that of the outermost invocation, so the source text of a
// predicate is not used at all: the comment of every contract is the
// predicate's tokens, separated by a space where whitespace preceded one,
// with a literal spelled from its value (GCC-707).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe -ftrack-macro-expansion=0" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define X x
#define ZERO 0
#define NEG(a) ((a) < 0)
#define MY_PRE(c) pre (c)
#define CHK(c) contract_assert ((c) > 0)

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

int plain (int x) pre (x
		       < 0x10) { return x; }
int two (int x) pre (X < ZERO) { return x; }
int one_fn (int x) pre (NEG (x)) { return x; }
int argument (int x) MY_PRE (x < 0) { return x; }

struct S
{
  int member (int x) pre (NEG (x)) { return x; }
};

int
main ()
{
  expected = "x < 16";
  plain (20);
  expected = "x < 0";
  two (5);
  expected = "((x) < 0)";
  one_fn (5);
  expected = "x < 0";
  argument (5);
  expected = "((x) < 0)";
  S ().member (5);

  int x = 5;
  expected = "(-x) > 0";
  CHK (-x);

  if (violations != 6 || mismatches != 0)
    std::abort ();
}
