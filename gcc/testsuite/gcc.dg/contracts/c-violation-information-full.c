/* -fcontract-violation-information=full, the default, given explicitly: a
   C contract violation carries the file name, function name, line and
   comment, as do the P3290 convenience macros of <contracts.h>, and the
   compiler does not predefine __gcc_contracts_violation_information_none.
   c-violation-information-none.c is the same program under =none.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe -fcontract-violation-information=full" } */

#ifdef __gcc_contracts_violation_information_none
#error "__gcc_contracts_violation_information_none is defined"
#endif

#include <contracts.h>
#include <string.h>

static int count;
static int expected_kind;
static const char *expected_function;
static unsigned expected_line;
static const char *expected_comment;

void
handle_contract_violation (const contract_violation_t *cv)
{
  if (strcmp (stdc_contract_violation_file (cv), __FILE__) != 0
      || strcmp (stdc_contract_violation_function (cv), expected_function) != 0
      || stdc_contract_violation_line (cv) != expected_line
      || strcmp (stdc_contract_violation_comment (cv), expected_comment) != 0
      || stdc_contract_violation_kind (cv) != expected_kind
      || stdc_contract_violation_semantic (cv) != STDC_CONTRACT_OBSERVE)
    __builtin_abort ();
  ++count;
}

int pre_f (int x) _Pre (x > 0) { return x; }		/* line 35 */
int post_f (int x) _Post (r : r > 0) { return x; }	/* line 36 */
void assert_f (int x) { _ContractAssert (x > 0); }	/* line 37 */

static void
expect (int kind, const char *function, unsigned line, const char *comment)
{
  expected_kind = kind;
  expected_function = function;
  expected_line = line;
  expected_comment = comment;
}

int
main (void)
{
  expect (STDC_CONTRACT_PRE, "pre_f", 35, "x > 0");
  pre_f (0);
  expect (STDC_CONTRACT_POST, "post_f", 36, "r > 0");
  post_f (0);
  expect (STDC_CONTRACT_ASSERT, "assert_f", 37, "x > 0");
  assert_f (0);
  expect (STDC_CONTRACT_MANUAL, "main", __LINE__ + 1, "manual comment");
  stdc_handle_observed_contract_violation ("manual comment");
  if (count != 4)
    __builtin_abort ();
  return 0;
}
