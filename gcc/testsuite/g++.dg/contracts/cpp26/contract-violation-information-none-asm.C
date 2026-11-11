// Under -fcontract-violation-information=none the object holds none of the
// text a contract-violation record would carry: not the predicate's source
// text, not the user message, not a label's computed comment or message, not
// an implicit assertion's comment, not the function name and not the file
// name (here a distinctive #line name, since the .file directive names the
// real file).  contract-violation-information-full-asm.C finds each of them
// under =full.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3850 -fcontract-evaluation-semantic=observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero:observe" }
// { dg-additional-options "-fcontract-violation-information=none" }
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

// { dg-final { scan-assembler-not "vi_pre_token" } }
// { dg-final { scan-assembler-not "vi_assert_token" } }
// { dg-final { scan-assembler-not "vi_message_token" } }
// { dg-final { scan-assembler-not "vi_computed_comment_token" } }
// { dg-final { scan-assembler-not "vi_computed_message_token" } }
// { dg-final { scan-assembler-not "integer division by zero" } }
// { dg-final { scan-assembler-not "vi_file_name_token" } }
// { dg-final { scan-assembler-not "\[a-z\] vi_\[a-z_\]*_function_token" } }
