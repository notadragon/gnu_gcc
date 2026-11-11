/* Under -fcontract-violation-information=full, the default, the object
   holds the text of a C contract violation: each string
   c-violation-information-none-asm.c requires to be absent under =none is
   present here, so that test cannot pass for want of the text.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe -fcontract-violation-information=full" } */
/* Debug information names the function, its parameters and the file.  */
/* { dg-skip-if "" { *-*-* } { "-g*" } { "" } } */

#include <contracts.h>

enum { vi_post_token_value = 1 };

#line 1 "vi_file_name_token.c"
int vi_pre_function_token (int vi_pre_token)
  _Pre (vi_pre_token > 0)
  _Post (r : r != -vi_post_token_value)
{
  _ContractAssert (vi_pre_token != 7);
  return vi_pre_token;
}

void
vi_manual_function_token (void)
{
  stdc_handle_observed_contract_violation ("vi_manual_comment_token");
}

/* { dg-final { scan-assembler "vi_pre_token" } } */
/* { dg-final { scan-assembler "vi_post_token_value" } } */
/* { dg-final { scan-assembler "vi_manual_comment_token" } } */
/* { dg-final { scan-assembler "vi_file_name_token" } } */
/* { dg-final { scan-assembler "\"vi_\[a-z_\]*_function_token\"" } } */
