// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

// The extension spelling "__contract_assert" and the standard spelling
// "contract_assert" both tokenize to RID_CONTASSERT (see c-common.cc).
// grok_contract must accept both; previously "__contract_assert" was not
// recognized there and triggered an internal compiler error.

void f (int x)
{
  contract_assert (x > 0);
  __contract_assert (x > 0);
}
