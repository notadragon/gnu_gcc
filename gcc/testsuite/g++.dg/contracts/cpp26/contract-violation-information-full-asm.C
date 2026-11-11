// Under -fcontract-violation-information=full, the default, the object holds
// the text of a contract-violation record: each string
// contract-violation-information-none-asm.C requires to be absent under
// =none is present here, so that test cannot pass for want of the text.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3850 -fcontract-evaluation-semantic=observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero:observe" }
// { dg-additional-options "-fcontract-violation-information=full" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#define __STDC_WANT_ASSERT_USES_CONTRACTS__ 1
#include <cassert>
#include <contracts>

struct vi_label_t
{
  using assertion_control_object = vi_label_t;
  constexpr const char *compute_comment (const char *) const
  { return "vi_computed_comment_token"; }
  constexpr const char *compute_message (const char *) const
  { return "vi_computed_message_token"; }
};
constexpr vi_label_t vi_label{};

#line 1 "vi_file_name_token.C"
int vi_pre_function_token (int vi_pre_token)
  pre (vi_pre_token > 0, "vi_message_token")
  post (r : r != -1)
{
  contract_assert (vi_pre_token != 7);
  return vi_pre_token;
}
void vi_label_function_token (int x) pre<vi_label> (x > 0) { }
int vi_div_function_token (int a, int b) { return a / b; }
void vi_assert_function_token (int vi_assert_token)
{
  assert (vi_assert_token > 0);
}

// { dg-final { scan-assembler "vi_pre_token" } }
// { dg-final { scan-assembler "vi_assert_token" } }
// { dg-final { scan-assembler "vi_message_token" } }
// { dg-final { scan-assembler "vi_computed_comment_token" } }
// { dg-final { scan-assembler "vi_computed_message_token" } }
// { dg-final { scan-assembler "integer division by zero" } }
// { dg-final { scan-assembler "vi_file_name_token" } }
// { dg-final { scan-assembler "\[a-z\] vi_\[a-z_\]*_function_token" } }
