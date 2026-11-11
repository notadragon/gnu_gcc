/* -fcontract-violation-information=none: a C contract violation carries an
   empty file name, an empty function name, line 0, column 0 and an empty
   comment -- for a precondition, a postcondition and a contract assertion --
   and so do the P3290 convenience macros of <contracts.h>, which still
   evaluate their comment argument.  The kind and semantic are unchanged,
   and the compiler predefines __gcc_contracts_violation_information_none.
   c-violation-information-full.c is the same program under =full.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe -fcontract-violation-information=none" } */

#ifndef __gcc_contracts_violation_information_none
#error "__gcc_contracts_violation_information_none is not defined"
#endif

#include <contracts.h>

static int count;
static int expected_kind;

void
handle_contract_violation (const contract_violation_t *cv)
{
  if (stdc_contract_violation_file (cv)[0] != '\0'
      || stdc_contract_violation_function (cv)[0] != '\0'
      || stdc_contract_violation_line (cv) != 0
      || stdc_contract_violation_column (cv) != 0
      || stdc_contract_violation_comment (cv)[0] != '\0'
      || stdc_contract_violation_kind (cv) != expected_kind
      || stdc_contract_violation_semantic (cv) != STDC_CONTRACT_OBSERVE)
    __builtin_abort ();
  ++count;
}

int pre_f (int x) _Pre (x > 0) { return x; }
int post_f (int x) _Post (r : r > 0) { return x; }
void assert_f (int x) { _ContractAssert (x > 0); }

int
main (void)
{
  int evaluated = 0;
  expected_kind = STDC_CONTRACT_PRE;
  pre_f (0);
  expected_kind = STDC_CONTRACT_POST;
  post_f (0);
  expected_kind = STDC_CONTRACT_ASSERT;
  assert_f (0);
  expected_kind = STDC_CONTRACT_MANUAL;
  stdc_handle_observed_contract_violation ((++evaluated, "manual comment"));
  if (count != 4 || evaluated != 1)
    __builtin_abort ();
  return 0;
}
