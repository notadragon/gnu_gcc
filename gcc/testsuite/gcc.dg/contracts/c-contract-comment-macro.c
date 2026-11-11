/* The comment of a contract whose predicate involves macros, as in C++
   (GCC-707): the source text where the contract was written, the text of
   a macro argument or replacement list when every token is from it, and
   otherwise the predicate's tokens.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */

#include <contracts.h>
#include <stdio.h>
#include <string.h>

#define X x
#define ZERO 0
#define NEG(a) ((a) < 0)
#define MY_PRE(c) _Pre (c)
#define MY_ASSERT(c) _ContractAssert (c)
#define CHK(c) _ContractAssert ((c) > 0)

static const char *expected;
static int violations, mismatches;

void handle_contract_violation(const contract_violation_t* cv)
{
  const char *comment = stdc_contract_violation_comment(cv);
  violations++;
  if (!comment || strcmp(comment, expected) != 0)
    {
      mismatches++;
      fprintf(stderr, "expected \"%s\", got \"%s\"\n", expected,
	      comment ? comment : "(null)");
    }
}

int begins_in(int x) _Pre(X < 0) { return x; }
int one_fn(int x) _Pre(NEG (x)) { return x; }
int argument(int x) MY_PRE(X < ZERO) { return x; }
int post_fn(int x) _Post(r: x > 9 || NEG (r)) { return x; }

int main(void)
{
  int x = 5;
  expected = "X < 0";
  begins_in(5);
  expected = "NEG (x)";
  one_fn(5);
  expected = "X < ZERO";
  argument(5);
  expected = "x > 9 || NEG (r)";
  post_fn(5);
  expected = "X < ZERO";
  _ContractAssert(X < ZERO);
  expected = "x < 0";
  MY_ASSERT(x < 0);
  expected = "(-x) > 0";
  CHK(-x);

  if (violations != 7 || mismatches != 0)
    __builtin_abort();
  return 0;
}
