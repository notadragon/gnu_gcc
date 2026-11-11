// Implementation unit of contract-p4451-impl-p3400_a.C: the interface's
// import of std precedes the assertion-control-specifier.
// { dg-additional-options "-fmodules -fcontracts-p3400" }

module p4451m_p3400;

static int g (int x) pre<terminating> (x > 0) { return x; }

int f (int x)
{
  contract_assert<terminating> (x > 0);
  return g (x);
}
