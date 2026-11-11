/* Under -fcontract-violation-information=none the object holds none of the
   text a C contract violation would carry: not the predicate's source text,
   not the function name, not the file name (here a distinctive #line name,
   since the .file directive names the real file), and not what the P3290
   convenience macros of <contracts.h> would pass.
   c-violation-information-full-asm.c finds each of them under =full.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe -fcontract-violation-information=none" } */
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

/* { dg-final { scan-assembler-not "vi_pre_token" } } */
/* { dg-final { scan-assembler-not "vi_post_token_value" } } */
/* { dg-final { scan-assembler-not "vi_manual_comment_token" } } */
/* { dg-final { scan-assembler-not "vi_file_name_token" } } */
/* { dg-final { scan-assembler-not "\"vi_\[a-z_\]*_function_token\"" } } */
