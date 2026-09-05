// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

// The extension spelling "__contract_assert" and the standard spelling
// "contract_assert" both tokenize to RID_CONTASSERT (see c-common.cc).
// grok_contract must accept both; previously "__contract_assert" was not
// recognized there and triggered an internal compiler error (GCC-29, which
// still reproduces on stock g++ trunk).

void f (int x)
{
  contract_assert (x > 0);
  __contract_assert (x > 0);
}

// The paths below reach grok_contract differently from a plain function body,
// and had no coverage until the audit of 2026-09-05.  The original ICE was an
// unhandled case at the bottom of grok_contract, so anything that arrives
// there by another route is worth pinning.

// In a template, where the assertion is grokked once for the pattern and
// again for each instantiation.
template <typename T>
void tmpl (T x)
{
  __contract_assert (x > 0);
}

template void tmpl<int> (int);

// Inside a lambda body.
void in_lambda (int x)
{
  auto l = [x] { __contract_assert (x > 0); };
  l ();
}

// Inside a lambda that is itself written in a contract predicate, which is
// the most indirect route to grok_contract in the front end.  Both with and
// without a capture named by the nested assert: adding this case is what
// found the second layer of GCC-31, where the nested assert's reference to a
// capture never reached the capture machinery.
int g_alt_spelling_n = 0;

void in_predicate_lambda (int x)
  pre ([] { __contract_assert (g_alt_spelling_n >= 0); return true; } ())
{
  (void) x;
}

void in_predicate_lambda_capture (int x)
  pre ([x] { __contract_assert (x >= 0); return x > 0; } ())
{
}

// Mixed spellings on one function, to be sure recognising one does not
// disturb the other.
void mixed (int x)
{
  contract_assert (x > 0);
  __contract_assert (x > 0);
  contract_assert (x != 0);
}
