// With the ignore semantic a contract predicate is not evaluated, so one
// that would exit via an exception during constant evaluation causes no
// violation and no diagnostic, and the enclosing expression is constant.
// GCC-1006 (PR128543).
// { dg-do compile { target c++26 } }
// { dg-require-effective-target exceptions_enabled }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=ignore" }

constexpr bool
thr ()
{
  throw 1;
  return true;
}

constexpr bool
g ()
  pre ((throw 1, true))
  post ((throw 1, true))
{
  contract_assert ((throw 1, true));
  contract_assert (thr ());
  return true;
}

static_assert (g ());
