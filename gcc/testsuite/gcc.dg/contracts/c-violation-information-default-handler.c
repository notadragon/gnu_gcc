/* Under -fcontract-violation-information=none the default violation
   handler prints no empty location parts and no empty comment: no
   " in function ", no " at :0" and no ": ".  A violation that names only a
   line, which only a caller of the explicit P3290 API can make, prints
   " at line N".

   Mirror, in the llvm_llvm-project fork:
   clang/test/Contracts/C/c-violation-information-default-handler.c.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe -fcontract-violation-information=none" } */

#include <contracts.h>

void pre_f (int x) _Pre (x > 0) { }
void assert_f (int x) { _ContractAssert (x > 0); }

int
main (void)
{
  pre_f (0);
  assert_f (0);
  stdc_handle_observed_contract_violation_explicit ("only a line", "", "", 7);
  return 0;
}

/* { dg-output "^contract violation(\n|\r\n|\r)contract violation(\n|\r\n|\r)contract violation at line 7: only a line(\n|\r\n|\r)$" } */
