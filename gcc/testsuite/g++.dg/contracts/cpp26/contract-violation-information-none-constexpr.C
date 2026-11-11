// -fcontract-violation-information=none leaves compile-time diagnostics
// alone: a contract violated during constant evaluation is still diagnosed
// at the assertion, with its user message.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3099 -fcontract-evaluation-semantic=enforce" }
// { dg-additional-options "-fcontract-violation-information=none" }

constexpr int
pre_f (int x)
  pre (x > 0, "pre message")	// { dg-error "contract predicate is false in constant expression .pre message." }
{
  return x;
}

constexpr int
assert_f (int x)
{
  contract_assert (x > 0, "assert message");	// { dg-error "contract predicate is false in constant expression .assert message." }
  return x;
}

constexpr int a = pre_f (0);
constexpr int b = assert_f (0);
